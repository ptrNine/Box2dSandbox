#pragma once

#include <memory>
#include <vector>
#include <functional>

#include <flat_hash_map.hpp>
#include <SFML/Graphics/Drawable.hpp>
#include <scl/scl.hpp>

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

class Camera;

using WindowP   = std::unique_ptr<sf::RenderWindow>;
using SettingsP = std::unique_ptr<sf::ContextSettings>;
using CameraSP  = std::shared_ptr<Camera>;

class Window {
public:
    DECLARE_SELF_FABRICS(Window);
    using UiCallbackT     = std::function<void(Window&, NkCtx*)>;
    using EventCallbackT  = std::function<void(Window&, const sf::Event&)>;
    using RenderCallbackT = std::function<void(Window&)>;

public:
    Window();
    ~Window();

    void run();
    void eventUpdate();
    void render();
    void update(float delta_time);

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

    void addRenderCallback(const std::string& name, const RenderCallbackT& callback) {
        _render_callbacks.emplace(name, callback);
    }

    void removeRenderCallback(const std::string& name) {
        _render_callbacks.erase(name);
    }

    void attachDrawableManager(const DrawableManagerSP &drawable_manager) {
        _drawable_manager = drawable_manager;
    }

    auto detachDrawableManager() {
        DrawableManagerSP res = _drawable_manager;

        _drawable_manager = nullptr;

        return res;
    }

    void addCamera(const CameraSP& camera) {
        _cameras.emplace(camera);
    }

    void removeCamera(const CameraSP& camera) {
        _cameras.erase(camera);
    }

    auto getMouseCoords() const -> scl::Vector2f;
    auto getMouseCoords(const Camera& camera) const -> scl::Vector2f;

    bool isOpen    ();
    void close     ();
    bool is_visible() { return _is_visible; }
    void is_visible(bool value);

    void size(uint32_t x, uint32_t y);

    void size(const scl::Vector2u32& value) {
        size(value.x(), value.y());
    }

    scl::Vector2u size() const;
    scl::Vector2f sizef() const;

private:

    NkCtx*       _ctx;
    NkSfml*      _nksfml;
    NkFontAtlas* _font_atlas;
    WindowP      _wnd;
    SettingsP    _settings;
    bool         _is_visible = true;

    ska::flat_hash_map<std::string, UiCallbackT>     _ui_callbacks;
    ska::flat_hash_map<std::string, EventCallbackT>  _event_callbacks;
    ska::flat_hash_map<std::string, RenderCallbackT> _render_callbacks;

    DrawableManagerSP _drawable_manager;
    ska::flat_hash_set<CameraSP> _cameras;
};
