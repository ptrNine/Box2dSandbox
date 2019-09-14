#include <iostream>
#include <SFML/Window.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <thread>

#include "src/graphics/MainWindow.hpp"
#include "src/graphics/nuklear.hpp"
#include "src/graphics/ObjectManager.hpp"

void two(const DrawableManagerSP& drawable_manager) {
    auto wnd2 = MainWindow();
    wnd2.attachManager(drawable_manager);
    wnd2.run();
}

int main() {
    //auto wnd = sf::Window(sf::VideoMode(800, 400), "Kek", sf::Style::Close | sf::Style::Resize);

    //sf::Clock clock;

    //auto last_timestamp = clock.getElapsedTime();

    auto wnd = MainWindow();

    wnd.addUiCallback("Main", [](NkCtx* ctx) {
        if (nk_begin(ctx, "Test", nk_rect(20, 20, 200, 300),
                     NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|
                     NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE)) {

        }
        nk_end(ctx);
    });

    auto drawable_manager = std::make_shared<DrawableManager>();
    drawable_manager->create<sf::CircleShape>(50.f);

    wnd.attachManager(drawable_manager);

    //std::thread thr(two, drawable_manager).;
    //thr.join();

    wnd.run();

    /*

    while(true) {
        auto timestamp = clock.getElapsedTime();
        auto delta = (timestamp - last_timestamp).asSeconds();
        last_timestamp = timestamp;

        std::cout << "delta = " << delta << std::endl;
    }
     */

    return 0;
}