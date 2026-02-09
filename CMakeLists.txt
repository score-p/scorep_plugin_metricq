cmake_minimum_required(VERSION 3.8)

project(metricq-sink-scorep LANGUAGES CXX)

include(cmake/DefaultBuildType.cmake)
include(cmake/GitSubmoduleUpdate.cmake)
git_submodule_update()

# We're building a shared library. D'oh.
set(NITRO_POSITION_INDEPENDENT_CODE ON CACHE INTERNAL "")
set(METRICQ_POSITION_INDEPENDENT_CODE ON CACHE INTERNAL "")
add_subdirectory(lib)

option(METRICQ_ADVANCED_TIMESYNC "Use advanced time sync capability (needs FFTW3)" OFF)
option(METRICQ_METRIC_PER_HOST "Use the plugin in 'per_host' instead of 'once' metric collection mode" OFF)

add_library(metricq_plugin
    MODULE
        src/main.cpp
)
target_compile_features(metricq_plugin PRIVATE cxx_std_17)
target_link_libraries(metricq_plugin
    PRIVATE
        Scorep::scorep-plugin-cxx
        metricq::sink
        metricq::logger-nitro
)

if(METRICQ_METRIC_PER_HOST)
    target_compile_definitions(metricq_plugin PRIVATE METRICQ_METRIC_PER_HOST=1)
endif()

if(METRICQ_ADVANCED_TIMESYNC)
    find_package(FFTW3 REQUIRED)
    target_link_libraries(metricq_plugin PRIVATE FFTW3::fftw3)
    target_compile_definitions(metricq_plugin PRIVATE ENABLE_TIME_SYNC)
    target_sources(metricq_plugin
        PRIVATE
            src/timesync/timesync.cpp
            src/timesync/footprint.cpp
            src/timesync/shifter.cpp
    )
endif()

install(
    TARGETS metricq_plugin
    LIBRARY DESTINATION lib
)
