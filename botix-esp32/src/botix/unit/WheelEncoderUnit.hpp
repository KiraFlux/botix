// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// Botix - https://github.com/KiraFlux/botix

#pragma once

#include <kf/driver/sensor/QuadratureEncoder.hpp>
#include <kf/gpio.hpp>// used for DriverSpecificConfig
#include <kf/units.hpp>

#include "botix/unit/Kind.hpp"
#include "botix/unit/Unit.hpp"

namespace botix::unit {

/// @brief Wheel Encoder Unit
/// @note Uses QuadratureEncoder driver
struct WheelEncoderUnit : Unit {

    /// @details This alias configures the generic QuadratureEncoder to output linear wheel travel in millimeters.
    /// @note The conversion from encoder ticks to millimeters relies on the `units_per_tick` configuration,
    ///       which must reflect the entire kinematic chain (gear ratio, wheel circumference).
    using Driver = kf::driver::sensor::QuadratureEncoder<kf::units::Millimeters>;

    // TODO: update toolkit's

    struct DriverSpecificConfig {
        kf::gpio::GpioNumber gpio_num_phase_a, gpio_num_phase_b;
    };

    struct DriverDependencies {
        Driver::Config const &common_config;
        DriverSpecificConfig specific_config;
    };

    struct Dependencies {
        DriverDependencies const &driver;
        Index index;
    };

    WheelEncoderUnit(Dependencies const &deps) noexcept :
        Unit{Kind::WheelEncoder, deps.index},
        _driver{deps.driver.common_config, deps.driver.specific_config.gpio_num_phase_a, deps.driver.specific_config.gpio_num_phase_b} {}

    [[nodiscard]] auto distance() const noexcept {
        return _driver.positionUnits();
    }

    [[nodiscard]] auto ticks() const noexcept {
        return _driver.positionTicks();
    }

protected:
    bool onInit() noexcept override {
        // one shot
        if (state() == State::Idle) {
            _driver.init();
        }
        // TODO: remove setters like that. move reset to driver's init
        _driver.positionTicks(0);// reset position
        return true;
    }

    void onQuit() noexcept override {
        // _driver.quit(); TODO: implement
    }

private:
    Driver _driver;
};

}// namespace botix::unit