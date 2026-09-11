// Copyright (c) 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <MAVLink.h>

#include <kf/Queue.hpp>
#include <kf/Timer.hpp>
#include <kf/core.hpp>
#include <kf/math.hpp>
#include <kf/units.hpp>

#include <kf/mixin/Callbacked.hpp>
#include <kf/mixin/Configured.hpp>
#include <kf/mixin/DefaultResettable.hpp>

#include "botix/transport/Address.hpp"

#include "botix/protocol/Protocol.hpp"

namespace botix::internal {

/// @brief Configuration for the MAVLink protocol
struct MavlinkProtocolConfig : kf::mixin::DefaultResettable<MavlinkProtocolConfig> {

    /// @brief Timer for HEARTBEAT messages (ms)
    kf::Timer::Config heartbeat_timer{.value = 2'000};

    kf::u8

        max_poll_messages{3},

        /// @brief MAVLink system ID of this controller
        system_id_self{0x01},

        /// @brief MAVLink system ID of the target drone (0 = broadcast)
        system_id_target{0x7F},

        // components

        component_id_heartbeat{MAV_COMP_ID_USER1},
        component_id_serial_control{MAV_COMP_ID_USER2},
        component_id_telemetry{MAV_COMP_ID_USER3};
};

}// namespace botix::internal

namespace botix::protocol {

struct MavlinkProtocol :

    Protocol,
    kf::mixin::Configured<internal::MavlinkProtocolConfig>,
    kf::mixin::Callbacked<void(transport::Address const &, mavlink_message_t const &)>

{
    using Config = internal::MavlinkProtocolConfig;

    using kf::mixin::Configured<Config>::Configured;

    void poll(PollContext const &context) noexcept override {

        if (context.outgoing_telemetry.wheel_distance.ready(context.timestamp)) {
            (void) sendWheelDistance(context);
        }

        if (context.outgoing_telemetry.obstacle_distance.ready(context.timestamp)) {
            (void) sendObstacleDistance(context);
        }

        if (_heartbeat_timer.expired(context.timestamp)) {
            _heartbeat_timer.start(context.timestamp);

            (void) sendHeartbeat(context);
        }

        sendSerialControl(context);

        auto const to_read = kf::math::min(this->config().max_poll_messages, messages.availableForRead());
        for (auto i = 0; i < to_read; i += 1) {

            kf::u8 buffer[MAVLINK_MAX_PACKET_LEN];
            auto const message = messages.read();
            auto const len = mavlink_msg_to_send_buffer(buffer, &message.unwrap());

            (void) context.transport_link.writeBuffer({buffer, len});
        }
    }

    void receive(ReceiveContext const &context) noexcept override {
        mavlink_message_t message;
        mavlink_status_t status;

        // separate channels for each transport kind
        auto const channel = static_cast<mavlink_channel_t>(static_cast<int>(context.transport.address.kind()));

        for (auto b: context.transport.buffer) {
            if (mavlink_parse_char(channel, b, &message, &status) != 0) {
                if (not onMessage(context, message)) {
                    this->invoke(context.transport.address, message);
                }
            }
        }
    }

    [[nodiscard]] bool onMessage(ReceiveContext const &context, mavlink_message_t const &message) noexcept {
        switch (message.msgid) {
            case MAVLINK_MSG_ID_MANUAL_CONTROL:
                onManualControl(context, message);
                break;

            case MAVLINK_MSG_ID_SERIAL_CONTROL:
                onSerialControl(context, message);
                break;

            default:
                return false;
        }

        return true;
    }

private:
    mavlink_message_t message_queue_buffer[64]{};
    kf::Queue<mavlink_message_t> messages{{message_queue_buffer}};
    kf::Timer _heartbeat_timer{this->config().heartbeat_timer};

    void onManualControl(ReceiveContext const &context, mavlink_message_t const &message) noexcept {
        mavlink_manual_control_t m;
        mavlink_msg_manual_control_decode(&message, &m);
        context.incoming_telemetry.control_input.update(
            {
                .r_axis = m.r,
                .z_axis = m.z,
                .y_axis = m.y,
                .x_axis = m.x,
            },
            context.timestamp);
    }

    void onSerialControl(ReceiveContext const &context, mavlink_message_t const &message) noexcept {
        mavlink_serial_control_t m;
        mavlink_msg_serial_control_decode(&message, &m);
        (void) context.cli_channel_input.feed({reinterpret_cast<char const *>(m.data), m.count});
    }

    [[nodiscard]] bool sendWheelDistance(PollContext const &context) noexcept {
        auto const &wheel_distance = context.outgoing_telemetry.wheel_distance;

        mavlink_message_t message;

        kf::u8 const wheel_count = 2;
        kf::f64 const distance[wheel_count]{
            wheel_distance.value().left_mm / 1'000,
            wheel_distance.value().right_mm / 1'000,
        };

        (void) mavlink_msg_wheel_distance_pack(
            this->config().system_id_self,
            this->config().component_id_telemetry,
            &message,
            static_cast<kf::u64>(wheel_distance.timestamp()) * 1'000,// time_usec
            wheel_count,
            distance);

        return messages.write(message);
    }

    [[nodiscard]] bool sendObstacleDistance(PollContext const &context) noexcept {
        auto const &obstacle_distance = context.outgoing_telemetry.obstacle_distance;
        auto const timestamp_useconds = static_cast<kf::u64>(obstacle_distance.timestamp()) * 1'000;

        bool all_ok = true;
        auto output = obstacle_distance.value().distances_mm;
        kf::usize angle_offset = 0;

        while (output.length() > 0) {
            kf::u16 distances_cm[MAVLINK_MSG_OBSTACLE_DISTANCE_FIELD_DISTANCES_LEN];
            kf::u8 const chunk_length = kf::math::min(output.length(), MAVLINK_MSG_OBSTACLE_DISTANCE_FIELD_DISTANCES_LEN);

            for (kf::usize i = 0; i < MAVLINK_MSG_OBSTACLE_DISTANCE_FIELD_DISTANCES_LEN; i += 1) {
                if (i < chunk_length and output[i] != 0xFFFF) {
                    distances_cm[i] = (output[i] + 5) / 10;
                } else {
                    distances_cm[i] = 0xFFFF;
                }
            }

            mavlink_message_t message;
            (void) mavlink_msg_obstacle_distance_pack(
                this->config().system_id_self,
                this->config().component_id_telemetry,
                &message,
                timestamp_useconds,
                MAV_DISTANCE_SENSOR_LASER,
                distances_cm,
                1,                                             // increment (angular width)
                obstacle_distance.value().min_distance_mm / 10,// cm
                obstacle_distance.value().max_distance_mm / 10,// cm
                0.0f,                                          // increment_f (not used)
                static_cast<kf::f32>(angle_offset),
                MAV_FRAME_BODY_FRD);

            if (not messages.write(message)) {
                all_ok = false;
            }

            output = output.fromOffset(chunk_length);
            angle_offset += chunk_length;
        }

        return all_ok;
    }

    void sendSerialControl(PollContext const &context) noexcept {
        mavlink_message_t message;

        auto output = context.cli_channel_output.drain();

        while (output.length() > 0) {

            kf::u8 const chunk_length = kf::math::min(output.length(), MAVLINK_MSG_SERIAL_CONTROL_FIELD_DATA_LEN);

            (void) mavlink_msg_serial_control_pack(
                this->config().system_id_self,
                this->config().component_id_serial_control,
                &message,
                SERIAL_CONTROL_DEV_SHELL,
                SERIAL_CONTROL_FLAG_EXCLUSIVE | SERIAL_CONTROL_FLAG_RESPOND,
                0,// use default timeout,
                0,// use baudrate
                chunk_length,
                reinterpret_cast<kf::u8 const *>(output.data()),
                this->config().system_id_target,
                0);

            (void) messages.write(message);

            output = output.fromOffset(chunk_length);
        }
    }

    [[nodiscard]] bool sendHeartbeat(PollContext const &context) noexcept {
        mavlink_message_t message;

        (void) mavlink_msg_heartbeat_pack(
            this->config().system_id_self,
            this->config().component_id_heartbeat,
            &message,
            MAV_TYPE_GROUND_ROVER,
            MAV_AUTOPILOT_GENERIC,
            0, 0, 0// Base mode, Custom mode, System status
        );

        return messages.write(message);
    }
};

}// namespace botix::protocol