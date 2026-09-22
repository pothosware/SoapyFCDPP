########################################################################
# Build Soapy SDR support module for FCDPP
########################################################################
cmake_minimum_required(VERSION 2.8.12...3.10)
project(SoapyFCDPP CXX C)

find_package(SoapySDR "0.6.0" NO_MODULE REQUIRED)

get_filename_component(_SoapySDR_ROOT "${SoapySDR_INCLUDE_DIRS}/.." ABSOLUTE)
message(STATUS "SoapySDR root directory: ${_SoapySDR_ROOT}")
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT AND NOT "${_SoapySDR_ROOT}" STREQUAL "/usr")
    set_property(CACHE CMAKE_INSTALL_PREFIX PROPERTY VALUE ${_SoapySDR_ROOT})
endif()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})

find_package(HIDAPI)
if (HIDAPI_FOUND)
    message(STATUS "HIDAPI_INCLUDE_DIR: ${HIDAPI_INCLUDE_DIR}")
    message(STATUS "HIDAPI_LIBRARY: ${HIDAPI_LIBRARY}")
else()
    message(FATAL_ERROR "HID API development files not found...")
endif ()

find_package(ALSA)
if (ALSA_FOUND)
    message(STATUS "ALSA_INCLUDE_DIRS: ${ALSA_INCLUDE_DIRS}")
    message(STATUS "ALSA_LIBRARIES: ${ALSA_LIBRARIES}")
else()
    message(FATAL_ERROR "ALSA development files not found...")
endif ()

if(CMAKE_COMPILER_IS_GNUCXX)

    #disable warnings for unused parameters+results
    add_definitions(-Wno-unused-parameter -Wno-unused-result)

endif(CMAKE_COMPILER_IS_GNUCXX)

include_directories(${HIDAPI_INCLUDE_DIR})

SOAPY_SDR_MODULE_UTIL(
    TARGET FCDPPSupport
    SOURCES
        SoapyFCDPP/SoapyFCDPP.cpp
        SoapyFCDPP/alsa.c
        SoapyFCDPP/fcd.c
    LIBRARIES ${HIDAPI_LIBRARY} ${ALSA_LIBRARIES}
)

add_executable(fcddirect fcddirect.cpp)
target_link_libraries(fcddirect SoapySDR)
