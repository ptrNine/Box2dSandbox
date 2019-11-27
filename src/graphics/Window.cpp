#include "Window.hpp"

#include <iostream>

#include <SFML/Graphics/RenderWindow.hpp>

#include "nuklear.hpp"

#include "Camera.hpp"
#include "HUD.hpp"
#include "../Engine.hpp"

#define MAX_VERTEX_BUFFER  512 * 1024
#define MAX_ELEMENT_BUFFER 128 * 1024


Window::Window() {
    _settings = std::make_unique<sf::ContextSettings>(24, 8, 4);
    _wnd      = std::make_unique<sf::RenderWindow>(sf::VideoMode(1600, 900), "Test", sf::Style::Default, *_settings);

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

        if (!_wnd->isOpen())
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

        nk_sfml_handle_event(_nksfml, &evt);

        if ((evt.type == sf::Event::MouseButtonPressed || evt.type == sf::Event::MouseButtonReleased) && nk_item_is_any_active(_ctx))
            continue;


        auto _camera_safe_copy = std::vector(_cameras.begin(), _cameras.end());
        // Update cameras callbacks
        for (auto& camera : _camera_safe_copy)
            camera->updateEvents(*this, evt);

        // Call all event callbacks
        for (auto &cb : _event_callbacks)
            cb.second(*this, evt);
    }

    nk_input_end(_ctx);
}

void Window::update(float delta_time) {
    {
        auto safe_copy = std::vector(_cameras.begin(), _cameras.end());

        for (auto& camera : safe_copy)
            camera->updateRegular(delta_time);
    }
}

void Window::render() {
    // Call all ui callbacks
    {
        auto safe_copy = std::vector(_ui_callbacks.begin(), _ui_callbacks.end());
        for (auto& cb : safe_copy)
            cb.second(*this, _ctx);
    }

    _wnd->clear(sf::Color(25, 25, 50));

    auto default_view = _wnd->getView();

    for (auto& camera : _cameras) {
        _wnd->setView(camera->_view);

        if (camera->_drawable_manager) {
            for (auto drawable : *camera->_drawable_manager) {
                _wnd->draw(*drawable);
            }
        }

        if (camera->hud()) {
            _wnd->setView(default_view);
            for (auto drawable : *camera->hud()->_drawable_manager)
                _wnd->draw(*drawable);
        }
    }

    if (_drawable_manager) {
        _wnd->setView(default_view);

        for (auto drawable : *_drawable_manager)
            _wnd->draw(*drawable);
    }

    {
        auto safe_copy = std::vector(_render_callbacks.begin(), _render_callbacks.end());
        for (auto& cb : safe_copy)
            cb.second(*this);
    }

    _wnd->setView(default_view);

    nk_sfml_render(_nksfml, NK_ANTI_ALIASING_OFF, MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);
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

auto Window::getMouseCoords() const -> scl::Vector2f {
    auto pos = _wnd->mapPixelToCoords(sf::Mouse::getPosition(*_wnd));
    return {pos.x, -pos.y};
}

auto Window::getMouseCoords(const Camera& camera) const -> scl::Vector2f {
    auto default_view = _wnd->getView();
    _wnd->setView(camera._view);

    auto res = getMouseCoords();
    _wnd->setView(default_view);

    return res;
}

void Window::size(uint32_t x, uint32_t y) {
    _wnd->setSize(sf::Vector2u(x, y));
}

scl::Vector2u Window::size() const {
    auto sz = _wnd->getSize();

    return {sz.x, sz.y};
}

scl::Vector2f Window::sizef() const {
    auto sz = _wnd->getSize();

    return {float(sz.x), float(sz.y)};
}