// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// Botix - https://github.com/KiraFlux/botix

#pragma once

#include <kf/mixin/NonCopyable.hpp>

#include "botix/behavior/Behavior.hpp"
#include "botix/behavior/Kind.hpp"
#include "botix/behavior/OperationalBehavior.hpp"

namespace botix::behavior {

struct Registry : kf::mixin::NonCopyable {

    struct Dependencies {
        OperationalBehavior::Dependencies operational;
    };

    explicit constexpr Registry(Dependencies const &deps) noexcept :
        operational{deps.operational} {}

    [[nodiscard]] constexpr Behavior &get(Kind kind) noexcept {
        (void) kind;
        return operational;
    }

    OperationalBehavior operational;
};

}// namespace botix::behavior