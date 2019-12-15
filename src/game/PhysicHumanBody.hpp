#pragma once

#include <Box2D/Common/b2Math.h>

#include <scl/vector2.hpp>
#include <Box2D/Dynamics/b2WorldCallbacks.h>
#include "PhysicBodyBase.hpp"
#include "JointProcessor.hpp"

class PhysicHumanBody : public PhysicBodyBase {
    friend class PhysicSimulation;
    friend class ContactFilter;

public:
    enum ShapeType {
        ShapeTypeCircle,
        ShapeTypeBox
    };

    enum BodyPart {
        BodyPartHead = 0,
        BodyPartChest,

        BodyPartHandL,
        BodyPartArmL,
        BodyPartHandR,
        BodyPartArmR,

        BodyPartThighL,
        BodyPartShinL,
        BodyPartThighR,
        BodyPartShinR,

        BodyPart_COUNT
    };

    static constexpr std::string_view part_names[BodyPart_COUNT] {
            "Head",
            "Chest",
            "HandL",
            "ArmL",
            "HandR",
            "ArmR",
            "ThighL",
            "ShinL",
            "ThighR",
            "ShinR"
    };

    static constexpr ShapeType parts_shape_types[BodyPart_COUNT] {
            ShapeTypeCircle, // Head
            ShapeTypeBox,    // Chest

            ShapeTypeBox,    // Left Hand
            ShapeTypeBox,    // Left Arm
            ShapeTypeBox,    // Right Hand
            ShapeTypeBox,    // Right Arm

            ShapeTypeBox,    // Left Thigh
            ShapeTypeBox,    // Left Shin
            ShapeTypeBox,    // Right Thigh
            ShapeTypeBox     // Right Shin
    };

    static constexpr float PART_MAX = 7.f;
    static constexpr float parts_heights[BodyPart_COUNT] {
            1.0f / PART_MAX, // Head
            2.5f / PART_MAX, // Chest

            1.5f / PART_MAX, // Left Hand
            1.5f / PART_MAX, // Left Arm
            1.5f / PART_MAX, // Right Hand
            1.5f / PART_MAX, // Right Arm

            1.9f / PART_MAX, // Left Thigh
            1.6f / PART_MAX, // Left Shin
            1.9f / PART_MAX, // Right Thigh
            1.6f / PART_MAX  // Right Shin
    };

    static constexpr float parts_widths[BodyPart_COUNT] {
            1.0f / PART_MAX, // Head
            1.2f / PART_MAX, // Chest

            0.7f / PART_MAX, // Left Hand
            0.7f / PART_MAX, // Left Arm
            0.7f / PART_MAX, // Right Hand
            0.7f / PART_MAX, // Right Arm

            0.7f / PART_MAX, // Left Thigh
            0.7f / PART_MAX, // Left Leg
            0.7f / PART_MAX, // Right Thigh
            0.7f / PART_MAX  // Right Leg
    };

    enum BodyEqPart {
        BodyEqPartFoot = 0,
        BodyEqPartShin,
        BodyEqPartHip,
        BodyEqPartWrist,
        BodyEqPartForearm,
        BodyEqPartShoulder,
        BodyEqPartHead,
        BodyEqPartUpperTorso,
        BodyEqPartMiddleTorso,
        BodyEqPartLowerTorso,

        BodyEqPart_COUNT
    };

    static constexpr float parts_mass_eq_k[BodyEqPart_COUNT][3] {
            { -0.83f, 0.008f,  0.007f }, // Foot
            { -1.59f, 0.036f,  0.012f }, // Shin
            { -2.65f, 0.146f,  0.014f }, // Hip
            { -0.12f, 0.004f,  0.002f }, // Wrist
            {  0.32f, 0.014f, -0.001f }, // Forearm
            {  0.25f, 0.030f, -0.003f }, // Shoulder
            {  1.30f, 0.017f,  0.014f }, // Head
            {  8.21f, 0.186f, -0.058f }, // Upper Torso
            {  7.18f, 0.223f, -0.066f }, // Middle Torso
            { -7.50f, 0.098f,  0.049f }, // Lower Torso
    };

    enum BodyJoint {
        BodyJoint_Chest_ArmL,
        BodyJoint_Chest_ArmR,
        BodyJoint_ArmL_HandL,
        BodyJoint_ArmR_HandR,
        BodyJoint_Chest_ThighL,
        BodyJoint_Chest_ThighR,
        BodyJoint_ThighL_ShinL,
        BodyJoint_ThighR_ShinR,
        BodyJoint_Head_Chest,
        BodyJoint_COUNT
    };

    static constexpr std::string_view joint_names[BodyJoint_COUNT] {
        "Chest_ArmL",
        "Chest_ArmR",
        "ArmL_HandL",
        "ArmR_HandR",
        "Chest_ThighL",
        "Chest_ThighR",
        "ThighL_ShinL",
        "ThighR_ShinR",
        "Head_Chest"
    };

    static constexpr struct { BodyPart what; BodyPart with; }
            joints_connections[BodyJoint_COUNT] {
            { .what = BodyPartChest,  .with = BodyPartArmL   },
            { .what = BodyPartChest,  .with = BodyPartArmR   },
            { .what = BodyPartArmL,   .with = BodyPartHandL  },
            { .what = BodyPartArmR,   .with = BodyPartHandR  },
            { .what = BodyPartChest,  .with = BodyPartThighL },
            { .what = BodyPartChest,  .with = BodyPartThighR },
            { .what = BodyPartThighL, .with = BodyPartShinL  },
            { .what = BodyPartThighR, .with = BodyPartShinR  },
            { .what = BodyPartHead,   .with = BodyPartChest  }
    };

    static constexpr struct { float min; float max; }
            joints_angle_limits[BodyJoint_COUNT] {
            { .min = -M_PIf32,        .max = M_PI_2f32 + 0.2f }, // Chest with Left Shoulder
            { .min = -M_PIf32,        .max = M_PI_2f32 + 0.2f }, // Chest with Right Shoulder
            { .min = -M_PIf32 + 0.4f, .max = 0.1f },             // Left Shoulder with Left Forearm
            { .min = -M_PIf32 + 0.4f, .max = 0.1f },             // Right Shoulder with Right Forearm
            { .min = -M_PIf32 + 0.4f, .max = M_PI_4f32 },        // Chest with Left Thigh
            { .min = -M_PIf32 + 0.4f, .max = M_PI_4f32 },        // Chest with Right Thigh
            { .min = -0.1f,           .max = M_PIf32 - 0.4f },   // Left Thigh with Left Thigh
            { .min = -0.1f,           .max = M_PIf32 - 0.4f },   // Right Thigh with Right Thigh
            { .min = -0.8f,           .max = 0.8f }              // Head with Chest
    };

    static constexpr struct { BodyPart what; BodyPart with; }
            enabled_collisions[] {
            { .what = BodyPartHead,  .with = BodyPartShinL  },
            { .what = BodyPartHead,  .with = BodyPartShinR  },
            { .what = BodyPartHead,  .with = BodyPartThighL },
            { .what = BodyPartHead,  .with = BodyPartThighR },
            { .what = BodyPartChest, .with = BodyPartShinL  },
            { .what = BodyPartChest, .with = BodyPartShinR  }
    };

    static inline bool should_collide(BodyPart a_type, BodyPart b_type) {
        auto check_comb = [a_type, b_type](BodyPart what, BodyPart with) {
            return  ((a_type == what && b_type == with) ||
                     (a_type == with && b_type == what));
        };

        bool result = true;
        for (auto& p : enabled_collisions)
            result = result && check_comb(p.what, p.with);

        return result;
    }

    static float calcBodyPartMass(BodyPart part, float height, float body_mass) {
        auto calc = [height, body_mass](BodyEqPart p) {
            return parts_mass_eq_k[p][0] +
                   parts_mass_eq_k[p][1] * body_mass +
                   parts_mass_eq_k[p][2] * height;
        };

        switch (part) {
            case BodyPartHead:
                return calc(BodyEqPartHead);

            case BodyPartArmL:
            case BodyPartArmR:
                return calc(BodyEqPartShoulder);

            case BodyPartHandL:
            case BodyPartHandR:
                return calc(BodyEqPartWrist) + calc(BodyEqPartForearm);

            case BodyPartThighL:
            case BodyPartThighR:
                return calc(BodyEqPartHip);

            case BodyPartShinL:
            case BodyPartShinR:
                return calc(BodyEqPartShin) + calc(BodyEqPartFoot);

            case BodyPartChest:
                return calc(BodyEqPartUpperTorso) + calc(BodyEqPartMiddleTorso) + calc(BodyEqPartLowerTorso);

            default:
                // Todo: assert
                return 1;
        }
    }

    static b2Vec2 calcPartPos(BodyPart part, b2Vec2 pos, float height) {
        pos.y += parts_heights[BodyPartShinR] * height / 2.f;

        switch (part) {
            case BodyPartHead:
                pos.y += parts_heights[BodyPartShinR]  * height / 2.f;
                pos.y += parts_heights[BodyPartThighR] * height;
                pos.y += parts_heights[BodyPartChest]  * height;
                pos.y += parts_heights[BodyPartHead]   * height / 2.f;
                break;

            case BodyPartChest:
                pos.y += parts_heights[BodyPartShinR]  * height / 2.f;
                pos.y += parts_heights[BodyPartThighR] * height;
                pos.y += parts_heights[BodyPartChest]  * height / 2.f;
                break;

            case BodyPartArmL:
                pos.y += parts_heights[BodyPartShinR]  * height / 2.f;
                pos.y += parts_heights[BodyPartThighR] * height;
                pos.y += parts_heights[BodyPartChest]  * height;
                pos.y -= parts_heights[BodyPartArmL]   * height / 2.f;
                pos.y -= parts_widths[BodyPartArmL]    * height / 4.f;
                break;

            case BodyPartArmR:
                pos.y += parts_heights[BodyPartShinR]  * height / 2.f;
                pos.y += parts_heights[BodyPartThighR] * height;
                pos.y += parts_heights[BodyPartChest]  * height;
                pos.y -= parts_heights[BodyPartArmR]   * height / 2.f;
                pos.y -= parts_widths[BodyPartArmR]    * height / 4.f;
                break;

            case BodyPartHandL:
                pos = calcPartPos(BodyPartArmL, pos, height);
                pos.y -= parts_heights[BodyPartArmL]  * height;
                pos.y -= parts_heights[BodyPartHandL] * height / 2.f;
                break;

            case BodyPartHandR:
                pos = calcPartPos(BodyPartArmR, pos, height);
                pos.y -= parts_heights[BodyPartArmR]  * height;
                pos.y -= parts_heights[BodyPartHandR] * height / 2.f;
                break;

            case BodyPartThighL:
                pos.y += parts_heights[BodyPartShinR]  * height / 2.f;
                pos.y += parts_heights[BodyPartThighR] * height / 2.f;
                break;

            case BodyPartThighR:
                pos.y += parts_heights[BodyPartShinR]  * height / 2.f;
                pos.y += parts_heights[BodyPartThighR] * height / 2.f;
                break;

            default:
                // Todo: assert
                break;
        }

        return pos;
    }

public:
    template <typename T>
    using JointTraverseF = std::function<T(const T& val, class b2Joint*)>;

    PhysicHumanBody(class b2World& world, const b2Vec2& pos, float height = 1.8f, float mass = 80.f);

    void makeMirror();

    void enableMotor (BodyJoint joint, float speed, float torque);
    void disableMotor(BodyJoint joint);

    void freeze  (BodyJoint joint);
    void unfreeze(BodyJoint joint);

    void apply_impulse_to_center(BodyPart part, const scl::Vector2f& impulse, bool wake);
    void apply_impulse(BodyPart part, const scl::Vector2f& impulse, const scl::Vector2f& pos, bool wake);

    void enable_ground_cast(bool shin_left, bool shin_right, bool chest) {
        _ground_raycast.enable_shin_l = shin_left;
        _ground_raycast.enable_shin_r = shin_right;
        _ground_raycast.enable_chest  = chest;
    }

    template <typename T, typename... ArgsT>
    auto joint_processor_new(const std::string& name, BodyJoint joint, ArgsT&&... args) {
        return _jpm.create<T>(name, _b2_joints[joint], args...);
    }

    void remove_joint_processor(const std::string& name) {
        _jpm.erase(name);
    }

    template <typename T, size_t _Sz>
    T joint_traverse(const std::array<BodyJoint, _Sz>& joints, const JointTraverseF<T>& callback, T initial) {
        for (auto j : joints)
            initial = callback(initial, _b2_joints[j]);
        return initial;
    }

    // Getters

    scl::Vector2f velocity() const;
    scl::Vector2f velocity(BodyPart part) const;

    scl::Vector2f center_of_mass() const;

    float angular_speed() const;
    float angular_speed(BodyPart part) const;

    float part_angle(BodyPart part) const;
    float part_angle() const;

    scl::Vector2f part_position(BodyPart part) const;

    float joint_angle(BodyJoint joint) const;
    float joint_motor_speed(BodyJoint joint) const;
    float joint_speed(BodyJoint joint) const;
    float joint_reaction_torque(BodyJoint joint, float dt) const;

    scl::Vector2f joint_position(BodyJoint joint) const;

    bool ground_raycast_chest_confirm     () const { return _ground_raycast.chest_confirm && _ground_raycast.enable_chest; }
    bool ground_raycast_shin_left_confirm () const { return _ground_raycast.shin_l_confirm && _ground_raycast.enable_shin_l; }
    bool ground_raycast_shin_right_confirm() const { return _ground_raycast.shin_r_confirm && _ground_raycast.enable_shin_r; }

    struct GroundRaycastInfo { scl::Vector2f position, normal, distance; };
    using GroundRaycastInfoOpt = std::optional<GroundRaycastInfo>;

    GroundRaycastInfoOpt ground_raycast_chest_info() const;
    GroundRaycastInfoOpt ground_raycast_shin_left_info() const;
    GroundRaycastInfoOpt ground_raycast_shin_right_info() const;

    auto joint_processor_get(const std::string& name) const {
        return _jpm.get(name);
    }

    template <typename T>
    auto joint_processor_cast_get(const std::string& name) const {
        return _jpm.cast_get<T>(name);
    }

    scl::Vector<scl::String> joint_processors_list() const {
        scl::Vector<scl::String> res;
        for (auto& p : _jpm.data())
            res.emplace_back(p.first);

        return res;
    }

    bool is_joint_processor_exists(const std::string& name) const {
        return _jpm.data().find(name) != _jpm.data().end();
    }

protected:
    void destroy() override;

private:
    void createHumanBody           (class b2World& world, const b2Vec2& pos, float height, float mass);
    static auto createHumanBodyPart(class b2World& world, uint32_t id, BodyPart type, const b2Vec2& pos, float height, float human_mass);
    static bool shouldCollide      (class b2Fixture* a, class b2Fixture* b);

    //void destroy(class b2World& world);

private:
    class RayCastCallback : public b2RayCastCallback {
    public:
        float32 ReportFixture(b2Fixture* fixture, const b2Vec2& point, const b2Vec2& normal, float32 fraction) override;

        BodyPart part;
        PhysicHumanBody* owner;
    } _raycast_shin_l, _raycast_shin_r, _raycast_chest;

private:
    float _height;
    float _mass;
    bool _left_orientation = true;

    struct {
        scl::Vector2f shin_left_ground_pos     = {0.f, 0.f};
        scl::Vector2f shin_right_ground_pos    = {0.f, 0.f};
        scl::Vector2f shin_left_ground_normal  = {0.f, 0.f};
        scl::Vector2f shin_right_ground_normal = {0.f, 0.f};

        scl::Vector2f chest_ground_pos     = {0.f, 0.f};
        scl::Vector2f chest_ground_normal  = {0.f, 0.f};

        bool shin_l_confirm = false;
        bool shin_r_confirm = false;
        bool chest_confirm  = false;

        bool enable_shin_l = true;
        bool enable_shin_r = false;
        bool enable_chest  = false;
    } _ground_raycast;

    class b2Body* _b2_parts [BodyPart_COUNT] = {nullptr};
    class b2RevoluteJoint* _b2_joints[BodyJoint_COUNT] = {nullptr};
    bool _freezed_joints[BodyJoint_COUNT] = {false};

    JointProcessorManager _jpm;
};