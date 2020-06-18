optimizer_wholetrace_LTLIBRARIES = optimizer_wholetrace.la
optimizer_wholetracedir = $(libdir)/iljit/optimizers

AM_CFLAGS= -Wall -std=gnu99 -D_GNU_SOURCE		\
		$(ILJITU_CFLAGS) 			\
		$(XAN_CFLAGS) 				\
		$(ILJITIR_CFLAGS) 			\
		$(ILJITIROPTIMIZER_CFLAGS)

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

optimizer_wholetrace_la_SOURCES=							\
		optimizer_wholetrace.c		optimizer_wholetrace.h

optimizer_wholetrace_la_LDFLAGS= -module -avoid_version

optimizer_wholetrace_la_LIBADD = $(ILJITU_LIBS) $(XAN_LIBS) $(ILJITIROPTIMIZER_LIBS) $(ILJITIR_LIBS) -lz


MAINTAINERCLEANFILES = Makefile.in config.h.in
