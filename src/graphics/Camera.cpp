#include "Camera.hpp"

#include <SFML/Graphics/View.hpp>
#include "CameraManipulator.hpp"

Camera::Camera(const std::string_view& name, float aspect_ratio, float width):
   _name(name), _aspect_ratio(aspect_ratio), _eye_width(width)
{
    _view.setSize(_eye_width, _eye_width / _aspect_ratio);
    _view.setCenter(0.f, 0.f);
}

Camera::~Camera() {
    std::cout << "Destroy [" << _name << "]" << std::endl;
}

void Camera::updateRegular(float delta_time)  {
    if (_manipulator)
        _manipulator->updateRegular(*this, delta_time);
}

void Camera::updateEvents(const Window &wnd, const sf::Event &evt)  {
    if (_manipulator)
        _manipulator->updateEvents(*this, evt, wnd);
}