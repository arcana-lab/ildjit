## Automake file to produce wss program
optimizer_loopinvariantsidentification_LTLIBRARIES = optimizer_loopinvariantsidentification.la
optimizer_loopinvariantsidentificationdir = $(libdir)/iljit/optimizers

AM_CFLAGS= -Wall -std=gnu99 -D_GNU_SOURCE		\
	$(ILJITU_CFLAGS) $(XAN_CFLAGS) $(ILJITIR_CFLAGS) $(ILJITIROPTIMIZER_CFLAGS) $(COMPILERMEMORYMANAGER_CFLAGS)

if PROFILE
AM_CFLAGS 	+= -DNDEBUG -DPROFILE -g -O0
else 
if PRINTDEBUG
AM_CFLAGS 	+= -ggdb -DPRINTDEBUG -DDEBUG -O0
else
if DEBUG
AM_CFLAGS	+= -ggdb -DDEBUG -O0
else
if OPTDEBUG
AM_CFLAGS	+= -ggdb -DDEBUG -O3
else
AM_CFLAGS	+= -DNDEBUG -O3
endif
endif
endif
endif

optimizer_loopinvariantsidentification_la_SOURCES=						\
	optimizer_loopinvariantsidentification.c	optimizer_loopinvariantsidentification.h

optimizer_loopinvariantsidentification_la_LDFLAGS= -module -avoid_version

optimizer_loopinvariantsidentification_la_LIBADD = $(ILJITU_LIBS) $(XAN_LIBS) $(ILJITIR_LIBS) $(ILJITIROPTIMIZER_LIBS) $(COMPILERMEMORYMANAGER_LIBS)

MAINTAINERCLEANFILES = Makefile.in config.h.in
