/*
 * Copyright (C) 2007 - 2012  Campanoni Simone
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
#include <optimizer_instructiontypes.h>
#include <dump_instructiontypes.h>
#include <config.h>
// End

#define INFORMATIONS 	"Profile the execution frequency of IR instructions"
#define	AUTHOR		"Simone Campanoni"
#define SKIP_USELESS_TYPE_CONVERSIONS

static inline JITUINT64 instructiontypes_get_ID_job (void);
static inline char * instructiontypes_get_version (void);
static inline void instructiontypes_do_job (ir_method_t * method);
static inline char * instructiontypes_get_informations (void);
static inline char * instructiontypes_get_author (void);
static inline JITUINT64 instructiontypes_get_dependences (void);
static inline void instructiontypes_shutdown (JITFLOAT32 totalTime);
static inline void instructiontypes_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix);
static inline JITUINT64 instructiontypes_get_invalidations (void);
static inline void instructiontypes_profile_method (ir_method_t *method);
static inline void instructiontypes_getCompilationTime (char *buffer, JITINT32 bufferLength);
static inline void instructiontypes_getCompilationFlags (char *buffer, JITINT32 bufferLength);

char 	        *trace_prefix		= NULL;
static		JITBOOLEAN	hasBeenExecuted;
static		XanHashTable	*profiledMethods;

ir_optimization_interface_t plugin_interface = {
	instructiontypes_get_ID_job		,
	instructiontypes_get_dependences	,
	instructiontypes_init			,
	instructiontypes_shutdown		,
	instructiontypes_do_job			, 
	instructiontypes_get_version		,
	instructiontypes_get_informations	,
	instructiontypes_get_author		,
	instructiontypes_get_invalidations	,
	NULL					,
	instructiontypes_getCompilationFlags	,
	instructiontypes_getCompilationTime
};

static inline char * instructiontypes_get_version (void){
	return VERSION;
}

static inline char * instructiontypes_get_informations (void){
	return INFORMATIONS;
}

static inline char * instructiontypes_get_author (void){
	return AUTHOR;
}

static inline JITUINT64 instructiontypes_get_invalidations (void){
	return INVALIDATE_ALL;
}

static inline JITUINT64 instructiontypes_get_ID_job (void){
	return CUSTOM;
}

static inline JITUINT64 instructiontypes_get_dependences (void){
	return 0;
}

static inline void instructiontypes_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix){
	JITUINT32 i;

	/* Assertions			*/
	assert(lib != NULL);
	assert(optimizer != NULL);
	assert(outputPrefix != NULL);

	trace_prefix	= "TRACE";
	if (outputPrefix != NULL){
		trace_prefix		= outputPrefix;
	}
	hasBeenExecuted	= JITFALSE;
	for (i=0; i < IREXITNODE; i++){
		profiled_data[i] = 0;
	}
	profiledMethods	= xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);

	return ;
}

static inline void instructiontypes_do_job (ir_method_t *method){
	XanList		*l;
	XanListItem	*item;

	/* Fetch the methods			*/
	l	= IRPROGRAM_getIRMethods();

	/* Profile the methods			*/
	item	= xanList_first(l);
	while (item != NULL){
		ir_method_t	*m;
		m	= item->data;
		if (IRMETHOD_hasMethodInstructions(m)){
			if (xanHashTable_lookup(profiledMethods, m) == NULL){
				xanHashTable_insert(profiledMethods, m, m);
				instructiontypes_profile_method(m);
			}
		}
		item	= item->next;
	}

	/* Free the memory			*/
	xanList_destroyList(l);

	return ;
}

static inline void instructiontypes_profile_method (ir_method_t *method){
	XanList *l;
	XanListItem *item;

	/* Assertions				*/
	assert(method != NULL);

	/* Set the variable			*/
	hasBeenExecuted	= JITTRUE;

	/* Fetch the list of instructions	*/
	l = IRMETHOD_getInstructions(method);

	/* Instrument the code			*/
	item = xanList_first(l);
	while (item != NULL){
		ir_instruction_t *inst;
		ir_instruction_t *load;
		ir_instruction_t *add;
		ir_instruction_t *store;
		ir_item_t	*par1;
		ir_item_t	*par2;

		/* Fetch the instruction			*/
		inst = item->data;

		/* Check if we need to profile the instruction	*/
		switch (IRMETHOD_getInstructionType(inst)){
			case IRLABEL:
			case IRPHI:
			case IRGETADDRESS:
			case IRNOP:
				break;

			case IRCONV:
				par1	= IRMETHOD_getInstructionParameter1(inst);
				par2	= IRMETHOD_getInstructionParameter2(inst);
				if (	(IRMETHOD_hasAnIntType(par1))					&&
					(IRMETHOD_hasAnIntType(par2))					&&
					(IRDATA_getSizeOfType(par1) == IRDATA_getSizeOfType(par2))	){
					#ifdef SKIP_USELESS_TYPE_CONVERSIONS
					break;
					#endif
				}

			default:

				/* Profile the instruction				*/
				load = IRMETHOD_newInstructionOfTypeBefore(method, inst, IRLOADREL);
				IRMETHOD_setInstructionParameter1(method, load, (IR_ITEM_VALUE)(JITNUINT)&(profiled_data[inst->type]), 0, IRNUINT, IRNUINT, NULL);
				IRMETHOD_setInstructionParameter2(method, load, 0, 0, IRINT32, IRINT32, NULL);
				IRMETHOD_setInstructionParameter3(method, load, IRUINT64, 0, IRTYPE, IRTYPE, NULL);
				IRMETHOD_setInstructionParameterWithANewVariable(method, load, IRUINT64, NULL, 0);

				add = IRMETHOD_newInstructionOfTypeAfter(method, load, IRADD);
				IRMETHOD_cpInstructionParameter(method, load, 0, add, 1);
				IRMETHOD_setInstructionParameter2(method, add, 1, 0, IRUINT64, IRUINT64, NULL);
				IRMETHOD_cpInstructionParameter(method, load, 0, add, 0);

				store = IRMETHOD_newInstructionOfTypeAfter(method, add, IRSTOREREL);
				IRMETHOD_setInstructionParameter1(method, store, (IR_ITEM_VALUE)(JITNUINT)&(profiled_data[inst->type]), 0, IRNUINT, IRNUINT, NULL);
				IRMETHOD_setInstructionParameter2(method, store, 0, 0, IRINT32, IRINT32, NULL);
				IRMETHOD_cpInstructionParameter(method, load, 0, store, 3);
		}

		item = item->next;
	}

	/* Free the memory			*/
	xanList_destroyList(l);

	/* Return				*/
	return;
}

static inline void instructiontypes_shutdown (JITFLOAT32 totalTime){

	/* Check if the plugin has been invoked	*/
	if (!hasBeenExecuted){
		return ;
	}

	/* Dump all instructions		*/
	dump_all_instruction_types(trace_prefix);

	/* Dump macro grouped instructions	*/
	dump_grouped_instruction_types(trace_prefix);

	return ;
}

static inline void instructiontypes_getCompilationFlags (char *buffer, JITINT32 bufferLength){

	/* Assertions				*/
	assert(buffer != NULL);

	snprintf(buffer, sizeof(char) * bufferLength, " ");
	#ifdef DEBUG
	strncat(buffer, "DEBUG ", sizeof(char) * bufferLength);
	#endif
	#ifdef PRINTDEBUG
	strncat(buffer, "PRINTDEBUG ", sizeof(char) * bufferLength);
	#endif
	#ifdef PROFILE
	strncat(buffer, "PROFILE ", sizeof(char) * bufferLength);
	#endif
}

static inline void instructiontypes_getCompilationTime (char *buffer, JITINT32 bufferLength){

	/* Assertions				*/
	assert(buffer != NULL);

	snprintf(buffer, sizeof(char) * bufferLength, "%s %s", __DATE__, __TIME__);

	/* Return				*/
	return;
}
