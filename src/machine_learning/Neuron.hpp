#pragma once

#include "details/Types.hpp"
#include "details/FixedView.hpp"

namespace nnw {

    struct InputNeuron {
        struct Neuron* neuron;
        FloatT         weight;
    };

    struct OutputNeuron {
        struct Neuron* neuron;
        FloatT*        weight;
        FloatT         grad_sum;
        FloatT         last_delta_weight;
    };

    struct Neuron {
        using Type = NeuronType;

        struct {
            FloatT input = 1, output, delta;
        } state;

        struct {
            FixedVector<InputNeuron>  input;
            FixedVector<OutputNeuron> output;
        } connections;

        ActivationFunction activation_func;

        size_t id;

        inline void accept_input() {
            state.input = connections.input.empty() ? state.input : 0;

            for (auto& input_connection : connections.input)
                state.input += input_connection.neuron->state.output * input_connection.weight;
        }

        inline void activate() {
            state.output = activation_func.normal(*this);
        }

        inline void trace() {
            accept_input();
            activate();
        }

        inline FloatT derivative_output() {
            return activation_func.derivative(*this);
        }

        void backpropagate_as_output() {

        }
    };
}
