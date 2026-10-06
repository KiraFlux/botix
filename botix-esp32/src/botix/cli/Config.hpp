// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// Botix - https://github.com/KiraFlux/botix

#pragma once

#include <kf/core.hpp>

#include <kf/mixin/DefaultResettable.hpp>

namespace botix::cli {

struct Config : kf::mixin::DefaultResettable<Config> {

    kf::u8
        max_group_count{0x10},
        max_command_count{0x10};

    kf::u16
        channel_input_queue_length{0x02'00},
        channel_input_line_length{0x00'80},
        channel_output_line_length{0x10'00};
};

}// namespace botix::cli