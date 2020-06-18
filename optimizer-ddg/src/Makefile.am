optimizer_ddg_LTLIBRARIES = optimizer_ddg.la
optimizer_ddgdir = $(libdir)/iljit/optimizers

AM_CFLAGS=$(ILJITU_CFLAGS) $(XAN_CFLAGS) $(ILJITIR_CFLAGS) $(ILJITIROPTIMIZER_CFLAGS)

if PROFILE
AM_CFLAGS 	+= -DNDEBUG -DPROFILE -g -O0
else 
if PRINTDEBUG
AM_CFLAGS 	+= -ggdb -DPRINTDEBUG -DDEBUG -O0
else
if DEBUG
AM_CFLAGS	+= -ggdb -DDEBUG -O0
else
AM_CFLAGS	+= -DNDEBUG -O3
endif
endif
endif

optimizer_ddg_la_SOURCES=				\
		optimizer_ddg.h		optimizer_ddg.c	

optimizer_ddg_la_LDFLAGS= -module -avoid_version $(ILJITU_LIBS) $(XAN_LIBS) $(ILJITIROPTIMIZER_LIBS) $(ILJITIR_LIBS)
