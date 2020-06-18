MAINTAINERCLEANFILES = Makefile.in config.h.in

libirvirtualmachine_llvm_LTLIBRARIES = libirvirtualmachine_llvm.la
libirvirtualmachine_llvmdir = $(libdir)/iljit

AM_CPPFLAGS 	= 	-Wall 				\
			-D_GNU_SOURCE			\
			$(XANLIB_CFLAGS) 		\
			$(COMPILERMEMORYMANAGER_CFLAGS) \
			$(LIBILJITU_CFLAGS) 		\
			$(LIBILJITIR_CFLAGS) 		\
			$(LIBILJITIROPTIMIZER_CFLAGS) 	\
			$(JIT_CFLAGS)			\
			$(PLATFORM_CFLAGS)		\
			$(ILJITGC_CFLAGS)		\
			$(IRVIRTUALMACHINE_CFLAGS)

if PROFILE
AM_CPPFLAGS 	+= -DNDEBUG -DPROFILE -O0 -g
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

libirvirtualmachine_llvm_la_SOURCES=								\
		IRtoBCMaterializer.cpp			IRtoBCMaterializer.h			\
		MachineCodeGeneratorEventListener.cpp	MachineCodeGeneratorEventListener.h	\
		ir_virtual_machine_runtime.cpp		ir_virtual_machine_runtime.h		\
		ir_virtual_machine_pass_manager.cpp	ir_virtual_machine_pass_manager.h	\
		ir_virtual_machine.cpp								\
		ir_virtual_machine_backend_structure_manager.cpp				\
		ir_virtual_machine_manager.cpp							\
		ir_virtual_machine_stack_trace.cpp						\
		ir_virtual_machine_exception.cpp						\
		ir_virtual_machine_execution.cpp						\
		ir_virtual_machine_backend.h							\
		ir_virtual_machine_system.h							\
		utilities.cpp				utilities.h

libirvirtualmachine_llvm_la_LDFLAGS= -fPIC      \
                    $(JIT_LDFLAGS)

libirvirtualmachine_llvm_la_LIBADD	= 				\
					$(XANLIB_LIBS) 			\
					$(COMPILERMEMORYMANAGER_LIBS) 	\
					$(LIBILJITU_LIBS)		\
			 		$(LIBILJITIR_LIBS) 		\
					$(LIBILJITIRPROFILER_LIBS)	\
					$(JIT_LIBS)			\
					$(ILJITIROPTIMIZER_LIBS)	\
					$(ILJITGC_LIBS)			\
					$(PLATFORM_LIBS)		\
					$(IRVIRTUALMACHINE_LIBS)
