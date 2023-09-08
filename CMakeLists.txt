cmake_minimum_required(VERSION 3.25)

project(xcl-cpp LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

add_subdirectory(src)
add_executable(test)
target_sources(test
    PRIVATE
    test.cpp
)
target_link_libraries(test
    PRIVATE
    xcl-lib
)