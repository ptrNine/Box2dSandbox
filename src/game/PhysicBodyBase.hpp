#pragma once

#include <functional>

class PhysicBodyBase {
    friend class PhysicSimulation;

protected:
    void setWorld(class b2World* world) {_world = world; }
    virtual void destroy() = 0;

    class b2World* _world = nullptr;
    std::function<void()> _update_func;
};

class SimpleBody : public PhysicBodyBase {
    friend class PhysicSimulation;
public:
    SimpleBody(class b2World* world, class b2Body* body) {
        _world = world;
        _body = body;
    }

    void destroy() override;

protected:
    class b2Body* _body;
};