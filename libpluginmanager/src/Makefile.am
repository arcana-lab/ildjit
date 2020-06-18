MAINTAINERCLEANFILES = Makefile.in config.h.in

libpluginmanager_LTLIBRARIES = libpluginmanager.la
libpluginmanagerdir = $(libdir)/iljit

AM_CFLAGS=-Wall $(XANLIB_CFLAGS) $(LIBILJITU_CFLAGS) $(COMPILERMEMORYMANAGER_CFLAGS) $(PLATFORM_CFLAGS)

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

libpluginmanager_la_SOURCES=		 					\
		plugin_manager.c		plugin_manager.h

libpluginmanager_la_LDFLAGS= -fPIC -no-undefined
libpluginmanager_la_LIBADD= $(XANLIB_LIBS) $(LIBILJITU_LIBS) $(COMPILERMEMORYMANAGER_LIBS) $(PLATFORM_LIBS)
