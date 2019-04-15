project(metricq-sink-scorep)

cmake_minimum_required(VERSION 3.8)

include(cmake/DefaultBuildType.cmake)
include(cmake/GitSubmoduleUpdate.cmake)
git_submodule_update()

add_subdirectory(lib)

find_package(FFTW3)

add_library(metricq_plugin
    MODULE
        src/main.cpp
        src/log.cpp

        src/timesync/footprint.cpp
        src/timesync/shifter.cpp
        src/timesync/timesync.cpp
)
target_compile_features(metricq_plugin PRIVATE cxx_std_17)
target_link_libraries(metricq_plugin
    PRIVATE
        Scorep::scorep-plugin-cxx
        metricq::sink
        FFTW3::fftw3
)

install(
    TARGETS metricq_plugin
    LIBRARY DESTINATION lib
)
