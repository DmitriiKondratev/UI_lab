cmake_minimum_required(VERSION 3.14)

project(logger LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories("${PROJECT_SOURCE_DIR}/include")

add_library(${PROJECT_NAME} SHARED src/logger_impl.cpp)
