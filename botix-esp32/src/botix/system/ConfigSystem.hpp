// Copyright (c) 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <kf/Arena.hpp>
#include <kf/Bytes.hpp>
#include <kf/Timer.hpp>
#include <kf/core.hpp>
#include <kf/esp/NVS.hpp>
#include <kf/units.hpp>

#include "botix/cli/Argument.hpp"
#include "botix/cli/Group.hpp"
#include "botix/config/Config.hpp"
#include "botix/config/DeviceConfig.hpp"
#include "botix/config/Registry.hpp"
#include "botix/config/UserConfig.hpp"
#include "botix/protocol/Kind.hpp"
#include "botix/service/ConfigService.hpp"
#include "botix/transport/Kind.hpp"

#include "botix/system/System.hpp"

namespace botix::system {

struct ConfigSystem : System<ConfigSystem> {

    template<kf::implements<config::ConfigTag> ConfigImpl> struct Strategy {

        static void load(kf::Bytes bytes) noexcept {
            auto maybe_config = ConfigImpl::fromBytes(bytes);
            if (maybe_config.isNone()) { return; }

            auto &config = maybe_config.unwrap();
            if (config.version != ConfigImpl::latest_version) {
                config.reset();
            }
        }

        static void reset(kf::Bytes bytes) noexcept {
            if (auto maybe_config = ConfigImpl::fromBytes(bytes); maybe_config.isSome()) {
                maybe_config.unwrap().reset();
            }
        }

        static void init(service::ConfigService &service) noexcept {
            service.onLoad(load);
            service.resettingStrategy(reset);
            service.requestLoad();
            service.sync();
        }
    };

    constexpr ConfigSystem() noexcept :
        System<ConfigSystem>{{.name{"config"}}} {};

private:
    static constexpr kf::Timer::Config _sync_timer_config{.value = 10'000};

    kf::esp::NVS _nvs{"botix"};

public:
    config::DeviceConfig device{};
    config::UserConfig user{};

    service::ConfigService device_service{{
        .nvs = _nvs,
        .key = "dev-cfg",
        .sync_timer_config = _sync_timer_config,
        .config_bytes = device.bytes(),
    }};

    service::ConfigService user_service{{
        .nvs = _nvs,
        .key = "usr-cfg",
        .sync_timer_config = _sync_timer_config,
        .config_bytes = user.bytes(),
    }};

private:
    config::Registry::EnumItem const _registry_mixer_service_mode[2]{
        {"direct", service::MixerService::Config::Mode::Direct},
        {"tank", service::MixerService::Config::Mode::Tank},
    };

    config::Registry::EnumItem const _registry_transport_entries[2]{
        {"espnow", transport::Kind::Espnow},
        {"wifi", transport::Kind::Wifi},
    };

    config::Registry::EnumItem const _registry_protocol_entries[2]{
        {"raw", protocol::Kind::Raw},
        {"mavlink", protocol::Kind::Mavlink},
    };

    config::Registry::EnumItem const _registry_lidar_strategies[4]{
        {"pass", unit::LidarUnit::Strategy::Pass},
        {"nearest", unit::LidarUnit::Strategy::Nearest},
        {"furthest", unit::LidarUnit::Strategy::Furthest},
        {"lpf", unit::LidarUnit::Strategy::LPF},
    };

    config::Registry::Field _registry_fields[44]{
        // common (1)
        {"hostname", user.wifi_service.hostname},

        // boot (4)
        {"boot.transport", user.boot.transport, _registry_transport_entries},
        {"boot.protocol", user.boot.protocol, _registry_protocol_entries},
        {"boot.init_lidar", user.boot.init_lidar},
        {"boot.lidar.strategy", user.boot.lidar.strategy, _registry_lidar_strategies},

        // services

        // service: wifi (3)
        {"wifi.enabled", user.wifi_service.enabled},// TODO: move to boot
        {"wifi.ssid", user.wifi_service.ssid},
        {"wifi.password", user.wifi_service.password},

        // service: mixer (4)
        {"mixer.mode", device.mixer_service.mode, _registry_mixer_service_mode},
        {"mixer.max_age_ms", device.mixer_service.max_control_input_age_ms},
        {"mixer.left_sign", device.mixer_service.motor_left_sign},
        {"mixer.right_sign", device.mixer_service.motor_right_sign},

        // systems

        // behavior: operational
        {"op.move_ls", user.operational_behavior.moving_lidar_strategy, _registry_lidar_strategies},
        {"op.stop_ls", user.operational_behavior.stopped_lidar_strategy, _registry_lidar_strategies},

        // telemetry: wheel_distance (3)
        {"telem.wd.enabled", device.outgoing_telemetry.wheel_distance.enabled},// TODO: move to boot
        {"telem.wd.period_ms", device.outgoing_telemetry.wheel_distance.timer.value},
        {"telem.wd.ahead_ms", device.outgoing_telemetry.wheel_distance.update_ahead_ms},

        // telemetry: obstacle_distance (3)
        {"telem.od.enabled", device.outgoing_telemetry.obstacle_distance.enabled},// TODO: move to boot
        {"telem.od.period_ms", device.outgoing_telemetry.obstacle_distance.timer.value},
        {"telem.od.ahead_ms", device.outgoing_telemetry.obstacle_distance.update_ahead_ms},

        // transport: (3)
        {"udp.local_port", device.transport_registry.wifi_udp.local_port},
        {"udp.remote_port", device.transport_registry.wifi_udp.remote.port},
        {"udp.remote_ip", device.transport_registry.wifi_udp.remote.address},

        // protocol: mavlink (4)
        {"mavlink.heartbeat_period_ms", device.protocol_registry.mavlink.heartbeat_timer.value},
        {"mavlink.max_poll_msg", device.protocol_registry.mavlink.max_poll_messages},
        {"mavlink.self_id", device.protocol_registry.mavlink.system_id_self},
        {"mavlink.target_id", device.protocol_registry.mavlink.system_id_target},

        // driver:

        // driver: lidar (5)
        {"lidar.dist_min_mm", device.unit_registry.lidar.min_distance_mm},
        {"lidar.dist_max_mm", device.unit_registry.lidar.max_distance_mm},
        {"lidar.min_intensity", device.unit_registry.lidar.min_intensity},
        {"lidar.lpf_factor", device.unit_registry.lidar.low_pass_filter_factor},
        {"lidar.baudrate", device.unit_registry.lidar.uart.baudrate},
        {"lidar.rx_buffer_len", reinterpret_cast<kf::u32 &>(device.unit_registry.lidar.uart.rx_buffer_length)},

        // driver: wheel_encoder (1)
        {"wheel_encoder.mm_per_tick", device.unit_registry.wheel_encoder.units_per_tick},

        // driver: motor (4)
        {"wheel_motor.pwm_hz", device.unit_registry.wheel_motor.pwm.frequency_hz},
        {"wheel_motor.pwm_bits", device.unit_registry.wheel_motor.pwm.resolution_bits},
        {"wheel_motor.max_input", device.unit_registry.wheel_motor.max_input},
        {"wheel_motor.dead_zone", device.unit_registry.wheel_motor.duty_dead_zone},

        // driver: servo (6)
        {"servo.pwm_hz", device.unit_registry.servo.pwm.frequency_hz},
        {"servo.pwm_bits", device.unit_registry.servo.pwm.resolution_bits},
        {"servo.angle_min", device.unit_registry.servo.angle_range.start},
        {"servo.angle_max", device.unit_registry.servo.angle_range.end},
        {"servo.pulse_min", device.unit_registry.servo.pulse_range.start},
        {"servo.pulse_max", device.unit_registry.servo.pulse_range.end},
    };

    config::Registry _registry{_registry_fields};

    enum class ServiceKind {
        Device,
        User,
    };

    cli::Argument list_command_arguments[1]{
        {
            {.name{"path_hint"}},
            cli::Argument::String{
                .params{.default_value{""}},
            },
        },
    };

    cli::Argument::Enum::Item const config_service_kinds[2]{
        {{.name{"device"}}, ServiceKind::Device},
        {{.name{"user"}}, ServiceKind::User},
    };

    cli::Argument service_related_command_arguments[1]{
        {
            {.name{"target_service"}},
            cli::Argument::Enum{
                .items{config_service_kinds},
            },
        },
    };

    cli::Argument field_command_arguments[2]{
        {
            {.name{"path"}},
            cli::Argument::String{},
        },
        {
            {.name{"value"}},
            cli::Argument::String{
                .params{.default_value{""}},
            },
        },
    };

    [[nodiscard]] constexpr service::ConfigService &getService(ServiceKind kind) noexcept {
        switch (kind) {
            case ServiceKind::User:
                return user_service;

            case ServiceKind::Device:
            default:
                return device_service;
        }
    }

    BOTIX_IMPL_SYSTEM(ConfigSystem);

    void onSetupImpl() noexcept {
        if (_nvs.init().isError()) {
            this->_logger.error("NVS init failed");
        }

        Strategy<config::DeviceConfig>::init(device_service);
        Strategy<config::UserConfig>::init(user_service);
    }

    void setupCliImpl(kf::Arena &arena, cli::Group &group) noexcept {
        (void) group.addCommand(arena, {.name{"list"}}, list_command_arguments, [this](cli::Command::Context const &context) -> void {
            auto const path_hint = context.arguments[0].string();

            context.channel.output.print("Available fields: {}", path_hint);

            for (auto const &entry: _registry.all()) {
                if (path_hint.empty() or entry.name.contains(path_hint)) {
                    context.channel.output.print("{}", entry);
                }
            }
        });

        (void) group.addCommand(arena, {.name{"sync"}}, service_related_command_arguments, [this](cli::Command::Context const &context) -> void {
            auto const target_kind = context.arguments[0].enumValue<ServiceKind>();
            auto const target_name = context.arguments[0].enumName();

            getService(target_kind).sync();
            context.channel.output.print("sync completed for '{}' service", target_name);
        });

        (void) group.addCommand(arena, {.name{"reset"}}, service_related_command_arguments, [this](cli::Command::Context const &context) -> void {
            auto const target_kind = context.arguments[0].enumValue<ServiceKind>();
            auto const target_name = context.arguments[0].enumName();

            context.channel.output.print("reset requested for '{}' service", target_name);
            getService(target_kind).requestReset();
        });

        (void) group.addCommand(arena, {.name{"field"}}, field_command_arguments, [this](cli::Command::Context const &context) -> void {
            auto const path = context.arguments[0].string();
            auto const lexeme = context.arguments[1].string();

            if (not lexeme.empty()) {
                // set

                auto const result = _registry.set(path, lexeme);
                if (result.isOk()) {
                    context.channel.output.print("field updated");
                } else {
                    context.channel.output.error("set failed: {}", result.error().message);
                }
            }

            // get

            if (auto field = _registry.get(path); field.isSome()) {
                context.channel.output.print("{}", field.unwrap());
                return;
            }

            context.channel.output.error("field '{}' not found", path);
        });
    }

    void pollImpl(kf::units::Milliseconds now) noexcept {
        device_service.poll(now);
        user_service.poll(now);
    }
};

}// namespace botix::system