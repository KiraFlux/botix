// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// Botix - https://github.com/KiraFlux/botix

#pragma once

#include <kf/core.hpp>
#include <kf/math.hpp>
#include <kf/units.hpp>

#include <kf/mixin/Configured.hpp>
#include <kf/mixin/DefaultResettable.hpp>

#include "botix/IncomingTelemetry.hpp"

#include "botix/service/Service.hpp"

namespace botix::internal {

struct MixerServiceConfig : kf::mixin::DefaultResettable<MixerServiceConfig> {

    kf::units::Milliseconds max_control_input_age_ms{100};

    enum class Mode : kf::u8 {
        Direct = 0x00,
        Tank = 0x01,
    } mode{Mode::Tank};

    kf::i8
        motor_left_sign{+1},
        motor_right_sign{+1};
};

}// namespace botix::internal

namespace botix::service {

struct MixerService :

    Service<MixerService>,
    kf::mixin::Configured<internal::MixerServiceConfig>

{
    using Self = MixerService;

    using Config = internal::MixerServiceConfig;

    struct Output : kf::mixin::DefaultResettable<Output> {
        using ValueType = kf::i16;

        ValueType
            motor_left_set{0},
            motor_right_set{0},
            servo_claw_set{0},
            servo_arm_set{0};

        [[nodiscard]] constexpr bool moving() const noexcept {
            return (motor_left_set != 0) and (motor_right_set != 0);
        }
    };

    struct Dependencies {
        Config const &config;
        IncomingTelemetry::ControlInputEntry const &control_input;
    };

    explicit constexpr MixerService(Dependencies const &deps) noexcept :
        kf::mixin::Configured<Config>{deps.config},
        _control_input{deps.control_input} {}

    constexpr Output const &output() const noexcept {
        return _output;
    }

private:
    IncomingTelemetry::ControlInputEntry const &_control_input;
    kf::units::Milliseconds _last_age{};
    Output _output{};

    BOTIX_IMPL_SERVICE(Self);
    void pollImpl(kf::units::Milliseconds now) noexcept {
        auto const age = _control_input.age(now);

        if (age > this->config().max_control_input_age_ms) {
            _output.reset();
            _last_age = age;
            return;
        }

        if (age != _last_age) {
            _last_age = age;
            auto const &input = _control_input.value();

            switch (this->config().mode) {
                case Config::Mode::Tank:
                    _output.motor_left_set = input.z_axis + input.r_axis;
                    _output.motor_right_set = input.z_axis - input.r_axis;
                    _output.servo_arm_set = input.x_axis;
                    _output.servo_claw_set = input.y_axis;
                    break;

                case Config::Mode::Direct:
                default:
                    _output.motor_left_set = input.z_axis;
                    _output.motor_right_set = input.x_axis;
                    _output.servo_arm_set = input.r_axis;
                    _output.servo_claw_set = input.y_axis;
                    break;
            }

            _output.motor_left_set *= this->config().motor_left_sign;
            _output.motor_right_set *= this->config().motor_right_sign;
        }
    }
};

}// namespace botix::service