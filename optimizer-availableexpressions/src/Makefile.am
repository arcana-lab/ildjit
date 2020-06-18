optimizer_availableexpressions_LTLIBRARIES = optimizer_availableexpressions.la
optimizer_availableexpressionsdir = $(libdir)/iljit/optimizers

AM_CFLAGS=$(ILJITU_CFLAGS) $(XAN_CFLAGS) $(ILJITIR_CFLAGS) $(ILJITIROPTIMIZER_CFLAGS)

if PROFILE
AM_CFLAGS 	+= -DNDEBUG -DPROFILE -g -O0
else 
if DEBUG
AM_CFLAGS	+= -ggdb -DDEBUG -O0
else
AM_CFLAGS	+= -DNDEBUG -O3
endif
endif

if PRINTDEBUG
AM_CFLAGS 	+= -DPRINTDEBUG
endif

optimizer_availableexpressions_la_SOURCES=							\
		optimizer_availableexpressions.c		optimizer_availableexpressions.h

optimizer_availableexpressions_la_LDFLAGS= -module -avoid_version $(ILJITU_LIBS) $(XAN_LIBS) $(ILJITIROPTIMIZER_LIBS) $(ILJITIR_LIBS)

MAINTAINERCLEANFILES = Makefile.in config.h.in

