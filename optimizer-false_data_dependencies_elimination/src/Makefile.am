optimizer_false_data_dependences_elimination_LTLIBRARIES = optimizer_false_data_dependences_elimination.la
optimizer_false_data_dependences_eliminationdir = $(libdir)/iljit/optimizers

AM_CFLAGS=$(ILJITU_CFLAGS) $(XAN_CFLAGS) $(ILJITIR_CFLAGS) $(ILJITIROPTIMIZER_CFLAGS)

if PROFILE
AM_CFLAGS 	+= -DNDEBUG -DPROFILE -ggdb3 -O2
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

optimizer_false_data_dependences_elimination_la_SOURCES=							\
		optimizer_false_data_dependences_elimination.c		optimizer_false_data_dependences_elimination.h

optimizer_false_data_dependences_elimination_la_LDFLAGS= -module -avoid_version $(ILJITU_LIBS) $(XAN_LIBS) $(ILJITIROPTIMIZER_LIBS) $(ILJITIR_LIBS)
