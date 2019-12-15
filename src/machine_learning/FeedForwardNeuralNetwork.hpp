#pragma once

#include <thread>
#include <map>

#include "details/md5.hpp"

#include "details/Types.hpp"
#include "details/Exception.hpp"
#include "details/FixedVector.hpp"
#include "Neuron.hpp"
#include "NeuronModel.hpp"
#include "SynapseModel.hpp"
#include "../utils/ReaderWriter.hpp"

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

        FeedForwardNeuralNetwork(FeedForwardNeuralNetwork&& ffnn) noexcept:
                _neurons               (std::move(ffnn._neurons)),
                _layers                (std::move(ffnn._layers )),
                _weights               (std::move(ffnn._weights)),
                _storage               (std::move(ffnn._storage)),
                _input_layer_size      (ffnn._input_layer_size),
                _learning_rate         (ffnn._learning_rate),
                _momentum              (ffnn._momentum),
                _current_batch         (ffnn._current_batch),
                _batch_size            (ffnn._batch_size),
                _new_batch_size        (ffnn._new_batch_size),
                _backpropagate_counter (ffnn._backpropagate_counter),
                _has_softmax_output    (ffnn._has_softmax_output)
        {}


        FeedForwardNeuralNetwork(const FeedForwardNeuralNetwork& ffnn):
                _storage               (ffnn._storage),
                _input_layer_size      (ffnn._input_layer_size),
                _learning_rate         (ffnn._learning_rate),
                _momentum              (ffnn._momentum),
                _current_batch         (ffnn._current_batch),
                _batch_size            (ffnn._batch_size),
                _new_batch_size        (ffnn._new_batch_size),
                _backpropagate_counter (ffnn._backpropagate_counter),
                _has_softmax_output    (ffnn._has_softmax_output)
        {
            // Displacement for applying to all pointers
            ptrdiff_t displacement = _storage.unsafe_data() - ffnn._storage.unsafe_data();
            auto magic_float = [displacement](float* ptr) {
                return reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(ptr) + displacement);
            };
            auto magic_neuron = [displacement](Neuron* ptr) {
                return reinterpret_cast<Neuron*>(reinterpret_cast<uint8_t*>(ptr) + displacement);
            };

            _layers.init(_storage, ffnn._layers.size());
            for (auto& layer : ffnn._layers) {
                auto to_insert = FixedVector<Neuron>(_storage, layer.size());

                for (auto& neuron : layer) {
                    auto it_neuron = Neuron();

                    it_neuron.state = neuron.state;
                    it_neuron.id = neuron.id;
                    it_neuron.activation_func = neuron.activation_func;

                    it_neuron.connections.input.init(_storage, neuron.connections.input.size(), false);
                    for (auto& input : it_neuron.connections.input)
                        input.neuron = magic_neuron(input.neuron);

                    it_neuron.connections.output.init(_storage, neuron.connections.output.size(), false);
                    for (auto& output : it_neuron.connections.output) {
                        output.weight = magic_float(output.weight);
                        output.neuron = magic_neuron(output.neuron);
                    }

                    to_insert.assign_back(std::move(it_neuron));
                }

                _layers.assign_back(std::move(to_insert));
            }

            _neurons.init(_storage, ffnn._neurons.size());
            for (auto neuron : ffnn._neurons)
                _neurons.assign_back(magic_neuron(neuron));

            _weights.init(_storage, ffnn._weights.size());
            for (auto weight : ffnn._weights)
                _weights.assign_back(magic_float(weight));
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

            size_t weights_count = 0;
            for (auto& neuron : neurons) {
                storage_size +=  neuron.input_idxs().size() * sizeof(InputNeuron);
                storage_size += neuron.output_idxs().size() * sizeof(OutputNeuron);
                weights_count += neuron.input_idxs().size(); // Weights and biases
            }
            storage_size += weights_count * sizeof(FloatT*);

            _storage.init(storage_size);

            // Init
            _layers.init(_storage, layers.size(), false);

            for (size_t i = 0; i < _layers.size(); ++i) {
                _layers.at(i).init(_storage, layers[i].size(), false);

                for (size_t j = 0; j < _layers[i].size(); ++j) {
                    auto& neuron       = _layers.at(i).at(j);
                    auto& neuron_model = neurons[layers[i][j]];

                    if (!neuron_model.input_idxs().empty())
                        neuron.connections.input.init(_storage, neuron_model.input_idxs().size(), false);

                    if (!neuron_model.output_idxs().empty())
                        neuron.connections.output.init(_storage, neuron_model.output_idxs().size(), false);
                }
            }

            _neurons.init(_storage, neurons.size(), false);

            _weights.init(_storage, weights_count);

            if (_storage.allocated_size() != _storage.max_size())
                throw Exception("FeedForwardNeuralNetwork:FeedForwardNeuralNetwork(): "
                                "storage allocated size != storage max size! (!?)");

            // Init neurons pointers
            for (size_t i = 0; i < _layers.size(); ++i) {
                for (size_t j = 0; j < _layers.at(i).size(); ++j) {
                    auto& neuron           = _layers.at(i).at(j);
                    auto  neuron_model_idx = layers[i][j];

                    _neurons.at(neuron_model_idx) = &neuron;
                }
            }

            // Todo: RIP RAM
            auto input_connections = std::map<std::pair<Neuron*, Neuron*>, FloatT*>();

            // Init neurons
            for (size_t i = 0; i < _layers.size(); ++i) {
                for (size_t j = 0; j < _layers.at(i).size(); ++j) {
                    auto& neuron       = _layers.at(i).at(j);
                    auto& neuron_model = neurons[layers[i][j]];

                    neuron.id = neuron_model.id();
                    neuron.activation_func = neuron_model.activation_func();
                    neuron.state.input = 1;

                    for (size_t k = 0; k < neuron.connections.input.size(); ++k) {
                        auto& synapse_model     = synapses[neuron_model.input_idxs()[k]];
                        auto& input_connection  = neuron.connections.input.at(k);

                        input_connection.weight = synapse_model.weight;
                        input_connection.neuron = _neurons.at(synapse_model.back_idx());

                        input_connections.emplace(std::pair{input_connection.neuron, &neuron}, &input_connection.weight);
                    }

                    for (size_t k = 0; k < neuron.connections.output.size(); ++k) {
                        auto& synapse_model     = synapses[neuron_model.output_idxs()[k]];
                        auto& output_connection = neuron.connections.output.at(k);

                        output_connection.neuron = _neurons.at(synapse_model.front_idx());
                        output_connection.grad_sum = 0;
                        output_connection.last_delta_weight = 0;
                    }
                }
            }

            for (size_t i = 0; i < _neurons.size(); ++i) {
                auto& outputs = _neurons.at(i)->connections.output;

                for (auto& out_connection : outputs)
                    out_connection.weight = input_connections[{_neurons.at(i), out_connection.neuron}];
            }

            // Weights and biases
            for (auto neuron : _neurons)
                for (auto& connection : neuron->connections.output)
                    _weights.assign_back(connection.weight);

        }

        template <bool _MultiThread = true>
        auto forward_pass(const scl::Vector<FloatT>& input) -> scl::Vector<FloatT> {
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
            auto res = scl::Vector<FloatT>(_layers.back().size());

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
        void backpropagate_sgd(const scl::Vector<FloatT>& ideal) {
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
        void backpropagate_bgd(const scl::Vector<FloatT>& ideal) {
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

        void foreach_weight(std::function<void(float&)>&& callback) {
            for(auto weight : _weights)
                callback(*weight);
        }

        void foreach_neuron(std::function<void(Neuron&)>&& callback) {
            for (auto& layer : _layers)
                for (auto& neuron : layer)
                    callback(neuron);
        }

        void deserialize(Reader& ids) {
            auto header = StringT(nnw_ffnn_file_header().size(), ' ');

            ids.read(header.data(), header.size());

            if (header != nnw_ffnn_file_header())
                throw Exception("FeedForwardNeuralNetwork::deserialize(): wrong header: " +
                                header + " vs " + nnw_ffnn_file_header());

            md5::Block128 md5;
            ids.read(md5.lo);
            ids.read(md5.hi);

            auto bytes = std::vector<uint8_t>(ids.read<uint64_t>());

            ids.read(bytes.data(), bytes.size());

            if (md5 != md5::md5(bytes.data(), bytes.size()))
                throw Exception("FeedForwardNeuralNetwork::deserialize(): md5 checksum not valid");

            auto ds = Reader(bytes.data(), bytes.size());

            _storage.unsafe_free();
            _neurons.unsafe_unbound();
            _layers .unsafe_unbound();
            _weights.unsafe_unbound();

            _storage.init(ds.read<uint64_t>());

            _input_layer_size      = ds.read<uint64_t>();
            _learning_rate         = ds.read<FloatT>();
            _momentum              = ds.read<FloatT>();
            _current_batch         = ds.read<uint64_t>();
            _batch_size            = ds.read<uint64_t>();
            _new_batch_size        = ds.read<uint64_t>();
            _backpropagate_counter = ds.read<uint64_t>();
            _has_softmax_output    = ds.read<bool>();

            std::unordered_map<uint64_t, Neuron*> id_neuron_map;

            size_t layers_count = ds.read<uint64_t>();
            _layers.init(_storage, layers_count);

            size_t weights_count = 0;
            for (auto& layer : _layers) {
                size_t layer_size = ds.read<uint64_t>();
                layer.init(_storage, layer_size);

                for (auto& neuron : layer) {
                    neuron.id = ds.read<uint64_t>();
                    neuron.connections.input.init(_storage, ds.read<uint64_t>());
                    neuron.connections.output.init(_storage, ds.read<uint64_t>());

                    weights_count += neuron.connections.output.size();

                    id_neuron_map.emplace(neuron.id, &neuron);
                }
            }

            size_t neurons_count = ds.read<uint64_t>();

            if (neurons_count != id_neuron_map.size())
                throw Exception("FeedForwardNeuralNetwork::deserialize(): data was corrupted (but hash is valid!?)");

            _neurons.init(_storage, neurons_count);
            _weights.init(_storage, weights_count);


            if (_storage.allocated_size() != _storage.max_size())
                throw Exception("FeedForwardNeuralNetwork:FeedForwardNeuralNetwork(): "
                                "storage allocated size != storage max size! (!?)");

            size_t connection_counter = 0;

            for (size_t i = 0; i < neurons_count; ++i) {
                size_t id = ds.read<uint64_t>();

                Neuron& neuron = *id_neuron_map[id];

                neuron.state.input  = ds.read<FloatT>();
                neuron.state.output = ds.read<FloatT>();
                neuron.state.delta  = ds.read<FloatT>();

                auto activation_type = static_cast<ActivationTypes>(ds.read<uint64_t>());

                neuron.activation_func = activations::is_parametrized_type(activation_type) ?
                        activations::create_parametrized_from_type(activation_type, ds.read<FloatT>()) :
                        activations::create_from_type(activation_type);

                connection_counter += neuron.connections.output.size();
            }

            if (connection_counter != ds.read<uint64_t>())
                throw Exception("FeedForwardNeuralNetwork::deserialize(): data was corrupted (but hash is valid!?)");

            // Read all connections
            for (size_t i = 0; i < neurons_count; ++i) {
                auto back = id_neuron_map[ds.read<uint64_t>()];

                for (auto& connection : back->connections.output) {
                    connection.neuron = id_neuron_map[ds.read<uint64_t>()];

                    auto& input = connection.neuron->connections.input.assign_back(
                            {.neuron = back, .weight = ds.read<FloatT>()});

                    connection.weight = &input.weight;
                    connection.last_delta_weight = ds.read<FloatT>();
                    connection.grad_sum = ds.read<FloatT>();
                }
            }

            for (auto& layer : _layers)
                for (auto& neuron : layer)
                    _neurons.assign_back(&neuron);

            // Weights and biases
            for (auto neuron : _neurons)
                for (auto& connection : neuron->connections.output)
                    _weights.assign_back(connection.weight);
        }

        void serialize(Writer& out_serializer) const {
            auto w = Writer();

            w.write<uint64_t>(_storage.max_size());
            w.write<uint64_t>(_input_layer_size);
            w.write<FloatT>  (_learning_rate);
            w.write<FloatT>  (_momentum);
            w.write<uint64_t>(_current_batch);
            w.write<uint64_t>(_batch_size);
            w.write<uint64_t>(_new_batch_size);
            w.write<uint64_t>(_backpropagate_counter);
            w.write<bool>    (_has_softmax_output);

            // Layers
            w.write<uint64_t>(_layers.size());
            for (auto& layer : _layers) {
                w.write<uint64_t>(layer.size());

                for (auto& neuron : layer) {
                    w.write<uint64_t>(neuron.id);
                    w.write<uint64_t>(neuron.connections.input.size());
                    w.write<uint64_t>(neuron.connections.output.size());
                }
            }

            size_t connection_counter = 0;

            // Neurons
            w.write<uint64_t>(_neurons.size());
            for (auto neuron : _neurons) {
                w.write<uint64_t>(neuron->id);
                w.write<FloatT>  (neuron->state.input);
                w.write<FloatT>  (neuron->state.output);
                w.write<FloatT>  (neuron->state.delta);

                // Activation functions
                w.write<uint64_t>((uint64_t)neuron->activation_func.type);

                if (auto parameter = activations::get_parameter(neuron->activation_func); parameter)
                    w.write<FloatT>(*parameter);

                connection_counter += neuron->connections.output.size(); // Increase connection counter
            }

            // Connections
            w.write<uint64_t>(connection_counter);
            for (auto neuron : _neurons) {
                w.write<uint64_t>(neuron->id);

                for (auto& connection : neuron->connections.output) {
                    w.write<uint64_t>(connection.neuron->id);
                    w.write<FloatT>  (*connection.weight);
                    w.write<FloatT>  (connection.last_delta_weight);
                    w.write<FloatT>  (connection.grad_sum);
                }
            }

            out_serializer.write(nnw_ffnn_file_header().data(), nnw_ffnn_file_header().size());

            std::vector<uint8_t> data;
            w >> data;

            auto md5 = md5::md5(data.data(), data.size());
            out_serializer.write(md5.lo);
            out_serializer.write(md5.hi);

            out_serializer.write<uint64_t>(data.size());

            out_serializer.write(data.data(), data.size());
        }

        void save(const StringT& path) const {
            std::cout << "FeedForwardNeuralNetwork::save(): save to '" + path + "'" << std::endl;
            auto file = Writer(path);
            serialize(file);
        }

        void load(const StringT& path) {
            std::cout << "FeedForwardNeuralNetwork::load(): load from '" + path + "'" << std::endl;
            auto file = Reader(path);
            deserialize(file);
        }

        size_t weights_count() const {
            return _weights.size();
        }

        size_t input_layer_size() const {
            return _layers.front().size();
        }

        size_t output_layer_size() const {
            return _layers.back().size();
        }

    private:
        FixedVector<Neuron*>              _neurons;
        FixedVector<FixedVector<Neuron>>  _layers;
        FixedVector<FloatT*>              _weights;
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
