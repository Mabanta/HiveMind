cmake_minimum_required(VERSION 3.10)

project(LiveObjectDetection LANGUAGES C CXX)

find_package(dv 1.3.0 REQUIRED)
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
link_directories(../cluster/build)

add_executable(cpp_object_detection.exe cpp_object_detection.cpp)
add_executable(cpp_object_detection_record.exe cpp_object_detection_record.cpp)
add_executable(cpp_object_detection_record_v2.exe cpp_object_detection_record_v2.cpp)
add_executable(file_object_detection.exe file_object_detection.cpp)
add_executable(cluster_visualize.exe cluster_visualize.cpp)
ADD_LIBRARY(tracker_module SHARED tracking_module.cpp)

set_target_properties(tracker_module PROPERTIES PREFIX "user_")

target_link_libraries(cpp_object_detection.exe PRIVATE ${DV_LIBRARIES})
target_link_libraries(cpp_object_detection.exe PRIVATE cluster)

target_link_libraries(cpp_object_detection_record.exe PRIVATE ${DV_LIBRARIES})
target_link_libraries(cpp_object_detection_record.exe PRIVATE cluster)

target_link_libraries(cpp_object_detection_record_v2.exe PRIVATE ${DV_LIBRARIES})
target_link_libraries(cpp_object_detection_record_v2.exe PRIVATE cluster)

target_link_libraries(file_object_detection.exe PRIVATE ${DV_LIBRARIES})
target_link_libraries(file_object_detection.exe PRIVATE cluster)

target_link_libraries(cluster_visualize.exe PRIVATE ${DV_LIBRARIES})
target_link_libraries(cluster_visualize.exe PRIVATE cluster)

target_link_libraries(cpp_object_detection.exe PRIVATE ${DV_LIBRARIES})
target_link_libraries(cpp_object_detection.exe PRIVATE cluster)

target_link_libraries(tracker_module PRIVATE dv::sdk)
target_link_libraries(tracker_module PRIVATE cluster)
install(TARGETS tracker_module DESTINATION ${DV_MODULES_DIR})

