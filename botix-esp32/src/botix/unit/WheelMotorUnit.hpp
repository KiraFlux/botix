// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// Botix - https://github.com/KiraFlux/botix

#pragma once

#include <kf/driver/actuator/DRV8871.hpp>
#include <kf/gpio.hpp>// used for DriverSpecificConfig

#include "botix/unit/Kind.hpp"
#include "botix/unit/Unit.hpp"

namespace botix::unit {

/// @brief Motor Actuator Unit
/// @note using Toolkit's DRV8871 as driver
struct WheelMotorUnit : Unit {

    using Driver = kf::driver::actuator::DRV8871;

    // TODO: update toolkit's

    struct DriverSpecificConfig {
        kf::gpio::GpioNumber gpio_num_forward, gpio_num_backward;
    };

    struct DriverDependencies {
        Driver::Config const &common_config;
        DriverSpecificConfig specific_config;
    };

    struct Dependencies {
        DriverDependencies const &driver;
        Index index;
    };

    WheelMotorUnit(Dependencies const &deps) noexcept :
        Unit{Kind::WheelMotor, deps.index},
        _driver{deps.driver.common_config, deps.driver.specific_config.gpio_num_forward, deps.driver.specific_config.gpio_num_backward} {}

    // TODO: move to ActuatorDriverUnit

    void set(Driver::Config::InputType input) noexcept {
        if (state() == State::Running) {
            _driver.set(input);
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