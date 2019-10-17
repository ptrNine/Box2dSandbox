#include "src/Engine.hpp"
#include "src/core/time.hpp"
#include "src/graphics/Camera.hpp"

void Engine::mainCreate() {
    auto drawable_manager = DrawableManager::createShared("Drawable manager");

    auto window = Window::createShared();
    addWindow(window);

    auto camera = Camera::createShared("Camera1");
    camera->attachDrawableManager(drawable_manager);
    window->addCamera(camera);

    physic_simulation = PhysicSimulation::createUnique();
    physic_simulation->gravity(0, 0);

    window->addUiCallback("Ui callback", uiPhysics(drawable_manager));

    window->addEventCallback("Event callback", [](Window& wnd, const sf::Event& evt){

    });
}


int main(int argc, char* argv[]) {
    auto engine = Engine();
    return engine.run(argc, argv);
}