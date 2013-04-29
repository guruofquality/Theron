########################################################################
# Include this file to get a list of include paths and definitions
# Input: THERON_SOURCE_DIR
# Output: THERON_INCLUDE_DIRS
# Output: THERON_LIBRARY_DIRS
# Output: THERON_LIBRARIES
# Output: THERON_DEFINES
# Output: THERON_SOURCES
########################################################################

list(APPEND CMAKE_MODULE_PATH ${THERON_SOURCE_DIR})

########################################################################
# Setup the list of sources
########################################################################
file(GLOB THERON_SOURCES "${THERON_SOURCE_DIR}/Theron/*.cpp")

########################################################################
# Detect the system defines
########################################################################
if(WIN32)
    list(APPEND THERON_DEFINES -DTHERON_WINDOWS=1)
endif()

if(MSVC)
    list(APPEND THERON_DEFINES -DTHERON_MSVC=1)
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
    list(APPEND THERON_DEFINES -DTHERON_GCC=1)
endif()

include(CheckTypeSize)
enable_language(C)
check_type_size("void*[8]" SIZEOF_CPU BUILTIN_TYPES_ONLY)
if(${SIZEOF_CPU} EQUAL 64)
    list(APPEND THERON_DEFINES -DTHERON_64BIT=1)
endif()

if(CMAKE_COMPILER_IS_GNUCXX AND ${SIZEOF_CPU} EQUAL 64)
    list(APPEND THERON_DEFINES -fPIC)
endif()

if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    list(APPEND THERON_DEFINES -DTHERON_DEBUG=1)
endif()

########################################################################
# Setup the include directories
########################################################################
list(APPEND THERON_INCLUDE_DIRS ${THERON_SOURCE_DIR}/Include)
list(APPEND THERON_INCLUDE_DIRS ${THERON_SOURCE_DIR}/Include/External)

if(MSVC)
    list(APPEND THERON_INCLUDE_DIRS ${THERON_SOURCE_DIR}/Include/Standard)
endif(MSVC)

########################################################################
# Find and setup Boost Threads
########################################################################
if(UNIX AND NOT BOOST_ROOT AND EXISTS "/usr/lib64" AND NOT BOOST_LIBRARYDIR)
    list(APPEND BOOST_LIBRARYDIR "/usr/lib64") #fedora 64-bit fix
endif(UNIX AND NOT BOOST_ROOT AND EXISTS "/usr/lib64" AND NOT BOOST_LIBRARYDIR)

set(Boost_ADDITIONAL_VERSIONS
    "1.35.0" "1.35" "1.36.0" "1.36" "1.37.0" "1.37" "1.38.0" "1.38" "1.39.0" "1.39"
    "1.40.0" "1.40" "1.41.0" "1.41" "1.42.0" "1.42" "1.43.0" "1.43" "1.44.0" "1.44"
    "1.45.0" "1.45" "1.46.0" "1.46" "1.47.0" "1.47" "1.48.0" "1.48" "1.49.0" "1.49"
    "1.50.0" "1.50" "1.51.0" "1.51" "1.52.0" "1.52" "1.53.0" "1.53" "1.54.0" "1.54"
    "1.55.0" "1.55" "1.56.0" "1.56" "1.57.0" "1.57" "1.58.0" "1.58" "1.59.0" "1.59"
    "1.60.0" "1.60" "1.61.0" "1.61" "1.62.0" "1.62" "1.63.0" "1.63" "1.64.0" "1.64"
    "1.65.0" "1.65" "1.66.0" "1.66" "1.67.0" "1.67" "1.68.0" "1.68" "1.69.0" "1.69"
)
find_package(Boost COMPONENTS thread system)

if(Boost_FOUND)
    list(APPEND THERON_INCLUDE_DIRS ${Boost_INCLUDE_DIRS})
    list(APPEND THERON_LIBRARY_DIRS ${Boost_LIBRARY_DIRS})
    list(APPEND THERON_LIBRARIES ${Boost_LIBRARIES})
    list(APPEND THERON_DEFINES -DTHERON_BOOST=1)
    if(${Boost_VERSION} LESS 104200)
        list(APPEND THERON_DEFINES "-Dmemory_order_consume=memory_order(8)")
    endif()
endif()

########################################################################
# Find and setup PThreads
########################################################################
find_package(Pthreads)

if(PTHREADS_FOUND)
    list(APPEND THERON_INCLUDE_DIRS ${PTHREADS_INCLUDE_DIR})
    list(APPEND THERON_LIBRARIES ${PTHREADS_LIBRARY})
    list(APPEND THERON_DEFINES -DTHERON_POSIX=1)
    list(APPEND THERON_DEFINES -DHAVE_PTHREAD_H)
endif()

########################################################################
# Find and setup c++11
########################################################################
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG(-std=c++11 HAS_STD_CPP11)

if(HAS_STD_CPP11)
    list(APPEND THERON_DEFINES -DTHERON_CPP11=1)
    list(APPEND THERON_DEFINES -std=c++11)
endif()

########################################################################
# Extra linux specific libraries
########################################################################
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")

    find_library(
        NUMA_LIBRARIES
        NAMES numa
        PATHS /usr/lib /usr/lib64
    )

    if(NUMA_LIBRARIES)
        list(APPEND THERON_LIBRARIES ${NUMA_LIBRARIES})
        list(APPEND THERON_DEFINES -DTHERON_NUMA=1)
    endif()

    find_library(
        RT_LIBRARIES
        NAMES rt
        PATHS /usr/lib /usr/lib64
    )

    if(RT_LIBRARIES)
        list(APPEND THERON_LIBRARIES ${RT_LIBRARIES})
    else()
        message(FATAL_ERROR "librt required to build Theron")
    endif()

endif()

########################################################################
# Setup Crossroads IO (optional)
########################################################################
find_package(XS)

if(XS_FOUND)
    list(APPEND THERON_LIBRARY_DIRS ${XS_INCLUDE_DIR})
    list(APPEND THERON_LIBRARIES ${XS_LIBRARIES})
    list(APPEND THERON_DEFINES -DTHERON_XS=1)
endif()

########################################################################
# Print results
########################################################################
message(STATUS "THERON_INCLUDE_DIRS: ${THERON_INCLUDE_DIRS}")
message(STATUS "THERON_LIBRARY_DIRS: ${THERON_LIBRARY_DIRS}")
message(STATUS "THERON_LIBRARIES: ${THERON_LIBRARIES}")
message(STATUS "THERON_DEFINES: ${THERON_DEFINES}")
