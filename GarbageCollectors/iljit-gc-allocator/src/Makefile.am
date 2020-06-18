
MAINTAINERCLEANFILES = Makefile.in config.h.in

iljit_gc_allocator_LTLIBRARIES = iljit_gc_allocator.la
iljit_gc_allocatordir = $(libdir)/iljit/gc

AM_CFLAGS= -shared -Wall $(ILJITES_ILJITU_CFLAGS) $(XAN_CFLAGS) $(COMPILERMEMORYMANAGER_CFLAGS) $(IRVIRTUALMACHINE_CFLAGS)

# Additive flags
if DEBUG
AM_CFLAGS 	+= -DDEBUG -O0 -ggdb3
endif
if PROFILE
AM_CFLAGS 	+= -DNDEBUG -DPROFILE -O0 -g
else 
if PRINTDEBUG
AM_CFLAGS 	+= -DPRINTDEBUG
else
if DEBUG
else
AM_CFLAGS 	+= -O3 -DNDEBUG 
endif
endif
endif

iljit_gc_allocator_la_SOURCES=	 						\
		iljit_gc_allocator.c			iljit_gc_allocator.h

iljit_gc_allocator_la_LDFLAGS= -module -avoid-version -no-undefined
if CYGWIN
  iljit_gc_allocator_la_LDFLAGS += -Wl,--enable-auto-import
endif

iljit_gc_allocator_la_LIBADD= $(ILJITES_ILJITU_LIBS) $(XAN_LIBS) $(COMPILERMEMORYMANAGER_LIBS) $(IRVIRTUALMACHINE_LIBS)
