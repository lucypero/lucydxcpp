cmake_minimum_required(VERSION 3.23)

project (lucydxcpp)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_VERBOSE_MAKEFILE ON CACHE BOOL "ON" FORCE)

add_compile_definitions("DEBUG")

add_executable (${PROJECT_NAME} WIN32 dxbook.cpp)