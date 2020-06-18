/*
 * Copyright (C) 2008 Simone Campanoni, William Castiglioni
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
#include <assert.h>
#include <ir_method.h>
#include <ir_optimization_interface.h>
#include <jitsystem.h>
#include <ir_language.h>
#include <xanlib.h>
#include <compiler_memory_manager.h>
#include <chiara.h>

// My headers
#include <loop_invariant_hoisting.h>
#include <config.h>
// End

#define INFORMATIONS 	        "This step compute loop invariant code hoisting transformation"
#define	AUTHOR			"Simone Campanoni"

static inline JITUINT64 loop_invariant_get_ID_job (void);
static inline JITUINT64 loop_invariant_get_dependences (void);
static inline void loop_invariant_init (ir_lib_t * lib, ir_optimizer_t * optimizer, char *outputPrefix);
static inline void loop_invariant_shutdown (JITFLOAT32 totalTime);
static inline void loop_invariant_do_job (ir_method_t * method);
static inline JITUINT64 loop_invariant_get_invalidations (void);
static inline char *get_version (void);
static inline char * get_informations (void) ;
static inline char *get_author (void);
static inline JITBOOLEAN internal_move_loop_invariant (loop_t *loop);

ir_lib_t *irLib = NULL;
ir_optimizer_t	*irOptimizer	= NULL;

ir_optimization_interface_t plugin_interface = {
  loop_invariant_get_ID_job,
  loop_invariant_get_dependences,
  loop_invariant_init,
  loop_invariant_shutdown,
  loop_invariant_do_job,
  get_version,
  get_informations,
  get_author,
  loop_invariant_get_invalidations
};

static inline JITUINT64 loop_invariant_get_invalidations (void){
	return INVALIDATE_ALL;
}

static inline void loop_invariant_init (ir_lib_t * lib, ir_optimizer_t * optimizer, char *outputPrefix){
  irLib 	= lib;
  irOptimizer	= optimizer;
}

static inline void loop_invariant_shutdown (JITFLOAT32 totalTime) {
  irLib = NULL;
}

static inline JITUINT64 loop_invariant_get_ID_job (void) {
  return LOOP_INVARIANT_CODE_HOISTING;
}

static inline JITUINT64 loop_invariant_get_dependences (void) {
  return LIVENESS_ANALYZER | REACHING_DEFINITIONS_ANALYZER | LOOP_IDENTIFICATION;
}

static inline void loop_invariant_do_job (ir_method_t * method){
	XanHashTable *loops;
	XanNode *loopsTree;
	XanList *loopsOrder;
	XanListItem *item;

	/* Check if the method has some loops		*/
	if (	(!IRMETHOD_hasMethodInstructions(method))	||
		(!IRMETHOD_hasMethodLoops(method))		){
		return;
	}

	/* Fetch the list of loops			*/
	loops = LOOPS_analyzeMethodLoops(irOptimizer, method);
	assert(loops != NULL);

	/* Fetch the tree of loops			*/
	loopsTree = LOOPS_getMethodLoopsNestingTree(loops, method);
	assert(loopsTree != NULL);

	/* Fetch the order to consider the loops	*/
	loopsOrder = loopsTree->toPreOrderList(loopsTree);
	assert(loopsOrder != NULL);

	/* Optimize the loops				*/
	item = xanList_first(loopsOrder);
	while (item != NULL){
		loop_t *loop;

		/* Fetch the loop				*/
		loop = item->data;
		if (loop != NULL){

			/* Optimize the loop				*/
			internal_move_loop_invariant(loop);
		}

		/* Fetch the next loop				*/
		item = item->next;
	}

	/* Free the memory				*/
	LOOPS_destroyLoops(loops);
	xanList_destroyList(loopsOrder);
	loopsTree->destroyTree(loopsTree);

	/* Return					*/
	return ;
}

static inline JITBOOLEAN internal_move_loop_invariant (loop_t *loop){
	XanListItem *item;
	ir_instruction_t *preheader;

	/* Assertions					*/
	assert(loop != NULL);

	/* Check if there are invariants		*/
	if (loop->invariants == NULL){
		return JITFALSE;
	}
	if (xanList_length(loop->invariants) == 0) {
		return JITFALSE;
	}

	/* Peel out the first iteration			*/
	LOOPS_peelOutFirstIteration(loop);

	/* Add the preheader				*/	
	preheader = LOOPS_addPreHeader(loop, NULL);
	preheader = IRMETHOD_getNextInstruction(loop->method, preheader);
	assert(preheader != NULL);

	/* Move every invariants outside the loop	*/
	item = xanList_first(loop->invariants);
	while (item != NULL){
		ir_instruction_t *inv;
		inv = item->data;
		assert(inv != NULL);
		IRMETHOD_moveInstructionBefore(loop->method, inv, preheader);
		item = item->next;
	}

	/* Return					*/
	return JITTRUE;
}

static inline char *get_version (void){
  return VERSION;
}

static inline char * get_informations (void) {
  return INFORMATIONS;
}

static inline char *get_author (void){
  return AUTHOR;
}
