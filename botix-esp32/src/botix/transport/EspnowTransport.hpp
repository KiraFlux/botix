// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// Botix - https://github.com/KiraFlux/botix

#pragma once

#include <utility>

#include <kf/BytesView.hpp>
#include <kf/Logger.hpp>
#include <kf/MacAddress.hpp>
#include <kf/Option.hpp>
#include <kf/Timer.hpp>
#include <kf/core.hpp>
#include <kf/esp/Espnow.hpp>

#include <kf/mixin/Initable.hpp>
#include <kf/mixin/NonCopyable.hpp>

#include "botix/transport/Address.hpp"
#include "botix/transport/Receiver.hpp"
#include "botix/transport/Transport.hpp"

namespace botix::transport {

struct EspnowTransport final :

    Transport,
    kf::mixin::Initable<EspnowTransport, bool()>

{
    using Self = EspnowTransport;

    constexpr EspnowTransport() noexcept :
        Transport{Kind::Espnow} {}

    void poll(kf::units::Milliseconds now) noexcept override {
        if (_broadcast_heartbeat_timer.expired(now)) {
            _broadcast_heartbeat_timer.start(now);

            if (_broadcast_peer.isSome()) {
                (void) _broadcast_peer.unwrap().writeByte(0xAA);
            }
        }
    }

    bool send(kf::BytesView buffer) noexcept override {
        if (_active_peer.isSome()) {
            return _active_peer.unwrap().writeBuffer(buffer).isOk();
        } else {
            return false;
        }
    }

protected:
    bool doConnect(Address const &address) noexcept override {
        _active_peer = createPeer(address.mac());
        return _active_peer.isSome();
    }

    void doDisconnect() noexcept override {
        if (not connected()) {
            logger.warn("Disconnect failed: No active peer");
            return;
        }

        auto &peer = _active_peer.unwrap();
        if (not peer.exist()) {
            logger.error("Disconnect failed: Peer not exit");
            return;
        }

        _active_peer.reset();
        logger.info("Disconnected: OK");
    }

private:
    using Espnow = kf::esp::Espnow;

    static constexpr kf::Timer::Config _broadcast_heartbeat_timer_config{.value = 2000};

    inline static char logger_buffer[64]{};

    inline static kf::Logger logger{"EspnowTransport", {logger_buffer}};

    kf::Option<Espnow::Peer> _active_peer{kf::none}, _broadcast_peer{kf::none};
    kf::Timer _broadcast_heartbeat_timer{_broadcast_heartbeat_timer_config};

    [[nodiscard]] static kf::Option<Espnow::Peer> createPeer(kf::MacAddress const &mac_address) noexcept {
        auto peer_result = Espnow::Peer::create({
            .mac_address = mac_address,
            .wifi_interface_sta = true,
        });

        if (peer_result.isOk()) {
            return kf::some(std::move(peer_result.ok()));
        } else {
            logger.error("Espnow peer add failed: {}", peer_result.error());
            return kf::none;
        }
    }

    KF_IMPL_INITABLE(Self, bool());
    bool initImpl() noexcept {
        auto &espnow = Espnow::instance();

        auto const init_result = espnow.init();
        if (init_result.isError()) {
            logger.error("Espnow init failed: {}", init_result.error());
            return false;
        }

        espnow.callback([this](kf::MacAddress const &mac_address, kf::BytesView buffer) -> void {
            if (auto maybe_receiver = receiver(); maybe_receiver.isSome()) {

                Receiver::ReceiveContext const context{
                    .address = Address::createForEspnow(mac_address),
                    .buffer = buffer,
                };

                if (_active_peer.isSome() and _active_peer.unwrap().mac() == mac_address) {
                    maybe_receiver.unwrap().invokeReceiveCallback(context);
                } else {
                    maybe_receiver.unwrap().invokeReceiveForeignCallback(context);
                }
            }
        });

        _broadcast_peer = createPeer({0xff, 0xff, 0xff, 0xff, 0xff, 0xff});
        return _broadcast_peer.isSome();
    }
};

}// namespace botix::transport