cmake_minimum_required(VERSION 3.12)
project(pixmote VERSION 0.1.0 LANGUAGES CXX)

add_library(pixmote STATIC
    src/canvas.cpp
    src/font5x7.cpp
    src/theme.cpp
    src/ui.cpp
    src/widget.cpp
    src/widgets.cpp
)
target_include_directories(pixmote PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(pixmote PUBLIC cxx_std_20)
set_target_properties(pixmote PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(pixmote PRIVATE -Wall -Wextra -Wpedantic)
endif()

option(PIXMOTE_BUILD_TESTS "Build Pixmote tests" ON)
option(PIXMOTE_BUILD_EXAMPLES "Build Pixmote examples" ON)

if(PIXMOTE_BUILD_TESTS)
    enable_testing()
    foreach(test_name test_pixmote test_no_heap)
        add_executable(${test_name} tests/${test_name}.cpp)
        target_link_libraries(${test_name} PRIVATE pixmote)
        add_test(NAME ${test_name} COMMAND ${test_name})
    endforeach()
endif()

if(PIXMOTE_BUILD_EXAMPLES)
    add_executable(pixmote_demo examples/demo.cpp)
    target_link_libraries(pixmote_demo PRIVATE pixmote)
endif()
