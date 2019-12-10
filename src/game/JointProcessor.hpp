#pragma once

#include <functional>
#include <cmath>
#include <memory>
#include <string>
#include <flat_hash_map.hpp>

#include "../core/helper_macros.hpp"
#include "../core/DerivedObjectManager.hpp"

class JointProcessor {
public:
    DECLARE_SMART_POINTERS_T(JointProcessor);

    virtual void update(const class PhysicBodyBase& body, double delta_time) = 0;
    virtual ~JointProcessor() = default;

    void delete_in_next_frame() {
        _should_be_deleted = true;
    }

    bool should_be_deleted() {
        return _should_be_deleted;
    }

protected:
    bool _should_be_deleted = false;
};

struct MotionFunction {
    static constexpr auto quadratic_downward = [](float k) {
        return 1.f - powf(2.f * (k - 0.5f), 2.f);
    };

    static constexpr auto linear = [](float k) {
        return k;
    };

    static constexpr auto linear_reverse = [](float k) {
        return 1.f - k;
    };

    static constexpr auto quadratic_downward_with_min = [](float k, float min) {
        float res = quadratic_downward(k);
        return res > min ? k : min;
    };

    static auto bind_second(const std::function<float(float, float)>& f, float what) {
        return [what, f] (float k) { return f(k, what); };
    }
};

using JointProcessorManager = DerivedNamedObjectManager<JointProcessor>;