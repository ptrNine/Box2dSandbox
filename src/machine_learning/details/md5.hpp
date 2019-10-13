#pragma once

#include <cstdint>
#include <ostream>
#include <iomanip>
#include <vector>
#include <cstring>

#define MD5_ROUND0_15(r, N, K)                    \
    F = (b & c) | ((~b) & d);                     \
    dTemp = d;                                    \
    d = c;                                        \
    c = b;                                        \
    b += dtls::md5_rotl(a + F + K + words[r], N); \
    a = dTemp

#define MD5_ROUND16_31(W, N, K)                   \
    F = (d & b) | ((~d) & c);                     \
    dTemp = d;                                    \
    d = c;                                        \
    c = b;                                        \
    b += dtls::md5_rotl(a + F + K + words[W], N); \
    a = dTemp

#define MD5_ROUND32_47(W, N, K)                   \
    F = b ^ c ^ d;                                \
    dTemp = d;                                    \
    d = c;                                        \
    c = b;                                        \
    b += dtls::md5_rotl(a + F + K + words[W], N); \
    a = dTemp

#define MD5_ROUND48_63(W, N, K)                   \
    F = c ^ (b | (~d));                           \
    dTemp = d;                                    \
    d = c;                                        \
    c = b;                                        \
    b += dtls::md5_rotl(a + F + K + words[W], N); \
    a = dTemp

#define MD5_ROUND1()                       \
    MD5_ROUND0_15(0,  7,  0xd76aa478);     \
    MD5_ROUND0_15(1,  12, 0xe8c7b756);     \
    MD5_ROUND0_15(2,  17, 0x242070db);     \
    MD5_ROUND0_15(3,  22, 0xc1bdceee);     \
    MD5_ROUND0_15(4,  7,  0xf57c0faf);     \
    MD5_ROUND0_15(5,  12, 0x4787c62a);     \
    MD5_ROUND0_15(6,  17, 0xa8304613);     \
    MD5_ROUND0_15(7,  22, 0xfd469501);     \
    MD5_ROUND0_15(8,  7,  0x698098d8);     \
    MD5_ROUND0_15(9,  12, 0x8b44f7af);     \
    MD5_ROUND0_15(10, 17, 0xffff5bb1);     \
    MD5_ROUND0_15(11, 22, 0x895cd7be);     \
    MD5_ROUND0_15(12, 7,  0x6b901122);     \
    MD5_ROUND0_15(13, 12, 0xfd987193);     \
    MD5_ROUND0_15(14, 17, 0xa679438e);     \
    MD5_ROUND0_15(15, 22, 0x49b40821)

#define MD5_ROUND2()                       \
    MD5_ROUND16_31(1,  5,  0xf61e2562);    \
    MD5_ROUND16_31(6,  9,  0xc040b340);    \
    MD5_ROUND16_31(11, 14, 0x265e5a51);    \
    MD5_ROUND16_31(0,  20, 0xe9b6c7aa);    \
    MD5_ROUND16_31(5,  5,  0xd62f105d);    \
    MD5_ROUND16_31(10, 9,  0x02441453);    \
    MD5_ROUND16_31(15, 14, 0xd8a1e681);    \
    MD5_ROUND16_31(4,  20, 0xe7d3fbc8);    \
    MD5_ROUND16_31(9,  5,  0x21e1cde6);    \
    MD5_ROUND16_31(14, 9,  0xc33707d6);    \
    MD5_ROUND16_31(3,  14, 0xf4d50d87);    \
    MD5_ROUND16_31(8,  20, 0x455a14ed);    \
    MD5_ROUND16_31(13, 5,  0xa9e3e905);    \
    MD5_ROUND16_31(2,  9,  0xfcefa3f8);    \
    MD5_ROUND16_31(7,  14, 0x676f02d9);    \
    MD5_ROUND16_31(12, 20, 0x8d2a4c8a)

#define MD5_ROUND3()                       \
    MD5_ROUND32_47(5,  4,  0xfffa3942);    \
    MD5_ROUND32_47(8,  11, 0x8771f681);    \
    MD5_ROUND32_47(11, 16, 0x6d9d6122);    \
    MD5_ROUND32_47(14, 23, 0xfde5380c);    \
    MD5_ROUND32_47(1,  4,  0xa4beea44);    \
    MD5_ROUND32_47(4,  11, 0x4bdecfa9);    \
    MD5_ROUND32_47(7,  16, 0xf6bb4b60);    \
    MD5_ROUND32_47(10, 23, 0xbebfbc70);    \
    MD5_ROUND32_47(13, 4,  0x289b7ec6);    \
    MD5_ROUND32_47(0,  11, 0xeaa127fa);    \
    MD5_ROUND32_47(3,  16, 0xd4ef3085);    \
    MD5_ROUND32_47(6,  23, 0x04881d05);    \
    MD5_ROUND32_47(9,  4,  0xd9d4d039);    \
    MD5_ROUND32_47(12, 11, 0xe6db99e5);    \
    MD5_ROUND32_47(15, 16, 0x1fa27cf8);    \
    MD5_ROUND32_47(2,  23, 0xc4ac5665)

#define MD5_ROUND4()                       \
    MD5_ROUND48_63(0,  6,  0xf4292244);    \
    MD5_ROUND48_63(7,  10, 0x432aff97);    \
    MD5_ROUND48_63(14, 15, 0xab9423a7);    \
    MD5_ROUND48_63(5,  21, 0xfc93a039);    \
    MD5_ROUND48_63(12, 6,  0x655b59c3);    \
    MD5_ROUND48_63(3,  10, 0x8f0ccc92);    \
    MD5_ROUND48_63(10, 15, 0xffeff47d);    \
    MD5_ROUND48_63(1,  21, 0x85845dd1);    \
    MD5_ROUND48_63(8,  6,  0x6fa87e4f);    \
    MD5_ROUND48_63(15, 10, 0xfe2ce6e0);    \
    MD5_ROUND48_63(6,  15, 0xa3014314);    \
    MD5_ROUND48_63(13, 21, 0x4e0811a1);    \
    MD5_ROUND48_63(4,  6,  0xf7537e82);    \
    MD5_ROUND48_63(11, 10, 0xbd3af235);    \
    MD5_ROUND48_63(2,  15, 0x2ad7d2bb);    \
    MD5_ROUND48_63(9,  21, 0xeb86d391)


namespace md5 {

    namespace dtls {
        inline uint32_t md5_rotl(uint32_t x, uint32_t n) {
            return (x << n) | (x >> (32U - n));
        }
    }

    struct Block128 {
        friend std::ostream &operator<<(std::ostream &os, const Block128 &b) {
            for (std::size_t i = 0; i < 8; ++i)
                os << std::hex << std::setfill('0') << std::setw(2) <<
                   static_cast<uint32_t>((static_cast<uint8_t>(b.lo >> 8 * i)));
            for (std::size_t i = 0; i < 8; ++i)
                os << std::hex << std::setfill('0') << std::setw(2) <<
                   static_cast<uint32_t>((static_cast<uint8_t>(b.hi >> 8 * i)));

            // Big endian
#if 0
            os << std::hex << std::setfill('0') << std::setw(16) << b.lo;
            os << std::hex << std::setfill('0') << std::setw(16) << b.hi;
#endif
            return os;
        }

        bool operator==(const Block128& b) const {
            return lo == b.lo && hi == b.hi;
        }

        bool operator!=(const Block128& b) const {
            return !(*this == b);
        }

        bool operator<(const Block128& b) const {
            if (hi < b.hi)
                return true;
            else if (hi == b.hi)
                return lo < b.lo;
            else
                return false;
        }

        inline uint32_t& operator[](std::size_t n) {
            return reinterpret_cast<uint32_t*>(this)[n];
        }

        uint64_t lo = 0;
        uint64_t hi = 0;
    };


    inline Block128 md5(const uint8_t* str, size_t size) {
        size_t blocks            = ((size + 8U) >> 6U) + 1U;
        size_t size_with_padding = blocks << 6U;

        auto data = std::vector<uint8_t>(size_with_padding, 0);
        std::memcpy(data.data(), str, size);

        data[size] = static_cast<char>(0x80);

        auto input_bits = static_cast<uint64_t>(size) << 3U;

        *reinterpret_cast<uint64_t*>(&data[(blocks << 6U) - 8U]) = input_bits;

        // Big endian
#if 0
        *reinterpret_cast<uint64_t*>(&data[(blocks << 6U) - 8U]) = swap_endian(input_bits);
#endif

        uint32_t a0 = 0x67452301;
        uint32_t b0 = 0xEFCDAB89;
        uint32_t c0 = 0x98BADCFE;
        uint32_t d0 = 0x10325476;

        uint32_t* bytes_ptr;
        uint32_t  words[16U];

        for (size_t cb = 0; cb < blocks; ++cb) {
            bytes_ptr = (uint32_t*)(&(data[cb << 6U]));

            words[0]  = bytes_ptr[0];
            words[1]  = bytes_ptr[1];
            words[2]  = bytes_ptr[2];
            words[3]  = bytes_ptr[3];
            words[4]  = bytes_ptr[4];
            words[5]  = bytes_ptr[5];
            words[6]  = bytes_ptr[6];
            words[7]  = bytes_ptr[7];
            words[8]  = bytes_ptr[8];
            words[9]  = bytes_ptr[9];
            words[10] = bytes_ptr[10];
            words[11] = bytes_ptr[11];
            words[12] = bytes_ptr[12];
            words[13] = bytes_ptr[13];
            words[14] = bytes_ptr[14];
            words[15] = bytes_ptr[15];

            uint32_t a = a0;
            uint32_t b = b0;
            uint32_t c = c0;
            uint32_t d = d0;
            uint32_t F;
            uint32_t dTemp;

            MD5_ROUND1();
            MD5_ROUND2();
            MD5_ROUND3();
            MD5_ROUND4();

            a0 += a;
            b0 += b;
            c0 += c;
            d0 += d;
        }

        Block128 hash;

        hash[0] = a0;
        hash[1] = b0;
        hash[2] = c0;
        hash[3] = d0;

        return hash;
    }

    Block128 md5(const char* str) {
        return md5(reinterpret_cast<const uint8_t*>(str), strlen(str));
    }

} // namespace md5

#undef MD5_ROUND0_15
#undef MD5_ROUND16_31
#undef MD5_ROUND32_47
#undef MD5_ROUND48_63

#undef MD5_ROUND1
#undef MD5_ROUND2
#undef MD5_ROUND3
#undef MD5_ROUND4