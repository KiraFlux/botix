// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// Botix - https://github.com/KiraFlux/botix

#pragma once

#include <kf/Arena.hpp>
#include <kf/units.hpp>

#include "botix/cli/Argument.hpp"
#include "botix/cli/Command.hpp"
#include "botix/cli/Group.hpp"
#include "botix/config/DeviceConfig.hpp"
#include "botix/config/UserConfig.hpp"
#include "botix/unit/Registry.hpp"

#include "botix/system/System.hpp"

namespace botix::system {

struct UnitSystem : System<UnitSystem> {

    struct Dependencies {
        config::DeviceConfig const &device_config;
        config::UserConfig const &user_config;
    };

    explicit UnitSystem(Dependencies deps) noexcept :
        System<UnitSystem>{{.name{"unit"}}},
        registry{deps.device_config.unit_registry},
        _deps{deps} {}

    unit::Registry registry;

private:
    Dependencies const _deps;

    // TODO: use Kind's Repr
    cli::Argument::Enum::Item const unit_kinds[4]{
        {{.name{"wheel_motor"}}, unit::Kind::WheelMotor},
        {{.name{"servo"}}, unit::Kind::Servo},
        {{.name{"lidar"}}, unit::Kind::Lidar},
        {{.name{"wheel_encoder"}}, unit::Kind::WheelEncoder},
    };

    // TODO: use Kind's Repr
    cli::Argument::Enum::Item const lidar_strategies[4]{
        {{.name{"pass"}}, unit::LidarUnit::Strategy::Pass},
        {{.name{"nearest"}}, unit::LidarUnit::Strategy::Nearest},
        {{.name{"furthest"}}, unit::LidarUnit::Strategy::Furthest},
        {{.name{"lpf"}}, unit::LidarUnit::Strategy::LPF},
    };

    cli::Argument unit_related_command_args[2]{
        {
            {.name{"kind"}},
            cli::Argument::Enum{.items{unit_kinds}},
        },
        {
            {.name{"index"}},
            cli::Argument::Integer{
                .params{.default_value{0}},
                .min_value{0},
            },
        },
    };

    cli::Argument lidar_strategy_command_args[1]{
        {
            {.name{"strategy"}},
            cli::Argument::Enum{.items{lidar_strategies}},
        },
    };

    void doOnUnit(cli::Command::Context const &context, kf::callable<void(cli::Channel::Output &, unit::Unit &)> auto on_unit) noexcept {
        auto const kind = context.arguments[0].enumValue<unit::Kind>();
        auto const kind_name = context.arguments[0].enumName();
        auto const index = context.arguments[1].integer();

        if (auto maybe_unit = registry.get(kind, index); maybe_unit.isSome()) {
            on_unit(context.channel.output, maybe_unit.unwrap());
        } else {
            context.channel.output.error("unit '{}' at index {} not registered", kind_name, index);
        }
    }

    BOTIX_IMPL_SYSTEM(UnitSystem);

    void onSetupImpl() noexcept {
        auto const init_group = [this](auto &group) {
            for (auto &unit: group.units) {
                unit.init();

                if (unit.state() == unit::Unit::State::Running) {
                    this->_logger.info("{} : init successfully", unit);
                } else {
                    this->_logger.error("{} : init failed", unit);
                }
            }
        };

        init_group(registry.wheel_motor);
        init_group(registry.wheel_encoder);
        init_group(registry.servo);

        registry.lidar.parameters() = _deps.user_config.boot.lidar;
        if (_deps.user_config.boot.init_lidar) {
            registry.lidar.init();
        }
    }

    void setupCliImpl(kf::Arena &arena, cli::Group &group) noexcept {

        (void) group.addCommand(arena, {.name{"init"}}, unit_related_command_args, [this](auto const &context) -> void {
            doOnUnit(context, [](auto &output, unit::Unit &u) {
                u.init();
                output.print("init requested for {}", u);
            });
        });

        (void) group.addCommand(arena, {.name{"quit"}}, unit_related_command_args, [this](auto const &context) -> void {
            doOnUnit(context, [](auto &output, unit::Unit &u) {
                u.quit();
                output.print("quit requested for {}", u);
            });
        });

        (void) group.addCommand(arena, {.name{"state"}}, unit_related_command_args, [this](auto const &context) -> void {
            doOnUnit(context, [](auto &output, unit::Unit &u) {
                // TODO: use State repr
                output.print("unit '{}' has state: {}", u, unit::Unit::stateName(u.state()));
            });
        });

        (void) group.addCommand(arena, {.name{"list"}}, {}, [this](cli::Command::Context const &context) -> void {
            // TODO: add filter arg

            context.channel.output.print("registered units:");

            auto const show_units = [&context](auto &group) {
                for (auto &u: group.units) {
                    // TODO: use State repr
                    context.channel.output.print("  {} : {}", u, unit::Unit::stateName(u.state()));
                }
            };

            show_units(registry.wheel_motor);
            show_units(registry.servo);
            show_units(registry.wheel_encoder);
            context.channel.output.print("  {} : {}", registry.lidar, unit::Unit::stateName(registry.lidar.state()));
        });

        (void) group.addCommand(arena, {.name{"lidar_strategy"}, .shortcut{'y'}}, lidar_strategy_command_args, [this](cli::Command::Context const &context) -> void {
            auto const target_strategy = context.arguments[0].enumValue<driver::sensor::Lidar::Parameters::Strategy>();
            auto const target_strategy_name = context.arguments[0].enumName();

            registry.lidar.parameters().strategy = target_strategy;
            context.channel.output.print("lidar strategy set to '{}'", target_strategy_name);            
        });
    }

    void pollImpl(kf::units::Milliseconds now) noexcept {
        // TODO: sleep on timeout, etc...
    }
};

}// namespace botix::system