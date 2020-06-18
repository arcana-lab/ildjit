/*
 * Copyright (C) 2007 - 2009  Campanoni Simone
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
#include <optimizer_loop_naturalization.h>
#include <config.h>
// End

#define INFORMATIONS 	"This is a loop_naturalization plugin"
#define	AUTHOR		"Campanoni Simone"

static inline JITUINT64 loop_naturalization_get_ID_job (void);
static inline char * loop_naturalization_get_version (void);
static inline void loop_naturalization_do_job (ir_method_t * method);
static inline char * loop_naturalization_get_informations (void);
static inline char * loop_naturalization_get_author (void);
static inline JITUINT64 loop_naturalization_get_dependences (void);
static inline void loop_naturalization_shutdown (void);
static inline void loop_naturalization_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix);
static inline void merge_loops_with_the_same_header (ir_method_t *method);
static inline void redirect_back_edge_to_label (ir_method_t *method, t_loop *loop, t_ir_instruction *label);
static inline void compute_method_information (ir_method_t *method);

ir_lib_t	*irLib		= NULL;
ir_optimizer_t	*irOptimizer	= NULL;
char 	        *prefix		= NULL;

ir_optimization_interface_t plugin_interface = {
	loop_naturalization_get_ID_job	, 
	loop_naturalization_get_dependences	,
	loop_naturalization_init		,
	loop_naturalization_shutdown		,
	loop_naturalization_do_job		, 
	loop_naturalization_get_version	,
	loop_naturalization_get_informations	,
	loop_naturalization_get_author
};

static inline void loop_naturalization_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix){

	/* Assertions			*/
	assert(lib != NULL);
	assert(optimizer != NULL);
	assert(outputPrefix != NULL);

	irLib		= lib;
	irOptimizer	= optimizer;
	prefix		= outputPrefix;
}

static inline void loop_naturalization_shutdown (void){
        irLib		= NULL;
	irOptimizer	= NULL;
	prefix		= NULL;
}

static inline JITUINT64 loop_naturalization_get_ID_job (void){
	return LOOP_NATURALIZATION;
}

static inline JITUINT64 loop_naturalization_get_dependences (void){
	return LOOP_IDENTIFICATION;
}

static inline void loop_naturalization_do_job (ir_method_t * method){

	/* Assertions				*/
	assert(method != NULL);

	/* Merge different loops with the same	*
	 * header at the same level of the loop	*
	 * nesting tree				*/
	merge_loops_with_the_same_header(method);

	/* Return				*/
	return;
}

static inline void merge_loops_with_the_same_header (ir_method_t *method){
	XanListItem	*item;
	XanListItem	*item2;
	t_loop		*loop;
	t_loop		*loop2;
	JITINT32	found;

	/* Assertions				*/
	assert(method != NULL);

	/* Merge loops with the same header	*/
	found	= JITTRUE;
	while (found){
		found	= JITFALSE;

		/* Iterate over the loops		*/
		item	= method->loop->first(method->loop);
		while (item != NULL){
			loop	= (t_loop *) method->loop->data(method->loop, item);
			assert(loop != NULL);
			item2	= method->loop->first(method->loop);
			while (item2 != NULL){
				loop2	= (t_loop *) method->loop->data(method->loop, item2);
				assert(loop2 != NULL);
				if (loop != loop2){
					
					/* Check if these two loops share the same header	*/
					if (	(loop->header_id == loop2->header_id)						&&
						(IRMETHOD_getParentLoop(method, loop) == IRMETHOD_getParentLoop(method, loop2))	){
						t_ir_instruction	*newBranch;
						t_ir_instruction	*newLabel;
						t_ir_instruction	*header;
						IR_ITEM_VALUE		labelID;
						assert((loop->backEdge).dst == (loop2->backEdge).dst);

						/* Fetch the header of the loop				*/
						header		= IRMETHOD_getInstructionAtPosition(method, loop->header_id);
						assert(header != NULL);
						assert(method->getInstructionType(method, header) == IRLABEL);
		
						/* Add a new label and a new branch to the header	*/
						newLabel	= IRMETHOD_newInstruction(method);
						newBranch	= IRMETHOD_newInstructionAfter(method, newLabel);
						labelID		= IRMETHOD_newLabelID(method);
						IRMETHOD_setInstructionType( newLabel, IRLABEL);
						method->setInstrPar1(newLabel, labelID, 0, IRLABELITEM, IRLABELITEM, JITFALSE, NULL);
						IRMETHOD_setInstructionType( newBranch, IRBRANCH);
						method->cpInstrPar1(newBranch, method->getInstrPar1(header));

						/* Redirect the two back edges to the new label instead	*
						 * of the header of the loop				*/
						redirect_back_edge_to_label(method, loop, newLabel);
						redirect_back_edge_to_label(method, loop2, newLabel);

						/* Recompute every information of the method		*/
						compute_method_information(method);

						/* Set the tag						*/
						found		= JITTRUE;
						break;
					}
				}
				item2	= method->loop->next(method->loop, item2);
			}
			if (found) break;
			item	= method->loop->next(method->loop, item);
		}
	}

	/* Return				*/
	return ;
}

static inline void redirect_back_edge_to_label (ir_method_t *method, t_loop *loop, t_ir_instruction *label){
	t_ir_instruction	*inst;
	t_ir_instruction	*newBranch;
	t_ir_instruction	*header;

	/* Assertions				*/
	assert(method != NULL);
	assert(loop != NULL);
	assert(method->getInstructionType(method, label) == IRLABEL);

	header	= IRMETHOD_getInstructionAtPosition(method, loop->header_id);
	inst	= (loop->backEdge).src;
	switch(method->getInstructionType(method, inst)){
		case IRBRANCH:
			method->cpInstrPar1(inst, method->getInstrPar1(label));
			break;
		case IRBRANCHIF:
		case IRBRANCHIFNOT:
			if (method->getInstrPar2Value(inst) == method->getInstrPar1Value(header)){
				method->cpInstrPar2(inst, method->getInstrPar1(label));
			} else {
				newBranch	= IRMETHOD_newInstructionAfter(method, inst);
				IRMETHOD_setInstructionType( newBranch, IRBRANCH);
				method->cpInstrPar1(newBranch, method->getInstrPar1(label));
			}
			break;
		default:
			assert(!IRMETHOD_isAJumpInstruction(method, inst));
			newBranch	= IRMETHOD_newInstructionAfter(method, inst);
			IRMETHOD_setInstructionType( newBranch, IRBRANCH);
			method->cpInstrPar1(newBranch, method->getInstrPar1(label));
	}

	/* Return				*/
	return ;
}

static inline void compute_method_information (ir_method_t *method){

	/* Assertions				*/
	assert(method != NULL);

	/* Compute method information		*/
	IROPTIMIZER_callMethodOptimization(irOptimizer, method, irLib, BASIC_BLOCK_IDENTIFIER);
	IROPTIMIZER_callMethodOptimization(irOptimizer, method, irLib, PRE_DOMINATOR_COMPUTER);
	IROPTIMIZER_callMethodOptimization(irOptimizer, method, irLib, LOOP_IDENTIFICATION);

	/* Return				*/
	return ;
}

static inline char * loop_naturalization_get_version (void){
	return VERSION;
}

static inline char * loop_naturalization_get_informations (void){
	return INFORMATIONS;
}

static inline char * loop_naturalization_get_author (void){
	return AUTHOR;
}
