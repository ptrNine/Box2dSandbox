#pragma once

namespace nnw {
    template <typename T>
    class FixedView {
    public:
        FixedView(T* data, size_t size): _data(data), _size(size) {}

        bool   empty() const { return _size == 0; }
        size_t size () const { return _size; }

        const T& operator[](size_t i) const { return _data[i]; }
        const T* begin() const { return _data; }
        const T* end  () const { return _data + _size; }
        const T* get  () const { return _data; }

        T& operator[](size_t i) { return _data[i]; }
        T* begin() { return _data; }
        T* end  () { return _data + _size; }
        T* get  () { return _data; }

        void unsafe_unbound() {
            _data = nullptr;
            _size = 0;
        }

    protected:
        T*     _data = nullptr;
        size_t _size = 0;
    };
}
