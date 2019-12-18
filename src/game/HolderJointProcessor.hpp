#pragma once

#include "JointProcessor.hpp"
#include "../core/helper_macros.hpp"

class HolderJointProcessor : public JointProcessor {
public:
    struct Pressets {
        static void human_hand_weak_relaxed(HolderJointProcessor& hjp);
        static void human_hand_normal      (HolderJointProcessor& hjp);
        static void human_hand_fast_tense  (HolderJointProcessor& hjp);
        static void human_leg_fast_tense   (HolderJointProcessor& hjp);
        static void human_shin_superweak   (HolderJointProcessor& hjp);
    };

public:
    DECLARE_SELF_FABRICS(HolderJointProcessor);

    explicit HolderJointProcessor(class b2Joint* j, float hold_angle = 0.0f);
    explicit HolderJointProcessor(class b2RevoluteJoint* j, float hold_angle = 0.0f):
            _joint(j),
            _hold_angle(hold_angle)
    {}

    ~HolderJointProcessor() final;
    void update(const class PhysicBodyBase&, double time_step) final;

private:
    enum class overflight : uint8_t { off, upper, lower };

    std::function<float(float)> _motion_function = MotionFunction::linear;

    b2RevoluteJoint* _joint;

    float _hold_angle = 0.f;
    float _last_hold_angle = 0.f;
    float _valid_hold_angle_epsilon = 0.01f;

    float _min_speed    = 1.f;
    float _max_speed    = 15.f;
    float _max_torque   = 8.f;
    float _acceleration = 80.f;

    float _deadzone_epsilon = 0.15f;
    float _deadzone_acceleration_factor = 20.f;
public:
    /// Sets motion function
    DECLARE_SET(motion_function);

    /// Sets hold angle if it in the lower/upper range
    void set_hold_angle_if_valid(float angle);

    /// Sets hold angle, clamped to lower/upper range
    void hold_angle(float angle);

    /// Returns hold angle
    DECLARE_GET(hold_angle);

    /// Returns max angle speed
    DECLARE_GET(max_speed);
    /// Sets max angle speed
    DECLARE_SET(max_speed);

    /// Returns max torque
    DECLARE_GET(max_torque);
    /// Sets max torque
    DECLARE_SET(max_torque);

    /// Returns angle speed acceleration
    DECLARE_GET(acceleration);
    /// Sets angle speed acceleration
    DECLARE_SET(acceleration);

    /// Returns angle epsilon of "deadzone"
    DECLARE_GET(deadzone_epsilon);
    /// Sets angle epsilon of "deadzone"
    DECLARE_SET(deadzone_epsilon);

    /// Returns angle speed acceleration in "deadzone"
    DECLARE_GET(deadzone_acceleration_factor);
    /// Sets angle speed acceleration in "deadzone"
    DECLARE_SET(deadzone_acceleration_factor);
};
