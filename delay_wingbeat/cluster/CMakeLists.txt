cmake_minimum_required(VERSION 3.10)

project(cluster LANGUAGES C CXX)

find_package(OpenCV)

find_package(dv 1.5.0 REQUIRED)
set(DV_LIBRARIES dv)

include_directories(/usr/include)

add_library(cluster SHARED cluster.cpp)
add_library(cluster_v2 SHARED cluster_v2.cpp)
add_library(cluster_v3 SHARED cluster_v3.cpp)

target_link_libraries(cluster PRIVATE ${OpenCV_LIBS} ${DV_LIBRARIES})
target_link_libraries(cluster_v2 PRIVATE ${OpenCV_LIBS} ${DV_LIBRARIES})
target_link_libraries(cluster_v3 PRIVATE ${OpenCV_LIBS} ${DV_LIBRARIES})