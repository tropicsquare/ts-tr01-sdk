message(DEBUG "SDK directory: ${DIR_SDK}")

if(NOT DEFINED CSP)
    message(FATAL_ERROR "CSP undefined")
endif()

set(CMAKE_MESSAGE_LOG_LEVEL STATUS)
# set(CMAKE_VERBOSE_MAKEFILE OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_definitions(-DCSP=${CSP})

if(${CSP} STREQUAL "linux")
    # linux platform for testing purposes
    message(FATAL_ERROR "Linux platform not supported in this environment")

elseif(${CSP} STREQUAL "umc55")
    # default bare metal platform
    set(PLATFORM_DEF "PLATFORM_UMC55")  # for platform recognition in code

    # Use standard deployed headers
    # Derive local "technology specific" variable from CSP root
    if(NOT DEFINED CSP_VERSION)
        set(CSP_VERSION "development")
        message(WARNING "CSP version undefined, using \"${CSP_VERSION}\"")
    else()
        message(STATUS "selected CSP version \"${CSP_VERSION}\"")
    endif()

    set(DIR_CSP_ROOT ${DIR_SDK}/csp/${CSP}/${CSP_VERSION})
    SET(DIR_TS_CSP_INCLUDE_ROOT ${DIR_CSP_ROOT}/includes)

    include_directories (
        ${DIR_TS_CSP_INCLUDE_ROOT}/defs
        ${DIR_TS_CSP_INCLUDE_ROOT}/regs
    )

    # we need CSP version information in source codes
    if(${CSP_VERSION} STREQUAL "tr01-c")
        add_definitions(-DCSP_VERSION=2)
    else()
        add_definitions(-DCSP_VERSION=0)

        # enable support for running in simulated environment
        # simulation make sense only for "development" version
        if (DEFINED SIMULATION_BUILD)
            add_definitions(-DTS_SIMULATION_BUILD=${SIMULATION_BUILD})
        endif()
    endif()

else()

    message(FATAL_ERROR "unknown CSP \"${CSP}\"")

endif()

message(STATUS "selected \"${CSP}\" CSP")

set(SDK_TARGET  tassic_sdk)

set(DIR_CSP        ${DIR_SDK}/csp/${CSP})
set(DIR_SDK_COMMON ${DIR_SDK}/common)
set(DIR_SDK_HAL    ${DIR_SDK}/hal)
set(DIR_SDK_DRV    ${DIR_SDK}/drv)

set(CMAKE_TOOLCHAIN_FILE ${DIR_CSP}/toolchain.cmake)

# add definitions for human readable detecting CSP version in code
# like i.e #if (CSP_VERSION == CSP_VERSION_TR01C)
add_definitions(
    -D${PLATFORM_DEF}
    -DCSP_VERSION_DEVELOPMENT=0
    -DCSP_VERSION_MPW1=1   # first silicon release, not supported any more
    -DCSP_VERSION_TR01C=2  # current ACAB silicon
)

project (${SDK_TARGET} C ASM)
add_library(${SDK_TARGET} STATIC)
target_compile_options(${SDK_TARGET} PRIVATE -flto)

ADD_SUBDIRECTORY(${DIR_SDK_COMMON} sdk_build/common)
ADD_SUBDIRECTORY(${DIR_SDK_HAL} sdk_build/hal)
ADD_SUBDIRECTORY(${DIR_CSP} sdk_build/csp)
ADD_SUBDIRECTORY(${DIR_SDK_DRV} sdk_build/drv)

include_directories(
    ${DIR_SDK}/api
    ${DIR_SDK_COMMON}
    ${DIR_SDK_HAL}
    ${DIR_SDK_DRV}
    ${DIR_CSP}/inc
)

# extend CMAKE_C_FLAGS by common flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OPT} -Wall -Wextra -Werror -Wcast-align=strict")
