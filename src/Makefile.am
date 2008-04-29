lib_LTLIBRARIES = libopenr2.la

include_HEADERS = openr2/openr2.h 
nobase_include_HEADERS = openr2/r2chan.h \
		         openr2/r2context.h \
		         openr2/r2proto.h \
		         openr2/r2utils.h \
		         openr2/r2log.h

libopenr2_la_SOURCES = r2chan.c r2context.c r2log.c r2proto.c r2utils.c

bin_PROGRAMS = r2test
r2test_SOURCES = r2test.c 
r2test_LDADD = -lspandsp libopenr2.la

INCLUDES = -Iopenr2
LIBS = -lpthread

