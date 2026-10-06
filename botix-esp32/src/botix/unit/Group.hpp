// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// Botix - https://github.com/KiraFlux/botix

#pragma once

#include <kf/Array.hpp>
#include <kf/Option.hpp>
#include <kf/core.hpp>

#include "botix/unit/Unit.hpp"

namespace botix::unit {

/// @brief Group of same units
/// @tparam UnitImpl Implementation class of unit
/// @tparam Mount Named index of mount (e.g. Left = 0, Right = 1)
/// @tparam N unit total count any `Mount` must be under this value
template<kf::implements<Unit> UnitImpl, kf::enum_type Mount, Unit::Index N> struct Group {

    static constexpr auto unit_total_count{N};

    [[nodiscard]] auto at(Unit::Index index) noexcept -> kf::Option<Unit &> {
        return (index < unit_total_count) ? kf::someRef<Unit &>(units[index]) : kf::none;
    }

    [[nodiscard]] decltype(auto) get(Mount mount) noexcept {
        return units[static_cast<Unit::Index>(mount)];
    }

    [[nodiscard]] decltype(auto) get(Mount mount) const noexcept {
        return units[static_cast<Unit::Index>(mount)];
    }

    kf::Array<UnitImpl, unit_total_count> units;
};

}// namespace botix::unit