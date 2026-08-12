set(CSP "umc55")                        # CSP to use
set(CSP_VERSION "development")          # CSP version
set(MEMORY "ROM")                       # select linker file (RAM/ROM) based target

# Needed for out-of-tree build
cmake_policy(SET CMP0076 NEW)

# Needed for duplicit build directories
cmake_policy(SET CMP0013 OLD)

set(DIR_SDK ${CMAKE_CURRENT_SOURCE_DIR}/../..) # set variable where your SDK is located
include(${DIR_SDK}/sdk.cmake)  # configure toolchain and build SDK

project(UMC55_DEMO)

cmake_minimum_required(VERSION 3.21)

###################################################################################################
# Building documentation
###################################################################################################

option(BUILD_DOCS "Build documentation" OFF)

if(BUILD_DOCS)
    find_package(Doxygen)
    if(Doxygen_FOUND)
        add_subdirectory(../../docs docs)
        message(STATUS "Doxygen found, building docs")
    else()
        message(STATUS "Doxygen not found, not building docs")
    endif()
endif()