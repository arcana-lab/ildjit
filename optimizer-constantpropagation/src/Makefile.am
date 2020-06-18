optimizer_constantpropagation_LTLIBRARIES = optimizer_constantpropagation.la
optimizer_constantpropagationdir = $(libdir)/iljit/optimizers

AM_CFLAGS= -Wall -std=gnu99 -D_GNU_SOURCE		\
		$(ILJITU_CFLAGS) $(XAN_CFLAGS) $(ILJITIR_CFLAGS) $(ILJITIROPTIMIZER_CFLAGS)

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

optimizer_constantpropagation_la_SOURCES=							\
		constant_propagation_codetool.c		constant_propagation_codetool.h		\
		constant_propagation.c			constant_propagation.h			\
		constant_folding.c			constant_folding.h			\
		misc.c					misc.h

optimizer_constantpropagation_la_LDFLAGS= -module -avoid_version

optimizer_constantpropagation_la_LIBADD = $(ILJITU_LIBS) $(XAN_LIBS) $(ILJITIROPTIMIZER_LIBS) $(ILJITIR_LIBS)

MAINTAINERCLEANFILES = Makefile.in config.h.in
