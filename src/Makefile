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
CXXFLAGS += -pipe -O2 -Wall -D_REENTRANT -fPIC -w

# Linker choices
#LXXFLAGS= -march=armv5te -pthread
LXXFLAGS=

# Header files prefix
INCLUDE_PATHS= -I . -I include

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
OBJS = cClient.o cServer.o cSetup.o flowping.o

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

	
cClient.o: $(SRC_DIR)/cClient.cpp
	$(CROSS_PREFIX)$(CXX) -c $(CXXFLAGS) $(CFLAGS_ARCH) $(INCLUDE_PATHS) -o cClient.o $(SRC_DIR)/cClient.cpp

cServer.o: $(SRC_DIR)/cServer.cpp
	$(CROSS_PREFIX)$(CXX) -c $(CXXFLAGS) $(CFLAGS_ARCH) $(INCLUDE_PATHS) -o cServer.o $(SRC_DIR)/cServer.cpp 

cSetup.o: $(SRC_DIR)/cSetup.cpp
	$(CROSS_PREFIX)$(CXX) -c $(CXXFLAGS) $(CFLAGS_ARCH) $(INCLUDE_PATHS) -o cSetup.o $(SRC_DIR)/cSetup.cpp 

flowping.o: $(SRC_DIR)/flowping.cpp
	$(CROSS_PREFIX)$(CXX) -c $(CXXFLAGS) $(CFLAGS_ARCH) $(INCLUDE_PATHS) -o flowping.o $(SRC_DIR)/flowping.cpp 

$(TARGET): $(OBJS)
	$(CROSS_PREFIX)$(CXX) $(LXXFLAGS) $(OBJS) $(LIBS_PATH) $(LIBS) -o $(TARGET)

clean:
	rm -f *.o $(TARGET)
	
.PHONY: all
.PHONY: clean
