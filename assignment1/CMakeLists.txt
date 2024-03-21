cmake_minimum_required(VERSION 3.22)
# I have to insert this line to make it compile on Mac.
set(CMAKE_CXX_STANDARD 11)

project(assignment-1-mesh)

add_subdirectory(assets/meshes)

add_executable(mesh_main src/main.cpp src/mesh.cpp src/mesh.h)
target_include_directories(mesh_main PRIVATE external/glm external/tiny_obj_loader)

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/results)
