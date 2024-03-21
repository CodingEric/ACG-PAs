cmake_minimum_required(VERSION 3.22)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/external/grassland/external/vcpkg/scripts/buildsystems/vcpkg.cmake)
set(CMAKE_CXX_STANDARD 17)

project(homework-2)

add_subdirectory(external/googletest)
add_subdirectory(external/grassland)

add_subdirectory(1-transformation)
add_subdirectory(2-rasterization)
add_subdirectory(3-raytracing)
