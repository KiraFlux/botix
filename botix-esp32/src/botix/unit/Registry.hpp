// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// Botix - https://github.com/KiraFlux/botix

#pragma once

#include <kf/Option.hpp>
#include <kf/core.hpp>
#include <kf/gpio.hpp>

#include <kf/mixin/DefaultResettable.hpp>
#include <kf/mixin/NonCopyable.hpp>

#include "botix/unit/Group.hpp"
#include "botix/unit/Kind.hpp"
#include "botix/unit/LidarUnit.hpp"
#include "botix/unit/ServoUnit.hpp"
#include "botix/unit/Unit.hpp"
#include "botix/unit/WheelEncoderUnit.hpp"
#include "botix/unit/WheelMotorUnit.hpp"

namespace botix::unit {

/// @brief Central object that owns all control units of the Botix robot
struct Registry : kf::mixin::NonCopyable {

    // TODO: Mount as enum wrapper: (enum + usize about max value)

    enum class SideMount : kf::usize {
        Left = 0,
        Right = 1,
    };

    enum class ManipulatorMount : kf::usize {
        Claw = 0,
        Arm = 1,
    };

    // TODO: merge WheelMotor and WheelEncoder units into WheelUnit

    using WheelMotorGroup = Group<WheelMotorUnit, SideMount, 2>;

    using WheelEncoderGroup = Group<WheelEncoderUnit, SideMount, 2>;

    // TODO: maybe just one manipulator unit ?
    using ServoGroup = Group<ServoUnit, ManipulatorMount, 2>;

    /// @brief Configuration aggregate for all hardware peripherals of the Botix robot
    struct Config : kf::mixin::DefaultResettable<Config> {
        // TODO: gpio num config

        WheelMotorUnit::Driver::Config wheel_motor{
            .pwm{
                .frequency_hz = 20000,
                .resolution_bits = 8,
            },
            .max_input = 1000,
            .duty_dead_zone = 10,
        };

        ServoUnit::Driver::Config servo{
            .pwm{
                .frequency_hz = 50,
                .resolution_bits = 12,
            },
            .angle_range{.start = 0, .end = 180},
            .pulse_range{.start = 500, .end = 2500},
        };

        WheelEncoderUnit::Driver::Config wheel_encoder{
            .units_per_tick = 1,
            .pull = kf::gpio::DigitalInput::Pull::External,
        };

        LidarUnit::Driver::Config lidar{};
    };

    explicit Registry(Config const &config) noexcept :

        // TODO: roll to loop.
        wheel_motor{.units{.items{
            {{
                .driver{
                    .common_config = config.wheel_motor,
                    .specific_config{
                        .gpio_num_forward = kf::gpio::G32,
                        .gpio_num_backward = kf::gpio::G33,
                    },
                },
                .index = 0,
            }},
            {{
                .driver{
                    .common_config = config.wheel_motor,
                    .specific_config{
                        .gpio_num_forward = kf::gpio::G25,
                        .gpio_num_backward = kf::gpio::G26,
                    },
                },
                .index = 1,
            }},
        }}},
        servo{.units{.items{
            {{
                .driver{
                    .common_config = config.servo,
                    .specific_config{
                        .gpio_num = kf::gpio::G13,
                        .angle_range{
                            .start = 0,
                            .end = 180,
                        },
                    },
                },
                .index = 0,
            }},
            {{
                .driver{
                    .common_config = config.servo,
                    .specific_config{
                        .gpio_num = kf::gpio::G14,
                        .angle_range{
                            .start = 135,
                            .end = 180,
                        },
                    },
                },
                .index = 1,
            }},
        }}},
        wheel_encoder{.units{.items{
            {{
                .driver{
                    .common_config = config.wheel_encoder,
                    .specific_config{
                        .gpio_num_phase_a = kf::gpio::G36,
                        .gpio_num_phase_b = kf::gpio::G39,
                    },
                },
                .index = 0,
            }},
            {{
                .driver{
                    .common_config = config.wheel_encoder,
                    .specific_config{
                        .gpio_num_phase_a = kf::gpio::G34,
                        .gpio_num_phase_b = kf::gpio::G35,
                    },
                },
                .index = 1,
            }},
        }}},
        lidar{{
            .driver{
                .common_config = config.lidar,
                .specific_config{
                    .uart_num = 2,
                    .rx_gpio_num = kf::gpio::G16,
                    .tx_gpio_num = kf::gpio::G17,
                },
            },
            .index = 0,
        }}

    {}

    [[nodiscard]] auto get(Kind kind, Unit::Index index) noexcept -> kf::Option<Unit &> {
        switch (kind) {
            case Kind::WheelMotor: return wheel_motor.at(index);
            case Kind::Servo: return servo.at(index);
            case Kind::Lidar: return (0 == index) ? kf::someRef<Unit &>(lidar) : kf::none;
            case Kind::WheelEncoder: return wheel_encoder.at(index);
            default: return kf::none;
        }
    }

    WheelMotorGroup wheel_motor;

    ServoGroup servo;

    WheelEncoderGroup wheel_encoder;

    LidarUnit lidar;
};

}// namespace botix::unit