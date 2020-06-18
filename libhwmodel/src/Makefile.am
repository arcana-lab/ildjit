libhwmodel_LTLIBRARIES = libhwmodel.la
libhwmodeldir = $(libdir)/iljit

AM_CPPFLAGS	= -std=gnu99 -Wall -I../include -D_GNU_SOURCE

if PROFILE
AM_CPPFLAGS	+= -g -DNDEBUG -O0
else
if PRINTDEBUG
AM_CPPFLAGS	+= -ggdb -DDEBUG -DPRINTDEBUG -O0
else 
if DEBUG
AM_CPPFLAGS	+= -ggdb -DDEBUG -O0
else 
AM_CPPFLAGS	+= -O3 -DNDEBUG 
endif
endif
endif

libhwmodel_la_SOURCES=								\
		bpred-2lev.cpp  						\
		zesto-bpred.cpp			zesto-bpred.h			\
		eval.cpp  			eval.h				\
		misc.cpp  			misc.h				\
		stats.cpp			stats.h				\
		cache.cpp			cache.h				\
		cache_array.cpp			cache_array.h		        \
		hwmodel_system.h						\
		host.h  							\
		machine.h 							\
		options.h  							\
		regs.h  							\
		sim.h  			  					\
		thread.h  							\
		valcheck.h    							\
		zesto-core.h  							\
		zesto-structs.h							\
		bpred.h	

libhwmodel_la_LIBADD	= 

libhwmodel_la_LDFLAGS	= -shared -fPIC
