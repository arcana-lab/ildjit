
# Automake file for buildind and running unit tests

# Search include files in library directory
AM_CFLAGS = -I$(top_srcdir)/lib $(ILJIT_CFLAGS) $(XAN_CFLAGS)

# Define test programs
TESTS = gc_copy_unit_test

# Define build rules
noinst_PROGRAMS = gc_copy_unit_test

# Define gc_copy_unit_test dependencies
gc_copy_unit_test_SOURCES = gc_copy_unit_test.c \
                            semi_space_test.c semi_space_test.h \
                            gc_memory_test.c gc_memory_test.h \
			    collector_test.c collector_test.h \
			    iljit_gc_copy_test.c iljit_gc_copy_test.h \
			    recursion_stack_test.c recursion_stack_test.h
gc_copy_unit_test_INCLUDES = $(CHECK_CFLAGS)
gc_copy_unit_test_LDADD = $(CHECK_LIBS) $(XAN_LIBS) \
                          $(top_builddir)/lib/libiljit_gc_copy.la
