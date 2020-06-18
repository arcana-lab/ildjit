clrbase_LTLIBRARIES = clrbase.la
clrbasedir = $(libdir)/iljit/clrs

AM_CFLAGS=-Wall -Wno-unused-function -std=gnu99 -D_GNU_SOURCE $(XANLIB_CFLAGS) $(LIBILJITU_CFLAGS) $(LIBILJITIR_CFLAGS) $(LIBILJITIROPTIMIZER_CFLAGS) $(IRVIRTUALMACHINE_CFLAGS) $(CLIMANAGER_CFLAGS) $(PLATFORM_CFLAGS) $(ILJIT_CFLAGS)

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

clrbase_la_SOURCES=										\
		clr.c										\
		lib_internalCall.c			lib_internalCall.h			\
		internal_calls_utilities.c		internal_calls_utilities.h		\
		internal_calls_misc.c			internal_calls_misc.h			\
		internal_calls_array.c			internal_calls_array.h			\
                internal_calls_string.c                 internal_calls_string.h                 \
                internal_calls_math.c                   internal_calls_math.h                   \
                internal_calls_environment.c            internal_calls_environment.h            \
                internal_calls_reflection.c             internal_calls_reflection.h             \
                internal_calls_IO.c                     internal_calls_IO.h                     \
                internal_calls_decimal.c                internal_calls_decimal.h                \
                internal_calls_delegate.c               internal_calls_delegate.h               \
		internal_calls_thread.c			internal_calls_thread.h			\
		internal_calls_charinfo.c		internal_calls_charinfo.h		\
		internal_calls_enum.c			internal_calls_enum.h			\
		internal_calls_runtime.c		internal_calls_runtime.h		\
		internal_calls_vararg.c			internal_calls_vararg.h			\
		internal_calls_net.c			internal_calls_net.h			\
		internal_calls_object.c			internal_calls_object.h			\
		internal_calls_numberformatter.c	internal_calls_numberformatter.h	\
		internal_calls_gc.c			internal_calls_gc.h			\
		internal_calls_security.c		internal_calls_security.h		\
		internal_calls_datetime.c		internal_calls_datetime.h		\
		internal_calls_currentsystemtimezone.c	internal_calls_currentsystemtimezone.h	\
		internal_calls_char.c			internal_calls_char.h			\
		clr_system.h

clrbase_la_LDFLAGS= -module -avoid_version
clrbase_la_LIBADD= $(XANLIB_LIBS) $(LIBILJITU_LIBS) $(LIBILJITIR_LIBS) $(LIBILJITIRPROFILER_LIBS) $(IRVIRTUALMACHINE_LIBS) $(CLIMANAGER_LIBS) $(PLATFORM_LIBS) $(ILJIT_LIBS)

EXTRA_DIST =                                    \
		lib_internalCall.gperf

CLEANFILES =                                    \
		lib_internalCall.c

lib_internalCall.c: lib_internalCall.gperf
	$(GPERF) --output-file=$@ $<

MAINTAINERCLEANFILES = Makefile.in config.h.in
