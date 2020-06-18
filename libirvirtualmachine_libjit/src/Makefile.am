MAINTAINERCLEANFILES = Makefile.in config.h.in

libirvirtualmachine_libjit_LTLIBRARIES = libirvirtualmachine_libjit.la
libirvirtualmachine_libjitdir = $(libdir)/iljit

# The libjit library, compiled into iljit for increasing performance
SUBDIRS	= libjit

AM_CFLAGS 	= 	-Wall 				\
			$(XANLIB_CFLAGS) 		\
			$(COMPILERMEMORYMANAGER_CFLAGS) \
			$(LIBILJITU_CFLAGS) 		\
			$(LIBILJITIR_CFLAGS) 		\
			$(LIBILJITIROPTIMIZER_CFLAGS) 	\
			$(JIT_CFLAGS)			\
			$(PLATFORM_CFLAGS)		\
			$(ILJITGC_CFLAGS)		\
			$(IRVIRTUALMACHINE_CFLAGS)	\
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

libirvirtualmachine_libjit_la_SOURCES=					\
		ir_virtual_machine.c					\
		ir_virtual_machine_backend.h				\
		ir_virtual_machine_system.h

libirvirtualmachine_libjit_la_LDFLAGS= -fPIC
libirvirtualmachine_libjit_la_LIBADD	= 				\
					$(XANLIB_LIBS) 			\
					$(COMPILERMEMORYMANAGER_LIBS) 	\
					$(LIBILJITU_LIBS)		\
			 		$(LIBILJITIR_LIBS) 		\
					$(LIBILJITIRPROFILER_LIBS)	\
					$(JIT_LIBS)			\
					$(ILJITIROPTIMIZER_LIBS)	\
					$(ILJITGC_LIBS)			\
					$(PLATFORM_LIBS)		\
					$(IRVIRTUALMACHINE_LIBS)

maintainer-clean-local:
	rm -rf libjit
