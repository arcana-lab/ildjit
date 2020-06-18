optimizer_postdominators_LTLIBRARIES = optimizer_postdominators.la
optimizer_postdominatorsdir = $(libdir)/iljit/optimizers

AM_CFLAGS= -Wall -std=gnu99 -D_GNU_SOURCE		\
	$(ILJITU_CFLAGS) $(XAN_CFLAGS) $(ILJITIR_CFLAGS) $(ILJITIROPTIMIZER_CFLAGS) $(COMPILERMEMORYMANAGER_CFLAGS) $(DOMINANCE_CFLAGS)

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

optimizer_postdominators_la_SOURCES=					\
		post_dominators.c		post_dominators.h

optimizer_postdominators_la_LDFLAGS= -module -avoid_version

optimizer_postdominators_la_LIBADD = $(ILJITU_LIBS) $(XAN_LIBS) $(ILJITIR_LIBS) $(ILJITIROPTIMIZER_LIBS) $(COMPILERMEMORYMANAGER_LIBS) $(DOMINANCE_LIBS)

MAINTAINERCLEANFILES = Makefile.in config.h.in
