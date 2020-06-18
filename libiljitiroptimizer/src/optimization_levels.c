/*
 * Copyright (C) 2008  Campanoni Simone
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
#include <ir_method.h>
#include <ir_optimizer.h>
#include <jitsystem.h>
#include <string.h>

// My headers
#include <optimization_levels.h>
#include <optimizations_utilities.h>
#include <ir_optimization_interface.h>
#include <ir_optimization_levels_interface.h>
// End

static ir_optimizer_t *libOptimizer = NULL;
static ir_optimization_levels_t *pl = NULL;
static pthread_once_t moduleLock = PTHREAD_ONCE_INIT;

static inline void internal_initialize (void);

void initialize_optimizationLevels_plugins (ir_optimizer_t *lib) {
    libOptimizer = lib;
    PLATFORM_pthread_once(&moduleLock, internal_initialize);

    return ;
}

JITUINT32 optimizeMethod_atLevel (ir_optimizer_t *lib, ir_method_t *method, JITUINT32 optimizationLevel, JITUINT32 state, XanVar *checkPoint) {
    libOptimizer = lib;
    PLATFORM_pthread_once(&moduleLock, internal_initialize);
    return pl->plugin->optimizeMethodAtLevel(lib, method, optimizationLevel, state, checkPoint);
}

JITUINT32 optimizeMethod_AOT_checkpointable (ir_optimizer_t *lib, ir_method_t *startMethod, JITUINT32 state, XanVar *checkPoint) {
    libOptimizer = lib;
    PLATFORM_pthread_once(&moduleLock, internal_initialize);
    return pl->plugin->optimizeMethodAOT(lib, startMethod, state, checkPoint);
}

JITUINT32 optimizeMethod_PostAOT_checkpointable (ir_optimizer_t *lib, ir_method_t *method, JITUINT32 state, XanVar *checkPoint) {
    libOptimizer = lib;
    PLATFORM_pthread_once(&moduleLock, internal_initialize);
    return pl->plugin->optimizeMethodPostAOT(lib, method, state, checkPoint);
}

static inline void internal_initialize (void) {
    if (pl == NULL) {
        XanListItem *item;
        item = xanList_first(libOptimizer->optimizationLevelsPlugins);
        if (item == NULL) {
            print_err("IROPTIMIZER: ERROR = No optimization levels plugins are available. ", 0);
            abort();
        }
        while (item != NULL) {
            ir_optimization_levels_t *tmpPl;
            tmpPl = item->data;
            assert(tmpPl != NULL);
            if (	(libOptimizer->optLevelToolName == NULL)						||
                    (STRCMP(libOptimizer->optLevelToolName, tmpPl->plugin->getName()) == 0)			) {
                pl	= tmpPl;
                break ;
            }
            item	= item->next;
        }
        if (pl == NULL) {
            fprintf(stderr, "ERROR: the optimizationleveltool called \"%s\" does not exist.\n", libOptimizer->optLevelToolName);
            abort();
        }
        pl->plugin->init(libOptimizer, NULL);
        if (libOptimizer->verbose) {
            printf("ILDJIT: Use the optimization levels plugin \"%s\"\n", pl->plugin->getInformation());
        }
    }
    assert(pl != NULL);

    return ;
}
