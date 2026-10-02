// Copyright (c) 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "botix/protocol/Kind.hpp"
#include "botix/service/WifiService.hpp"
#include "botix/transport/Kind.hpp"
#include "botix/driver/sensor/Lidar.hpp"
#include "botix/behavior/OperationalBehavior.hpp"

#include "botix/config/Config.hpp"

namespace botix::config {

struct UserConfig : Config<UserConfig, 4> {

    service::WifiService::Config wifi_service{};
    behavior::OperationalBehavior::Config operational_behavior{};

    struct {
        transport::Kind transport{transport::Kind::Wifi};
        protocol::Kind protocol{protocol::Kind::Mavlink};
        bool init_lidar{false};
        driver::sensor::Lidar::Parameters lidar{};
    } boot;
};

}// namespace botix::config