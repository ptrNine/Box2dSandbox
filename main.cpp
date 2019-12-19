#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/ConvexShape.hpp>

#include "src/Engine.hpp"
#include "src/graphics/nuklear.hpp"

#include "src/machine_learning/NeuralNetwork.hpp"
#include "src/core/time.hpp"
#include "src/graphics/Camera.hpp"
#include "src/graphics/CameraManipulator.hpp"
#include "src/graphics/HUD.hpp"
#include "src/EngineState.hpp"
#include "src/game/KeyCombo.hpp"
#include "src/game/PhysicHumanBody.hpp"

#include "src/core/math.hpp"
#include "src/machine_learning/Neuron.hpp"
#include <Box2D/Dynamics/Joints/b2RevoluteJoint.h>

#include "src/game/RepeaterJointProcessor.hpp"
#include "src/game/HolderJointProcessor.hpp"
#include "src/game/MotionInterfaces.hpp"

void Engine::mainCreate() {
    auto wnd = Window::createShared();
    addWindow(wnd);

    auto drawable_manager = DrawableManager::createShared("Drawable manager");

    auto camera = Camera::createShared("Camera1", 16.f/9.f, 20.f);
    camera->attachDrawableManager(drawable_manager);
    wnd->addCamera(camera);

    auto camera_manipulator = CameraManipulator::createShared();
    camera->attachCameraManipulator(camera_manipulator);

    auto hud = HUD::createShared();
    camera->attachHUD(hud);

    auto& font = font_manager().get("/usr/share/fonts/truetype/freefont/FreeMono.ttf");
    auto fpsText = hud->drawable_manager()->create<sf::Text>("", font);

    static float box_mass = 1.0f;
    static bool start_shoot = false;
    static float human_height = 1.8f;
    static bool on_height_edit = false;
    static scl::Vector2f start_pos;
    static scl::Vector2f start_pos_wnd;
    static sf::ConvexShape* shot_shape;
    static std::weak_ptr<PhysicHumanBody> last_body;

    static auto physics_callback = [wnd, camera](PhysicSimulation& it) {
        if (auto human = last_body.lock()) {
            //if (auto arm_l = human->joint_processor_cast_get<HolderJointProcessor>("arm_l").lock()) {
            //    auto cursor_pos = wnd->getMouseCoords(*camera);
            //    auto joint_pos  = human->joint_position(PhysicHumanBody::BodyJoint_Chest_ArmL);
            //    auto dir = (cursor_pos - joint_pos).normalize();
            //    auto chest_angle = human->part_angle(PhysicHumanBody::BodyPartChest);
//
            //    arm_l->set_hold_angle_if_valid(
            //            math::angle::constraint(
            //                    std::atan2(dir.x(), -dir.y()) - chest_angle) - math::angle::radian(17.f));
            //}
        }
    };

    wnd->addRenderCallback("Hud callback", [fpsText, camera](Window& wnd) {
        scl::String info;
        auto [x, y] = wnd.getMouseCoords(*camera);
        info.sprintf("{}  Mouse world pos: {: .2f}, {: .2f}", engine_state().fps_str(), x, y);

        if (start_shoot)
            info.sprintf("{}  Velocity: {: .2f}ms  Mass: {: .3f}kg",
                         info, ((scl::Vector2f{x, y} - start_pos) * 10).magnitude(), box_mass);

        if (on_height_edit)
            info.sprintf("{} Height: {: .3f}m", info, human_height);

        if (auto human = last_body.lock()) {
            info.sprintf("{} Human speed: {: .2f}ms", info, human->velocity().magnitude());

            if (auto cast = human->ground_raycast_shin_left_info())
                info.sprintf("{} Confirm: {: .2f}, {: .2f}m", info, cast->distance.x(), cast->distance.y());
        }

        fpsText->setString(info.data());
    });

    camera_manipulator->attachEventCallback([this, drawable_manager] CAMERA_MANIPULATOR_EVENT_CALLBACK(it, cam, evt, wnd) {
        float speed = 1.0f;

        if (start_shoot) {
            float thickness = 4.f;
            auto coords = wnd.getMouseCoords();

            auto move = coords - start_pos_wnd;
            float colorf = 255 * (move.magnitude() / wnd.sizef().magnitude());
            uint8_t color = colorf > 255 ? 255 : static_cast<uint8_t>(colorf);

            move.set(-move.y(), move.x());
            move.makeNormalize();
            move.makeScalarMul(thickness);

            shot_shape->setPoint(0, sf::Vector2f(start_pos_wnd.x(), -start_pos_wnd.y()));
            shot_shape->setPoint(1, sf::Vector2f(coords.x(), -coords.y()));
            shot_shape->setPoint(2, sf::Vector2f(coords.x() + move.x(), -coords.y() - move.y()));
            shot_shape->setPoint(3, sf::Vector2f(start_pos_wnd.x() + move.x(), -start_pos_wnd.y() - move.y()));

            shot_shape->setFillColor(sf::Color(color, 255 - color, 0, 125));
        }

        if (evt.type == sf::Event::KeyPressed) {
            if (evt.key.code == sf::Keyboard::A)
                cam.move(-speed, 0.f);
            else if (evt.key.code == sf::Keyboard::D)
                cam.move(speed, 0.f);
            else if (evt.key.code == sf::Keyboard::W)
                cam.move(0.f, speed);
            else if (evt.key.code == sf::Keyboard::S)
                cam.move(0.f, -speed);
            else if (evt.key.code == sf::Keyboard::E)
                cam.rotate(5.f);
            else if (evt.key.code == sf::Keyboard::R) {
                physic_simulation = PhysicSimulation::createTestSimulation();
                physic_simulation->debug_draw(true);
                physic_simulation->attachDrawableManager(drawable_manager);
                physic_simulation->addPostUpdateCallback("clbk", physics_callback);
            }
            else if (evt.key.code == sf::Keyboard::H)
                on_height_edit = true;
            else if (evt.key.code == sf::Keyboard::X) {
                if (auto body = last_body.lock())
                    body->makeMirror();
            }
            else if (evt.key.code == sf::Keyboard::LBracket) {
                if (auto body = last_body.lock()) {
                    for(size_t i = 0; i < PhysicHumanBody::BodyJoint_COUNT; ++i)
                        body->freeze(PhysicHumanBody::BodyJoint(i));
                }
            }
        }
        else if (evt.type == sf::Event::KeyReleased) {
            if (evt.key.code == sf::Keyboard::H)
                on_height_edit = false;
            else if (evt.key.code == sf::Keyboard::LBracket) {
                if (auto body = last_body.lock()) {
                    for(size_t i = 0; i < PhysicHumanBody::BodyJoint_COUNT; ++i)
                        body->unfreeze(PhysicHumanBody::BodyJoint(i));
                }
            }
        }
        else if (evt.type == sf::Event::MouseWheelMoved) {
            if (start_shoot) {
                box_mass += evt.mouseWheel.delta * 0.1f * box_mass;
            } else if (on_height_edit) {
                human_height += evt.mouseWheel.delta * 0.1f * human_height;
            } else
                cam.eye_width(cam.eye_width() - evt.mouseWheel.delta);
        }
        else if (evt.type == sf::Event::MouseButtonPressed) {
            if (evt.mouseButton.button == sf::Mouse::Left && !start_shoot) {
                start_shoot = true;
                start_pos = wnd.getMouseCoords(cam);
                start_pos_wnd = wnd.getMouseCoords();

                shot_shape = cam.hud()->drawable_manager()->create<sf::ConvexShape>();
                shot_shape->setPointCount(4);
                b2Joint* j;
            }
            else if (evt.mouseButton.button == sf::Mouse::Right) {
                auto pos = wnd.getMouseCoords(cam);
                last_body = physic_simulation->createHumanBody(pos, human_height, 80.f);
                last_body.lock()->makeMirror();

                auto& human = *last_body.lock();

                auto pc = motion_interface::PeriodicCounter(last_body, "counter");
                pc.set_period(0.5);

                auto jp1n = motion_interface::AnimatedJoint(last_body, "animated_leg_r", PhysicHumanBody::BodyJoint_Chest_ThighR, "counter")
                    .set_frames({{0.0, -0.3f}, {0.5, 0.6f}})
                    .n_joint_processor();

                auto jp2n = motion_interface::AnimatedJoint(last_body, "animated_leg_l", PhysicHumanBody::BodyJoint_Chest_ThighL, "counter")
                    .set_frames({{0.0, -0.3}, {0.5, 0.6}})
                    .set_shift(0.5)
                    .n_joint_processor();

                auto jp1 = human.joint_processor_cast_get<HolderJointProcessor>(jp1n);
                auto jp2 = human.joint_processor_cast_get<HolderJointProcessor>(jp2n);
                HolderJointProcessor::Pressets::human_leg_fast_tense(*jp1.lock());
                HolderJointProcessor::Pressets::human_leg_fast_tense(*jp2.lock());

                auto shin_l = human.joint_processor_new<HolderJointProcessor>("shin_l", PhysicHumanBody::BodyJoint_ThighL_ShinL);
                auto shin_r = human.joint_processor_new<HolderJointProcessor>("shin_r", PhysicHumanBody::BodyJoint_ThighR_ShinR);
                HolderJointProcessor::Pressets::human_shin_superweak(*shin_l.lock());
                HolderJointProcessor::Pressets::human_shin_superweak(*shin_r.lock());

                auto arm_l = human.joint_processor_new<HolderJointProcessor>("arm_l", PhysicHumanBody::BodyJoint_Chest_ArmL, 1.4f);
                auto arm_r = human.joint_processor_new<HolderJointProcessor>("arm_r", PhysicHumanBody::BodyJoint_Chest_ArmR, 1.4f);
                auto hand_l = human.joint_processor_new<HolderJointProcessor>("hand_l", PhysicHumanBody::BodyJoint_ArmL_HandL);
                auto hand_r = human.joint_processor_new<HolderJointProcessor>("hand_r", PhysicHumanBody::BodyJoint_ArmR_HandR);

                HolderJointProcessor::Pressets::human_hand_fast_tense(*arm_l.lock());
                HolderJointProcessor::Pressets::human_hand_fast_tense(*arm_r.lock());
                HolderJointProcessor::Pressets::human_hand_fast_tense(*hand_l.lock());
                HolderJointProcessor::Pressets::human_hand_fast_tense(*hand_r.lock());
            }
            else if (evt.mouseButton.button == sf::Mouse::Middle) {
                if (auto human = last_body.lock()) {
                    if (auto jp = human->joint_processor_cast_get<HolderJointProcessor>("hand_l").lock()) {
                        auto hand_pos  = human->part_position(PhysicHumanBody::BodyPartHandL);
                        auto joint_pos = human->joint_position(PhysicHumanBody::BodyJoint_ArmL_HandL);
                        auto dir = (hand_pos - joint_pos).normalize();

                        human->apply_impulse(PhysicHumanBody::BodyPartHandL, dir * -0.34, hand_pos + scl::Vector2f{-dir.y(), dir.x()} * 0.1f, true);
                    }
                }
            }
        }
        else if (evt.type == sf::Event::MouseButtonReleased) {
            if (evt.mouseButton.button == sf::Mouse::Left && start_shoot) {
                cam.hud()->drawable_manager()->remove(shot_shape);

                start_shoot = false;
                auto vel = (wnd.getMouseCoords(cam) - start_pos) * 10;
                physic_simulation->spawnBox(start_pos.x(), start_pos.y(), box_mass, vel);
            }
        }
    });

    physic_simulation = PhysicSimulation::createTestSimulation();
    physic_simulation->debug_draw(true);
    physic_simulation->attachDrawableManager(drawable_manager);
    physic_simulation->addPostUpdateCallback("clbk", physics_callback);
    //physic_simulation->gravity(0, 0);

    wnd->addUiCallback("Physics Ui", uiPhysics(drawable_manager));
}


int main(int argc, char* argv[]) {
    auto engine = Engine();
    return engine.run(argc, argv);

    //scl::Array a{1, 2, 3, 4};
    //a.reduce(std::plus<>{});
    return 0;
}
