MAINTAINERCLEANFILES = Makefile.in config.h.in

libirvirtualmachine_LTLIBRARIES = libirvirtualmachine.la
libirvirtualmachinedir = $(libdir)/iljit

AM_CFLAGS 	= 	-Wall 				\
			$(XANLIB_CFLAGS) 		\
			$(COMPILERMEMORYMANAGER_CFLAGS) \
			$(LIBILJITU_CFLAGS) 		\
			$(LIBILJITIR_CFLAGS) 		\
			$(LIBILJITIROPTIMIZER_CFLAGS) 	\
			$(PLATFORM_CFLAGS)		\
			$(ILJITGC_CFLAGS)		\
			-D_GNU_SOURCE

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

libirvirtualmachine_la_SOURCES=						\
		ir_virtual_machine.h

libirvirtualmachine_la_LDFLAGS= -fPIC
libirvirtualmachine_la_LIBADD	= 					\
					$(XANLIB_LIBS) 			\
					$(COMPILERMEMORYMANAGER_LIBS) 	\
					$(LIBILJITU_LIBS)		\
			 		$(LIBILJITIR_LIBS) 		\
					$(LIBILJITIRPROFILER_LIBS)	\
					$(ILJITIROPTIMIZER_LIBS)	\
					$(ILJITGC_LIBS)			\
					$(PLATFORM_LIBS)
