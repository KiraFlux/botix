// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// Botix - https://github.com/KiraFlux/botix

#pragma once

#include <type_traits>

#include <kf/Option.hpp>
#include <kf/Result.hpp>
#include <kf/Slice.hpp>
#include <kf/StringView.hpp>
#include <kf/core.hpp>

#include <kf/mixin/NonCopyable.hpp>
#include <kf/mixin/ReprTo.hpp>

#include "botix/Parser.hpp"
#include "botix/transport/IPv4.hpp"

namespace botix::internal {

enum class TypeKind : kf::u8 {

    // integers

    I8,
    U8,
    I16,
    U16,
    I32,
    U32,
    I64,
    U64,

    // float

    F32,
    F64,

    // mics

    Boolean,
    Char,
    IPv4,
};

template<typename T> struct type_kind_of {
    static_assert(not sizeof(T), "unsupported type");
};

template<> struct type_kind_of<kf::i8> : std::integral_constant<TypeKind, TypeKind::I8> {};
template<> struct type_kind_of<kf::u8> : std::integral_constant<TypeKind, TypeKind::U8> {};
template<> struct type_kind_of<kf::i16> : std::integral_constant<TypeKind, TypeKind::I16> {};
template<> struct type_kind_of<kf::u16> : std::integral_constant<TypeKind, TypeKind::U16> {};
template<> struct type_kind_of<kf::i32> : std::integral_constant<TypeKind, TypeKind::I32> {};
template<> struct type_kind_of<kf::u32> : std::integral_constant<TypeKind, TypeKind::U32> {};
template<> struct type_kind_of<kf::i64> : std::integral_constant<TypeKind, TypeKind::I64> {};
template<> struct type_kind_of<kf::u64> : std::integral_constant<TypeKind, TypeKind::U64> {};
template<> struct type_kind_of<kf::f32> : std::integral_constant<TypeKind, TypeKind::F32> {};
template<> struct type_kind_of<kf::f64> : std::integral_constant<TypeKind, TypeKind::F64> {};
template<> struct type_kind_of<bool> : std::integral_constant<TypeKind, TypeKind::Boolean> {};
template<> struct type_kind_of<char> : std::integral_constant<TypeKind, TypeKind::Char> {};
template<> struct type_kind_of<transport::IPv4> : std::integral_constant<TypeKind, TypeKind::IPv4> {};

template<typename T> constexpr auto type_kind_of_v = type_kind_of<std::decay_t<T>>::value;

template<typename Impl> struct FieldBase :

    kf::mixin::NonCopyable

{

    [[nodiscard]] bool set(kf::StringView lexeme) noexcept {
        return static_cast<Impl *>(this)->setImpl(lexeme);
    }

    [[nodiscard]] constexpr kf::usize reprValue(auto &char_writable) const noexcept {
        return static_cast<Impl const *>(this)->reprValueImpl(char_writable);
    }

    [[nodiscard]] constexpr kf::usize reprType(auto &char_writable) const noexcept {
        return static_cast<Impl const *>(this)->reprTypeImpl(char_writable);
    }
};

struct ValueField : FieldBase<ValueField> {

    template<typename T> constexpr ValueField(T &value) noexcept :
        _ptr{static_cast<void *>(&value)},
        _length{kf::none},
        _type_kind{type_kind_of_v<T>} {}

    template<typename T, kf::usize N> constexpr ValueField(T (&array)[N]) noexcept :
        _ptr{static_cast<void *>(array)},
        _length{N},
        _type_kind{type_kind_of_v<T>} {}

private:
    void *const _ptr;
    kf::Option<kf::usize> const _length;
    TypeKind const _type_kind;

    template<typename T> [[nodiscard]] constexpr bool setFrom(kf::StringView lexeme) const noexcept {
        if (_length.isNone()) {
            Parser<T> parser{};

            if (auto const parsed_value = parser.parse(lexeme); parsed_value.isSome()) {
                *static_cast<T *>(_ptr) = parsed_value.unwrap();
                return true;
            }

            return false;
        } else {

            if (TypeKind::Char == _type_kind) {
                for (kf::usize i = 0; i < _length.unwrap(); i += 1) {
                    static_cast<char *>(_ptr)[i] = '\0';
                }
                for (kf::usize i = 0; i < lexeme.length() and i < _length.unwrap(); i += 1) {
                    static_cast<char *>(_ptr)[i] = lexeme[i];
                }
                return true;
            }

            return false;
        }
    }

    [[nodiscard]] constexpr kf::StringView kindName() const noexcept {
        switch (_type_kind) {
            case TypeKind::I8: return "i8";
            case TypeKind::U8: return "u8";
            case TypeKind::I16: return "i16";
            case TypeKind::U16: return "u16";
            case TypeKind::I32: return "i32";
            case TypeKind::U32: return "u32";
            case TypeKind::I64: return "i64";
            case TypeKind::U64: return "u64";
            case TypeKind::F32: return "f32";
            case TypeKind::F64: return "f64";
            case TypeKind::Boolean: return "bool";
            case TypeKind::Char: return "char";
            case TypeKind::IPv4: return "IPv4";
            default: return "?";
        }
    }

    template<typename T> [[nodiscard]] constexpr kf::usize reprValueTo(auto &char_writable) const noexcept {
        if (_length.isNone()) {
            return char_writable.append(*static_cast<T *>(_ptr));
        } else {
            return char_writable.append(kf::Slice<T>{
                static_cast<T *>(_ptr),
                _length.unwrap(),
            });
        }
    }

    // impl for internal::Field
    friend struct ::botix::internal::FieldBase<ValueField>;

    bool setImpl(kf::StringView lexeme) noexcept {
        switch (_type_kind) {
            case TypeKind::I8: return setFrom<kf::i8>(lexeme);
            case TypeKind::U8: return setFrom<kf::u8>(lexeme);
            case TypeKind::I16: return setFrom<kf::i16>(lexeme);
            case TypeKind::U16: return setFrom<kf::u16>(lexeme);
            case TypeKind::I32: return setFrom<kf::i32>(lexeme);
            case TypeKind::U32: return setFrom<kf::u32>(lexeme);
            case TypeKind::I64: return setFrom<kf::i64>(lexeme);
            case TypeKind::U64: return setFrom<kf::u64>(lexeme);
            case TypeKind::F32: return setFrom<kf::f32>(lexeme);
            case TypeKind::F64: return setFrom<kf::f64>(lexeme);
            case TypeKind::Boolean: return setFrom<bool>(lexeme);
            case TypeKind::Char: return setFrom<char>(lexeme);
            case TypeKind::IPv4: return setFrom<transport::IPv4>(lexeme);
            default: return false;
        }
    }

    constexpr kf::usize reprTypeImpl(auto &char_writable) const noexcept {
        kf::usize written = char_writable.append(kindName());

        if (_length.isSome()) {
            written += char_writable.append('[');
            written += char_writable.append(_length.unwrap());
            written += char_writable.append(']');
        }

        return written;
    }

    constexpr kf::usize reprValueImpl(auto &char_writable) const noexcept {
        switch (_type_kind) {
            case TypeKind::I8: return reprValueTo<kf::i8>(char_writable);
            case TypeKind::U8: return reprValueTo<kf::u8>(char_writable);
            case TypeKind::I16: return reprValueTo<kf::i16>(char_writable);
            case TypeKind::U16: return reprValueTo<kf::u16>(char_writable);
            case TypeKind::I32: return reprValueTo<kf::i32>(char_writable);
            case TypeKind::U32: return reprValueTo<kf::u32>(char_writable);
            case TypeKind::I64: return reprValueTo<kf::i64>(char_writable);
            case TypeKind::U64: return reprValueTo<kf::u64>(char_writable);
            case TypeKind::F32: return reprValueTo<kf::f32>(char_writable);
            case TypeKind::F64: return reprValueTo<kf::f64>(char_writable);
            case TypeKind::Boolean: return reprValueTo<bool>(char_writable);
            case TypeKind::Char: return reprValueTo<char>(char_writable);
            case TypeKind::IPv4: return reprValueTo<transport::IPv4>(char_writable);
            default: return char_writable.append('?');
        }
    }
};

struct EnumField : internal::FieldBase<EnumField> {

    using Underlying = kf::u8;

    struct Item {

        constexpr Item(kf::StringView key, kf::enum_type auto value) noexcept :
            key{key}, value{static_cast<Underlying>(value)} {}

        kf::StringView key;
        Underlying value;
    };

    using ItemCollection = kf::Slice<Item const>;

    template<kf::enum_type T> constexpr EnumField(T &value, ItemCollection items) noexcept :
        _value{reinterpret_cast<Underlying &>(value)}, _items{items} {
        static_assert(sizeof(Underlying) == sizeof(T));
    }

private:
    Underlying &_value;
    ItemCollection _items;

    // impl for internal::Field
    friend struct ::botix::internal::FieldBase<EnumField>;

    bool setImpl(kf::StringView target_key) noexcept {
        for (auto const &e: _items) {
            if (e.key == target_key) {
                _value = e.value;
                return true;
            }
        }
        return false;
    }

    constexpr kf::usize reprTypeImpl(auto &char_writable) const noexcept {
        kf::usize written = 0;

        bool write_delimeter = false;

        for (auto const &e: _items) {
            if (write_delimeter) {
                written += char_writable.append('|');
            }
            write_delimeter = true;
            written += char_writable.append(e.key);
        }

        return written;
    }

    constexpr kf::usize reprValueImpl(auto &char_writable) const noexcept {
        for (auto const &e: _items) {
            if (e.value == _value) {
                return char_writable.append(e.key);
            }
        }

        return char_writable.append('?');
    }
};

}// namespace botix::internal

namespace botix::config {

struct Registry :

    kf::mixin::NonCopyable

{

    using EnumItem = internal::EnumField::Item;

    struct Field :

        internal::FieldBase<Field>,
        kf::mixin::ReprTo<Field>

    {

        enum class Kind : kf::u8 {
            Value,
            Enum,
        };

        constexpr Field(kf::StringView name, auto &value) noexcept :
            name{name}, _value{value}, _kind{Kind::Value} {}

        constexpr Field(kf::StringView name, kf::enum_type auto &value, kf::Slice<EnumItem const> items) noexcept :
            name{name}, _enum{value, items}, _kind{Kind::Enum} {}

        kf::StringView const name;

    private:
        union {
            internal::ValueField _value;
            internal::EnumField _enum;
        };

        Kind _kind;

        // impl for internal::Field
        friend struct ::botix::internal::FieldBase<Field>;

        bool setImpl(kf::StringView target_key) noexcept {
            switch (_kind) {
                case Kind::Value: return _value.set(target_key);
                case Kind::Enum: return _enum.set(target_key);
                default: return false;
            }
        }

        constexpr kf::usize reprTypeImpl(auto &char_writable) const noexcept {
            switch (_kind) {
                case Kind::Value: return _value.reprType(char_writable);
                case Kind::Enum: return _enum.reprType(char_writable);
                default: return 0;
            }
        }

        constexpr kf::usize reprValueImpl(auto &char_writable) const noexcept {
            switch (_kind) {
                case Kind::Value: return _value.reprValue(char_writable);
                case Kind::Enum: return _enum.reprValue(char_writable);
                default: return 0;
            }
        }

        KF_IMPL_REPR_TO(Field);
        constexpr kf::usize reprToImpl(auto &char_writable) const noexcept {
            return (
                char_writable.append(name) +
                char_writable.append(':') +
                char_writable.append(' ') +
                reprType(char_writable) +
                char_writable.append(' ') +
                char_writable.append('=') +
                char_writable.append(' ') +
                reprValue(char_writable));
        }
    };

    struct Error {
        kf::StringView message;
    };

    explicit constexpr Registry(kf::Slice<Field> fields) noexcept :
        _fields{fields} {}

    [[nodiscard]] auto all() noexcept {
        return _fields;
    }

    [[nodiscard]] auto get(kf::StringView name) noexcept {
        return _fields.firstWhere([name](auto const &field) {
            return field.name == name;
        });
    }

    [[nodiscard]] auto set(kf::StringView name, kf::StringView lexeme) noexcept -> kf::Result<void, Error> {

        if (auto field = get(name); field.isSome()) {
            if (field.unwrap().set(lexeme)) {
                return kf::ok();
            } else {
                return kf::error(Error{.message{"parsing failed"}});
            }
        }

        return kf::error(Error{.message{"field not found"}});
    }

private:
    kf::Slice<Field> _fields;
};

}// namespace botix::config