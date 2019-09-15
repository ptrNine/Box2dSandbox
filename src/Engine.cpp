#include "Engine.hpp"

#include <SFML/Graphics/CircleShape.hpp>
#include "graphics/nuklear.hpp"

Engine::Engine() : window_manager("WindowManager") {

}

int Engine::run(int argc, char* argv[]) {

    onCreate();
    mainUpdate();

    return 0;
}

void Engine::onCreate() {
    auto wnd = window_manager.create<Window>();

    wnd->addUiCallback("Main", [this](NkCtx* ctx) {
        if (nk_begin(ctx, "Test", nk_rect(20, 20, 200, 300),
                     NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|
                     NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE)) {
        }
        nk_end(ctx);
    });

    auto drawable_manager = std::make_shared<DrawableManager>("Drawable manager");
    auto circle = drawable_manager->create<sf::CircleShape>(50.f);

    wnd->addEventCallback("Events", [this, circle](const sf::Event& evt) {
        float speed = 6.0f;
        if (evt.type == sf::Event::KeyPressed) {
            if (evt.key.code == sf::Keyboard::A)
                circle->move(-speed, 0.f);
            else if (evt.key.code == sf::Keyboard::D)
                circle->move(speed, 0.f);
            else if (evt.key.code == sf::Keyboard::W)
                circle->move(0.f, -speed);
            else if (evt.key.code == sf::Keyboard::S)
                circle->move(0.f, speed);
        }
    });

    wnd->addUiCallback("kek", [this](NkCtx* ctx) {
        if (nk_begin(ctx, "KEEKEKE", nk_rect(230, 20, 200, 300),
                     NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|
                     NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE)) {
            nk_layout_row_dynamic(ctx, 25, 1);

            if (nk_button_label(ctx, "Create wnd")) {
            }
        }
        nk_end(ctx);
    });

    wnd->attachDrawableManager(drawable_manager);
}

void Engine::mainUpdate() {
    bool anyone_window_visible = true;

    while (anyone_window_visible) {
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