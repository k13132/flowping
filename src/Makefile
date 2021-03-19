####################################################
# MAKE Makefile FlowPing
#
# Description: Makefile for normal and cross 
# compilation in C++ language.
#
# File:   Makefile
# Author: Zbynek Kocur (zbynek.kocur@fel.cvut.cz)
#	
####################################################

# Cross tools prefix
#CROSS_PREFIX=arm-none-linux-gnueabi-
CROSS_PREFIX=

# Compiler and linker
#CXX = g++

# linker	
#LXX = g++

# Arch choices
#CXXFLAGS_ARCH=-march=armv5te -mabi=aapcs-linux
CXXFLAGS_ARCH=

# Common compiler choices
//CXXFLAGS += -pipe -O2 -Wall -D_REENTRANT -fPIC -W
CXXFLAGS += -Wall -Wextra -g -O3 -mssse3  -m64 -faligned-new
# Linker choices
#LXXFLAGS= -march=armv5te -pthread
LXXFLAGS=

# Header files prefix
INCLUDE_PATHS= -I . -I include -I external

# Source files directory
SRC_DIR = sources

# Binary name
TARGET = flowping

# Delete files
DEL_FILE = rm -f

# Libraries path
LIBS_PATH=

# Libs
#LIBS = -lpthread -lrt


# Object files
OBJS = cClient.o cServer.o cSetup.o cStats.o cMBroker.o flowping.o

all: $(TARGET)
	
ifeq ($(OS),Windows_NT)

else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
		LIBS += -lpthread -lrt
    endif
    ifeq ($(UNAME_S),Darwin)
		LIBS += -lCoreFoundation -lIOKit
    endif
endif		

#Debug
ifeq ($(FP_DEBUG),true)
        CXXFLAGS += -g -D_DEBUG
        LXXFLAGS += -g -D_DEBUG
endif

#Profiling
ifeq ($(FP_PROFILE),true)
        CXXFLAGS += -pg
        LXXFLAGS += -pg
endif

#Turn Off Stats Module
ifeq ($(FP_STATS),false)
        CXXFLAGS += -D_NOSTATS
        LXXFLAGS += -D_NOSTATS
endif

#Enforce platform
ifeq ($(FP_PLATFORM),x86_64)
    	CXXFLAGS += -m64
	LXXFLAGS += -m64 
endif

ifeq ($(FP_PLATFORM),x86_32)
    	CXXFLAGS += -m32
	LXXFLAGS += -m32 
endif
	
cClient.o: $(SRC_DIR)/cClient.cpp
	$(CROSS_PREFIX)$(CXX) -c $(CXXFLAGS) $(CFLAGS_ARCH) $(INCLUDE_PATHS) -o cClient.o $(SRC_DIR)/cClient.cpp

cServer.o: $(SRC_DIR)/cServer.cpp
	$(CROSS_PREFIX)$(CXX) -c $(CXXFLAGS) $(CFLAGS_ARCH) $(INCLUDE_PATHS) -o cServer.o $(SRC_DIR)/cServer.cpp 

cSetup.o: $(SRC_DIR)/cSetup.cpp
	$(CROSS_PREFIX)$(CXX) -c $(CXXFLAGS) $(CFLAGS_ARCH) $(INCLUDE_PATHS) -o cSetup.o $(SRC_DIR)/cSetup.cpp 

cStats.o: $(SRC_DIR)/cStats.cpp
	$(CROSS_PREFIX)$(CXX) -c $(CXXFLAGS) $(CFLAGS_ARCH) $(INCLUDE_PATHS) -o cStats.o $(SRC_DIR)/cStats.cpp 

cMBroker.o: $(SRC_DIR)/cMBroker.cpp
	$(CROSS_PREFIX)$(CXX) -c $(CXXFLAGS) $(CFLAGS_ARCH) $(INCLUDE_PATHS) -o cMBroker.o $(SRC_DIR)/cMBroker.cpp 

flowping.o: $(SRC_DIR)/flowping.cpp
	$(CROSS_PREFIX)$(CXX) -c $(CXXFLAGS) $(CFLAGS_ARCH) $(INCLUDE_PATHS) -o flowping.o $(SRC_DIR)/flowping.cpp 

$(TARGET): $(OBJS)
	$(CROSS_PREFIX)$(CXX) $(LXXFLAGS) $(OBJS) $(LIBS_PATH) $(LIBS) -o $(TARGET)

clean:
	rm -f *.o $(TARGET)
	
.PHONY: all
.PHONY: clean
