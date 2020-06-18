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
 * @file plugin_manager.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <dirent.h>
#include <assert.h>
#include <sys/types.h>
#include <compiler_memory_manager.h>
#include <platform_API.h>

// My headers
#include <iljit_plugin_manager.h>
#include <iljit-utils.h>
#include <iljit.h>
#include <general_tools.h>
// Done

/**
 *
 * Free memory allocated by the plugin to decode its assemblies
 */
void shutdown_assemblies_decoded_by_plugin (t_plugins *plugin);

XanList * load_garbage_collectors (void) {
    DIR		*plugin_dir;
    struct dirent	*info_current_plugin;
    int		first_time = 1;
    XanList		*plugins;
    char		*buf;
    JITINT32	bufLength;

    plugins = xanList_new(allocFunction, freeFunction, NULL);
    assert(plugins != NULL);
    plugin_dir = PLATFORM_openDir(PATH_GARBAGE_COLLECTOR_PLUGIN);
    if (plugin_dir == NULL) {
        buf = allocMemory(sizeof(char) * DIM_BUF);
        snprintf(buf, sizeof(char) * DIM_BUF, "PLUGIN MANAGER: ERROR= I couldn't open the garbage collectors plugin directory %s. ", PATH_GARBAGE_COLLECTOR_PLUGIN);
        print_err(buf, errno);
        freeMemory(buf);
        if (PLATFORM_closeDir(plugin_dir) != 0) {
            print_err("PLUGIN MANAGER: ERROR= Cannot close the plugin directory. ", errno);
        }
        return plugins;
    }

    /* Allocate the buffer		*/
    bufLength = 100;
    buf = allocMemory(sizeof(char) * bufLength);

    /* Load the plugins		*/
    while (1) {
        info_current_plugin = PLATFORM_readDir(plugin_dir);
        if (info_current_plugin == NULL) {
            if (first_time != 1) {
                PDEBUG("PLUGIN MANAGER: Plug-in scan complete\n");
            }
            assert(plugin_dir != NULL);
            if (PLATFORM_closeDir(plugin_dir) != 0) {
                print_err("PLUGIN MANAGER: ERROR= Cannot close the plugin directory. ", errno);
                freeMemory(buf);
                return plugins;
            }
            freeMemory(buf);
            return plugins;
        }
        if (info_current_plugin->d_name[0] != '.' && str_has_suffix(info_current_plugin->d_name, SHARED_LIB_FILE_EXTENSION)) {
            void                            *temp;
            t_garbage_collector             *plugin;
            t_garbage_collector_plugin      *plugin_handle;

            if (strlen(PATH_GARBAGE_COLLECTOR_PLUGIN) + strlen(info_current_plugin->d_name) >= bufLength) {
                bufLength = (strlen(PATH_GARBAGE_COLLECTOR_PLUGIN) + strlen(info_current_plugin->d_name)) * 2;
                buf = reallocMemory(buf, bufLength);
            }
            snprintf(buf, sizeof(char) * bufLength, "%s%s", PATH_GARBAGE_COLLECTOR_PLUGIN, info_current_plugin->d_name);

            // Load plugin with name stored in buf
            temp = PLATFORM_dlopen(buf, RTLD_NOW);
            if (temp == NULL) {
                char buf2[DIM_BUF];
                snprintf(buf2, sizeof(buf2), "PLUGIN MANAGER: WARNING= Cannot load plugin %s. Error=%s\n", buf, PLATFORM_dlerror());
                print_err(buf2, 0);
                PDEBUG("PLUGIN MANAGER: I'm not brave so i will block the normal execution\n");
                if (PLATFORM_closeDir(plugin_dir) != 0) {
                    print_err("PLUGIN MANAGER: ERROR= Cannot close the plugin directory. ", errno);
                }
                freeMemory(buf);
                return NULL;
            }
            assert(temp != NULL);
            PLATFORM_dlerror();
            plugin_handle = PLATFORM_dlsym(temp, PLUGIN_SYMBOL);
            if (plugin_handle == NULL) {
                char buf2[DIM_BUF];
                snprintf(buf2, sizeof(buf2), "PLUGIN MANAGER: WARNING= Cannot resolv simbol in plugin %s. Error=%s", buf, PLATFORM_dlerror());
                print_err(buf2, 0);
                // Unload the last plugin loaded
                if (PLATFORM_dlclose(temp) != 0) {
                    char buf2[DIM_BUF];
                    snprintf(buf2, sizeof(buf2), "PLUGIN MANAGER: WARNING= During unload plugin %s. ", buf);
                    print_err(buf, 0);
                }
                PDEBUG("PLUGIN MANAGER: I'm not brave so i will block the normal execution\n");
                if (PLATFORM_closeDir(plugin_dir) != 0) {
                    print_err("PLUGIN MANAGER: ERROR= Cannot close the plugin directory. ", errno);
                }
                freeMemory(buf);
                return NULL;
            }
            assert(plugin_handle != NULL);

            /* Allocate memory for the plugin struct		*/
            plugin = (t_garbage_collector *) allocMemory(sizeof(t_garbage_collector));
            if (plugin == NULL) {
                PDEBUG("PLUGIN MANAGER: ERROR= Cannot allocate memory for the plugin %s\n", buf);
                if (PLATFORM_closeDir(plugin_dir) != 0) {
                    print_err("PLUGIN MANAGER: ERROR= Cannot close the plugin directory. ", errno);
                }
                if (PLATFORM_dlclose(temp) != 0) {
                    char buf2[DIM_BUF];
                    snprintf(buf2, sizeof(buf2), "PLUGIN MANAGER: WARNING= During unload plugin %s. ", buf);
                    print_err(buf, 0);
                }
                freeMemory(buf);
                return NULL;
            }

            /* Fill the fields of the current plugin struct		*/
            plugin->handle = temp;
            plugin->plugin = plugin_handle;
            assert(plugin->plugin->init != NULL);
            assert(plugin->plugin->shutdown != NULL);
            assert(plugin->plugin->allocObject != NULL);
            assert(plugin->plugin->collect != NULL);
            assert(plugin->plugin->gcInformations != NULL);
            assert(plugin->plugin->getName != NULL);
            assert(plugin->plugin->getVersion != NULL);
            assert(plugin->plugin->getAuthor != NULL);
            strncpy(plugin->name, buf, sizeof(char) * MAX_PLUGIN_NAME);

            /* Insert the current plugin into the plugins list	*/
            xanList_insert(plugins, plugin);
            PDEBUG("PLUGIN MANAGER: <%s> plugin loaded\n", buf);
        }
        PLATFORM_dlerror();
    } // End while

    /* Free the memory			*/
    freeMemory(buf);

    /* Return				*/
    return NULL;;
}

XanList * load_plugin (t_system *system) {
    DIR             *plugin_dir;
    struct dirent   *info_current_plugin;
    XanList         *plugins;

    /* Make the list		*/
    plugins = xanList_new(allocFunction, freeFunction, NULL);
    assert(plugins != NULL);

    plugin_dir = PLATFORM_openDir(PATH_PLUGIN);
    if (plugin_dir == NULL) {
        print_err("PLUGIN MANAGER: ERROR= I couldn't open the plugin directory. ", errno);
        if (PLATFORM_closeDir(plugin_dir) != 0) {
            print_err("PLUGIN MANAGER: ERROR= Cannot close the plugin directory. ", errno);
        }
        return plugins;
    }
    while (1) {
        info_current_plugin = PLATFORM_readDir(plugin_dir);
        if (info_current_plugin == NULL) {
            if (PLATFORM_closeDir(plugin_dir) != 0) {
                print_err("PLUGIN MANAGER: ERROR= Cannot close the plugin directory. ", errno);
            }
            return plugins;
        }
        if (info_current_plugin->d_name[0] != '.' && str_has_suffix(info_current_plugin->d_name, SHARED_LIB_FILE_EXTENSION)) {
            char buf[DIM_BUF];
            void            *temp;
            t_plugins       *plugin;

            snprintf(buf, sizeof(buf), "%s%s", PATH_PLUGIN, info_current_plugin->d_name);

            // Load plugin with name stored in buf
            temp = PLATFORM_dlopen(buf, RTLD_NOW);
            if (temp == NULL) {
                char buf2[DIM_BUF];
                snprintf(buf2, sizeof(buf2), "PLUGIN MANAGER: WARNING= Cannot load plugin %s. Error=%s\n", buf, PLATFORM_dlerror());
                print_err(buf2, 0);
                if (PLATFORM_closeDir(plugin_dir) != 0) {
                    print_err("PLUGIN MANAGER: ERROR= Cannot close the plugin directory. ", errno);
                }
                return plugins;
            }
            assert(temp != NULL);
            PLATFORM_dlerror();
            t_plugin_interface *plugin_handle = PLATFORM_dlsym(temp, DECODER_SYMBOL);
            if (plugin_handle == NULL) {
                char buf2[DIM_BUF];
                snprintf(buf2, sizeof(buf2), "PLUGIN MANAGER: WARNING= Cannot resolv simbol in plugin %s. Error=%s", buf, PLATFORM_dlerror());
                print_err(buf2, 0);
                // Unload the last plugin loaded
                if (PLATFORM_dlclose(temp) != 0) {
                    char buf2[DIM_BUF];
                    snprintf(buf2, sizeof(buf2), "PLUGIN MANAGER: WARNING= During unload plugin %s. ", buf);
                    print_err(buf, 0);
                }
                if (PLATFORM_closeDir(plugin_dir) != 0) {
                    print_err("PLUGIN MANAGER: ERROR= Cannot close the plugin directory. ", errno);
                }
                return NULL;
            }
            assert(plugin_handle != NULL);

            /* Allocate memory for the plugin struct		*/
            plugin = (t_plugins *) allocMemory(sizeof(t_plugins));

            /* Fill the fields of the current plugin struct		*/
            plugin->module_handle = temp;
            plugin->get_ID_binary = plugin_handle->get_ID_binary;
            plugin->decode_image = plugin_handle->decode_image;
            plugin->shutdown = plugin_handle->shutdown;
            plugin->getName = plugin_handle->getName;
            plugin->getVersion = plugin_handle->getVersion;
            plugin->getAuthor = plugin_handle->getAuthor;
            plugin->getCompilationTime = plugin_handle->getCompilationTime;
            plugin->getCompilationFlags = plugin_handle->getCompilationFlags;
            strncpy(plugin->name, buf, sizeof(char) * MAX_PLUGIN_NAME);
            plugin->assemblies_decoded = xanList_new(allocFunction, freeFunction, NULL);
            assert(plugin->assemblies_decoded != NULL);

            /* Insert the current plugin into the plugins list	*/
            xanList_insert(plugins, plugin);
            PDEBUG("PLUGIN MANAGER: <%s> plugin loaded\n", buf);
        }
        PLATFORM_dlerror();
    } // End while
}

void unload_plugins (t_system *system) {
    XanListItem     *item;

    /* Assertions		*/
    assert(system != NULL);

    if (system->plugins == NULL) {
        return ;
    }

    PDEBUG("PLUGIN MANAGER:	 Unload plugins\n");
    item = xanList_first(system->plugins);
    while (item != NULL) {
        t_plugins *plugin;

        /* Fetch the next plugin			*/
        PDEBUG("PLUGIN MANAGER:		Fetch a plugin named ");
        plugin 	= item->data;
        PDEBUG("%s\n", plugin->name);

        /* Shutdown the assemblies decoded by the
         * current assembly				*/
        shutdown_assemblies_decoded_by_plugin(plugin);

        /* Unload the current plugin			*/
        PDEBUG("PLUGIN MANAGER:		Unload the current plugin\n");
        if (PLATFORM_dlclose(plugin->module_handle) != 0) {
            print_err("PLUGIN MANAGER: ERROR= Cannot unload plugins. ", errno);
        }

        /* Free the struct bind to the current plugin	*/
        PDEBUG("PLUGIN MANAGER:		Free memory of the current plugin\n");
        freeMemory(plugin);

        /* Fetch the next				*/
        item = item->next;
    }

    /* Free the list		*/
    xanList_destroyList(system->plugins);

    /* Return			*/
    return;
}

void shutdown_assemblies_decoded_by_plugin (t_plugins *plugin) {
    XanListItem             *temp_list;
    t_binary_information    *assembly;
#ifdef DEBUG
    JITUINT32 size;
#endif

    /* Assertions					*/
    assert(plugin != NULL);

    /* Get the length				*/
#ifdef DEBUG
    size = xanList_length(plugin->assemblies_decoded);
    PDEBUG("PLUGIN MANAGER:                 Shutdown the current plugin\n");
    PDEBUG("PLUGIN MANAGER:                         %d Assemblies decoded\n", size);
#endif

    /* Fetch the first assembly decoded by the
     * current plugin				*/
    temp_list = xanList_first(plugin->assemblies_decoded);
    while (temp_list != NULL) {
        assembly = (t_binary_information *) temp_list->data;
        PDEBUG("PLUGIN MANAGER:                         Found an assembly decoded named %s\n", assembly->name);

        /* Call the shutdown function of the plugin	*/
        PDEBUG("PLUGIN MANAGER:					Call the shutdown function of the current plugin\n");
        if (plugin->shutdown(assembly) != 0) {
            print_err("PLUGIN MANAGER: ERROR during shutting down a plugin. ", 0);
        }

        /* Free the memory of the assembly		*/
        PDEBUG("PLUGIN MANAGER:					Free assembly memory\n");
#ifdef DEBUG
        size = xanList_length(plugin->assemblies_decoded);
#endif
        xanList_removeElement(plugin->assemblies_decoded, assembly, JITTRUE);
        assert(xanList_length(plugin->assemblies_decoded) == (size - 1));
        if ((assembly->binary.reader) != NULL) {
            freeFunction(*(assembly->binary.reader));
        }
        freeFunction(assembly->binary.reader);
        freeFunction(assembly->prefix);
        freeFunction(assembly);

        /* Fetch the next assembly decoded by the
         * current plugin				*/
        temp_list = xanList_first(plugin->assemblies_decoded);
    }

    /* Free the assemblies list		*/
    PDEBUG("PLUGIN MANAGER:                 Free the assembly decoded list\n");
    xanList_destroyList(plugin->assemblies_decoded);

    /* Return				*/
    return;
}
