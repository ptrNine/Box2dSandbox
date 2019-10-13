#pragma once

#include "Types.hpp"

namespace nnw {
    class Exception : public std::exception {
    public:
        explicit Exception(const StringT& error) : _exc(error.data()) {}
        const char* what() const noexcept override { return _exc.data(); }
    private:
        StringT _exc;
    };
}