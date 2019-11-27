#pragma once

#include <mutex>

#include <flat_hash_map.hpp>
#include <memory>

#include "../graphics/DrawableManager.hpp"

#include "../core/helper_macros.hpp"
#include "../core/time.hpp"

namespace sf {
    class Drawable;
    class ConvexShape;
}

class b2World;
class b2Fixture;
class b2PolygonShape;

class PhysicSimulation {
public:
    static constexpr float MASS_FACTOR = 0.01;
    static constexpr float MIN_STEP    = 1/15.f;

    using DrawableManagerSP = std::shared_ptr<DrawableManager>;
    using B2WorldUP         = std::unique_ptr<b2World>;
    using PhysicBodyBaseWP  = std::weak_ptr<class PhysicBodyBase>;
    using PhysicHumanBodyWP = std::weak_ptr<class PhysicHumanBody>;
    using PhysicSimpleBodyWP = std::weak_ptr<class SimpleBody>;
    using ContactFilterUP   = std::unique_ptr<class ContactFilter>;

    using UpdatePostCallbackT = std::function<void(PhysicSimulation&)>;

    DrawableManagerSP _drawable_manager;
    B2WorldUP         _world;
    ContactFilterUP   _contact_filter;

    // Fabric methods
public:
    PhysicHumanBodyWP createHumanBody(const scl::Vector2f& position, float height = 1.8f, float mass = 80.f);
    void deleteBody(std::weak_ptr<PhysicBodyBase> body);

    PhysicSimpleBodyWP spawnBox(float x, float y, float mass = 1.0f, scl::Vector2f velocity = {0, 0});

public:
    DECLARE_SELF_FABRICS(PhysicSimulation);

    static UniquePtr createTestSimulation();

    PhysicSimulation();
    ~PhysicSimulation();

    void attachDrawableManager(const DrawableManagerSP& drawable_manager);
    auto detachDrawableManager();

    void update();
    void step(double delta_time);
    void step() { step(_step_time / _slowdown_factor); }

    double simulation_time() {
        return _simulation_time;
    }

    void addPostUpdateCallback(const std::string& name, const UpdatePostCallbackT& callback) {
        _post_callbacks.emplace(name, callback);
    }

    void removePostUpdateCallback(const std::string& name) {
        _post_callbacks.erase(name);
    }

private:
    void enableDebugDraw();
    void disableDebugDraw();
    void createDebugDrawObjects();
    void updateDebugDraw();
    void clearDrawableManager();

private:
    ska::flat_hash_set<std::shared_ptr<class PhysicBodyBase>> _bodies;
    ska::flat_hash_map<b2Fixture*, sf::Drawable*> _draw_map;
    ska::flat_hash_map<std::string, UpdatePostCallbackT> _post_callbacks;
    bool _debug_draw = false;
    bool _on_pause   = false;
    bool _adaptive_timestep = true;
    bool _force_update = false;

    float   _step_time       = 1.f/60.f;
    int32_t _velocity_iters  = 8;
    int32_t _position_iters  = 3;
    double  _simulation_time = 0;
    double  _slowdown_factor = 1.0;

    Timestamp _last_update_time = timer().timestamp();

public:
    // Getters / setters
    void debug_draw(bool value);
    bool debug_draw() { return _debug_draw; }

    DECLARE_GET_SET(velocity_iters);
    DECLARE_GET_SET(position_iters);
    DECLARE_GET_SET(step_time);
    DECLARE_GET_SET(on_pause);
    DECLARE_GET_SET(adaptive_timestep);
    DECLARE_GET_SET(slowdown_factor);
    DECLARE_GET_SET(force_update);

    void gravity(float x, float y);
};
