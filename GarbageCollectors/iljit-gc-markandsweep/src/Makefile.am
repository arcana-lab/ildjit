iljit_gc_markandsweep_LTLIBRARIES = iljit_gc_markandsweep.la
iljit_gc_markandsweepdir = $(libdir)/iljit/gc

AM_CFLAGS= -shared -Wall $(ILJITES_ILJITU_CFLAGS) $(XAN_CFLAGS) $(IRVIRTUALMACHINE_CFLAGS)

# Additive flags
if DEBUG
AM_CFLAGS 	+= -DDEBUG -O0 -ggdb3
if !CYGWIN
AM_CFLAGS += -fstack-protector
endif
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

iljit_gc_markandsweep_la_SOURCES=	 						\
		iljit_gc_markandsweep.c			iljit_gc_markandsweep.h		\
		memory_manager.c			memory_manager.h		\
		internal_memory_manager.c		internal_memory_manager.h

iljit_gc_markandsweep_la_LDFLAGS= -module -avoid-version -no-undefined
if CYGWIN
  iljit_gc_markandsweep_la_LDFLAGS += -Wl,--enable-auto-import
endif
iljit_gc_markandsweep_la_LIBADD= $(ILJITES_ILJITU_LIBS) $(XAN_LIBS) $(IRVIRTUALMACHINE_LIBS)
