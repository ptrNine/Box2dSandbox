#include "MainWindow.hpp"

#include <SFML/Graphics/RenderWindow.hpp>
#include <iostream>
#include "nuklear.hpp"

#define MAX_VERTEX_BUFFER 512 * 1024
#define MAX_ELEMENT_BUFFER 128 * 1024

MainWindow::MainWindow() {
    _settings = std::make_unique<sf::ContextSettings>(24, 8, 4);
    _wnd      = std::make_unique<sf::RenderWindow>(sf::VideoMode(800, 600), "Test", sf::Style::Default, *_settings);

    _wnd->setVerticalSyncEnabled(true);
    _wnd->setActive(true);

    auto rc = glewInit();
    if(rc) {
        std::cerr << glewGetErrorString(rc) << std::endl;
        // Todo: assert
    }

    glViewport(0, 0, _wnd->getSize().x, _wnd->getSize().y);

    _ctx = nk_sfml_init(_wnd.get());
    nk_sfml_font_stash_begin(&_font_atlas);
    nk_sfml_font_stash_end();
}


MainWindow::~MainWindow() {
    nk_sfml_shutdown();
}


void MainWindow::run() {

    while (_wnd->isOpen()) {

        //////////////////// Event update ////////////////////////
        auto evt = sf::Event();

        nk_input_begin(_ctx);

        while (_wnd->pollEvent(evt)) {
            if (evt.type == sf::Event::Closed)
                _wnd->close();
            else if (evt.type == sf::Event::Resized)
                glViewport(0, 0, evt.size.width, evt.size.height);


            // Call all event callbacks
            for (auto& cb : _event_callbacks)
                cb.second(evt);


            nk_sfml_handle_event(&evt);
        }

        nk_input_end(_ctx);

        // Quit if window closed
        if (!_wnd->isOpen())
            break;


        /////////////////////// UI ///////////////////////

        // Call all ui callbacks
        for (auto& cb : _ui_callbacks)
            cb.second(_ctx);



        //////////////////////// Renderer //////////////////////////

        _wnd->setActive(true);

        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(0.1f, 0.18f, 0.24f, 1.f);

        for (auto& drawable : *_drawable_manager)
            _wnd->draw(*drawable);

        nk_sfml_render(NK_ANTI_ALIASING_ON, MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);
        _wnd->display();
    }
}
