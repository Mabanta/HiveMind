cmake_minimum_required(VERSION 3.10)

project(FileNoiseFilter LANGUAGES C CXX)

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

add_executable(dvxplorer_file_nf.exe dvxplorer_file_nf.cpp)

target_link_libraries(dvxplorer_file_nf.exe PRIVATE ${DV_LIBRARIES})