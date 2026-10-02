// Copyright (c) 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <kf/Slice.hpp>
#include <kf/Timer.hpp>
#include <kf/core.hpp>
#include <kf/units.hpp>

#include <kf/mixin/Callbacked.hpp>
#include <kf/mixin/Configured.hpp>
#include <kf/mixin/DefaultResettable.hpp>
#include <kf/mixin/NonCopyable.hpp>
#include <kf/mixin/Poll.hpp>

namespace botix {

namespace internal {

struct OutgoingTelemetryTopicConfig : kf::mixin::DefaultResettable<OutgoingTelemetryTopicConfig> {

    kf::Timer::Config timer{.value = 100};
    kf::u32 update_ahead_ms{10};
    bool enabled{true};
};

struct OutgoingTelemetryConfig : kf::mixin::DefaultResettable<OutgoingTelemetryConfig> {

    OutgoingTelemetryTopicConfig
        wheel_distance{},
        obstacle_distance{
            .timer{.value{(1000 / 5)}},
            .enabled{false},
        };
};

}// namespace internal

struct OutgoingTelemetry :

    kf::mixin::NonCopyable,
    kf::mixin::Configured<internal::OutgoingTelemetryConfig>,
    kf::mixin::Poll<OutgoingTelemetry>

{
    using Config = internal::OutgoingTelemetryConfig;

    template<kf::trivial T> struct Topic :

        kf::mixin::NonCopyable,
        kf::mixin::Poll<Topic<T>>,
        kf::mixin::Callbacked<T()>,
        kf::mixin::Configured<internal::OutgoingTelemetryTopicConfig>

    {
        using Config = internal::OutgoingTelemetryTopicConfig;

        using kf::mixin::Configured<Config>::Configured;

        /// @brief Time mark when value was updated
        [[nodiscard]] constexpr auto timestamp() const noexcept {
            return _timestamp;
        }

        [[nodiscard]] constexpr T const &value() const noexcept {
            return _value;
        }

        [[nodiscard]] bool ready(kf::units::Milliseconds now) noexcept {
            if (not this->config().enabled) {
                return false;
            }

            if (_timer.expired(now)) {
                _timer.start(now);
                return true;
            }

            return false;
        }

    private:
        kf::Timer _timer{this->config().timer};
        T _value{};
        kf::units::Milliseconds _timestamp{};

        KF_IMPL_POLL(Topic<T>);
        void pollImpl(kf::units::Milliseconds now) noexcept {
            if (not this->config().enabled or (_timer.remaining(now) >= this->config().update_ahead_ms)) {
                return;
            }

            if (auto const maybe_value = this->invoke(); maybe_value.isSome()) {
                _value = maybe_value.unwrap();
                _timestamp = now;
            }
        }
    };

    using kf::mixin::Configured<Config>::Configured;

    struct WheelDistance {
        kf::f64 left_mm, right_mm;
    };

    Topic<WheelDistance> wheel_distance{this->config().wheel_distance};

    struct ObstacleDistance {
        using Unit = kf::u16;

        kf::Slice<Unit const> distances_mm;
        Unit min_distance_mm, max_distance_mm;
    };

    Topic<ObstacleDistance> obstacle_distance{this->config().obstacle_distance};

private:
    KF_IMPL_POLL(OutgoingTelemetry);
    void pollImpl(kf::units::Milliseconds now) noexcept {
        wheel_distance.poll(now);
        obstacle_distance.poll(now);
    }
};

}// namespace botix