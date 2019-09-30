//#include "src/Engine.hpp"
#include "src/machine_learning/NeuralNetwork.hpp"
#include "src/machine_learning/TruevisionImage.hpp"
#include "src/machine_learning/SimpleSerializer.hpp"
#include "src/machine_learning/MnistDataset.hpp"


int main(int argc, char* argv[]) {
    auto dataset = nnw::MnistDataset("/home/slava/Рабочий стол/mnist/train-images.idx3-ubyte",
                                     "/home/slava/Рабочий стол/mnist/train-labels.idx1-ubyte");

    auto testset = nnw::MnistDataset("/home/slava/Рабочий стол/mnist/t10k-images.idx3-ubyte",
                                     "/home/slava/Рабочий стол/mnist/t10k-labels.idx1-ubyte");

    auto nw = nnw::NeuralNetwork("Mnist");

    auto group0 = nw.new_neuron_group(784);
    auto group1 = nw.new_neuron_group(400, nnw::activation_funcs::Sigmoid());
    auto group2 = nw.new_neuron_group(400, nnw::activation_funcs::Sigmoid());
    auto group3 = nw.new_neuron_group(10, nnw::activation_funcs::Sigmoid());

    auto s1 = nw.allover_connect(group0, group1);
    auto s2 = nw.allover_connect(group1, group2);
    auto s3 = nw.allover_connect(group2, group3);

    for (auto& s : s1)
        s->weight = nnw::helper::uniform_dist(-1, 1);

    for (auto& s : s2)
        s->weight = nnw::helper::uniform_dist(-1, 1);

    for (auto& s : s3)
        s->weight = nnw::helper::uniform_dist(-1, 1);

    //nw.init_weights(nnw::InitializerStrategy::Xavier);

    nw.set_learning_rate(0.01);
    nw.set_momentum(0.5); // 0.5, 0.9, 0.99

    nw.mark_as_input(group0);

    auto mnist = nw.compile();

    size_t errors = 0;
    size_t total_errors = 0;

    for (size_t i = 0; i < dataset.count(); ++i) {
        auto actual = mnist.run(dataset.data()[i].data());

        auto ideal  = std::vector<float>(10, 0);
        ideal[dataset.labels()[i]] = 1.f;

        mnist.backpropagation(ideal);

        uint8_t act = 0;
        for (uint8_t j = 1; j < 10; ++j)
            if (actual[j] > actual[act])
                act = j;

        if (act != dataset.labels()[i]) {
            errors++;
            total_errors++;
        }

        if ((i + 1) % 100 == 0) {
            std::cout << "test_number: " << i+1 << ", epoch: " << mnist.epoch() << ", iteration: "
                      << mnist.iteration() << ", accuracy: "
                      << (mnist.iteration() - errors) / (mnist.iteration() * 0.01) << "%, total accuracy: "
                      << (i - total_errors) / (i * 0.01) << "%" << std::endl;
        }

        if ((i + 1) % 500 == 0) {
            errors = 0;
            mnist.new_epoch();
        }
    }

    errors = 0;
    for (size_t i = 0; i < testset.count(); ++i) {
        auto actual = mnist.run(testset.data()[i].data());
        uint8_t act = 0;

        for (uint8_t j = 1; j < 10; ++j)
            if (actual[j] > actual[act])
                act = j;

        if (act != testset.labels()[i])
            errors++;

        if (i % 100 == 0) {
            std::cout << "Test iter: " << i << std::endl;
            std::cout << "Actual: " << size_t(act) << ", Ideal: "
                      << size_t(testset.labels()[i]) << std::endl << std::endl;

            if (act != testset.labels()[i]) {
                auto img = nnw::TruevisionImage();
                img.from_color_map(testset.data()[i]);
                img.save(std::string("/home/slava/Рабочий стол/mnist/tga/") + std::to_string(i) + "_actual_" + std::to_string(act) + ".tga");
            }
        }
    }

    std::cout << "Tests finished." << std::endl;
    std::cout << "Error: " << 100.0 * (1.0 * errors / testset.count()) << "%" << std::endl;

    return 0;
}