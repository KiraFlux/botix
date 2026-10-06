// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// Botix - https://github.com/KiraFlux/botix

#pragma once

#include <kf/core.hpp>
#include <kf/units.hpp>

#include <kf/mixin/NonCopyable.hpp>

namespace botix {

struct IncomingTelemetry :

    kf::mixin::NonCopyable

{
    template<kf::trivial T> struct Entry :

        kf::mixin::NonCopyable

    {
        [[nodiscard]] constexpr T const &value() const noexcept {
            return _value;
        }

        [[nodiscard]] constexpr kf::units::Milliseconds age(kf::units::Milliseconds now) const noexcept {
            return now - _last_update;
        }

        void update(T const &new_value, kf::units::Milliseconds now) noexcept {
            _value = new_value;
            _last_update = now;
        }

    private:
        T _value{};
        kf::units::Milliseconds _last_update{0};
    };

    /// @brief Raw data received from the remote controller
    struct ControlInput {
        using ValueType = kf::i16;

        static constexpr ValueType max_value{1000};// MAVLink MANUAL_CONTROL format

        /// @note DJC's Left X
        ValueType r_axis;

        /// @note DJC's Left Y
        ValueType z_axis;

        /// @note DJC's Right X
        ValueType y_axis;

        /// @note DJC's Right Y
        ValueType x_axis;
    };

    using ControlInputEntry = Entry<ControlInput>;

    constexpr IncomingTelemetry() noexcept = default;

    ControlInputEntry control_input{};
};

}// namespace botix