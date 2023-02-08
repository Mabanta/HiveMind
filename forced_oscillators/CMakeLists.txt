cmake_minimum_required(VERSION 3.10)

project(LiveObjectDetection LANGUAGES C CXX)

find_package(dv 1.3.0 REQUIRED)
set(DV_LIBRARIES dv::dvsdk)

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

#add_executable(cpp_object_detection cpp_object_detection.cpp)
add_executable(file_forced_osc file_forced_osc.cpp)
add_executable(forced_osc_record cpp_forced_osc.cpp)
#add_executable(object_record cpp_object_detection_record.cpp)
#add_executable(cluster_visualize cluster_visualize.cpp)

#target_link_libraries(cpp_object_detection PRIVATE ${DV_LIBRARIES})
#target_link_libraries(cpp_object_detection PRIVATE cluster)

target_link_libraries(file_forced_osc PRIVATE ${DV_LIBRARIES})
target_link_libraries(file_forced_osc PRIVATE cluster)

target_link_libraries(forced_osc_record PRIVATE ${DV_LIBRARIES})
target_link_libraries(forced_osc_record PRIVATE cluster)

#target_link_libraries(object_record PRIVATE ${DV_LIBRARIES})
#target_link_libraries(object_record PRIVATE cluster)

#target_link_libraries(cluster_visualize PRIVATE ${DV_LIBRARIES})
#target_link_libraries(cluster_visualize PRIVATE cluster)
