cmake_minimum_required(VERSION 3.10)

# Sets CMAKE_INSTALL_PREFIX so the later functions know where the modules folder is
IF (UNIX AND NOT APPLE)
    set(CMAKE_INSTALL_PREFIX /usr)
ELSEIF(APPLE)
    set(CMAKE_INSTALL_PREFIX "/usr/local" CACHE STRING "CMake default install prefix, set to /usr/local on macOS")
ENDIF()

project(dvs128 LANGUAGES C CXX)

# May need to change based on install path of DV. This is for Linux
include(/usr/share/dv/dv-modules.cmake)

# USES CMAKE_INSTALL_PREFIX to find dv/modules for DV_MODULES
# Also creates DV_LIBRARIES to link needed libraries
DV_MODULE_SETUP()

# Adds files as shared libraries, which DV uses once installed
# Also need to link other DV libraries
add_library(dvs128_MultiCluster SHARED dvs128_MultiCluster.cpp)
target_link_libraries(dvs128_MultiCluster PRIVATE ${DV_LIBRARIES})

add_library(dvs128_On_Track SHARED dvs128_On_track.cpp)
target_link_libraries(dvs128_On_Track PRIVATE ${DV_LIBRARIES})

add_library(dvs128_OnOff_ts1 SHARED dvs128_OnOff_ts1.cpp)
target_link_libraries(dvs128_OnOff_ts1 PRIVATE ${DV_LIBRARIES})

add_library(dvs128_ts1 SHARED dvs128_ts1.cpp)
target_link_libraries(dvs128_ts1 PRIVATE ${DV_LIBRARIES})

# Files must be in dv/modules folder for DV to see it
install(TARGETS dvs128_MultiCluster dvs128_On_Track dvs128_OnOff_ts1 dvs128_ts1 DESTINATION ${DV_MODULES_DIR})