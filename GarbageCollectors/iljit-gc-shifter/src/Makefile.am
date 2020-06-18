iljit_gc_shifter_LTLIBRARIES = iljit_gc_shifter.la
iljit_gc_shifterdir = $(libdir)/iljit/gc

AM_CFLAGS= -Wall $(ILJITES_ILJITU_CFLAGS) $(XAN_CFLAGS)

# Additive flags
if PROFILE
AM_CFLAGS 	+= -DNDEBUG -DPROFILE -O0 -g
else 
if PRINTDEBUG
AM_CFLAGS 	+= -Wall -DPRINTDEBUG -DDEBUG -O0
else
if DEBUG
AM_CFLAGS 	+= -Wall -DDEBUG -O0
else
AM_CFLAGS 	+= -O3 -DNDEBUG 
endif
endif
endif

iljit_gc_shifter_la_SOURCES=	 							\
		iljit_gc_shifter.c			iljit_gc_shifter.h		\
		memory_manager.c			memory_manager.h		\
		internal_memory_manager.c		internal_memory_manager.h	\
		sweeper.c				sweeper.h			\
		gc_shifter_general_tools.c		gc_shifter_general_tools.h	\
		system.h

iljit_gc_shifter_la_LDFLAGS= -module -avoid-version $(ILJITES_ILJITU_LIBS) $(XAN_LIBS)
