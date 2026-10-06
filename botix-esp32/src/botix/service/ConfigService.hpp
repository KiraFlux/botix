// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// Botix - https://github.com/KiraFlux/botix

#pragma once

#include <utility>

#include <kf/Bytes.hpp>
#include <kf/Function.hpp>
#include <kf/Logger.hpp>
#include <kf/StringView.hpp>
#include <kf/Timer.hpp>
#include <kf/core.hpp>
#include <kf/esp/NVS.hpp>
#include <kf/math.hpp>
#include <kf/units.hpp>

#include <kf/mixin/Callbacked.hpp>

#include "botix/service/Service.hpp"

namespace botix::internal {

using CallbackedByConfigView = kf::mixin::Callbacked<void(kf::Bytes)>;

struct ConfigServiceOnLoadCallbacked : private CallbackedByConfigView {

    /// @brief Set config service behavior on load
    void onLoad(auto &&f) noexcept {
        this->callback(std::forward<decltype(f)>(f));
    }

protected:
    void invokeOnLoadCallback(kf::Bytes bytes) noexcept {
        this->invoke(bytes);
    }
};

struct ConfigServiceResettingStrategy : private CallbackedByConfigView {

    /// @brief Set config service resetting strategy
    void resettingStrategy(auto &&f) noexcept {
        this->callback(std::forward<decltype(f)>(f));
    }

protected:
    void invokeResetStrategy(kf::Bytes bytes) noexcept {
        this->invoke(bytes);
    }
};

}// namespace botix::internal

namespace botix::service {

/// @brief Config service with delayed NVS operations
/// @note Requests are batched and executed on a 5-second timer from the main loop.
struct ConfigService :

    Service<ConfigService>,
    internal::ConfigServiceOnLoadCallbacked,
    internal::ConfigServiceResettingStrategy

{
    struct Dependencies {
        kf::esp::NVS &nvs;
        kf::StringView key;
        kf::Timer::Config const &sync_timer_config;
        kf::Bytes config_bytes;
    };

    explicit constexpr ConfigService(Dependencies deps) noexcept :
        _nvs{deps.nvs},
        _logger{deps.key},
        _sync_timer{deps.sync_timer_config},
        _config_bytes{deps.config_bytes} {}

    /// @brief Requests an deferred load of the config from NVS
    void requestLoad() noexcept {
        _load_requested = true;
        _logger.debug("Load requested");
    }

    /// @brief Requests an deferred reset of the config to defaults
    void requestReset() noexcept {
        _reset_requested = true;
        _logger.debug("Reset requested");
    }

    /// @brief Calculate CRC32 for config view
    [[nodiscard]] kf::u32 crc() const noexcept {
        return kf::math::crc32(_config_bytes);
    }

    /// @brief Force sync now
    void sync() noexcept {
        if (_load_requested) {
            _load_requested = false;

            _logger.info("Loading config from NVS...");

            if (_nvs.getBlob(blobKey(), _config_bytes).isOk()) {
                _stored_crc = crc();
                _logger.info("Loaded from NVS (CRC: {})", _stored_crc);

                this->invokeOnLoadCallback(_config_bytes);

            } else {
                _logger.error("Load failed");
                requestReset();
            }
        }

        if (_reset_requested) {
            _reset_requested = false;

            this->invokeResetStrategy(_config_bytes);
            _logger.info("Reset to defaults");
        }

        if (auto const current_crc = crc(); current_crc != _stored_crc) {
            _logger.info("Changed, saving (CRC: {} -> {})...", _stored_crc, current_crc);

            if (_nvs.setBlob(blobKey(), _config_bytes).isOk() and _nvs.commit().isOk()) {
                _stored_crc = current_crc;
                _logger.info("Saved, CRC updated");
            } else {
                _logger.error("Save failed");
            }
        }
    }

private:
    kf::esp::NVS &_nvs;
    kf::Logger _logger;
    kf::Bytes _config_bytes;
    kf::Timer _sync_timer;
    kf::u32 _stored_crc{};
    bool _load_requested{false}, _reset_requested{false};

    [[nodiscard]] constexpr char const *blobKey() const noexcept {
        return _logger.key().data();
    }

    KF_IMPL_POLL(ConfigService);
    void pollImpl(kf::units::Milliseconds now) noexcept {
        if (_sync_timer.expired(now)) {
            _sync_timer.start(now);
            sync();
        }
    }
};

}// namespace botix::service