// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// Botix - https://github.com/KiraFlux/botix

#pragma once

#include <kf/Option.hpp>
#include <kf/Slice.hpp>
#include <kf/StringView.hpp>
#include <kf/core.hpp>

#include <kf/mixin/NonCopyable.hpp>
#include <kf/mixin/ReprTo.hpp>
#include <kf/mixin/Resettable.hpp>

#include "botix/Parser.hpp"

#include "botix/cli/Channel.hpp"
#include "botix/cli/Identifier.hpp"

namespace botix::internal {

template<typename T> struct ArgumentValueItem;

template<typename T> struct ArgumentValueItemBase :

    cli::Identifier,
    kf::mixin::ReprTo<ArgumentValueItemBase<T>>

{
private:
    KF_IMPL_REPR_TO(ArgumentValueItemBase<T>);
    constexpr kf::usize reprToImpl(auto &char_writable) const noexcept {
        return char_writable.append(this->name);
    }
};

template<kf::trivial T> struct ArgumentValueItem<T> : ArgumentValueItemBase<T> {
    using ValueType = T;

    constexpr ArgumentValueItem(cli::Identifier id, ValueType value) noexcept :
        ArgumentValueItemBase<T>{id}, _value{value} {}

    [[nodiscard]] constexpr T value() const noexcept {
        return _value;
    }

private:
    ValueType _value;
};

template<> struct ArgumentValueItem<kf::StringView> : ArgumentValueItemBase<kf::StringView> {
    using ValueType = kf::StringView;

    constexpr ArgumentValueItem(cli::Identifier id) noexcept :
        ArgumentValueItemBase<kf::StringView>{id} {}

    [[nodiscard]] constexpr kf::StringView value() const noexcept {
        return this->name;
    }
};

struct ArgumentBase {

protected:
    struct ParseContext {
        cli::Channel::Output &channel_output;
        kf::StringView lexeme;// not empty

        template<kf::implements<cli::Identifier> T> [[nodiscard]] constexpr auto parseEnumerated(kf::Slice<T const> items) const noexcept -> kf::Option<T const &> {
            auto maybe_item = items.firstWhere([this](auto const &item) { return item.match(lexeme); });
            if (maybe_item.isNone()) {
                channel_output.error("'{}' not allowed, use: {}", lexeme, items);
            }
            return maybe_item;
        }
    };

private:
    // TODO: make as kf::mixin::Parsable<Impl, InputType, OutputType> static interface
    template<typename Impl> struct Parsable {
        [[nodiscard]] constexpr bool parse(ParseContext const &context) noexcept {
            return static_cast<Impl *>(this)->parseImpl(context);
        }
    };

    template<kf::trivial T> struct Parameters : kf::mixin::Resettable<Parameters<T>> {
        T value{};
        kf::Option<T> default_value;

    private:
        KF_IMPL_RESETTABLE(Parameters<T>);
        constexpr void resetImpl() noexcept {
            if (default_value.isSome()) {
                value = default_value.unwrap();
            }
        }
    };

    template<typename Impl, kf::trivial ParametersType> struct Value : ParametersType, Parsable<Impl> {
        constexpr Value(ParametersType params) noexcept :
            ParametersType{params} {}
    };

    template<kf::arithmetic T> struct NumberParameters {

        using ValueType = T;

        Parameters<T> params;
        kf::Option<T> min_value, max_value;
    };

    template<typename T> struct NumberValue : Value<NumberValue<T>, T> {

        using Value<NumberValue, T>::Value;

    private:
        friend struct Parsable<NumberValue<T>>;
        constexpr bool parseImpl(ParseContext const &context) noexcept {
            Parser<typename T::ValueType> parser{};
            auto const maybe_number = parser.parse(context.lexeme);

            if (maybe_number.isNone()) {
                context.channel_output.error("'{}' cannot be interpreted as number", context.lexeme);
                return false;
            }

            if (this->min_value.isSome() and maybe_number.unwrap() < this->min_value.unwrap()) {
                context.channel_output.error("'{}' is lower than minimal value ({})", context.lexeme, this->min_value.unwrap());
                return false;
            }

            if (this->max_value.isSome() and maybe_number.unwrap() > this->max_value.unwrap()) {
                context.channel_output.error("'{}' is higher than maximal value ({})", context.lexeme, this->max_value.unwrap());
                return false;
            }

            this->params.value = maybe_number.unwrap();
            return true;
        }
    };

    struct EnumItem : ArgumentValueItem<kf::usize> {

        constexpr EnumItem(Identifier id, kf::enum_type auto value) noexcept :
            ArgumentValueItem<kf::usize>{id, static_cast<kf::usize>(value)} {
            static_assert(sizeof(value) <= sizeof(kf::usize));
        }
    };

    using BooleanItem = ArgumentValueItem<bool>;

public:
    using Integer = NumberParameters<kf::i32>;

    using Real = NumberParameters<kf::f32>;

    struct Enum {

        using Item = EnumItem;

        Parameters<Item::ValueType> params;
        kf::Slice<Item const> items;
    };

    struct Boolean {
        Parameters<BooleanItem::ValueType> params;
    };

    struct String {
        Parameters<kf::StringView> params;
    };

protected:
    using IntegerValue = NumberValue<Integer>;

    using RealValue = NumberValue<Real>;

    struct EnumValue : Value<EnumValue, Enum> {

        using Value<EnumValue, Enum>::Value;

    private:
        friend struct Parsable<EnumValue>;
        constexpr bool parseImpl(ParseContext const &context) noexcept {
            if (auto maybe_item = context.parseEnumerated(items); maybe_item.isSome()) {
                this->params.value = maybe_item.unwrap().value();
                return true;
            }
            return false;
        }
    };

    struct BooleanValue : Value<BooleanValue, Boolean> {

        using Value<BooleanValue, Boolean>::Value;

    private:
        friend struct Parsable<BooleanValue>;
        constexpr bool parseImpl(ParseContext const &context) noexcept {

            BooleanItem const items[4]{
                {{.name = "true"}, true},
                {{.name = "false"}, false},
                {{.name = "yes"}, true},
                {{.name = "no"}, false},
            };

            if (auto maybe_item = context.parseEnumerated<BooleanItem>(items); maybe_item.isSome()) {
                this->params.value = maybe_item.unwrap().value();
                return true;
            }

            return false;
        }
    };

    struct StringValue : Value<StringValue, String> {

        using Value<StringValue, String>::Value;

    private:
        friend struct Parsable<StringValue>;
        constexpr bool parseImpl(ParseContext const &context) noexcept {
            this->params.value = context.lexeme;
            return true;
        }
    };
};

}// namespace botix::internal

namespace botix::cli {

// TODO: make as dyn interface. default getters returns safe mock values.
struct Argument :

    internal::ArgumentBase,
    Identifier,
    kf::mixin::NonCopyable,
    kf::mixin::Resettable<Argument>,
    kf::mixin::ReprTo<Argument>

{

    enum class Kind : kf::u8 {
        Enum,
        Boolean,
        Integer,
        Real,
        String,// TODO: add IPv4, Mac
    };

    // construct

    constexpr Argument(Identifier id, Enum params) noexcept :
        Identifier{id}, _enum{params}, _kind{Kind::Enum} {}

    constexpr Argument(Identifier id, Boolean params) noexcept :
        Identifier{id}, _boolean{params}, _kind{Kind::Boolean} {}

    constexpr Argument(Identifier id, Integer params) noexcept :
        Identifier{id}, _integer{params}, _kind{Kind::Integer} {}

    constexpr Argument(Identifier id, Real params) noexcept :
        Identifier{id}, _real{params}, _kind{Kind::Real} {}

    constexpr Argument(Identifier id, String params) noexcept :
        Identifier{id}, _string{params}, _kind{Kind::String} {}

    // get

    [[nodiscard]] constexpr auto enumIndex() const noexcept {
        return _enum.params.value;
    }

    template<kf::enum_type E> [[nodiscard]] constexpr auto enumValue() const noexcept {
        return static_cast<E>(enumIndex());
    }

    [[nodiscard]] constexpr auto enumName() const noexcept {
        return _enum.items[enumIndex()].name;// TODO: return firstWhere same name
    }

    [[nodiscard]] constexpr auto boolean() const noexcept {
        return _boolean.params.value;
    }

    [[nodiscard]] constexpr auto integer() const noexcept {
        return _integer.params.value;
    }

    [[nodiscard]] constexpr auto real() const noexcept {
        return _real.params.value;
    }

    [[nodiscard]] constexpr auto string() const noexcept {
        return _string.params.value;
    }

    // properties

    [[nodiscard]] constexpr auto kind() const noexcept {
        return _kind;
    }

    [[nodiscard]] constexpr bool positional() const noexcept {
        auto const is_positional = [](auto const &x) {
            return x.params.default_value.isNone();
        };
        switch (_kind) {
            case Kind::Enum: return is_positional(_enum);
            case Kind::Boolean: return is_positional(_boolean);
            case Kind::Integer: return is_positional(_integer);
            case Kind::Real: return is_positional(_real);
            case Kind::String: return is_positional(_string);
            default: return true;
        }
    }

    [[nodiscard]] constexpr bool parse(ParseContext const &context) noexcept {
        switch (_kind) {
            case Kind::Enum: return _enum.parse(context);
            case Kind::Boolean: return _boolean.parse(context);
            case Kind::Integer: return _integer.parse(context);
            case Kind::Real: return _real.parse(context);
            case Kind::String: return _string.parse(context);
            default: return false;
        };
    }

private:
    union {
        EnumValue _enum;
        BooleanValue _boolean;
        IntegerValue _integer;
        RealValue _real;
        StringValue _string;
    };

    Kind _kind;

    template<kf::implements<Identifier> T> static constexpr kf::usize reprList(auto &char_writable, kf::Slice<T const> items) noexcept {
        bool f = false;
        kf::usize write_count = 0;
        for (auto const &item: items) {
            if (f) {
                write_count += char_writable.append('|');
            }
            f = true;
            write_count += char_writable.append(item.name);
        }
        return write_count;
    }

    constexpr kf::usize reprType(auto &char_writable) const noexcept {
        switch (_kind) {
            case Kind::Enum: return reprList(char_writable, _enum.items);
            case Kind::Boolean: return char_writable.append("bool");
            case Kind::Integer: return char_writable.append("int");
            case Kind::Real: return char_writable.append("float");
            case Kind::String: return char_writable.append("str");
        }
        return 0;
    }

    constexpr kf::usize reprDefault(auto &char_writable) const noexcept {
        switch (_kind) {
            case Kind::Enum: return char_writable.append(_enum.params.default_value.unwrap());
            case Kind::Boolean: return char_writable.append(_boolean.params.default_value.unwrap());
            case Kind::Integer: return char_writable.append(_integer.params.default_value.unwrap());
            case Kind::Real: return char_writable.append(_real.params.default_value.unwrap());
            case Kind::String: return (
                char_writable.append('"') +
                char_writable.append(_string.params.default_value.unwrap()) +
                char_writable.append('"'));
        }
        return 0;
    }

    KF_IMPL_REPR_TO(Argument);
    constexpr kf::usize reprToImpl(auto &char_writable) const noexcept {
        bool const has_default = not positional();

        kf::usize write_count = char_writable.append(has_default ? '[' : '<');
        write_count += char_writable.append(this->name);
        write_count += char_writable.append(':');
        write_count += char_writable.append(' ');

        write_count += this->reprType(char_writable);

        if (has_default) {
            write_count += char_writable.append(' ');
            write_count += char_writable.append('=');
            write_count += char_writable.append(' ');
            write_count += reprDefault(char_writable);
        }

        write_count += char_writable.append(has_default ? ']' : '>');
        return write_count;
    }

    KF_IMPL_RESETTABLE(Argument);
    void resetImpl() noexcept {
        switch (_kind) {
            case Kind::Enum: _enum.params.reset(); break;
            case Kind::Boolean: _boolean.params.reset(); break;
            case Kind::Integer: _integer.params.reset(); break;
            case Kind::Real: _real.params.reset(); break;
            case Kind::String: _string.params.reset(); break;
        };
    }
};

}// namespace botix::cli