//#include "src/Engine.hpp"
#include "src/machine_learning/NeuralNetwork.hpp"
#include "src/machine_learning/TruevisionImage.hpp"
#include "src/machine_learning/SimpleSerializer.hpp"
#include "src/machine_learning/MnistDataset.hpp"
#include "src/core/time.hpp"


int main(int argc, char* argv[]) {
    auto dataset = nnw::MnistDataset("/home/slava/Рабочий стол/mnist/train-images.idx3-ubyte",
                                     "/home/slava/Рабочий стол/mnist/train-labels.idx1-ubyte");

    auto testset = nnw::MnistDataset("/home/slava/Рабочий стол/mnist/t10k-images.idx3-ubyte",
                                     "/home/slava/Рабочий стол/mnist/t10k-labels.idx1-ubyte");

    /*
    auto nw = nnw::NeuralNetwork("Mnist");

    auto group0 = nw.new_neuron_group(784);
    auto group1 = nw.new_neuron_group(800, nnw::activations::LeakyRELU());
    auto group2 = nw.new_neuron_group(10, nnw::activations::Softmax());

    auto s1 = nw.allover_connect(group0, group1);
    auto s2 = nw.allover_connect(group1, group2);

    auto biases = nw.new_neuron_group(2, nnw::NeuronModel::Type::Bias);
    nw.allover_connect(biases[0], group1);
    nw.allover_connect(biases[1], group2);

    nw.init_weights(nnw::InitializerStrategy::Xavier);

    nw.set_learning_rate(0.001); // 0.001, 0.003, 0.01, 0.03, 0.1, 0.3
    nw.set_momentum(0.99); // 0.5, 0.9, 0.99

    auto mnist = nw.compile();

    size_t errors = 0;
    size_t error_iters = 0;
    size_t total_errors = 0;

    bool sgd = true;

    auto time = timer().timestamp();

    for (size_t i = 0; ; ++i) {
        ++error_iters;

        auto random_index = nnw::helper::uniform_dist<size_t>(0, dataset.count() - 1);
        //auto random_index = sgd ? nnw::helper::uniform_dist<size_t>(0, dataset.count() - 1) : i % 60000;

        auto actual = mnist.forward_pass(dataset.data()[random_index].data());

        auto ideal  = std::vector<float>(10, 0);
        ideal[dataset.labels()[random_index]] = 1.f;

        if (sgd)
            mnist.backpropagate_sgd(ideal);
        else
            mnist.backpropagate_bgd(ideal);

        auto act = static_cast<uint8_t>(std::max_element(actual.begin(), actual.end()) - actual.begin());

        if (act != dataset.labels()[random_index]) {
            errors++;
            total_errors++;
        }

        if (mnist.get_backpropagate_count() % 1000 == 0) {
            auto accuracy = (error_iters - errors) / (error_iters * 0.01);

            std::cout << "test_number: " << i+1 << ", learning rate: " << mnist.get_learning_rate()
                      << ", batch size: " << mnist.get_batch_size() << ", accuracy: " << accuracy
                      << "%, total accuracy: " << (i - total_errors) / (i * 0.01) << "%, time elapsed: "
                      << (timer().timestamp() - time).sec() << "sec"  << std::endl;

            error_iters = 0;
            errors = 0;


            if (mnist.get_backpropagate_count() == 200000) {
                sgd = false;
                mnist.update_batch_size(50);
                mnist.set_learning_rate(0.01);
            }

            if (mnist.get_backpropagate_count() == 300000) {
                mnist.update_batch_size(250);
                mnist.set_learning_rate(0.03);
            }

            if (mnist.get_backpropagate_count() == 500000)
                break;
        }
    }
     */



    //mnist.save("/home/slava/Рабочий стол/mnist/mnist-ffnn2.nnw");


    auto mnist = nnw::FeedForwardNeuralNetwork("/home/slava/Рабочий стол/mnist/mnist-ffnn2.nnw");
    mnist.save("/home/slava/Рабочий стол/mnist/mnist-ffnn2.nnw");
    mnist.load("/home/slava/Рабочий стол/mnist/mnist-ffnn2.nnw");

    size_t errors = 0;
    std::cout << "Tests started..." << std::endl;
    auto ts = timer().timestamp();
    for (size_t i = 0; i < testset.count(); ++i) {
        auto actual = mnist.forward_pass(testset.data()[i].data());
        uint8_t act = 0;

        for (uint8_t j = 1; j < 10; ++j)
            if (actual[j] > actual[act])
                act = j;

        if (act != testset.labels()[i])
            errors++;
    }

    std::cout << "Tests finished, " << (timer().timestamp() - ts).sec() << "sec" << std::endl;
    std::cout << "Error: " << 100.0 * (1.0 * errors / testset.count()) << "%" << std::endl;


    return 0;
}