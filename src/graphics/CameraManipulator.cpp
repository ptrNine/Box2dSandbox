#include "CameraManipulator.hpp"

#include "Camera.hpp"

Camera& CameraManipulator::camera() {
    return *_camera;
}