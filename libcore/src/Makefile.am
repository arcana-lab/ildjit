# The iljit binary is the compilation framework
libiljit_LTLIBRARIES= libiljit.la
libiljitdir = $(libdir)/iljit

# Default compiler flags
AM_CFLAGS =										\
		$(ILJIT_ILJITU_CFLAGS)			$(ILJIT_ILJITSM_CFLAGS)		\
		$(ILJIT_ILJITMM_CFLAGS)			$(ILJITIR_CFLAGS)		\
		$(XAN_CFLAGS)				$(ILJITIROPTIMIZER_CFLAGS) 	\
		$(MANFRED_CFLAGS)			$(COMPILERMEMORYMANAGER_CFLAGS)	\
		$(DLA_CFLAGS)				$(IRVIRTUALMACHINE_CFLAGS)	\
		$(CLIMANAGER_CFLAGS)			$(PLATFORM_CFLAGS)		\
		$(IRVIRTUALMACHINE_BACKEND_CFLAGS) 	$(LIBPLUGINMANAGER_CFLAGS)	\
		$(ILJIT_ILBINARY_CFLAGS)						\
		-I../include -D_GNU_SOURCE

if PROFILE
AM_CFLAGS 	+= -DNDEBUG -DPROFILE -O0 -g
else
if PRINTDEBUG
AM_CFLAGS	+= -g -DDEBUG -DPRINTDEBUG -O0
else 
if DEBUG
AM_CFLAGS 	+= -DDEBUG -O0 -g
else 
AM_CFLAGS	+= -O3 -DNDEBUG 
endif
endif
endif

# Declare using of a lock library
if USE_LIB_LOCK_OS
LOCK_LIBRARY_SRCS = lib_lock_os.c                       lib_lock_os.h
AM_CFLAGS += -DUSE_LIB_LOCK_OS
else
if USE_LIB_LOCK_THIN
LOCK_LIBRARY_SRCS = lib_lock_thin.c			lib_lock_thin.h
AM_CFLAGS += -DUSE_LIB_LOCK_THIN
else
LOCK_LIBRARY_SRCS = lib_lock_os.c			lib_lock_os.h
AM_CFLAGS += -DUSE_LIB_LOCK_OS
endif
endif

# Source files
libiljit_la_SOURCES =										\
		system_manager.c			system_manager.h			\
		iljit_plugin_manager.c			iljit_plugin_manager.h			\
                initial_checker.c                       initial_checker.h                       \
                system_initializer.c                    system_initializer.h                    \
                loader.c                                loader.h                                \
                cil_ir_translator.c                     cil_ir_translator.h                     \
                executor.c                              executor.h                              \
                runtime.c                               runtime.h                               \
		ildjit_profiler.c			ildjit_profiler.h			\
		clr_manager.c				clr_manager.h				\
		garbage_collector_interactions.c	garbage_collector_interactions.h	\
		recompiler.c				recompiler.h				\
		exception_manager.c			exception_manager.h			\
		general_tools.c				general_tools.h				\
		translation_pipeline.c			translation_pipeline.h			\
		translation_pipeline_sequential.c	translation_pipeline_sequential.h	\
		translation_pipeline_parallel.c		translation_pipeline_parallel.h		\
		lib_compilerNativeCall.c		lib_compilerNativeCall.h		\
		static_memory.c				static_memory.h				\
                iljit.h                                 iljit.c                                 \
		code_generator.c			code_generator.h			\
		ildjit_system.h									\
                lib_lock.h                                                                      \
		clr_interface.h									\
                $(LOCK_LIBRARY_SRCS)

# Libraries needed by iljit
libiljit_la_LIBADD =										\
		$(ILJIT_ILJITU_LIBS)		$(LIBPLUGINMANAGER_LIBS)		\
		$(ILJIT_ILJITSM_LIBS)		$(ILJIT_ILJITMM_LIBS)			\
		$(ILJITIR_LIBS)			$(ILJITIROPTIMIZER_LIBS)		\
		$(MANFRED_LIBS)			$(COMPILERMEMORYMANAGER_LIBS)		\
		$(XAN_LIBS)			$(DLA_LIBS)				\
		$(IRVIRTUALMACHINE_LIBS)	$(CLIMANAGER_LIBS)			\
		$(PLATFORM_LIBS)		$(IRVIRTUALMACHINE_BACKEND_LIBS)	\
		$(ILJIT_ILBINARY_LIBS)							\
                -lm                     -lpthread                

libiljit_la_LDFLAGS = -fPIC -no-undefined

# Real time clock library
if CLOCKGETTIME
libiljit_la_LIBADD +=-lrt
endif

# Also distribute this files
EXTRA_DIST =                                    \
		lib_compilerNativeCall.gperf

# Build by lex, don't keep on source tree
CLEANFILES =                                    \
		lib_compilerNativeCall.c

MAINTAINERCLEANFILES = Makefile.in config.h.in

lib_compilerNativeCall.c: lib_compilerNativeCall.gperf
	$(GPERF) --output-file=$@ $<

