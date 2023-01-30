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
link_directories(../cluster)

add_executable(cpp_object_detection_old.exe cpp_object_detection_old.cpp)
add_executable(cpp_object_detection_record_old.exe cpp_object_detection_record_old.cpp)
add_executable(file_object_detection_old.exe file_object_detection_old.cpp)

target_link_libraries(cpp_object_detection_old.exe PRIVATE ${DV_LIBRARIES})
target_link_libraries(cpp_object_detection_old.exe PRIVATE cluster)

target_link_libraries(cpp_object_detection_record_old.exe PRIVATE ${DV_LIBRARIES})
target_link_libraries(cpp_object_detection_record_old.exe PRIVATE cluster)

target_link_libraries(file_object_detection_old.exe PRIVATE ${DV_LIBRARIES})
target_link_libraries(file_object_detection_old.exe PRIVATE cluster)
