optimizer_scalarization_LTLIBRARIES = optimizer_scalarization.la
optimizer_scalarizationdir = $(libdir)/iljit/optimizers

AM_CFLAGS= -Wall -std=gnu99				\
		$(ILJITU_CFLAGS) 			\
		$(XAN_CFLAGS) 				\
		$(CHIARA_CFLAGS)			\
		$(ILJITIR_CFLAGS) 			\
		$(ILJITIROPTIMIZER_CFLAGS)

if PROFILE
AM_CFLAGS 	+= -DNDEBUG -DPROFILE -g -O0
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

if PRINTDEBUG
AM_CFLAGS 	+= -DPRINTDEBUG
endif

optimizer_scalarization_la_SOURCES=							\
		optimizer_scalarization.c		optimizer_scalarization.h

optimizer_scalarization_la_LDFLAGS= -module -avoid_version 

optimizer_scalarization_la_LIBADD = $(ILJITU_LIBS) $(XAN_LIBS) $(CHIARA_LIBS) $(ILJITIROPTIMIZER_LIBS) $(ILJITIR_LIBS)

MAINTAINERCLEANFILES = Makefile.in config.h.in
