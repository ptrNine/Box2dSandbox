#pragma once

#include <iostream>

#include "details/Types.hpp"
#include "ActivationFunctions.hpp"

namespace nnw {
    class NeuronModel {
        friend class NeuralNetwork;
        friend class PerceptronFFN;

    public:
        using Type = NeuronType;

        static StringT str_type(Type type) {
            switch (type) {
                case Type::Input:  return "Input";
                case Type::Output: return "Output";
                case Type::Hidden: return "Hidden";
                case Type::Bias:   return "Bias";
                default:           return "";
            }
        }

    public:
        explicit
        NeuronModel(ActivationFunction func = activations::Logit()):
                _type(Type::Hidden), _activation_func(std::move(func)) {}

        explicit
        NeuronModel(Type type, ActivationFunction func = activations::Logit()):
                _type(type), _activation_func(std::move(func)) {}

        Type type() const {
            return _type;
        }

        auto str_type() const {
            return str_type(_type);
        }

        auto id() const {
            return _id;
        }

        auto name() const {
            return StringT("Neuron_") + std::to_string(_id);
        }

        auto info() const {
            return name() + ":" + str_type(_type);
        }

        void activation_func(const ActivationFunction& func) {
            if (_type != Type::Bias && _type != Type::Input)
                _activation_func = func;
            else
                std::cerr << "Warning: Neuron::activation_func: attempt to set activation func to "
                          << str_type(_type) << " neuron [" << _id << "]" << std::endl;
        }

        auto& input_idxs() const {
            return _input_idxs;
        }

        auto& output_idxs() const {
            return _output_idxs;
        }

        auto& activation_func() const {
            return _activation_func;
        }

    private:
        void switch_type(Type type) {
            _type = type;

            switch (_type) {
                case Type::Bias:
                case Type::Input:
                    _input = 1;
                    _activation_func = activations::Identity();
                    break;
                default:
                    break;
            }
        }

    private:
        Type _type;

        std::vector<size_t> _input_idxs;  // Input synapses
        std::vector<size_t> _output_idxs; // Output synapses

        ActivationFunction _activation_func;

        FloatT _input  = 1;
        FloatT _output = 1;

        size_t _id = helper::next_neuron_id();
    };
}