#include "PhysicSimulation.hpp"

#include <Box2D/Box2D.h>
#include <SFML/Graphics/ConvexShape.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/Texture.hpp>

#include "PhysicHumanBody.hpp"


#define MASS_FACT 0.01f

class ContactFilter : public b2ContactFilter {
public:
    bool ShouldCollide(b2Fixture* a, b2Fixture* b) override {
        return PhysicHumanBody::shouldCollide(a, b);
    }
};


static b2Body* createBox(b2World& world, const b2Vec2& pos, float mass, const b2Vec2& size, const b2Vec2& velocity) {
    b2BodyDef body_def;
    body_def.type = b2_dynamicBody;
    body_def.position = pos;
    body_def.bullet = true;
    body_def.angularDamping = 0.f;
    //body_def.angle = 1.5f;
    b2Body* body = world.CreateBody(&body_def);

    b2PolygonShape dynamic_box;

    //std::vector<b2Vec2> points;
    //points.emplace_back(from_meters(-0.04f, -0.08f));
    //points.emplace_back(from_meters(-0.08f,  0.12f));
    //points.emplace_back(from_meters( 0.08f,  0.12f));
    //points.emplace_back(from_meters( 0.04f, -0.08f));
//
    //dynamic_box.Set(points.data(), points.size());
    //dynamic_box.m_radius = 0.005f;
    dynamic_box.SetAsBox(size.x, size.y);

    b2FixtureDef fixture_def;
    fixture_def.shape = &dynamic_box;
    fixture_def.density = 1.0f;
    fixture_def.friction = 0.3f;
    fixture_def.restitution = 0.6f;

    body->CreateFixture(&fixture_def);

    b2MassData data;
    body->GetMassData(&data);
    data.mass = mass;
    body->SetMassData(&data);

    body->SetLinearVelocity(velocity);

    return body;
}

PhysicSimulation::PhysicSimulation() {
    _world = std::make_unique<b2World>(b2Vec2(0.0f, -9.8f));

    _contact_filter = std::make_unique<ContactFilter>();
    _world->SetContactFilter(_contact_filter.get());
}

PhysicSimulation::~PhysicSimulation() {
    clearDrawableManager();
}

auto PhysicSimulation::createTestSimulation() -> PhysicSimulation::UniquePtr {
    auto ps = PhysicSimulation::createUnique();

    {
        b2BodyDef body_def;
        body_def.position.Set(0.0f, -10.f);
        b2Body* body = ps->_world->CreateBody(&body_def);

        b2PolygonShape box;
        //ground_box.m_radius = 0.005f;
        box.SetAsBox(200.f, 10.f);

        b2FixtureDef fixture_def;
        fixture_def.density = 0.f;
        fixture_def.shape = &box;
        fixture_def.friction = 1.f;

        body->CreateFixture(&fixture_def);
    }

    /*
    for (int j = 0; j < 25; ++j)
        for (int i = 0; i < 25 - j; ++i)
            createBox(*ps->_world, b2Vec2(0.0f + i / 4.f, 104.f + j / 4.f), b2Vec2(0.1f, 0.1f));
            */


    return ps;
}


void PhysicSimulation::update() {
    if (_on_pause) {
        _last_update_time = timer().timestamp();
        return;
    }

    if (!_world)
        return;

    auto current_time = timer().timestamp();
    auto delta_time = (current_time - _last_update_time).sec();

    if (_force_update || delta_time > _step_time) {
        step(delta_time);
        _last_update_time = current_time;
    }
}

void PhysicSimulation::step(double delta_time) {
    if (!_world)
        return;

    auto delta = _adaptive_timestep && delta_time > _step_time ? delta_time : _step_time;

    if (delta > MIN_STEP)
        delta = _step_time;

    delta /= _slowdown_factor;

    for (auto& body : _bodies) {
        if (body->_update_func)
            body->_update_func();
    }

    _world->Step(delta, _velocity_iters, _position_iters);
    _simulation_time += delta;

    for (auto& pair : _post_callbacks)
        pair.second(*this);

    if (_debug_draw)
        updateDebugDraw();
}


void PhysicSimulation::attachDrawableManager(const DrawableManagerSP& drawable_manager)  {
    if (_drawable_manager == drawable_manager)
        return;

    clearDrawableManager();

    _drawable_manager = drawable_manager;

    // Reset debug draw;
    if (_debug_draw) {
        _debug_draw = false;
        enableDebugDraw();
    }
}


auto PhysicSimulation::detachDrawableManager()  {
    clearDrawableManager();

    DrawableManagerSP res = _drawable_manager;
    _drawable_manager = nullptr;
    return res;
}

static void setSfmlConvexFromB2Polygon(sf::ConvexShape* cvx, b2PolygonShape* poly) {
    cvx->setPointCount(size_t(poly->m_count));

    for (int i = 0; i < poly->m_count; ++i) {
        b2Vec2 pos = poly->m_vertices[i];
        cvx->setPoint(size_t(i), sf::Vector2f(pos.x, -pos.y));
    }
}

static void setSfmlCircle(sf::ConvexShape* cvx, float radius, size_t points_count = 32) {
    cvx->setPointCount(points_count + 4);

    float angle = 0;
    float delta = M_PIf32 * 2 / points_count;
    float epsilon = 0.001f;

    for (size_t i = 0; i < points_count; ++i) {
        cvx->setPoint(i, {cos(angle) * radius, sin(angle) * radius});
        angle += delta;
    }

    // Draw line
    cvx->setPoint(points_count    , {radius, 0});
    cvx->setPoint(points_count + 1, {0, 0});
    cvx->setPoint(points_count + 2, {0, 0});
    cvx->setPoint(points_count + 3, {radius, 0});
}

static void setSfmlFromB2(sf::ConvexShape* sf_circ, b2CircleShape* b2_circ) {
    setSfmlCircle(sf_circ, b2_circ->m_radius);
    sf_circ->setPosition(b2_circ->m_p.x, -b2_circ->m_p.y);
}

void PhysicSimulation::debug_draw(bool value) {
    if (value)
        enableDebugDraw();
    else
        disableDebugDraw();
}

void PhysicSimulation::clearDrawableManager() {
    for (auto pair : _draw_map)
        _drawable_manager->remove(pair.second);

    _draw_map.clear();
}

void PhysicSimulation::disableDebugDraw() {
    if (!_debug_draw)
        return;

    _debug_draw = false;

    if (!_drawable_manager)
        return;

    clearDrawableManager();

    _debug_draw = false;
}


void PhysicSimulation::enableDebugDraw() {
    if (_debug_draw)
        return;

    _debug_draw = true;

    if (!_drawable_manager)
        return;

    createDebugDrawObjects();
    updateDebugDraw();
}

void PhysicSimulation::createDebugDrawObjects() {
    if (!_debug_draw)
        return;

    auto body = _world->GetBodyList();

    while (body) {
        auto fixtures = body->GetFixtureList();

        while (fixtures) {
            if (_draw_map.find(fixtures) != _draw_map.end()) {
                fixtures = fixtures->GetNext();
                continue;
            }

            sf::Shape* shape = nullptr;

            switch (fixtures->GetType()) {
                case b2Shape::e_polygon: {
                    auto cvx_shape = _drawable_manager->create<sf::ConvexShape>();
                    shape = cvx_shape;

                    _draw_map[fixtures] = cvx_shape;
                    setSfmlConvexFromB2Polygon(cvx_shape, reinterpret_cast<b2PolygonShape*>(fixtures->GetShape()));
                } break;

                case b2Shape::e_circle: {
                    auto sf_shape = _drawable_manager->create<sf::ConvexShape>();
                    shape = sf_shape;

                    _draw_map[fixtures] = sf_shape;
                    setSfmlFromB2(sf_shape, reinterpret_cast<b2CircleShape*>(fixtures->GetShape()));
                } break;

                default:
                    std::cout << "\t\tUnhandled shape" << std::endl;
                    break;
            }

            if (shape) {
                if (fixtures->GetBody()->GetType() == b2_dynamicBody) {
                    auto color = sf::Color::Green;
                    color.a = 30;
                    shape->setFillColor(color);
                    shape->setOutlineColor(sf::Color::Green);
                    shape->setOutlineThickness(-0.02f);
                }
                else {
                    auto color = sf::Color::Magenta;
                    color.a = 30;
                    shape->setFillColor(color);
                    shape->setOutlineColor(sf::Color::Magenta);
                    shape->setOutlineThickness(-0.02f);
                }
            }

            fixtures = fixtures->GetNext();
        }

        body = body->GetNext();
    }
}

void PhysicSimulation::updateDebugDraw() {
    if (!_debug_draw)
        return;

    for (auto pair : _draw_map) {
         b2Fixture*    b2_fixture  = pair.first;
         sf::Drawable* sf_drawable = pair.second;

         b2Vec2 b2_pos = b2_fixture->GetBody()->GetPosition();

         if (b2_fixture->GetType() == b2Shape::e_polygon) {
             auto b2_shape = reinterpret_cast<b2PolygonShape*>(b2_fixture->GetShape());
             auto sf_shape = reinterpret_cast<sf::ConvexShape*>(sf_drawable);

             sf_shape->setPosition(b2_pos.x, -b2_pos.y);
             sf_shape->setRotation(-b2_fixture->GetBody()->GetAngle() * 180.f / M_PIf32);
         }
         else if (b2_fixture->GetType() == b2Shape::e_circle) {
             auto b2_shape = reinterpret_cast<b2CircleShape*>(b2_fixture->GetShape());
             auto sf_shape = reinterpret_cast<sf::CircleShape*>(sf_drawable);

             sf_shape->setPosition(b2_pos.x, -b2_pos.y);
             sf_shape->setRotation(-b2_fixture->GetBody()->GetAngle() * 180 / M_PIf32);
         }
    }
}

auto PhysicSimulation::spawnBox(float x, float y, float mass, scl::Vector2f velocity) -> PhysicSimpleBodyWP {
    auto ptr = createBox(*_world, b2Vec2(x, y), mass * MASS_FACT, b2Vec2(0.1f, 0.1f), b2Vec2{velocity.x(), velocity.y()});
    createDebugDrawObjects();
    updateDebugDraw();

    auto body = std::make_shared<SimpleBody>(_world.get(), ptr);
    _bodies.emplace(body);

    return std::weak_ptr<SimpleBody>(body);
}

auto PhysicSimulation::createHumanBody(const scl::Vector2f& position, float height, float mass) -> PhysicHumanBodyWP {
    auto res = std::shared_ptr<PhysicHumanBody>(new PhysicHumanBody(*_world, b2Vec2{position.x(), position.y()}, height, mass));
    _bodies.emplace(res);

    createDebugDrawObjects();
    updateDebugDraw();

    return std::weak_ptr(res);
}

void PhysicSimulation::deleteBody(std::weak_ptr<PhysicBodyBase> body) {
    if (auto lock = body.lock()) {
        lock.get()->destroy();

        _bodies.erase(lock);
    }

    clearDrawableManager();
    createDebugDrawObjects();
    updateDebugDraw();
}

void PhysicSimulation::gravity(float x, float y) {
    _world->SetGravity(b2Vec2(x, y));
}