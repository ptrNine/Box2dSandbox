#pragma once

#include <functional>
#include <vector>
#include <memory>

#include <scl/scl.hpp>

namespace nnw {
    using FloatT  = float;
    using StringT = std::string;


    template <typename T, typename AllocT = std::allocator<T>>
    using VectorT = scl::Vector<T, AllocT>;

    using NeuronStorage   = VectorT<class NeuronModel>;
    using SynapseStorage  = VectorT<class SynapseModel>;
    using LayerStorage    = VectorT<VectorT<size_t>>;

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
        Tanh,
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