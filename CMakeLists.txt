cmake_minimum_required(VERSION 3.16)
project(SpiltView)

set(CMAKE_CXX_STANDARD 17)
add_compile_options("-g")

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} X11 Xtst pthread)