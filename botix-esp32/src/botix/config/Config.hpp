// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// Botix - https://github.com/KiraFlux/botix

#pragma once

#include <kf/Bytes.hpp>
#include <kf/Option.hpp>
#include <kf/core.hpp>

#include <kf/mixin/DefaultResettable.hpp>

namespace botix::config {

struct ConfigTag {};

/// @brief Persistent configuration structure
template<typename Impl, auto V> struct Config :

    ConfigTag,
    kf::mixin::DefaultResettable<Impl>

{
    static constexpr kf::u8 latest_version{V};

    kf::u8 version{latest_version};

    [[nodiscard]] static constexpr auto fromBytes(kf::Bytes bytes) noexcept -> kf::Option<Impl &> {
        return (bytes.length() == sizeof(Impl)) ? kf::someRef(*reinterpret_cast<Impl *>(bytes.data())) : kf::none;
    }

    [[nodiscard]] constexpr auto bytes() noexcept -> kf::Bytes {
        return {
            reinterpret_cast<kf::u8 *>(this),
            sizeof(Impl),
        };
    }
};

}// namespace botix::config