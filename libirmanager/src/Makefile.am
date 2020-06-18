MAINTAINERCLEANFILES = Makefile.in config.h.in

libirmanager_LTLIBRARIES = libirmanager.la
libirmanagerdir = $(libdir)/iljit

AM_CFLAGS= -std=c99 -Wall -I../include $(XAN_CFLAGS) $(ILJITU_CFLAGS) $(ILJITMM_CFLAGS) $(COMPILERMEMORYMANAGER_CFLAGS) $(PLATFORM_CFLAGS) -D_GNU_SOURCE

if PROFILE
AM_CFLAGS 	+= -DNDEBUG -DPROFILE -O0 -g
else
if PRINTDEBUG
AM_CFLAGS	+= -ggdb -DDEBUG -DPRINTDEBUG -O0
else 
if DEBUG
AM_CFLAGS 	+= -DDEBUG -O0 -ggdb3 -fstack-protector-all
else 
AM_CFLAGS	+= -O3 -DNDEBUG 
endif
endif
endif

if HEAVY_DEBUG
AM_CFLAGS	+= -DHEAVY_DEBUG
endif

libirmanager_la_SOURCES=			 						\
			ir_lib.c								\
			ir_method.c				ir_method.h			\
			ir_symbol.c				ir_symbol.h			\
			ir_keyword.c				ir_keyword.h			\
			ildjit.c				ildjit.h			\
			ir_data.c				ir_data.h			\
			ir_loop.c								\
			ir_dataflow.c								\
			ir_internal_functions.h							\
			ir_program.c								\
			ir_global.c								\
			codetool_types.h	

EXTRA_DIST = ir_keyword.gperf

CLEANFILES = ir_keyword.c ir_keyword.h

libirmanager_la_LIBADD= $(ILJITU_LIBS) $(ILJITMM_LIBS) $(XAN_LIBS) $(COMPILERMEMORYMANAGER_LIBS) $(PLATFORM_LIBS)

libirmanager_la_LDFLAGS= -fPIC 

ir_keyword.c ir_keyword.h: ir_keyword.gperf
	$(GPERF) --output-file=$@ $<
