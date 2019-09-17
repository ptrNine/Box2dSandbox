#pragma once

#include <cstdint>

#if __GNUC__ == 8 && (__GNUC_MINOR__ < 3) && __GNUC__ != 9 || defined(__clang__)
    enum Byte : uint8_t {};
#else
    using Byte    = std::byte;
#endif

using U8      = std::uint8_t;
using U16     = std::uint16_t;
using U32     = std::uint32_t;
using U64     = std::uint64_t;

using S8      = std::int8_t;
using S16     = std::int16_t;
using S32     = std::int32_t;
using S64     = std::int64_t;

using Char8   = char;
using Char16  = char16_t;
using Char32  = char32_t;

using Float32 = float;
using Float64 = double;

using SizeT   = std::size_t;
using PtrDiff = std::ptrdiff_t;
