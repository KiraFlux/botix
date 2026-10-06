// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// Botix - https://github.com/KiraFlux/botix

#pragma once

namespace botix::unit {

enum class Kind : char {

    // actuators

    WheelMotor,
    Servo,

    // sensors

    Lidar,
    WheelEncoder,
};

}// namespace botix::unit