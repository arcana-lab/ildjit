/*
 * Copyright (C) 2007 - 2010  Campanoni Simone
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
#include <optimizer_false_data_dependences_elimination.h>
#include <config.h>
// End

#define INFORMATIONS 	"This is a dummy plugin"
#define	AUTHOR		"Campanoni Simone"

static inline JITUINT64 dummy_get_ID_job (void);
static inline JITUINT64 dummy_get_dependences (void);
static inline char * dummy_get_version (void);
static inline void dummy_do_job (ir_method_t * method);
static inline char * dummy_get_informations (void);
static inline char * dummy_get_author (void);
static inline void dummy_shutdown (void);
static inline void dummy_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix);
static void internal_substituteVariableInsideInstruction (ir_method_t *method, t_ir_instruction *inst, IR_ITEM_VALUE fromVarID, IR_ITEM_VALUE toVarID);
static inline XanHashTable * compute_definition_domains (ir_method_t *method);
void cluster_live_ranges (ir_method_t *method, XanHashTable *liveRanges);
void print_live_ranges (ir_method_t *method, XanHashTable *liveRanges);
void delete_single_definitions (ir_method_t *method, XanHashTable *liveRanges);
void rename_variables (ir_method_t *method, XanHashTable *liveRanges);
static inline void dummy_rename_variables (ir_method_t * method);

ir_lib_t	*irLib		= NULL;
ir_optimizer_t	*irOptimizer	= NULL;
char 	        *prefix		= NULL;

ir_optimization_interface_t plugin_interface = {
	dummy_get_ID_job	, 
	dummy_get_dependences	,
	dummy_init		,
	dummy_shutdown		,
	dummy_do_job		, 
	dummy_get_version	,
	dummy_get_informations	,
	dummy_get_author
};

static inline void dummy_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix){

	/* Assertions			*/
	assert(lib != NULL);
	assert(optimizer != NULL);
	assert(outputPrefix != NULL);

	irLib		= lib;
	irOptimizer	= optimizer;
	prefix		= outputPrefix;
}

static inline void dummy_shutdown (void){
        irLib		= NULL;
	irOptimizer	= NULL;
	prefix		= NULL;
}

static inline JITUINT64 dummy_get_ID_job (void){
	return VARIABLES_LIVE_RANGE_SPLITTING;
}

static inline JITUINT64 dummy_get_dependences (void){
	return 0;
}

static inline void dummy_do_job (ir_method_t * method){

	/* Assertions			*/
	assert(method != NULL);

	/* Rename the variables		*/
	dummy_rename_variables(method);

	/* Return			*/
	return ;
}

static inline void dummy_rename_variables (ir_method_t * method){
	XanHashTable 		*liveRanges;
	JITINT32               	instID;
	
	/* Assertions					*/
	assert(method != NULL);

	/* Compute the live ranges of each definitions	*/
	liveRanges	= compute_definition_domains(method);
	assert(liveRanges != NULL);
	
	/* Cluster the live ranges			*/
	cluster_live_ranges(method, liveRanges);

	/* Delete definition with only one live range	*/
	delete_single_definitions(method, liveRanges);

	/* Print the live ranges			*/
	#ifdef PRINTDEBUG
	print_live_ranges(method, liveRanges);
	#endif
	
	/* Rename the variables				*/
	rename_variables(method, liveRanges);

	/* Free the memory				*/
	for (instID=0; instID < IRMETHOD_getInstructionsNumber(method); instID++){
		t_ir_instruction	*inst;
		XanList			*domain;
		inst	= IRMETHOD_getInstructionAtPosition(method, instID);
		domain	= liveRanges->lookup(liveRanges, inst);
		if (domain == NULL) continue;
		domain->destroyList(domain);
	}
	liveRanges->destroy(liveRanges);

	/* Return					*/
	return ;
}

void rename_variables (ir_method_t *method, XanHashTable *liveRanges){
	t_ir_instruction	*inst;
	t_ir_instruction	*inst2;
	t_ir_instruction	*inst3;
	XanList			*domain;
	XanList			*domain2;
	XanListItem		*item;
	JITINT32               	instID;
	JITINT32               	instID2;
	JITINT32		varDef;
	JITINT32		varDef2;
	JITINT32		newVarID;

	for (instID=0; instID < IRMETHOD_getInstructionsNumber(method); instID++){
		inst	= IRMETHOD_getInstructionAtPosition(method, instID);
		varDef	= livenessGetDef(method, inst);
		domain	= liveRanges->lookup(liveRanges, inst);
		if (domain == NULL) continue;
		assert(varDef != -1);

		for (instID2=instID+1; instID2 < IRMETHOD_getInstructionsNumber(method); instID2++){
			inst2	= IRMETHOD_getInstructionAtPosition(method, instID2);
			varDef2	= livenessGetDef(method, inst2);
			domain2	= liveRanges->lookup(liveRanges, inst2);
			if (domain2 == NULL) continue;
			assert(varDef2 != -1);
			assert(domain != domain2);
			if (varDef == varDef2){

				/* Make a new variable		*/
				newVarID	= IRMETHOD_newVariableID(method);

				/* Rename the variable			*/	
				item	= domain2->first(domain2);
				while (item != NULL){
					inst3	= domain2->data(domain2, item);
					internal_substituteVariableInsideInstruction(method, inst3, varDef2, newVarID);
					item	= domain2->next(domain2, item);
				}

				/* Rename the variable defined by the 	*
				 * current definition			*/
				internal_substituteVariableInsideInstruction(method, inst2, varDef2, newVarID);

				/* Delete the current definition	*/
				liveRanges->delete(liveRanges, inst2);
			
				/* Free the memory			*/
				domain2->destroyList(domain2);
			}
		}
	}

	/* Return			*/
	return ;
}

void print_live_ranges (ir_method_t *method, XanHashTable *liveRanges){
	t_ir_instruction	*inst;
	t_ir_instruction	*inst2;
	XanList			*domain;
	XanListItem		*item;
	JITINT32               	instID;

	for (instID=0; instID < IRMETHOD_getInstructionsNumber(method); instID++){
		inst	= IRMETHOD_getInstructionAtPosition(method, instID);
		domain	= liveRanges->lookup(liveRanges, inst);
		if (domain == NULL) continue;
		printf("Method %s	Inst %d\n", IRMETHOD_getCompleteMethodName(method), inst->ID);
		item	= domain->first(domain);
		while (item != NULL){
			inst2	= domain->data(domain, item);
			printf("	Inst %d\n", inst2->ID);
			item	= domain->next(domain, item);
		}
	}

	/* Return					*/
	return ;
}

void delete_single_definitions (ir_method_t *method, XanHashTable *liveRanges){
	t_ir_instruction	*inst;
	t_ir_instruction	*inst2;
	XanList			*domain;
	XanList			*domain2;
	JITINT32               	instID;
	JITINT32               	instID2;
	JITINT32		varDef;
	JITINT32		varDef2;

	/* Cluster the live ranges			*/
	for (instID=0; instID < IRMETHOD_getInstructionsNumber(method); instID++){
		inst	= IRMETHOD_getInstructionAtPosition(method, instID);
		varDef	= livenessGetDef(method, inst);
		domain	= liveRanges->lookup(liveRanges, inst);
		if (domain == NULL) continue;
		assert(varDef != -1);

		for (instID2=0; instID2 < IRMETHOD_getInstructionsNumber(method); instID2++){
			if (instID == instID2) continue;
			inst2	= IRMETHOD_getInstructionAtPosition(method, instID2);
			varDef2	= livenessGetDef(method, inst2);
			domain2	= liveRanges->lookup(liveRanges, inst2);
			if (domain2 == NULL) continue;
			assert(varDef2 != -1);
			assert(domain != domain2);
			if (varDef == varDef2){
				break;
			}
		}

		/* Check if the current definition	*
		 * is a single one			*/
		if (instID2 == IRMETHOD_getInstructionsNumber(method)){
				
			/* Delete the definition		*/
			liveRanges->delete(liveRanges, inst);

			/* Free the memory			*/
			domain->destroyList(domain);
		}
	}
	
	/* Return					*/
	return ;
}

void cluster_live_ranges (ir_method_t *method, XanHashTable *liveRanges){
	t_ir_instruction	*inst;
	t_ir_instruction	*inst2;
	XanList			*domain;
	XanList			*domain2;
	JITINT32               	instID;
	JITINT32               	instID2;
	JITINT32		varDef;
	JITINT32		varDef2;

	/* Cluster the live ranges			*/
	for (instID=0; instID < IRMETHOD_getInstructionsNumber(method); instID++){
		inst	= IRMETHOD_getInstructionAtPosition(method, instID);
		varDef	= livenessGetDef(method, inst);
		domain	= liveRanges->lookup(liveRanges, inst);
		if (domain == NULL) continue;
		assert(varDef != -1);

		for (instID2=instID + 1; instID2 < IRMETHOD_getInstructionsNumber(method); instID2++){
			inst2	= IRMETHOD_getInstructionAtPosition(method, instID2);
			varDef2	= livenessGetDef(method, inst2);
			domain2	= liveRanges->lookup(liveRanges, inst2);
			if (domain2 == NULL) continue;
			assert(varDef2 != -1);
			assert(domain != domain2);
			if (	(varDef == varDef2)				&&
				(domain->shareSomeElements(domain, domain2))	){
				
				/* Cluster the live ranges	*/
				domain->append(domain, inst2);
				domain->appendList(domain, domain2);
				liveRanges->delete(liveRanges, inst2);
				assert(liveRanges->lookup(liveRanges, inst2) == NULL);
				
				/* Free the memory		*/
				domain2->destroyList(domain2);
			}
		}
	}

	/* Clean the lists				*/
	for (instID=0; instID < IRMETHOD_getInstructionsNumber(method); instID++){
		inst	= IRMETHOD_getInstructionAtPosition(method, instID);
		domain	= liveRanges->lookup(liveRanges, inst);
		if (domain == NULL) continue;
		domain->deleteClones(domain);
	}
	
	/* Return					*/
	return ;
}

static inline char * dummy_get_version (void){
	return VERSION;
}

static inline char * dummy_get_informations (void){
	return INFORMATIONS;
}

static inline char * dummy_get_author (void){
	return AUTHOR;
}

static void internal_substituteVariableInsideInstruction (ir_method_t *method, t_ir_instruction *inst, IR_ITEM_VALUE fromVarID, IR_ITEM_VALUE toVarID){
        ir_item_t               *item;
        JITUINT32               count2;

        /* Assertions                           */
        assert(method != NULL);
        assert(inst != NULL);

        /* Check the parameters			*/
        switch(method->getInstrParametersNumber(method, inst)){
                case 3:
                        item	= method->getInstrPar3(inst);
                        assert(item != NULL);
                        if (item->type == IROFFSET){
                                if (item->value == fromVarID){
                                        item->value     = toVarID;
                                }
                        }
                case 2:
                        item	= method->getInstrPar2(inst);
                        assert(item != NULL);
                        if (item->type == IROFFSET){
                                if (item->value == fromVarID){
                                        item->value     = toVarID;
                                }
                        }
                case 1:
                        item	= method->getInstrPar1(inst);
                        assert(item != NULL);
                        if (item->type == IROFFSET){
                                if (item->value == fromVarID){
                                        item->value     = toVarID;
                                }
                        }
                        break;
        }

        /* Check the result			*/
        item	= method->getInstrResult(inst);
        assert(item != NULL);
        if (item->type == IROFFSET){
                if (item->value == fromVarID){
                        item->value     = toVarID;
                }
        }

        /* Check the call parameters		*/
        for (count2 = 0; count2 < IRMETHOD_getInstructionCallParametersNumber(inst); count2++){
                if (method->getCallParameterType(inst, count2) == IROFFSET){
                        if (method->getCallParameterValue(inst, count2) == fromVarID){
                                method->setCallParameterValue(inst, count2, toVarID);
                        }
                }
        }

        /* Return                               */
        return;
}

static inline XanHashTable * compute_definition_domains (ir_method_t *method){
	t_ir_instruction	*inst;
	t_ir_instruction	*succ;
	JITINT32		varDef;
	JITUINT32		instID;
	JITUINT32		instructionsNumber;
	XanList			*domain;
	XanList			*todo;
	XanListItem		*item;
	XanHashTable		*liveRanges;

	/* Assertions						*/
	assert(method != NULL);

	todo		= xanListNew(allocFunction, freeFunction, NULL);
	liveRanges	= xanHashTableNew(1, 0, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);

	/* Fetch the number of instructions of the method	*/
	instructionsNumber	= IRMETHOD_getInstructionsNumber(method);

	/* Compute the live ranges				*/
	for (instID=0; instID < instructionsNumber; instID++){

		/* Fetch the next definition		*/
		inst	= IRMETHOD_getInstructionAtPosition(method, instID);
		varDef	= livenessGetDef(method, inst);
		if (varDef == -1) continue;

		/* Allocate the live range of the 	*
		 * current definition.			*/
		todo->emptyOutList(todo);
		domain	= xanListNew(allocFunction, freeFunction, NULL);
		liveRanges->insert(liveRanges, inst, domain);
		todo->insert(todo, inst);

		/* Fill out the current live range	*/
		while (todo->length(todo) > 0){
			
			/* Fetch the next instruction to consider	*/
			item	= todo->first(todo);
			inst	= todo->data(todo, item);
			todo->deleteItem(todo, item);

			/* Check its successors.			*/
			succ	= IRMETHOD_getSuccessorInstruction(method, inst, NULL);
			while (succ != NULL){
				if (	ir_instr_liveness_live_in_is(method, succ, varDef)	&&
					(domain->find(domain, succ) == NULL)			){
					if (todo->find(todo, succ) == NULL){
						todo->append(todo, succ);
					}
					domain->insert(domain, succ);
				}
				succ	= IRMETHOD_getSuccessorInstruction(method, inst, succ);
			}
		}
	}
	
	/* Free the memory			*/
	todo->destroyList(todo);

	/* Return				*/
	return liveRanges;
}
