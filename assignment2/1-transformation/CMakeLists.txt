add_library(transformation_lib lib/transformation.h lib/transformation.cpp)
target_include_directories(transformation_lib PRIVATE ${GRASSLAND_INCLUDE_DIRS})

add_executable(transformation_gui gui/main.cpp gui/application.h gui/application.cpp)
target_include_directories(transformation_gui PRIVATE ${GRASSLAND_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/lib)

target_link_libraries(transformation_gui PRIVATE transformation_lib grassland)


add_executable(transformation_test test/main.cpp)
target_include_directories(transformation_test PRIVATE ${GRASSLAND_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/lib)
target_link_libraries(transformation_test PRIVATE transformation_lib grassland gtest gtest_main)

target_compile_definitions(transformation_test PRIVATE TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/test/")
