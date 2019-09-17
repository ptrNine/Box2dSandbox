#include <type_traits>
#include <cstdint>

#define IC inline constexpr

/**
 * Flag type implementation
 *
 * example:
 *
 * DEF_FLAG_TYPE(Flagger, Flags8,
        A = Flagger::def<0>,
        B = Flagger::def<1>,
        C = Flagger::def<2>,
        D = Flagger::def<3>,
        E = Flagger::def<4>,
        F = Flagger::def<5>,
        G = Flagger::def<6>,
        H = Flagger::def<7>,
        // I = Flagger::def<8>   <- compilation error, because Flag8 is 8-bit type
    );

    Flagger flag;

    flag.set(Flagger::A);
    flag.set(flag.B); // also works

    if (flag.test(Flagger::A | Flagger::B)) {
        std::cout << "Success!" << std::endl;
    }
 */

template <typename Type>
class FlagsTmpl {
    static_assert(std::is_integral_v<Type> && std::is_unsigned_v<Type>,
                 "Flag type must be unsigned int!");
public:
    using IntType = Type;

    template <Type flag>
    struct _define_helper {
        static_assert(flag < sizeof(Type) * 8, "Can't define that bit!");
        inline static constexpr Type def = Type(1) << flag;
    };

    template <Type flag>
    inline static constexpr Type def = _define_helper<flag>::def;


public:
    FlagsTmpl(): _data(0) {}

    IC void set    (Type flags) noexcept       { _data |= flags; }
    IC void set_if (Type flags, bool expr)     { if (expr) set(flags); }
    IC bool test   (Type flags) const noexcept { return _data & flags; }
    IC void reset  () noexcept                 { _data = 0; }
    IC Type data   () const noexcept           { return _data; }

private:
    Type _data;
};

using Flags8  = FlagsTmpl<uint8_t>;
using Flags16 = FlagsTmpl<uint16_t>;
using Flags32 = FlagsTmpl<uint32_t>;
using Flags64 = FlagsTmpl<uint64_t>;
using Flags   = FlagsTmpl<size_t>;


#define DEF_FLAG_TYPE(NAME, TYPE, ...) \
struct NAME : TYPE {                   \
    NAME() : TYPE() {}                 \
    enum : TYPE::IntType {             \
        __VA_ARGS__                    \
    };                                 \
}


#undef IC


