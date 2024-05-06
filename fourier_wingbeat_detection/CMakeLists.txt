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
link_directories(./cluster/build)

add_executable(wingbeat_file fourier_wingbeat_from_file.cpp)
add_executable(wingbeat_record fourier_wingbeat_record.cpp)
add_executable(wingbeat_visualize cluster_visualize.cpp)

target_link_libraries(wingbeat_file PRIVATE ${DV_LIBRARIES})
target_link_libraries(wingbeat_file PRIVATE cluster)

target_link_libraries(wingbeat_record PRIVATE ${DV_LIBRARIES})
target_link_libraries(wingbeat_record PRIVATE cluster)

target_link_libraries(wingbeat_visualize PRIVATE ${DV_LIBRARIES})
target_link_libraries(wingbeat_visualize PRIVATE cluster)
