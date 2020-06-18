MAINTAINERCLEANFILES = Makefile.in config.h.in

libiljitgc_LTLIBRARIES = libiljitgc.la
libiljitgcdir = $(libdir)/iljit

AM_CFLAGS=-Wall $(PLATFORM_CFLAGS)		\
		$(XAN_CFLAGS)			\
		$(ILJITIROPTIMIZER_CFLAGS)

if PROFILE
AM_CFLAGS 	+= -DNDEBUG -DPROFILE -O0 -g
else
if PRINTDEBUG
AM_CFLAGS	+= -ggdb -DDEBUG -DPRINTDEBUG -O0
else 
if DEBUG
AM_CFLAGS 	+= -DDEBUG -O0 -ggdb3 
else 
AM_CFLAGS	+= -O3 -DNDEBUG 
endif
endif
endif

libiljitgc_la_SOURCES=		 					\
		garbage_collector.h					\
		framework_garbage_collector.h

libiljitgc_la_LDFLAGS= -fPIC -no-undefined

libiljitgc_la_LIBADD= $(PLATFORM_LIBS)		\
		$(XAN_LIBS)			\
		$(ILJITIROPTIMIZER_LIBS)
