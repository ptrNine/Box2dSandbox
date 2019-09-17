#include "Engine.hpp"

#include "graphics/nuklear.hpp"
#include "graphics/Camera.hpp"
#include "EngineState.hpp"

Engine::Engine() : window_manager("WindowManager") {

}

int Engine::run(int argc, char* argv[]) {

    onCreate();
    mainUpdate();

    return 0;
}

void Engine::onCreate() {
    gameMain();
}

void Engine::mainUpdate() {
    bool anyone_window_visible = true;

    while (anyone_window_visible) {
        engine_state().updateDeltaTime();

        auto wnds = std::vector(window_manager.begin(), window_manager.end());

        anyone_window_visible = false;

        for (auto wnd : wnds)
            anyone_window_visible |= wnd.first->is_visible();


        if (anyone_window_visible) {
            // Event update
            for (auto wnd : wnds)
                if (wnd.first->is_visible())
                    wnd.first->eventUpdate();

            // Do other stuff
            if (physic_simulation) {
                physic_simulation->step();
            }

            // Renderer
            for (auto wnd : wnds)
                if (wnd.first->is_visible())
                    wnd.first->render();
        }


        // Remove closed windows
        for (auto wnd : wnds) {
            if (!wnd.first->is_visible() && wnd.second.destroy_on_close) {
                wnd.first->close();
                window_manager.remove(wnd.first);
            }
        }
    }
}