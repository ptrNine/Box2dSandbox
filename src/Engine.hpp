#pragma once

#include <flat_hash_map.hpp>

#include "core/helper_macros.hpp"
#include "graphics/Window.hpp"
#include "graphics/ObjectManager.hpp"
#include "game/PhysicSimulation.hpp"
#include "graphics/FontManager.hpp"

class Window;
class PhysicSimulation;


struct WindowParams {
    bool destroy_on_close = true;
};


class Engine {
public:
    using WindowManager = ObjectManager<Window, WindowParams>;

public:
    Engine ();
    ~Engine() = default;

    int run(int argc, char* argv[]);


public:
    // Ui callbacks
    Window::UiCallbackT uiPhysics(DrawableManagerSP& drawable_manager);

    FontManager& font_manager() {
        return _font_manager;
    }

private:
    void onCreate();
    void onUpdate();
    void mainCreate();

    void addWindow   (const std::shared_ptr<Window>& window, const WindowParams& params = WindowParams());
    void removeWindow(const std::shared_ptr<Window>& window);

private:
    ska::flat_hash_map<std::shared_ptr<Window>, WindowParams> _windows;
    std::unique_ptr<PhysicSimulation> physic_simulation;
    FontManager _font_manager;
};
