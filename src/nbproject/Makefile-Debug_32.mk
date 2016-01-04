#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU-Linux
CND_DLIB_EXT=so
CND_CONF=Debug_32
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/cClient.o \
	${OBJECTDIR}/cServer.o \
	${OBJECTDIR}/cSetup.o \
	${OBJECTDIR}/flowping.o


# C Compiler Flags
CFLAGS=-m32 -lpthread

# CC Compiler Flags
CCFLAGS=-m64 -lpthread -lrt
CXXFLAGS=-m64 -lpthread -lrt

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/src

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/src: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/src ${OBJECTFILES} ${LDLIBSOPTIONS}

${OBJECTDIR}/cClient.o: cClient.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Wall -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/cClient.o cClient.cpp

${OBJECTDIR}/cServer.o: cServer.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Wall -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/cServer.o cServer.cpp

${OBJECTDIR}/cSetup.o: cSetup.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Wall -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/cSetup.o cSetup.cpp

${OBJECTDIR}/flowping.o: flowping.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Wall -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/flowping.o flowping.cpp

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/src

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
