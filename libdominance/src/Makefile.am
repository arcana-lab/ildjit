libdominance_LTLIBRARIES = libdominance.la
libdominancedir = $(libdir)/iljit

AM_CFLAGS= -Wall -I../include $(XAN_CFLAGS) $(ILJITU_CFLAGS) $(ILJITSM_CFLAGS) $(ILJITMM_CFLAGS) $(LIBILJITIR_CFLAGS) $(COMPILERMEMORYMANAGER_CFLAGS)

if PROFILE
AM_CFLAGS	+= -g -DNDEBUG -O0
else
if PRINTDEBUG
AM_CFLAGS	+= -ggdb -DDEBUG -DPRINTDEBUG -O0
else 
if DEBUG
AM_CFLAGS	+= -ggdb -DDEBUG -O0
else 
AM_CFLAGS	+= -O3 -DNDEBUG 
endif
endif
endif

libdominance_la_SOURCES=									\
		dominance.c			dominance.h					\
		bitset.c			bitset.h

libdominance_la_LIBADD	= $(ILJITU_LIBS) $(ILJITSM_LIBS) $(ILJITMM_LIBS) $(XAN_LIBS) $(COMPILERMEMORYMANAGER_LIBS)

libdominance_la_LDFLAGS= -shared -fPIC $(ILJITU_LIBS) $(ILJITSM_LIBS) $(ILJITMM_LIBS) $(LIBILJITIR_LIBS) $(XAN_LIBS)
