// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// Botix - https://github.com/KiraFlux/botix

#pragma once

#include <utility>

#include <WiFi.h>

#include <kf/Arena.hpp>
#include <kf/Option.hpp>
#include <kf/units.hpp>

#include "botix/cli/Argument.hpp"
#include "botix/cli/Command.hpp"
#include "botix/cli/Group.hpp"
#include "botix/config/DeviceConfig.hpp"
#include "botix/config/UserConfig.hpp"
#include "botix/service/WifiService.hpp"
#include "botix/transport/Kind.hpp"
#include "botix/transport/Link.hpp"
#include "botix/transport/Receiver.hpp"
#include "botix/transport/Registry.hpp"

#include "botix/system/System.hpp"

namespace botix::system {

struct TransportSystem : System<TransportSystem> {

    struct Dependencies {
        config::DeviceConfig const &device_config;
        config::UserConfig const &user_config;
    };

    explicit TransportSystem(Dependencies deps) noexcept :
        System<TransportSystem>{{.name{"transport"}}},
        _registry{deps.device_config.transport_registry},
        link{_registry.get(deps.user_config.boot.transport)},
        wifi_service{deps.user_config.wifi_service, deps.device_config.transport_registry.wifi_udp.local_port} {}

    [[nodiscard]] auto &get(transport::Kind kind) noexcept {
        return _registry.get(kind);
    }

    void onReceive(auto &&f) noexcept {
        _receiver.onReceive(std::forward<decltype(f)>(f));
    }

    void onReceiveForeign(auto &&f) noexcept {
        _receiver.onReceiveForeign(std::forward<decltype(f)>(f));
    }

private:
    transport::Receiver _receiver{};
    transport::Registry _registry;

public:
    transport::Link link;
    service::WifiService wifi_service;

private:
    cli::Argument::Enum::Item const transport_kinds[2]{
        {{.name{"espnow"}}, transport::Kind::Espnow},
        {{.name{"wifi"}}, transport::Kind::Wifi},
    };

    cli::Argument use_command_arguments[1]{
        {
            {.name{"kind"}},
            cli::Argument::Enum{.items{transport_kinds}},
        },
    };

    BOTIX_IMPL_SYSTEM(TransportSystem);

    void onSetupImpl() noexcept {
        WiFi.mode(WIFI_MODE_STA);

        (void) _registry.espnow.init();

        _registry.espnow.receiver(kf::someRef(_receiver));
        _registry.wifi_udp.receiver(kf::someRef(_receiver));
    }

    void setupCliImpl(kf::Arena &arena, cli::Group &group) noexcept {

        (void) group.addCommand(arena, {.name{"status"}}, {}, [this](cli::Command::Context const &context) {
            context.channel.output.print("active address: {}", link.activeAddress());

            switch (link.kind()) {
                case transport::Kind::Espnow:
                    context.channel.output.print("self: {}", kf::esp::Espnow::instance().mac());
                    break;

                case transport::Kind::Wifi:
                    context.channel.output.print("local ip: {}", wifi_service.localAddress());
                    break;
            }
        });

        (void) group.addCommand(arena, {.name{"use"}}, use_command_arguments, [this](cli::Command::Context const &context) {
            auto const target_kind = context.arguments[0].enumValue<transport::Kind>();
            auto const target_name = context.arguments[0].enumName();

            link.set(_registry.get(target_kind));
            context.channel.output.print("switched to '{}' transport", target_name);
        });

        (void) group.addCommand(arena, {.name{"connect"}}, {}, [this](cli::Command::Context const &context) {
            switch (link.kind()) {
                case transport::Kind::Espnow:
                    context.channel.output.error("special connecting is not implemented for espnow");// TODO:
                    break;

                case transport::Kind::Wifi:
                    context.channel.output.print("connecting...");
                    auto const &address = _registry.wifi_udp.configuredAddress();

                    wifi_service.connect();

                    if (link.connect(address)) {
                        context.channel.output.print("connecting to {}", address);
                    } else {
                        context.channel.output.error("failed to set destination address");
                    }
                    break;
            }
        });

        (void) group.addCommand(arena, {.name{"disconnect"}}, {}, [this](cli::Command::Context const &context) {
            if (link.kind() == transport::Kind::Wifi) {
                wifi_service.disconnect();
            }
            link.disconnect();
            context.channel.output.print("disconnected");
        });
    }

    void pollImpl(kf::units::Milliseconds now) noexcept {
        wifi_service.poll(now);
        link.poll(now);
    }
};

}// namespace botix::system