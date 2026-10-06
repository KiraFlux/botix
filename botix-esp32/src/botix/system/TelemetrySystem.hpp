// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// Botix - https://github.com/KiraFlux/botix

#pragma once

#include <kf/Arena.hpp>
#include <kf/units.hpp>

#include "botix/IncomingTelemetry.hpp"
#include "botix/OutgoingTelemetry.hpp"
#include "botix/cli/Group.hpp"
#include "botix/config/DeviceConfig.hpp"

#include "botix/system/System.hpp"

namespace botix::system {

struct TelemetrySystem : System<TelemetrySystem> {

    struct Dependencies {
        config::DeviceConfig const &config;
    };

    constexpr explicit TelemetrySystem(Dependencies deps) noexcept :
        System<TelemetrySystem>{{.name{"telemetry"}, .shortcut{'m'}}},
        outgoing{deps.config.outgoing_telemetry} {}

    IncomingTelemetry incoming{};
    OutgoingTelemetry outgoing;

private:
    BOTIX_IMPL_SYSTEM(TelemetrySystem);

    void onSetupImpl() noexcept {}

    void setupCliImpl(kf::Arena &arena, cli::Group &group) noexcept {}

    void pollImpl(kf::units::Milliseconds now) noexcept {
        outgoing.poll(now);
    }
};

}// namespace botix::system