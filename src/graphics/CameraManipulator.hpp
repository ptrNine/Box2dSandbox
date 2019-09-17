#pragma once

#include <functional>
#include <memory>

namespace sf {
    class Event;
}

class Camera;

class CameraManipulator {
    friend Camera;

    using EventCallback   = std::function<void(class CameraManipulator&, const sf::Event&, const class Window&)>;
    using RegularCallback = std::function<void(class CameraManipulator&, float delta_time)>;

public:
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

    Camera& camera();

private:
    void updateEvents(const class Window& wnd, const sf::Event& evt) {
        if (_event_callback)
            _event_callback(*this, evt, wnd);
    }

    void updateRegular(float delta_time) {
        if (_regular_callback)
            _regular_callback(*this, delta_time);
    }

private:
    Camera*         _camera = nullptr;
    EventCallback   _event_callback;
    RegularCallback _regular_callback;

private:
    float _max_speed;
    float _current_speed;
};