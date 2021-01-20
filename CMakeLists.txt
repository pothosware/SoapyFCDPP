######################################################################## 
# Build Soapy SDR support module for FCDPP
######################################################################## 
cmake_minimum_required(VERSION 2.8.7)
project(SoapyFCDPP CXX C)

find_package(SoapySDR "0.6.0" NO_MODULE)
if (NOT SoapySDR_FOUND)
    message(FATAL_ERROR "Soapy SDR development files not found...") 
endif ()

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

    #disable warnings for unused parameters
    add_definitions(-Wno-unused-parameter)

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
