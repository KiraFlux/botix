// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// Botix - https://github.com/KiraFlux/botix

#pragma once

#include <kf/Arena.hpp>
#include <kf/Option.hpp>
#include <kf/Registry.hpp>
#include <kf/StringView.hpp>
#include <kf/core.hpp>
#include <kf/units.hpp>

#include <kf/mixin/Configured.hpp>
#include <kf/mixin/ExtraAllocationLength.hpp>
#include <kf/mixin/NonCopyable.hpp>
#include <kf/mixin/Poll.hpp>

#include "botix/cli/Channel.hpp"
#include "botix/cli/Config.hpp"
#include "botix/cli/Group.hpp"
#include "botix/cli/Identifier.hpp"

namespace botix::internal {

using ConsoleGroupRegistryBase = kf::Registry<cli::Group>;

template<typename Impl> struct ConsoleGroupRegistry : private ConsoleGroupRegistryBase {

    explicit ConsoleGroupRegistry(kf::Arena &arena, kf::usize max_group_count) noexcept :
        ConsoleGroupRegistryBase{arena, max_group_count} {
        // caller ensure
        (void) this->addGroup(
            arena,
            {
                .name{"global"},
                .description{"Common commands. Prefix is optional."},
                .shortcut{kf::none},
            });

        (void) this->globalGroup().addCommand(
            arena,
            {
                .name{"help"},
                .description{"Show help about command or group."},
                .shortcut{kf::none},
            },
            help_command_arguments,
            [this](cli::Command::Context const &context) -> void {
                auto &output = context.channel.output;
                auto const target = context.arguments[0].string();

                if (auto const &maybe_command = this->resolveCommand(target); maybe_command.isSome()) {
                    maybe_command.unwrap().writeHelp(output.string, false);
                    return;
                }

                if (auto const &maybe_group = this->getGroup(target); maybe_group.isSome()) {
                    writeGroupHelp(output.string, maybe_group.unwrap());
                    return;
                }

                if (not target.empty()) {
                    output.error("'{}' is not a valid group or command.", target);
                }

                for (auto const ns: this->groups()) {
                    writeGroupHelp(output.string, *ns);
                }
            });
    }

    [[nodiscard]] decltype(auto) groups() noexcept {
        return this->items();
    }

    [[nodiscard]] decltype(auto) groups() const noexcept {
        return this->items();
    }

    [[nodiscard]] decltype(auto) globalGroup() noexcept {
        return *groups()[0];
    }

    [[nodiscard]] decltype(auto) globalGroup() const noexcept {
        return *groups()[0];
    }

    [[nodiscard]] decltype(auto) getGroup(kf::StringView name_or_shortcut) noexcept {
        return this->get(name_or_shortcut);
    }

    [[nodiscard]] decltype(auto) getGroup(kf::StringView name_or_shortcut) const noexcept {
        return this->get(name_or_shortcut);
    }

    [[nodiscard]] decltype(auto) addGroup(kf::Arena &arena, cli::Identifier id) {
        return this->add(arena, static_cast<Impl const *>(this)->config(), id);
    }

    [[nodiscard]] auto resolveCommand(kf::StringView path) noexcept -> kf::Option<cli::Command &> {
        auto const maybe_delimeter_index = path.indexOf('.');

        if (maybe_delimeter_index.isNone()) {
            return globalGroup().getCommand(path);
        }

        auto const delimeter_index = maybe_delimeter_index.unwrap();

        auto maybe_group = getGroup(path.first(delimeter_index));
        if (maybe_group.isNone()) { return kf::none; }

        return maybe_group.unwrap().getCommand(path.fromOffset(delimeter_index + 1));
    }

protected:
    void writeGroupHelp(auto &char_writable, cli::Group const &group) const noexcept {
        (void) char_writable.append("\nGroup:\n  ");
        (void) char_writable.append(group.name);

        if (group.shortcut.isSome()) {
            (void) char_writable.append('/');
            (void) char_writable.append(group.shortcut.unwrap());
        }

        if (not group.description.empty()) {
            (void) char_writable.append(" - ");
            (void) char_writable.append(group.description);
        }

        (void) char_writable.append("\nCommands:\n");

        for (auto const c: group.commands()) {
            (void) char_writable.appendFormat("  {}.", group.name);
            c->writeHelp(char_writable, true);
        }
    }

private:
    cli::Argument help_command_arguments[1]{
        {
            {
                .name{"target"},
                .description{"resolvable name"},
            },
            cli::Argument::String{
                .params{.default_value{""}},
            },
        },
    };
};

}// namespace botix::internal

namespace botix::cli {

struct Console final :

    kf::mixin::Configured<Config>,
    internal::ConsoleGroupRegistry<Console>,
    kf::mixin::NonCopyable,
    kf::mixin::Poll<Console>,
    kf::mixin::ExtraAllocationLength<Console>

{

    explicit Console(kf::Arena &arena, Config const &config, kf::Slice<Channel> channels) noexcept :
        kf::mixin::Configured<Config>{config},
        _channels{channels},
        internal::ConsoleGroupRegistry<Console>{arena, config.max_group_count} {}

    [[nodiscard]] auto channels() noexcept {
        return _channels;
    }

private:
    static constexpr kf::usize max_tokens_count{8};

    kf::Slice<Channel> _channels;

    void onInputLineReady(Channel::Context const &channel_context) noexcept {
        if (channel_context.parameters.echo) {
            channel_context.output.print("[#{}]>>> {}", channel_context.num, channel_context.input_line);
        }

        kf::StringView tokens_buffer[max_tokens_count]{};
        auto tokens = channel_context.input_line.trim().split({tokens_buffer});
        if (tokens.empty()) { return; }

        auto const name = tokens[0];
        if (auto maybe_command = this->resolveCommand(name); maybe_command.isSome()) {
            maybe_command.unwrap().execute(channel_context, tokens.fromOffset(1));
        } else {
            channel_context.output.error("unknown command: {}", name);
        }
    }

    KF_IMPL_POLL(Console);
    void pollImpl(kf::units::Milliseconds now) noexcept {
        for (kf::u8 channel_num = 0; channel_num < _channels.length(); channel_num += 1) {
            auto &channel = _channels[channel_num];

            auto const status = channel.input.process();
            using S = decltype(status);

            switch (status) {
                case S::Idle:
                    break;

                case S::HintRequested:
                    channel.output.print("hint: '{}'", channel.input.peekLine());// TODO: parse, suggestion depends on input
                    break;

                case S::LineReady:
                    onInputLineReady({
                        .input_line = channel.input.consumeLine(),
                        .parameters = channel.parameters,
                        .output = channel.output,
                        .timestamp = now,
                        .num = channel_num,
                    });
                    break;
            }
        }
    }

    KF_IMPL_EXTRA_ALLOCATION_LENGTH(Console);
    static constexpr auto getExtraAllocationLengthImpl(Config const &config) noexcept {
        return static_cast<kf::usize>(config.max_group_count * sizeof(Group));
    }
};

}// namespace botix::cli