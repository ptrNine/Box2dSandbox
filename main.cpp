#include "src/Engine.hpp"
#include "src/graphics/nuklear.hpp"

#include "src/machine_learning/NeuralNetwork.hpp"
#include "src/core/time.hpp"
#include "src/graphics/Camera.hpp"
#include "src/graphics/CameraManipulator.hpp"
#include "src/EngineState.hpp"

void Engine::mainCreate() {

    auto wnd = Window::createShared();
    addWindow(wnd);

    auto drawable_manager = DrawableManager::createShared("Drawable manager");

    auto camera = Camera::createShared("Camera1", 16.f/9.f, 20.f);
    camera->attachDrawableManager(drawable_manager);
    wnd->addCamera(camera);

    auto camera_manipulator = CameraManipulator::createShared();
    camera->attachCameraManipulator(camera_manipulator);

    camera_manipulator->attachEventCallback([](CameraManipulator& it, Camera& cam, const sf::Event& evt) {
        float speed = 1.0f;

        if (evt.type == sf::Event::KeyPressed) {
            if (evt.key.code == sf::Keyboard::A)
                cam.move(-speed, 0.f);
            else if (evt.key.code == sf::Keyboard::D)
                cam.move(speed, 0.f);
            else if (evt.key.code == sf::Keyboard::W)
                cam.move(0.f, speed);
            else if (evt.key.code == sf::Keyboard::S)
                cam.move(0.f, -speed);
            else if (evt.key.code == sf::Keyboard::R)
                cam.rotate(5.f);
        }
        else if (evt.type == sf::Event::MouseWheelMoved) {
            cam.eye_width(cam.eye_width() - evt.mouseWheel.delta);
        }
    });


    physic_simulation = PhysicSimulation::createTestSimulation();
    physic_simulation->debug_draw(true);
    physic_simulation->attachDrawableManager(drawable_manager);


    wnd->addEventCallback("Events", [this, camera](Window& it, const sf::Event& evt) {
        if (evt.type == sf::Event::MouseButtonPressed) {
            if (evt.mouseButton.button == sf::Mouse::Left) {
                auto pos = it.getCurrentCoords();
                physic_simulation->spawnBox(pos.x, pos.y);
            }
        }
    });


    wnd->addUiCallback("Physics Ui", uiPhysics(drawable_manager));
}

int main(int argc, char* argv[]) {
    auto engine = Engine();
    return engine.run(argc, argv);
}