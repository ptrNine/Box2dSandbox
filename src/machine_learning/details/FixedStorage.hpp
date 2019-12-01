#pragma once

#include "Exception.hpp"

namespace nnw {

    class FixedStorage {
    public:
        FixedStorage() = default;

        explicit FixedStorage(size_t bytes_size): _size(bytes_size) {
            _data = new uint8_t[_size];
        }

        ~FixedStorage() {
            delete [] _data;
        }

        FixedStorage(FixedStorage&& alloc) noexcept: _size(alloc._size), _data(alloc._data) {
            alloc._data = nullptr;
        }

        FixedStorage(const FixedStorage& alloc): _size(alloc._size) {
            _data = new uint8_t[_size]();
        }

        FixedStorage& operator=(const FixedStorage& alloc) {
            _size = alloc._size;
            _data = new uint8_t[_size];
            return *this;
        }

        void init(size_t bytes_size) {
            if (_data)
                throw Exception("FixedStorage::init(): Attempt to reinit allocator!");

            _size = bytes_size;
            _data = new uint8_t[_size]();
        }

        template <typename T>
        T* alloc(size_t size = 1) {
            if (_data == nullptr)
                throw Exception("FixedStorage::alloc(): allocator was not initialized");

            if (size == 0)
                return nullptr;

            T* allocated = reinterpret_cast<T*>(_data + _pos);
            _pos += size * sizeof(T);

            if (_pos > _size)
                throw Exception("FixedStorage::alloc(): _pos > size : " +std::to_string(_pos) +
                                " > " + std::to_string(_size));

            return allocated;
        }

        void unsafe_free() {
            delete [] _data;

            _data = nullptr;
            _size = 0;
            _pos  = 0;
        }

        size_t max_size() const {
            return _size;
        }

        size_t allocated_size() const {
            return _pos;
        }

    private:
        uint8_t* _data = nullptr;
        size_t   _size = 0;
        size_t   _pos  = 0;
    };
}
