# The ildjit binary is the compilation framework
if MULTIAPP
bin_PROGRAMS = ildjitgen ildjit
else
bin_PROGRAMS = ildjit
endif

# Default compiler flags
AM_CFLAGS =										\
		$(ILJIT_ILJITU_CFLAGS)			$(ILJIT_ILJITSM_CFLAGS)		\
		$(ILJIT_ILJITMM_CFLAGS)			$(ILJITIR_CFLAGS)		\
		$(XAN_CFLAGS)				$(ILJITIROPTIMIZER_CFLAGS) 	\
		$(MANFRED_CFLAGS)			$(COMPILERMEMORYMANAGER_CFLAGS)	\
		$(DLA_CFLAGS)				$(IRVIRTUALMACHINE_CFLAGS)	\
		$(CLIMANAGER_CFLAGS)			$(PLATFORM_CFLAGS)		\
		$(ILJIT_CFLAGS) 			$(LIBPLUGINMANAGER_CFLAGS)	\
		$(ILBINARY_CFLAGS)							\
		-I../include -D_GNU_SOURCE

# Enable Multi Application support
if MULTIAPP
AM_CFLAGS += -DMULTIAPP
endif

# Enable debug flags
if DEBUG
AM_CFLAGS += -DDEBUG -O0 -ggdb3
endif # DEBUG

# Enabling profiling flags
if PROFILE
AM_CFLAGS 	+= -DNDEBUG -DPROFILE -O0 -g
else 
# Profiling and debug output enabled
if PRINTDEBUG 
AM_CFLAGS += -DPRINTDEBUG
else

# Profiling enabled, debug checks and output disabled
if DEBUG
else
AM_CFLAGS += -O3 -DNDEBUG
endif # DEBUG

endif # PRINTDEBUG

endif # PROFILE


# Virtual machine source files
if MULTIAPP
ildjitgen_SOURCES =								\
		ildjit.c

ildjit_SOURCES = iljit_loader.c

# Libraries needed by iljitgen
ildjitgen_LDADD =								\
		$(ILJIT_ILJITU_LIBS)						\
		$(ILJIT_ILJITSM_LIBS)		$(ILJIT_ILJITMM_LIBS)		\
		$(ILJITIR_LIBS)			$(ILJITIROPTIMIZER_LIBS)	\
		$(MANFRED_LIBS)			$(COMPILERMEMORYMANAGER_LIBS)	\
		$(XAN_LIBS)			$(DLA_LIBS)			\
		$(IRVIRTUALMACHINE_LIBS)	$(CLIMANAGER_LIBS)		\
		$(PLATFORM_LIBS)		$(ILJIT_LIBS)			\
                -lm                     -lpthread                

ildjitgen_LDFLAGS = 

# DUMA library
if DUMA
ildjitgen_LDADD +=-lduma
endif

else
ildjit_SOURCES =										\
		ildjit.c

# Libraries needed by iljit
ildjit_LDADD =										\
		$(ILJIT_ILJITU_LIBS)		$(LIBPLUGINMANAGER_LIBS)		\
		$(ILJIT_ILJITSM_LIBS)		$(ILJIT_ILJITMM_LIBS)			\
		$(ILJITIR_LIBS)			$(ILJITIROPTIMIZER_LIBS)		\
		$(MANFRED_LIBS)			$(COMPILERMEMORYMANAGER_LIBS)		\
		$(XAN_LIBS)			$(DLA_LIBS)				\
		$(IRVIRTUALMACHINE_LIBS)	$(CLIMANAGER_LIBS)			\
		$(PLATFORM_LIBS)		$(ILJIT_LIBS)				\
		$(ILBINARY_LIBS)							\
                -lm                     -lpthread                

ildjit_LDFLAGS = 

# DUMA library
if DUMA
ildjit_LDADD +=-lduma
endif

endif

MAINTAINERCLEANFILES = Makefile.in config.h.in
