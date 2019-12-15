#pragma once

#include "FixedView.hpp"
#include "FixedStorage.hpp"

namespace nnw {
    /**
     * FixedVector
     * Note: T must be trivially destructible
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

        FixedVector(const FixedVector&) = delete;
        FixedVector& operator=(const FixedVector&) = delete;

        void init(FixedStorage& allocator, size_t size) {
            if (this->_data != nullptr)
                throw Exception("Attempt to reinit FixedVector");

            this->_data = allocator.alloc<T>(size);
            this->_size = size;

            _current = 0;
        }

        T& assign_back(const T& val) {
            if (_current >= this->_size)
                throw Exception("FixedVector::assign_back(): out of bounds");

            new (this->_data + _current) T(val);
            ++_current;

            return *(this->_data + _current - 1);
        }

        T& assign_back(T&& val) {
            if (_current >= this->_size)
                throw Exception("FixedVector::assign_back(): out of bounds");

            new (this->_data + _current) T(std::forward<T>(val));
            ++_current;

            return *(this->_data + _current - 1);
        }

        T& front() {
            return *(this->_data);
        }

        T& back() {
            return *(this->_data + _current - 1);
        }

        const T& front() const {
            return *(this->_data);
        }

        const T& back() const {
            return *(this->_data + _current - 1);
        }

    private:
        size_t _current = 0;
    };
}
