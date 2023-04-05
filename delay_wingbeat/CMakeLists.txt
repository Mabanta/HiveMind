cmake_minimum_required(VERSION 3.10)

project(fileRead LANGUAGES C CXX)

find_package(dv 1.5.0 REQUIRED)
set(DV_LIBRARIES dv::sdk)

find_package(libcaer REQUIRED)
set(DV_LIBRARIES ${DV_LIBRARIES} libcaer::caer)

find_package(fmt 7.0.3 REQUIRED)
set(DV_LIBRARIES ${DV_LIBRARIES} fmt::fmt)

set(BOOST_ROOT /opt/inivation/boost/)
find_package(Boost 1.73 REQUIRED COMPONENTS filesystem)
set(DV_LIBRARIES ${DV_LIBRARIES} Boost::boost Boost::filesystem)

find_package(OpenCV)
set(DV_LIBRARIES ${DV_LIBRARIES} ${OpenCV_LIBS})

include_directories(/usr/include, /opt/inivation)
link_directories(./cluster/build)

add_executable(file_naive_delay_detection file_naive_delay_detection.cpp)

target_link_libraries(file_naive_delay_detection PRIVATE ${DV_LIBRARIES})
target_link_libraries(file_naive_delay_detection PRIVATE cluster)

add_executable(file_restricted_delay_detection file_restricted_delay_detection.cpp)

target_link_libraries(file_restricted_delay_detection PRIVATE ${DV_LIBRARIES})
target_link_libraries(file_restricted_delay_detection PRIVATE cluster_v2)

add_executable(file_center_delay_detection file_center_delay_detection.cpp)

target_link_libraries(file_center_delay_detection PRIVATE ${DV_LIBRARIES})
target_link_libraries(file_center_delay_detection PRIVATE cluster_v3)

add_executable(file_indiv_delay_detection file_indiv_delay_detection.cpp)

target_link_libraries(file_indiv_delay_detection PRIVATE ${DV_LIBRARIES})
target_link_libraries(file_indiv_delay_detection PRIVATE cluster_v4)

add_executable(file_heatmap_record file_heatmap_record.cpp)

target_link_libraries(file_heatmap_record PRIVATE ${DV_LIBRARIES})
target_link_libraries(file_heatmap_record PRIVATE heatmap_cluster)