// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// Botix - https://github.com/KiraFlux/botix

#pragma once

#include "botix/OutgoingTelemetry.hpp"
#include "botix/cli/Config.hpp"
#include "botix/protocol/Registry.hpp"
#include "botix/service/MixerService.hpp"
#include "botix/transport/Registry.hpp"
#include "botix/unit/Registry.hpp"

#include "botix/config/Config.hpp"

namespace botix::config {

struct DeviceConfig : Config<DeviceConfig, 5> {

    cli::Config cli{};

    OutgoingTelemetry::Config outgoing_telemetry{};

    unit::Registry::Config unit_registry{};
    transport::Registry::Config transport_registry{};
    protocol::Registry::Config protocol_registry{};

    service::MixerService::Config mixer_service{};
};

}// namespace botix::config