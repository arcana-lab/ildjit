
MAINTAINERCLEANFILES = Makefile.in config.h.in

libiljitiroptimizer_LTLIBRARIES = libiljitiroptimizer.la
libiljitiroptimizerdir = $(libdir)/iljit

AM_CFLAGS= -Wall $(XANLIB_CFLAGS) $(LIBILJITU_CFLAGS) $(LIBILJITIR_CFLAGS) $(COMPILERMEMORYMANAGER_CFLAGS) $(PLATFORM_CFLAGS) $(LIBPLUGINMANAGER_CFLAGS)

if CYGWIN
AM_CFLAGS 	+= -shared 
endif

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

# Morpheus project
if MORPHEUS
AM_CFLAGS	+= -DMORPHEUS
endif

libiljitiroptimizer_la_SOURCES=		 					\
		ir_optimizer.c			ir_optimizer.h			\
		optimization_levels.c		optimization_levels.h		\
		optimizations_utilities.c	optimizations_utilities.h	\
		ir_optimization_interface.h					\
		ir_optimization_levels_interface.h

libiljitiroptimizer_la_LDFLAGS= -fPIC -no-undefined
libiljitiroptimizer_la_LIBADD= $(XANLIB_LIBS) $(LIBILJITU_LIBS) $(LIBILJITIR_LIBS) $(COMPILERMEMORYMANAGER_LIBS) $(PLATFORM_LIBS) $(LIBPLUGINMANAGER_LIBS)

if CYGWIN
  libiljitiroptimizer_la_LDFLAGS += -Wl,--enable-auto-import
endif
