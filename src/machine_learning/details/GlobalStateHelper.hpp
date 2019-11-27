#pragma once

#include <random>
#include <ctime>

namespace nnw {
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

        template <typename T>
        auto uniform_dist(T min, T max) -> std::enable_if_t<std::is_integral_v<T>, T> {
            return std::uniform_int_distribution<T>(min, max)(_mt);
        }

        template <typename T>
        auto uniform_dist(T min, T max) -> std::enable_if_t<std::is_floating_point_v<T>, T> {
            return std::uniform_real_distribution<T>(min, max)(_mt);
        }

        void init_mt19937() {
            _mt = std::mt19937(static_cast<size_t>(std::time(nullptr)));
        }

    private:
        GlobalStateHelper() /* : _mt(static_cast<size_t>(std::time(nullptr))) */ {}
        ~GlobalStateHelper() = default;

        size_t _current_neuron  = 0;
        size_t _current_synapse = 0;
        std::mt19937 _mt;
    };

    namespace helper {
        inline auto next_neuron_id() {
            return GlobalStateHelper::instance().get_neuron_id();
        }

        inline auto next_synapse_id() {
            return GlobalStateHelper::instance().get_synapse_id();
        }

        template <typename T>
        T uniform_dist(T min, T max) {
            return GlobalStateHelper::instance().uniform_dist(min, max);
        }

        inline void randomize() {
            GlobalStateHelper::instance().init_mt19937();
        }
    }
}