#pragma once

#include <type_traits>
#include <cstdint>

#define ICA inline constexpr auto

namespace details {
    template <typename T>
    struct _magic_constant_helper;
    template <>
    struct _magic_constant_helper<float> : public std::integral_constant<uint32_t, 0x5f3759df> {};
    template <>
    struct _magic_constant_helper<double> : public std::integral_constant<uint64_t, 0x5fe6eb50c7b537a9> {};



    template <typename T>
    struct _uint_flt_analog;
    template <>
    struct _uint_flt_analog<float>  { typedef uint32_t type; };
    template <>
    struct _uint_flt_analog<double> { typedef uint64_t type; };

    template <typename T>
    using uint_flt_analog_t = typename _uint_flt_analog<T>::type;


    template <typename T, std::size_t _step>
    ICA fis_step(T y, T x2) -> std::enable_if_t<(_step == 0), T> {
        return y;
    }

    template <typename T, std::size_t _step>
    ICA fis_step(T y, T x2) -> std::enable_if_t<(_step > 0), T> {
        return fis_step<T, _step - 1>(y * (1.5 - x2 * y * y), x2);
    }



    template <std::uint64_t _mask, std::uint64_t _num>
    struct _is_power_of_two;

    template <std::uint64_t _num>
    struct _is_power_of_two <1, _num> : public std::false_type {};

    template <>
    struct _is_power_of_two <0, 0> : public std::false_type {};

    template <>
    struct _is_power_of_two <1, 0> : public std::true_type {};

    template <std::uint64_t _num>
    struct _is_power_of_two <0, _num> : public _is_power_of_two<(_num & 0x1U), (_num >> 1U)> {};

}

namespace math {
    template <typename T>
    inline constexpr typename details::uint_flt_analog_t<T>
            magic_constant = details::_magic_constant_helper<T>::value;

    template <std::size_t _steps = 1, typename T>
    ICA fast_inv_sqrt(T val) {
        auto x2 = val * 0.5;
        auto y  = val;
        auto i  = *reinterpret_cast<details::uint_flt_analog_t<T>*>(&y);

        i  = magic_constant<T> - (i >> 1);
        y  = *reinterpret_cast<T*>(&i);
        return details::fis_step<T, _steps>(y, x2);
    }


    template <uint64_t _num>
    struct is_power_of_two : public details::_is_power_of_two<(_num & 0x1U), (_num >> 1U)> {};

    template <uint64_t _num>
    inline constexpr bool is_power_of_two_v = is_power_of_two<_num>::value;


} // namespace math


#undef ICA // inline constexpr auto