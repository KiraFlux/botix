// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// Botix - https://github.com/KiraFlux/botix

#pragma once

#include <kf/units.hpp>

#include <kf/mixin/NonCopyable.hpp>
#include <kf/mixin/Poll.hpp>

#include "botix/behavior/Behavior.hpp"

namespace botix::behavior {

struct Link :

    kf::mixin::NonCopyable,
    kf::mixin::Poll<Link>

{

    explicit constexpr Link(Behavior &default_behavior) noexcept :
        _default_behavior{default_behavior} {}

    void set(Behavior &new_behavior) noexcept {
        _current_behavior->onQuit();
        new_behavior.onEnter();
        _current_behavior = &new_behavior;
    }

private:
    Behavior &_default_behavior;
    Behavior *_current_behavior{&_default_behavior};

    KF_IMPL_POLL(Link);
    void pollImpl(kf::units::Milliseconds now) noexcept {
        auto const request = _current_behavior->onPoll(now);

        if (request == Behavior::Request::Quit) {
            set(_default_behavior);
        }
    }
};

}// namespace botix::behavior