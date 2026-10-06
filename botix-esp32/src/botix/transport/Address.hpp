// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// Botix - https://github.com/KiraFlux/botix

#pragma once

#include <kf/MacAddress.hpp>

#include <kf/mixin/Equatable.hpp>
#include <kf/mixin/ReprTo.hpp>

#include "botix/transport/IpEndpoint.hpp"
#include "botix/transport/Kind.hpp"

namespace botix::transport {

struct Address :

    kf::mixin::Equatable<Address>,
    kf::mixin::ReprTo<Address>

{
    using Self = Address;

    [[nodiscard]] static constexpr Self createForEspnow(kf::MacAddress const &mac_address) noexcept {
        return Self{mac_address};
    }

    [[nodiscard]] static constexpr Self createForWifi(IpEndpoint const &ip_endpoint) noexcept {
        return Self{ip_endpoint};
    }

    [[nodiscard]] constexpr Kind kind() const noexcept {
        return _kind;
    }

    [[nodiscard]] constexpr kf::MacAddress const &mac() const noexcept {
        return _espnow_mac_address;
    }

    [[nodiscard]] constexpr IpEndpoint const &endpoint() const noexcept {
        return _wifi_ip_endpoint;
    }

private:
    explicit constexpr Address(kf::MacAddress const &mac_address) noexcept :
        _espnow_mac_address{mac_address}, _kind{Kind::Espnow} {}

    explicit constexpr Address(IpEndpoint const &ip_endpoint) noexcept :
        _wifi_ip_endpoint{ip_endpoint}, _kind{Kind::Wifi} {}

    union {
        kf::MacAddress _espnow_mac_address;
        IpEndpoint _wifi_ip_endpoint;
    };

    Kind _kind;

    KF_IMPL_EQUATABLE(Self);
    constexpr bool isEqualsImpl(Self const &other) const noexcept {
        if (other._kind != _kind) {
            return false;
        }

        switch (_kind) {
            case Kind::Espnow: return _espnow_mac_address == other._espnow_mac_address;
            case Kind::Wifi: return _wifi_ip_endpoint == other._wifi_ip_endpoint;
            default: return false;
        }
    }

    KF_IMPL_REPR_TO(Self);
    constexpr kf::usize reprToImpl(auto &char_writable) const noexcept {
        switch (_kind) {
            case Kind::Espnow: return char_writable.append(_espnow_mac_address) + char_writable.append("@espnow");
            case Kind::Wifi: return char_writable.append(_wifi_ip_endpoint) + char_writable.append("@wifi");
            default: return 0;
        }
    }
};

}// namespace botix::transport