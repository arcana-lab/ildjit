/*
 * Copyright (C) 2010-2011  Timothy M. Jones and Simone Campanoni
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
#include <jitsystem.h>
#include <xanlib.h>
#include <ir_language.h>
#include <ir_method.h>
#include <compiler_memory_manager.h>

// My headers
#include <chiara.h>
// End


/**
 * Convert from pointer to integer or back.
 **/
#define ptrToInt(ptr) ((JITINT32)(JITNINT)ptr)
#define ptrToUint(ptr) ((JITUINT32)(JITNUINT)ptr)
#define intToPtr(val) ((void *)(JITNINT)val)
#define uintToPtr(val) ((void *)(JITNUINT)val)


/**
 * Allocate a new list.
 **/
static inline XanList *
newList(void) {
    return xanList_new(allocFunction, freeFunction, NULL);
}


/**
 * Free a list.
 **/
static inline void
freeList(XanList *list) {
    xanList_destroyList(list);
}


/**
 * Set up parameters that are common to all profiling functions.
 **/
static void
setupCommonProfileInstParams(ir_method_t *method, ir_instruction_t *inst, ir_item_t *methodParam, ir_item_t *instParam, XanList *params) {
    memset(methodParam, 0, sizeof(ir_item_t));
    memset(instParam, 0, sizeof(ir_item_t));
    xanList_append(params, methodParam);
    xanList_append(params, instParam);
    methodParam->value.v = ptrToInt(method);
    methodParam->type = IRNUINT;
    methodParam->internal_type = methodParam->type;
    instParam->value.v = ptrToInt(inst);
    instParam->type = IRNUINT;
    instParam->internal_type = instParam->type;
}


/**
 * Add profiling code to a load or store instruction.
 **/
void
INSTS_profileLoadOrStore(ir_method_t *method, ir_instruction_t *inst, void *nativeMethod) {
    XanList *params = newList();
    ir_item_t *param1;
    ir_item_t *param2;
    ir_item_t methodParam;
    ir_item_t instParam;
    ir_item_t addrParam;
    ir_item_t sizeParam;

    /* Set up the common parameters. */
    setupCommonProfileInstParams(method, inst, &methodParam, &instParam, params);

    /* Require two additional parameters. */
    memset(&addrParam, 0, sizeof(ir_item_t));
    memset(&sizeParam, 0, sizeof(ir_item_t));
    xanList_append(params, &addrParam);
    xanList_append(params, &sizeParam);

    /* Original instruction parameters. */
    param1 = IRMETHOD_getInstructionParameter1(inst);
    param2 = IRMETHOD_getInstructionParameter2(inst);

    /* Number of bytes accessed. */
    sizeParam.type = IRUINT32;
    sizeParam.internal_type = sizeParam.type;
    if (inst->type == IRLOADREL) {
        sizeParam.value.v = IRDATA_getSizeOfType(IRMETHOD_getInstructionResult(inst));
    } else {
        sizeParam.value.v = IRDATA_getSizeOfType(IRMETHOD_getInstructionParameter3(inst));
    }

    /* Work out the address accessed. */
    if (param2->value.v == 0) {
        memcpy(&addrParam, param1, sizeof(ir_item_t));
    } else {
        ir_instruction_t *add = IRMETHOD_newInstructionOfTypeBefore(method, inst, IRADD);
        ir_item_t *addRes = IRMETHOD_getInstructionResult(add);
        IRMETHOD_cpInstructionParameter1(add, param1);
        IRMETHOD_cpInstructionParameter2(add, param2);
        IRMETHOD_setInstructionParameterWithANewVariable(method, add, param1->internal_type, param1->type_infos, 0);
        memcpy(&addrParam, addRes, sizeof(ir_item_t));
    }

    /* Add the new instruction. */
    if (inst->type == IRLOADREL) {
        IRMETHOD_newNativeCallInstructionBefore(method, inst, "ProfileLoad", nativeMethod, NULL, params);
    } else {
        ir_item_t allocParam;
        memset(&allocParam, 0, sizeof(ir_item_t));
        allocParam.type = IRINT16;
        allocParam.internal_type = allocParam.type;
        xanList_append(params, &allocParam);
        IRMETHOD_newNativeCallInstructionBefore(method, inst, "ProfileStore", nativeMethod, NULL, params);
    }
    freeList(params);
}
