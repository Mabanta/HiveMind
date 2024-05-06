cmake_minimum_required(VERSION 3.10)

project(fileRead LANGUAGES C CXX)

find_package(dv 1.5.0 REQUIRED)
set(DV_LIBRARIES dv)

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

add_executable(file_reading_test file_reading_test.cpp)
add_executable(dvxplorer_file_read dvxplorer_file_read.cpp)

target_link_libraries(file_reading_test PRIVATE ${DV_LIBRARIES})
target_link_libraries(dvxplorer_file_read PRIVATE ${DV_LIBRARIES})