#pragma once

#include "JointProcessor.hpp"
#include "../core/helper_macros.hpp"

class RepeaterJointProcessor : public JointProcessor {
public:
    DECLARE_SELF_FABRICS(RepeaterJointProcessor);

    RepeaterJointProcessor(
            class b2RevoluteJoint* j,
            float lower_limit = 228.f,
            float upper_limit = 228.f);

    ~RepeaterJointProcessor() final;

    void update(const class PhysicBodyBase&, double delta_time) final;

private:
    //enum class catch_type : uint8_t { up, low };

    std::function<float(float)> _motion_function = MotionFunction::quadratic_downward;

    b2RevoluteJoint* _joint;
    //double _acc_time = 0.0;

    //float  _change_dir_catch_epsilon   = 0.05f;
    //float  _change_dir_release_epsilon = 0.1f;

    float _max_speed    = 5.f;
    float _max_torque   = 0.2f;
    float _acceleration = 80.f;
    float _overmove_acceleration_factor = 2.f;

    float _lower_limit;
    float _upper_limit;

    //bool _is_change_dir_catch = false;
    bool _sign = false;
    //catch_type _change_dir_catch_type = catch_type::low;

public:
    void change_sign() {
        _sign = !_sign;
    }

    bool sign() {
        return _sign;
    }

    /*
    /// Sets motion function
    DECLARE_SET(motion_function);

    /// Returns max time for one side moving (seconds)
    DECLARE_GET(time_period);
    /// Sets max time for one side moving (seconds)
    DECLARE_SET(time_period);

    /// Returns angle epsilon where move dir considered changed
    DECLARE_GET(change_dir_catch_epsilon);
    /// Sets angle epsilon where move dir considered changed
    DECLARE_SET(change_dir_catch_epsilon);

    /// Returns angle epsilon after which dir may be changed again
    DECLARE_GET(change_dir_release_epsilon);
    /// Sets angle epsilon after which dir may be changed again
    DECLARE_SET(change_dir_release_epsilon);
     */

    /// Returns max angle speed
    DECLARE_GET(max_speed);
    /// Sets max angle speed
    DECLARE_SET(max_speed);

    /// Returns max torque
    DECLARE_GET(max_torque);
    /// Sets max torque
    DECLARE_SET(max_torque);
};