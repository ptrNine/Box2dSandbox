#include "RepeaterJointProcessor.hpp"

#include <Box2D/Dynamics/Joints/b2RevoluteJoint.h>

#include "../core/math.hpp"

#include <iostream>
RepeaterJointProcessor::RepeaterJointProcessor(
        class b2RevoluteJoint* j,
        float lower_limit,
        float upper_limit
):
        _joint(j),
        _lower_limit(lower_limit),
        _upper_limit(upper_limit)
{
    //_lower_limit = lower_limit == 228.f ? _joint->GetLowerLimit() :
    //        std::clamp(math::angle::constraint(lower_limit), _joint->GetLowerLimit(), _joint->GetUpperLimit());
    //_upper_limit = upper_limit == 228.f ? _joint->GetUpperLimit() :
    //        std::clamp(math::angle::constraint(upper_limit), _lower_limit, _joint->GetUpperLimit());
}

RepeaterJointProcessor::~RepeaterJointProcessor() {
    _joint->EnableMotor(false);
}

void RepeaterJointProcessor::update(const PhysicBodyBase& body, double delta_time) {
    float accel = _sign ? -_acceleration : _acceleration;

    float angle = _joint->GetJointAngle();
    float speed = _joint->GetJointSpeed();

    if (angle > _upper_limit && speed > 0.f) {
        if (!_sign)
            _sign = true;
        accel *= _overmove_acceleration_factor;
    }
    else if (angle < _lower_limit && speed < 0.f) {
        if (_sign)
            _sign = false;
        accel *= _overmove_acceleration_factor;
    }

    speed = std::clamp(speed + accel * float(delta_time), -_max_speed, _max_speed);

    _joint->EnableMotor(true);
    _joint->SetMotorSpeed(speed);
    _joint->SetMaxMotorTorque(_max_torque);

    /*
    _acc_time += delta_time;

    if (_acc_time > _time_period)
        _acc_time = _time_period;

    float k = _motion_function(float(_acc_time / _time_period));
    float torque = _max_torque * k;
    float speed  = _max_speed  * k;

    if (_sign)
        speed = -speed;

    _joint->EnableMotor(true);
    _joint->SetMotorSpeed(speed);
    _joint->SetMaxMotorTorque(torque);

    if (_acc_time == _time_period) {
        _sign = !_sign;
        _acc_time = 0.f;
    }
    */

    /*
    bool change_dir = false;
    if (_acc_time >= _time_period) {
        change_dir = true;
    } else {
        auto angle = _joint->GetJointAngle();

        if (_is_change_dir_catch) {
            if (_change_dir_catch_type == catch_type::up) {
                if (_upper_limit - angle > _change_dir_release_epsilon)
                    _is_change_dir_catch = false;
            }
            else if (angle - _lower_limit > _change_dir_release_epsilon)
                _is_change_dir_catch = false;
        }
        else {
            if (math::float_eq(angle, _lower_limit, _change_dir_catch_epsilon)) {
                change_dir = true;
                _is_change_dir_catch = true;
                _change_dir_catch_type = catch_type::low;
            }
            else if (math::float_eq(angle, _upper_limit, _change_dir_catch_epsilon)) {
                change_dir = true;
                _is_change_dir_catch = true;
                _change_dir_catch_type = catch_type::up;
            }
        }
    }

    if (change_dir) {
        _acc_time = 0.0;
        _sign = !_sign;
    }
     */
}