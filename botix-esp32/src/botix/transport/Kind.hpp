// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// Botix - https://github.com/KiraFlux/botix

#pragma once

namespace botix::transport {

enum class Kind : unsigned char {
    Espnow = 0x00,
    Wifi = 0x01,
};

}