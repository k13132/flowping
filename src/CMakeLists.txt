cmake_minimum_required (VERSION 3.7.2)
project (flowping)
set (FlowPing_VERSION_MAJOR 2)


set(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_FLAGS "-Wall -Wextra -g -O3 -mssse3  -m64  -pthread -std=c++17")

find_package (Threads)

include_directories(./include ./external)
link_directories(./sources)

#include(GNUInstallDirs)

add_library(cSetup ./sources/cSetup.cpp ./include/cSetup.h  external/SPSCQueue.h)
add_library(cStats ./sources/cStats.cpp ./include/cStats.h ./sources/cSetup.cpp ./include/cSetup.h)
add_library(cServer ./sources/cServer.cpp ./sources/cSetup.cpp ./sources/cStats.cpp sources/cMBroker.cpp include/cMBroker.h)
add_library(cClient ./sources/cClient.cpp ./sources/cSetup.cpp ./sources/cStats.cpp sources/cMBroker.cpp include/cMBroker.h )
add_library(cMBroker ./sources/cSetup.cpp include/cSetup.h ./sources/cStats.cpp ./include/cStats.h sources/cMBroker.cpp include/cMBroker.h external/SPSCQueue.h)


link_libraries(cSetup ${CMAKE_THREAD_LIBS_INIT})
link_libraries(cStats ${CMAKE_THREAD_LIBS_INIT})
link_libraries(cServer ${CMAKE_THREAD_LIBS_INIT})
link_libraries(cClient ${CMAKE_THREAD_LIBS_INIT})
link_libraries(cMBroker ${CMAKE_THREAD_LIBS_INIT})

link_libraries(cSetup)
link_libraries(cStats)
link_libraries(cServer)
link_libraries(cClient)
link_libraries(cMBroker)

add_executable(flowping ./sources/flowping.cpp ./sources/cClient.cpp ./sources/cSetup.cpp ./sources/cServer.cpp ./sources/cStats.cpp sources/cMBroker.cpp include/cMBroker.h external/SPSCQueue.h sources/cSlotTimer.cpp include/cSlotTimer.h)
target_link_libraries(flowping cSetup cStats cServer cClient cMBroker ${CMAKE_THREAD_LIBS_INIT})

install(TARGETS flowping RUNTIME DESTINATION /usr/bin)



