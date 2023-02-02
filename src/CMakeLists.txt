cmake_minimum_required (VERSION 3.7.2)
project (flowping)
set (FlowPing_VERSION_MAJOR 2)


set(CMAKE_CXX_STANDARD 17)
if(CMAKE_CROSSCOMPILING)
    SET(CMAKE_CXX_FLAGS "-Wall -Wextra -g -O3 -pthread -faligned-new")
else()
    if (CMAKE_HOST_UNIX)
        if (${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "x86_64")
            SET(CMAKE_CXX_FLAGS "-Wall -Wextra -g -O3 -mssse3  -m64  -pthread -faligned-new")
        else()
            message( STATUS ${CMAKE_HOST_SYSTEM_PROCESSOR})
        endif()
    else()
        message( FATAL_ERROR "Only 64bit version is supported." )
    endif()
endif()

find_package (Threads)

include_directories(./include ./external)
link_directories(./sources)

#necessary for gcc prior to version 9.1
link_libraries(stdc++fs)

#include(GNUInstallDirs)

add_library(cSetup ./sources/cSetup.cpp ./include/cSetup.h  external/SPSCQueue.h)
add_library(cServer ./sources/cServer.cpp ./sources/cSetup.cpp sources/cMBroker.cpp include/cMBroker.h sources/cCBroker.cpp include/cCBroker.h)
add_library(cClient ./sources/cClient.cpp ./sources/cSetup.cpp sources/cMBroker.cpp include/cMBroker.h )
add_library(cMBroker ./sources/cSetup.cpp include/cSetup.h sources/cMBroker.cpp include/cMBroker.h external/SPSCQueue.h)
add_library(cCBroker ./sources/cCBroker.cpp include/cCBroker.h)
add_library(cSlotTimer ./sources/cSlotTimer.cpp ./include/cSlotTimer.h ./sources/cSetup.cpp ./include/cSetup.h sources/cMBroker.cpp include/cMBroker.h)


link_libraries(cSetup ${CMAKE_THREAD_LIBS_INIT})
link_libraries(cServer ${CMAKE_THREAD_LIBS_INIT})
link_libraries(cClient ${CMAKE_THREAD_LIBS_INIT})
link_libraries(cMBroker ${CMAKE_THREAD_LIBS_INIT})
link_libraries(cCBroker ${CMAKE_THREAD_LIBS_INIT})
link_libraries(cSlotTimer ${CMAKE_THREAD_LIBS_INIT})

link_libraries(cSetup)
link_libraries(cSlotTimer)
link_libraries(cServer)
link_libraries(cClient)
link_libraries(cMBroker)
link_libraries(cCBroker)

add_executable(flowping ./sources/flowping.cpp ./sources/cClient.cpp ./sources/cSetup.cpp ./sources/cServer.cpp sources/cMBroker.cpp include/cMBroker.h external/SPSCQueue.h sources/cSlotTimer.cpp include/cSlotTimer.h sources/cCBroker.cpp include/cCBroker.h)
target_link_libraries(flowping cSetup cServer cClient cMBroker cCBroker cSlotTimer ${CMAKE_THREAD_LIBS_INIT})

install(TARGETS flowping RUNTIME DESTINATION /usr/bin)



