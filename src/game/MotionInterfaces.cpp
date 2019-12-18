#include "MotionInterfaces.hpp"

#include <Box2D/Dynamics/Joints/b2RevoluteJoint.h>
#include <scl/scl.hpp>

#include "PhysicHumanBody.hpp"
#include "HolderJointProcessor.hpp"
#include "../core/math.hpp"

namespace motion_interface {
    PeriodicCounter::PeriodicCounter(const std::weak_ptr<PhysicBodyBase>& body, StrCRefT name): _name(name) {
        add_binding(body);
    }

    void PeriodicCounter::add_binding(const std::weak_ptr<PhysicBodyBase>& body) {
        _bodies.emplace_back(body);
        update_bindings();
    }

    void PeriodicCounter::set(double period, double start) const {
        for (auto& b : _bodies) {
            if (auto body = b.lock()) {
                auto& ud = body->user_data();
                ud.cast<double>(_n_period) = period;
                ud.cast<double>(_n_acc) = std::clamp(start, 0.0, period);
                ud.cast<double>(_n_factor) = std::clamp(start, 0.0, period) / period;
            }
        }
    }

    void PeriodicCounter::set_period(double period) const {
        for (auto& b : _bodies) {
            if (auto body = b.lock()) {
                auto& ud = body->user_data();
                auto& p = ud.cast<double>(_n_period);
                auto& a = ud.cast<double>(_n_acc);

                a = ud.cast<double>(_n_factor) * period;
                p = period;
            }
        }
    }

    void PeriodicCounter::set_start(double start) const {
        for (auto& b : _bodies) {
            if (auto body = b.lock()) {
                auto& p = body->user_data().cast<double>(_n_period);
                body->user_data().cast<double>(_n_acc) = std::clamp(start, 0.0, p);
            }
        }
    }

    void PeriodicCounter::destroy() const {
        for (auto& b : _bodies) {
            if (auto body = b.lock()) {
                auto& ud = body->user_data();
                auto used_by = ud.cast<size_t>(_n_used_by);

                if (used_by == 0) {
                    ud.erase(_n_factor);
                    ud.erase(_n_acc);
                    ud.erase(_n_period);
                    ud.erase(_n_used_by);
                    body->remove_update(_n_update);
                } else {
                    // Todo: log warning
                }
            }
        }
    }

    struct PeriodicCounterFunctor {
        void operator()(PhysicBodyBase& body, double timestep) const {
            auto& acc    = body.user_data().cast<double>(_n_acc);
            auto& period = body.user_data().cast<double>(_n_period);

            acc = fmod(acc + timestep, period);
            body.user_data().cast<double>(_n_factor) = acc / period;
        }

        StrT _n_acc, _n_period, _n_factor;
    };

    void PeriodicCounter::update_bindings() {
        std::string pref = std::string("__") + _name;
        _n_acc     = pref + "_acc";
        _n_factor  = pref + "_factor";
        _n_update  = pref + "_update";
        _n_used_by = pref + "_used_by";
        _n_period  = pref + "_period";

        double last_valid_period = 1.0;

        for (auto& b : _bodies) {
            if (auto body = b.lock()) {
                if (!body->is_update_exists(_n_update)) {
                    auto& ud = body->user_data();

                    ud[_n_acc]     = 0.0;
                    ud[_n_factor]  = 0.0;
                    ud[_n_period]  = last_valid_period;
                    ud[_n_used_by] = size_t(0);

                    body->add_update(_n_update, PeriodicCounterFunctor{_n_acc, _n_period, _n_factor});
                } else {
                    last_valid_period = body->user_data().cast<double>(_n_period);
                }
            }
        }
    }


    AnimatedJoint::AnimatedJoint(const std::weak_ptr<BodyWithJoints>& body, StrCRefT name, int joint_index, StrCRefT counter_name): _name(name) {
        add_binding(body, joint_index, counter_name);
    }

    void AnimatedJoint::add_binding(const std::weak_ptr<BodyWithJoints>& body, int joint_index, StrCRefT counter_name) {
        _bindings.push_back({body, joint_index, counter_name});
        update_bindings();
    }

    const AnimatedJoint& AnimatedJoint::set_frames(VecCRefT<Frame> _frames) const {
        auto frames = _frames;
        for (auto& f : frames)
            f.start_point = std::clamp(f.start_point, 0.0, 1.0);

        for (auto& b : _bindings) {
            if (auto body = b.body.lock()) {
                auto& ud = body->user_data();

                auto& frames_count = ud.cast<size_t>(_n_frames_count);

                // Remove unused frames
                if (frames.size() < frames_count) {
                    for (size_t i = frames.size(); i < frames_count; ++i) {
                        auto p = _n_frame_pref + std::to_string(i);
                        ud.erase(p + "_start");
                        ud.erase(p + "_angle");
                    }
                }

                frames_count = frames.size();

                // Setup frames
                for (size_t i = 0; i < frames.size(); ++i) {
                    auto p = _n_frame_pref + std::to_string(i);
                    ud[p + "_start"] = frames[i].start_point;
                    ud[p + "_angle"] = frames[i].target_angle;
                }
            }
            else {
                // Todo: log warning
            }
        }

        return *this;
    }
    const AnimatedJoint& AnimatedJoint::set_shift(double time_shift) const {
        for (auto& b : _bindings) {
            if (auto body = b.body.lock()) {
                auto& ud = body->user_data();
                body->user_data().cast<double>(_n_shift) = time_shift;
            }
            else {
                // Todo: log warning
            }
        }

        return *this;
    }

    void AnimatedJoint::destroy() const {
        for (auto& b : _bindings) {
            if (auto body = b.body.lock()) {
                auto& ud = body->user_data();

                auto frames_count = ud.cast<size_t>(_n_frames_count);
                for (size_t i = 0; i < frames_count; ++i) {
                    auto pref = _n_frame_pref + std::to_string(i);
                    ud.erase(pref + "_start");
                    ud.erase(pref + "_angle");
                }

                body->remove_update(_n_update);
                body->remove_joint_processor(_n_joint_processor);
                ud.erase(_n_frames_count);
                ud.erase(_n_last_frame);
                ud.erase(_n_shift);
            }
            else {
                // Todo: log warning
            }
        }
    }

    struct AnimatedJointFunctor {
        void operator()(PhysicBodyBase& _body, double) const {
            auto& body = static_cast<BodyWithJoints&>(_body);

            auto& ud = body.user_data();
            auto counter_name = ud.cast<StrT>(_n_counter);
            auto factor       = fmod(ud.cast<double>("__" + counter_name + "_factor") + ud.cast<double>(_n_shift), 1.0);
            auto frames_count = ud.cast<size_t>(_n_frames_count);
            auto& last_frame  = ud.cast<size_t>(_n_last_frame);
            auto hjp          = body.joint_processor_cast_get<HolderJointProcessor>(_n_joint_processor).lock();

            if (!hjp) {
                // Todo: assert
            }

            auto last_frame_start = ud.cast<double>(_n_frame_pref + std::to_string(last_frame) + "_start");
            size_t found_idx = last_frame;

            if (factor < last_frame_start)
                found_idx = 0;

            for (; found_idx != frames_count - 1; ++found_idx) {
                auto start = ud.cast<double>(_n_frame_pref + std::to_string(found_idx + 1) + "_start");

                if (factor < start)
                    break;
            }

            last_frame = found_idx;
            auto angle = ud.cast<float>(_n_frame_pref + std::to_string(last_frame) + "_angle");

            DEBUG_VAL_LMAO(angle);

            hjp->hold_angle(angle);
        }

        StrT _n_shift, _n_frame_pref, _n_last_frame, _n_frames_count, _n_joint_processor, _n_counter;
    };

    void AnimatedJoint::update_bindings() {
        _n_shift           = "__" + _name + "_shift";
        _n_frame_pref      = "__" + _name + "_frame";
        _n_last_frame      = "__" + _name + "_last_frame";
        _n_frames_count    = "__" + _name + "_frames_count";
        _n_joint_processor = "__" + _name + "_joint_processor";
        _n_counter         = "__" + _name + "_counter_name";
        _n_update          = "__" + _name + "_update";

        size_t last_valid_index = std::numeric_limits<size_t>::max();

        for (size_t ib = 0; ib < _bindings.size(); ++ib) {
            auto& b = _bindings[ib];

            if (auto body = b.body.lock()) {
                if (!body->is_joint_processor_exists(_n_joint_processor)) {
                    auto& ud = body->user_data();

                    ud[_n_shift]      = 0.0;
                    ud[_n_last_frame] = size_t(0);
                    ud[_n_counter]    = b.counter_name;

                    if (b.joint_index == -1) {
                        // Todo: assert
                    }

                    body->joint_processor_new<HolderJointProcessor>(_n_joint_processor, b.joint_index);
                    body->add_update(_n_update, AnimatedJointFunctor{
                            _n_shift, _n_frame_pref, _n_last_frame, _n_frames_count, _n_joint_processor, _n_counter
                    });

                    if (last_valid_index != std::numeric_limits<size_t>::max()) {
                        // Todo: set frames
                    } else
                        ud[_n_frames_count] = size_t(0);


                } else
                    last_valid_index = ib;
            }
        }
    }
} // namespace motion_interface



void MotionInterface::Human::create_ticker_joint(
        PhysicHumanBody& human,
        int body_joint,
        StrCRefT name,
        StrCRefT counter_name)
{
    if (!ticker_joint_is_exists(human, name)) {
        using BJ = PhysicHumanBody::BodyJoint;

        auto hjp = human.joint_processor_new<HolderJointProcessor>(name, BJ(body_joint));

        human.user_data()[name + "_upper_limit"] = PhysicHumanBody::joints_angle_limits->max;
        human.user_data()[name + "_lower_limit"] = PhysicHumanBody::joints_angle_limits->min;
        human.user_data()[name + "_last_time"] = human.user_data().cast<double>(std::string("__") + counter_name + "_acc");

        HolderJointProcessor::Pressets::human_leg_fast_tense(*hjp.lock());
        hjp.lock()->hold_angle(PhysicHumanBody::joints_angle_limits->max);

        human.add_update(name + "_update", [counter_name, name](PhysicBodyBase& body, double timestep) {
            auto& human = static_cast<PhysicHumanBody&>(body);
            auto& hjp   = *human.joint_processor_cast_get<HolderJointProcessor>(name).lock();

            auto hold  = hjp.hold_angle();
            auto lower = human.user_data().cast<float>(name + "_lower_limit");
            auto upper = human.user_data().cast<float>(name + "_upper_limit");
            auto& time  = human.user_data().cast<double>(name + "_last_time");
            auto current_time = human.user_data().cast<double>(std::string("__") + counter_name + "_acc");

            if (current_time < time)
                hjp.hold_angle(hold == upper ? lower : upper);

            time = current_time;
        });


    }
    else {
        // Todo: log warning
    }
}

void MotionInterface::Human::remove_ticker_joint(PhysicHumanBody& human, StrCRefT name) {
    if (ticker_joint_is_exists(human, name)) {
        human.remove_update(name + "_update");
        human.user_data().erase(name + "_upper_limit");
        human.user_data().erase(name + "_lower_limit");
        human.user_data().erase(name + "_last_time");
    }
    else {
        // Todo: log warning
    }
}

void MotionInterface::Human::ticker_joint_change_dir(PhysicHumanBody& human, StrCRefT name) {
    if (ticker_joint_is_exists(human, name)) {
        auto& hjp = *human.joint_processor_cast_get<HolderJointProcessor>(name).lock();
        auto hold  = hjp.hold_angle();
        auto lower = human.user_data().cast<float>(name + "_lower_limit");
        auto upper = human.user_data().cast<float>(name + "_upper_limit");

        hjp.hold_angle(hold == upper ? lower : upper);
    }
    else {
        // Todo: log warning
    }
}

void MotionInterface::Human::ticker_joint_set_constraint(PhysicHumanBody& human, StrCRefT name, float min, float max) {
    if (ticker_joint_is_exists(human, name)) {
        auto& hjp = *human.joint_processor_cast_get<HolderJointProcessor>(name).lock();
        auto  hold  = hjp.hold_angle();
        auto& lower = human.user_data().cast<float>(name + "_lower_limit");
        auto& upper = human.user_data().cast<float>(name + "_upper_limit");

        auto old_upper = upper;

        lower = min;
        upper = max;

        hjp.hold_angle(hold == old_upper ? lower : upper);
    }
    else {
        // Todo: log warning
    }
}

bool MotionInterface::Human::ticker_joint_is_exists(PhysicHumanBody& human, StrCRefT name) {
    return human.is_joint_processor_exists(name);
}
