cmake_minimum_required (VERSION 3.7.2)
project (flowping)
set (FlowPing_VERSION_MAJOR 2)


set(CMAKE_CXX_STANDARD 14)
SET(CMAKE_CXX_FLAGS -pthread)

find_package (Threads)

include_directories(./include)
link_directories(./sources)

#include(GNUInstallDirs)

add_library(cSetup ./sources/cSetup.cpp)
add_library(cStats ./sources/cStats.cpp ./sources/cSetup.cpp)
add_library(cServer ./sources/cServer.cpp ./sources/cSetup.cpp ./sources/cStats.cpp)
add_library(cClient ./sources/cClient.cpp ./sources/cSetup.cpp ./sources/cStats.cpp)

link_libraries(cSetup ${CMAKE_THREAD_LIBS_INIT})
link_libraries(cStats ${CMAKE_THREAD_LIBS_INIT})
link_libraries(cServer ${CMAKE_THREAD_LIBS_INIT})
link_libraries(cClient ${CMAKE_THREAD_LIBS_INIT})

link_libraries(cSetup)
link_libraries(cStats)
link_libraries(cServer)
link_libraries(cClient)

add_executable(flowping ./sources/flowping.cpp ./sources/cClient.cpp ./sources/cSetup.cpp ./sources/cServer.cpp ./sources/cStats.cpp)
target_link_libraries(flowping cSetup cStats cServer cClient ${CMAKE_THREAD_LIBS_INIT})