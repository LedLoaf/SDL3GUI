cmake_minimum_required(VERSION 3.15)
project(SDL3Template LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL3 CONFIG REQUIRED)
find_package(SDL3_image CONFIG REQUIRED)
find_package(SDL3_ttf CONFIG REQUIRED)
find_package(SDL3_mixer CONFIG REQUIRED)

add_executable(SDL3Template src/main.cpp)
target_link_libraries(SDL3Template PRIVATE SDL3::SDL3 SDL3_image::SDL3_image SDL3_ttf::SDL3_ttf SDL3_mixer::SDL3_mixer)