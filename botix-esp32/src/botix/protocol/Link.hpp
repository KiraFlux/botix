// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// Botix - https://github.com/KiraFlux/botix

#pragma once

#include <kf/mixin/NonCopyable.hpp>

#include "botix/protocol/Protocol.hpp"

namespace botix::protocol {

struct Link : kf::mixin::NonCopyable {

    explicit constexpr Link(protocol::Protocol &protocol) noexcept :
        _protocol{&protocol} {}

    constexpr void set(protocol::Protocol &new_protocol) noexcept {
        _protocol = &new_protocol;
    }

    void poll(Protocol::PollContext const &context) noexcept {
        _protocol->poll(context);
    }

    void receive(Protocol::ReceiveContext const &context) noexcept {
        _protocol->receive(context);
    }

private:
    protocol::Protocol *_protocol;
};

}// namespace botix::protocol