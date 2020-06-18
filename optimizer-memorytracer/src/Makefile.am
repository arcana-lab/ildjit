optimizer_memorytracer_LTLIBRARIES = optimizer_memorytracer.la
optimizer_memorytracerdir = $(libdir)/iljit/optimizers

AM_CFLAGS= -Wall -std=gnu99 -D_GNU_SOURCE		\
	$(ILJITU_CFLAGS) $(XAN_CFLAGS) $(ILJITIR_CFLAGS) $(ILJITIROPTIMIZER_CFLAGS) $(CAM_CFLAGS) $(CHIARA_CFLAGS)

if PROFILE
AM_CFLAGS 	+= -DNDEBUG -DPROFILE -g -O0
else 
if PRINTDEBUG
AM_CFLAGS 	+= -ggdb -DPRINTDEBUG -DDEBUG -O0
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
endif

optimizer_memorytracer_la_SOURCES=							\
		optimizer_memorytracer.c	optimizer_memorytracer.h		\
		runtime.c			runtime.h				\
		code_injector.c			code_injector.h

optimizer_memorytracer_la_LDFLAGS= -module -avoid_version 

optimizer_memorytracer_la_LIBADD = $(ILJITU_LIBS) $(XAN_LIBS) $(ILJITIROPTIMIZER_LIBS) $(ILJITIR_LIBS) $(CAM_LIBS) $(CHIARA_LIBS)

MAINTAINERCLEANFILES = Makefile.in config.h.in
