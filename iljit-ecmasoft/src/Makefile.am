iljit_ecmasoft_decoder_LTLIBRARIES = iljit_ecmasoft_decoder.la
iljit_ecmasoft_decoderdir = $(libdir)/iljit/loaders

AM_CFLAGS= 	-Wall 									\
		$(ILJITES_ILJITU_CFLAGS) 	$(ILJITES_ILJITSM_CFLAGS) 		\
		$(ILJITES_ILJITMM_CFLAGS) 	$(XAN_CFLAGS) 				\
		$(COMPILERMEMORYMANAGER_CFLAGS)

if PROFILE
AM_CFLAGS 	+= -DNDEBUG -DPROFILE -O0 -g
else 
if PRINTDEBUG
AM_CFLAGS 	+= -DPRINTDEBUG -DDEBUG -O0 -ggdb3
else
if DEBUG
AM_CFLAGS 	+= -DDEBUG -O0 -ggdb3
else
AM_CFLAGS 	+= -O3 -DNDEBUG 
endif
endif
endif

iljit_ecmasoft_decoder_la_SOURCES=							\
		iljit_ecmasoft_decoder.c		iljit_ecmasoft_decoder.h

iljit_ecmasoft_decoder_la_LDFLAGS= -module -avoid-version

iljit_ecmasoft_decoder_la_LIBADD= $(ILJITES_ILJITU_LIBS) $(ILJITES_ILJITSM_LIBS) $(ILJITES_ILJITMM_LIBS) $(XAN_LIBS) $(COMPILERMEMORYMANAGER_LIBS)

MAINTAINERCLEANFILES = Makefile.in config.h.in
