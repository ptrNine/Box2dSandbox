#include "HolderJointProcessor.hpp"

#include <Box2D/Dynamics/Joints/b2RevoluteJoint.h>

#include "../core/math.hpp"

#include <iostream>

HolderJointProcessor::HolderJointProcessor(b2Joint* j, float hold_angle) {
    if (j->GetType() != e_revoluteJoint) {
        //todo: assert
    }
    _joint = (b2RevoluteJoint*)j;
    _hold_angle = hold_angle;
}

HolderJointProcessor::~HolderJointProcessor() {
    _joint->EnableMotor(false);
}

void HolderJointProcessor::update(const PhysicBodyBase&, double time_step)  {
    float angle = std::clamp(_joint->GetJointAngle(), _joint->GetLowerLimit(), _joint->GetUpperLimit());

    bool on_deadzone = math::float_eq(angle, _hold_angle, _deadzone_epsilon);

    float speed = _joint->GetJointSpeed();

    if (on_deadzone && speed > 0.f) {
        speed -= time_step * _acceleration * _deadzone_acceleration_factor;
        if (speed < 0)
            speed = angle > _hold_angle ? - _max_speed * (angle - _hold_angle) : _max_speed * (_hold_angle - angle);
    }
    else if (on_deadzone && speed < 0.f) {
        speed += time_step * _acceleration * _deadzone_acceleration_factor;
        if (speed > 0)
            speed = angle > _hold_angle ? - _max_speed * (angle - _hold_angle) : _max_speed * (_hold_angle - angle);
    }
    else {
        if (angle > _hold_angle)
            speed -= time_step * _acceleration;
        else
            speed += time_step * _acceleration;
    }

    speed = std::clamp(speed, -_max_speed, _max_speed);

    _joint->EnableMotor(true);
    _joint->SetMotorSpeed(speed);
    _joint->SetMaxMotorTorque(_max_torque);
}

void HolderJointProcessor::set_hold_angle_if_valid(float angle) {
    if (angle > _joint->GetLowerLimit() && angle < _joint->GetUpperLimit())
        _hold_angle = angle;
}

void HolderJointProcessor::hold_angle(float angle) {
    _hold_angle = std::clamp(angle, _joint->GetLowerLimit(), _joint->GetUpperLimit());
}

void HolderJointProcessor::Pressets::human_hand_weak_relaxed(HolderJointProcessor& hjp)  {
    hjp.max_speed (13.f);
    hjp.max_torque(1.3f);
    hjp.acceleration(200.f);

    hjp.deadzone_epsilon(0.25f);
    hjp.deadzone_acceleration_factor(7.f);
}

void HolderJointProcessor::Pressets::human_hand_normal(HolderJointProcessor& hjp)  {
    hjp.max_speed (14.f);
    hjp.max_torque(2.6f);
    hjp.acceleration(200.f);

    hjp.deadzone_epsilon(0.20f);
    hjp.deadzone_acceleration_factor(8.5f);
}

void HolderJointProcessor::Pressets::human_hand_fast_tense(HolderJointProcessor& hjp)  {
    hjp.max_speed (15.f);
    hjp.max_torque(4.f);
    hjp.acceleration(200.f);

    hjp.deadzone_epsilon(0.15f);
    hjp.deadzone_acceleration_factor(8.5f);
}

void HolderJointProcessor::Pressets::human_leg_fast_tense(HolderJointProcessor& hjp)  {
    hjp.max_speed (8.f);
    hjp.max_torque(6.f);
    hjp.acceleration(150.f);

    hjp.deadzone_epsilon(0.01f);
    hjp.deadzone_acceleration_factor(1.f);
}

void HolderJointProcessor::Pressets::human_shin_superweak(HolderJointProcessor& hjp) {
    hjp.max_speed (10.f);
    hjp.max_torque(0.3f);
    hjp.acceleration(140.f);

    hjp.deadzone_epsilon(0.25f);
    hjp.deadzone_acceleration_factor(4.f);
}