#pragma once

#include <cmath>

#include "details/Types.hpp"
#include "details/GlobalStateHelper.hpp"

namespace nnw {
    enum class InitializerStrategy {
        ReluStandart, Xavier
    };

    namespace initializers {
        FloatT relu_standart(size_t inputs) {
            return helper::uniform_dist<FloatT>(0, inputs) * std::sqrt(FloatT(2.0)/inputs);
        }

        FloatT xavier_init(size_t inputs_and_outputs) {
            FloatT u = std::sqrt(FloatT(6)) / std::sqrt(FloatT(inputs_and_outputs));
            return helper::uniform_dist<FloatT>(-u, u);
        }
    }
}