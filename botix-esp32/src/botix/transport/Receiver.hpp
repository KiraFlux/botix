// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// Botix - https://github.com/KiraFlux/botix

#pragma once

#include <utility>

#include <kf/BytesView.hpp>

#include <kf/mixin/Callbacked.hpp>
#include <kf/mixin/NonCopyable.hpp>

#include "botix/transport/Address.hpp"

namespace botix::internal {

struct ReceiveContext {
    transport::Address const &address;
    kf::BytesView buffer;
};

using OnReceiveBase = kf::mixin::Callbacked<void(ReceiveContext const &)>;

struct OnReceiveCallbacked : private OnReceiveBase {
    void onReceive(auto &&f) noexcept {
        this->callback(std::forward<decltype(f)>(f));
    }

    void invokeReceiveCallback(ReceiveContext const &context) noexcept {
        this->invoke(context);
    }
};

struct OnReceiveForeignCallbacked : private OnReceiveBase {
    void onReceiveForeign(auto &&f) noexcept {
        this->callback(std::forward<decltype(f)>(f));
    }

    void invokeReceiveForeignCallback(ReceiveContext const &context) noexcept {
        this->invoke(context);
    }
};

}// namespace botix::internal

namespace botix::transport {

struct Receiver final :

    kf::mixin::NonCopyable,
    internal::OnReceiveCallbacked,
    internal::OnReceiveForeignCallbacked

{
    using ReceiveContext = internal::ReceiveContext;

    constexpr Receiver() noexcept = default;
};

}// namespace botix::transport