cmake_minimum_required(VERSION 3.16)
project(SpiltView)

set(CMAKE_CXX_STANDARD 17)
add_compile_options("-g")

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

include_directories(${GTK3_INCLUDE_DIRS})

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} X11 Xtst ${GTK3_LIBRARIES} pthread)