cmake_minimum_required(VERSION 3.13)
project(platformer)

set(CMAKE_CXX_STANDARD 17)

# CMake modules
include(CMake/git_submodule_build.cmake)
include(CMake/git_submodule_update.cmake)

# SFML
set(CMAKE_PREFIX_PATH "3p/usr/local/")
set(SFML_DIR "3p/usr/local/lib/cmake/SFML")
find_package(SFML 2.5 REQUIRED COMPONENTS system window graphics network audio)
set(SFML_LIBS sfml-system sfml-window sfml-graphics sfml-network sfml-audio)

# Update and build submodules
git_submodule_update()
git_submodule_build(SimpleConfigManager)
git_submodule_build(SimpleContainersLibrary INSTALL_SUB_DEPS)
git_submodule_build(gainput)
git_submodule_build(cpp-httplib)

set(CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR}/fakeroot)

# SCM
find_package(SCM REQUIRED)

# SCL
find_package(SCL REQUIRED)

# fmt
find_package(fmt REQUIRED)

#utf8cpp
find_package(utf8cpp REQUIRED)

include_directories(${CMAKE_BINARY_DIR}/fakeroot/include)

include_directories(3p/usr/local/include)
link_directories(3p/usr/local/lib)

set(${PROJECT_NAME}_sources
        src/graphics/nuklear.cpp
        src/graphics/Window.cpp
        src/graphics/HUD.cpp
        src/graphics/FontManager.cpp
        src/Engine.cpp
        src/graphics/Camera.cpp
        src/graphics/CameraManipulator.cpp
        src/game/PhysicSimulation.cpp
        src/game/PhysicHumanBody.cpp
        src/game/KeyCombo.cpp
        src/core/time.cpp
        src/ui_callbacks.cpp
        )

set(_libraries ${SFML_LIBS} GL GLEW Box2D -pthread ${SCM_STATIC_LIBRARIES} fmt)

add_executable(platformer main.cpp ${${PROJECT_NAME}_sources})
add_executable(physic_body_constructor physic_body_constructor.cpp ${${PROJECT_NAME}_sources})
add_executable(mnist_test mnist_test.cpp)
#add_executable(walk_neuro_evolution walk_neuro_evolution.cpp ${${PROJECT_NAME}_sources})
#add_executable(stand_neuroevolution stand_neuroevolution.cpp ${${PROJECT_NAME}_sources})


target_link_libraries(platformer ${_libraries})
target_link_libraries(physic_body_constructor ${_libraries})
target_link_libraries(mnist_test -pthread ${SCM_STATIC_LIBRARIES} fmt)
#target_link_libraries(walk_neuro_evolution ${_libraries})
#target_link_libraries(stand_neuroevolution ${_libraries})
