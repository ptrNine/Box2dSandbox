#pragma once

#include <functional>
#include <SFML/Graphics/Transform.hpp>
#include <SFML/Graphics/View.hpp>
#include <scl/scl.hpp>

#include "DrawableManager.hpp"
#include "../core/helper_macros.hpp"

namespace sf {
    class Event;
}

class CameraManipulator;

class Camera {
public:
    friend class Window;

    using HUDSP               = std::shared_ptr<class HUD>;
    using DrawableManagerSP   = std::shared_ptr<DrawableManager>;
    using CameraManipulatorSP = std::shared_ptr<CameraManipulator>;
    using EventCallback       = std::function<void(Camera&, const sf::Event&, const class Window&)>;

public:
    DECLARE_SELF_FABRICS(Camera);

    Camera(const std::string_view& name = "", float aspect_ratio = 16.f/9.f, float width = 30.f);

    ~Camera();

    void attachDrawableManager(const DrawableManagerSP &drawable_manager) {
        _drawable_manager = drawable_manager;
    }

    auto detachDrawableManager() {
        DrawableManagerSP res = _drawable_manager;

        _drawable_manager = nullptr;

        return res;
    }

    auto drawable_manager() const {
        return _drawable_manager;
    }

    void attachHUD(const HUDSP& hud) {
        _hud = hud;
    }

    auto detachHUD() {
        HUDSP res = _hud;

        _hud = nullptr;

        return res;
    }

    auto hud() const {
        return _hud;
    }

    void attachCameraManipulator(const CameraManipulatorSP& camera_manipulator) {
        _manipulator = camera_manipulator;
    }

    auto detachCameraManipulator() {
        CameraManipulatorSP res = _manipulator;

        _manipulator = nullptr;

        return res;
    }

    auto camera_manipulator() {
        return _manipulator;
    }

    void move(float x, float y) {
        _view.move(x, -y);
    }

    void move(const scl::Vector2f& displ) {
        _view.move(displ.x(), displ.y());
    }

    void rotate(float angle) {
        _view.rotate(angle);
    }

    void setViewport(float left, float top, float width, float height) {
        _view.setViewport(sf::FloatRect(left, top, width, height));
    }

    void size(float width, float height) {
        _aspect_ratio = width / height;
        _eye_width = width;
        recalcSizeFromEyeAspect();
    }

    void size(const scl::Vector2f& isize) {
        size(isize.x(), isize.y());
    }

    auto size() const -> scl::Vector2f {
        return {_view.getSize().x, _view.getSize().y};
    }

    void aspect_ratio(float value) {
        _aspect_ratio = value;
        recalcSizeFromEyeAspect();
    }

    float aspect_ratio() const {
        return _aspect_ratio;
    }

    void eye_width(float meters) {
        _eye_width = meters;

        if (_eye_width < 1.f)
            _eye_width = 1.f;

        recalcSizeFromEyeAspect();
    }

    float eye_width() const {
        return _eye_width;
    }

    void position(float x, float y) {
        _view.setCenter(x, y);
    }

    void position(const scl::Vector2f& pos) {
        position(pos.x(), pos.y());
    }

    auto position() const -> scl::Vector2f {
        return {_view.getCenter().x, _view.getCenter().y};
    }

private:
    void recalcSizeFromEyeAspect() {
        _view.setSize(_eye_width, _eye_width / _aspect_ratio);
    }

    // Call from window
    void updateEvents (const class Window& wnd, const sf::Event& evt);
    void updateRegular(float delta_time);

private:
    sf::View          _view;
    DrawableManagerSP _drawable_manager;
    std::string       _name;
    float             _aspect_ratio = 16.f/9.f;
    float             _eye_width = 10;

    CameraManipulatorSP _manipulator;
    HUDSP               _hud;
};