iljit_gc_bdw_LTLIBRARIES = iljit_gc_bdw.la
iljit_gc_bdwdir = $(libdir)/iljit/gc

AM_CFLAGS=-Wall -shared $(ILJITES_ILJITU_CFLAGS) $(ILJIT_CFLAGS) $(XAN_CFLAGS) $(BDW_CFLAGS) -DGC_THREADS
if PROFILE
AM_CFLAGS 	+= -DNDEBUG -DPROFILE -O0 -g
else 
if PRINTDEBUG
AM_CFLAGS 	+= -DPRINTDEBUG -DDEBUG -O0 -ggdb3
else
if DEBUG
AM_CFLAGS 	+= -DDEBUG -O0 -ggdb3
else
AM_CFLAGS 	+= -O3 -DNDEBUG 
endif
endif
endif

iljit_gc_bdw_la_SOURCES=	 						\
		iljit_gc_bdw.c			iljit_gc_bdw.h			\
		system.h

iljit_gc_bdw_la_LDFLAGS= -module -avoid-version -no-undefined
if CYGWIN
  iljit_gc_bdw_la_LDFLAGS += -Wl,--enable-auto-import
endif
iljit_gc_bdw_la_LIBADD= $(ILJITES_ILJITU_LIBS) $(ILJIT_LIBS) $(XAN_LIBS) $(BDW_LIBS) -lpthread -lgc
