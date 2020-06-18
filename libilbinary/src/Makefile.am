# The iljit binary is the compilation framework
libilbinary_LTLIBRARIES= libilbinary.la
libilbinarydir = $(libdir)/iljit

# Default compiler flags
AM_CFLAGS =										\
		$(COMPILERMEMORYMANAGER_CFLAGS)	  	$(XAN_CFLAGS)			\
		$(ILJIT_ILJITU_CFLAGS)			$(ILJIT_ILJITSM_CFLAGS)		\
		$(ILJIT_ILJITMM_CFLAGS)	 		$(PLATFORM_CFLAGS)		\
		$(LIBPLUGINMANAGER_CFLAGS)

if PROFILE
AM_CFLAGS 	+= -DNDEBUG -DPROFILE -O0 -g
else
if PRINTDEBUG
AM_CFLAGS	+= -ggdb -DDEBUG -DPRINTDEBUG -O0
else 
if DEBUG
AM_CFLAGS 	+= -DDEBUG -O0 -g
else 
AM_CFLAGS	+= -O3 -DNDEBUG 
endif
endif
endif

# Source files
libilbinary_la_SOURCES =										\
		ilbinary.c				ilbinary.h

# Libraries needed by iljit
libilbinary_la_LIBADD =									\
		$(COMPILERMEMORYMANAGER_LIBS)	$(XAN_LIBS)				\
		$(ILJIT_ILJITU_LIBS)		$(ILJIT_ILJITSM_LIBS)			\
		$(ILJIT_ILJITMM_LIBS)		$(PLATFORM_LIBS)			\
		$(LIBPLUGINMANAGER_LIBS)

libilbinary_la_LDFLAGS = -fPIC -no-undefined

MAINTAINERCLEANFILES = Makefile.in config.h.in
