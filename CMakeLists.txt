########################################################################
# Project setup
########################################################################
cmake_minimum_required(VERSION 2.6)
project(Theron CXX)
enable_testing()

#select the release build type by default to get optimization flags
if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE "Release")
   message(STATUS "Build type not specified: defaulting to release.")
endif(NOT CMAKE_BUILD_TYPE)
set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "")

########################################################################
# Setup Boost and handle some system specific things
########################################################################
if(NOT MSVC)
    if(UNIX AND EXISTS "/usr/lib64")
        list(APPEND BOOST_LIBRARYDIR "/usr/lib64") #fedora 64-bit fix
    endif(UNIX AND EXISTS "/usr/lib64")

    set(Boost_ADDITIONAL_VERSIONS
        "1.35.0" "1.35" "1.36.0" "1.36" "1.37.0" "1.37" "1.38.0" "1.38" "1.39.0" "1.39"
        "1.40.0" "1.40" "1.41.0" "1.41" "1.42.0" "1.42" "1.43.0" "1.43" "1.44.0" "1.44"
        "1.45.0" "1.45" "1.46.0" "1.46" "1.47.0" "1.47" "1.48.0" "1.48" "1.49.0" "1.49"
        "1.50.0" "1.50" "1.51.0" "1.51" "1.52.0" "1.52" "1.53.0" "1.53" "1.54.0" "1.54"
        "1.55.0" "1.55" "1.56.0" "1.56" "1.57.0" "1.57" "1.58.0" "1.58" "1.59.0" "1.59"
        "1.60.0" "1.60" "1.61.0" "1.61" "1.62.0" "1.62" "1.63.0" "1.63" "1.64.0" "1.64"
        "1.65.0" "1.65" "1.66.0" "1.66" "1.67.0" "1.67" "1.68.0" "1.68" "1.69.0" "1.69"
    )
    find_package(Boost COMPONENTS thread)

    if(NOT Boost_FOUND)
        message(FATAL_ERROR "boost threads required to build Theron")
    endif()

    include_directories(${Boost_INCLUDE_DIRS})
    link_directories(${Boost_LIBRARY_DIRS})
    set(theron_libs ${Boost_LIBRARIES})

    add_definitions(-DTHERON_USE_BOOST_THREADS)
endif()

########################################################################
# Build library
########################################################################
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Include/External)

if(MSVC)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Include/External/Standard)
endif(MSVC)

if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    add_definitions(-DTHERON_DEBUG)
    add_definitions(-DTHERON_ENABLE_ASSERTS)
endif()

file(GLOB theron_sources "${CMAKE_CURRENT_SOURCE_DIR}/Theron/*.cpp")
add_library(theron STATIC ${theron_sources})
target_link_libraries(theron ${theron_libs})
set_target_properties(theron PROPERTIES VERSION "4.02.02")

########################################################################
# Build unittests
########################################################################
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Tests)
file(GLOB test_sources "${CMAKE_CURRENT_SOURCE_DIR}/Tests/*.cpp")
add_executable(test_suite ${test_sources})
target_link_libraries(test_suite theron ${theron_libs})
add_test(theron_test_suite test_suite)

########################################################################
# Build samples
########################################################################
file(GLOB sample_dirs
    "${CMAKE_CURRENT_SOURCE_DIR}/Benchmarks/*"
    "${CMAKE_CURRENT_SOURCE_DIR}/Tutorial/*"
)
foreach(sample_dir ${sample_dirs})
    get_filename_component(sample ${sample_dir} NAME)
    file(GLOB sample_sources "${sample_dir}/*.cpp")
    if(sample_sources)
        add_executable(${sample} ${sample_sources})
        target_link_libraries(${sample} theron ${theron_libs})
    endif()
endforeach(sample_dir)
