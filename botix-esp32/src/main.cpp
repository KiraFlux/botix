// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// Botix - https://github.com/KiraFlux/botix

#include <kf/main.hpp>
#include <kf/rtos/Clock.hpp>
#include <kf/rtos/Task.hpp>

#include "botix/OutgoingTelemetry.hpp"
#include "botix/cli/Channel.hpp"
#include "botix/cli/Console.hpp"
#include "botix/protocol/Kind.hpp"
#include "botix/transport/Address.hpp"

#include "botix/system/BehaviorSystem.hpp"
#include "botix/system/ConfigSystem.hpp"
#include "botix/system/ProtocolSystem.hpp"
#include "botix/system/TelemetrySystem.hpp"
#include "botix/system/TransportSystem.hpp"
#include "botix/system/UnitSystem.hpp"

void kf::main(kf::Init &init) {
    init.logger.debug("Starting");

    // system instances

    static botix::system::ConfigSystem system_config{};

    static botix::cli::Channel console_channels[2]{
        botix::cli::Channel{
            init.arena,
            system_config.device.cli,
            {.echo = true},
        },
        botix::cli::Channel{
            init.arena,
            system_config.device.cli,
            {.echo = false},
        },
    };

    static botix::cli::Console console{init.arena, system_config.device.cli, console_channels};

    // setup config before other system could use config some values as one-shot constructor values
    system_config.setup(init.arena, console);

    auto &main_channel = console_channels[0];
    auto &other_channel = console_channels[1];

    static botix::system::UnitSystem system_unit{{
        .device_config = system_config.device,
        .user_config = system_config.user,
    }};

    static botix::system::TelemetrySystem system_telemetry{{
        .config = system_config.device,
    }};

    static botix::system::TransportSystem system_transport{{
        .device_config = system_config.device,
        .user_config = system_config.user,
    }};

    static botix::system::ProtocolSystem system_protocol{{
        .device_config = system_config.device,
        .user_config = system_config.user,
        .transport_link = system_transport.link,
        .outgoing_telemetry = system_telemetry.outgoing,
        .cli_channel_output = other_channel.output,
    }};

    static botix::system::BehaviorSystem system_behavior{{
        .operational{
            .config = system_config.user.operational_behavior,
            .mixer_service{
                .config = system_config.device.mixer_service,
                .control_input = system_telemetry.incoming.control_input,
            },
            .wheel_motor_left = system_unit.registry.wheel_motor.get(botix::unit::Registry::SideMount::Left),
            .wheel_motor_right = system_unit.registry.wheel_motor.get(botix::unit::Registry::SideMount::Right),
            .servo_claw = system_unit.registry.servo.get(botix::unit::Registry::ManipulatorMount::Claw),
            .servo_arm = system_unit.registry.servo.get(botix::unit::Registry::ManipulatorMount::Arm),
            .lidar = system_unit.registry.lidar,
        },
    }};

    // system initialization

    // hardware

    system_unit.setup(init.arena, console);

    // telemetry

    system_telemetry.setup(init.arena, console);

    system_telemetry.outgoing.wheel_distance.callback([]() -> botix::OutgoingTelemetry::WheelDistance {
        // TODO: generalize just to loop
        return {
            .left_mm = system_unit.registry.wheel_encoder.get(botix::unit::Registry::SideMount::Left).distance(),
            .right_mm = system_unit.registry.wheel_encoder.get(botix::unit::Registry::SideMount::Right).distance(),
        };
    });

    system_telemetry.outgoing.obstacle_distance.callback([]() {
        auto &lidar_unit = system_unit.registry.lidar;

        botix::OutgoingTelemetry::ObstacleDistance ret{
            .distances_mm{},
            .min_distance_mm = lidar_unit.config().min_distance_mm,
            .max_distance_mm = lidar_unit.config().max_distance_mm,
        };

        if (lidar_unit.state() == botix::unit::Unit::State::Running) {
            ret.distances_mm = lidar_unit.read().distances_mm.slice();
        }

        return ret;
    });

    // transport

    system_transport.setup(init.arena, console);

    system_transport.onReceive([&other_channel](auto const &context) -> void {
        system_protocol.link.receive({
            .transport = context,
            .incoming_telemetry = system_telemetry.incoming,
            .cli_channel_input = other_channel.input,
            .timestamp = kf::rtos::Clock::now(),
        });
    });

    system_transport.onReceiveForeign([&init](auto const &context) -> void {
        init.logger.debug("Found device");

        if (system_transport.link.connected()) {
            init.logger.error("connect denied (already connected)");
            return;
        }

        if (not system_transport.link.connect(context.address)) {
            init.logger.error("system_transport.link.connect failed");
        }
    });

    // protocol

    system_protocol.setup(init.arena, console);

    system_protocol.onRawFallback([&init](botix::transport::Address const &address, auto buffer) -> void {
        (void) address;
        init.logger.debug("raw (fallback): got {} bytes", buffer.length());
    });

    system_protocol.onMavlinkFallback([&init](botix::transport::Address const &address, auto const &message) -> void {
        (void) address;
        init.logger.debug("mavlink (fallback): msg id: {}, seq: {}", message.msgid, message.seq);
    });

    // behavior

    system_behavior.setup(init.arena, console);

    init.logger.info("Ready");

    // loop

    while (true) {
        constexpr auto loop_period{1000 / 100};

        auto const to_read = init.io.availableForRead();
        for (int i = 0; i < to_read; i += 1) {
            if (auto const read = init.io.readByte(); read.isOk()) {
                (void) main_channel.input.feed({reinterpret_cast<char const *>(&read.ok()), 1});
            }
        }

        if (main_channel.output.availableForRead() > 0) {
            auto const str = main_channel.output.drain();
            (void) init.io.writeBuffer({reinterpret_cast<kf::u8 const *>(str.data()), str.length()});
        }

        auto const now = rtos::Clock::now();

        system_unit.poll(now);
        system_telemetry.poll(now);
        system_transport.poll(now);
        console.poll(now);
        system_protocol.poll(now);
        system_behavior.poll(now);
        system_config.poll(now);

        rtos::Task::sleep(loop_period);
    }
}
