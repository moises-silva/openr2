# author: Arnaldo Pereira <arnaldo@sangoma.com>

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(openr2)

ADD_DEFINITIONS(-DHAVE_CONFIG_H)
INCLUDE_DIRECTORIES(. ${CMAKE_CURRENT_BINARY_DIR}/..)

# if we're building on windows, use our own inttypes.h
IF(DEFINED WIN32)
	SET(HAVE_INTTYPES_H 1)
	INCLUDE_DIRECTORIES(openr2/msvc)
ELSE()
	ADD_DEFINITIONS(-DHAVE_GETTIMEOFDAY)
	ADD_DEFINITIONS(-std=c99 -Wall -Werror -Wwrite-strings -Wunused-variable -Wstrict-prototypes -Wmissing-prototypes) # -pedantic
ENDIF()

IF(DEFINED WANT_OR2_TRACE_STACKS)
	ADD_DEFINITIONS(-DOR2_TRACE_STACKS)
ENDIF()

IF(DEFINED HAVE_SVNVERSION)
	ADD_DEFINITIONS(-DREVISION=\"$(shell svnversion -n .)\")
ENDIF()

IF(DEFINED HAVE_ATTR_VISIBILITY_HIDDEN)
	ADD_DEFINITIONS(-fvisibility=hidden)
ENDIF()

ADD_DEFINITIONS(-D__OR2_COMPILING_LIBRARY__)

SET(SOURCES r2chan.c r2context.c r2log.c r2proto.c r2utils.c
	r2engine.c r2ioabs.c queue.c r2thread.c
)

IF(DEFINED WANT_R2TEST)
	ADD_TARGET(r2test r2test.c)
	TARGET_LINK_LIBRARIES(r2test pthread ${PROJECT_NAME})

	ADD_TARGET(r2dtmf_detect r2dtmf_detect.c)
	TARGET_LINK_LIBRARIES(r2dtmf_detect pthread ${PROJECT_NAME})
ENDIF()

ADD_LIBRARY(${PROJECT_NAME} SHARED ${SOURCES})
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES SOVERSION ${SOVERSION})

# on windows, we check if winmm is available (guess it's always),
# if it's not generate gettimeofday() with 20ms resolution instead of 1
IF(DEFINED WIN32)
	FIND_LIBRARY(MM_LIB NAMES winmm)
	IF(NOT ${MM_LIB})
		ADD_DEFINITIONS(-DWITHOUT_MM_LIB)
	ELSE()
		TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${MM_LIB})
	ENDIF()
ENDIF()

# install - all relative to CMAKE_INSTALL_PREFIX
INSTALL(TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
)

INSTALL(FILES openr2/openr2.h DESTINATION include)
INSTALL(FILES
		openr2/r2chan.h
		openr2/r2context.h
		openr2/r2proto.h
		openr2/r2utils.h
		openr2/r2log.h
		openr2/r2exports.h
		openr2/r2thread.h
	DESTINATION include/openr2
)
