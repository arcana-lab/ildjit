optimizer_methodinliner_LTLIBRARIES = optimizer_methodinliner.la
optimizer_methodinlinerdir = $(libdir)/iljit/optimizers

AM_CFLAGS= -Wall -std=gnu99 -D_GNU_SOURCE		\
	-Wno-unused-function			 	\
	$(ILJITU_CFLAGS) 				\
	$(XAN_CFLAGS) 					\
	$(ILJITIR_CFLAGS) 				\
	$(ILJITIROPTIMIZER_CFLAGS) 			\
	$(CHIARA_CFLAGS)

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

optimizer_methodinliner_la_SOURCES=							\
		optimizer_methodinliner.c		optimizer_methodinliner.h	\
		inline_algorithm.c			inline_algorithm.h		\
		inline_heuristics.c			inline_heuristics.h		\
		inline_printer.c			inline_printer.h		\
		inliner_configuration.h

optimizer_methodinliner_la_LDFLAGS= -module -avoid_version

optimizer_methodinliner_la_LIBADD = $(ILJITU_LIBS) $(XAN_LIBS) $(ILJITIROPTIMIZER_LIBS) $(ILJITIR_LIBS) $(CHIARA_LIBS)

MAINTAINERCLEANFILES = Makefile.in config.h.in
