#pragma once

#include <string>

class PhysicBodyBase;
class PhysicHumanBody;

struct MotionInterfaces {
    static void create_periodic_counter(
            PhysicBodyBase& body,
            const std::string& name,
            double period,
            double start = 0.0);

    static void delete_periodic_counter(PhysicBodyBase& body, const std::string& name);

    struct HumanRun {
        static void apply           (PhysicHumanBody& human);
        static void destroy         (PhysicHumanBody& human);
        static bool is_exists       (const PhysicHumanBody& human);
        static void set_run_period  (PhysicHumanBody& human, double time);
        static void set_run_state   (PhysicHumanBody& human, float state);
        static void set_thigh_limits(PhysicHumanBody& human, float lower, float upper);
    };
};