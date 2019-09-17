#pragma once

#include <type_traits>
#include <utility>

#include "../types.hpp"
#include "../math.hpp"
//#include "containers_base.hpp"
//#include "../concepts.hpp"
//#include "string.hpp"

#define ICA inline constexpr auto
#define IA inline auto

#ifndef VECTOR_FISR_ITERS_COUNT
#define VECTOR_FISR_ITERS_COUNT 3
#endif

namespace ftl {

    template<typename T>
    class Vector2 {
        static_assert(std::is_floating_point_v<T> || std::is_integral_v<T>, "Template type must be number");

    public:
        using ValType = T;

        constexpr Vector2() noexcept : _x(0), _y(0) {}

        constexpr Vector2(const Vector2& vec) noexcept
                : Vector2(vec._x, vec._y) {}

        constexpr Vector2(Vector2&& vec) noexcept
                : _x(std::move(vec._x)), _y(std::move(vec._y)) {}

        constexpr Vector2(T x, T y) noexcept : _x(x), _y(y) {}

        explicit constexpr
        Vector2(T val) noexcept : _x(val), _y(val) {}


    public:
        ICA& set(T x, T y) noexcept {
            _x = x;
            _y = y;
            return *this;
        }

        ICA& x() noexcept { return _x; }
        ICA& y() noexcept { return _y; }

        const ICA& x() const noexcept { return _x; }
        const ICA& y() const noexcept { return _y; }

        // Same to '+', '-' ... operators
        ICA add(const Vector2& r) const noexcept {
            return Vector2(_x + r._x, _y + r._y);
        }

        ICA sub(const Vector2& r) const noexcept {
            return Vector2(_x - r._x, _y - r._y);
        }

        ICA scalarAdd(const T& val) const noexcept {
            return Vector2(_x + val, _y + val);
        }

        ICA scalarSub(const T& val) const noexcept {
            return Vector2(_x - val, _y - val);
        }

        ICA scalarMul(const T& val) const noexcept {
            return Vector2(_x * val, _y * val);
        }

        ICA scalarDiv(const T& val) const {
            return Vector2(_x / val, _y / val);
        }

        // Same to '+=', '-=', ... operators
        ICA& makeAdd(const Vector2& r) noexcept {
            _x += r._x;
            _y += r._y;
            return *this;
        }

        ICA& makeSub(const Vector2& r) noexcept {
            _x -= r._x;
            _y -= r._y;
            return *this;
        }

        ICA& makeScalarAdd(const T& val) noexcept {
            _x += val;
            _y += val;
            return *this;
        }

        ICA& makeScalarSub(const T& val) noexcept {
            _x -= val;
            _y -= val;
            return *this;
        }

        ICA& makeScalarMul(const T& val) noexcept {
            _x *= val;
            _y *= val;
            return *this;
        }

        ICA& makeScalarDiv(const T& val) {
            _x /= val;
            _y /= val;
            return *this;
        }

        ICA dotProduct(const Vector2 &r) const noexcept { return _x * r._x + _y * r._y; }

        ICA magnitude2() const noexcept { return _x * _x + _y * _y; }


        // Format and print
        //auto to_string() const -> ftl::String {
        //    return ftl::String().sprintf("{} {}, {} {}", "{", _x, _y, "}");
        //}
//
        //void print(std::ostream& os = std::cout) const {
        //    os << to_string();
        //}

        friend std::ostream& operator<< (std::ostream& os, const Vector2& vec) { vec.print(os); return os; }

        // Operators

        // Equality
        ICA operator==(const Vector2& r) const noexcept { return _x == r._x && _y == r._y; }
        ICA operator!=(const Vector2& r) const noexcept { return !(*this == r); }

        // Vectors
        ICA  operator+(const Vector2& r) const noexcept { return add(r); }
        ICA  operator-(const Vector2& r) const noexcept { return sub(r); }

        ICA& operator+=(const Vector2& r) noexcept { return makeAdd(r); }
        ICA& operator-=(const Vector2& r) noexcept { return makeSub(r); }

        // Scalars
        ICA  operator+(const T& val) const noexcept { return scalarAdd(val); }
        ICA  operator-(const T& val) const noexcept { return scalarSub(val); }
        ICA  operator*(const T& val) const noexcept { return scalarMul(val); }
        ICA  operator/(const T& val) const          { return scalarDiv(val); }

        ICA& operator+=(const T& val) noexcept { return makeScalarAdd(val); }
        ICA& operator-=(const T& val) noexcept { return makeScalarSub(val); }
        ICA& operator*=(const T& val) noexcept { return makeScalarMul(val); }
        ICA& operator/=(const T& val)          { return makeScalarDiv(val); }

        ICA& operator=(const Vector2& r) noexcept {
            _x = r._x;
            _y = r._y;
            return *this;
        }

        template<std::size_t _pos>
        friend constexpr auto get(Vector2<T> &v) noexcept -> T & {
            if constexpr (_pos == 0) return v._x;
            return v._y;
        }

        template<std::size_t _pos>
        friend constexpr auto get(const Vector2<T> &v) noexcept -> const T & {
            if constexpr (_pos == 0) return v._x;
            return v._y;
        }

        template<std::size_t _pos>
        friend constexpr auto get(Vector2<T> &&v) noexcept -> T && {
            if constexpr (_pos == 0) return std::move(v._x);
            return std::move(v._y);
        }

    protected:
        T _x, _y;
    };
} // namespace ftl

namespace std {

    template<typename Type>
    struct tuple_size<ftl::Vector2<Type>> : public integral_constant<std::size_t, 2> {
    };

    template<std::size_t _pos, typename Type>
    struct tuple_element<_pos, ftl::Vector2<Type>> {
        using type = Type;
    };
}

namespace ftl {
    template<typename Type>
    class Vector2Flt : public Vector2<Type> {
        using inherited = Vector2<Type>;
    public:
        constexpr Vector2Flt() noexcept : inherited() {}

        constexpr Vector2Flt(const Vector2Flt& vec) noexcept
                : inherited(vec._x, vec._y) {}

        constexpr Vector2Flt(Vector2Flt&& vec) noexcept
                : inherited(std::move(vec._x), std::move(vec._y)) {}

        constexpr Vector2Flt(Type x, Type y) noexcept : inherited(x, y) {}

        explicit constexpr
        Vector2Flt(Type val) noexcept : inherited(val) {}


        template<std::size_t _steps = 1>
        IA fastInvMagnitude() const { return math::fast_inv_sqrt<_steps>(this->magnitude2()); }

        template<std::size_t _steps = 1>
        IA fastMagnitude() const { return 1 / fastInvMagnitude<_steps>(); }
        IA magnitude    () const { return 1 / fastInvMagnitude<VECTOR_FISR_ITERS_COUNT>(); }

        template<std::size_t _steps = 1>
        IA& makeFastNormalize()  { return ((*this) *= fastInvMagnitude<_steps>()); }
        IA& makeNormalize()      { return ((*this) *= fastInvMagnitude<VECTOR_FISR_ITERS_COUNT>()); }

        template<std::size_t _steps = 1>
        IA fastNormalize() const { Vector2Flt((*this) * fastInvMagnitude<_steps>()); }
        IA normalize    () const { return inherited::scalarMul(fastInvMagnitude<VECTOR_FISR_ITERS_COUNT>()); }

        ICA& operator=(const Vector2Flt& r) noexcept {
            this->_x = r._x;
            this->_y = r._y;
            return *this;
        }

        /*
        IA& makeRawPerpendicular() {
            using inh = inherited;
            inh::_x = 1;
            inh::_y = -inh::_x / inh::_y;

            return std::isinf(inh::_y) ?
                inh::set(0, 1) :
                *this;
        };

        IA& makePerpendicular() {
            return makeRawPerpendicular().makeNormalize();
        }

        ICA rawPerpendicular() const {
            using inh = inherited;
            Type x = 1;
            Type y = -inh::_x / inh::_y;

            return std::isinf(y) ?
                Vector2Flt(0, 1) :
                Vector2Flt(x, y);
        }

        IA perpendicular() const {
            return rawPerpendicular().makeNormalize();
        }
         */
    };

} // namespace ftl


namespace std {
    template <typename Type>
    struct tuple_size<ftl::Vector2Flt<Type>> : public integral_constant<std::size_t, 2> {};

    template <size_t _pos, typename Type>
    struct tuple_element<_pos, ftl::Vector2Flt<Type>> { using type = Type; };
}


namespace ftl {
    using Vector2u   = Vector2<unsigned long>;
    using Vector2i   = Vector2<long>;

    using Vector2u8  = Vector2<U8>;
    using Vector2u16 = Vector2<U16>;
    using Vector2u32 = Vector2<U32>;
    using Vector2u64 = Vector2<U64>;
    using Vector2s8  = Vector2<S8>;
    using Vector2s16 = Vector2<S16>;
    using Vector2s32 = Vector2<S32>;
    using Vector2s64 = Vector2<S64>;

    using Vector2f   = Vector2Flt<Float32>;
    using Vector2d   = Vector2Flt<Float64>;
    using Vector2f32 = Vector2Flt<Float32>;
    using Vector2f64 = Vector2Flt<Float64>;

    template <typename Type>
    ICA Vector2T(Type x, Type y) -> std::enable_if_t<std::is_integral_v<Type>, Vector2<Type>> {
    return Vector2<Type>(x, y);
}
template <typename T>
ICA Vector2T(T x, T y) -> std::enable_if_t<std::is_floating_point_v<T>, Vector2Flt<T>> {
    return Vector2Flt<T>(x, y);
}

template <typename OutType, typename InType, std::enable_if_t<std::is_integral_v<InType>>...>
ICA Vector2Convert(const Vector2<InType>& vec) {
    return Vector2T<OutType>(vec.x(), vec.y());
}
template <typename OutType, typename InType, std::enable_if_t<std::is_floating_point_v<InType>>...>
ICA Vector2Convert(const Vector2Flt<InType>& vec) {
    return Vector2T<OutType>(vec.x(), vec.y());
}

} // namespace ftl

// fmt format
/*
template <typename Type>
struct fmt::formatter<ftl::Vector2<Type>> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext &ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const ftl::Vector2<Type>& vec, FormatContext& ctx) {
        return format_to(ctx.out(), "{}", vec.to_string());
    }
};

template <typename Type>
struct fmt::formatter<ftl::Vector2Flt<Type>> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext &ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const ftl::Vector2Flt<Type>& vec, FormatContext& ctx) {
        return format_to(ctx.out(), "{}", vec.to_string());
    }
};
*/

#undef ICA // inline constexpr auto
#undef ICA // inline auto