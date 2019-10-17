cmake_minimum_required(VERSION 3.13)
project(platformer)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_PREFIX_PATH "3p/usr/local/")
set(SFML_DIR "3p/usr/local/lib/cmake/SFML")
find_package(SFML 2.5 REQUIRED COMPONENTS system window graphics network audio)
set(SFML_LIBS sfml-system sfml-window sfml-graphics sfml-network sfml-audio)

find_package(SCM REQUIRED)
include(${SCM_USE_FILE})

find_package(fmt REQUIRED)

include_directories(3p/usr/local/include)
link_directories(3p/usr/local/lib)

set(${PROJECT_NAME}_sources
        src/graphics/nuklear.cpp
        src/graphics/Window.cpp
        src/Engine.cpp
        src/graphics/Camera.cpp
        src/graphics/CameraManipulator.cpp
        src/game/PhysicSimulation.cpp
        src/core/time.cpp
        src/ui_callbacks.cpp
        )

add_executable(platformer main.cpp ${${PROJECT_NAME}_sources})
add_executable(physic_body_constructor physic_body_constructor.cpp ${${PROJECT_NAME}_sources})


target_link_libraries(platformer ${SFML_LIBS} GL GLEW Box2D -pthread ${SCM_STATIC_LIBRARIES} fmt)
target_link_libraries(physic_body_constructor ${SFML_LIBS} GL GLEW Box2D -pthread ${SCM_STATIC_LIBRARIES} fmt)