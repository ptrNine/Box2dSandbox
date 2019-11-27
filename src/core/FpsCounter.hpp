#pragma once

#include <string>
#include <scl/array.hpp>
#include "time.hpp"

class FpsCounter {
public:
    void update() {
        durations[current_duration++] = timer.tick().secf();
        current_duration %= durations.size();
    }

    float get() {
        return durations.size() / durations.reduce(std::plus<float>());
    }

private:
    scl::Array<float, 100> durations = {};
    int current_duration = 0;
    Timer timer;
};