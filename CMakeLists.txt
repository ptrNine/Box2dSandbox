cmake_minimum_required(VERSION 3.13)
project(platformer)

set(CMAKE_CXX_STANDARD 17)

set(SFML_DIR "3p/usr/local/lib/cmake/SFML")
find_package(SFML 2.5 REQUIRED COMPONENTS system window graphics network audio)
set(SFML_LIBS sfml-system sfml-window sfml-graphics sfml-network sfml-audio)

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
        src/gameMain.cpp
        )

add_executable(platformer main.cpp ${${PROJECT_NAME}_sources})

target_link_libraries(platformer ${SFML_LIBS} GL GLEW Box2D -pthread)