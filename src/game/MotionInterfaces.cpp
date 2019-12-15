#include "MotionInterfaces.hpp"

#include <scl/scl.hpp>

#include "PhysicHumanBody.hpp"
#include "HolderJointProcessor.hpp"
#include "../core/math.hpp"


// Periodic counter
void MotionInterfaces::create_periodic_counter(PhysicBodyBase& body, const std::string& name, double period, double start) {
    body.user_data()[name + "_period"] = period;
    body.user_data()[name + "_acc"] = start;
    body.user_data()[name + "_used_by"] = size_t(0);
}

void MotionInterfaces::delete_periodic_counter(PhysicBodyBase& body, const std::string& name) {
    if (!body.user_data().has(name + "_used_by")) {
        // Todo: log warning
        return;
    }
    if (body.user_data().cast<size_t>(name + "_used_by") != 0) {
        // Todo: log warning
        return;
    }

    body.user_data().erase(name + "_period");
    body.user_data().erase(name + "_acc");
    body.user_data().erase(name + "_used_by");
}



auto operated_joints = scl::Array{
    PhysicHumanBody::BodyJoint_Chest_ThighL,
    PhysicHumanBody::BodyJoint_Chest_ThighR,
    PhysicHumanBody::BodyJoint_ThighL_ShinL,
    PhysicHumanBody::BodyJoint_ThighR_ShinR
};

void MotionInterfaces::HumanRun::apply(PhysicHumanBody& human) {
    if (!is_exists(human)) {
        using BJ = PhysicHumanBody::BodyJoint;
        auto joint_processors = scl::Vector<HolderJointProcessor::WeakPtr>(operated_joints.size());


        for (size_t i = 0; i < joint_processors.size(); ++i) {
            joint_processors[i] = human.joint_processor_new<HolderJointProcessor>(
                    std::string("Motion::") + PhysicHumanBody::joint_names[operated_joints[i]].data(),
                    operated_joints[i], 0);
            HolderJointProcessor::Pressets::human_leg_fast_tense(*joint_processors[i].lock());
        }

        joint_processors[0].lock()->hold_angle(0.8f);
        joint_processors[1].lock()->hold_angle(-0.8f);

        human.user_data()["_leg_time_period"] = 0.6;
        human.user_data()["_leg_time_acc"] = 0.0;
        human.user_data()["_leg_upper_limit"] = 0.8f;
        human.user_data()["_leg_lower_limit"] = -0.8f;
        human.user_data()["_leg_enable_interpolation"] = false;


        human.add_update("Motion::update", [joint_processors] (PhysicBodyBase& body, double timestep) {
            auto& human = static_cast<PhysicHumanBody&>(body);

            auto locked = joint_processors.map(
                    [](const HolderJointProcessor::WeakPtr& w) { return w.lock(); });
            auto all_locked = locked.reduce(
                    [](bool r, const HolderJointProcessor::SharedPtr& s) { return r && s; }, true);

            if (all_locked) {
                HMAP_GET(double&, _leg_time_acc,    human.user_data());
                HMAP_GET(double&, _leg_time_period, human.user_data());
                HMAP_GET(float,   _leg_upper_limit, human.user_data());
                HMAP_GET(float,   _leg_lower_limit, human.user_data());
                HMAP_GET(bool,    _leg_enable_interpolation, human.user_data());

                if (_leg_enable_interpolation) {
                    _leg_time_acc = fmod(_leg_time_acc + timestep, _leg_time_period);

                    auto factor = std::fabs(2.f * float(math::inverse_lerp(0.0, _leg_time_period, _leg_time_acc)) - 1.f);

                    locked[0]->hold_angle(
                            math::lerp(_leg_lower_limit, _leg_upper_limit, factor));
                    locked[1]->hold_angle(
                            math::lerp(_leg_lower_limit, _leg_upper_limit, 1.f - factor));
                }
                else {
                    _leg_time_acc += timestep;
                    if (_leg_time_acc > _leg_time_period / 2.f) {
                        _leg_time_acc = fmod(_leg_time_acc, _leg_time_period / 2.f);

                        float tmp = locked[0]->hold_angle();
                        locked[0]->hold_angle(locked[1]->hold_angle());
                        locked[1]->hold_angle(tmp);
                    }
                }
            }
        });
    } else {
        // Todo: log warning
    }
}

void MotionInterfaces::HumanRun::set_run_period(PhysicHumanBody& human, double time) {
    human.user_data()["_leg_time_period"] = time;
}

void MotionInterfaces::HumanRun::set_run_state(PhysicHumanBody& human, float state) {
    human.user_data()["_leg_time_acc"] =
            human.user_data().cast<double>("_leg_time_period") * std::clamp(state, 0.f, 1.f);
}

void MotionInterfaces::HumanRun::set_thigh_limits(class PhysicHumanBody& human, float lower, float upper) {
    human.user_data()["_leg_upper_limit"] = math::angle::constraint(lower);
    human.user_data()["_leg_lower_limit"] = math::angle::constraint(upper);
}

void MotionInterfaces::HumanRun::destroy(PhysicHumanBody& human) {
    if (is_exists(human)) {
        for (auto j : operated_joints)
            human.remove_joint_processor(std::string("Motion::") + PhysicHumanBody::joint_names[j].data());

        human.remove_update("Motion::update");
        human.user_data().erase("_leg_time_period");
        human.user_data().erase("_leg_time_acc");
        human.user_data().erase("_leg_upper_limit");
        human.user_data().erase("_leg_lower_limit");
    } else {
        // Todo: log warning
    }
}

bool MotionInterfaces::HumanRun::is_exists(const PhysicHumanBody& human) {
    bool res = human.is_update_exists("Motion::update") &&
               human.user_data().has("_leg_time_period") &&
               human.user_data().has("_leg_time_acc"   ) &&
               human.user_data().has("_leg_upper_limit") &&
               human.user_data().has("_leg_lower_limit");

    for (auto j : operated_joints)
        res = res && human.is_joint_processor_exists(std::string("Motion::") + PhysicHumanBody::joint_names[j].data());

    return res;
}