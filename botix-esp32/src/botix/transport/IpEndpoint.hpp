// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// Botix - https://github.com/KiraFlux/botix

#pragma once

#include <kf/core.hpp>

#include <kf/mixin/Equatable.hpp>
#include <kf/mixin/ReprTo.hpp>

#include "botix/transport/IPv4.hpp"

namespace botix::transport {

struct IpEndpoint :

    kf::mixin::Equatable<IpEndpoint>,
    kf::mixin::ReprTo<IpEndpoint>

{

    IPv4 address;
    kf::u16 port;

private:
    KF_IMPL_EQUATABLE(IpEndpoint);
    constexpr bool isEqualsImpl(IpEndpoint const &other) const noexcept {
        return address == other.address and port == other.port;
    }

    KF_IMPL_REPR_TO(IpEndpoint);
    constexpr kf::usize reprToImpl(auto &char_writable) const noexcept {
        return (
            char_writable.append(address) +
            char_writable.append(':') +
            char_writable.append(port));
    }
};

}// namespace botix::transport