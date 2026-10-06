// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// Botix - https://github.com/KiraFlux/botix

#pragma once

#include <kf/driver/actuator/PwmPositionServo.hpp>
#include <kf/gpio.hpp>// used for DriverSpecificConfig
#include <kf/units.hpp>

#include "botix/unit/Kind.hpp"
#include "botix/unit/Unit.hpp"

namespace botix::unit {

/// @brief Servo Unit
/// @note uses MG90S servo driver
struct ServoUnit : Unit {

    using Driver = kf::driver::actuator::PwmPositionServo;

    // TODO: update toolkit's

    struct DriverSpecificConfig {
        kf::gpio::GpioNumber gpio_num;
        Driver::Config::AngleRange angle_range;
    };

    struct DriverDependencies {
        Driver::Config const &common_config;
        DriverSpecificConfig specific_config;
    };

    struct Dependencies {
        DriverDependencies const &driver;
        Index index;
    };

    ServoUnit(Dependencies const &deps) noexcept :
        Unit{Kind::Servo, deps.index},
        _driver{deps.driver.common_config, deps.driver.specific_config.gpio_num, deps.driver.specific_config.angle_range} {}

    // TODO: move to ActuatorDriverUnit

    void set(kf::units::Degrees angle) noexcept {
        if (state() == State::Running) {
            _driver.set(angle);
        }
    }

    void stop() noexcept {
        if (state() == State::Running) {
            _driver.stop();
        }
    }

protected:
    bool onInit() noexcept override {
        if (state() == State::Idle) {
            return _driver.init();
        } else {
            return true;
        }
    }

    void onQuit() noexcept override {
        // _driver.quit(); TODO: implement
    }

private:
    Driver _driver;
};

}// namespace botix::unit