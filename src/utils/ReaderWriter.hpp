#pragma once

#include <string>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <sstream>
#include <variant>
#include <endian.h>
#include <scl/traits.hpp>


template <typename T>
static inline T byte_swap(const T& a) {
    if constexpr (sizeof(T) == 1) {
        return a;
    }
    else {
        T b = 0;
        auto buf = reinterpret_cast<uint8_t*>(&b);

        for (size_t i = 0; i < sizeof(T); ++i)
            *(buf + i) =
                    static_cast<uint8_t>((a) >> ((sizeof(T) - i - 1) << 3));

        return b;
    }
}


template <typename T>
static inline T cpu_to_real(const T& val) {
#if __BYTE_ORDER == __BIG_ENDIAN
    return byte_swap(val);
#elif __BYTE_ORDER == __LITTLE_ENDIAN
    return val;
#else
    #error "Can't determine byte order"
#endif
}


class Reader {
private:
    static constexpr const size_t CHUNK_SIZE = 16834;

public:
    Reader() : _is(nullptr) {}

    Reader(Reader&& r) noexcept;
    Reader(const void* data, size_t size);
    Reader(const std::string& path);

    Reader& operator=(Reader&& r) noexcept;

    bool is_file_streamer() const {
        return _buf.index() == 0;
    }

    bool is_buffer_streamer() const {
        return _buf.index() == 1;
    }

    size_t size() const {
        auto saved = _is.tellg();

        _is.seekg(0, std::ios::beg);
        auto start = _is.tellg();

        _is.seekg(0, std::ios::end);
        auto end = _is.tellg();

        _is.seekg(saved);

        return static_cast<size_t>(end - start);
    }

    size_t gcount() const {
        return static_cast<size_t>(_is.gcount());
    }

    size_t skip(size_t count) {
        _is.ignore(count);
        return static_cast<size_t>(_is.gcount());
    }

    void read(void* dst, size_t size) {
        _is.read(reinterpret_cast<char*>(dst), size);
    }

    template <typename T>
    auto read(T& value) -> std::enable_if_t<scl::is_number_v<T>, T> {
        read(&value, sizeof(T));
        return cpu_to_real(value);
    }

    template <typename T>
    auto read() -> std::enable_if_t<scl::is_number_v<T>, T> {
        T res;
        return read(res);
    }

    template <typename T>
    auto read(T& value) -> std::enable_if_t<std::is_enum_v<T>, T> {
        read(&value, sizeof(T));
        return static_cast<T>(cpu_to_real(static_cast<std::underlying_type_t<T>>(value)));
    }

    template <typename T>
    auto read() -> std::enable_if_t<std::is_enum_v<T>, T> {
        T res;
        return read(res);
    }

    template <template <typename...> class V, typename T>
    void read(V<T>& v, size_t count) {
        for (size_t i = 0; i < count; ++i)
            v[i] = read<T>();
    }

    template <typename V, typename T = typename V::value_type>
    auto read(size_t count) {
        V v(count, static_cast<T>(0));
        for (size_t i = 0; i < count; ++i)
            v[i] = read<T>();

        return std::move(v);
    }

    Reader& operator >>(class Writer& writer);
    Reader& operator >>(class Writer&& writer);

    template <typename V, typename T = typename V::value_type>
    auto operator >>(V& data) -> std::enable_if_t<sizeof(T) == 1, Reader&> {
        auto saved = _is.tellg();

        if (_is.eof()) {
            _is.clear();
            _is.seekg(0, std::ios::beg);
        }

        char* chunk[CHUNK_SIZE];

        do {
            read(chunk, CHUNK_SIZE);
            auto avail = gcount();

            size_t size = data.size();
            data.resize(size + avail);

            memcpy(data.data() + size, chunk, avail);
        } while(!_is.eof());

        _is.clear();
        _is.seekg(saved);

        return *this;
    }

private:
    mutable std::istream _is;
    std::variant<std::filebuf, std::stringbuf> _buf;
};


class Writer {
private:
    static constexpr const size_t CHUNK_SIZE = 16834;
    static constexpr auto _open_flags = std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc;

public:

    Writer& operator=(Writer&& w) noexcept;

    Writer();
    Writer(Writer&& w) noexcept;
    Writer(const std::string& path);

    bool is_file_streamer() const {
        return _buf.index() == 0;
    }

    bool is_buffer_streamer() const {
        return _buf.index() == 1;
    }

    void attach_to(const std::string& path);
    void detach();

    size_t size() const {
        auto saved = _os.tellp();

        _os.seekp(0, std::ios::beg);
        auto start = _os.tellp();

        _os.seekp(0, std::ios::end);
        auto end = _os.tellp();

        _os.seekp(saved);

        return static_cast<size_t>(end - start);
    }

    void zero_fill(size_t count) {
        auto zeros = std::string(count, 0);
        _os.write(zeros.data(), count);
    }

    void write(const void* src, size_t size) {
        _os.write(reinterpret_cast<const char*>(src), size);
    }

    template <typename T>
    auto write(const T& value) -> std::enable_if_t<scl::is_number_v<T>> {
        auto real = cpu_to_real(value);
        write(&real, sizeof(T));
    }

    template <typename T>
    auto write(const T& value) -> std::enable_if_t<std::is_enum_v<T>> {
        auto real = cpu_to_real(static_cast<std::underlying_type_t<T>>(value));
        write(&real, sizeof(T));
    }

    template <template <typename...> class V, typename T>
    void write(const V<T>& vec) {
        for (size_t i = 0; i < vec.size(); ++i)
            write(vec[i]);
    }

    template <typename V, typename T = typename V::value_type>
    auto operator >>(V& data) -> std::enable_if_t<sizeof(T) == 1, Writer&> {
        auto is = std::istream(nullptr);

        if (is_file_streamer())
            is.rdbuf(&std::get<std::filebuf>(_buf));
        else
            is.rdbuf(&std::get<std::stringbuf>(_buf));

        char chunk[CHUNK_SIZE];

        do {
            is.read(chunk, CHUNK_SIZE);
            auto avail = is.gcount();

            size_t size = data.size();
            data.resize(size + avail);

            memcpy(data.data() + size, chunk, avail);
        } while(!is.eof());

        return *this;
    }

private:
    mutable std::ostream _os;
    std::variant<std::filebuf, std::stringbuf> _buf;
};
