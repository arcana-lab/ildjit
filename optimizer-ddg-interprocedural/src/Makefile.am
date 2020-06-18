optimizer_ddg_LTLIBRARIES = optimizer_ddg.la
optimizer_ddgdir = $(libdir)/iljit/optimizers

AM_CFLAGS= -Wall -std=gnu99 -D_GNU_SOURCE		\
	$(ILJITU_CFLAGS) $(XAN_CFLAGS) $(ILJITIR_CFLAGS) $(ILJITIROPTIMIZER_CFLAGS) $(CHIARA_CFLAGS)
#	-Wno-unused-function				

if PROFILE
AM_CFLAGS 	+= -DNDEBUG -DPROFILE -g -O0
else 
if DEBUG
AM_CFLAGS	+= -ggdb -DDEBUG -O0
else
if OPTDEBUG
AM_CFLAGS	+= -ggdb -DDEBUG -O3
else
AM_CFLAGS	+= -DNDEBUG -O3
endif
endif
endif

if PRINTDEBUG
AM_CFLAGS 	+= -DPRINTDEBUG
endif

if CALLGRIND
AM_CFLAGS += -DCALLGRIND_PROFILE -g
endif

if SMALLLIB_DEBUG
AM_CFLAGS += -DSMALLLIB_DEBUG
endif

if MEMUSE_DEBUG
AM_CFLAGS += -DMEMUSE_DEBUG
endif

optimizer_ddg_la_SOURCES=					\
		optimizer_ddg.c		optimizer_ddg.h		\
		vllpa.c			vllpa.h			\
		vset.c			vset.h 			\
		vllpa_aliases.c		vllpa_aliases.h		\
		vllpa_info.c		vllpa_info.h		\
		vllpa_memory.c		vllpa_memory.h		\
		vllpa_types.c		vllpa_types.h		\
		vllpa_util.c		vllpa_util.h		\
		memory_aliases.c	memory_aliases.h	\
		smalllib.c		smalllib.h		\
		vllpa_macros.h

optimizer_ddg_la_LDFLAGS= -module -avoid_version

optimizer_ddg_la_LIBADD = $(ILJITU_LIBS) $(XAN_LIBS) $(ILJITIROPTIMIZER_LIBS) $(ILJITIR_LIBS) $(CHIARA_LIBS)

MAINTAINERCLEANFILES = Makefile.in config.h.in
