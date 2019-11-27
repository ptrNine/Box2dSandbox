#pragma once

#include <utility>
#include <vector>
#include <functional>
#include <random>
#include <scl/scl.hpp>

#include "../core/helper_macros.hpp"

template <typename T>
class Chromosome {
public:
    using Type = T;

    explicit Chromosome(T data): _data(data) {}
    explicit Chromosome(T&& data) noexcept: _data(std::move(data)) {}

    T& get() { return _data; }
    const T& get() const { return _data; }

    float fitness_factor() const { return _fitness_factor; }
    void fitness_factor(float f) { _fitness_factor = f; }


private:
    float _fitness_factor = 0;
    T _data;
};


template <typename DataType>
class Genetic {
public:
    using ChromosomeT = Chromosome<DataType>;
    using InitCallbackT = std::function<DataType()>;
    using CrossingOverCallbackT = std::function<DataType(ChromosomeT&, ChromosomeT&)>;
    using MutationCallbackT = std::function<void(ChromosomeT&, float)>;

    Genetic(size_t generation_size): _generation_size(generation_size), _rand_gen(timer().getSystemDateTime().ms) {
        _generation.reserve(generation_size);
        _last_generation.reserve(generation_size);
    }

    void init(InitCallbackT callback) {
        for (size_t i = 0; i < _generation_size; ++i) {
            auto data = callback();
            _generation.emplace_back(std::move(data));
            _last_generation.emplace_back(_generation.back());
        }
    }

    void set_crossing_over_callback(CrossingOverCallbackT&& callback) {
        _crossover_callback = callback;
    }

    void set_mutation_callback(MutationCallbackT&& callback) {
        _mutation_callback = callback;
    }

    void init_factors(
            float identity_factor = 0.4f,
            float crossing_over_factor = 0.1f,
            float random_identity_factor = 0.2f,
            float random_crossing_over_factor = 0.3f,
            float mutation_factor = 0.2f)
    {
        float sum = identity_factor + crossing_over_factor + random_crossing_over_factor + random_identity_factor;

        _identity_factor = identity_factor / sum;
        _crossing_over_factor = crossing_over_factor / sum;
        _random_identity_factor = random_identity_factor / sum;
        _random_crossing_over_factor = random_crossing_over_factor / sum;
        _mutation_factor = mutation_factor / 1.f;
    }

    size_t generation_size() const {
        return _generation_size;
    }

    auto begin()       { return _generation.begin(); }
    auto begin() const { return _generation.begin(); }

    auto end()       { return _generation.end(); }
    auto end() const { return _generation.end(); }

    ChromosomeT& at(size_t i) { return _generation.at(i); }
    const ChromosomeT& at(size_t i) const { return _generation.at(i); }

    void set_fitness(size_t index, float factor) {
        _generation.at(index).fitness_factor(factor);
    }


    struct Combination {
        size_t a;
        size_t b;

        bool operator==(const Combination& c) const { return (a == c.a && b == c.b) || (a == c.b && b == c.a); }
    };

    template <typename T>
    struct CombinationHash {
        size_t operator()(const T& c) const {
            return c.a * c.a + c.b * c.b;
        }
    };

    void perform_new_generation() {
        _last_generation = _generation;

        scl::Vector<ChromosomeT> new_generation;
        new_generation.reserve(_generation_size);

        // sort by fitness factors
        _generation.sort([](const ChromosomeT& lhs, const ChromosomeT& rhs) {
            return lhs.fitness_factor() > rhs.fitness_factor();
        });

        std::cout << "Identity:" << std::endl;
        auto identity_max = size_t(_generation_size * _identity_factor);
        for (size_t i = 0; i < identity_max; ++i) {
            std::cout << "\t" << i << std::endl;
            new_generation.emplace_back(_generation.at(i));
        }


        std::cout << "Crossover:" << std::endl;
        size_t crossover_max = size_t(_generation_size * _crossing_over_factor);


        std::unordered_set<Combination, CombinationHash<Combination>> combinations;
        for (size_t c = 0, i = 0, j = 1; c < crossover_max && i < _generation_size && j < _generation_size; ++c) {
            std::cout << "\t" << i << " with " << j << std::endl;
            new_generation.emplace_back(_crossover_callback(_generation.at(i), _generation.at(j)));

            combinations.emplace(Combination{.a = i, .b = j});

            do {
                if (i > j)
                    ++j;
                else if (i < j)
                    ++i;
                else {
                    ++i;
                    j = 0;
                }
            } while (j == i || combinations.find(Combination{.a = i, .b = j}) != combinations.end());
        }


        size_t reminder = _generation_size - new_generation.size();

        size_t rand_ident_count = static_cast<size_t>(round(
                reminder * (_random_identity_factor / (_random_identity_factor + _random_crossing_over_factor))));
        size_t rand_cross_count = reminder - rand_ident_count;

        if (rand_ident_count > reminder)
            rand_ident_count = 0;

        if (rand_cross_count > reminder)
            rand_cross_count = 0;

        std::cout << "Random identity:" << std::endl;
        for (size_t i = 0; i < rand_ident_count; ++i) {
            auto uid = std::uniform_int_distribution<size_t>(identity_max + crossover_max, _generation_size - 1);
            auto rand_i = uid(_rand_gen);

            std::cout << "\t" << rand_i << std::endl;
            new_generation.emplace_back(_generation.at(rand_i));
        }

        std::cout << "Random crossover:" << std::endl;
        for (size_t i = 0; i < rand_cross_count; ++i) {
            auto uid = std::uniform_int_distribution<size_t>(0, _generation_size - 1);
            auto rand_i = uid(_rand_gen);
            auto rand_j = uid(_rand_gen);

            while(rand_j == rand_i)
                rand_j = uid(_rand_gen);

            std::cout << "\t" << rand_i << " with " << rand_j << std::endl;
            new_generation.emplace_back(_crossover_callback(_generation.at(rand_i), _generation.at(rand_j)));
        }

        std::cout << "Mutation:" << std::endl;
        auto mutation_count = static_cast<size_t>(round(_mutation_factor * _generation_size));
        bool enable_mutation = std::uniform_real_distribution<float>(0, 1)(_rand_gen) < _mutation_probability;

        bool supermutation = false;
        if (_supermutation_enabled) {
            std::unordered_map<float, size_t> counter;
            float threshold = _supermutation_threshold * _generation_size;

            for (size_t i = 0; i < _generation_size; ++i) {
                float fitness = _generation.at(i).fitness_factor();
                auto found = counter.find(fitness);

                if (found != counter.end())
                    found->second++;
                else
                    counter[fitness] = 1;
            }

            for (auto& c : counter) {
                if (c.second > threshold) {
                    //std::cout << "Too many same chromosomes, SUPERMUTATION ENABLED" << std::endl;
                    mutation_count = static_cast<size_t>(round(_supermutation_factor * _generation_size));
                    supermutation = true;
                    break;
                }
            }
        }

        if (enable_mutation || supermutation) {
            mutation_count = supermutation ? mutation_count : std::uniform_int_distribution<size_t>(0, mutation_count)(_rand_gen);
            std::cout << "MUTATION " << mutation_count << " chromosomes" << std::endl;

            for (size_t i = 0; i < mutation_count; ++i) {
                //auto min_i = size_t(_generation_size * _identity_factor);
                auto uid = std::uniform_int_distribution<size_t>(1, _generation_size - 1);
                auto rand_i = supermutation ? i : uid(_rand_gen);
                std::cout << "\t" << rand_i << std::endl;

                float intensity = supermutation ? _mutation_intensity_factor : std::uniform_real_distribution<float>(0, _mutation_intensity_factor)(_rand_gen);
                _mutation_callback(new_generation.at(rand_i), intensity);
            }
        }

        _generation = new_generation;

        for (auto& c : _generation)
            c.fitness_factor(0.f);

        ++_generation_num;
    }

    size_t generation() const {
        return _generation_num;
    }

    void enable_supermutation(bool value = true) {
        _supermutation_enabled = value;
    }

    bool is_supermutation_enabled() {
        return _supermutation_enabled;
    }

private:
    scl::Vector<ChromosomeT> _generation;
    scl::Vector<ChromosomeT> _last_generation;

    CrossingOverCallbackT _crossover_callback;
    MutationCallbackT _mutation_callback;

    size_t _generation_num = 1;
    size_t _generation_size = 0;
    float _identity_factor = 0.4f;
    float _crossing_over_factor = 0.1f;
    float _random_identity_factor = 0.2f;
    float _random_crossing_over_factor = 0.3f;

    float _mutation_factor = 0.2f;
    float _mutation_intensity_factor = 0.01f;
    float _mutation_probability = 0.1f;

    bool _supermutation_enabled = true;
    float _supermutation_factor = 0.9f;
    float _supermutation_threshold = 0.4f;


    std::mt19937 _rand_gen;

public:
    DECLARE_GET_SET(mutation_intensity_factor);
    DECLARE_GET_SET(supermutation_factor);
    DECLARE_GET_SET(supermutation_threshold);
    DECLARE_GET_SET(mutation_probability);
};