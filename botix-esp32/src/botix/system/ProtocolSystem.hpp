// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// Botix - https://github.com/KiraFlux/botix

#pragma once

#include <utility>

#include <kf/Arena.hpp>
#include <kf/units.hpp>

#include "botix/OutgoingTelemetry.hpp"
#include "botix/cli/Argument.hpp"
#include "botix/cli/Channel.hpp"
#include "botix/cli/Command.hpp"
#include "botix/cli/Group.hpp"
#include "botix/config/DeviceConfig.hpp"
#include "botix/protocol/Kind.hpp"
#include "botix/protocol/Link.hpp"
#include "botix/protocol/Protocol.hpp"
#include "botix/protocol/Registry.hpp"
#include "botix/transport/Link.hpp"

#include "botix/system/System.hpp"

namespace botix::system {

struct ProtocolSystem : System<ProtocolSystem> {

    struct Dependencies {
        config::DeviceConfig const &device_config;
        config::UserConfig const &user_config;
        transport::Link &transport_link;
        OutgoingTelemetry &outgoing_telemetry;
        cli::Channel::Output &cli_channel_output;
    };

    explicit constexpr ProtocolSystem(Dependencies deps) noexcept :
        System<ProtocolSystem>{{.name = "protocol"}},
        _registry{deps.device_config.protocol_registry},
        _transport_link{deps.transport_link},
        _outgoing_telemetry{deps.outgoing_telemetry},
        _cli_channel_output{deps.cli_channel_output},
        link{_registry.get(deps.user_config.boot.protocol)} {}

    [[nodiscard]] auto &get(protocol::Kind kind) noexcept {
        return _registry.get(kind);
    }

    void onRawFallback(auto &&f) noexcept {
        _registry.raw.callback(std::forward<decltype(f)>(f));
    }

    void onMavlinkFallback(auto &&f) noexcept {
        _registry.mavlink.callback(std::forward<decltype(f)>(f));
    }

private:
    static constexpr cli::Argument::Enum::Item protocol_kind_options[2]{
        {{.name{"raw"}}, protocol::Kind::Raw},
        {{.name{"mavlink"}}, protocol::Kind::Mavlink},
    };

    protocol::Registry _registry;
    transport::Link &_transport_link;
    OutgoingTelemetry &_outgoing_telemetry;
    cli::Channel::Output &_cli_channel_output;

public:
    protocol::Link link;

private:
    cli::Argument set_command_arguments[1]{
        {
            {.name{"kind"}},
            cli::Argument::Enum{
                .items{protocol_kind_options},
            },
        },
    };

    BOTIX_IMPL_SYSTEM(ProtocolSystem);

    void onSetupImpl() noexcept {}

    void setupCliImpl(kf::Arena &arena, cli::Group &group) noexcept {
        (void) group.addCommand(
            arena,
            {.name = "set"},
            set_command_arguments,
            [this](cli::Command::Context const &context) -> void {
                auto const kind = context.arguments[0].enumValue<protocol::Kind>();
                auto const kind_name = context.arguments[0].enumName();
                auto const index = context.arguments[0].enumIndex();

                link.set(_registry.get(kind));
                context.channel.output.print("set {} protocol (index: {}, kind: {})", kind_name, index, static_cast<int>(kind));
            });
    }

    void pollImpl(kf::units::Milliseconds now) noexcept {
        link.poll({
            .transport_link = _transport_link,
            .outgoing_telemetry = _outgoing_telemetry,
            .cli_channel_output = _cli_channel_output,
            .timestamp = now,
        });
    }
};

}// namespace botix::system