/*
 * Copyright (C) 2012  Campanoni Simone
 *
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
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <assert.h>
#include <time.h>
#include <ir_virtual_machine.h>
#include <platform_API.h>
#include <plugin_manager.h>

// My headers
#include <system_manager.h>
#include <clr_interface.h>
// End

#define DIM_BUF 1024

typedef struct {
    void                 	*handle;
    clr_interface_t        	*plugin;
} clr_t;

extern t_system *ildjitSystem;

static inline void shutdown_clr_plugin (ildjit_plugin_t *plugin, void *data);

void CLR_loadCLRs (void) {
    JITINT8		*clrPaths;
    XanListItem *item;

    /* Fetch the paths where CLRs are installed.
     */
    clrPaths	= (JITINT8 *) get_path_clr_plugin();
    assert(clrPaths != NULL);

    /* Allocate the list where pointers to CLRs will be stored.
     */
    ildjitSystem->CLRPlugins	= xanList_new(allocFunction, freeFunction, NULL);

    /* Load the CLRs.
     */
    PLUGIN_loadPluginsFromDirectories(ildjitSystem->CLRPlugins, clrPaths, sizeof(clr_t), (JITINT8 *) "plugin_interface");

    /* Init the CLRs.
     */
    item    = xanList_first(ildjitSystem->CLRPlugins);
    while (item != NULL){
        clr_t   *clr;
        clr     = item->data;
        clr->plugin->init(ildjitSystem);
        item    = item->next;
    }

    return ;
}

void CLR_getFunctionNameAndFunctionPointer (JITINT8 *signatureInString, JITINT8 **functionName, void **functionPointer) {
    XanListItem	*item;

    /* Assertions.
     */
    assert(signatureInString != NULL);
    assert(functionName != NULL);
    assert(functionPointer != NULL);
    assert(ildjitSystem->CLRPlugins != NULL);

    /* Initialize variables.
     */
    (*functionName)		= NULL;
    (*functionPointer)	= NULL;

    /* Search the function.
     */
    item	= xanList_first(ildjitSystem->CLRPlugins);
    while (item != NULL) {
        clr_t	*clr;
        clr	= item->data;
        assert(clr != NULL);
        if (clr->plugin->getNameAndFunctionPointerOfMethod(signatureInString, functionName, functionPointer)) {
            return;
        }
        item	= item->next;
    }

    fprintf(stderr, "ILDJIT: CLR_MANAGER: ERROR = No native function with name %s has been found in available CLRs\n", signatureInString);
    abort();

    return ;
}

JITBOOLEAN CLR_doesExistMethod (Method method) {
    XanListItem	*item;

    /* Assertions.
     */
    assert(ildjitSystem->CLRPlugins != NULL);

    item	= xanList_first(ildjitSystem->CLRPlugins);
    while (item != NULL) {
        clr_t	*clr;
        clr	= item->data;
        assert(clr != NULL);
        if (clr->plugin->buildMethod(method)) {
            return JITTRUE;
        }
        item	= item->next;
    }

    return JITFALSE;
}

void CLR_buildMethod (Method method) {
    XanListItem	*item;

    /* Assertions.
     */
    assert(ildjitSystem->CLRPlugins != NULL);

    item	= xanList_first(ildjitSystem->CLRPlugins);
    while (item != NULL) {
        clr_t	*clr;
        clr	= item->data;
        assert(clr != NULL);
        if (clr->plugin->buildMethod(method)) {
            return;
        }
        item	= item->next;
    }

    return ;
}

void CLR_unloadCLRs (void) {

    /* Unload the plugins.
     */
    if (ildjitSystem->CLRPlugins != NULL) {
        PLUGIN_unloadPluginsFromDirectory(ildjitSystem->CLRPlugins, shutdown_clr_plugin, NULL);
    }
    ildjitSystem->CLRPlugins	= NULL;

    return ;
}

static inline void shutdown_clr_plugin (ildjit_plugin_t *plugin, void *data) {
    clr_t   *clr;

    clr     = (clr_t *)plugin;
    assert(clr != NULL);

    clr->plugin->shutdown();

    return;
}
