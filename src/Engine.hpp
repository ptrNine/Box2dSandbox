#pragma once

#include <flat_hash_map.hpp>

#include "core/helper_macros.hpp"
#include "graphics/Window.hpp"
#include "graphics/ObjectManager.hpp"
#include "game/PhysicSimulation.hpp"

class Window;
class PhysicSimulation;

class Engine {
public:
    struct WindowParams {
        bool destroy_on_close = true;
    };

    using WindowManager = ObjectManager<Window, WindowParams>;

public:
    Engine ();
    ~Engine() = default;

    int run(int argc, char* argv[]);

private:
    void onCreate();
    void mainUpdate();

    void gameMain();

private:
    WindowManager window_manager;
    std::unique_ptr<PhysicSimulation> physic_simulation;
};


//inline auto& engine() {
//    return Engine::instance();
//}