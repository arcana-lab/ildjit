MAINTAINERCLEANFILES = Makefile.in config.h.in

libclimanager_LTLIBRARIES = libclimanager.la
libclimanagerdir = $(libdir)/iljit

AM_CFLAGS= -Wall -I../include $(XAN_CFLAGS) $(ILJITU_CFLAGS) $(ILJITSM_CFLAGS) $(ILJITMM_CFLAGS) $(LIBILJITIR_CFLAGS) $(IRVIRTUALMACHINE_CFLAGS) $(PLATFORM_CFLAGS) -D_GNU_SOURCE

if PROFILE
AM_CFLAGS 	+= -DNDEBUG -DPROFILE -O0 -g
else
if PRINTDEBUG
AM_CFLAGS	+= -ggdb -DDEBUG -DPRINTDEBUG -O0
else 
if DEBUG
AM_CFLAGS	+= -ggdb -DDEBUG -O0
else 
AM_CFLAGS	+= -O3 -DNDEBUG 
endif
endif
endif

libclimanager_la_SOURCES=									\
		ilmethod.c				ilmethod.h				\
		cil_stack.c				cil_stack.h				\
		lib_pinvoke.c				lib_pinvoke.h				\
		lib_delegates.c				lib_delegates.h				\
		lib_thread.c				lib_thread.h				\
		lib_string.c				lib_string.h				\
		lib_stringBuilder.c			lib_stringBuilder.h			\
		lib_array.c				lib_array.h				\
		lib_runtimehandle.c			lib_runtimehandle.h			\
		lib_typedReference.c			lib_typedReference.h			\
		lib_vararg.c				lib_vararg.h				\
		lib_decimal.c				lib_decimal.h				\
		lib_reflect.c				lib_reflect.h				\
		lib_culture.c				lib_culture.h				\
		lib_net.c                               lib_net.h   				\
		cli_manager.c				cli_manager.h				\
		layout_manager.c			layout_manager.h			\
		climanager_system.h

libclimanager_la_LIBADD	= $(ILJITU_LIBS) $(ILJITSM_LIBS) $(ILJITMM_LIBS) $(XAN_LIBS) $(LIBILJITIR_LIBS) $(IRVIRTUALMACHINE_LIBS) $(ILJITIROPTIMIZER_LIBS)  $(PLATFORM_LIBS)

libclimanager_la_LDFLAGS= -fPIC
