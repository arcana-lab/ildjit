optimizer_simplemethodcloning_LTLIBRARIES = optimizer_simplemethodcloning.la
optimizer_simplemethodcloningdir = $(libdir)/iljit/optimizers

AM_CFLAGS= -Wall $(ILJITU_CFLAGS) $(XAN_CFLAGS) $(ILJITIR_CFLAGS) $(ILJITIROPTIMIZER_CFLAGS)

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

optimizer_simplemethodcloning_la_SOURCES=							\
		optimizer_simplemethodcloning.c		optimizer_simplemethodcloning.h

optimizer_simplemethodcloning_la_LDFLAGS= -module -avoid_version 

optimizer_simplemethodcloning_la_LIBADD = $(ILJITU_LIBS) $(XAN_LIBS) $(ILJITIROPTIMIZER_LIBS) $(ILJITIR_LIBS)

MAINTAINERCLEANFILES = Makefile.in config.h.in

