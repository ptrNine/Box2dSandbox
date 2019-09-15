#pragma once

#include <memory>
#include <vector>
#include <functional>

#include <flat_hash_map.hpp>
#include <SFML/Graphics/Drawable.hpp>

#include "DrawableManager.hpp"
#include "../core/helper_macros.hpp"

using NkCtx       = struct nk_context;
using NkSfml      = struct nk_sfml;
using NkFontAtlas = struct nk_font_atlas;

namespace sf {
    class RenderWindow;
    class ContextSettings;
    class Event;
    class RenderTarget;
}

using WindowP           = std::unique_ptr<sf::RenderWindow>;
using SettingsP         = std::unique_ptr<sf::ContextSettings>;
using DrawableManagerSP = std::shared_ptr<DrawableManager>;

class Window {
public:
    using UiCallbackT    = std::function<void(NkCtx*)>;
    using EventCallbackT = std::function<void(const sf::Event&)>;

public:
    Window();
    ~Window();

    void run();
    void eventUpdate();
    void render();

    void addUiCallback(const std::string& name, const UiCallbackT& callback) {
        _ui_callbacks.emplace(name, callback);
    }

    void removeUiCallback(const std::string& name) {
        _ui_callbacks.erase(name);
    }

    void addEventCallback(const std::string& name, const EventCallbackT& callback) {
        _event_callbacks.emplace(name, callback);
    }

    void removeEventCallback(const std::string& name) {
        _ui_callbacks.erase(name);
    }

    void attachDrawableManager(const DrawableManagerSP &drawable_manager) {
        _drawable_manager = drawable_manager;
    }

    auto detachDrawableManager() {
        DrawableManagerSP res = _drawable_manager;

        _drawable_manager = nullptr;

        return res;
    }

    auto drawable_manager() -> DrawableManagerSP {
        return _drawable_manager;
    }

    bool isOpen    ();
    void close     ();
    bool is_visible() { return _is_visible; }
    void is_visible(bool value);


private:
    NkCtx*       _ctx;
    NkSfml*      _nksfml;
    NkFontAtlas* _font_atlas;
    WindowP      _wnd;
    SettingsP    _settings;
    bool         _is_visible = true;

    ska::flat_hash_map<std::string, UiCallbackT>    _ui_callbacks;
    ska::flat_hash_map<std::string, EventCallbackT> _event_callbacks;
    DrawableManagerSP _drawable_manager;
};
