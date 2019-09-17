#pragma once

#include <mutex>

#include <flat_hash_map.hpp>

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
    using DrawableManagerSP = std::shared_ptr<DrawableManager>;
    using B2WorldUP         = std::unique_ptr<b2World>;

    DrawableManagerSP _drawable_manager;
    B2WorldUP         _world;

public:
    PhysicSimulation();
    ~PhysicSimulation();

    void attachDrawableManager(const DrawableManagerSP& drawable_manager);
    auto detachDrawableManager();

    void spawnBox(float x, float y);

    void step();

private:
    void enableDebugDraw();
    void disableDebugDraw();
    void createDebugDrawObjects();
    void updateDebugDraw();
    void clearDrawableManager();

    void setSfmlConvexFromB2Polygon(sf::ConvexShape* cvx, b2PolygonShape* poly);

private:
    ska::flat_hash_map<b2Fixture*, sf::Drawable*> _draw_map;
    bool _debug_draw = false;
    bool _on_pause   = false;


    float _step_time  = 1.f/60.f;
    int32_t _velocity_iters = 8;
    int32_t _position_iters = 3;

    Timestamp _last_update_time = timer().timestamp();

public:
    // Getters / setters
    void debug_draw(bool value);
    bool debug_draw() { return _debug_draw; }

    DEFINE_GET_SET(velocity_iters);
    DEFINE_GET_SET(position_iters);
    DEFINE_GET_SET(step_time);
    DEFINE_GET_SET(on_pause);
};
