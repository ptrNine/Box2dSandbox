#include "MotionInterfaces.hpp"

#include <scl/scl.hpp>

#include "PhysicHumanBody.hpp"
#include "HolderJointProcessor.hpp"

void MotionInterfaces::HumanRun::apply(PhysicHumanBody& human) {
    if (!is_exists(human)) {
        auto thigh_l = human.joint_processor_new<HolderJointProcessor>
                ("Motion::thigh_l", PhysicHumanBody::BodyJoint_Chest_ThighL, 0);
        auto thigh_r = human.joint_processor_new<HolderJointProcessor>
                ("Motion::thigh_r", PhysicHumanBody::BodyJoint_Chest_ThighR, 0);
        auto shin_l = human.joint_processor_new<HolderJointProcessor>
                ("Motion::shin_l", PhysicHumanBody::BodyJoint_ThighL_ShinL, 0);
        auto shin_r = human.joint_processor_new<HolderJointProcessor>
                ("Motion::shin_r", PhysicHumanBody::BodyJoint_ThighR_ShinR, 0);

        human.user_data()["_leg_time_period"] = 0.2;
        human.user_data()["_leg_time_acc"] = 0.0;

        human.add_update("Motion::update", [thigh_l, thigh_r, shin_l, shin_r] (PhysicBodyBase& body, double timestep) {
            auto& human  = static_cast<PhysicHumanBody&>(body);
            auto  locked = scl::Array{thigh_l.lock(), thigh_r.lock(), shin_l.lock(), shin_r.lock()};
            auto  user_data = human.user_data().get_caster<double>();

            bool all_locked = true;
            for (auto& l : locked)
                all_locked = all_locked && l;

            if (all_locked) {

            }

        });
    } else {
        // Todo: log warning
    }
}

void MotionInterfaces::HumanRun::destroy(PhysicHumanBody& human) {
    if (is_exists(human)) {
        human.remove_update("Motion::update");
        human.remove_joint_processor("Motion::thigh_l");
        human.remove_joint_processor("Motion::thigh_r");
        human.remove_joint_processor("Motion::shin_l" );
        human.remove_joint_processor("Motion::shin_r" );
        human.user_data().erase("_leg_time_period");
        human.user_data().erase("_leg_time_acc");
    } else {
        // Todo: log warning
    }
}

bool MotionInterfaces::HumanRun::is_exists(const PhysicHumanBody& human) {
    return human.is_update_exists("Motion::update") &&
           human.is_joint_processor_exists("Motion::thigh_l") &&
           human.is_joint_processor_exists("Motion::thigh_r") &&
           human.is_joint_processor_exists("Motion::shin_l" ) &&
           human.is_joint_processor_exists("Motion::shin_r" ) &&
           human.user_data().has("_leg_time_period") &&
           human.user_data().has("_leg_time_acc");
}