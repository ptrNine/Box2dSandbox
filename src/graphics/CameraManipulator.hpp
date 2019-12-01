#pragma once

#include <functional>
#include <memory>
#include "../core/helper_macros.hpp"

namespace sf {
    class Event;
}

#define CAMERA_MANIPULATOR_EVENT_CALLBACK(MANIPULATOR, CAMERA, EVENT, WINDOW) \
(CameraManipulator& MANIPULATOR, Camera& CAMERA, const sf::Event& EVENT, const class Window& WINDOW)

class CameraManipulator {
    friend class Camera;

    using EventCallback   = std::function<void(class CameraManipulator&, class Camera&, const sf::Event&, const class Window&)>;
    using RegularCallback = std::function<void(class CameraManipulator&, class Camera&, float delta_time)>;

public:
    DECLARE_SELF_FABRICS(CameraManipulator);

    void attachEventCallback(const EventCallback& callback) {
        _event_callback = callback;
    }

    auto detachEventCallback() {
        auto tmp = _event_callback;
        _event_callback = nullptr;
        return tmp;
    }

    auto& event_callback() const {
        return _event_callback;
    }

    void attachRegularCallback(const RegularCallback& callback) {
        _regular_callback = callback;
    }

    auto detachRegularCallback() {
        auto tmp = _regular_callback;
        _regular_callback = nullptr;
        return tmp;
    }

    auto& regular_callback() const {
        return _regular_callback;
    }

private:
    void updateEvents(class Camera& camera, const sf::Event& evt, const class Window& wnd) {
        if (_event_callback)
            _event_callback(*this, camera, evt, wnd);
    }

    void updateRegular(class Camera& camera, float delta_time) {
        if (_regular_callback)
            _regular_callback(*this, camera, delta_time);
    }

private:
    EventCallback   _event_callback;
    RegularCallback _regular_callback;

private:
    float _max_speed;
    float _current_speed;
};
