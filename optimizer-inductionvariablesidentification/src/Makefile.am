optimizer_inductionvariablesidentification_LTLIBRARIES = optimizer_inductionvariablesidentification.la
optimizer_inductionvariablesidentificationdir = $(libdir)/iljit/optimizers

AM_CFLAGS=-Wall $(ILJITU_CFLAGS) $(XAN_CFLAGS) $(ILJITIR_CFLAGS) $(ILJITIROPTIMIZER_CFLAGS) $(COMPILERMEMORYMANAGER_CFLAGS)

if PROFILE
AM_CFLAGS 	+= -DNDEBUG -DPROFILE -g -O0
else 
if PRINTDEBUG
AM_CFLAGS 	+= -ggdb -DPRINTDEBUG -DDEBUG -O0
else
if DEBUG
AM_CFLAGS	+= -ggdb3 -DDEBUG -O0
else
AM_CFLAGS	+= -DNDEBUG -O3
endif
endif
endif

optimizer_inductionvariablesidentification_la_SOURCES=			\
		optimizer_inductionvariablesidentification.c		\
		optimizer_inductionvariablesidentification.h

optimizer_inductionvariablesidentification_la_LDFLAGS= -module -avoid_version $(ILJITU_LIBS) $(XAN_LIBS) $(ILJITIROPTIMIZER_LIBS) $(ILJITIR_LIBS) $(COMPILERMEMORYMANAGER_LIBS)

MAINTAINERCLEANFILES = Makefile.in config.h.in

