file(GLOB SOURCES lib/*.h lib/*.cpp)
add_library(rasterization_lib ${SOURCES})
target_include_directories(rasterization_lib PRIVATE ${GRASSLAND_INCLUDE_DIRS})

add_executable(rasterization_gui gui/main.cpp gui/application.h gui/application.cpp)
target_include_directories(rasterization_gui PRIVATE ${GRASSLAND_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/lib)

target_link_libraries(rasterization_gui PRIVATE rasterization_lib grassland)


add_executable(rasterization_test test/main.cpp)
target_include_directories(rasterization_test PRIVATE ${GRASSLAND_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/lib)
target_link_libraries(rasterization_test PRIVATE rasterization_lib grassland gtest gtest_main)

target_compile_definitions(rasterization_test PRIVATE TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/test/")
add_subdirectory(gui)
