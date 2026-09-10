// Copyright (c) 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <kf/units.hpp>

#include "botix/service/MixerService.hpp"
#include "botix/unit/ServoUnit.hpp"
#include "botix/unit/WheelMotorUnit.hpp"
#include "botix/unit/LidarUnit.hpp"

#include "botix/behavior/Behavior.hpp"

namespace botix::behavior {

struct OperationalBehavior : Behavior {

    struct Config {
        unit::LidarUnit::Strategy 
            moving_lidar_strategy{unit::LidarUnit::Strategy::Nearest},
            stopped_lidar_strategy{unit::LidarUnit::Strategy::LPF};
    };

    struct Dependencies {
        Config const &config;

        service::MixerService::Dependencies mixer_service;

        unit::WheelMotorUnit
            &wheel_motor_left,
            &wheel_motor_right;

        unit::ServoUnit
            &servo_claw,
            &servo_arm;
        
        unit::LidarUnit &lidar;
    };

    explicit constexpr OperationalBehavior(Dependencies const &deps) noexcept :
        _deps{deps}, _mixer_service{deps.mixer_service} {}

    void onEnter() noexcept override {}

    void onQuit() noexcept override {
        _deps.wheel_motor_left.stop();
        _deps.wheel_motor_right.stop();
        _deps.servo_arm.stop();
        _deps.servo_claw.stop();
    }

    Request onPoll(kf::units::Milliseconds now) noexcept override {
        _mixer_service.poll(now);

        auto const &output = _mixer_service.output();

        _deps.lidar.parameters().strategy = output.moving() ? _deps.config.moving_lidar_strategy : _deps.config.stopped_lidar_strategy;

        _deps.wheel_motor_left.set(output.motor_left_set);
        _deps.wheel_motor_right.set(output.motor_right_set);
        _deps.servo_arm.set(output.servo_arm_set);
        _deps.servo_claw.set(output.servo_claw_set);

        return Request::Keep;
    }

private:
    Dependencies _deps;
    service::MixerService _mixer_service;// TODO: is this really a service? rest of firmware and other behavior actually will not reuse this mixer.. So extract here?
};

}// namespace botix::behavior