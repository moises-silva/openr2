AM_CFLAGS = -std=c99 -pedantic -Wall -Werror -Wwrite-strings -Wunused-variable -Wstrict-prototypes -Wmissing-prototypes 
if WANT_OR2_TRACE_STACKS
AM_CFLAGS += -DOR2_TRACE_STACKS
endif

if HAVE_SVNVERSION
AM_CFLAGS += -DREVISION=\"$(shell svnversion -n .)\"
endif

lib_LTLIBRARIES = libopenr2.la

include_HEADERS = openr2/openr2.h 
nobase_include_HEADERS = openr2/r2chan.h \
		         openr2/r2context.h \
		         openr2/r2proto.h \
		         openr2/r2utils.h \
		         openr2/r2log.h \
		         openr2/r2exports.h 

libopenr2_la_SOURCES = r2chan.c r2context.c r2log.c r2proto.c r2utils.c r2engine.c r2ioabs.c
libopenr2_la_CFLAGS = $(AM_CFLAGS) -D__OR2_COMPILING_LIBRARY__
if HAVE_ATTR_VISIBILITY_HIDDEN
libopenr2_la_CFLAGS += -fvisibility=hidden
endif
libopenr2_la_LDFLAGS = -lm -lpthread -version-info $(OPENR2_LT_CURRENT):@OPENR2_LT_REVISION@:@OPENR2_LT_AGE@


if WANT_R2TEST
bin_PROGRAMS = r2test
r2test_SOURCES = r2test.c 
r2test_LDADD = -lpthread libopenr2.la
r2test_CFLAGS = $(AM_CFLAGS)
endif

INCLUDES = -Iopenr2

