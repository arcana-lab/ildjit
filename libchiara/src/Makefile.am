libchiara_LTLIBRARIES = libchiara.la
libchiaradir = $(libdir)/iljit

AM_CFLAGS= -std=gnu99 -Wall -Wno-unused-function -I../include $(XAN_CFLAGS) $(ILJITU_CFLAGS) $(ILJITSM_CFLAGS) $(ILJITMM_CFLAGS) $(LIBILJITIR_CFLAGS) $(DOMINANCE_CFLAGS) -D_GNU_SOURCE

if PROFILE
AM_CFLAGS	+= -g -DNDEBUG -O0
else
if PRINTDEBUG
AM_CFLAGS	+= -ggdb -DDEBUG -DPRINTDEBUG -O0
else 
if DEBUG
AM_CFLAGS	+= -ggdb -DDEBUG -O0
else
if OPTDEBUG
AM_CFLAGS	+= -ggdb -DDEBUG -O3
else 
AM_CFLAGS	+= -O3 -DNDEBUG 
endif
endif
endif
endif

libchiara_la_SOURCES=										\
		loops_invocations.c			loops_invocations.h			\
		loop_names_scanner.c 			loop_names_scanner.h			\
		chiara_load_loop_names.c		chiara_load_loop_names.h		\
		chiara_tools.c				chiara_tools.h				\
		chiara.c				chiara.h				\
		chiara_loop_transformations.c		chiara_loop_transformations.h		\
		chiara_loops_profiler_misc.c		chiara_loops_profiler_misc.h		\
		chiara_profile.c			chiara_profile.h			\
		status_file_scanner.c 			status_file_scanner.h			\
		instruction_scanner.c 			instruction_scanner.h			\
		loop_scanner.c				loop_scanner.h				\
		load_instructions.c								\
		load_loops.c									\
		chiara_dominators.c								\
		chiara_methods.c								\
		chiara_dependencies.c								\
		chiara_misc.c									\
		chiara_dependencies_profiler.c							\
		chiara_dopipe_profiler.c							\
		chiara_loops_profiler.c								\
		chiara_loops_IR_profiler.c							\
		chiara_time.c									\
		chiara_memory.c									\
		chiara_program_profiler.c							\
		chiara_schedule.c								\
		chiara_instructions.c								\
		chiara_system.h

libchiara_la_LIBADD	= $(ILJITU_LIBS) $(ILJITSM_LIBS) $(ILJITMM_LIBS) $(XAN_LIBS) $(DOMINANCE_LIBS)

libchiara_la_LDFLAGS= -shared -fPIC $(ILJITU_LIBS) $(ILJITSM_LIBS) $(ILJITMM_LIBS) $(LIBILJITIR_LIBS) $(XAN_LIBS) $(DOMINANCE_LIBS)

EXTRA_DIST =                           		         	\
	instruction.lex						\
	loop_names.lex						\
	loop.lex

CLEANFILES = 							\
		loops_invocations.c				\
		loops_invocations.h				\
		status_file_scanner.c 				\
		status_file_scanner.h				\
		loop_names_scanner.c				\
		loop_names_scanner.h				\
		instruction_scanner.c 				\
		instruction_scanner.h				\
		loop_scanner.c					
		loop_scanner.h

loops_invocations.c loops_invocations.h: loops_invocations.lex
	$(LEX) $<

status_file_scanner.c status_file_scanner.h: status_file.lex
	$(LEX) $<

loop_names_scanner.c loop_names_scanner.h: loop_names.lex
	$(LEX) $<

instruction_scanner.c instruction_scanner.h: instruction.lex
	$(LEX) $<

loop_scanner.c loop_scanner.h: loop.lex
	$(LEX) $<
