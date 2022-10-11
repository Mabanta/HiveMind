cmake_minimum_required(VERSION 3.10)

project(cluster LANGUAGES C CXX)

find_package(OpenCV)

#add_definitions(${GCC_COMPILE_FLAGS} "-lfftw3 -lm")
#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_COMPILE_FLAGS}")

find_package(PkgConfig REQUIRED)
pkg_search_module(FFTW REQUIRED fftw3 IMPORTED_TARGET)
include_directories(PkgConfig::FFTW)
link_libraries     (PkgConfig::FFTW)

include_directories(/usr/include)

add_library(cluster SHARED cluster.cpp)

#add_executable(fft_test fft_test_2.cpp)
#target_link_libraries(fft_test PRIVATE PkgConfig::FFTW ${FFTW_DOUBLE_THREADS_LIB})

target_link_libraries(cluster PRIVATE ${OpenCV_LIBS} PkgConfig::FFTW ${FFTW_DOUBLE_THREADS_LIB})
