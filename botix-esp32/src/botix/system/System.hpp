// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// Botix - https://github.com/KiraFlux/botix

#pragma once

#include <kf/Arena.hpp>
#include <kf/Logger.hpp>

#include <kf/mixin/NonCopyable.hpp>
#include <kf/mixin/Poll.hpp>

#include "botix/cli/Console.hpp"
#include "botix/cli/Identifier.hpp"

namespace botix::system {

struct SystemTag {};

template<typename Impl> struct System :

    SystemTag,
    kf::mixin::NonCopyable,
    kf::mixin::Poll<Impl>

{

    explicit constexpr System(cli::Identifier id) noexcept :
        _id{id} {}

    void setup(kf::Arena &arena, cli::Console &console) noexcept {
        static_cast<Impl *>(this)->onSetupImpl();

        if (auto maybe_group = console.addGroup(arena, _id); maybe_group.isSome()) {
            static_cast<Impl *>(this)->setupCliImpl(arena, maybe_group.unwrap());
        }

        this->_logger.info("setup completed");
    }

private:
    cli::Identifier _id;

protected:
    kf::Logger _logger{_id.name};
};

}// namespace botix::system

#define BOTIX_IMPL_SYSTEM(__impl__)                  \
    friend struct ::botix::system::System<__impl__>; \
    KF_IMPL_POLL(__impl__)
