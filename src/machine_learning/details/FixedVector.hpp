#pragma once

#include "FixedView.hpp"
#include "FixedStorage.hpp"

namespace nnw {
    /**
     * FixedVector
     * Note: T must be trivial
     */
    template <typename T>
    class FixedVector : public FixedView<T> {
    public:
        FixedVector() : FixedView<T>(nullptr, 0) {}

        explicit FixedVector(FixedStorage& allocator, size_t size):
                FixedView<T>(allocator.alloc<T>(size), size) {}

        FixedVector(FixedVector&& vector) noexcept : FixedView<T>(vector._data, vector._size) {
            vector._size = 0;
            vector._data = nullptr;
        }

        FixedVector(FixedVector&) = delete;
        FixedVector& operator=(FixedVector&) = delete;

        void init(FixedStorage& allocator, size_t size) {
            if (this->_data != nullptr)
                throw Exception("Attempt to reinit FixedVector");

            this->_data = allocator.alloc<T>(size);
            this->_size = size;

            _current = 0;
        }

        T& assign_back(const T &val) {
            if (_current >= this->_size)
                throw Exception("FixedVector::push(): out of bounds");

            *(this->_data + _current) = val;
            ++_current;

            return *(this->_data + _current - 1);
        }


    private:
        size_t _current = 0;
    };
}
