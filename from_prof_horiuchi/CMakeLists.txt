cmake_minimum_required(VERSION 3.10)

set(CMAKE_C_STANDARD 11)

set(CMAKE_CXX_STANDARD 17)

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

include_directories(/usr/include)

add_executable(dvs128_MultiCluster.exe dvs128_MultiCluster.cpp)

target_link_libraries(dvs128_MultiCluster.exe PRIVATE ${DV_LIBRARIES})

add_executable(dvs128_On_Track.exe dvs128_On_track.cpp)
target_link_libraries(dvs128_On_Track.exe PRIVATE ${DV_LIBRARIES})

add_executable(dvs128_OnOff_ts1.exe dvs128_OnOff_ts1.cpp)
target_link_libraries(dvs128_OnOff_ts1.exe PRIVATE ${DV_LIBRARIES})

add_executable(dvs128_ts1.exe dvs128_ts1.cpp)
target_link_libraries(dvs128_ts1.exe PRIVATE ${DV_LIBRARIES})
