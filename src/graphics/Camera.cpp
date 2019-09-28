#include "Camera.hpp"

#include <SFML/Graphics/View.hpp>
#include "CameraManipulator.hpp"

void Camera::updateRegular(float delta_time)  {
    if (_manipulator)
        _manipulator->updateRegular(*this, delta_time);
}

void Camera::updateEvents(const Window &wnd, const sf::Event &evt)  {
    if (_manipulator)
        _manipulator->updateEvents(*this, evt);
}