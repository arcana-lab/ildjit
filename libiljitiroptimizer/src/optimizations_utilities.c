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

#include <stdlib.h>
#include <ir_method.h>
#include <jitsystem.h>

// My headers
#include <optimizations_utilities.h>
// End

#define DIM_BUF 1024

JITBOOLEAN internal_isOptimizationInstalled (ir_optimizer_t *lib, JITUINT64 optimizationKind) {
    XanListItem	*item;

    /* Assertions.
     */
    assert(lib != NULL);

    item = xanList_first(lib->plugins);
    while (item != NULL) {
        ir_optimization_t	*plugin;

        /* Fetch a plugin.
         */
        plugin = (ir_optimization_t *) item->data;
        assert(plugin != NULL);
        assert(plugin->plugin != NULL);
        assert(plugin->plugin->do_job != NULL);

        /* Check whether the plugin is the one to be called.
         */
        if (plugin->plugin->get_job_kind() == optimizationKind) {
            return JITTRUE;
        }

        /* Fetch the next plugin.
         */
        item = item->next;
    }

    return JITFALSE;
}

JITBOOLEAN internal_isOptimizationEnabled (ir_optimizer_t *lib, JITUINT64 optimizationKind, JITBOOLEAN anyOneEnabled) {
    JITUINT64	tmp;
    JITBOOLEAN	*ptr;

    /* Assertions.
     */
    assert(lib != NULL);

    ptr	= &((lib->enabledOptimizations).ddg);
    tmp	= 0x1;
    while (JITTRUE) {

        /* Check if the current element belongs to the set requested.
         */
        if ((optimizationKind & tmp) != 0) {

            /* Check if the current element has been enabled.
             */
            if (*ptr) {
                break ;
            }

            /* The current element has not been enabled.
             * Check whether we need to look for any other element.
             */
            if (!anyOneEnabled) {
                break ;
            }
        }

        /* Fetch the next element.
         */
        tmp	= tmp << 1;
        ptr++;
    }
    if ((optimizationKind & tmp) != 0) {
        return *ptr;
    }

    return JITFALSE;
}

void internal_callMethodOptimization (ir_optimizer_t *lib, ir_method_t *method, JITUINT64 optimizationKind, JITBOOLEAN checkWhetherIsEnabled) {
    ir_optimization_t       *plugin;
    ir_instruction_t        *firstInst;
    JITUINT64 		dependences;

    /* Assertions						*/
    assert(lib != NULL);
    assert(method != NULL);
    assert(lib->plugins != NULL);

    /* Check if there is one plugin at least		*/
    if (xanList_length(lib->plugins) == 0) {
        return;
    }

    /* Check if the method has a body			*/
    if (!IRMETHOD_hasMethodInstructions(method)) {
        return;
    }

    /* Check if the optimization has been enabled		*/
    if (	(checkWhetherIsEnabled)							&&
            (!internal_isOptimizationEnabled(lib, optimizationKind, JITFALSE))	) {
        return ;
    }

    /* Check if we need to allocate memory for the          *
     * optimizations					*/
    firstInst = IRMETHOD_getFirstInstruction(method);
    if (    (firstInst != NULL)                     &&
            (firstInst->metadata == NULL)           ) {
        IRMETHOD_allocateMethodExtraMemory(method);
    }

    /* Call the plugin and the necessary dependences.
     */
    plugin	= xanHashTable_lookup(lib->jobCodeToolsMap, &optimizationKind);
    if (plugin == NULL) {
        char buf[DIM_BUF];
        snprintf(buf, DIM_BUF, "Optimization %s is missing. ", IROPTIMIZER_jobToString(optimizationKind));
        print_err(buf, 0);
        abort();
    }
    assert(plugin->plugin != NULL);
    assert(plugin->plugin->do_job != NULL);

    /* Fetch the dependences.
     */
    dependences = plugin->plugin->get_dependences();

    /* Satisfy the dependences.
     */
    while ((method->valid_optimization & dependences) != dependences) {
        JITUINT64	to_call;
        for (to_call = 1; to_call != 0; to_call = to_call << 1) {
            if (	(dependences & to_call)				&&
                    (!(method->valid_optimization & to_call))	) {
                assert(to_call != optimizationKind);

                /* Call the dependence.
                 */
                internal_callMethodOptimization(lib, method, to_call, JITFALSE);
                break;
            }
        }
    }
    method->valid_optimization &= ~(plugin->plugin->get_invalidations());
    plugin->plugin->do_job(method);
    method->valid_optimization |= optimizationKind;

    return;
}
