## Automake file to produce wss program
optimizer_cdg_LTLIBRARIES = optimizer_cdg.la
optimizer_cdgdir = $(libdir)/iljit/optimizers

AM_CFLAGS= -Wall  $(ILJITU_CFLAGS) $(XAN_CFLAGS) $(ILJITIR_CFLAGS) $(ILJITIROPTIMIZER_CFLAGS)

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

optimizer_cdg_la_SOURCES=						\
		optimizer_cdg.c			optimizer_cdg.h

optimizer_cdg_la_LDFLAGS= -module -avoid_version $(ILJITU_LIBS) $(XAN_LIBS) $(ILJITIR_LIBS) $(ILJITIROPTIMIZER_LIBS)
