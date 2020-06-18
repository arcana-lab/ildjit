MAINTAINERCLEANFILES = Makefile.in config.h.in

libiljitu_LTLIBRARIES = libiljitu.la
libiljitudir = $(libdir)/iljit
AM_CFLAGS= -I../include -Wall -Wextra $(XAN_CFLAGS) $(COMPILERMEMORYMANAGER_CFLAGS) $(PLATFORM_CFLAGS) $(PLATFORM_CFLAGS) -D_GNU_SOURCE

if CYGWIN
  AM_CFLAGS += -shared
endif

if PROFILE
AM_CFLAGS 	+= -DNDEBUG -DPROFILE -O0 -g
else
if PRINTDEBUG
AM_CFLAGS+= -g -DDEBUG -DPRINTDEBUG -O0
else 
if DEBUG
AM_CFLAGS += -g -DDEBUG -O0
else 
AM_CFLAGS+= -O3 -DNDEBUG
endif
endif
endif

libiljitu_la_SOURCES=			 				\
		iljit-utils.c			iljit-utils.h		\
		small_unicode.c			small_unicode.h		\
		ildjit_locale.c			ildjit_locale.h		\
		gc_root_sets.c			gc_root_sets.h		\
		iljitu-system.h						\
		ir_language.h						\
		cil_opcodes.h						\
		unicat.h						\
		error_codes.h						\
		jit_metadata.h						\
		base_symbol.h						\
		ecma_constant.h	

libiljitu_la_LDFLAGS=-fPIC -no-undefined
if CYGWIN
  libiljitu_la_LDFLAGS += -Wl,--enable-auto-import
endif

libiljitu_la_LIBADD = -L$(libdir) -L/usr/lib $(XAN_LIBS) $(COMPILERMEMORYMANAGER_LIBS) $(PLATFORM_LIBS) $(PLATFORM_LIBS)
if CLOCKGETTIME
if NEEDRT 
libiljitu_la_LIBADD	+= -lrt
endif
endif
