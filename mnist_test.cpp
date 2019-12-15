#include <scm/scm_filesystem.hpp>

#include "src/machine_learning/NeuralNetwork.hpp"
#include "src/machine_learning/MnistDataset.hpp"
#include "src/utils/ReaderWriter.hpp"

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

auto readOrCreateNetwork() {
    auto files = scm::fs::list_files(scm::fs::current_path());

    if (std::find(files.begin(), files.end(), "mnist.nnw") != files.end()) {
        auto path = scm::append_path(scm::fs::current_path(), "mnist.nnw");
        return std::pair{nnw::FeedForwardNeuralNetwork(path), false};
    }
    else
        return std::pair{createNetwork(), true};
}

int main() {
    auto [trainset, testset] = nnw::MnistDataset::remote_load();

    auto uid_gen_train = std::bind(nnw::helper::uniform_dist<size_t>, 0, trainset.count() - 1);
    auto uid_gen_test  = std::bind(nnw::helper::uniform_dist<size_t>, 0, testset. count() - 1);

    size_t stage1_iters = 40000;
    size_t stage2_iters = 120000;

    size_t all  = 0;
    size_t hits = 0;

    auto [network, need_training] = readOrCreateNetwork();

    auto get_accuracy = [stage1_iters, stage2_iters, &all, &hits] (const scl::Vector<float>& out, size_t real_answer) {
        ++all;

        auto answer = out.reduce([](std::pair<size_t, float> r, float v, size_t idx) {
            return v > r.second ? std::pair{idx, v} : r;
        }).first;

        if (answer == real_answer)
            ++hits;

        return (hits * 100.f) / all;
    };

    auto generate_ideal = [&, n = std::ref(network)](uint8_t real_answer) {
        auto res = scl::Vector<float>(n.get().output_layer_size(), 0.f);
        res[real_answer] = 1.f;
        return std::move(res);
    };

    if (need_training) {
        for (size_t i = 0; i < stage1_iters; ++i) {
            auto rand_idx = uid_gen_train();
            auto output = network.forward_pass(trainset.data()[rand_idx].data());

            auto real_answer = trainset.labels()[rand_idx];
            fmt::print("\rStage 1, stochastic gradient descend, Iteration: {}/{} Accuracy: {:3.2f}%",
                       all, stage1_iters, get_accuracy(output, real_answer));
            std::flush(std::cout);

            network.backpropagate_sgd(generate_ideal(real_answer));
        }
        std::cout << std::endl;
        network.update_batch_size(50);

        for (size_t i = 0; i < stage2_iters; ++i) {
            auto rand_idx = uid_gen_train();
            auto output = network.forward_pass(trainset.data()[rand_idx].data());

            auto real_answer = trainset.labels()[rand_idx];
            fmt::print("\rStage 2, batch gradient descend, Iteration: {}/{} Accuracy: {:3.2f}%",
                       all - stage1_iters, stage2_iters, get_accuracy(output, real_answer));
            std::flush(std::cout);

            network.backpropagate_bgd(generate_ideal(real_answer));
        }
    }

    all = hits = 0;
    float accuracy = 0;
    for (size_t i = 0; i < testset.count(); ++i) {
        auto output = network.forward_pass(testset.data()[i].data());
        accuracy = get_accuracy(output, testset.labels()[i]);
        fmt::print("\rTest stage, Iteration: {}/{}", all, testset.count());
        std::flush(std::cout);
    }

    fmt::print("\nDone, result accuracy: {:3.2f}%\n", accuracy);

    network.save("mnist.nnw");

    return 0;
}
