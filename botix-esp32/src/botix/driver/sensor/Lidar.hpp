// Copyright (c) 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Acknowledge: 36-byte packet format got from @Mantlio (https://github.com/Mantlio)

#pragma once

#include <atomic>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include <kf/Array.hpp>
#include <kf/UART.hpp>
#include <kf/core.hpp>
#include <kf/gpio.hpp>
#include <kf/math.hpp>
#include <kf/rtos/Task.hpp>

#include <kf/mixin/Configured.hpp>
#include <kf/mixin/DefaultResettable.hpp>
#include <kf/mixin/Initable.hpp>
#include <kf/mixin/Quitable.hpp>

#include <kf/driver/sensor/SensorDriver.hpp>

namespace botix::internal {

template<kf::integer_type T> struct LidarData : kf::mixin::DefaultResettable<LidarData<T>> {

    using Distance = T;

    static constexpr kf::usize distances_total{360};
    static constexpr auto invalid_value{static_cast<Distance>(-1)};

    kf::Array<Distance, distances_total> distances_mm{};

    constexpr LidarData() noexcept {
        distances_mm.fill(invalid_value);
    }
};

using PublicLidarData = LidarData<kf::u16>;

struct LidarConfig {

    kf::UART::Config uart{
        .baudrate{115'200},
        .rx_buffer_length{0x08'00},
    };

    PublicLidarData::Distance
        min_distance_mm{20},
        max_distance_mm{12'000};

    kf::u8
        min_intensity{15},
        low_pass_filter_factor{16};
};

}// namespace botix::internal

namespace botix::driver::sensor {

struct Lidar :

    kf::driver::sensor::SensorDriver<Lidar, internal::PublicLidarData const &, void>,
    kf::mixin::Configured<internal::LidarConfig>,
    kf::mixin::Quitable<Lidar>

{

    using Self = Lidar;

    using Config = internal::LidarConfig;

    using Data = internal::PublicLidarData;

    /// @brief Runtime driver parameters (Active strategies, etc.)
    struct Parameters : kf::mixin::DefaultResettable<Parameters> {

        /// @brief Update strategies for each individual angular sector of the lidar scan.
        enum class Strategy : kf::u8 {
            Pass,    ///> No filtering: every new valid measurement completely replaces the previous stored value for that angle.
            Nearest, ///> Accumulate the minimum: keeps the smallest valid distance ever received for each angle.
            Furthest,///> Accumulate the maximum: keeps the largest valid distance ever received for each angle.
            LPF,     ///> Exponential smoothing (first‑order low‑pass filter).
        } strategy{Strategy::Pass};
    };

    struct SpecificConfig {
        kf::u8 uart_num;
        kf::gpio::GpioNumber rx_gpio_num, tx_gpio_num;
    };

    struct Dependencies {
        Config const &common_config;
        SpecificConfig specific_config;
    };

    explicit Lidar(Dependencies const &deps) noexcept :
        kf::mixin::Configured<Config>{deps.common_config},
        _uart{deps.common_config.uart, deps.specific_config.uart_num, deps.specific_config.rx_gpio_num, deps.specific_config.tx_gpio_num},
        _parser{deps.common_config} {}

    /// @brief Is lidar task running
    [[nodiscard]] bool running() const noexcept {
        return (_task_handle != nullptr) and eTaskState::eSuspended != eTaskGetState(_task_handle);
    }

    [[nodiscard]] Parameters &parameters() noexcept {
        return _parser.parameters;
    }

private:
    // Parser is composed in driver for future runtime switch between differrend LD models
    // TODO: extract Data updater from parser
    struct Parser :

        kf::mixin::Configured<Config>,
        kf::mixin::Initable<Parser, void()>

    {

        using q24_8 = kf::u32;
        using SampleData = internal::LidarData<q24_8>;

        using kf::mixin::Configured<Config>::Configured;

        void feed(kf::u8 byte) noexcept {
            switch (_state) {
                case State::Idle:
                    if (byte == header_sample.bytes[0]) {
                        _state = State::Header;
                        _header_buffer.reset();
                        // no break here: flow to Header label.
                    } else {
                        break;
                    }

                case State::Header:
                    _header_buffer.push(byte);
                    if (_header_buffer.ready()) {
                        if (_header_buffer.interpret().value == header_sample.value) {
                            _state = State::Body;
                            _body_buffer.reset();
                        } else {
                            _state = State::Idle;
                        }
                    }
                    break;

                case State::Body:
                    _body_buffer.push(byte);
                    if (_body_buffer.ready()) {
                        _state = State::Idle;
                        onBody(_body_buffer.interpret());
                        _body_buffer.reset();
                    }
                    break;
            }
        }

        void updateTo(Data &output_data) noexcept {
            if (mutexTake()) {

                for (kf::usize i = 0; i < Data::distances_total; i += 1) {
                    auto &sample = _sample_data.distances_mm[i];
                    output_data.distances_mm[i] = (SampleData::invalid_value == sample) ? Data::invalid_value : ((sample + 0x80) >> 8);
                    sample = SampleData::invalid_value;
                }

                (void) mutexGive();
            }
            // TODO: fill holes if enabled
        }

    private:
        struct Header {
            union {
                kf::u32 value;
                kf::u8 bytes[4];
            };
        };

        struct [[gnu::packed]] Body {

            struct [[gnu::packed]] Measurement {
                kf::u16 distance_mm;
                kf::u8 intensity;
            };

            static constexpr kf::usize measurement_count{8};
            static constexpr auto units_per_degree{64};

            [[maybe_unused]] kf::u16 rotation_speed;
            kf::u16 start_angle;
            Measurement measurements[measurement_count];
            kf::u16 end_angle;
            [[maybe_unused]] kf::u8 reserved[2];// TODO: CRC and byte ????

            [[nodiscard]] kf::u16 delta() const noexcept {
                auto ret = end_angle;
                if (end_angle < start_angle) {
                    ret += (Data::distances_total * units_per_degree);
                }
                return ret - start_angle;
            }

            [[nodiscard]] kf::usize index(kf::usize measurement_num, kf::u16 delta) const noexcept {
                constexpr auto angle_offset{0xA000};
                auto const raw_angle = start_angle + ((delta * measurement_num + (measurement_count / 2)) / measurement_count);
                return static_cast<kf::usize>(((raw_angle - angle_offset) / units_per_degree + Data::distances_total) % Data::distances_total);
            }
        };

        template<typename T> struct ByteBuffer {

            static constexpr kf::usize max_length{sizeof(T)};

            void push(kf::u8 byte) noexcept {
                _buffer[_index] = byte;
                _index += 1;
            }

            [[nodiscard]] bool ready() const noexcept {
                return max_length == _index;
            }

            [[nodiscard]] T const &interpret() const noexcept {
                return *reinterpret_cast<T const *>(_buffer);
            }

            void reset() noexcept {
                _index = 0;
            }

        private:
            alignas(T) kf::u8 _buffer[max_length]{};
            kf::usize _index{};
        };

        enum class State {
            Idle,
            Header,
            Body,
        };

        static constexpr Header header_sample{.bytes{0x55, 0xAA, 0x03, 0x08}};

        SampleData _sample_data{};
        ByteBuffer<Body> _body_buffer{};
        ByteBuffer<Header> _header_buffer{};
        StaticSemaphore_t _mutex_buffer;
        SemaphoreHandle_t _mutex{nullptr};
        State _state{State::Idle};

    public:
        Parameters parameters{};

    private:
        [[nodiscard]] bool mutexTake() noexcept {
            return (nullptr != _mutex) and xSemaphoreTake(_mutex, pdMS_TO_TICKS(10)) == pdTRUE;
        }

        [[nodiscard]] bool mutexGive() noexcept {
            return (nullptr != _mutex) and xSemaphoreGive(_mutex) == pdTRUE;
        }

        [[nodiscard]] q24_8 calcSample(q24_8 last_sample, Body::Measurement measurement) const noexcept {
            if (
                (measurement.intensity < this->config().min_intensity) or
                (measurement.distance_mm < this->config().min_distance_mm) or
                (measurement.distance_mm > this->config().max_distance_mm)) {
                return last_sample;
            }

            auto const current_sample = static_cast<SampleData::Distance>(measurement.distance_mm << 8);
            if (SampleData::invalid_value == last_sample) {
                return current_sample;
            }

            switch (parameters.strategy) {
                case Parameters::Strategy::Nearest:
                    return kf::math::min(current_sample, last_sample);

                case Parameters::Strategy::Furthest:
                    return kf::math::max(current_sample, last_sample);

                case Parameters::Strategy::LPF:
                    return static_cast<q24_8>(static_cast<kf::i32>(last_sample) + ((static_cast<kf::i32>(current_sample) - static_cast<kf::i32>(last_sample)) * static_cast<kf::i32>(this->config().low_pass_filter_factor) >> 8));

                case Parameters::Strategy::Pass:
                default:
                    return current_sample;
            }
        }

        void onBody(Body const &body) noexcept {
            auto const delta = body.delta();

            if (mutexTake()) {
                for (kf::usize i = 0; i < Body::measurement_count; i += 1) {
                    auto &sample = _sample_data.distances_mm[body.index(i, delta)];
                    sample = calcSample(sample, body.measurements[i]);
                }

                (void) mutexGive();
            }
        }

        KF_IMPL_INITABLE(Parser, void());
        void initImpl() noexcept {
            _sample_data.reset();
            _header_buffer.reset();
            _body_buffer.reset();
            _state = State::Idle;

            if (nullptr == _mutex) {
                _mutex = xSemaphoreCreateMutexStatic(&_mutex_buffer);
            }
        }
    };

    static constexpr kf::usize task_stack_length{0x10'00};
    StackType_t _task_stack[task_stack_length]{};
    StaticTask_t _task_buffer{};
    TaskHandle_t _task_handle{nullptr};

    Data _data{};
    kf::UART _uart;
    Parser _parser;
    std::atomic<bool> _start_requested{false}, _stop_requested{false};

    KF_IMPL_SENSOR_DRIVER(Self, Data const &, void);

    void initImpl() noexcept {
        _start_requested = true;

        auto const task_code = [](void *arg) noexcept {
            auto &self = *static_cast<Self *>(arg);

            while (true) {
                if (self._stop_requested.exchange(false)) {
                    self._uart.quit();
                    vTaskSuspend(nullptr);
                }

                if (self._start_requested.exchange(false)) {
                    self._uart.init();
                    self._parser.init();
                }

                if (auto const read = self._uart.readByte(); read.isOk()) {
                    self._parser.feed(read.ok());
                } else {
                    kf::rtos::Task::sleep(1);
                }
            }
        };

        if (nullptr == _task_handle) {
            _task_handle = xTaskCreateStatic(
                task_code,
                "lidar",
                task_stack_length,
                static_cast<void *>(this),
                1,// priority
                _task_stack,
                &_task_buffer);
        }

        if (not running()) {
            vTaskResume(_task_handle);
        }
    }

    Data const &readImpl() noexcept {
        _parser.updateTo(_data);
        return _data;
    }

    KF_IMPL_QUITABLE(Self);

    void quitImpl() noexcept {
        _stop_requested = true;
    }
};

}// namespace botix::driver::sensor