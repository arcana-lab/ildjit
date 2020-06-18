## Automake file to produce wss program
optimizer_basicblockidentifier_LTLIBRARIES = optimizer_basicblockidentifier.la
optimizer_basicblockidentifierdir = $(libdir)/iljit/optimizers

AM_CFLAGS=$(ILJITU_CFLAGS) $(XAN_CFLAGS) $(ILJITIR_CFLAGS) $(ILJITIROPTIMIZER_CFLAGS)

if PROFILE
AM_CFLAGS 	+= -DNDEBUG -DPROFILE -ggdb3 -O2
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

optimizer_basicblockidentifier_la_SOURCES=								\
		optimizer_basicblockidentifier.c		optimizer_basicblockidentifier.h

optimizer_basicblockidentifier_la_LDFLAGS= -module -avoid_version 

optimizer_basicblockidentifier_la_LIBADD = $(ILJITU_LIBS) $(XAN_LIBS) $(ILJITIROPTIMIZER_LIBS) $(ILJITIR_LIBS)

MAINTAINERCLEANFILES = Makefile.in config.h.in

