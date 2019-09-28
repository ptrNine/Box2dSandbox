#pragma once

#include <vector>
#include <functional>
#include <cmath>
#include <memory>
#include <random>
#include <ctime>
#include <iostream>
#include <unordered_set>

namespace nnw {
    using FloatT  = float;
    using StringT = std::string;


    class Exception : public std::exception {
    public:
        explicit Exception(const StringT& error) : _exc(error.data()) {}
        const char* what() const noexcept override { return _exc.data(); }
    private:
        StringT _exc;
    };


    class GlobalStateHelper {
    public:
        GlobalStateHelper(const GlobalStateHelper&) = delete;
        GlobalStateHelper& operator= (const GlobalStateHelper&) = delete;

        static auto& instance() {
            static GlobalStateHelper _inst;
            return _inst;
        }

        auto get_neuron_id() {
            return ++_current_neuron;
        }

        auto get_synapse_id() {
            return ++_current_synapse;
        }

        FloatT next_rand(FloatT min, FloatT max) {
            return std::uniform_real_distribution<FloatT>(min, max)(_mt);
        }

    private:
        GlobalStateHelper() : _mt(static_cast<size_t>(std::time(nullptr))) {}
        ~GlobalStateHelper() = default;

        size_t _current_neuron  = 0;
        size_t _current_synapse = 0;
        std::mt19937 _mt;
    };

    namespace helper {
        inline auto next_neuron_name() {
            return StringT("Nrn_") + StringT(std::to_string(GlobalStateHelper::instance().get_neuron_id()));
        }

        inline auto next_synapse_name() {
            return StringT("Snp_") + StringT(std::to_string(GlobalStateHelper::instance().get_synapse_id()));
        }

        inline auto next_rand(FloatT min, FloatT max) {
            return GlobalStateHelper::instance().next_rand(min, max);
        }
    }


    ///////////////////////////////////////////////////////////////////////////
    ///                        Activation functions
    ///////////////////////////////////////////////////////////////////////////
    
    struct ActivationFunc {
        std::function<FloatT(FloatT)> normal, derivative;
    };

    enum class InitializerStrategy {
        ReluStandart, Xavier
    };

    namespace initializers {

        FloatT relu_standart(size_t inputs) {
            return helper::next_rand(0, inputs) * std::sqrt(FloatT(2.0)/inputs);
        }

        FloatT xavier_init(size_t inputs_and_outputs) {
            FloatT u = std::sqrt(FloatT(6)) / std::sqrt(FloatT(inputs_and_outputs));
            return helper::next_rand(-u, u);
        }
    }
    
    namespace activation_funcs {
        FloatT identity(FloatT x) {
            return x;
        }

        FloatT sigmoid(FloatT x) {
            return 1.0f / (1.0f + std::exp(-x));
        }

        FloatT relu(FloatT x) {
            return x >= 0.0 ? x : 0;
        }

        FloatT leaky_relu(FloatT x, FloatT alpha = 0.1) {
            return x >= 0.0 ? x : alpha * x;
        }

        namespace derivative {
            FloatT identity(FloatT) {
                return 1;
            }

            FloatT sigmoid(FloatT x) {
                return (1 - x) * x;
            }

            FloatT relu(FloatT x) {
                return x >= 0.0 ? 1 : 0;
            }

            FloatT leaky_relu(FloatT x, FloatT alpha = 0.1) {
                return x >= 0.0 ? 1 : alpha;
            }
        }

        inline auto Identity() {
            return ActivationFunc{.normal = identity, .derivative = derivative::identity};
        }

        inline auto Sigmoid() {
            return ActivationFunc{.normal = sigmoid, .derivative = derivative::sigmoid};
        }

        inline auto Relu() {
            return ActivationFunc{.normal = relu, .derivative = derivative::relu};
        }

        inline auto LeakyRelu(FloatT alpha = 0.1) {
            return ActivationFunc{.normal     = [alpha](FloatT x) { return leaky_relu(x, alpha); },
                                  .derivative = [alpha](FloatT x) { return derivative::leaky_relu(x, alpha); }
            };
        }
    } // namespace activation_funcs


    class Neuron {
        friend class NeuralNetwork;
        friend class PerceptronFFN;

    public:
        enum class Type {
            Input, Output, Hidden, Bias
        };

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
        Neuron(ActivationFunc func = activation_funcs::LeakyRelu()):
                _type(Type::Hidden), _activation_func(std::move(func)) {}

        explicit
        Neuron(Type type, ActivationFunc func = activation_funcs::LeakyRelu()):
                _type(type), _activation_func(std::move(func)) {}

        Type type() const {
            return _type;
        }

        auto str_type() const {
            return str_type(_type);
        }

        auto& name() const {
            return _name;
        }

        auto info() const {
            return _name + ":" + str_type(_type);
        }

        FloatT derivative_output() {
            return _activation_func.derivative(_output);
        }

        void activation_func(const ActivationFunc& func) {
            if (_type != Type::Bias && _type != Type::Input)
                _activation_func = func;
            else
                std::cerr << "Warning: Neuron::activation_func: attempt to set activation func to "
                          << str_type(_type) << " neuron [" << _name << "]" << std::endl;
        }

    private:
        void switch_type(Type type) {
            _type = type;

            switch (_type) {
                case Type::Bias:
                case Type::Input:
                    _input = 1;
                    _activation_func = activation_funcs::Identity();
                    break;
                default:
                    break;
            }
        }

        void reset_input() {
            _input = type() != Type::Bias ? 0 : 1;
        }

        void activate() {
            _output = _activation_func.normal(_input);
        }

    private:
        Type _type;

        std::vector<size_t> _input_idxs;  // Input synapses
        std::vector<size_t> _output_idxs; // Output synapses

        ActivationFunc _activation_func;

        FloatT _input  = 1;
        FloatT _output = 1;

        StringT _name = helper::next_neuron_name();
    };


    class Synapse {
        friend class NeuralNetwork;
        friend class PerceptronFFN;
    public:
        Synapse(size_t backward_index, size_t forward_index, FloatT iweight = 0):
                _backward_idx(backward_index), _forward_idx(forward_index), weight(iweight) {}

    public:
        auto& name() const {
            return _name;
        }

        FloatT weight;

    private:
        // For test run
        bool _is_traversed = false;

        FloatT  _last_delta_weight = 0;

        size_t  _backward_idx;
        size_t  _forward_idx;

        StringT _name = helper::next_synapse_name();
    };


    using NeuronStorage   = std::vector<class Neuron>;
    using SynapseStorage  = std::vector<class Synapse>;
    using LayerStorage    = std::vector<std::vector<size_t>>;
    using SharedNS        = std::shared_ptr<NeuronStorage>;
    using SharedSS        = std::shared_ptr<SynapseStorage>;
    using WeakNS          = std::weak_ptr<NeuronStorage>;
    using WeakSS          = std::weak_ptr<SynapseStorage>;

    class PerceptronFFN {
        friend class NeuralNetwork;
    public:
        auto run(const std::vector<FloatT>& input) -> std::vector<FloatT> {
            if (input.size() != input_count())
                throw Exception("PerceptronFFN::run(): input vector size != input neurons count");

            // Reset all inputs
            for (auto& neuron : _neurons)
                neuron.reset_input();

            // Init input
            for (size_t i = 0; i < input.size(); ++i)
                _neurons[_layers.front()[i]]._input = input[i];


            // Feed forward run
            for (auto& layer : _layers) {
                for (auto neuron_idx : layer) {
                    // Activation function
                    auto& neuron = _neurons[neuron_idx];
                    neuron.activate();

                    //std::cout << neuron.info() << " input:   " << neuron._input << std::endl;
                    //std::cout << neuron.info() << " output:  " << neuron._output << std::endl;

                    // Feed output
                    for (auto synapse_idx : neuron._output_idxs) {
                        auto& synapse     = _synapses[synapse_idx];
                        auto& next_neuron = _neurons[synapse._forward_idx];

                        next_neuron._input += neuron._output * synapse.weight;
                    }
                }
            }

            auto output = std::vector<FloatT>();
            output.reserve(_layers.back().size());

            for (auto neuron_idx : _layers.back())
                output.emplace_back(_neurons[neuron_idx]._output);

            return output;
        }

        FloatT mse_error(const std::vector<FloatT>& actual) {
            if (actual.size() != output_count())
                throw Exception("PerceptronFFN::backpropagation(): actual vector size != output neurons count");

            FloatT result = 0;

            for (size_t i = 0; i < actual.size(); ++i) {
                auto tmp = _neurons[_layers.back()[i]]._output - actual[i];
                result += tmp * tmp;
            }

            return result / actual.size();
        }

        void backpropagation(const std::vector<FloatT>& ideal) {
            if (ideal.size() != output_count())
                throw Exception("PerceptronFFN::backpropagation(): ideal vector size != output neurons count");

            for (size_t i = 0; i < ideal.size(); ++i) {
                auto& neuron = _neurons[_layers.back()[i]];

                auto delta = (ideal[i] - neuron._output) * neuron.derivative_output();
                neuron._input = delta; // input become delta
            }

            for (ptrdiff_t i = _layers.size() - 2; i >= 0; --i) {
                for (auto neuron_idx : _layers[i]) {
                    auto& neuron = _neurons[neuron_idx];

                    if (!neuron._input_idxs.empty()) {
                        FloatT sigma = 0;
                        for (auto synapse_idx : neuron._output_idxs) {
                            auto &synapse = _synapses[synapse_idx];
                            auto &next_neuron = _neurons[synapse._forward_idx];

                            sigma += synapse.weight * next_neuron._input;
                        }

                        auto delta = neuron.derivative_output() * sigma;
                        neuron._input = delta;
                    }


                    for (auto synapse_idx : neuron._output_idxs) {
                        auto& synapse     = _synapses[synapse_idx];
                        auto& next_neuron = _neurons[synapse._forward_idx];

                        auto grad = neuron._output * next_neuron._input;
                        auto delta_weight = _learning_rate * grad + _momentum * synapse._last_delta_weight;

                        synapse._last_delta_weight = delta_weight;
                        synapse.weight += delta_weight;
                    }
                }
            }

            ++_iteration;
        }

        void new_epoch() {
            _iteration = 0;
            ++_epoch;
        }

        // Statistics
        size_t input_count() {
            return _bias_enabled ? _layers.front().size() - 1 :
                                   _layers.front().size();
        }

        size_t output_count() {
            return _layers.back().size();
        }

        size_t neurons_count() {
            return _neurons.size();
        }

        size_t synapses_count() {
            return _synapses.size();
        }

        size_t layers_count() {
            return _layers.size() - 1;
        }

        size_t iteration() {
            return _iteration;
        }

        size_t epoch() {
            return _epoch;
        }


    protected:
        PerceptronFFN(NeuronStorage&& neurons, SynapseStorage&& synapses, LayerStorage&& layers,
                      FloatT learning_rate, FloatT momentum)
                : _neurons(neurons), _synapses(synapses), _layers(layers), _learning_rate(learning_rate),
                  _momentum(momentum)
        {
            for (auto i = _neurons.rbegin(); i != _neurons.rend() && !_bias_enabled; ++i)
                if (i->type() == Neuron::Type::Bias)
                    _bias_enabled = true;

            // Move bias-neuron to the end
            if (_bias_enabled && _neurons[_layers.front().back()].type() != Neuron::Type::Bias) {
                size_t i = 0;

                while (i < _layers.front().size() && _neurons[_layers.front()[i]].type() != Neuron::Type::Bias)
                    ++i;

                std::swap(_layers.front()[i], _layers.front().back());
            }
        }

    protected:
        StringT _name;

        bool           _bias_enabled = false;
        NeuronStorage  _neurons;
        SynapseStorage _synapses;
        LayerStorage   _layers;

        // Backpropagation
        FloatT _learning_rate;
        FloatT _momentum;
        // !Backpropagation

        size_t _iteration = 0;
        size_t _epoch     = 0;
    };
    
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

        using NeuronProvider  = Provider<Neuron, NeuronStorage>;
        using SynapseProvider = Provider<Synapse, SynapseStorage>;

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

        /**
         * Mark neurons as input neurons
         * @tparam _Size - size of input array of neurons
         * @param neurons - array of neurons
         */
        template <size_t _Size>
        void mark_as_input(const std::array<NeuronProvider, _Size>& neurons) {
            _layers.clear();

            _layers.emplace_back();
            auto& input = _layers.front();

            for (auto& neuron : neurons) {
                if (neuron->type() != Neuron::Type::Bias)
                    neuron->switch_type(Neuron::Type::Input);
                input.emplace_back(neuron._index);
            }
        }

        /**
         * Mark neurons as input neurons
         * @param neurons - vector of neurons
         */
        void mark_as_input(const std::vector<NeuronProvider>& neurons) {
            _layers.clear();

            if (neurons.empty())
                return;

            _layers.emplace_back();
            auto& input = _layers.front();

            for (auto& neuron : neurons) {
                if (neuron->type() != Neuron::Type::Bias) {
                    neuron->switch_type(Neuron::Type::Input);
                    input.emplace_back(neuron._index);
                }
            }
        }

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
                            if (_neurons->at(synapse._backward_idx).type() == Neuron::Type::Bias) {
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

                        if ((neuron.type() != Neuron::Type::Bias && neuron._input_idxs.empty()) ||
                            neuron._output_idxs.empty())
                        {
                            ++input_output_count;
                        }
                    }

                    if (input_output_count == 0)
                        throw Exception("NeuralNetwork::init_weights(): can't found any input/output neurons");

                    for (auto& synapse : *_synapses) {
                        if constexpr (_ZeroBiases) {
                            if (_neurons->at(synapse._backward_idx).type() == Neuron::Type::Bias) {
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
            if (synapses_count() == 0)
                throw Exception("NeuralNetwork::compile(): no synapses!");

            if (_layers.empty() || _layers.front().empty())
                throw Exception("NeuralNetwork::compile(): no input layer provided");

            _layers.resize(1);

            // Layers computation


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
                            _neurons->at(synapse._backward_idx).type() != Neuron::Type::Bias) {
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

                        if (backward_neuron.type() == Neuron::Type::Bias) {
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
                        is_bias_found = is_bias_found || (_neurons->at(neuron_idx).type() == Neuron::Type::Bias);

                    if (!is_bias_found)
                        throw Exception(StringT("NeuralNetwork::compile(): missing bias-neuron in ") +
                                        std::to_string(i) + " layer");
                }
            }

            // Switch last layer neurons type to Output
            if (_layers.size() > 1)
                for (auto neuron_idx : _layers.back())
                    _neurons->at(neuron_idx).switch_type(Neuron::Type::Output);

            // Throw in unconnected neurons found
            {
                size_t actual = 0;
                size_t all    = _neurons->size();

                for (auto& layer : _layers)
                    actual += layer.size();

                if (actual < all)
                    throw Exception("NeuralNetwork::compile(): unconnected neurons found");

                if (actual > all)
                    throw Exception("NeuralNetwork::compile(): actual(" + std::to_string(actual) + ") > all(" + std::to_string(all) + ") (!?)");
            }

            auto result = PerceptronFFN(std::move(*_neurons), std::move(*_synapses), std::move(_layers),
                                        _learning_rate, _momentum);

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

        // Statistics
        size_t neurons_count() {
            return _neurons->size();
        }

        size_t synapses_count() {
            return _synapses->size();
        }

        size_t layers_count() {
            return _layers.size() - 1;
        }

        FloatT learning_rate() {
            return _learning_rate;
        }

        FloatT momentum() {
            return _momentum;
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
        FloatT _momentum = 0.1;

        std::vector<std::vector<size_t>> _layers;
    };

}