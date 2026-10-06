// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// Botix - https://github.com/KiraFlux/botix

#pragma once

#include <kf/units.hpp>

#include <kf/mixin/NonCopyable.hpp>

namespace botix::behavior {

struct Behavior : kf::mixin::NonCopyable {

    /// @brief Action request after behavior's poll
    enum class Request {

        /// @brief Keep this behavior for next poll
        Keep,

        /// @brief Change link to default behavior
        Quit,
    };

    virtual void onEnter() noexcept = 0;

    virtual void onQuit() noexcept = 0;

    [[nodiscard]] virtual Request onPoll(kf::units::Milliseconds now) noexcept = 0;
};

}// namespace botix::behavior