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
#include <assert.h>
#include <ir_optimization_interface.h>
#include <iljit-utils.h>
#include <jitsystem.h>
#include <ir_language.h>
#include <compiler_memory_manager.h>
#include <platform_API.h>
#include <ildjit.h>

// My headers
#include <optimizer_simplemethodcloning.h>
#include <config.h>
// End

#define INFORMATIONS 	"This is a simplemethodcloning plugin"
#define	AUTHOR		"Simone Campanoni"
#define JOB            	CUSTOM

static inline JITUINT64 simplemethodcloning_get_ID_job (void);
static inline char * simplemethodcloning_get_version (void);
static inline void simplemethodcloning_do_job (ir_method_t * method);
static inline char * simplemethodcloning_get_informations (void);
static inline char * simplemethodcloning_get_author (void);
static inline JITUINT64 simplemethodcloning_get_dependences (void);
static inline void simplemethodcloning_shutdown (JITFLOAT32 totalTime);
static inline void simplemethodcloning_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix);
static inline JITUINT64 simplemethodcloning_get_invalidations (void);
static inline void simplemethodcloning_getCompilationTime (char *buffer, JITINT32 bufferLength);
static inline void simplemethodcloning_getCompilationFlags (char *buffer, JITINT32 bufferLength);
static inline void simplemethodcloning_start_execution (void);
void my_injected_code (char *methodCalled);

ir_lib_t	*irLib		= NULL;
ir_optimizer_t	*irOptimizer	= NULL;
char 	        *prefix		= NULL;

ir_optimization_interface_t plugin_interface = {
	simplemethodcloning_get_ID_job		,
	simplemethodcloning_get_dependences	,
	simplemethodcloning_init		,
	simplemethodcloning_shutdown		,
	simplemethodcloning_do_job		, 
	simplemethodcloning_get_version		,
	simplemethodcloning_get_informations	,
	simplemethodcloning_get_author		,
	simplemethodcloning_get_invalidations	,
	simplemethodcloning_start_execution	,
	simplemethodcloning_getCompilationFlags	,
	simplemethodcloning_getCompilationTime
};

static inline void simplemethodcloning_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix){

	/* Assertions			*/
	assert(lib != NULL);
	assert(optimizer != NULL);
	assert(outputPrefix != NULL);

	irLib		= lib;
	irOptimizer	= optimizer;
	prefix		= outputPrefix;
}

static inline void simplemethodcloning_shutdown (JITFLOAT32 totalTime){
        irLib		= NULL;
	irOptimizer	= NULL;
	prefix		= NULL;
}

static inline JITUINT64 simplemethodcloning_get_ID_job (void){
	return JOB;
}

static inline JITUINT64 simplemethodcloning_get_dependences (void){
	return 0;
}

static inline void simplemethodcloning_do_job (ir_method_t * method){
	XanList			*callInsts;
	XanListItem		*item;
	compilation_scheme_t	s;

	/* Assertions.
	 */
	assert(method != NULL);

	/* Check that we have been called in a static compilation scheme.
	 */
	s	= ILDJIT_compilationSchemeInUse();
	if (!ILDJIT_isAStaticCompilationScheme(s)){
		return ;
	}

	/* Fetch the set of directed call instructions.
	 */
	callInsts	= IRMETHOD_getInstructionsOfType(method, IRCALL);

	/* Clone all callees.
	 */
	item		= xanList_first(callInsts);
	while (item != NULL){
		ir_instruction_t	*i;
		ir_method_t 		*callee;

		i	= item->data;
		assert(i != NULL);

		callee	= IRMETHOD_getCalledMethod(method, i);
		if (	(callee != NULL)				&&
			(IRMETHOD_hasMethodInstructions(callee))	){
			ir_method_t 		*clone;
			ir_instruction_t	*firstInst;
			ir_instruction_t	*nativeCall;
			ir_item_t		callPar;
			JITINT8			*name;
			JITINT8			*origName;
			JITUINT32		nameLen;

			/* Create a new name for the clone we are going to create.
		  	 */
			origName	= IRMETHOD_getSignatureInString(callee);
			if (origName != NULL){
				ir_item_t	*retItem;

				nameLen		= (STRLEN(origName) + 7);
				name		= allocFunction(sizeof(char) * nameLen);
				SNPRINTF(name, nameLen, "%s_clone", origName);

				/* Clone the method.
				 */
				clone	= IRMETHOD_cloneMethod(callee, name);
				assert(clone != NULL);

				/* Inject some code at the beginning of the method.
				 */
				firstInst	= IRMETHOD_getFirstInstructionNotOfType(clone, IRNOP);
				assert(firstInst != NULL);

				memset(&callPar, 0, sizeof(ir_item_t));
				callPar.internal_type	= IRNUINT;
				callPar.type		= callPar.internal_type;
				callPar.value.v		= (IR_ITEM_VALUE)(JITNUINT)name;
				nativeCall		= IRMETHOD_newNativeCallInstructionBefore(clone, firstInst, "injected_code", my_injected_code, NULL, NULL);
				IRMETHOD_addInstructionCallParameter(clone, nativeCall, &callPar);

				/* Redirect the original call to the new cloned method.
				 */
				retItem			= IRMETHOD_getInstructionResult(i);
				IRMETHOD_newCallInstructionAfter(method, clone, i, retItem, i->callParameters);
			}
		}
		
		item	= item->next;
	}

	/* Free the memory.
	 */
	xanList_destroyList(callInsts);

	return;
}

static inline char * simplemethodcloning_get_version (void){
	return VERSION;
}

static inline char * simplemethodcloning_get_informations (void){
	return INFORMATIONS;
}

static inline char * simplemethodcloning_get_author (void){
	return AUTHOR;
}

static inline JITUINT64 simplemethodcloning_get_invalidations (void){
	return INVALIDATE_ALL;
}

static inline void simplemethodcloning_getCompilationFlags (char *buffer, JITINT32 bufferLength){

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

static inline void simplemethodcloning_getCompilationTime (char *buffer, JITINT32 bufferLength){

	/* Assertions				*/
	assert(buffer != NULL);

	snprintf(buffer, sizeof(char) * bufferLength, "%s %s", __DATE__, __TIME__);

	/* Return				*/
	return;
}

static inline void simplemethodcloning_start_execution (void){
	return ;
}

void my_injected_code (char *methodCalled){
	printf("Starting cloned method %s\n", methodCalled);
	return ;
}
