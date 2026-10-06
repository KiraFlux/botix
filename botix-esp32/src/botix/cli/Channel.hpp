// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// Botix - https://github.com/KiraFlux/botix

#pragma once

#include <kf/Arena.hpp>
#include <kf/Queue.hpp>
#include <kf/Slice.hpp>
#include <kf/String.hpp>
#include <kf/StringView.hpp>
#include <kf/core.hpp>
#include <kf/units.hpp>

#include <kf/mixin/Drain.hpp>
#include <kf/mixin/ExtraAllocationLength.hpp>
#include <kf/mixin/NonCopyable.hpp>
#include <kf/mixin/ReadAvailable.hpp>

#include "botix/cli/Config.hpp"

namespace botix::cli {

struct Channel :

    kf::mixin::NonCopyable,
    kf::mixin::ExtraAllocationLength<Channel>

{

    struct Parameters {
        bool echo;
    };

    struct Input :

        kf::mixin::NonCopyable

    {

        enum class Status {
            Idle,         // Queue is empty, nothing to process
            LineReady,    // Line can be consumed (reach enter or buffer is full)
            HintRequested,// Tabulation
        };

        explicit constexpr Input(kf::Slice<char> input_queue_buffer, kf::Slice<char> input_line_buffer) noexcept :
            _input_queue{input_queue_buffer}, _input_line{input_line_buffer} {}

        [[nodiscard]] bool feed(kf::StringView str) noexcept {
            for (auto const c: str) {
                if (not _input_queue.write(c)) {
                    return false;
                }
            }
            return true;
        }

        [[nodiscard]] auto peekLine() noexcept {
            return _input_line.view();
        }

        [[nodiscard]] auto consumeLine() noexcept {
            auto const line = _input_line.view();
            _input_line.reset();// just pointer got moved, no string modification
            return line;        // still view at line
        }

        [[nodiscard]] Status process() noexcept {
            while (_input_queue.availableForRead() > 0) {
                char const c = _input_queue.read().unwrap();

                switch (c) {
                    case '\t':
                        return Status::HintRequested;

                    case '\n':
                        return Status::LineReady;

                    case '\b':
                    case '\x7F':
                        (void) _input_line.read();// discarded: "common behavior"
                        break;

                    case 0x20 ... 0x7E:
                        if (not _input_line.write(c)) {
                            return Status::LineReady;
                        }
                        break;
                }
            }
            return Status::Idle;
        }

    private:
        kf::Queue<char> _input_queue;
        kf::String _input_line;
    };

    struct Output :

        kf::mixin::NonCopyable,
        kf::mixin::ReadAvailable<Output>,
        kf::mixin::Drain<Output, kf::StringView>

    {

        explicit constexpr Output(kf::Slice<char> buffer) noexcept :
            string{buffer} {}

        template<typename... Args> void error(kf::internal::FormatString<Args...> const &fmt, Args const &...args) noexcept {
            (void) string.append("error: ");
            print(fmt, args...);
        }

        template<typename... Args> void print(kf::internal::FormatString<Args...> const &fmt, Args const &...args) noexcept {
            (void) string.appendFormat(fmt, args...);
            (void) string.write('\n');
        }

        kf::String string;

    private:
        KF_IMPL_READ_AVAILABLE(Output);
        kf::usize availableForReadImpl() const noexcept {
            return string.availableForRead();
        }

        KF_IMPL_DRAIN(Output, kf::StringView);
        constexpr kf::StringView drainImpl() noexcept {
            auto const output = string.view();
            string.reset();// just pointer got moved, no string modification
            return output; // still valid
        }
    };

    struct Context {
        kf::StringView input_line;
        Parameters const &parameters;
        Output &output;
        kf::units::Milliseconds timestamp;
        kf::u8 num;
    };

    explicit constexpr Channel(kf::Arena &arena, cli::Config const &config, Parameters parameters) noexcept :
        parameters{parameters},
        input{arena.allocate<char>(config.channel_input_queue_length), arena.allocate<char>(config.channel_input_line_length)},
        output{arena.allocate<char>(config.channel_output_line_length)} {}

    Parameters parameters;
    Input input;
    Output output;

private:
    KF_IMPL_EXTRA_ALLOCATION_LENGTH(Channel);
    static constexpr auto getExtraAllocationLengthImpl(cli::Config const &config, auto) noexcept {
        return static_cast<kf::usize>(config.channel_input_queue_length + config.channel_input_line_length + config.channel_output_line_length);
    }
};

}// namespace botix::cli