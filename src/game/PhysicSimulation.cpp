#include "PhysicSimulation.hpp"

#include <Box2D/Box2D.h>
#include <SFML/Graphics/ConvexShape.hpp>
#include <SFML/Graphics/Texture.hpp>


static void createBox(b2World& world, const b2Vec2& pos, const b2Vec2& size) {
    b2BodyDef body_def;
    body_def.type = b2_dynamicBody;
    body_def.position = pos;
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
}

PhysicSimulation::PhysicSimulation() {
    _world = std::make_unique<b2World>(b2Vec2(0.0f, -9.8f));

    b2BodyDef ground_body_def;
    ground_body_def.position.Set(0.0f, -10.f);
    b2Body* ground_body = _world->CreateBody(&ground_body_def);

    b2PolygonShape ground_box;
    //ground_box.m_radius = 0.005f;
    ground_box.SetAsBox(50.f, 10.f);

    ground_body->CreateFixture(&ground_box, 0.0f);

    for (int j = 0; j < 25; ++j)
        for (int i = 0; i < 25 - j; ++i)
            createBox(*_world, b2Vec2(0.0f + i / 4.f, 104.f + j / 4.f), b2Vec2(0.1f, 0.1f));
}


PhysicSimulation::~PhysicSimulation() {
    clearDrawableManager();
}


void PhysicSimulation::update() {
    if (_on_pause) {
        _last_update_time = timer().timestamp();
        return;
    }

    if (!_world)
        return;

    auto current_time = timer().timestamp();

    if ((current_time - _last_update_time).sec() > _step_time) {
        step();
        _last_update_time = current_time;
    }
}

void PhysicSimulation::step() {
    if (!_world)
        return;

    _world->Step(_step_time, _velocity_iters, _position_iters);

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

void PhysicSimulation::setSfmlConvexFromB2Polygon(sf::ConvexShape* cvx, b2PolygonShape* poly) {
    cvx->setPointCount(size_t(poly->m_count));

    for (int i = 0; i < poly->m_count; ++i) {
        b2Vec2 pos = poly->m_vertices[i];
        cvx->setPoint(size_t(i), sf::Vector2f(pos.x, -pos.y));
    }
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
    auto body = _world->GetBodyList();

    while (body) {
        auto fixtures = body->GetFixtureList();

        while (fixtures) {
            if (_draw_map.find(fixtures) != _draw_map.end()) {
                fixtures = fixtures->GetNext();
                continue;
            }

            switch (fixtures->GetType()) {
                case b2Shape::e_polygon: {
                    auto cvx_shape = _drawable_manager->create<sf::ConvexShape>();
                    _draw_map[fixtures] = cvx_shape;
                    setSfmlConvexFromB2Polygon(cvx_shape, reinterpret_cast<b2PolygonShape*>(fixtures->GetShape()));

                    if (fixtures->GetBody()->GetType() == b2_dynamicBody) {
                        auto color = sf::Color::Green;
                        color.a = 30;
                        cvx_shape->setFillColor(color);
                        cvx_shape->setOutlineColor(sf::Color::Green);
                        cvx_shape->setOutlineThickness(-0.02f);
                    }
                    else {
                        auto color = sf::Color::Magenta;
                        color.a = 30;
                        cvx_shape->setFillColor(color);
                        cvx_shape->setOutlineColor(sf::Color::Magenta);
                        cvx_shape->setOutlineThickness(-0.02f);
                    }
                } break;

                default:
                    std::cout << "\t\tUnhandled shape" << std::endl;
                    break;
            }

            fixtures = fixtures->GetNext();
        }

        body = body->GetNext();
    }
}

void PhysicSimulation::updateDebugDraw() {
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
    }
}

void PhysicSimulation::spawnBox(float x, float y) {
    createBox(*_world, b2Vec2(x, y), b2Vec2(0.1f, 0.1f));
    createDebugDrawObjects();
    updateDebugDraw();
}