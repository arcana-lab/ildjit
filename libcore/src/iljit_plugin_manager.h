/*
 * Copyright (C) 2006  Campanoni Simone
 *
 * iljit - This is a Just-in-time for the CIL language specified with the ECMA-335
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
 * @file plugin_manager.h
 * @brief Plugin manager
 */
#ifndef ILJIT_PLUGIN_LOADER_H
#define ILJIT_PLUGIN_LOADER_H

#include <decoder.h>
#include <jitsystem.h>
#include <xanlib.h>
#include <garbage_collector.h>

// My headers
#include <system_manager.h>
// End

#define PATH_PLUGIN get_path_plugin()
#define PATH_GARBAGE_COLLECTOR_PLUGIN get_path_garbage_collector_plugin()
#define PLUGIN_SYMBOL "plugin_interface"
#define MAX_PLUGIN_NAME 1024

/**
 * @brief Plugins
 *
 * This struct trace which plugin has decoded which assembly and it stores information of a plugin
 */
typedef struct {
    char		name[MAX_PLUGIN_NAME];								/**< Name of the plugin			*/
    void		*module_handle;									/**< Module handle of the plugin	*/
    XanList		*assemblies_decoded;								/**< List of (t_binary_information *) element; these are the assemblies that
													  * the plugin has decoded						*/
    JITINT8 *       (*get_ID_binary)	(void);							/**< Return the ID of a bytecode that the decoder can decode	*/
    JITBOOLEAN	(*decode_image)		(t_binary_information *binary_info);			/**< Run the decoder to decode an image of the bytecode*/
    JITBOOLEAN	(*shutdown)		(t_binary_information *binary_info);			/**< Shutdown the decoder that it has decode some bytecode*/
    char *		(*getName)		();
    char *		(*getVersion)		();
    char *		(*getAuthor)		();
    void		(*getCompilationFlags)	(char *buffer, JITINT32 bufferLength);
    void		(*getCompilationTime)	(char *buffer, JITINT32 bufferLength);

} t_plugins;

typedef struct {
    t_garbage_collector_plugin	*plugin;
    void				*handle;
    char				name[MAX_PLUGIN_NAME];
} t_garbage_collector;

/**
 *
 * Load the decoder plugins
 */
XanList * load_plugin (t_system *system);

/**
 *
 * Load the garbage collector plugins
 */
XanList * load_garbage_collectors (void);

/**
 *
 * Unload all the decoder plugins
 */
void unload_plugins (t_system *system);

#endif
