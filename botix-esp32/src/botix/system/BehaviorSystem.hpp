// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// Botix - https://github.com/KiraFlux/botix

#pragma once

#include <kf/Arena.hpp>
#include <kf/units.hpp>

#include "botix/behavior/Kind.hpp"
#include "botix/behavior/Link.hpp"
#include "botix/behavior/Registry.hpp"
#include "botix/cli/Group.hpp"

#include "botix/system/System.hpp"

namespace botix::system {

struct BehaviorSystem : System<BehaviorSystem> {

    explicit constexpr BehaviorSystem(behavior::Registry::Dependencies deps) noexcept :
        System<BehaviorSystem>{{.name{"behavior"}}},
        _registry{deps} {}

    [[nodiscard]] constexpr auto &get(behavior::Kind kind) noexcept {
        return _registry.get(kind);
    }

private:
    behavior::Registry _registry;

public:
    behavior::Link link{_registry.operational};

private:
    BOTIX_IMPL_SYSTEM(BehaviorSystem);

    void onSetupImpl() noexcept {}

    void setupCliImpl(kf::Arena &arena, cli::Group &group) noexcept {}

    void pollImpl(kf::units::Milliseconds now) noexcept {
        link.poll(now);
    }
};

}// namespace botix::system