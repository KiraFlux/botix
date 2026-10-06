// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// Botix - https://github.com/KiraFlux/botix

#pragma once

#include <WiFi.h>
#include <WiFiUdp.h>

#include <kf/BytesView.hpp>
#include <kf/Logger.hpp>
#include <kf/Option.hpp>
#include <kf/core.hpp>
#include <kf/units.hpp>

#include <kf/mixin/Configured.hpp>
#include <kf/mixin/DefaultResettable.hpp>
#include <kf/mixin/Initable.hpp>

#include "botix/transport/Address.hpp"
#include "botix/transport/IPv4.hpp"
#include "botix/transport/IpEndpoint.hpp"
#include "botix/transport/Receiver.hpp"
#include "botix/transport/Transport.hpp"

namespace botix::internal {

/// @brief Configuration for the WiFi UDP transport
struct WifiUdpTransportConfig : kf::mixin::DefaultResettable<WifiUdpTransportConfig> {

    kf::u16 local_port{14550};
    transport::IpEndpoint remote{
        .address{0},
        .port{14555},
    };
};

}// namespace botix::internal

namespace botix::transport {

struct WifiUdpTransport final :

    Transport,
    kf::mixin::Configured<internal::WifiUdpTransportConfig>

{

    using Self = WifiUdpTransport;
    using Config = internal::WifiUdpTransportConfig;

    explicit WifiUdpTransport(Config const &config) noexcept :
        Transport{Kind::Wifi},
        kf::mixin::Configured<Config>{config} {}

    [[nodiscard]] constexpr Address configuredAddress() const noexcept {
        return Address::createForWifi(this->config().remote);
    }

    [[nodiscard]] bool ready() const noexcept {
        return _listening and WiFi.status() == WL_CONNECTED;
    }

    void poll(kf::units::Milliseconds now) noexcept override {
        (void) now;

        if (not _listening) {
            if (WiFi.status() == WL_CONNECTED) {
                (void) startListening();
            }
            return;
        }

        if (WiFi.status() != WL_CONNECTED) {
            stopListening();
            return;
        }

        auto maybe_receiver = receiver();
        if (maybe_receiver.isNone()) {
            return;
        }

        auto &target = maybe_receiver.unwrap();

        while (true) {
            auto const packet_length = _udp.parsePacket();
            if (packet_length <= 0) {
                return;
            }

            auto const read = _udp.read(_receive_buffer, _receive_buffer_length);
            if (read <= 0) {
                continue;
            }

            auto const remote_ip = _udp.remoteIP();

            IpEndpoint source{
                .address{remote_ip[0], remote_ip[1], remote_ip[2], remote_ip[3]},
                .port{static_cast<kf::u16>(_udp.remotePort())},
            };

            Receiver::ReceiveContext const context{
                .address = Address::createForWifi(source),
                .buffer = {_receive_buffer, static_cast<kf::usize>(read)},
            };

            auto const maybe_active = activeAddress();

            if (maybe_active.isSome() and maybe_active.unwrap().endpoint() == source) {
                target.invokeReceiveCallback(context);
            } else {
                target.invokeReceiveForeignCallback(context);
            }
        }
    }

    bool send(kf::BytesView buffer) noexcept override {
        if (not _listening or buffer.empty()) {
            return false;
        }

        auto const maybe_address = activeAddress();
        if (maybe_address.isNone()) {
            return false;
        }

        auto const &remote = maybe_address.unwrap().endpoint();

        if (_udp.beginPacket({remote.address.octet.a, remote.address.octet.b, remote.address.octet.c, remote.address.octet.d}, remote.port) != 1) {
            return false;
        }

        _udp.write(buffer.data(), buffer.length());

        return _udp.endPacket() == 1;
    }

protected:
    bool doConnect(Address const &address) noexcept override {
        if (address.kind() != Kind::Wifi) {
            return false;
        }
        return true;
    }

    void doDisconnect() noexcept override {
        stopListening();
    }

private:
    inline static kf::Logger logger{"WifiUdpTransport"};

    static constexpr kf::usize _receive_buffer_length{512};

    WiFiUDP _udp{};
    kf::u8 _receive_buffer[_receive_buffer_length]{};
    bool _listening{false};

    [[nodiscard]] bool startListening() noexcept {
        if (_listening) {
            return true;
        }

        if (_udp.begin(this->config().local_port) == 1) {
            _listening = true;
            logger.info("Listening on port {}", this->config().local_port);
            return true;
        } else {
            logger.error("UDP bind failed on port {}", this->config().local_port);
            _listening = false;
            return false;
        }
    }

    void stopListening() noexcept {
        if (_listening) {
            _udp.stop();
            _listening = false;
            logger.info("Stopped listening");
        }
    }
};

}// namespace botix::transport