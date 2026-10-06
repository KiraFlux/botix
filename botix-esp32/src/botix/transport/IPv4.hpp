// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// Botix - https://github.com/KiraFlux/botix

#pragma once

#include <kf/core.hpp>

#include <kf/mixin/Equatable.hpp>
#include <kf/mixin/ReprTo.hpp>

namespace botix::transport {

struct IPv4 :

    kf::mixin::Equatable<IPv4>,
    kf::mixin::ReprTo<IPv4>

{

    constexpr IPv4(kf::u32 value) noexcept :
        value{value} {}

    constexpr IPv4(kf::u8 a, kf::u8 b, kf::u8 c, kf::u8 d) noexcept :
        octet{a, b, c, d} {}

    union {
        kf::u32 value;
        struct {
            kf::u8 a, b, c, d;
        } octet;
    };

private:
    KF_IMPL_EQUATABLE(IPv4);
    constexpr bool isEqualsImpl(IPv4 const &other) const noexcept {
        return value == other.value;
    }

    KF_IMPL_REPR_TO(IPv4);
    constexpr kf::usize reprToImpl(auto &char_writable) const noexcept {
        return (
            char_writable.append(octet.a) +
            char_writable.append('.') +
            char_writable.append(octet.b) +
            char_writable.append('.') +
            char_writable.append(octet.c) +
            char_writable.append('.') +
            char_writable.append(octet.d));
    }
};

}// namespace botix::transport