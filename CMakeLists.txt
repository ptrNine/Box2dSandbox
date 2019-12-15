cmake_minimum_required(VERSION 3.13)
project(platformer)

set(CMAKE_CXX_STANDARD 17)

# CMake modules
include(CMake/git_submodule_build.cmake)
include(CMake/git_submodule_update.cmake)
include(CMake/prepare_box2d_build.cmake)

# SFML
#set(CMAKE_PREFIX_PATH "3p/usr/local/")
#set(SFML_DIR "3p/usr/local/lib/cmake/SFML")
#find_package(SFML 2.5 REQUIRED COMPONENTS system window graphics network audio)
#set(SFML_LIBS sfml-system sfml-window sfml-graphics sfml-network sfml-audio)

# Update and build submodules
git_submodule_update()
git_submodule_build(SFML)

prepare_box2d_build()
git_submodule_build(Box2D/Box2D)

git_submodule_build(SimpleConfigManager)
git_submodule_build(SimpleContainersLibrary INSTALL_SUB_DEPS)
git_submodule_build(gainput)
git_submodule_build(cpp-httplib)
git_submodule_build(zlib)
git_submodule_copy_files(nuklear NO_NAME_INCLUDE INCLUDES nuklear.h)
git_submodule_copy_files(flat_hash_map NO_NAME_INCLUDE INCLUDES flat_hash_map.hpp)
git_submodule_copy_files(libcuckoo NO_NAME_INCLUDE INCLUDES
        libcuckoo/cuckoohash_map.hh
        libcuckoo/cuckoohash_util.hh
        libcuckoo/cuckoohash_config.hh
        libcuckoo/bucket_container.hh)

set(CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR}/fakeroot)

# SFML
find_package(SFML 2.5 REQUIRED COMPONENTS system window graphics network audio)
set(SFML_LIBS sfml-system sfml-window sfml-graphics sfml-network sfml-audio)

# SCM
find_package(SCM REQUIRED)

# SCL
find_package(SCL REQUIRED)

# fmt
find_package(fmt REQUIRED)

#utf8cpp
find_package(utf8cpp REQUIRED)

include_directories(${CMAKE_BINARY_DIR}/fakeroot/include)
link_directories(${CMAKE_BINARY_DIR}/fakeroot/lib)

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
        src/game/RepeaterJointProcessor.cpp
        src/game/HolderJointProcessor.cpp
        src/game/MotionInterfaces.cpp
        src/utils/ReaderWriter.cpp
        src/core/time.cpp
        src/ui_callbacks.cpp
        )

set(_libraries ${SFML_LIBS} GL GLEW Box2D -pthread ${SCM_STATIC_LIBRARIES} fmt::fmt)

add_executable(platformer main.cpp ${${PROJECT_NAME}_sources})
add_executable(physic_body_constructor physic_body_constructor.cpp ${${PROJECT_NAME}_sources})
add_executable(mnist_test mnist_test.cpp src/machine_learning/MnistDataset.cpp src/utils/ReaderWriter.cpp)
#add_executable(walk_neuro_evolution walk_neuro_evolution.cpp ${${PROJECT_NAME}_sources})
#add_executable(stand_neuroevolution stand_neuroevolution.cpp ${${PROJECT_NAME}_sources})


target_link_libraries(platformer ${_libraries})
target_link_libraries(physic_body_constructor ${_libraries})
target_link_libraries(mnist_test ${_libraries} z)
#target_link_libraries(walk_neuro_evolution ${_libraries})
#target_link_libraries(stand_neuroevolution ${_libraries})
