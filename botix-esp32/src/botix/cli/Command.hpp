// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// Botix - https://github.com/KiraFlux/botix

#pragma once

#include <kf/Function.hpp>
#include <kf/Slice.hpp>
#include <kf/StringView.hpp>
#include <kf/core.hpp>

#include <kf/mixin/NonCopyable.hpp>
#include <kf/mixin/ReprTo.hpp>

#include "botix/cli/Argument.hpp"
#include "botix/cli/Identifier.hpp"

namespace botix::cli {

struct Command :

    Identifier,
    kf::mixin::NonCopyable,
    kf::mixin::ReprTo<Command>

{
    using ArgumentsView = kf::Slice<Argument const>;

    struct Context {
        Channel::Context const &channel;
        ArgumentsView arguments;
    };

    explicit constexpr Command(Identifier id, kf::Slice<Argument> arguments, auto &&handler) noexcept :
        Identifier{id},
        _arguments{arguments},
        _handler{std::forward<decltype(handler)>(handler)} {}

    // properties

    [[nodiscard]] constexpr auto arguments() noexcept {
        return _arguments;
    }

    [[nodiscard]] constexpr ArgumentsView arguments() const noexcept {
        return _arguments;
    }

    [[nodiscard]] constexpr kf::usize positionalArgumentsCount() const noexcept {
        kf::usize count = 0;
        for (auto const &a: _arguments) {
            count += static_cast<kf::usize>(a.positional());
        }
        return count;
    }

    // argument
    // TODO: check if default out of constraint
    // TODO: check if non-default after default
    // TODO: check for name arg collision

    // control

    void execute(Channel::Context const &channel_context, kf::Slice<kf::StringView> argument_tokens) noexcept {

        if (argument_tokens.length() < positionalArgumentsCount() or argument_tokens.length() > _arguments.length()) {
            channel_context.output.error("expected {}..{} arguments, got {}", positionalArgumentsCount(), _arguments.length(), argument_tokens.length());
            writeHelp(channel_context.output.string, false);
            return;
        }

        int argument_index = 0;
        bool parse_failed = false;
        for (; argument_index < argument_tokens.length(); argument_index += 1) {
            auto lexeme = argument_tokens[argument_index];
            auto &argument = _arguments[argument_index];

            if (not argument.parse({.channel_output = channel_context.output, .lexeme = lexeme})) {
                parse_failed = true;
                channel_context.output.print("note: failed argument '{}'", argument.name);
            }
        }

        if (parse_failed) {
            channel_context.output.error("failed to parse positional argument(s)");
            writeHelp(channel_context.output.string, false);
            return;
        }

        auto default_value_arguments = _arguments.fromOffset(argument_index);
        for (auto &a: default_value_arguments) {
            a.reset();
        }

        _handler(Context{
            .channel = channel_context,
            .arguments = _arguments,
        });
    }

    void writeHelp(auto &char_writable, bool inline_description) const noexcept {
        if (not inline_description) {
            (void) char_writable.append("\nCommand:\n  ");
            (void) char_writable.append(this->name);

            if (this->shortcut.isSome()) {
                (void) char_writable.append('/');
                (void) char_writable.append(this->shortcut.unwrap());
            }

            if (not this->description.empty()) {
                (void) char_writable.append(" - ");
                (void) char_writable.append(this->description);
            }

            (void) char_writable.append("\nUsage:\n  ");
        }

        auto const write_length = char_writable.append(*this);

        if (inline_description and not this->description.empty()) {
            constexpr auto description_position{60};
            for (int i = description_position; i > write_length; i -= 1) {
                (void) char_writable.append(' ');
            }
            (void) char_writable.append(" - ");
            (void) char_writable.append(this->description);
        }
        (void) char_writable.append('\n');
    }

private:
    kf::Slice<Argument> _arguments;
    kf::Function<void(Context const &)> _handler;

    KF_IMPL_REPR_TO(Command);
    constexpr kf::usize reprToImpl(auto &char_writable) const noexcept {
        kf::usize write_count = char_writable.append(this->name);

        for (auto const &a: _arguments) {
            write_count += char_writable.append(' ');
            write_count += char_writable.append(a);
        }

        return write_count;
    }
};

}// namespace botix::cli