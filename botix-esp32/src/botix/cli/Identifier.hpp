// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// Botix - https://github.com/KiraFlux/botix

#pragma once

#include <kf/Option.hpp>
#include <kf/StringView.hpp>

#include <kf/mixin/Match.hpp>

namespace botix::cli {

struct Identifier :

    kf::mixin::Match<Identifier, kf::StringView>

{

    kf::StringView name, description{};
    kf::Option<char> shortcut{name.empty() ? kf::none : kf::some(name[0])};

private:
    KF_IMPL_MATCH(Identifier, kf::StringView);
    constexpr bool matchImpl(kf::StringView name_or_shortcut) const noexcept {
        return (name == name_or_shortcut) or (name_or_shortcut.length() == 1 and shortcut.unwrapOr(0) == name_or_shortcut[0]);
    }
};

}// namespace botix::cli