# author: Arnaldo Pereira <arnaldo@sangoma.com>

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(openr2)

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR})

#
# fetch current COMPILE_FLAGS for TARGET_NAME target, append
# DEFS to it and save it back. these flags gets stored on the target
# property, so they're not globally available to every compilation,
# differently from add_definitions()
#
macro(target_add_cflags TARGET_NAME DEFS)
	get_target_property(MYDEFS ${TARGET_NAME} COMPILE_FLAGS)
	if(NOT "${MYDEFS}" STREQUAL "MYDEFS-NOTFOUND")
		set(mydefs "${MYDEFS} ${DEFS}")
	else()
		set(mydefs ${DEFS})
	endif()
	set_target_properties(${TARGET_NAME} PROPERTIES COMPILE_FLAGS "${mydefs}")
endmacro(target_add_cflags)

# cmake doens't automatically prepend 'lib' to the project name on win32,
# so we do manually.
IF(DEFINED WIN32)
    SET(PROJECT_TARGET lib${PROJECT_NAME})
ELSE()
    SET(PROJECT_TARGET ${PROJECT_NAME})
ENDIF()

SET(SOURCES r2chan.c r2context.c r2log.c r2proto.c r2utils.c
	r2engine.c r2ioabs.c queue.c r2thread.c
)
ADD_LIBRARY(${PROJECT_TARGET} SHARED ${SOURCES})

# helper to incrementally set cflags
macro(or2_cflags DEFS)
	target_add_cflags(${PROJECT_TARGET} ${DEFS})
endmacro(or2_cflags)

SET_TARGET_PROPERTIES(${PROJECT_TARGET} PROPERTIES SOVERSION ${SOVERSION})
or2_cflags("-DHAVE_CONFIG_H -DOR2_EXPORTS -D__OR2_COMPILING_LIBRARY__")

# if we're building on windows, use our own inttypes.h
IF(DEFINED WIN32)
	SET(HAVE_INTTYPES_H 1)
	or2_cflags(-DWIN32_LEAN_AND_MEAN)
	INCLUDE_DIRECTORIES(openr2/msvc)
ELSE()
	or2_cflags("-ggdb3 -O0 -DHAVE_GETTIMEOFDAY")
	ADD_DEFINITIONS(-std=c99 -Wall -Werror -Wwrite-strings -Wunused-variable -Wstrict-prototypes -Wmissing-prototypes) # -pedantic
ENDIF()

IF(DEFINED HAVE_SVNVERSION)
	or2_cflags(-DREVISION=\"$(shell svnversion -n .)\")
ENDIF()

IF(DEFINED HAVE_ATTR_VISIBILITY_HIDDEN)
	or2_cflags(-fvisibility=hidden)
ENDIF()

# if WANT_R2TEST is defined, build tests binaries
IF(DEFINED WANT_R2TEST)
	FOREACH(TEST_TARGET r2test r2dtmf_detect r2mf_detect r2mf_generate)
		ADD_EXECUTABLE(${TEST_TARGET} ${TEST_TARGET}.c)
		TARGET_LINK_LIBRARIES(${TEST_TARGET} pthread m ${PROJECT_TARGET})
	ENDFOREACH(TEST_TARGET)
ENDIF()

# on windows, we check if winmm is available (guess it's always),
# if it's not generate gettimeofday() with 20ms resolution instead of 1
IF(DEFINED WIN32)
	FIND_LIBRARY(MM_LIB NAMES winmm)
	IF(NOT ${MM_LIB})
		or2_cflags(-DWITHOUT_MM_LIB)
	ELSE()
		TARGET_LINK_LIBRARIES(${PROJECT_TARGET} ${MM_LIB})
	ENDIF()
ENDIF()

# install - all relative to CMAKE_INSTALL_PREFIX
INSTALL(TARGETS ${PROJECT_TARGET}
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION ${MY_LIB_PATH}
	ARCHIVE DESTINATION ${MY_LIB_PATH}
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
		openr2/r2declare.h
		openr2/r2engine.h
	DESTINATION include/openr2
)

IF(DEFINED WIN32)
	# on windows, also add our own inttypes.h to the distributed headers
	INSTALL(FILES openr2/msvc/inttypes.h DESTINATION include/openr2)
ENDIF()
