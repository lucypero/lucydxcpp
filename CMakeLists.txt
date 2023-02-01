cmake_minimum_required(VERSION 3.23)

project (lucydxcpp)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)

set(MY_DEBUG_OPTIONS /Zi /DDEBUG /MDd)
set(MY_RELEASE_OPTIONS /O2 /MD)


add_compile_options(
    /W3 /WX /EHsc /FC /nologo /Felucydxcpp
    "$<$<CONFIG:Debug>:${MY_DEBUG_OPTIONS}>"
    "$<$<CONFIG:Release>:${MY_RELEASE_OPTIONS}>"
)

set(MY_DEBUG_LINKER_OPTIONS Effects11d.lib imguid.obj DirectXTKd.lib)
set(MY_RELEASE_LINKER_OPTIONS Effects11.lib imgui.obj DirectXTK.lib)

add_link_options(
    /LIBPATH:..\\third_party\\lib
    "$<$<CONFIG:Debug>:${MY_DEBUG_LINKER_OPTIONS}>"
    "$<$<CONFIG:Release>:${MY_RELEASE_LINKER_OPTIONS}>"
)

include_directories(third_party/include)
include_directories(third_party/include/imgui)
include_directories(src/)

set(SOURCE
    src/main.cpp
    src/lucy_math.cpp
    src/obj_loader.cpp
    src/utils.cpp
    src/demo_box.cpp
    src/demo_hills.cpp
    src/demo_shapes.cpp
    src/demo_light.cpp
    src/exercises.cpp
)

add_executable (${PROJECT_NAME} WIN32 ${SOURCE})
set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")