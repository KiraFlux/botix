// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// Botix - https://github.com/KiraFlux/botix

#pragma once

namespace botix::protocol {

enum class Kind : unsigned char {
    Raw = 0x00,
    Mavlink = 0x01,
};

}