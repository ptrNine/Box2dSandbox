#pragma once

struct MotionInterfaces {
    struct HumanRun {
        static void apply    (class PhysicHumanBody& human);
        static void destroy  (class PhysicHumanBody& human);
        static bool is_exists(const class PhysicHumanBody& human);
    };
};