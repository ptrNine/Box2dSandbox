#pragma once

#include "details/Types.hpp"
#include "details/GlobalStateHelper.hpp"
#include "Neuron.hpp"

namespace nnw {
    namespace activations {
        inline FloatT identity(const Neuron& it) {
            return it.state.input;
        }

        inline FloatT logit(const Neuron& it) {
            return FloatT(1) / (FloatT(1) + std::exp(-it.state.input));
        }

        inline FloatT tanh(const Neuron& it) {
            return (std::exp(it.state.input) - std::exp(-it.state.input)) /
                   (std::exp(it.state.input) + std::exp(-it.state.input));
        }

        inline FloatT relu(const Neuron& it) {
            return it.state.input < 0 ? 0 : it.state.input;
        }

        inline FloatT prelu(const Neuron& it, FloatT alpha) {
            return it.state.input < 0 ? alpha * it.state.input : it.state.input;
        }

        inline FloatT elu(const Neuron& it, FloatT alpha) {
            return it.state.input < 0 ? alpha * (std::exp(it.state.input) - 1) : it.state.input;
        }

        // Softmax calc on whole layer, that return identity
        inline FloatT softmax(const Neuron& it) {
            return identity(it);
        }

        namespace derivative {
            inline FloatT identity(const Neuron&) {
                return 1;
            }

            inline FloatT logit(const Neuron& it) {
                return it.state.output * (1 - it.state.output);
            }

            inline FloatT tanh(const Neuron& it) {
                return 1 - it.state.output * it.state.output;
            }

            inline FloatT relu(const Neuron& it) {
                return it.state.output < 0 ? 0 : 1;
            }

            inline FloatT prelu(const Neuron& it, FloatT alpha) {
                return it.state.output < 0 ? alpha : 1;
            }

            inline FloatT elu(const Neuron& it, FloatT alpha) {
                return it.state.output < 0 ? it.state.output + alpha : 1;
            }

            // Softmax derivative is equal to logistic
            inline FloatT softmax(const Neuron& it) {
                return logit(it);
            }
        }

        auto Identity() {
            return ActivationFunction{
                    .type       = ActivationTypes::Identity,
                    .normal     = identity,
                    .derivative = derivative::identity
            };
        }

        auto Logit() {
            return ActivationFunction{
                    .type       = ActivationTypes::Logit,
                    .normal     = logit,
                    .derivative = derivative::logit
            };
        }

        auto Tanh() {
            return ActivationFunction{
                    .type       = ActivationTypes::Tanh,
                    .normal     = tanh,
                    .derivative = derivative::tanh
            };
        }

        auto RELU() {
            return ActivationFunction{
                    .type       = ActivationTypes::RELU,
                    .normal     = relu,
                    .derivative = derivative::relu,
            };
        }

        auto PRELU(FloatT alpha) {
            return ActivationFunction{
                    .type       = ActivationTypes ::PRELU,
                    .normal     = [alpha](const Neuron& it) { return prelu(it, alpha); },
                    .derivative = [alpha](const Neuron& it) { return derivative::prelu(it, alpha); }
            };
        }

        auto LeakyRELU() {
            return ActivationFunction{
                    .type       = ActivationTypes ::LeakyRELU,
                    .normal     = [](const Neuron& it) { return prelu(it, 0.01); },
                    .derivative = [](const Neuron& it) { return derivative::prelu(it, 0.01); }
            };
        }

        auto ELU(FloatT alpha) {
            return ActivationFunction{
                    .type       = ActivationTypes::ELU,
                    .normal     = [alpha](const Neuron& it) { return elu(it, alpha); },
                    .derivative = [alpha](const Neuron& it) { return derivative::elu(it, alpha); }
            };
        }

        auto Softmax() {
            return ActivationFunction{
                    .type       = ActivationTypes::Softmax,
                    .normal     = softmax,
                    .derivative = derivative::softmax
            };
        }


        namespace layer {
            inline void softmax(FixedVector<Neuron>& layer) {
                FloatT sigma = 0;

                std::vector<FloatT> exps(layer.size());

                for (size_t i = 0; i < layer.size(); ++i) {
                    FloatT exp = std::exp(layer[i].state.input);
                    exps[i] = exp;
                    sigma += exp;
                }

                for (size_t i = 0; i < layer.size(); ++i)
                    layer[i].state.output = exps[i] / sigma;
            }
        }

        bool is_parametrized_type(ActivationTypes type) {
            return type == ActivationTypes::PRELU || type == ActivationTypes::ELU;
        }

        /**
         * @param func - function to check
         * @return true if function is parametrized
         */
        bool is_parametrized(const ActivationFunction& func) {
            return is_parametrized_type(func.type);
        }

        /**
         * @param func - function
         * @return FloatT parameter if function is parametrized (PRELU or ELU) or std::nullopt if not
         */
        std::optional<FloatT> get_parameter(const ActivationFunction& func) {
            if (is_parametrized(func)) {
                Neuron dummy;
                dummy.state.output = -1;

                if (func.type == ActivationTypes::PRELU)
                    return func.derivative(dummy);
                else if (func.type == ActivationTypes::ELU)
                    return func.derivative(dummy) - dummy.state.output;
                else
                    throw Exception("nnw::activations::get_parameter(): Unhandled function type");
            } else {
                return {};
            }
        }

        auto create_from_type(ActivationTypes type) {
            switch (type) {
                case ActivationTypes::Identity:
                    return Identity();
                case ActivationTypes::Logit:
                    return Logit();
                case ActivationTypes::RELU:
                    return RELU();
                case ActivationTypes::LeakyRELU:
                    return LeakyRELU();
                case ActivationTypes::Softmax:
                    return Softmax();
                default:
                    throw Exception("nnw::activations::create_from_type(): Parametrized or unknown type");
            }
        }

        auto create_parametrized_from_type(ActivationTypes type, FloatT alpha) {
            switch (type) {
                case ActivationTypes::PRELU:
                    return PRELU(alpha);
                case ActivationTypes::ELU:
                    return ELU(alpha);
                default:
                    throw Exception("nnw::activations::create_from_type(): Not parametrized or unknown type");

            }
        }
    } // namespace activations
}
