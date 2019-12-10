#include "PhysicHumanBody.hpp"

#include <Box2D/Box2D.h>
#include <random>
#include "PhysicSimulation.hpp"

#include "../core/math.hpp"


// Todo: move
void SimpleBody::destroy() {
    _world->DestroyBody(_body);
}


PhysicHumanBody::PhysicHumanBody(b2World& world, const b2Vec2& pos, float height, float mass) {
    createHumanBody(world, pos, height, mass);

    _world = &world;

    _height = height;
    _mass   = mass * PhysicSimulation::MASS_FACTOR;

    _raycast_shin_l.owner = this;
    _raycast_shin_r.owner = this;
    _raycast_chest .owner = this;

    _raycast_shin_l.part = BodyPartShinL;
    _raycast_shin_r.part = BodyPartShinR;
    _raycast_chest .part = BodyPartChest;

    // Update function
    _update_functions.emplace_back("Main_update", [this](PhysicBodyBase&, double delta_time) {
        // Update raycasts
        auto find_end = [](b2Vec2 start, float angle, float height) {
            auto sn = sin(angle);
            auto cs = cos(angle);

            auto down = start + b2Vec2{0, -height};
            auto vec  = down - start;

            auto rot = b2Vec2(vec.x * cs - vec.y * sn,
                              vec.x * sn + vec.y * cs);

            return start + rot;
        };

        if (_ground_raycast.enable_shin_l) {
            b2Vec2 start = _b2_parts[BodyPartShinL]->GetPosition();
            b2Vec2 end   = find_end(start, _b2_parts[BodyPartShinL]->GetAngle(), _height * parts_heights[BodyPartShinL] * 2.f);

            _ground_raycast.shin_l_confirm = false;
            _world->RayCast(&_raycast_shin_l, start, end);
        }

        if (_ground_raycast.enable_shin_r) {
            b2Vec2 start = _b2_parts[BodyPartShinR]->GetPosition();
            b2Vec2 end   = find_end(start, _b2_parts[BodyPartShinR]->GetAngle(), _height * parts_heights[BodyPartShinR] * 2.f);

            _ground_raycast.shin_r_confirm = false;
            _world->RayCast(&_raycast_shin_r, start, end);
        }

        if (_ground_raycast.enable_chest) {
            b2Vec2 start = _b2_parts[BodyPartChest]->GetPosition();
            b2Vec2 end   = find_end(start, _b2_parts[BodyPartChest]->GetAngle(), _height);

            _ground_raycast.chest_confirm = false;
            _world->RayCast(&_raycast_chest, start, end);
        }

        // Update joint processors
        std::vector<decltype(_jpm.data().begin())> to_delete;
        for (auto iter = _jpm.data().begin(); iter != _jpm.data().end(); ++iter)
            if (iter->second->should_be_deleted())
                to_delete.push_back(iter);

        for (auto iter : to_delete)
            _jpm.data().erase(iter);

        for (auto& p : _jpm.data())
            p.second->update(*this, delta_time);
    });
}

void PhysicHumanBody::destroy() {
    for (auto joint : _b2_joints)
      _world->DestroyJoint(joint);

    for (auto part : _b2_parts)
        _world->DestroyBody(part);
}

float32 PhysicHumanBody::RayCastCallback::ReportFixture(
        b2Fixture* fixture,
        const b2Vec2& point,
        const b2Vec2& normal,
        float32 fraction)
{
    bool self_cast = false;

    for (auto b2_body : owner->_b2_parts) {
        auto fixt = b2_body->GetFixtureList();
        self_cast |= (fixture == fixt);
    }

    if (self_cast)
        return -1;

    switch (part) {
        case BodyPartShinL:
            owner->_ground_raycast.shin_left_ground_pos   .set(point.x, point.y);
            owner->_ground_raycast.shin_left_ground_normal.set(normal.x, normal.y);
            owner->_ground_raycast.shin_l_confirm = true;
            break;

        case BodyPartShinR:
            owner->_ground_raycast.shin_right_ground_pos   .set(point.x, point.y);
            owner->_ground_raycast.shin_right_ground_normal.set(normal.x, normal.y);
            owner->_ground_raycast.shin_r_confirm = true;
            break;

        case BodyPartChest:
            owner->_ground_raycast.chest_ground_pos   .set(point.x, point.y);
            owner->_ground_raycast.chest_ground_normal.set(normal.x, normal.y);
            owner->_ground_raycast.chest_confirm = true;
            break;

        default:
            break;
    }

    //std::cout << "Collision at " << point.x << ", " << point.y << std::endl;
    return fraction;
}

static auto rand_gen = std::bind(std::uniform_int_distribution<uint32_t>(1, std::numeric_limits<uint32_t>::min()), std::mt19937());

auto PhysicHumanBody::createHumanBodyPart(b2World& world, uint32_t id, BodyPart type, const b2Vec2& pos, float height, float human_mass) {
    b2BodyDef body_def;
    body_def.type = b2_dynamicBody;
    body_def.position = calcPartPos(BodyPart(type), pos, height);

    b2Body* body;
    body = world.CreateBody(&body_def);

    b2PolygonShape box;
    b2CircleShape circle;
    b2FixtureDef fixture_def;

    auto size = b2Vec2{height * parts_widths[type] / 2.f, height * parts_heights[type] / 2.f};
    if (parts_shape_types[type] == ShapeTypeBox) {
        box.SetAsBox(size.x, size.y);
        fixture_def.shape = &box;
    } else if (parts_shape_types[type] == ShapeTypeCircle) {
        circle.m_radius = height * parts_heights[type] / 2.f;
        fixture_def.shape = &circle;
    }

    if constexpr (sizeof(void*) == 4) {
        // Todo: ASSERT
    }
    else {
        auto data = reinterpret_cast<uint32_t*>(&fixture_def.userData);
        *data = id;
        *(data + 1) = static_cast<uint32_t>(type);
    }

    fixture_def.density = 1.0f;
    fixture_def.restitution = 0.6f;

    if (type == BodyPartShinL || type == BodyPartShinR)
        fixture_def.friction = 1.0f;
    else
        fixture_def.friction = 0.3f;

    body->CreateFixture(&fixture_def);

    b2MassData mass_data;
    body->GetMassData(&mass_data);
    mass_data.mass = calcBodyPartMass(type, height * 100, human_mass) * PhysicSimulation::MASS_FACTOR;
    body->SetMassData(&mass_data);

    using RetT = struct { b2Body* body; b2Vec2 pos; b2Vec2 size; };

    return RetT{ body, body_def.position, size };
}

bool PhysicHumanBody::shouldCollide(b2Fixture* a, b2Fixture* b) {
    void* a_data_p = a->GetUserData();
    void* b_data_p = b->GetUserData();

    if constexpr (sizeof(void*) == 4) {
        //Todo: NOT IMPLEMENTED
    }
    else if (a_data_p != nullptr) {
        auto a_data = reinterpret_cast<uint32_t*>(&a_data_p);
        auto b_data = reinterpret_cast<uint32_t*>(&b_data_p);

        if (*a_data == *b_data) {
            auto a_type = static_cast<BodyPart>(*(a_data + 1));
            auto b_type = static_cast<BodyPart>(*(b_data + 1));

            if (a_type < BodyPart_COUNT && b_type < BodyPart_COUNT)
                return should_collide(a_type, b_type);
        }
    }

    return true;
}

void PhysicHumanBody::createHumanBody(b2World& world, const b2Vec2& pos, float height, float mass) {
    scl::return_type_of_t<decltype(createHumanBodyPart)> parts[BodyPart_COUNT];

    uint32_t id = rand_gen();

    for (size_t i = 0; i < BodyPart_COUNT; ++i) {
        parts[i] = createHumanBodyPart(world, id, BodyPart(i), pos, height, mass);
        _b2_parts[i] = parts[i].body;
    }

    auto connector = [&world, &parts](BodyJoint joint) {
        auto upper = joints_connections[joint].what;
        auto lower = joints_connections[joint].with;

        auto min_angle = joints_angle_limits[joint].min;
        auto max_angle = joints_angle_limits[joint].max;

        b2Vec2 anchor_pos = parts[lower].pos;
        anchor_pos.y     += parts[lower].size.y;

        b2RevoluteJointDef joint_def;
        joint_def.Initialize(parts[upper].body, parts[lower].body, anchor_pos);
        joint_def.lowerAngle = min_angle;
        joint_def.upperAngle = max_angle;
        joint_def.enableLimit = true;

        return reinterpret_cast<b2RevoluteJoint*>(world.CreateJoint(&joint_def));
    };

    for (size_t i = 0; i < BodyJoint_COUNT; ++i)
        _b2_joints[i] = connector(static_cast<BodyJoint>(i));
}


void PhysicHumanBody::makeMirror() {
    for (auto joint : _b2_joints) {
        auto l = joint->GetLowerLimit();
        auto u = joint->GetUpperLimit();

        joint->SetLimits(-u, -l);
    }
    _left_orientation = !_left_orientation;
}

void PhysicHumanBody::enableMotor(BodyJoint joint_type, float speed, float torque) {
    b2RevoluteJoint* joint = _b2_joints[joint_type];

    joint->SetMotorSpeed(speed);
    joint->SetMaxMotorTorque(torque);
    joint->EnableMotor(true);
}

void PhysicHumanBody::disableMotor(BodyJoint joint) {
    _b2_joints[joint]->EnableMotor(false);
}

void PhysicHumanBody::freeze(BodyJoint joint_type) {
    b2RevoluteJoint* joint = _b2_joints[joint_type];
    joint->SetLimits(joint->GetJointAngle(), joint->GetJointAngle());
}

void PhysicHumanBody::unfreeze(BodyJoint joint_type) {
    b2RevoluteJoint* joint = _b2_joints[joint_type];

    if (_left_orientation)
        joint->SetLimits(joints_angle_limits[joint_type].min, joints_angle_limits[joint_type].max);
    else
        joint->SetLimits(-joints_angle_limits[joint_type].max, -joints_angle_limits[joint_type].min);

}

void PhysicHumanBody::apply_impulse_to_center(BodyPart part, const scl::Vector2f& impulse, bool wake) {
    _b2_parts[part]->ApplyLinearImpulseToCenter(b2Vec2(impulse.x(), impulse.y()), wake);
}

void PhysicHumanBody::apply_impulse(BodyPart part, const scl::Vector2f& impulse, const scl::Vector2f& pos, bool wake) {
    _b2_parts[part]->ApplyLinearImpulse(b2Vec2(impulse.x(), impulse.y()), b2Vec2(pos.x(), pos.y()), wake);
}

scl::Vector2f PhysicHumanBody::velocity(BodyPart part) const {
    auto vel = _b2_parts[part]->GetLinearVelocity();
    return {vel.x, vel.y};
}

scl::Vector2f PhysicHumanBody::velocity() const {
    return velocity(BodyPartChest);
}

float PhysicHumanBody::angular_speed(BodyPart part) const {
    return _b2_parts[part]->GetAngularVelocity();
}

float PhysicHumanBody::angular_speed() const {
    return angular_speed(BodyPartChest);
}

float PhysicHumanBody::part_angle(BodyPart part) const {
    return math::angle::constraint(_b2_parts[part]->GetAngle());
}

float PhysicHumanBody::part_angle() const {
    return part_angle(BodyPartChest);
}

float PhysicHumanBody::joint_angle(BodyJoint joint) const {
    return math::angle::constraint(_b2_joints[joint]->GetJointAngle());
}

float PhysicHumanBody::joint_motor_speed(BodyJoint joint) const {
    return _b2_joints[joint]->GetMotorSpeed();
}

float PhysicHumanBody::joint_speed(BodyJoint joint) const {
    return _b2_joints[joint]->GetJointSpeed();
}

float PhysicHumanBody::joint_reaction_torque(BodyJoint joint, float dt) const {
    return _b2_joints[joint]->GetReactionTorque(dt);
}

auto PhysicHumanBody::ground_raycast_chest_info() const -> GroundRaycastInfoOpt {
    if (!ground_raycast_chest_confirm())
        return std::nullopt;

    b2Vec2 body_pos = _b2_parts[BodyPartChest]->GetPosition();
    auto distance = scl::Vector2f(body_pos.x, body_pos.y);

    distance = _ground_raycast.chest_ground_pos - distance;
    distance -= distance.normalize() * parts_heights[BodyPartChest] * _height * 0.5f;

    return GroundRaycastInfo{_ground_raycast.chest_ground_pos, _ground_raycast.chest_ground_normal, distance};
}

auto PhysicHumanBody::ground_raycast_shin_left_info() const -> GroundRaycastInfoOpt {
    if (!ground_raycast_shin_left_confirm())
        return std::nullopt;

    b2Vec2 body_pos = _b2_parts[BodyPartShinL]->GetPosition();
    auto distance = scl::Vector2f(body_pos.x, body_pos.y);

    distance = _ground_raycast.shin_left_ground_pos - distance;
    distance -= distance.normalize() * parts_heights[BodyPartShinL] * _height * 0.5f;

    return GroundRaycastInfo{_ground_raycast.shin_left_ground_pos, _ground_raycast.shin_left_ground_normal, distance};
}

auto PhysicHumanBody::ground_raycast_shin_right_info() const -> GroundRaycastInfoOpt {
    if (!ground_raycast_shin_right_confirm())
        return std::nullopt;

    b2Vec2 body_pos = _b2_parts[BodyPartShinR]->GetPosition();
    auto distance = scl::Vector2f(body_pos.x, body_pos.y);

    distance = _ground_raycast.shin_right_ground_pos - distance;
    distance -= distance.normalize() * parts_heights[BodyPartShinR] * _height * 0.5f;

    return GroundRaycastInfo{_ground_raycast.shin_right_ground_pos, _ground_raycast.shin_right_ground_normal, distance};
}

scl::Vector2f PhysicHumanBody::center_of_mass() const {
    auto res = scl::Vector2f{0, 0};

    for (b2Body* body : _b2_parts) {
        auto pos = body->GetPosition();
        res += scl::Vector2f{pos.x, pos.y} * body->GetMass();
    }

    return res / _mass;
}

scl::Vector2f PhysicHumanBody::part_position(BodyPart part) const {
    auto pos = _b2_parts[part]->GetPosition();
    return {pos.x, pos.y};
}

scl::Vector2f PhysicHumanBody::joint_position(BodyJoint joint) const {
    auto pos = _b2_joints[joint]->GetAnchorA();
    return {pos.x, pos.y};
}