
# Automake config file for building iljit-gc-copy garbage collector

# Setting include flags
AM_CFLAGS = $(ILJIT_CFLAGS) $(LIBILJITU_CFLAGS) $(XAN_CFLAGS)

# Define DEBUG symbol if user have selected that
if DEBUG
  AM_CFLAGS += -ggdb -Wall -DDEBUG
endif

# Where to install library (Is this the right place where set this variable?)
gclibdir = $(libdir)/iljit/gc

# Define garbage collector as a libtool library
# Library is iljit_gc_copy.la
# It is a libtool library (_LTLIBRARIES)
# Install it in $(gclib) path (gclib_)
gclib_LTLIBRARIES = iljit_gc_copy.la

# libiljit_gc_copy.la is used only for testing
noinst_LTLIBRARIES = libiljit_gc_copy.la

iljit_gc_copy_la_SOURCES = semi_space.c semi_space.h \
			   gc_memory.c gc_memory.h \
                           gc_copy_utils.c gc_copy_utils.h \
			   collector.c collector.h \
			   iljit_gc_copy.c iljit_gc_copy.h \
			   recursion_stack.c recursion_stack.h \
			   gc_copy_types.h

libiljit_gc_copy_la_SOURCES = $(iljit_gc_copy_la_SOURCES)

# iljit_gc_copy needs some external library
# pthread needed by xanlib, but not declared in xanlib project
# Final object and and testing module requires different options
iljit_gc_copy_la_LIBADD = $(LIBILJITU_LIBS) $(XAN_LIBS) -lpthread -lrt
iljit_gc_copy_la_LDFLAGS = -module -avoid-version
libiljit_gc_copy_la_LIBADD = $(iljit_gc_copy_la_LIBADD)
