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

        explicit FixedVector(FixedStorage& allocator, size_t size, bool enable_assignments = true):
                FixedView<T>(allocator.alloc<T>(size), size), _current(enable_assignments ? 0 : size) {}

        FixedVector(FixedVector&& vector) noexcept : FixedView<T>(vector._data, vector._size) {
            vector._size = 0;
            vector._data = nullptr;
            _current = vector._current;
        }

        FixedVector(const FixedVector&) = delete;
        FixedVector& operator=(const FixedVector&) = delete;

        void init(FixedStorage& allocator, size_t size, bool enable_assignments = true) {
            if (this->_data != nullptr)
                throw Exception("Attempt to reinit FixedVector");

            this->_data = allocator.alloc<T>(size);
            this->_size = size;

            _current = enable_assignments ? 0 : size;
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

        const T& at(size_t idx) const {
            if (idx >= this->_size)
                throw Exception("FixedVector::at(): out of bounds");

            return this->operator[](idx);
        }

        T& at(size_t idx) {
            if (idx >= this->_size)
                throw Exception("FixedVector::at(): out of bounds");

            return this->operator[](idx);
        }

    private:
        size_t _current = 0;
    };
}
