optimizer_ddg_profile_LTLIBRARIES = optimizer_ddg_profile.la
optimizer_ddg_profiledir = $(libdir)/iljit/optimizers

AM_CFLAGS= -Wall -D_FILE_OFFSET_BITS=64				\
	$(ILJITU_CFLAGS) $(XAN_CFLAGS) $(ILJITIR_CFLAGS) $(ILJITIROPTIMIZER_CFLAGS) $(CHIARA_CFLAGS) 

if PROFILE
AM_CFLAGS 	+= -DNDEBUG -DPROFILE -g -O0
else 
if PRINTDEBUG
AM_CFLAGS 	+= -ggdb -DPRINTDEBUG -DDEBUG -O0
else
if DEBUG
AM_CFLAGS	+= -ggdb3 -DDEBUG -O0
else
AM_CFLAGS	+= -DNDEBUG -O3
endif
endif
endif

optimizer_ddg_profile_la_SOURCES=							\
		optimizer_ddg_profile.c		optimizer_ddg_profile.h			\
		runtime.c			runtime.h				\
		code_injector.c			code_injector.h				\
		instruction_scanner.c 		instruction_scanner.h			\
		loop_scanner.c 			loop_scanner.h				\
		load_loops.c			load_loops.h				\
		load_instructions.c		load_instructions.h

optimizer_ddg_profile_la_LDFLAGS= -module -avoid_version 

optimizer_ddg_profile_la_LIBADD = $(ILJITU_LIBS) $(XAN_LIBS) $(ILJITIROPTIMIZER_LIBS) $(ILJITIR_LIBS) $(CHIARA_LIBS) -lbz2

MAINTAINERCLEANFILES = Makefile.in config.h.in

EXTRA_DIST =					\
	instruction.lex				\
	loop.lex

CLEANFILES = 					\
	instruction_scanner.c 			\
	instruction_scanner.h			\
	loop_scanner.c 				\
	loop_scanner.h
	
loop_scanner.c loop_scanner.h: loop.lex
	$(LEX) $<

instruction_scanner.c instruction_scanner.h: instruction.lex
	$(LEX) $<

