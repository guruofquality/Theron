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
# Setup Theron Deps
########################################################################
set(THERON_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
include(${THERON_SOURCE_DIR}/TheronSetup.cmake)
include_directories(${THERON_INCLUDE_DIRS})
link_directories(${THERON_LIBRARY_DIRS})
add_definitions(${THERON_DEFINES})

########################################################################
# Build library
########################################################################
add_library(theron STATIC ${THERON_SOURCES})
target_link_libraries(theron ${THERON_LIBRARIES})
set_target_properties(theron PROPERTIES VERSION "6.00.00")

########################################################################
# Build unittests
########################################################################
include_directories(${THERON_SOURCE_DIR}/Tests)
file(GLOB test_sources "${THERON_SOURCE_DIR}/Tests/*.cpp")
add_executable(test_suite ${test_sources})
target_link_libraries(test_suite theron ${THERON_LIBRARIES})
add_test(theron_test_suite test_suite)

########################################################################
# Build samples
########################################################################
file(GLOB sample_dirs
    "${THERON_SOURCE_DIR}/Benchmarks/*"
    "${THERON_SOURCE_DIR}/Tutorial/*"
)
foreach(sample_dir ${sample_dirs})
    get_filename_component(sample ${sample_dir} NAME)
    file(GLOB sample_sources "${sample_dir}/*.cpp")
    if(sample_sources AND NOT "${sample}" STREQUAL "Theron")
        add_executable(${sample} ${sample_sources})
        target_link_libraries(${sample} theron ${THERON_LIBRARIES})
    endif()
endforeach(sample_dir)
