#pragma once

#include <functional>
#include <vector>
#include <memory>

namespace nnw {
    using FloatT  = float;
    using StringT = std::string;

    using NeuronStorage   = std::vector<class NeuronModel>;
    using SynapseStorage  = std::vector<class SynapseModel>;
    using LayerStorage    = std::vector<std::vector<size_t>>;

    using SharedNS        = std::shared_ptr<NeuronStorage>;
    using SharedSS        = std::shared_ptr<SynapseStorage>;
    using WeakNS          = std::weak_ptr<NeuronStorage>;
    using WeakSS          = std::weak_ptr<SynapseStorage>;


    enum class NeuronType {
        Input, Output, Hidden, Bias
    };

    enum class ActivationTypes {
        Identity,
        Logit,
        RELU,
        PRELU,
        LeakyRELU,
        ELU,
        Softmax
    };

    struct ActivationFunction {
        using CFT = FloatT(const class Neuron&);
        using FT  = std::function<CFT>;

        inline bool operator==(const ActivationFunction& f) const {
            return type == f.type;
        }

        inline bool operator!=(const ActivationFunction& f) const {
            return !(*this == f);
        }

        ActivationTypes type;
        FT normal, derivative;
    };
}