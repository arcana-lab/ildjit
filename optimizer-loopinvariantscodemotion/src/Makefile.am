optimizer_loop_invariant_code_motion_LTLIBRARIES = optimizer_loop_invariant_code_motion.la
optimizer_loop_invariant_code_motiondir = $(libdir)/iljit/optimizers

AM_CFLAGS= -Wall  $(ILJITU_CFLAGS) $(XAN_CFLAGS) $(ILJITIR_CFLAGS) $(ILJITIROPTIMIZER_CFLAGS) $(COMPILERMEMORYMANAGER_CFLAGS) $(CHIARA_CFLAGS)

if PROFILE
AM_CFLAGS 	+= -DNDEBUG -DPROFILE -O0 -g
else 
if PRINTDEBUG
AM_CFLAGS 	+= -Wall -DPRINTDEBUG -DDEBUG -O0
else
if DEBUG
AM_CFLAGS 	+= -Wall -DDEBUG -O0 -ggdb3
else
AM_CFLAGS 	+= -O3 -DNDEBUG 
endif
endif
endif

optimizer_loop_invariant_code_motion_la_SOURCES=					\
		loop_invariant_hoisting.c		loop_invariant_hoisting.h

optimizer_loop_invariant_code_motion_la_LDFLAGS= -module -avoid_version $(ILJITU_LIBS) $(XAN_LIBS) $(ILJITIR_LIBS) $(ILJITIROPTIMIZER_LIBS) $(COMPILERMEMORYMANAGER_LIBS) $(CHIARA_LIBS)
