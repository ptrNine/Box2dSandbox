macro(prepare_box2d_build)
    configure_file(CMake/Box2DCMakeLists.txt.in ${PROJECT_SOURCE_DIR}/remote/Box2D/Box2D/CMakeLists.txt COPYONLY)
endmacro()