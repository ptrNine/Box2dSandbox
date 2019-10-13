#pragma once

#include "details/Types.hpp"
#include "details/GlobalStateHelper.hpp"

namespace nnw {
    class SynapseModel {
        friend class NeuralNetwork;
        friend class PerceptronFFN;
    public:
        SynapseModel(size_t backward_index, size_t forward_index, FloatT iweight = 0):
                _backward_idx(backward_index), _forward_idx(forward_index), weight(iweight) {}

    public:
        auto id() const {
            return _id;
        }

        auto name() const {
            return StringT("Synapse_") + std::to_string(_id);
        }

        size_t back_idx() const {
            return _backward_idx;
        }

        size_t front_idx() const {
            return _forward_idx;
        }

        FloatT weight;

    private:
        // For test run
        bool _is_traversed = false;

        size_t  _backward_idx;
        size_t  _forward_idx;

        size_t _id = helper::next_synapse_id();
    };
}