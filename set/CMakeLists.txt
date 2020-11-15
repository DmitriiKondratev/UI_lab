cmake_minimum_required(VERSION 3.14)

project(set LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories("${PROJECT_SOURCE_DIR}/include")

add_library(${PROJECT_NAME} SHARED "src/${PROJECT_NAME}_impl.cpp")

link_directories("${PROJECT_SOURCE_DIR}/libs")

target_link_libraries(${PROJECT_NAME} PUBLIC logger)
target_link_libraries(${PROJECT_NAME} PUBLIC vector)
