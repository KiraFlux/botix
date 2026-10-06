// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// Botix - https://github.com/KiraFlux/botix

#pragma once

#include <kf/mixin/DefaultResettable.hpp>
#include <kf/mixin/NonCopyable.hpp>

#include "botix/transport/EspnowTransport.hpp"
#include "botix/transport/Kind.hpp"
#include "botix/transport/Transport.hpp"
#include "botix/transport/WifiUdpTransport.hpp"

namespace botix::internal {

struct TransportRegistryConfig : kf::mixin::DefaultResettable<TransportRegistryConfig> {

    transport::WifiUdpTransport::Config wifi_udp{};
};

}// namespace botix::internal

namespace botix::transport {

struct Registry : kf::mixin::NonCopyable {

    using Config = internal::TransportRegistryConfig;

    explicit Registry(Config const &config) noexcept :
        wifi_udp{config.wifi_udp} {};

    [[nodiscard]] constexpr Transport &get(Kind kind) noexcept {
        switch (kind) {
            case Kind::Wifi:
                return wifi_udp;

            case Kind::Espnow:
            default:
                return espnow;
        }
    }

    EspnowTransport espnow{};
    WifiUdpTransport wifi_udp;
};

}// namespace botix::transport