MAINTAINERCLEANFILES = Makefile.in config.h.in

libmanfred_LTLIBRARIES = libmanfred.la
libmanfreddir = $(libdir)/iljit

AM_CFLAGS=-Wall $(XANLIB_CFLAGS) $(LIBILJITU_CFLAGS) $(LIBILJITIR_CFLAGS) $(LIBILJITIROPTIMIZER_CFLAGS) $(IRVIRTUALMACHINE_CFLAGS) $(CLIMANAGER_CFLAGS) $(PLATFORM_CFLAGS)
if CYGWIN
AM_CFLAGS += -shared
endif
if PROFILE
AM_CFLAGS	+= -g -DNDEBUG -O0 -DPROFILE
else
if PRINTDEBUG
AM_CFLAGS	+= -ggdb -DDEBUG -DPRINTDEBUG -O0
else 
if DEBUG
AM_CFLAGS	+= -g -DDEBUG -O0
else 
AM_CFLAGS	+= -O3 -DNDEBUG 
endif
endif
endif

libmanfred_la_SOURCES=										\
		manfred_parser.tab.h			manfred_parser.tab.c 			\
                manfred_scanner.h			manfred_scanner.c	 		\
                manfred.h                               manfred.c				\
		manfred_load.h									\
		manfred_system.h

libmanfred_la_LDFLAGS= -fPIC
libmanfred_la_LIBADD= $(XANLIB_LIBS) $(LIBILJITU_LIBS) $(LIBILJITIR_LIBS) $(LIBILJITIRPROFILER_LIBS) $(IRVIRTUALMACHINE_LIBS) $(CLIMANAGER_LIBS) $(PLATFORM_LIBS)

EXTRA_DIST =                                    \
                manfred_parser.y                \
                manfred_scanner.lex

CLEANFILES =                                    \
		manfred_parser.tab.c            \
		manfred_parser.tab.h            \
		manfred_scanner.c               \
		manfred_scanner.h		\
		manfred_parser.output              

manfred_parser.tab.c manfred_parser.tab.h: manfred_parser.y
	$(YACC) -d -r all -b manfred_parser --name-prefix manfred_yy $<

manfred_scanner.c manfred_scanner.h: manfred_scanner.lex manfred_parser.tab.h
	$(LEX) --header-file=manfred_scanner.h -o manfred_scanner.c --prefix manfred_yy $<

manfred.c: manfred_parser.tab.h manfred_parser.tab.h
