#pragma once

#include <string>
#include <fstream>
#include <cstring>

namespace nnw {
    using StringT = std::string;

    template <typename T>
    T swap_endian(const T& a) {
        T b = 0;
        auto buf = reinterpret_cast<uint8_t*>(&b);

        for (size_t i = 0; i < sizeof(T); ++i)
            *(buf + i) =
                    static_cast<uint8_t>((a) >> ((sizeof(T) - i - 1) << 3));

        return b;
    }

    class FileSerializer {
    public:
        FileSerializer(const StringT& path): _ofs(path, std::ios::out | std::ios::binary) {
            if (!_ofs.is_open())
                throw "Can't open file " + path;
        }

        template <typename T>
        void push(const T* ptr, size_t size) {
            _ofs.write(reinterpret_cast<const char*>(ptr), sizeof(T) * size);
        }

        template <typename T>
        void push(const T& val) {
            _ofs.write(reinterpret_cast<const char*>(&val), sizeof(T));
        }

        void zero_fill(size_t size) {
            while(size--)
                _ofs.put(0);
        }

    private:
        std::ofstream _ofs;
    };

    class FileDeserializer {
    public:
        FileDeserializer(const StringT& path): _ifs(path, std::ios::in | std::ios::binary) {
            if (!_ifs.is_open())
                throw "Can't open file " + path;
        }

        template <typename T>
        T pop() {
            T res;
            _ifs.read(reinterpret_cast<char*>(&res), sizeof(T));
            return res;
        }

        template <typename T>
        void pop(T& res) {
            _ifs.read(reinterpret_cast<char*>(&res), sizeof(T));
        }

        template <typename T>
        void pop(T* res, size_t size) {
            _ifs.read(reinterpret_cast<char*>(res), sizeof(T) * size);
        }

        void skip(size_t count) {
            _ifs.ignore(count);
        }

    private:
        std::ifstream _ifs;
    };


    class Serializer {
    public:
        template <typename T>
        void push(const T* ptr, size_t size) {
            for (size_t i = 0; i < size * sizeof(T); ++i)
                _data.push_back(reinterpret_cast<const char*>(ptr)[i]);
        }

        template <typename T>
        void push(T val) {
            push(&val, 1);
        }

        template <typename T>
        static constexpr bool is_char = std::is_same_v<std::remove_const_t<T>, char> ||
                                        std::is_same_v<std::remove_const_t<T>, unsigned char>;

        template <typename T, size_t _Sz>
        auto push(T(&array)[_Sz]) -> std::enable_if_t<!is_char<T>> {
            for (size_t i = 0; i < _Sz; ++i)
                push(array[i]);
        }

        template <typename T, size_t _Sz>
        auto push(T(&array)[_Sz]) -> std::enable_if_t<is_char<T>> {
            for (size_t i = 0; i < _Sz - 1; ++i)
                push(array[i]);
        }

        void zero_fill(size_t size) {
            auto pos = _data.size();
            _data.resize(_data.size() + size, 0);
        }

        void write(const StringT& path) {
            auto a = FileSerializer(path);
            a.push(_data.data(), _data.size());
        }

    private:
        std::vector<char> _data;
    };
}