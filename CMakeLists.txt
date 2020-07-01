project(metricq-sink-scorep)

cmake_minimum_required(VERSION 3.8)

include(cmake/DefaultBuildType.cmake)
include(cmake/GitSubmoduleUpdate.cmake)
git_submodule_update()

# We're building a shared library. D'oh.
set(NITRO_POSITION_INDEPENDENT_CODE ON CACHE INTERNAL "")
set(METRICQ_POSITION_INDEPENDENT_CODE ON CACHE INTERNAL "")
add_subdirectory(lib)

find_package(FFTW3)

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

if(FFTW3_FOUND)
    target_link_libraries(metricq_plugin PRIVATE FFTW3::fftw3)
    target_compile_definitions(metricq_plugin PRIVATE ENABLE_TIME_SYNC)
    target_sources(metricq_plugin
        PRIVATE
            src/timesync/timesync.cpp
            src/timesync/footprint.cpp
            src/timesync/shifter.cpp
    )
else()
    message(STATUS "Couldn't find FFTW3, advanced time syncronization is not available")
endif()

install(
    TARGETS metricq_plugin
    LIBRARY DESTINATION lib
)
