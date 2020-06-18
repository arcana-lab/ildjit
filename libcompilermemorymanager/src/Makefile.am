MAINTAINERCLEANFILES = config.h.in Makefile.in

libcompilermemorymanager_LTLIBRARIES = libcompilermemorymanager.la
libcompilermemorymanagerdir = $(libdir)/iljit

# Compile internal garbage collector only if enabled
#if INTERNAL_GC
#  MAYBE_INTERNAL_GC = libgc
#endif

#SUBDIRS = $(MAYBE_INTERNAL_GC)

# Default compiler flags
AM_CFLAGS=  -Wall -I../include $(XAN_CFLAGS) $(PLATFORM_CFLAGS)	

if INTERNAL_GC
  AM_CFLAGS += -DINTERNAL_GC
endif

if MULTIAPP
  AM_CFLAGS += -DMULTIAPP
endif

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

# Tell make where to found internal garbage collector library, if enabled
if INTERNAL_GC
AM_CFLAGS += $(GC_CFLAGS)
endif

libcompilermemorymanager_la_SOURCES=							\
		compiler_memory_manager.c	 	compiler_memory_manager.h	\
		private_memory_pool.c			private_memory_pool.h		\
		shared_memory_pool.c			shared_memory_pool.h		\
		compiler_memory_manager_system.h					\
		private_memory_wrapper.c						\
		shared_memory_wrapper.c

libcompilermemorymanager_la_LIBADD	= $(XAN_LIBS) $(PLATFORM_LIBS)

libcompilermemorymanager_la_LDFLAGS= -fPIC -no-undefined

# Windows porting
if CYGWIN
  libcompilermemorymanager_la_LDFLAGS += -Wl,--enable-auto-import
endif

# Add internal garbage collector library, if needed
if INTERNAL_GC
libcompilermemorymanager_la_LIBADD += $(GC_LIBS)
endif 

maintainer-clean-local:
	rm -rf libgc
