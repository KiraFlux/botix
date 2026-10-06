// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// Botix - https://github.com/KiraFlux/botix

#pragma once

#include <kf/mixin/Configured.hpp>
#include <kf/mixin/DefaultResettable.hpp>
#include <kf/mixin/NonCopyable.hpp>

#include "botix/protocol/Kind.hpp"
#include "botix/protocol/MavlinkProtocol.hpp"
#include "botix/protocol/Protocol.hpp"
#include "botix/protocol/RawProtocol.hpp"

namespace botix::internal {

struct ProtocolRegistryConfig : kf::mixin::DefaultResettable<ProtocolRegistryConfig> {

    protocol::MavlinkProtocol::Config mavlink{};
};

}// namespace botix::internal

namespace botix::protocol {

struct Registry :

    kf::mixin::NonCopyable,
    kf::mixin::Configured<internal::ProtocolRegistryConfig>

{

    using Config = internal::ProtocolRegistryConfig;

    using kf::mixin::Configured<Config>::Configured;

    constexpr Protocol &get(Kind kind) noexcept {
        switch (kind) {
            case Kind::Mavlink:
                return mavlink;

            case Kind::Raw:
            default:
                return raw;
        }
    }

    RawProtocol raw{};
    MavlinkProtocol mavlink{this->config().mavlink};
};

}// namespace botix::protocol