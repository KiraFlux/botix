// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// Botix - https://github.com/KiraFlux/botix

#pragma once

#include <algorithm>

#include <kf/BytesView.hpp>
#include <kf/core.hpp>

#include <kf/mixin/BinaryWritable.hpp>
#include <kf/mixin/NonCopyable.hpp>
#include <kf/mixin/Poll.hpp>

#include "botix/transport/Address.hpp"
#include "botix/transport/Transport.hpp"

namespace botix::transport {

struct Link final :

    kf::mixin::NonCopyable,
    kf::mixin::Poll<Link>,
    kf::mixin::BinaryWritable<Link, bool>

{
    using Self = Link;

    explicit constexpr Link(transport::Transport &transport) noexcept :
        _transport{&transport} {}

    void set(Transport &new_transport) noexcept {
        if (connected()) {
            disconnect();
        }

        _transport = &new_transport;
    }

    // properties

    [[nodiscard]] constexpr Kind kind() const noexcept {
        return _transport->kind();
    }

    [[nodiscard]] constexpr bool connected() const noexcept {
        return _transport->connected();
    }

    [[nodiscard]] constexpr auto activeAddress() const noexcept -> kf::Option<Address const &> {
        return _transport->activeAddress();
    }

    // control

    [[nodiscard]] bool connect(Address const &address) noexcept {
        return _transport->connect(address);
    }

    void disconnect() noexcept {
        _transport->disconnect();
    }

private:
    Transport *_transport;

    KF_IMPL_POLL(Self);
    void pollImpl(kf::units::Milliseconds now) noexcept {
        _transport->poll(now);
    }

    KF_IMPL_BINARY_WRITABLE(Self, bool);

    bool writeBufferImpl(kf::BytesView buffer) noexcept {
        return _transport->send(buffer);
    }

    bool writePacketImpl(kf::trivial auto const &packet) noexcept {
        return this->writeBuffer({
            reinterpret_cast<kf::u8 const *>(&packet),
            sizeof(packet),
        });
    }

    bool writeMixedImpl(kf::trivial auto const &header, kf::BytesView buffer) noexcept {
        kf::u8 mixed_buffer[sizeof(header) + buffer.length()];

        auto const header_data = reinterpret_cast<kf::u8 const *>(&header);
        std::copy(header_data, header_data + sizeof(header), mixed_buffer);
        std::copy(buffer.begin(), buffer.end(), mixed_buffer + sizeof(header));

        return this->writeBuffer(mixed_buffer);
    }
};

}// namespace botix::transport