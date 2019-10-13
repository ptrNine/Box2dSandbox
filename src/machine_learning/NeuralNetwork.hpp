#pragma once

#include <unordered_set>

#include "details/Types.hpp"
#include "details/Exception.hpp"
#include "Initializers.hpp"
#include "FeedForwardNeuralNetwork.hpp"


namespace nnw {
    class NeuralNetwork {
    public:
        template <typename T, typename StorageT>
        class Provider {
            friend NeuralNetwork;
        public:
            T* operator-> () const {
                if (auto locked = _storage.lock())
                    return &(locked->at(_index));
                else
                    throw Exception("Provider::operator->: storage was corrupted");
            }

            T& operator* () const {
                if (auto locked = _storage.lock())
                    return locked->at(_index);
                else
                    throw Exception("Provider::operator->: storage was corrupted");
            }

            operator bool() {
                return _index != null_index;
            }

            ~Provider() = default;

        private:
            Provider(const std::shared_ptr<StorageT>& storage, size_t index)
                    : _storage(storage), _index(index) {}

            size_t _index;
            std::weak_ptr<StorageT> _storage;
        };

        using NeuronProvider  = Provider<NeuronModel, NeuronStorage>;
        using SynapseProvider = Provider<SynapseModel, SynapseStorage>;

    private:
        // Helpers
        template <typename T, size_t _Size, size_t ... idx>
        auto _non_default_array_init_impl(T init, std::index_sequence<idx...>&&) -> std::array<T, _Size> {
            return std::array<T, _Size>{(static_cast<void>(idx), init)...};
        }

        template <typename T, size_t _Size, typename ... ArgsT>
        auto _non_default_array_init(ArgsT ... args) -> std::array<T, _Size> {
            return _non_default_array_init_impl<T, _Size>(T(args...), std::make_index_sequence<_Size>());
        }
        inline static size_t null_index = std::numeric_limits<size_t>::min();

    public:
        NeuralNetwork(StringT name = ""): _name(std::move(name)) {
            _neurons  = std::make_shared<NeuronStorage>();
            _synapses = std::make_shared<SynapseStorage>();
        }

        /**
         * Create a new neuron
         * @param args - argumets to Neuron constructor (activation function)
         * @return created neuron
         */
        template <typename ... ArgsT>
        auto new_neuron(ArgsT&& ... args) -> NeuronProvider {
            _neurons->emplace_back(args...);
            return NeuronProvider(_neurons, _neurons->size() - 1);
        }

        /**
         * Create new neurons
         * @tparam _Count - count of neurons
         * @param args - argumets to Neuron constructor (activation function)
         * @return array with created neurons
         */
        template <size_t _Count, typename ... ArgsT>
        auto new_neuron_group(const ArgsT& ... args) -> std::array<NeuronProvider, _Count> {
            auto neurons = _non_default_array_init<NeuronProvider, _Count>(nullptr, 0);

            for (size_t i = 0; i < _Count; ++i)
                neurons[i] = new_neuron(args...);

            return neurons;
        }

        /**
         * Create new neurons
         * @param count - count of neurons
         * @param args - argumets to Neuron constructor (activation function)
         * @return vector with created neurons
         */
        template <typename ... ArgsT>
        auto new_neuron_group(size_t count, const ArgsT &... args) -> std::vector<NeuronProvider> {
            auto neurons = std::vector<NeuronProvider>();
            neurons.reserve(count);

            for (size_t i = 0; i < count; ++i)
                neurons.emplace_back(new_neuron(args...));

            return neurons;
        }

        /**
         * Test connection between two neurons (order doesn't matter)
         * @param one - first neuron
         * @param two - second neuron
         * @return true if its connected, false if no
         */
        bool test_connection(const NeuronProvider& one, const NeuronProvider& two) {
            for (auto& synapse : *_synapses)
                if ((synapse._backward_idx == one._index && synapse._forward_idx == two._index) ||
                    (synapse._backward_idx == two._index && synapse._forward_idx == one._index))
                    return true;

            return false;
        }

        /**
         * Create connection (Synapse) between two neurons
         * @param backward - first neuron
         * @param forward  - second neuron
         * @param weight - weight
         * @return created synapse
         */
        inline auto connect(const NeuronProvider &backward,
                            const NeuronProvider &forward, FloatT weight = 0) -> SynapseProvider {
            //if (forward->type() == Neuron::Type::Bias)
            //    throw Exception(StringT("NeuralNetwork::connect(): attempt to connect with bias neuron ") +
            //                    forward->name());

            _synapses->emplace_back(backward._index, forward._index, weight);

            auto synapse = SynapseProvider(_synapses, _synapses->size() - 1);

            backward->_output_idxs.emplace_back(synapse._index);
            forward ->_input_idxs .emplace_back(synapse._index);

            return synapse;
        }

        /**
         * Create connection (Synapse) between two neurons
         * @param backward - first neuron
         * @param forward  - second neuron
         * @return created synapse
         */
        inline auto allover_connect(const NeuronProvider& backward, const NeuronProvider& forward) -> SynapseProvider {
            return connect(backward, forward);
        }

        /**
         * Perform all-over connection between two groups of neurons
         * @tparam _Size1 - size of first group
         * @tparam _Size2 - size of second group
         * @param backward - first group of neurons (std::array)
         * @param forward - second group of neurons (std::array)
         * @return - group of created synapses (std::array)
         */
        template <size_t _Size1, size_t _Size2>
        auto allover_connect(
                const std::array<NeuronProvider, _Size1>& backward,
                const std::array<NeuronProvider, _Size2>& forward) -> std::array<SynapseProvider, _Size1 * _Size2>
        {
            auto synapses = _non_default_array_init<SynapseProvider, _Size1 * _Size2>(nullptr, 0);
            size_t idx = 0;

            for (auto& back : backward)
                for (auto& front : forward)
                    synapses[idx++] = connect(back, front);

            return synapses;
        }

        /**
         * Perform all-over connection between one neuron and group of neurons
         * @tparam _Size - size of group
         * @param backward - neuron
         * @param forward - group of neurons (std::array)
         * @return - group of created synapses (std::array)
         */
        template <size_t _Size>
        auto allover_connect(
                const NeuronProvider& backward,
                const std::array<NeuronProvider, _Size>& forward) -> std::array<SynapseProvider, _Size>
        {
            auto synapses = _non_default_array_init<SynapseProvider, _Size>(nullptr, 0);
            size_t idx = 0;

            for (auto& front : forward)
                synapses[idx++] = connect(backward, front);

            return synapses;
        }

        /**
         * Perform all-over connection between group of neurons and one neuron
         * @tparam _Size - size of group
         * @param backward - group of neurons (std::array)
         * @param forward - neuron
         * @return - group of created synapses (std::array)
         */
        template <size_t _Size>
        auto allover_connect(
                const std::array<NeuronProvider, _Size>& backward,
                const NeuronProvider& forward) -> std::array<SynapseProvider, _Size>
        {
            auto synapses = _non_default_array_init<SynapseProvider, _Size>(nullptr, 0);
            size_t idx = 0;

            for (auto& back : backward)
                synapses[idx++] = connect(back, forward);

            return synapses;
        }

        /**
         * Perform all-over connection between two groups of neurons
         * @param backward - first group of neurons (std::vector)
         * @param forward - second group of neurons (std::vector)
         * @return - group of created synapses (std::vector)
         */
        auto allover_connect(
                const std::vector<NeuronProvider>& backward,
                const std::vector<NeuronProvider>& forward) -> std::vector<SynapseProvider>
        {
            auto synapses = std::vector<SynapseProvider>();
            synapses.reserve(backward.size() * forward.size());

            for (auto& back : backward)
                for (auto &front : forward)
                    synapses.emplace_back(connect(back, front));

            return synapses;
        }

        /**
         * Perform all-over connection between one neuron and group of neurons
         * @param backward - neuron
         * @param forward - group of neurons (std::vector)
         * @return - group of created synapses (std::vector)
         */
        auto allover_connect(
                const NeuronProvider& backward,
                const std::vector<NeuronProvider>& forward) -> std::vector<SynapseProvider>
        {
            auto synapses = std::vector<SynapseProvider>();
            synapses.reserve(forward.size());

            for (auto &front : forward)
                synapses.emplace_back(connect(backward, front));

            return synapses;
        }

        /**
         * Perform all-over connection between group of neurons and one neuron
         * @param backward - group of neurons (std::vector)
         * @param forward - neuron
         * @return - group of created synapses (std::vector)
         */
        auto allover_connect(
                const std::vector<NeuronProvider>& backward,
                const NeuronProvider& forward) -> std::vector<SynapseProvider>
        {
            auto synapses = std::vector<SynapseProvider>();
            synapses.reserve(backward.size());

            for (auto& back : backward)
                synapses.emplace_back(connect(back, forward));

            return synapses;
        }

//        /**
//         * Mark neurons as input neurons
//         * @tparam _Size - size of input array of neurons
//         * @param neurons - array of neurons
//         */
//        template <size_t _Size>
//        void mark_as_input(const std::array<NeuronProvider, _Size>& neurons) {
//            _layers.clear();
//
//            _layers.emplace_back();
//            auto& input = _layers.front();
//
//            for (auto& neuron : neurons) {
//                if (neuron->type() != NeuronModel::Type::Bias)
//                    neuron->switch_type(NeuronModel::Type::Input);
//                input.emplace_back(neuron._index);
//            }
//        }
//
//        /**
//         * Mark neurons as input neurons
//         * @param neurons - vector of neurons
//         */
//        void mark_as_input(const std::vector<NeuronProvider>& neurons) {
//            _layers.clear();
//
//            if (neurons.empty())
//                return;
//
//            _layers.emplace_back();
//            auto& input = _layers.front();
//
//            for (auto& neuron : neurons) {
//                if (neuron->type() != NeuronModel::Type::Bias) {
//                    neuron->switch_type(NeuronModel::Type::Input);
//                    input.emplace_back(neuron._index);
//                }
//            }
//        }

        /**
         * Init synapses weights with specified strategy
         * @tparam _ZeroBiases - set weights of synapses from bias-neurons to zero (false as default)
         * @param strategy - initializer strategy
         */
        template <bool _ZeroBiases = true>
        void init_weights(InitializerStrategy strategy) {
            switch (strategy) {
                case InitializerStrategy::ReluStandart: {
                    if (_layers.empty())
                        throw Exception("NeuralNetwork::init_weights(): call ReluStandart "
                                        "initializer, but input layer not formed");

                    size_t inputs = _layers[0].size();

                    for (auto& synapse : *_synapses) {
                        if constexpr (_ZeroBiases) {
                            if (_neurons->at(synapse._backward_idx).type() == NeuronModel::Type::Bias) {
                                synapse.weight = 0;
                                break;
                            }
                        }
                        synapse.weight = initializers::relu_standart(inputs);
                    }
                }
                break;

                case InitializerStrategy::Xavier: {
                    size_t input_output_count = 0;

                    for (size_t idx = 0; idx < _neurons->size(); ++idx) {
                        auto& neuron = (*_neurons)[idx];

                        if ((neuron.type() != NeuronModel::Type::Bias && neuron._input_idxs.empty()) ||
                            neuron._output_idxs.empty())
                        {
                            ++input_output_count;
                        }
                    }

                    if (input_output_count == 0)
                        throw Exception("NeuralNetwork::init_weights(): can't found any input/output neurons");

                    for (auto& synapse : *_synapses) {
                        if constexpr (_ZeroBiases) {
                            if (_neurons->at(synapse._backward_idx).type() == NeuronModel::Type::Bias) {
                                synapse.weight = 0;
                                break;
                            }
                        }
                        synapse.weight = initializers::xavier_init(input_output_count);
                    }
                }
                break;
            }
        }

        auto compile() {
            if (get_neurons_count() == 0)
                throw Exception("NeuralNetwork::compile(): no neurons!");

            if (get_synapses_count() == 0)
                throw Exception("NeuralNetwork::compile(): no synapses!");

            _layers.clear();
            _layers.emplace_back();


            // Layers computation

            // Find input layer
            for (size_t idx = 0; idx < _neurons->size(); ++idx) {
                auto& neuron = (*_neurons)[idx];

                if (neuron.type() != NeuronModel::Type::Bias && neuron._input_idxs.empty()) {
                    neuron.switch_type(NeuronModel::Type::Input);
                    _layers.front().emplace_back(idx);
                }
            }

            if (_layers.empty() || _layers.front().empty())
                throw Exception("NeuralNetwork::compile(): no input layer provided");

            // Check is input layer input
            for (auto neuron_idx : _layers.front())
                if (!_neurons->at(neuron_idx)._input_idxs.empty())
                    throw Exception("NeuralNetwork::compile(): Input layer isn't first layer");

            // Check self connections
            for (auto& synapse : *_synapses)
                if (synapse._backward_idx == synapse._forward_idx)
                    throw Exception("NeuralNetwork::compile(): Self connection found: " + synapse.name());

            // Todo: It takes about 70% of the execution time of this method!
            // Check identical connections
            {
                auto& synapses = *_synapses;
                std::unordered_set<size_t> test;
                for (auto &neuron : *_neurons) {
                    test.clear();

                    auto &input  = neuron._input_idxs;
                    auto &output = neuron._output_idxs;

                    for (auto idx : input)
                        test.emplace(synapses[idx]._backward_idx);

                    for (auto idx : output)
                        test.emplace(synapses[idx]._forward_idx);

                    if (test.size() != input.size() + output.size())
                        throw Exception("NeuralNetwork::compile(): Identical connection found: " + neuron.name());
                }
            }


            // Turn traverse flag in forward synapses
            for (auto neuron_idx : _layers.front())
                for (auto synapse_idx : _neurons->at(neuron_idx)._output_idxs)
                    _synapses->at(synapse_idx)._is_traversed = true;


            bool is_anyone_found = true;
            std::vector<size_t> new_layer_neurons;

            while (is_anyone_found) {

                // Founding neurons with traversed input synapses
                for (size_t neuron_idx = 0; neuron_idx < _neurons->size(); ++neuron_idx) {
                    auto &neuron = _neurons->at(neuron_idx);

                    bool is_all_input_synapse_traversed = true;
                    bool has_input_synapses = !neuron._input_idxs.empty();

                    for (auto synapse_idx : neuron._input_idxs) {
                        auto &synapse = _synapses->at(synapse_idx);

                        if (!synapse._is_traversed &&
                            _neurons->at(synapse._backward_idx).type() != NeuronModel::Type::Bias) {
                            is_all_input_synapse_traversed = false;
                            break;
                        }
                    }

                    if (has_input_synapses && is_all_input_synapse_traversed) {
                        new_layer_neurons.emplace_back(neuron_idx);

                        // Remove traverse flag
                        for (auto synapse_idx : neuron._input_idxs)
                            _synapses->at(synapse_idx)._is_traversed = false;
                    }
                }

                // Turn on traverse flag to output synapses
                for (auto neuron_idx : new_layer_neurons)
                    for (auto synapse_idx : _neurons->at(neuron_idx)._output_idxs)
                        _synapses->at(synapse_idx)._is_traversed = true;

                if (new_layer_neurons.empty()) {
                    is_anyone_found = false;
                } else {
                    _layers.push_back(new_layer_neurons);
                    new_layer_neurons.clear();
                }
            }

            // Find bias-neurons
            bool is_anyone_bias_found = false;
            for (size_t i = _layers.size() - 1; i > 0; --i) {
                auto& layer = _layers[i];

                bool is_found = false;
                for (auto neuron_idx : layer) {
                    auto& neuron  = _neurons->at(neuron_idx);

                    for (auto synapse_idx : neuron._input_idxs) {
                        auto& synapse = _synapses->at(synapse_idx);
                        auto& backward_neuron = _neurons->at(synapse._backward_idx);

                        if (backward_neuron.type() == NeuronModel::Type::Bias) {
                            if (!backward_neuron._input_idxs.empty())
                                throw Exception("NeuralNetwork::compile(): "
                                                "connection to bias neuron " + backward_neuron.name());

                            if (is_found)
                                throw Exception("NeuralNetwork::compile(): "
                                                "found two ore more bias neurons in one layer");

                            is_found = true;
                            is_anyone_bias_found = true;
                            _layers[i - 1].emplace_back(synapse._backward_idx);
                        }
                    }

                    if (is_found)
                        break;
                }
            }

            // Check bias-neuron in each layer
            if (is_anyone_bias_found && _layers.size() > 1) {
                for (size_t i = 0; i < _layers.size() - 1; ++i) {
                    bool is_bias_found = false;
                    for (auto neuron_idx : _layers[i])
                        is_bias_found = is_bias_found || (_neurons->at(neuron_idx).type() == NeuronModel::Type::Bias);

                    if (!is_bias_found)
                        throw Exception(StringT("NeuralNetwork::compile(): missing bias-neuron in ") +
                                        std::to_string(i) + " layer");
                }
            }

            // Switch last layer neurons type to Output
            if (_layers.size() > 1)
                for (auto neuron_idx : _layers.back())
                    _neurons->at(neuron_idx).switch_type(NeuronModel::Type::Output);

            // Check softmax layer
            bool has_softmax_output = false;
            if (_layers.size() > 1) {
                has_softmax_output = true;
                bool softmax_meet  = false;

                for (auto& neuron : *_neurons) {
                    if (!neuron.output_idxs().empty()) {
                        if (neuron.activation_func().type == ActivationTypes::Softmax)
                            throw Exception("NeuralNetwork::compile(): softmax activation "
                                            "supported in output layer only");
                    } else {
                        if (neuron.activation_func().type == ActivationTypes::Softmax)
                            softmax_meet = true;
                        else
                            has_softmax_output = false;
                    }
                }

                if (softmax_meet && !has_softmax_output)
                    throw Exception("NeuralNetwork::compile(): softmax must be on each neuron in output layer!");
            }

            // Throw in unconnected neurons found
            {
                size_t actual = 0;
                size_t all    = _neurons->size();

                for (auto& layer : _layers)
                    actual += layer.size();

                if (actual < all)
                    throw Exception("NeuralNetwork::compile(): unconnected neurons found");

                if (actual > all)
                    throw Exception("NeuralNetwork::compile(): actual(" + std::to_string(actual) +
                                    ") > all(" + std::to_string(all) + ") (!?)");
            }

            size_t input_layer_size = _layers.front().size();
            for (auto neuron_idx : _layers.front())
                if ((*_neurons)[neuron_idx].type() == NeuronType::Bias)
                    --input_layer_size;

            auto result = FeedForwardNeuralNetwork(
                    *_neurons, *_synapses, _layers, input_layer_size, _learning_rate, _momentum, _batch_size, has_softmax_output);

            _neurons  = std::make_shared<NeuronStorage>();
            _synapses = std::make_shared<SynapseStorage>();
            _layers   = {};

            return result;
        }

        void set_learning_rate(FloatT value) {
            _learning_rate = value;
        }

        void set_momentum(FloatT value) {
            _momentum = value;
        }

        void set_batch_size(size_t value) {
            _batch_size = value;
        }

        // Statistics
        size_t get_neurons_count() {
            return _neurons->size();
        }

        size_t get_synapses_count() {
            return _synapses->size();
        }

        size_t get_layers_count() {
            return _layers.size() - 1;
        }

        FloatT get_learning_rate() {
            return _learning_rate;
        }

        FloatT get_momentum() {
            return _momentum;
        }

        size_t get_batch_size() {
            return _batch_size;
        }

    private:
        void _drop_synapse(size_t idx, const StringT& why) {
            auto& synapse = _synapses->at(idx);

            std::cerr << "NeuralNetwork: Warning! Synapse " << synapse.name()
                      << " will be dropped: " << why << std::endl;

            auto& backward_outputs =_neurons->at(synapse._backward_idx)._output_idxs;
            auto& forward_inputs   =_neurons->at(synapse._forward_idx )._input_idxs;

            auto found_bo = std::find(backward_outputs.begin(), backward_outputs.end(), idx);
            if (found_bo != backward_outputs.end())
                backward_outputs.erase(found_bo);

            auto found_fi = std::find(forward_inputs.begin(), forward_inputs.end(), idx);
            if (found_fi != forward_inputs.end())
                forward_inputs.erase(found_fi);

            synapse._backward_idx = null_index;
            synapse._forward_idx  = null_index;
        }

    private:
        StringT _name;

        SharedNS _neurons;
        SharedSS _synapses;

        FloatT _learning_rate = 0.01;
        FloatT _momentum = 0;

        size_t _batch_size = 1;

        std::vector<std::vector<size_t>> _layers;
    };

}