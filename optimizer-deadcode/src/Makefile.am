optimizer_deadcode_LTLIBRARIES = optimizer_deadcode.la
optimizer_deadcodedir = $(libdir)/iljit/optimizers

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

optimizer_deadcode_la_SOURCES=									\
		instructions_cleaning.c		instructions_cleaning.h				\
		optimizer_deadcode.c		optimizer_deadcode.h				\
		method_deadcode.c		method_deadcode.h				\
		program_deadcode.c		program_deadcode.h

optimizer_deadcode_la_LDFLAGS= -module -avoid_version

optimizer_deadcode_la_LIBADD = $(ILJITU_LIBS) $(XAN_LIBS) $(ILJITIR_LIBS) $(ILJITIROPTIMIZER_LIBS) $(COMPILERMEMORYMANAGER_LIBS)

MAINTAINERCLEANFILES = Makefile.in config.h.in
