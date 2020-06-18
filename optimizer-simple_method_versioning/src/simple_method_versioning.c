/*
 * Copyright (C) 2007 - 2011  Campanoni Simone
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
#include <assert.h>
#include <errno.h>
#include <ir_optimization_interface.h>
#include <ir_language.h>
#include <iljit-utils.h>
#include <compiler_memory_manager.h>

// My headers
#include <simple_method_versioning.h>
#include <config.h>
// End

#define INFORMATIONS 	"This is a simple method versioning plugin"
#define	AUTHOR		"Campanoni Simone"

static inline JITUINT64 simple_method_versioning_get_ID_job (void);
static inline char * simple_method_versioning_get_version (void);
static inline void simple_method_versioning_do_job (ir_method_t * method);
static inline char * simple_method_versioning_get_informations (void);
static inline char * simple_method_versioning_get_author (void);
static inline JITUINT64 simple_method_versioning_get_dependences (void);
static inline void simple_method_versioning_shutdown (JITFLOAT32 totalTime);
static inline void simple_method_versioning_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix);
static inline JITUINT64 simple_method_versioning_get_invalidations (void);
void myProfile (ir_method_t *method, ir_instruction_t *inst, JITUINT32 value);

ir_lib_t	*irLib		= NULL;
ir_optimizer_t	*irOptimizer	= NULL;
char 	        *prefix		= NULL;
static XanHashTable *h ;

ir_optimization_interface_t plugin_interface = {
	simple_method_versioning_get_ID_job	, 
	simple_method_versioning_get_dependences	,
	simple_method_versioning_init		,
	simple_method_versioning_shutdown		,
	simple_method_versioning_do_job		, 
	simple_method_versioning_get_version	,
	simple_method_versioning_get_informations	,
	simple_method_versioning_get_author,
	simple_method_versioning_get_invalidations
};

static inline void simple_method_versioning_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix){

	/* Assertions			*/
	assert(lib != NULL);
	assert(optimizer != NULL);
	assert(outputPrefix != NULL);

	irLib		= lib;
	irOptimizer	= optimizer;
	prefix		= outputPrefix;
	
	h = xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
}

static inline void simple_method_versioning_shutdown (JITFLOAT32 totalTime){
        irLib		= NULL;
	irOptimizer	= NULL;
	prefix		= NULL;
}

static inline JITUINT64 simple_method_versioning_get_ID_job (void){
	return CUSTOM;
}

static inline JITUINT64 simple_method_versioning_get_dependences (void){
	return 0;
}

static inline void simple_method_versioning_do_job (ir_method_t * method){
	XanList		*l;
	XanListItem	*item;

	/* Assertions				*/
	assert(method != NULL);

	/* Fetch the call instructions		*/
	l	= IRMETHOD_getInstructionsOfType(method, IRCALL);

	/* Inject code to profile		*/
	item	= xanList_first(l);
	while (item != NULL){
		ir_instruction_t	*inst;
		ir_method_t 		*calledMethod;
		inst	= item->data;
		calledMethod = IRMETHOD_getCalledMethod(method, inst);

		if (	(!IRPROGRAM_doesMethodBelongToALibrary(calledMethod))	&&
			(!IRPROGRAM_doesMethodBelongToALibrary(method))		&&
			(IRMETHOD_getMethodParametersNumber(calledMethod) >= 1)	&&
			IRMETHOD_isInstructionCallParameterAVariable(inst, 0)	){
			ir_item_t *par;
			par = IRMETHOD_getInstructionCallParameter(inst, 0);
			if (par->internal_type == IRUINT32){
				XanList	*pars;
				ir_item_t methodPar;
				ir_item_t instPar;

				pars	= xanList_new(allocFunction, freeFunction, NULL);
				memset(&instPar, 0, sizeof(ir_item_t));
				memset(&methodPar, 0, sizeof(ir_item_t));

				methodPar.type = IRNUINT;
				methodPar.internal_type = IRNUINT;
				methodPar.value.v = (IR_ITEM_VALUE)(JITNUINT)method;

				instPar.type = IRNUINT;
				instPar.internal_type = IRNUINT;
				instPar.value.v = (IR_ITEM_VALUE)(JITNUINT)inst;

				xanList_append(pars, &methodPar);
				xanList_append(pars, &instPar);
				xanList_append(pars, par);

				IRMETHOD_newNativeCallInstructionBefore(method, inst, "myProfile", myProfile, NULL, pars);

				xanList_destroyList(pars);
			}
		}

		item	= item->next;
	}

	/* Return				*/
	return;
}

static inline char * simple_method_versioning_get_version (void){
	return VERSION;
}

static inline char * simple_method_versioning_get_informations (void){
	return INFORMATIONS;
}

static inline char * simple_method_versioning_get_author (void){
	return AUTHOR;
}

static inline JITUINT64 simple_method_versioning_get_invalidations (void){
	return INVALIDATE_ALL;
}

void myProfile (ir_method_t *method, ir_instruction_t *inst, JITUINT32 value){
	ir_item_t *par;	
	ir_item_t cons;
	ir_item_t *origMethodPar;
	ir_item_t versMethodPar;
	ir_instruction_t *condition;
	ir_instruction_t *label;
	ir_instruction_t *end_label;
	ir_instruction_t *cond_branch;
	ir_instruction_t *versioned_call;
	ir_instruction_t *move;
	ir_instruction_t *firstInst;
	ir_method_t *versionedMethod;
	ir_method_t *calledMethod;
	JITUINT32 methodNameLen;
	JITINT8 *methodName;

	XanHashTable *versions;
	versions = xanHashTable_lookup(h, inst);
	if (versions != NULL){
		if (xanHashTable_lookup(versions, (void *)(JITNUINT)(value + 1)) != NULL){
			return ;
		}
	} else {
		versions = xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
		xanHashTable_insert(h, inst, versions);
	}

	memset(&cons, 0, sizeof(ir_item_t));
	cons.type = IRUINT32;
	cons.internal_type = IRUINT32;
	cons.value.v = value;

	methodNameLen	= STRLEN(method->name) + 4 + 1 + 10;
	methodName	= allocFunction(methodNameLen);
	SNPRINTF(methodName, methodNameLen, "%s_ver_%u", method->name, value);
	calledMethod	= IRMETHOD_getCalledMethod(method, inst);
	versionedMethod = IRMETHOD_cloneMethod(calledMethod, methodName);
	xanHashTable_insert(versions, (void *)(JITNUINT)(value + 1), versionedMethod);

	fprintf(stderr, "New version\n");
	fprintf(stderr, "	Callee method: %s\n", IRMETHOD_getSignatureInString(method));
	fprintf(stderr, "	Called method: %s\n", IRMETHOD_getSignatureInString(calledMethod));
	fprintf(stderr, "	Runtime constant: %d\n", value);

	((versionedMethod->signature).parameters_number)--;
	if ((versionedMethod->signature).parameters_number > 0){
		memmove((versionedMethod->signature).parameter_internal_types, &((versionedMethod->signature).parameter_internal_types[1]), (versionedMethod->signature).parameters_number * sizeof(JITUINT32));
		memmove((versionedMethod->signature).type_infos, &((versionedMethod->signature).type_infos[1]), (versionedMethod->signature).parameters_number * sizeof(TypeDescriptor *));
	}

	firstInst = IRMETHOD_getFirstInstructionNotOfType(versionedMethod, IRNOP);
	move = IRMETHOD_newInstructionOfTypeBefore(versionedMethod, firstInst, IRMOVE);
	IRMETHOD_setInstructionResult(versionedMethod, move, 0, 0, IROFFSET, IRUINT32, NULL);
	IRMETHOD_cpInstructionParameter1(move, &cons);

	IRMETHOD_destroyMethodExtraMemory(versionedMethod);
	IRMETHOD_allocateMethodExtraMemory(versionedMethod);
	IROPTIMIZER_optimizeMethod(irOptimizer, versionedMethod);

	par = IRMETHOD_getInstructionCallParameter(inst, 0);

	condition = IRMETHOD_newInstructionOfTypeBefore(method, inst, IREQ);
	IRMETHOD_cpInstructionParameter1(condition, par);
	IRMETHOD_cpInstructionParameter2(condition, &cons);
	IRMETHOD_setInstructionParameterWithANewVariable(method, condition, IRINT32, NULL, 0);

	label = IRMETHOD_newLabelAfter(method, condition);
	end_label = IRMETHOD_newLabelAfter(method, inst);
	
	cond_branch = IRMETHOD_newInstructionOfTypeBefore(method, label, IRBRANCHIFNOT);
	IRMETHOD_cpInstructionParameter(method, condition, 0, cond_branch, 1);
	IRMETHOD_cpInstructionParameter(method, label, 1, cond_branch, 2);

	versioned_call = IRMETHOD_cloneInstruction(method, inst);
	IRMETHOD_deleteInstructionCallParameter(method, versioned_call, 0);
	origMethodPar = IRMETHOD_getInstructionParameter1(inst);
	memcpy(&versMethodPar, origMethodPar, sizeof(ir_item_t));
	versMethodPar.value.v = IRMETHOD_getMethodID(versionedMethod);
	IRMETHOD_cpInstructionParameter1(versioned_call, &versMethodPar);

	IRMETHOD_translateMethodToMachineCode(versionedMethod);
	IRMETHOD_translateMethodToMachineCode(method);
}
