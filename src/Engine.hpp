#pragma once

#include <flat_hash_map.hpp>

#include "core/helper_macros.hpp"
#include "graphics/Window.hpp"
#include "graphics/ObjectManager.hpp"

class Window;

class Engine {
    //SINGLETON_IMPL(Engine)

public:
    struct WindowParams {
        bool destroy_on_close = true;
    };

    using WindowManager = ObjectManager<Window, WindowParams>;
//
    Engine ();
    //~Engine() = default;

public:
    int run(int argc, char* argv[]);

private:
    void onCreate();
    void mainUpdate();

private:
    WindowManager window_manager;
};


//inline auto& engine() {
//    return Engine::instance();
//}