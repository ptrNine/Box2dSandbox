#pragma once

#include <string>
#include <fstream>
#include <cstring>

namespace fft {
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
                throw std::runtime_error("Can't open file '" + path + "'");
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
                throw std::runtime_error("Can't open file '" + path + "'");
        }

        template <typename T>
        T pop() {
            T res;
            _ifs.read(reinterpret_cast<char*>(&res), sizeof(T));
            return res;
        }

        template <typename T>
        bool pop(T& res) {
            _ifs.read(reinterpret_cast<char*>(&res), sizeof(T));
            return _ifs.gcount() == sizeof(T);
        }

        template <typename T>
        bool pop(T* res, size_t size) {
            _ifs.read(reinterpret_cast<char*>(res), sizeof(T) * size);
            return _ifs.gcount() == sizeof(T) * size;
        }

        bool skip(size_t count) {
            _ifs.ignore(count);
            return _ifs.gcount() == count;
        }

        auto gcount() {
            return _ifs.gcount();
        }

        bool is_end() const {
            return _ifs.eof();
        }

        template <typename ContainerT = std::vector<uint8_t>>
        auto read_all_buffered(size_t buffer_size = 1024) {
            auto res = ContainerT();

            size_t old_size = 0;

            do {
                old_size = res.size();
                res.resize(res.size() + buffer_size);
                pop(res.data() + old_size, buffer_size);
            }
            while (gcount() == buffer_size);

            res.resize(res.size() - (buffer_size - gcount()));
            pop(res.data() + old_size, gcount());

            return res;
        }

    private:
        std::ifstream _ifs;
    };


    class Serializer {
    public:
        Serializer() = default;
        Serializer(Serializer&& sr) noexcept : _data(std::move(sr._data)) {}

        template <typename T>
        void push(const T* ptr, size_t size) {
            for (size_t i = 0; i < size * sizeof(T); ++i)
                _data.push_back(reinterpret_cast<const uint8_t*>(ptr)[i]);
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

        void save_to(const StringT &path) {
            auto a = FileSerializer(path);
            a.push(_data.data(), _data.size());
        }

        size_t size() const {
            return _data.size();
        }

        auto unsafe_data() {
            return _data.data();
        }

        auto& unsafe_vector() {
            return _data;
        }

    private:
        std::vector<uint8_t> _data;
    };


    class Deserializer {
    public:
        explicit Deserializer(std::vector<uint8_t> data) : _data(std::move(data)) {}
        explicit Deserializer(Serializer&& sr) : _data(std::move(sr.unsafe_vector())) {}

        template <typename T>
        T pop() {
            throw_if_ofb();

            T res = *reinterpret_cast<T*>(_data.data() + _p);
            _p += sizeof(T);
            return res;
        }

        template <typename T>
        void pop(T& res) {
            throw_if_ofb();

            res = *reinterpret_cast<T*>(_data.data() + _p);
            _p += sizeof(T);
        }

        template <typename T>
        void pop(T* res, size_t size) {
            throw_if_ofb();

            for (int i = 0; i < size; ++i)
                pop(res[i]);
        }

        void skip(size_t count) {
            throw_if_ofb();

            _p += count;
        }

        size_t available() const {
            return _data.size() - _p;
        }

        bool is_end() const {
            return available() == 0;
        }

    private:
        void throw_if_ofb() {
            if (_p >= _data.size())
                throw std::runtime_error("Deserializer: out of bound: " + std::to_string(_p) +
                                         " vs " + std::to_string(_data.size()));
        }

    private:
        size_t _p = 0;
        std::vector<uint8_t> _data;
    };
}
