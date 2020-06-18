/*
 * Copyright (C) 2012 - 2013 Campanoni Simone
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABIRITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ir_language.h>
#include <metadata_manager.h>
#include <compiler_memory_manager.h>
#include <platform_API.h>
#include <base_symbol.h>

// My headers
#include <ir_method.h>
#include <codetool_types.h>
#include <ir_data.h>
// End

extern ir_lib_t * ir_library;

XanList * IRDATA_getGlobals (void) {
    return xanHashTable_toList(ir_library->globals);
}

ir_global_t * IRGLOBAL_createGlobal (JITINT8 *globalName, JITUINT16 irType, JITUINT64 size, JITBOOLEAN isConstant, void *initialValue) {
    ir_global_t	*g;

    /* Check the inputs.
     */
    if (globalName == NULL) {
        fprintf(stderr, "ILDJIT: ERROR in IRGLOBAL_createGlobal = A global must have a name\n");
        abort();
    }
    if (size == 0) {
        fprintf(stderr, "ILDJIT: ERROR in IRGLOBAL_createGlobal = A global must include at least one Byte\n");
        abort();
    }

    /* Create the global.
     */
    g		= allocFunction(sizeof(ir_global_t));
    g->IRType	= irType;
    g->name		= (JITINT8 *)strdup((char *)globalName);
    g->size		= size;
    g->isConstant	= isConstant;
    if (initialValue != NULL) {
        g->initialValue	= allocFunction(size);
        memcpy(g->initialValue, initialValue, size);
    }

    /* Keep track of the global.
     */
    xanHashTable_insert(ir_library->globals, g, g);

    return g;
}

void IRGLOBAL_storeGlobalToInstructionParameter (ir_method_t *method, ir_instruction_t *inst, JITUINT8 toParam, ir_global_t *global, TypeDescriptor *typeInfos) {
    ir_item_t	*par;

    /* Assertions.
     */
    assert(method != NULL);
    assert(IRMETHOD_doesInstructionBelongToMethod(method, inst));

    /* Set the modified flag.
     */
    method->modified	= JITTRUE;

    /* Fetch the parameter.
     */
    par	= IRMETHOD_getInstructionParameter(inst, toParam);
    if (par == NULL) {
        fprintf(stderr, "ERROR: IRGLOBAL_storeGlobalToInstructionParameter: IR parameter %u does not exist.\n", toParam);
        abort();
    }

    /* Store the global.
     */
    IRGLOBAL_storeGlobalToIRItem(par, global, typeInfos);

    return ;
}

void IRGLOBAL_storeGlobalToIRItem (ir_item_t *item, ir_global_t *global, TypeDescriptor *typeInfos) {
    memset(item, 0, sizeof(ir_item_t));

    (item->value).v		= (IR_ITEM_VALUE)(JITNUINT)global;
    item->type		= IRGLOBAL;
    item->internal_type	= global->IRType;
    item->type_infos	= typeInfos;

    return ;
}

JITBOOLEAN IRDATA_isAGlobalVariable (ir_item_t *item) {
    if (internal_isATaggedSymbol(item, STATIC_OBJECT_SYMBOL)) {
        return JITTRUE;
    }
    if (item->type == IRGLOBAL) {
        return JITTRUE;
    }
    return JITFALSE;
}

void * IRDATA_getGlobalVariableEffectiveAddress (ir_item_t *item) {
    ir_item_t	resolvedItem;
    void		*globalVarAddress;

    /* Check whether the parameter contains the address of a global variable.
     */
    if (!IRDATA_isAGlobalVariable(item)) {
        return NULL;
    }
    if (item->type == IRGLOBAL) {
        return NULL;
    }

    /* Check whether the IR item is a symbol and therefore it needs to be resolved.
     */
    if (IRDATA_isASymbol(item)) {

        /* Resolve the symbol.
         */
        IRSYMBOL_resolveSymbolFromIRItem(item, &resolvedItem);

    } else {
        memcpy(&resolvedItem, item, sizeof(ir_item_t));
    }

    /* Fetch the address.
     */
    globalVarAddress	= (void *)(JITNUINT)(resolvedItem.value.v);

    return globalVarAddress;
}
