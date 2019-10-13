#pragma once

#include <thread>
#include <map>

#include "details/md5.hpp"

#include "details/Types.hpp"
#include "details/Exception.hpp"
#include "details/FixedVector.hpp"
#include "Neuron.hpp"
#include "Synapse.hpp"
#include "NeuronModel.hpp"
#include "SynapseModel.hpp"
#include "SimpleSerializer.hpp"

namespace nnw {
    inline StringT nnw_ffnn_file_header() {
        return "NNW-FFNN-0.1";
    }

    template <typename T, typename F>
    inline void multithread_vector_job(FixedVector<T>& vec, F& callback, size_t njobs) {
        size_t count = vec.size();

        if (count < njobs) {
            callback(&vec, 0, count);
        } else {
            size_t count_per_job = count / njobs;
            size_t remainder     = count - count_per_job * (njobs - 1);
            size_t position      = 0;

            std::vector<std::thread> threads;
            for (size_t i = 0; i < njobs - 1; ++i) {
                threads.emplace_back(callback, &vec, position, count_per_job);
                position += count_per_job;
            }
            threads.emplace_back(callback, &vec, position, remainder);

            for (auto& th : threads)
                th.join();
        }
    }

    class FeedForwardNeuralNetwork {
    public:
        FeedForwardNeuralNetwork(const StringT& path) {
            load(path);
        }

        FeedForwardNeuralNetwork(
                const NeuronStorage&  neurons,
                const SynapseStorage& synapses,
                const LayerStorage&   layers,
                size_t input_layer_size,
                FloatT learning_rate,
                FloatT momentum,
                size_t batch_size,
                bool   softmax_output
        ):
                _input_layer_size  (input_layer_size),
                _learning_rate     (learning_rate),
                _momentum          (momentum),
                _batch_size        (batch_size),
                _new_batch_size    (batch_size),
                _has_softmax_output(softmax_output)
        {
            // Calculate storage size
            size_t storage_size = 0;
            storage_size += neurons.size() * sizeof(Neuron*);
            storage_size += layers.size()  * sizeof(FixedVector<Neuron>);

            for (auto& layer : layers) {
                storage_size += layer.size() * sizeof(Neuron);
            }

            for (auto& neuron : neurons) {
                storage_size +=  neuron.input_idxs().size() * sizeof(InputNeuron);
                storage_size += neuron.output_idxs().size() * sizeof(OutputNeuron);
            }

            _storage.init(storage_size);


            // Init
            _layers.init(_storage, layers.size());

            for (size_t i = 0; i < _layers.size(); ++i) {
                _layers[i].init(_storage, layers[i].size());

                for (size_t j = 0; j < _layers[i].size(); ++j) {
                    auto& neuron       = _layers[i][j];
                    auto& neuron_model = neurons[layers[i][j]];

                    if (!neuron_model.input_idxs().empty())
                        neuron.connections.input.init(_storage, neuron_model.input_idxs().size());

                    if (!neuron_model.output_idxs().empty())
                        neuron.connections.output.init(_storage, neuron_model.output_idxs().size());
                }
            }

            _neurons.init(_storage, neurons.size());

            if (_storage.allocated_size() != _storage.max_size())
                throw Exception("FeedForwardNeuralNetwork:FeedForwardNeuralNetwork(): "
                                "storage allocated size != storage max size! (!?)");

            // Init neurons pointers
            for (size_t i = 0; i < _layers.size(); ++i) {
                for (size_t j = 0; j < _layers[i].size(); ++j) {
                    auto& neuron           = _layers[i][j];
                    auto  neuron_model_idx = layers[i][j];

                    _neurons[neuron_model_idx] = &neuron;
                }
            }

            // Todo: RIP RAM
            auto input_connections = std::map<std::pair<Neuron*, Neuron*>, FloatT*>();

            // Init neurons
            for (size_t i = 0; i < _layers.size(); ++i) {
                for (size_t j = 0; j < _layers[i].size(); ++j) {
                    auto& neuron       = _layers[i][j];
                    auto& neuron_model = neurons[layers[i][j]];

                    neuron.id = neuron_model.id();
                    neuron.activation_func = neuron_model.activation_func();
                    neuron.state.input = 1;

                    for (size_t k = 0; k < neuron.connections.input.size(); ++k) {
                        auto& synapse_model     = synapses[neuron_model.input_idxs()[k]];
                        auto& input_connection  = neuron.connections.input[k];

                        input_connection.weight = synapse_model.weight;
                        input_connection.neuron = _neurons[synapse_model.back_idx()];

                        input_connections.emplace(std::pair{input_connection.neuron, &neuron}, &input_connection.weight);
                    }

                    for (size_t k = 0; k < neuron.connections.output.size(); ++k) {
                        auto& synapse_model     = synapses[neuron_model.output_idxs()[k]];
                        auto& output_connection = neuron.connections.output[k];

                        output_connection.neuron = _neurons[synapse_model.front_idx()];
                        output_connection.grad_sum = 0;
                        output_connection.last_delta_weight = 0;
                    }
                }
            }

            for (size_t i = 0; i < _neurons.size(); ++i) {
                auto& outputs = _neurons[i]->connections.output;

                for (auto& out_connection : outputs)
                    out_connection.weight = input_connections[{_neurons[i], out_connection.neuron}];
            }
        }

        template <bool _MultiThread = true>
        auto forward_pass(const std::vector<FloatT>& input) -> std::vector<FloatT> {
            if (input.size() != _input_layer_size)
                throw Exception("FeedForwardNeuralNetwork::forward_pass(): "
                                "input vector size != input layer neurons count");

            // Input
            // Note: _input_layer_size may be < _layers.front().size() because of bias neuron at the end of layer
            for (size_t i = 0; i < _input_layer_size; ++i)
                _layers.front()[i].state.input = input[i];


            // Forward pass, multi-thread
            if constexpr (_MultiThread) {
                size_t jobs_count = std::thread::hardware_concurrency();

                auto layer_callback = [](FixedVector<Neuron>* layer, size_t start, size_t size) {
                    for (size_t i = start; i < start + size; ++i)
                        (*layer)[i].trace();
                };

                for (auto& layer : _layers) {
                    size_t neurons_count = layer.size();

                    if (neurons_count < jobs_count) {
                        layer_callback(&layer, 0, neurons_count);
                    }
                    else {
                        size_t neurons_per_job   = neurons_count / jobs_count;
                        size_t neurons_remainder = neurons_count - neurons_per_job * (jobs_count - 1);
                        size_t neurons_pos       = 0;

                        std::vector<std::thread> threads;
                        for (size_t i = 0; i < jobs_count - 1; ++i) {
                            threads.emplace_back(layer_callback, &layer, neurons_pos, neurons_per_job);
                            neurons_pos += neurons_per_job;
                        }

                        threads.emplace_back(layer_callback, &layer, neurons_pos, neurons_remainder);

                        for (auto& th : threads)
                            th.join();
                    }
                }
            }
            // single-thread
            else {
                for (auto &layer : _layers)
                    for (auto &neuron : layer)
                        neuron.trace();
            }

            if (_has_softmax_output)
                activations::layer::softmax(_layers.back());

            // Output
            auto res = std::vector<FloatT>(_layers.back().size());

            for (size_t i = 0; i < _layers.back().size(); ++i)
                res[i] = _layers.back()[i].state.output;

            return std::move(res);
        }

        FloatT crossentropy_der(FloatT ideal, FloatT actual) {
            return actual - ideal;
        }

        FloatT mse_partial_der(FloatT ideal, FloatT actual) {
            return 2 * (actual - ideal);
        }

        template <bool _MultiThread = true>
        void backpropagate_sgd(const std::vector<FloatT>& ideal) {
            if (ideal.size() != _layers.back().size())
                throw Exception("FeedForwardNeuralNetwork::backpropagate_sgd(): "
                                "ideal vector size != output layer neurons count");

            // Output layer
            for (size_t i = 0; i < _layers.back().size(); ++i) {
                auto& neuron = _layers.back()[i];
                neuron.state.delta = crossentropy_der(ideal[i], neuron.state.output) * neuron.derivative_output();
            }

            if constexpr (_MultiThread) {
                size_t jobs_count = std::thread::hardware_concurrency();

                // Hidden layers
                for (size_t i = _layers.size() - 2; i > 0; --i) {
                    auto callback = [this](FixedVector<Neuron>* layer_neurons, size_t start, size_t size) {
                        for (size_t j = start; j < start + size; ++j) {
                            auto& neuron = (*layer_neurons)[j];

                            // Calculate delta
                            neuron.state.delta = 0;

                            for (auto &out_connection : neuron.connections.output) {
                                neuron.state.delta += *out_connection.weight * out_connection.neuron->state.delta;

                                FloatT grad = out_connection.neuron->state.delta * neuron.state.output;
                                FloatT delta_weight = _learning_rate * grad + _momentum * out_connection.last_delta_weight;
                                *out_connection.weight -= delta_weight;
                                out_connection.last_delta_weight = delta_weight;
                            }

                            neuron.state.delta *= neuron.derivative_output();
                        }
                    };

                    multithread_vector_job(_layers[i], callback, jobs_count);
                }

                // Input layer

                auto callback = [this](FixedVector<Neuron>* layer_neurons, size_t start, size_t size) {
                    for (size_t i = start; i < start + size; ++i) {// Update weights
                        auto& neuron = (*layer_neurons)[i];

                        for (auto &out_connection : neuron.connections.output) {
                            FloatT grad = out_connection.neuron->state.delta * neuron.state.output;
                            FloatT delta_weight = _learning_rate * grad + _momentum * out_connection.last_delta_weight;
                            *out_connection.weight -= delta_weight;
                            out_connection.last_delta_weight = delta_weight;
                        }
                    }
                };

                multithread_vector_job(_layers.front(), callback, jobs_count);
            }
            // Single thread
            else {
                // Hidden layers
                for (size_t i = _layers.size() - 2; i > 0; --i) {
                    for (auto& neuron : _layers[i]) {
                        // Calculate delta
                        neuron.state.delta = 0;

                        for (auto& out_connection : neuron.connections.output) {
                            neuron.state.delta += *out_connection.weight * out_connection.neuron->state.delta;

                            FloatT grad = out_connection.neuron->state.delta * neuron.state.output;
                            FloatT delta_weight = _learning_rate * grad + _momentum * out_connection.last_delta_weight;
                            *out_connection.weight -= delta_weight;
                            out_connection.last_delta_weight = delta_weight;
                        }

                        neuron.state.delta *= neuron.derivative_output();
                    }
                }

                // Input layer
                for (auto& neuron : _layers.front()) {
                    // Update weights
                    for (auto& out_connection : neuron.connections.output) {
                        FloatT grad = out_connection.neuron->state.delta * neuron.state.output;
                        FloatT delta_weight = _learning_rate * grad + _momentum * out_connection.last_delta_weight;
                        *out_connection.weight -= delta_weight;
                        out_connection.last_delta_weight = delta_weight;
                    }
                }
            }

            ++_backpropagate_counter;
        }

        template <bool _MultiThread = true>
        void backpropagate_bgd(const std::vector<FloatT>& ideal) {
            if (ideal.size() != _layers.back().size())
                throw Exception("FeedForwardNeuralNetwork::backpropagate_bgd(): "
                                "ideal vector size != output layer neurons count");

            ++_current_batch;
            ++_backpropagate_counter;

            if constexpr (_MultiThread) {
                size_t jobs_count = std::thread::hardware_concurrency();


                if (_current_batch == _batch_size) {
                    auto callback = [this](FixedVector<Neuron*>* neurons, size_t start, size_t size) {
                        for (size_t i = start; i < start + size; ++i) {
                            for (auto& connection : (*neurons)[i]->connections.output) {
                                FloatT grad = connection.grad_sum / _batch_size;
                                FloatT delta_weight = _learning_rate * grad + _momentum * connection.last_delta_weight;
                                *connection.weight -= delta_weight;
                                connection.last_delta_weight = delta_weight;
                                connection.grad_sum = 0;
                            }
                        }
                    };

                    multithread_vector_job(_neurons, callback, jobs_count);

                    _batch_size = _new_batch_size;
                    _current_batch = 0;
                }


                // Todo: parallelize that?
                // Output layer
                for (size_t i = 0; i < _layers.back().size(); ++i) {
                    auto &neuron = _layers.back()[i];
                    neuron.state.delta = crossentropy_der(ideal[i], neuron.state.output) * neuron.derivative_output();
                }

                // Hidden layers
                for (size_t i = _layers.size() - 2; i > 0; --i) {
                    auto callback = [](FixedVector<Neuron>* layer_neurons, size_t start, size_t size) {
                        for (size_t j = start; j < start + size; ++j) {
                            auto& neuron = (*layer_neurons)[j];

                            // Calculate delta
                            neuron.state.delta = 0;

                            for (auto &out_connection : neuron.connections.output) {
                                neuron.state.delta += *out_connection.weight * out_connection.neuron->state.delta;
                                FloatT grad = out_connection.neuron->state.delta * neuron.state.output;

                                out_connection.grad_sum += grad;
                            }

                            neuron.state.delta *= neuron.derivative_output();
                        }
                    };

                    multithread_vector_job(_layers[i], callback, jobs_count);
                }

                // Input layer
                {
                    auto callback = [](FixedVector<Neuron>* layer_neurons, size_t start, size_t size) {
                        for (size_t i = start; i < start + size; ++i) {
                            auto& neuron = (*layer_neurons)[i];

                            // Update weights
                            for (auto &out_connection : neuron.connections.output) {
                                FloatT grad = out_connection.neuron->state.delta * neuron.state.output;
                                out_connection.grad_sum += grad;
                            }
                        }
                    };

                    multithread_vector_job(_layers.front(), callback, jobs_count);
                }
            }
            // Single thread impl
            else {
                if (_current_batch == _batch_size) {
                    for (auto neuron : _neurons) {
                        for (auto &connection : neuron->connections.output) {
                            FloatT grad = connection.grad_sum / _batch_size;
                            FloatT delta_weight = _learning_rate * grad + _momentum * connection.last_delta_weight;
                            *connection.weight -= delta_weight;
                            connection.last_delta_weight = delta_weight;
                            connection.grad_sum = 0;
                        }
                    }

                    _batch_size = _new_batch_size;
                    _current_batch = 0;
                }


                // Output layer
                for (size_t i = 0; i < _layers.back().size(); ++i) {
                    auto &neuron = _layers.back()[i];
                    neuron.state.delta = crossentropy_der(ideal[i], neuron.state.output) * neuron.derivative_output();
                }

                // Hidden layers
                for (size_t i = _layers.size() - 2; i > 0; --i) {
                    for (auto &neuron : _layers[i]) {
                        // Calculate delta
                        neuron.state.delta = 0;

                        for (auto &out_connection : neuron.connections.output) {
                            neuron.state.delta += *out_connection.weight * out_connection.neuron->state.delta;
                            FloatT grad = out_connection.neuron->state.delta * neuron.state.output;

                            out_connection.grad_sum += grad;
                        }

                        neuron.state.delta *= neuron.derivative_output();
                    }
                }

                // Input layer
                for (auto &neuron : _layers.front()) {
                    // Update weights
                    for (auto &out_connection : neuron.connections.output) {
                        FloatT grad = out_connection.neuron->state.delta * neuron.state.output;
                        out_connection.grad_sum += grad;
                    }
                }
            }

            if ((_backpropagate_counter + 1) % 1000 == 0)
                check_gradient_vanishing_bgd();
        }

        // Return dead weights factor of layer outputs
        FloatT check_gradient_vanishing_bgd(size_t layer = 0, FloatT epsilon = 0.0, FloatT factor = 0.5) {
            if (layer >= _layers.size() - 1) {
                std::cerr << "FeedForwardNetwork::check_gradient_vanishing_bgd(): wrong layer " << layer << std::endl;
                return 0.0;
            }

            FloatT total    = 0;
            FloatT affected = 0;

            for (auto& neuron : _layers[layer]) {
                for (auto& connection : neuron.connections.output) {
                    total += 1.0;

                    if (connection.grad_sum <= epsilon && connection.grad_sum >= -epsilon)
                        affected += 1.0;
                }
            }

            auto res = affected / total;

            if (res > factor)
                std::cerr << "Detect vanishing gradients in layer " << layer
                          << ", " << res * 100 << "% affected" << std::endl;

            return res;
        }

        FloatT get_momentum() const {
            return _momentum;
        }

        FloatT get_learning_rate() const {
            return _learning_rate;
        }

        size_t get_batch_size() const {
            return _batch_size;
        }

        size_t get_current_batch() const {
            return _current_batch;
        }

        size_t get_backpropagate_count() const {
            return _backpropagate_counter;
        }

        void set_momentum(FloatT value) {
            _momentum = value;
        }

        void set_learning_rate(FloatT value) {
            _learning_rate = value;
        }

        void update_batch_size(size_t value) {
            _new_batch_size = value;

            if (_current_batch == 0)
                _batch_size = _new_batch_size;
        }

        void momentum_mult(FloatT multiplier) {
            _momentum *= multiplier;
        }

        void learning_rate_mult(FloatT multiplier) {
            _learning_rate *= multiplier;
        }

        void deserialize(Deserializer& ids) {
            auto header = StringT(nnw_ffnn_file_header().size(), ' ');

            ids.pop(header.data(), header.size());

            if (header != nnw_ffnn_file_header())
                throw Exception("FeedForwardNeuralNetwork::deserialize(): wrong header: " +
                                header + " vs " + nnw_ffnn_file_header());

            md5::Block128 md5;
            ids.pop(md5.lo);
            ids.pop(md5.hi);

            auto bytes = std::vector<uint8_t>(ids.pop<uint64_t>());

            ids.pop(bytes.data(), bytes.size());

            if (md5 != md5::md5(bytes.data(), bytes.size()))
                throw Exception("FeedForwardNeuralNetwork::deserialize(): md5 checksum not valid");

            auto ds = Deserializer(bytes);

            _storage.unsafe_free();
            _neurons.unsafe_unbound();
            _layers.unsafe_unbound();

            _storage.init(ds.pop<uint64_t>());

            _input_layer_size      = ds.pop<uint64_t>();
            _learning_rate         = ds.pop<FloatT>();
            _momentum              = ds.pop<FloatT>();
            _current_batch         = ds.pop<uint64_t>();
            _batch_size            = ds.pop<uint64_t>();
            _new_batch_size        = ds.pop<uint64_t>();
            _backpropagate_counter = ds.pop<uint64_t>();
            _has_softmax_output    = ds.pop<bool>();

            std::unordered_map<uint64_t, Neuron*> id_neuron_map;

            size_t layers_count = ds.pop<uint64_t>();
            _layers.init(_storage, layers_count);

            for (auto& layer : _layers) {
                size_t layer_size = ds.pop<uint64_t>();
                layer.init(_storage, layer_size);

                for (auto& neuron : layer) {
                    neuron.id = ds.pop<uint64_t>();
                    neuron.connections.input.init(_storage, ds.pop<uint64_t>());
                    neuron.connections.output.init(_storage, ds.pop<uint64_t>());

                    id_neuron_map.emplace(neuron.id, &neuron);
                }
            }

            size_t neurons_count = ds.pop<uint64_t>();

            if (neurons_count != id_neuron_map.size())
                throw Exception("FeedForwardNeuralNetwork::deserialize(): data was corrupted (but hash is valid!?)");

            _neurons.init(_storage, neurons_count);

            if (_storage.allocated_size() != _storage.max_size())
                throw Exception("FeedForwardNeuralNetwork:FeedForwardNeuralNetwork(): "
                                "storage allocated size != storage max size! (!?)");

            size_t connection_counter = 0;

            for (size_t i = 0; i < neurons_count; ++i) {
                size_t id = ds.pop<uint64_t>();

                Neuron& neuron = *id_neuron_map[id];

                neuron.state.input  = ds.pop<FloatT>();
                neuron.state.output = ds.pop<FloatT>();
                neuron.state.delta  = ds.pop<FloatT>();

                auto activation_type = static_cast<ActivationTypes>(ds.pop<uint64_t>());

                neuron.activation_func = activations::is_parametrized_type(activation_type) ?
                        activations::create_parametrized_from_type(activation_type, ds.pop<FloatT>()) :
                        activations::create_from_type(activation_type);

                connection_counter += neuron.connections.output.size();
            }

            if (connection_counter != ds.pop<uint64_t>())
                throw Exception("FeedForwardNeuralNetwork::deserialize(): data was corrupted (but hash is valid!?)");

            // Read all connections
            for (size_t i = 0; i < neurons_count; ++i) {
                auto back = id_neuron_map[ds.pop<uint64_t>()];

                for (auto& connection : back->connections.output) {
                    connection.neuron = id_neuron_map[ds.pop<uint64_t>()];

                    auto& input = connection.neuron->connections.input.assign_back(
                            {.neuron = back, .weight = ds.pop<FloatT>()});

                    connection.weight = &input.weight;
                    connection.last_delta_weight = ds.pop<FloatT>();
                    connection.grad_sum = ds.pop<FloatT>();

                }
            }

            for (auto& layer : _layers)
                for (auto& neuron : layer)
                    _neurons.assign_back(&neuron);

        }

        void serialize(Serializer& out_serializer) const {
            auto serializer = Serializer();

            serializer.push<uint64_t>(_storage.max_size());
            serializer.push<uint64_t>(_input_layer_size);
            serializer.push<FloatT>  (_learning_rate);
            serializer.push<FloatT>  (_momentum);
            serializer.push<uint64_t>(_current_batch);
            serializer.push<uint64_t>(_batch_size);
            serializer.push<uint64_t>(_new_batch_size);
            serializer.push<uint64_t>(_backpropagate_counter);
            serializer.push<bool>    (_has_softmax_output);

            // Layers
            serializer.push<uint64_t>(_layers.size());
            for (auto& layer : _layers) {
                serializer.push<uint64_t>(layer.size());

                for (auto& neuron : layer) {
                    serializer.push<uint64_t>(neuron.id);
                    serializer.push<uint64_t>(neuron.connections.input.size());
                    serializer.push<uint64_t>(neuron.connections.output.size());
                }
            }

            size_t connection_counter = 0;

            // Neurons
            serializer.push<uint64_t>(_neurons.size());
            for (auto neuron : _neurons) {
                serializer.push<uint64_t>(neuron->id);
                serializer.push<FloatT>  (neuron->state.input);
                serializer.push<FloatT>  (neuron->state.output);
                serializer.push<FloatT>  (neuron->state.delta);

                // Activation functions
                serializer.push<uint64_t>((uint64_t)neuron->activation_func.type);

                if (auto parameter = activations::get_parameter(neuron->activation_func); parameter)
                    serializer.push<FloatT>(*parameter);

                connection_counter += neuron->connections.output.size(); // Increase connection counter
            }

            // Connections
            serializer.push<uint64_t>(connection_counter);
            for (auto neuron : _neurons) {
                serializer.push<uint64_t>(neuron->id);

                for (auto& connection : neuron->connections.output) {
                    serializer.push<uint64_t>(connection.neuron->id);
                    serializer.push<FloatT>  (*connection.weight);
                    serializer.push<FloatT>  (connection.last_delta_weight);
                    serializer.push<FloatT>  (connection.grad_sum);
                }
            }

            out_serializer.push(nnw_ffnn_file_header().data(), nnw_ffnn_file_header().size());

            auto md5 = md5::md5(serializer.unsafe_data(), serializer.size());
            out_serializer.push(md5.lo);
            out_serializer.push(md5.hi);

            out_serializer.push<uint64_t>(serializer.size());

            out_serializer.push(serializer.unsafe_data(), serializer.size());
        }

        void save(const StringT& path) const {
            std::cout << "FeedForwardNeuralNetwork::save(): save to '" + path + "'" << std::endl;
            auto file = FileSerializer(path);
            auto sr   = Serializer();

            serialize(sr);

            file.push(sr.unsafe_data(), sr.size());
        }

        void load(const StringT& path) {
            std::cout << "FeedForwardNeuralNetwork::load(): load from '" + path + "'" << std::endl;
            auto file  = FileDeserializer(path);
            auto bytes = file.read_all_buffered();
            auto ds    = Deserializer(bytes);

            deserialize(ds);
        }

    private:
        FixedVector<Neuron*>              _neurons;
        FixedVector<FixedVector<Neuron>>  _layers;
        FixedStorage                      _storage;

        size_t _input_layer_size;

        FloatT _learning_rate;
        FloatT _momentum;

        size_t _current_batch = 0;
        size_t _batch_size;
        size_t _new_batch_size;

        size_t _backpropagate_counter = 0;

        bool   _has_softmax_output;
    };
}