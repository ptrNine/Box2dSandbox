#pragma once

#include <string>
#include "time.hpp"

class FpsCounter {
public:
    void update() {
        auto   cur = timer().timestamp();
        float  dur = (cur - _last_update_ts).secf();
        _last_update_ts = cur;

        durations[current_duration++] = dur;
        current_duration %= 100;

        if (init_durations_count <= 100)
            ++init_durations_count;
    }

    float get() {
        float fps = 0;
        for (int i = 0; i < init_durations_count; ++i)
            fps += durations[i];

        fps = init_durations_count / fps;

        return fps;
    }

private:
    float  durations[100]       = {0};
    int    init_durations_count = 0;
    int    current_duration     = 0;

    Timestamp _last_update_ts;
};