// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// Botix - https://github.com/KiraFlux/botix

#pragma once

#include <utility>// For std::forward, std::move

#include <kf/Arena.hpp>
#include <kf/Registry.hpp>
#include <kf/Slice.hpp>
#include <kf/StringView.hpp>
#include <kf/core.hpp>

#include <kf/mixin/ExtraAllocationLength.hpp>
#include <kf/mixin/NonCopyable.hpp>

#include "botix/cli/Argument.hpp"
#include "botix/cli/Command.hpp"
#include "botix/cli/Config.hpp"
#include "botix/cli/Identifier.hpp"

namespace botix::cli {

// TODO: just make owns a slice with command (no runtime registration)
struct Group :

    Identifier,
    private kf::Registry<Command>,
    kf::mixin::NonCopyable,
    kf::mixin::ExtraAllocationLength<Group>

{

    explicit constexpr Group(kf::Arena &arena, Config const &config, Identifier id) noexcept :
        Identifier{id},
        kf::Registry<Command>{arena, config.max_command_count} {}

    [[nodiscard]] constexpr auto commands() noexcept {
        return this->items();
    }

    [[nodiscard]] constexpr auto commands() const noexcept {
        return this->items();
    }

    [[nodiscard]] auto getCommand(kf::StringView name_or_shortcut) noexcept {
        return this->get(name_or_shortcut);
    }

    [[nodiscard]] auto getCommand(kf::StringView name_or_shortcut) const noexcept {
        return this->get(name_or_shortcut);
    }

    [[nodiscard]] auto addCommand(kf::Arena &arena, Identifier id, kf::Slice<Argument> arguments, auto &&handler) noexcept {
        return this->add(arena, std::move(id), std::move(arguments), std::forward<decltype(handler)>(handler));
    }

private:
    KF_IMPL_EXTRA_ALLOCATION_LENGTH(Group);

    static constexpr auto getExtraAllocationLengthImpl(Config const &config, auto const &...args) noexcept {
        return static_cast<kf::usize>(config.max_command_count * sizeof(Command));
    }
};

}// namespace botix::cli