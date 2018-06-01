project(dataheap2-sink-scorep)

cmake_minimum_required(VERSION 3.8)

include(cmake/DefaultBuildType.cmake)
include(cmake/GitSubmoduleUpdate.cmake)
git_submodule_update()

add_subdirectory(lib)

find_package(FFTW3)

add_library(dataheap2_plugin
    MODULE
        src/main.cpp
        src/log.cpp

        src/timesync/footprint.cpp
        src/timesync/timesync.cpp
)
target_compile_features(dataheap2_plugin PRIVATE cxx_std_17)
target_link_libraries(dataheap2_plugin
    PRIVATE
        Scorep::scorep-plugin-cxx
        dataheap2::sink
        FFTW3::fftw3
)

install(
    TARGETS dataheap2_plugin
    LIBRARY DESTINATION lib
)
