MAINTAINERCLEANFILES = Makefile.in config.h.in

libiljitil_LTLIBRARIES = libiljitil.la
libiljitildir = $(libdir)/iljit

AM_CFLAGS=-Wall $(PLATFORM_CFLAGS)		\
		$(XAN_CFLAGS)

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

libiljitil_la_SOURCES=		 					\
		iltype.h

libiljitil_la_LDFLAGS= -fPIC -no-undefined

libiljitil_la_LIBADD= $(PLATFORM_LIBS)		\
		$(XAN_LIBS)
