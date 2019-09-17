#pragma once

#include <string>

#include "core/helper_macros.hpp"
#include "core/time.hpp"
#include "core/FpsCounter.hpp"

class EngineState {
    SINGLETON_IMPL(EngineState)
    
    friend class Engine;

private:
    EngineState()  = default;
    ~EngineState() = default;

private:
    float      _last_delta_time = 0.f;
    Timestamp  _last_timestamp;
    FpsCounter _fps;

    void updateDeltaTime() {
        auto timestamp = timer().timestamp();
        _last_delta_time = float((timestamp - _last_timestamp).sec());
        _last_timestamp = timestamp;
    }

public:
    float       fps            () { _fps.update(); return _fps.get(); }
    std::string fps_str        () { return std::to_string(fps()) + " fps"; }
    float       last_delta_time() { return _last_delta_time; }
};


inline auto& engine_state() {
    return EngineState::instance();
}