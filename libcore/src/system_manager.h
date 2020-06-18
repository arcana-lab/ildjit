/*
 * Copyright (C) 2006 - 2012 Campanoni Simone
 *
 * iljit - This is the core of the ILDJIT compilation framework
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/**
 * @file system_manager.h
 */
#ifndef SYSTEM_MANAGER_H
#define SYSTEM_MANAGER_H

#include <ildjit_system.h>

#define DEFAULT_HEAP_SIZE                       0       // Bytes
#define DEFAULT_LAB                                                     0.0
#define ILDJIT_MEMORY_MAYMOVE                   1
#define ILDJIT_MEMORY_FIXEDSIZE                 2

/* FILTER RETURN VALUE DEFINITIONS */
#define   EXCEPTION_CONTINUE_SEARCH     0       // to continue searching for an exception handler
#define   EXCEPTION_EXECUTE_HANDLER     1       /* to start the second phase of exception handling              *
	                                        * where finally blocks are run until the handler               *
	                                        * associated with this filter clause is located. Then the      *
	                                        * handler is executed                                          */

/**
 *
 * Bootstrap the generator
 */
t_system * generator_bootstrapper (int argc, char **argv);

/**
 *
 * Bootstrap the single instance
 */
t_system * system_bootstrapper_multiapp (t_system *system, int argc, char **argv);
t_system * system_bootstrapper (int argc, char **argv);

t_system * allocateAndInitializeSystem (JITBOOLEAN loadPlugins, JITBOOLEAN initMemoryManager, JITBOOLEAN parseArguments, int argc, char *argv[], JITBOOLEAN enableCodeGenerator, JITBOOLEAN enableDynamicCLRs);

char * get_path_garbage_collector_plugin ();
char * get_path_plugin ();
char * get_cil_path ();
char * get_machinecode_path ();
char * get_library_path ();
char * get_prefix ();
char * get_path_clr_plugin ();
void init_paths ();

#endif
