// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// Botix - https://github.com/KiraFlux/botix

#pragma once

#include <kf/StringView.hpp>
#include <kf/core.hpp>

#include <kf/mixin/Initable.hpp>
#include <kf/mixin/NonCopyable.hpp>
#include <kf/mixin/Quitable.hpp>
#include <kf/mixin/ReprTo.hpp>

#include "botix/unit/Kind.hpp"

namespace botix::unit {

/// @brief Basic peripheral unit. A driver wrapper or some drivers logical composition
/// @note `init()` is idempotent and has effect only from not `Running` state
/// @note `quit()` is idempotent and has effect only from `Running` state
/// @note string representation format: `<name>#<index>` e.g. `my_unit#123`
struct Unit :

    kf::mixin::NonCopyable,
    kf::mixin::Initable<Unit, void()>,
    kf::mixin::Quitable<Unit>,
    kf::mixin::ReprTo<Unit>

{
    using Self = Unit;

    /// @brief Index number of unit
    using Index = kf::u8;

    enum class State : char {
        /// @brief Waiting to be first initialized
        /// @note Pre init() stage
        Idle,

        /// @brief Initialing failed
        /// @note May be solved by init() again.
        Failed,

        /// @brief Active
        /// @note Was successfully initialized
        Running,

        /// @brief Unit suspended
        /// @note May be re-init()
        Suspended,
    };

    explicit constexpr Unit(Kind kind, Index index) noexcept :
        _kind{kind}, _index{index} {}

protected:
    // dyn interface

    /// @brief Unit-specific init logic
    /// @return true if init is succeed
    /// @note called only from not `Running` state
    [[nodiscard]] virtual bool onInit() noexcept = 0;

    /// @brief Unit-specific quit logic
    /// @note called only from `Running` state
    virtual void onQuit() noexcept = 0;

public:
    // properties

    [[nodiscard]] constexpr auto kind() const noexcept { return _kind; }

    [[nodiscard]] constexpr auto index() const noexcept { return _index; }

    [[nodiscard]] constexpr auto state() const noexcept { return _state; }

    // TODO: actually, Kind should be a struct and implement Repr
    [[nodiscard]] constexpr kf::StringView name() const noexcept {
        switch (_kind) {
            case Kind::WheelMotor: return "wheel_motor";
            case Kind::Servo: return "servo";
            case Kind::Lidar: return "lidar";
            case Kind::WheelEncoder: return "wheel_encoder";
            default: return "?";
        }
    }

    // TODO: actually, State should be a struct and implement Repr
    [[nodiscard]] static constexpr kf::StringView stateName(State s) noexcept {
        switch (s) {
            case State::Idle: return "idle";
            case State::Failed: return "failed";
            case State::Running: return "running";
            case State::Suspended: return "suspended";
            default: return "?";
        }
    }

private:
    Kind const _kind;
    Index const _index;
    State _state{State::Idle};

    KF_IMPL_INITABLE(Self, void());
    void initImpl() noexcept {
        if (State::Running != _state) {
            _state = (onInit() ? State::Running : State::Failed);
        }
    }

    KF_IMPL_QUITABLE(Self);
    void quitImpl() noexcept {
        if (State::Running == _state) {
            onQuit();
            _state = State::Suspended;
        }
    }

    KF_IMPL_REPR_TO(Self);
    constexpr kf::usize reprToImpl(auto &char_writable) const noexcept {
        return (
            char_writable.append(name()) +
            char_writable.append('#') +
            char_writable.append(_index));
    }
};

}// namespace botix::unit