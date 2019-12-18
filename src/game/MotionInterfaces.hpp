#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include "../core/helper_macros.hpp"

class PhysicBodyBase;
class PhysicHumanBody;
class BodyWithJoints;

namespace motion_interface {
    using StrT     = std::string;
    using StrRefT  = StrT&;
    using StrCRefT = const StrT&;

    template <typename T>
    using VecT     = std::vector<T>;
    template <typename T>
    using VecRefT  = VecT<T>&;
    template <typename T>
    using VecCRefT = const VecT<T>&;


    class PeriodicCounter {
    public:
        PeriodicCounter (const std::weak_ptr<PhysicBodyBase>& body, StrCRefT name);

        /// Bind to body
        void add_binding(const std::weak_ptr<PhysicBodyBase>& body);

        /// Bind to bodies
        template <typename WeakPtr>
        void add_binding(VecCRefT<WeakPtr> bodies) {
            for (auto& p : bodies)
                add_binding(p);
        }

        /// Set settings
        void set       (double period, double start) const;
        void set_start (double start = 0.f) const;
        void set_period(double period) const;

        /// Destroy all resources from bodies
        void destroy() const;

    protected:
        void update_bindings();

        VecT<std::weak_ptr<PhysicBodyBase>> _bodies;
        StrT _name;

        StrT _n_period;
        StrT _n_acc;
        StrT _n_used_by;
        StrT _n_update;
        StrT _n_factor;

    public:
        DECLARE_GET_SET(name);
    };


    class AnimatedJoint {
    public:
        struct Frame {
            double start_point;
            float  target_angle;
        };

        struct Binding {
            std::weak_ptr<BodyWithJoints> body;
            int joint_index;
            StrT counter_name;
        };

        AnimatedJoint(const std::weak_ptr<BodyWithJoints>& body, StrCRefT name, int joint_index = -1, StrCRefT counter_name = "");

        /// Bind to body
        void add_binding(const std::weak_ptr<BodyWithJoints>& body, int joint_index = -1, StrCRefT counter_name = "");

        /// Bind to bodies
        template <typename WeakPtr>
        void add_binding(VecCRefT<WeakPtr> bodies) {
            for (auto& p : bodies)
                add_binding(p);
        }

        const AnimatedJoint& set_frames(VecCRefT<Frame> frames) const;
        const AnimatedJoint& set_shift (double time_shift) const;

        void destroy() const;

    protected:
        void update_bindings();

        VecT<Binding> _bindings;
        StrT _name;

        StrT _n_frame_pref;
        StrT _n_last_frame;
        StrT _n_frames_count;
        StrT _n_shift;
        StrT _n_joint_processor;
        StrT _n_counter;
        StrT _n_update;

    public:
        DECLARE_GET(n_joint_processor);
    };
} // namespace motion_interface



struct MotionInterface {
    using StrT     = std::string;
    using StrRefT  = StrT&;
    using StrCRefT = const StrT&;

    template <typename T>
    using VecT     = std::vector<T>;
    template <typename T>
    using VecRefT  = VecT<T>&;
    template <typename T>
    using VecCRefT = const VecT<T>&;

    struct Human {
        static void create_ticker_joint        (PhysicHumanBody& human, int body_joint, StrCRefT name, StrCRefT counter_name);
        static void remove_ticker_joint        (PhysicHumanBody& human, StrCRefT name);
        static bool ticker_joint_is_exists     (PhysicHumanBody& human, StrCRefT name);
        static void ticker_joint_change_dir    (PhysicHumanBody& human, StrCRefT name);
        static void ticker_joint_set_constraint(PhysicHumanBody& human, StrCRefT name, float min, float max);


        struct AnimationFrame {
            double start_time   = 0.0;
            float  target_angle = 0.f;
        };

        static void create_animated_joint    (PhysicHumanBody& human, int body_joint, StrCRefT name, StrCRefT counter_name);
        static void remove_animated_joint    (PhysicHumanBody& human, StrCRefT name);
        static bool animated_joint_is_exists (PhysicHumanBody& human, StrCRefT name);
        static void animated_joint_set_frames(PhysicHumanBody& human, StrCRefT name, VecCRefT<AnimationFrame> frames);
    };
};