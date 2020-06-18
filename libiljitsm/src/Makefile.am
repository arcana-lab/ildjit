
MAINTAINERCLEANFILES = Makefile.in config.h.in

libiljitsm_LTLIBRARIES = libiljitsm.la
libiljitsmdir = $(libdir)/iljit

AM_CFLAGS= -Wall -Wextra $(ILJITU_CFLAGS) $(XAN_CFLAGS) $(COMPILERMEMORYMANAGER_CFLAGS) $(PLATFORM_CFLAGS)
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

libiljitsm_la_SOURCES=			 						\
		section_manager.c			section_manager.h		\
		iljitsm-system.h	

libiljitsm_la_LDFLAGS= -fPIC -no-undefined
libiljitsm_la_LIBADD = $(XAN_LIBS) $(ILJITU_LIBS) $(COMPILERMEMORYMANAGER_LIBS) $(PLATFORM_LIBS)
if CYGWIN
  libiljitsm_la_LDFLAGS += -Wl,--enable-auto-import
endif
