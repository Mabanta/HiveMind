cmake_minimum_required(VERSION 3.10)

project(cluster LANGUAGES C CXX)

find_package(OpenCV)

include_directories(/usr/include)

add_library(cluster SHARED cluster.cpp)

target_link_libraries(cluster PRIVATE ${OpenCV_LIBS})