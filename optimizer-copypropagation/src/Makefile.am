optimizer_copypropagation_LTLIBRARIES = optimizer_copypropagation.la
optimizer_copypropagationdir = $(libdir)/iljit/optimizers

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

optimizer_copypropagation_la_SOURCES=							    \
        cyclic_moves.c                  cyclic_moves.h              \
		copy_propagation_patterns.c		copy_propagation_patterns.h	\
		copy_propagation_front.c		copy_propagation_front.h	\
		copy_propagation.c			    copy_propagation.h

optimizer_copypropagation_la_LDFLAGS= -module -avoid_version 

optimizer_copypropagation_la_LIBADD = $(ILJITU_LIBS) $(XAN_LIBS) $(ILJITIROPTIMIZER_LIBS) $(ILJITIR_LIBS)

MAINTAINERCLEANFILES = Makefile.in config.h.in
