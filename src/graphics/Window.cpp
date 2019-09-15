#include "Window.hpp"

#include <iostream>

#include <SFML/Graphics/RenderWindow.hpp>

#include "nuklear.hpp"

#include "../Engine.hpp"

#define MAX_VERTEX_BUFFER  512 * 1024
#define MAX_ELEMENT_BUFFER 128 * 1024


Window::Window() {
    _settings = std::make_unique<sf::ContextSettings>(24, 8, 4);
    _wnd      = std::make_unique<sf::RenderWindow>(sf::VideoMode(800, 600), "Test", sf::Style::Default, *_settings);

    _wnd->setVerticalSyncEnabled(true);
    _wnd->setActive(true);

    auto rc = glewInit();
    if (rc) {
        std::cerr << glewGetErrorString(rc) << std::endl;
        // Todo: assert
    }

    glViewport(0, 0, _wnd->getSize().x, _wnd->getSize().y);


    auto info = nk_sfml_init(_wnd.get());

    _ctx    = std::get<0>(info);
    _nksfml = std::get<1>(info);

    nk_sfml_font_stash_begin(_nksfml, &_font_atlas);
    nk_sfml_font_stash_end(_nksfml);
}


Window::~Window() {
    nk_sfml_shutdown(_nksfml);
}


void Window::run() {
    while (_wnd->isOpen()) {
        eventUpdate();

        if (_wnd->isOpen())
            break;

        render();
    }
}

void Window::eventUpdate() {
    auto evt = sf::Event();

    nk_input_begin(_ctx);

    while (_wnd->pollEvent(evt)) {
        if (evt.type == sf::Event::Closed)
            is_visible(false);
        else if (evt.type == sf::Event::Resized)
            glViewport(0, 0, evt.size.width, evt.size.height);


        // Call all event callbacks
        for (auto &cb : _event_callbacks)
            cb.second(evt);


        nk_sfml_handle_event(_nksfml, &evt);
    }

    nk_input_end(_ctx);
}


void Window::render() {
    // Call all ui callbacks
    for (auto& cb : _ui_callbacks)
        cb.second(_ctx);

    _wnd->clear(sf::Color(25, 25, 25));

    if (_drawable_manager) {
        for (auto drawable : *_drawable_manager)
            _wnd->draw(*drawable);
    }

    nk_sfml_render(_nksfml, NK_ANTI_ALIASING_ON, MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);
    _wnd->display();
}


bool Window::isOpen() {
    return _wnd->isOpen();
}

void Window::close() {
    _wnd->close();
}

void Window::is_visible(bool value) {
    _wnd->setVisible(value);
    _is_visible = value;
}