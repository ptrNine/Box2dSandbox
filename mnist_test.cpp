#include <fmt/format.h>

#include "src/machine_learning/NeuralNetwork.hpp"
#include "src/machine_learning/MnistDataset.hpp"

auto createNetwork() {
    auto builder = nnw::NeuralNetwork("Mnist FFNN");
    auto input  = builder.new_neuron_group(784, nnw::activations::LeakyRELU());
    auto hidden = builder.new_neuron_group(800, nnw::activations::LeakyRELU());
    auto output = builder.new_neuron_group(10, nnw::activations::Softmax());
    auto biases = builder.new_neuron_group(2, nnw::NeuronType::Bias);

    builder.allover_connect(input, hidden);
    builder.allover_connect(hidden, output);

    builder.allover_connect(biases[0], hidden);
    builder.allover_connect(biases[1], output);

    builder.set_learning_rate(0.001);
    builder.set_momentum(0.99);

    builder.init_weights(nnw::InitializerStrategy::Xavier);

    return builder.compile();
}

int main() {
    //auto mnist = nnw::MnistDataset()

    return 0;
}