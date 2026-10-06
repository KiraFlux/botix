// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// Botix - https://github.com/KiraFlux/botix

#pragma once

#include <ESPmDNS.h>
#include <WiFi.h>

#include <kf/Logger.hpp>
#include <kf/Option.hpp>
#include <kf/StringView.hpp>
#include <kf/Timer.hpp>
#include <kf/core.hpp>
#include <kf/units.hpp>

#include <kf/mixin/Configured.hpp>
#include <kf/mixin/DefaultResettable.hpp>

#include "botix/service/Service.hpp"
#include "botix/transport/IPv4.hpp"

namespace botix::internal {

struct WifiServiceConfig : kf::mixin::DefaultResettable<WifiServiceConfig> {

    /// @brief Access point to join
    char ssid[32]{""};

    /// @brief WPA passphrase, empty for an open network
    char password[32]{""};

    /// @brief mDNS name, reachable as `<hostname>.local`
    char hostname[32]{"botix"};

    /// @brief Delay before a stalled association attempt is retried
    kf::Timer::Config retry_timer{.value{15'000}};

    /// @brief Delay to wait for IP address after association
    kf::Timer::Config ip_timeout{.value{5'000}};

    /// @brief Whether the station should be brought up at all
    bool enabled{false};
};

}// namespace botix::internal

namespace botix::service {

struct WifiService final :

    Service<WifiService>,
    kf::mixin::Configured<internal::WifiServiceConfig>

{
    using Self = WifiService;
    using Config = internal::WifiServiceConfig;

    /// @brief Stage of the WiFi station link
    enum class State : kf::u8 {
        Disabled,   ///< Station disabled by configuration
        Connecting, ///< Waiting for WL_CONNECTED
        ObtainingIp,///< WL_CONNECTED obtained, waiting for IP
        Connected,  ///< IP acquired, mDNS published
    };

    explicit constexpr WifiService(Config const &config, kf::u16 const &service_port) noexcept :
        kf::mixin::Configured<Config>{config},
        _service_port{service_port} {}

    [[nodiscard]] constexpr State state() const noexcept {
        return _state;
    }

    /// @brief Local address, only valid when state is Connected and IP is non-zero
    [[nodiscard]] auto localAddress() const noexcept -> kf::Option<transport::IPv4> {
        if (_state != State::Connected) {
            return kf::none;
        }
        auto const ip = WiFi.localIP();
        if (ip[0] == 0 and ip[1] == 0 and ip[2] == 0 and ip[3] == 0) {
            return kf::none;
        }
        return kf::some(transport::IPv4{ip[0], ip[1], ip[2], ip[3]});
    }

    /// @brief Request a (re)connection. Safe to call multiple times.
    void connect() noexcept {
        if (_state == State::Connecting or _state == State::ObtainingIp) {
            // already in progress
            return;
        }
        teardown();
        _state = State::Disabled;
        _connect_requested = true;
    }

    void disconnect() noexcept {
        teardown();
        _state = State::Disabled;
    }

private:
    inline static kf::Logger logger{"WifiService"};

    kf::u16 const &_service_port;
    kf::Timer _retry_timer{this->config().retry_timer};
    kf::Timer _ip_timer{this->config().ip_timeout};
    State _state{State::Disabled};
    bool _mdns_started{false};
    bool _connect_requested{false};

    void teardown() noexcept {
        if (_mdns_started) {
            MDNS.end();
            _mdns_started = false;
        }
        // Full reset of WiFi stack
        WiFi.disconnect(true, true);
        WiFi.mode(WIFI_MODE_STA);
        _state = State::Disabled;
    }

    void beginAssociation(kf::units::Milliseconds now) noexcept {
        auto const &c = this->config();
        if (c.ssid[0] == '\0') {
            return;
        }

        (void) WiFi.setHostname(c.hostname);
        logger.info("Joining '{}'", c.ssid);
        (void) WiFi.begin(c.ssid, c.password[0] == '\0' ? nullptr : c.password);

        _retry_timer.start(now);
        _state = State::Connecting;
    }

    void publishMdns() noexcept {
        auto const &c = this->config();
        if (not MDNS.begin(c.hostname)) {
            logger.error("mDNS begin failed");
            return;
        }
        MDNS.addService("botix", "udp", _service_port);
        _mdns_started = true;

        auto ip = WiFi.localIP();
        logger.info("Published as '{}.local' (IP {}.{}.{}.{}) on port {}",
                    c.hostname, ip[0], ip[1], ip[2], ip[3], _service_port);
    }

    BOTIX_IMPL_SERVICE(Self);
    void pollImpl(kf::units::Milliseconds now) noexcept {
        // Handle explicit connect request
        if (_connect_requested and _state != State::Connecting and _state != State::ObtainingIp) {
            _connect_requested = false;
            beginAssociation(now);
            return;
        }

        // If disabled by config, tear down
        if (not this->config().enabled) {
            if (_state != State::Disabled) {
                logger.info("Disabled by config");
                teardown();
                _state = State::Disabled;
            }
            return;
        }

        switch (_state) {
            case State::Disabled:
                beginAssociation(now);
                return;

            case State::Connecting: {
                if (WiFi.status() == WL_CONNECTED) {
                    // Switch to IP waiting state
                    logger.info("Associated, waiting for IP...");
                    _ip_timer.start(now);
                    _state = State::ObtainingIp;
                    return;
                }
                if (_retry_timer.expired(now)) {
                    logger.warn("Association timed out, retrying");
                    teardown();
                    beginAssociation(now);
                }
                return;
            }

            case State::ObtainingIp: {
                auto const ip = WiFi.localIP();
                if (ip[0] != 0 or ip[1] != 0 or ip[2] != 0 or ip[3] != 0) {
                    // IP obtained
                    publishMdns();
                    _state = State::Connected;
                    return;
                }
                if (_ip_timer.expired(now)) {
                    logger.warn("IP acquisition timed out, retrying");
                    teardown();
                    beginAssociation(now);
                }
                return;
            }

            case State::Connected: {
                if (WiFi.status() != WL_CONNECTED) {
                    logger.warn("Link lost");
                    teardown();
                    beginAssociation(now);
                } else {
                    // Check that IP is still valid (could be lost)
                    auto const ip = WiFi.localIP();
                    if (ip[0] == 0 and ip[1] == 0 and ip[2] == 0 and ip[3] == 0) {
                        logger.warn("IP address lost, reconnecting");
                        teardown();
                        beginAssociation(now);
                    }
                }
                return;
            }
        }
    }
};

}// namespace botix::service