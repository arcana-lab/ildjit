optimizer_dowhile_to_while_LTLIBRARIES = optimizer-dowhile_to_while.la
optimizer_dowhile_to_whiledir = $(libdir)/iljit/optimizers

AM_CFLAGS=$(ILJITU_CFLAGS) $(XAN_CFLAGS) $(ILJITIR_CFLAGS) $(ILJITIROPTIMIZER_CFLAGS)

if PROFILE
AM_CFLAGS 	+= -DNDEBUG -DPROFILE -g -O0
else 
if SUPERDEBUG
AM_CFLAGS       += -ggdb -DSUPERDEBUG -DPRINTDEBUG -DDEBUG -O0
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
endif

optimizer_dowhile_to_while_la_SOURCES = whilifier.c		whilifier.h \
					pretty_print.c		pretty_print.h \
					bits.c			bits.h \
								implementation.h

optimizer_dowhile_to_while_la_LDFLAGS = -module -avoid_version $(ILJITU_LIBS) $(XAN_LIBS) $(ILJITIROPTIMIZER_LIBS) $(ILJITIR_LIBS)
