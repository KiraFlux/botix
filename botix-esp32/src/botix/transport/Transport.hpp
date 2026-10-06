// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// Botix - https://github.com/KiraFlux/botix

#pragma once

#include <kf/BytesView.hpp>
#include <kf/Option.hpp>
#include <kf/core.hpp>
#include <kf/units.hpp>

#include <kf/mixin/NonCopyable.hpp>

#include "botix/transport/Address.hpp"
#include "botix/transport/Receiver.hpp"

namespace botix::transport {

struct Transport : kf::mixin::NonCopyable {

    explicit constexpr Transport(Kind kind) noexcept :
        _kind{kind} {}

    // dynamic interface

    virtual void poll(kf::units::Milliseconds now) noexcept = 0;

    [[nodiscard]] virtual bool send(kf::BytesView buffer) noexcept = 0;

protected:
    [[nodiscard]] virtual bool doConnect(Address const &address) noexcept = 0;

    virtual void doDisconnect() noexcept = 0;

public:
    // properties

    [[nodiscard]] constexpr Kind kind() const noexcept {
        return _kind;
    }

    [[nodiscard]] constexpr bool connected() const noexcept {
        return _active_connection_address.isSome();
    }

    [[nodiscard]] constexpr auto activeAddress() const noexcept -> kf::Option<Address const &> {
        return _active_connection_address.isNone() ? kf::none : kf::someRef(_active_connection_address.unwrap());
    }

    [[nodiscard]] constexpr auto receiver() noexcept -> kf::Option<Receiver &> {
        return _receiver;
    }

    constexpr void receiver(kf::Option<Receiver &> new_receiver) noexcept {
        _receiver = new_receiver;
    }

    // control

    [[nodiscard]] bool connect(Address const &address) noexcept {
        if (connected()) {
            if (_active_connection_address.unwrap() == address) {
                return true;// already on this peer
            }

            disconnect();
        }

        if (_kind != address.kind()) {
            return false;// transport kind mismatch
        }

        if (not doConnect(address)) {
            return false;// connection failed
        }

        _active_connection_address = kf::some(address);

        return true;
    }

    void disconnect() noexcept {
        doDisconnect();
        _active_connection_address.reset();
    }

private:
    kf::Option<Address> _active_connection_address{kf::none};
    kf::Option<Receiver &> _receiver{kf::none};
    Kind const _kind;
};

}// namespace botix::transport