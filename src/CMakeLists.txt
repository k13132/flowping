cmake_minimum_required (VERSION 3.7)
project (flowping)
set (FlowPing_VERSION_MAJOR 1)
set (Tutorial_VERSION_MINOR 5.2)

set(CMAKE_CXX_STANDARD 11)
SET(CMAKE_CXX_FLAGS -pthread)

#include_directories("./include")
#link_directories("sources")

include_directories(./include)
link_directories(./sources)

add_library(cSetup ./sources/cSetup.cpp)
add_library(cStats ./sources/cStats.cpp ./sources/cSetup.cpp)
add_library(cServer ./sources/cServer.cpp ./sources/cSetup.cpp ./sources/cStats.cpp)
add_library(cClient ./sources/cClient.cpp ./sources/cSetup.cpp ./sources/cStats.cpp)

link_libraries(cSetup ${CMAKE_THREAD_LIBS_INIT})
link_libraries(cStats ${CMAKE_THREAD_LIBS_INIT})
link_libraries(cServer ${CMAKE_THREAD_LIBS_INIT})
link_libraries(cClient ${CMAKE_THREAD_LIBS_INIT})

add_executable(flowping ./sources/flowping.cpp ./sources/cClient.cpp ./sources/cSetup.cpp ./sources/cServer.cpp ./sources/cStats.cpp)
target_link_libraries(flowping cSetup cStats cServer cClient ${CMAKE_THREAD_LIBS_INIT})



include(GNUInstallDirs)
find_package (Threads)



#add_subdirectory(sources)


