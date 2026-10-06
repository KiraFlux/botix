// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// Botix - https://github.com/KiraFlux/botix

#pragma once

#include <kf/BytesView.hpp>
#include <kf/Timer.hpp>
#include <kf/units.hpp>

#include <kf/mixin/Callbacked.hpp>

#include "botix/IncomingTelemetry.hpp"
#include "botix/transport/Address.hpp"
#include "botix/transport/Link.hpp"

#include "botix/protocol/Protocol.hpp"

namespace botix::protocol {

struct RawProtocol :

    Protocol,
    kf::mixin::Callbacked<void(transport::Address const &address, kf::BytesView)>

{

    void poll(PollContext const &context) noexcept override {
        if (context.outgoing_telemetry.wheel_distance.ready(context.timestamp)) {
            (void) context.transport_link.writePacket(context.outgoing_telemetry.wheel_distance.value());
        }

        if (_heartbeat_timer.expired(context.timestamp)) {
            _heartbeat_timer.start(context.timestamp);

            (void) context.transport_link.writeByte(0xAA);
        }
    }

    void receive(ReceiveContext const &context) noexcept override {
        switch (context.transport.buffer.length()) {
            case sizeof(IncomingTelemetry::ControlInput):
                context.incoming_telemetry.control_input.update(
                    *reinterpret_cast<IncomingTelemetry::ControlInput const *>(context.transport.buffer.data()),
                    context.timestamp);
                return;

            default:
                this->invoke(context.transport.address, context.transport.buffer);
                return;
        }
    }

private:
    static constexpr kf::Timer::Config heartbeat_timer_config{.value = 2000};

    kf::Timer _heartbeat_timer{heartbeat_timer_config};
};

}// namespace botix::protocol