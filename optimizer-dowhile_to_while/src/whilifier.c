/*
 * Copyright (C) 2009-2010 Simone Campanoni, Mastrandrea Daniele
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

/**
 * @file whilifier.c
 * @brief ILDJIT Optimizer: do-while to while loops transformer.
 * @author Simone Campanoni, Mastrandrea Daniele
 * @version 0.1.0
 * @date 2009-2010
 *
 * @section <description> (Description)
 * Sometimes when optimising some code, a while loop is more suitable for
 * the task than a do-while one. This module is a plugin to perform such
 * transformation of do-while loops into while loops.
 *
 * @section <algorithm> (Algorithm)
 * First of all, the plugins needed to identify loops are executed. Then
 * the loop list is parsed to store the information about every loop.
 * At the same time a test is performed to identify the type of the loop:
 * a loop is taken into consideration only if it looks like a do-while
 * one (obviously) and if isn't nested inside another one with the same
 * header instruction. The new list is then parsed again, and when a valid
 * loop is found, the following steps are executed:
 *   - identify the instruction where the loop condition starts being computed.
 *     This task is performed in two steps:
 *       -# identification of the basic block: the correct BB is the one
 *          whose instructions (pre)dominate all the back edges' tails
 *          and all the loop exits. This is still true when the loop is
 *          made of only one BB. This task is actually performed while the
 *          loop type is being computer, to avoid code duplication;
 *       -# identification of the instruction: the correct instruction
 *          is the one that defines the first temporary (or variable)
 *          that is involved in the condition computation;
 *   - add the instructions to turn the loop into a while one, obviously
 *     preserving the semantic of the code.
 *       -# a pre-header block is placed before every possible path entering
 *          the loop header; it performs an unconditional branch to the first
 *          instruction of the condition evaluation;
 *       -# the block is then filled with all the instructions composing
 *          the loop body, in order to execute such code at least one, just
 *          as happens in any do-while loop; in other words some kind of
 *          `loop unrolling` is performed.
 * If a do-while loop has been `whilified` the entire process would be re-executed,
 * since the previously computed information on the other loops are no longer
 * valid, because the instructions' IDs were changed. This isn't actually needed
 * beacause of the pre-processing steps, where all the needed information is
 * stored in an ad-hoc structure. This trick leads to a little waste in space
 * occupation, but to a major gain in speed of execution, since the dependencies
 * are ran only once.
**/

// Standard headers
#include <stdio.h>
#include <stddef.h>
#include <assert.h>
#include <errno.h>

// Standard ILDJIT headers
#include <ir_optimization_interface.h>
#include <ir_language.h>
#include <iljit-utils.h>
#include <compiler_memory_manager.h>

// My headers
#include <config.h>
#include "whilifier.h"
#ifdef PRINTDEBUG
#include "pretty_print.h"
#ifdef SUPERDEBUG
#include "bits.h"
#endif /* SUPERDEBUG */
#endif /* PRINTDEBUG */

// A system to handle dependencies has not been set up yet, so...
#define MANUALLY_RUN_DEPS

// Plugin informations
#define INFORMATIONS 	"Plugin to trasform do-while loops into while loops"
#define	AUTHOR		"Simone Campanoni, Mastrandrea Daniele"
#define JOB		LOOP_FROM_DO_WHILE_TO_WHILE

/**
 * @defgroup Macros
 * Useful macros
**/

/**
 * @def LOOP_BODY_SIZE(extended_loop)
 * @ingroup Macros
 * @brief A macro that returns the number of instruction composing the loop body.
**/
#define LOOP_BODY_SIZE(extended_loop) abs(extended_loop->loop_head->ID-extended_loop->loop_cond->ID)

/**
 * @defgroup Interface
 * Standard interface functions
**/

/**
 * @ingroup Interface
 * @brief Get the ID of the job.
**/
static inline JITUINT64 dwtw_get_ID_job(void);

/**
 * @ingroup Interface
 * @brief Get the plugin version.
**/
static inline char * dwtw_get_version(void);

/**
 * @ingroup Interface
 * @brief Run the job of the plugin.
**/
static inline void dwtw_do_job(ir_method_t *method);

/**
 * @ingroup Interface
 * @brief Get informations about the plugin.
**/
static inline char * dwtw_get_informations(void);

/**
 * @ingroup Interface
 * @brief Get the author of the plugin.
**/
static inline char * dwtw_get_author(void);

static inline JITUINT64 dwtw_get_invalidations (void);

/**
 * @ingroup Interface
 * @brief Get the dependences of the plugin.
**/
static inline JITUINT64 dwtw_get_dependences(void);

/**
 * @ingroup Interface
 * @brief Perform the shutdown of the plugin.
**/
static inline void dwtw_shutdown(void);

/**
 * @ingroup Interface
 * @brief Perform the initialisation of the plugin.
**/
static inline void dwtw_init(ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix);

/**
 * @defgroup Internals
 * Optimizer internal functions
**/

/**
 * @ingroup Internals
 * @brief Run the dependencies of the plugin.
**/
static inline void internal_run_deps(ir_method_t * method);

/**
 * @ingroup Internals
 * @brief Start the plugin's task.
**/
static inline JITUINT32 internal_whilify(ir_method_t * method);

/**
 * @ingroup Internals
 * @brief Check the type of the loop, and if do-while look for the basic block where condition evaluation starts.
 * @note The function stores the results of its computation inside the data structure of type t_loop_extended
**/
static inline void internal_checkLoop(ir_method_t * method, t_loop_extended * current_loop);

/**
 * @ingroup Internals
 * @brief Look for the instruction where condition evaluation starts.
 * @note The function stores the results of its computation inside the data structure of type t_loop_extended
**/
static inline void internal_findLoopCondition(ir_method_t * method, t_loop_extended * current_loop);

/**
 * @ingroup Internals
 * @brief Add new instructions to perform the actual `whilification`.
 * @note The function stores the results of its computation inside the data structure of type t_loop_extended
**/
static inline void internal_addInstructions(ir_method_t * method, t_loop_extended * current_loop);

/*****************************************************************************************************************************************************/
/*                                                                                                                                                   */
/*****************************************************************************************************************************************************/

// Global variables
ir_lib_t	*irLib			= NULL;
ir_optimizer_t	*irOptimizer		= NULL;
char 	        *prefix			= NULL;
#ifdef PRINTDEBUG
PP_t		*pretty_printer		= NULL;
#endif /* PRINTDEBUG */

// Plugin interface
ir_optimization_interface_t plugin_interface = {
	dwtw_get_ID_job		, 
	dwtw_get_dependences	,
	dwtw_init		,
	dwtw_shutdown		,
	dwtw_do_job		, 
	dwtw_get_version	,
	dwtw_get_informations	,
	dwtw_get_author		,
	dwtw_get_invalidations
};

static inline void dwtw_init(ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix) {
	// Assertions
	assert(lib != NULL);
	assert(optimizer != NULL);
	assert(outputPrefix != NULL);

	irLib		= lib;
	irOptimizer	= optimizer;
	prefix		= outputPrefix;

#ifdef PRINTDEBUG
	pretty_printer	= allocFunction(sizeof(*pretty_printer));
	PP_INIT();
#endif /* PRINTDEBUG */
}

static inline void dwtw_shutdown(void) {
	irLib		= NULL;
	irOptimizer	= NULL;
	prefix		= NULL;
#ifdef PRINTDEBUG
	freeFunction(pretty_printer);
#endif /* PRINTDEBUG */
}

static inline JITUINT64 dwtw_get_ID_job(void) {
	return JOB;
}

static inline JITUINT64 dwtw_get_dependences(void) {
	return
#ifdef SUPERDEBUG
		METHOD_PRINTER			|
#endif /* SUPERDEBUG */
		BASIC_BLOCK_IDENTIFIER		|
		PRE_DOMINATOR_COMPUTER		|
		LOOP_IDENTIFICATION;
}

static inline char * dwtw_get_version(void) {
	return VERSION;
}

static inline char * dwtw_get_informations(void) {
	return INFORMATIONS;
}

static inline char * dwtw_get_author(void) {
	return AUTHOR;
}

static inline JITUINT64 dwtw_get_invalidations (void){
	return INVALIDATE_ALL;
}

static inline void internal_run_deps(ir_method_t * method) {
	PDEBUG("Running dependencies...\n");
	PP_INDENT();
	PDEBUG("Loop identification\n");
	PP_INDENT();
	SDEBUG("BASIC_BLOCK_IDENTIFIER\n");
	IROPTIMIZER_callMethodOptimization(irOptimizer, method, irLib, BASIC_BLOCK_IDENTIFIER);
	SDEBUG("PRE_DOMINATOR_COMPUTER\n");
	IROPTIMIZER_callMethodOptimization(irOptimizer, method, irLib, PRE_DOMINATOR_COMPUTER);
	SDEBUG("LOOP_IDENTIFICATION\n");
	IROPTIMIZER_callMethodOptimization(irOptimizer, method, irLib, LOOP_IDENTIFICATION);
	PP_UNINDENT();
	PP_UNINDENT();
	PP_UNMARK();
	PDEBUG("... done\n");
	PP_MARK();
}

static inline void dwtw_do_job(ir_method_t * method) {
	JITUINT32		loops_whilified;

	// Assertions
	assert(method != NULL);

	PDEBUG("OPTIMIZER_DOWHILE_TO_WHILE: Start with method %s, which is %d IR-instructions long\n", METH_NAME(method), METH_N_INST(method));
	PP_INDENT();

#ifdef MANUALLY_RUN_DEPS
	internal_run_deps(method);
#endif /* MANUALLY_RUN_DEPS */

	loops_whilified = internal_whilify(method);

	PP_UNINDENT();
	PDEBUG("OPTIMIZER_DOWHILE_TO_WHILE: Finished with method %s: %u do-while loops were whilified\n", METH_NAME(method), loops_whilified);

	return;
}

static inline JITUINT32 internal_whilify(ir_method_t * method) {
	XanList			*loop_list;
	XanListItem		*loop_item;
	t_loop_extended		*current_loop;
	t_ir_instruction	*temp_inst;
	JITUINT32		loops_whilified;

#ifdef PRINTDEBUG
	XanListItem		*temp_item;
#ifdef SUPERDEBUG
	const JITUINT32		n_inst		=	METH_N_INST(method);
#endif /* SUPERDEBUG */
#endif /* PRINTDEBUG */

	// Nothing to do if the method contains no loops
	if (NULL == LOOPS(method))
		return JITFALSE;

	// Initialise the list of loops
	loop_list = xanListNew(allocFunction, freeFunction, NULL);

	PDEBUG("Parsing the list of identified loops\n");
	PP_INDENT();

	// Initialise iteration variable
	loop_item = LOOPS(method)->firstItem;

	// Populate the list of (extended) loops
	while (NULL != loop_item) {

		// Allocate resources for the extended loop data structure
		current_loop = NULL;
		current_loop = allocFunction(sizeof(*current_loop));
		assert(NULL != current_loop);

		// Get current loop
		current_loop->loop = (t_loop*) loop_item->data;
		PDEBUG("Loop id: %u\n", current_loop->loop->loop_id);

		// Get parent loop
		current_loop->parent_loop = PARENT_LOOP(method, current_loop->loop);

		// Get the loop header
		current_loop->loop_head = INST_AT(method, current_loop->loop->header_id);

		// Get the list of loop instructions
		current_loop->loop_instructions = LOOP_INSTRUCTIONS(method, current_loop->loop);

		// Identify back edges
		current_loop->loop_back_edges = xanListNew(allocFunction, freeFunction, NULL);
		assert(NULL != current_loop->loop_back_edges);
		temp_inst = GET_PREDECESSOR(method, current_loop->loop_head, NULL);
		while (NULL != temp_inst) {
			if (NULL != current_loop->loop_instructions->find(current_loop->loop_instructions, temp_inst))
				current_loop->loop_back_edges->append(current_loop->loop_back_edges, temp_inst);
			temp_inst = GET_PREDECESSOR(method, current_loop->loop_head, temp_inst);
		}
		// A loop is being considered: one back edge should be present, at least
		assert(current_loop->loop_back_edges->length(current_loop->loop_back_edges) > 0);

		// Get the loop exits
		current_loop->loop_exits = current_loop->loop->loopExits->cloneList(current_loop->loop->loopExits);

		// Check the type of the loop
		PP_INDENT();
		internal_checkLoop(method, current_loop);
		assert(LOOP_UNKNOWN != current_loop->loop_type);
		PP_UNINDENT();

		// Add the loop to the list
		loop_list->append(loop_list, current_loop);

		// Proceed with next loop
		loop_item = loop_item->next;
	}
	PP_UNINDENT();

#ifdef PRINTDEBUG
	if (loop_list->length(loop_list) > 0) { 

		PDEBUG("Dumping the list of loops (%d found)\n", loop_list->length(loop_list));
		PP_INDENT();

		loop_item = loop_list->firstItem;
		while (NULL != loop_item) {

			current_loop = (t_loop_extended*) loop_item->data;
			PDEBUG("Loop with id: %u\n", current_loop->loop->loop_id);
			PP_INDENT();
			PDEBUG("Type: %u\n", current_loop->loop_type);
			PDEBUG("Header: #%u\n", current_loop->loop_head->ID);
			PDEBUG("Made of %d IR instructions\n", current_loop->loop_instructions->length(current_loop->loop_instructions));
			if (NULL != current_loop->BB_cond_inst)
				PDEBUG("Basic block of condition starts at: #%u\n", FIRST_IN_BB(method, GET_BB(method, current_loop->BB_cond_inst))->ID);
			if (NULL != current_loop->parent_loop && 0 != current_loop->parent_loop->loop_id)
				PDEBUG("Nested inside loop with id: %u\n", current_loop->parent_loop->loop_id);

			SDEBUG("Info according to bitmaps\n");
			PP_INDENT();
			SDEBUG("Belong_inst: %s\n", blks_to_string(current_loop->loop->belong_inst, n_inst));
			SDEBUG("First instruction: #%u\n", find_first_bit(current_loop->loop->belong_inst, n_inst));
			SDEBUG("Last instruction: #%u\n", find_last_bit(current_loop->loop->belong_inst, n_inst));
			SDEBUG("Total instructions: %u\n", count_bits(current_loop->loop->belong_inst, n_inst));
			PP_UNINDENT();

			PDEBUG("Back edges\n");
			temp_item = current_loop->loop_back_edges->firstItem;
			while (NULL != temp_item) {
				temp_inst = (t_ir_instruction*) temp_item->data;
				PP_INDENT();
				PDEBUG("Starting from instruction #%u\n", temp_inst->ID);
				PP_UNINDENT();
				temp_item = temp_item->next;
			}

			PDEBUG("Loop exits\n");
			temp_item = current_loop->loop_exits->firstItem;
			while (NULL != temp_item) {
				temp_inst = (t_ir_instruction*) temp_item->data;
				PP_INDENT();
				PDEBUG("Instruction #%u\n", temp_inst->ID);
				PP_UNINDENT();
				temp_item = temp_item->next;
			}
			PP_UNINDENT();

			loop_item = loop_item->next;
		}
		PP_UNINDENT();
	}
#endif /* PRINTDEBUG */

	// No loop `whilified`, yet
	loops_whilified = 0;

	PDEBUG("Starting actual `whilification`\n");
	PP_INDENT();

	// Initialise iteration variable
	loop_item = loop_list->firstItem;

	// Iterate over the list of (extended) loops
	while (NULL != loop_item) {

		// Get the loop
		current_loop = (t_loop_extended*) loop_item->data;

		// Act according to the validity and type of the loop
		if (LOOP_DOWHILE == current_loop->loop_type) {
			// A regular do-while loop, then start actual `whilification`
			PDEBUG("Loop_id: %u\n", current_loop->loop->loop_id);
			PP_INDENT();
			PDEBUG("Current is a valid do-while loop\n");

			// If some code modification were already applied, need to recalculate basic blocks
			if (loops_whilified >= 1)
				IROPTIMIZER_callMethodOptimization(irOptimizer, method, irLib, BASIC_BLOCK_IDENTIFIER);

			// Get the instruction where the loop condition evaluation starts
			assert(NULL != current_loop->BB_cond_inst);
			PDEBUG("Looking for the first conditional instruction\n");
			internal_findLoopCondition(method, current_loop);
			assert(NULL != current_loop->loop_cond);
			PDEBUG("Loop condition evaluation starts at: %u\n", current_loop->loop_cond->ID);

			// Unroll one iteration of the loop
			PDEBUG("Add new instructions in order to emulate the do-while behavior\n");
			internal_addInstructions(method, current_loop);

			// Loop correctly modified
			loops_whilified++;
			PP_UNINDENT();
			PP_UNMARK();
			PDEBUG("Ended with do-while loop with id: %u\n", current_loop->loop->loop_id);
			PP_MARK();
		} else {
			// Everything but a do-while loop
			PDEBUG("Loop_id: %u\n", current_loop->loop->loop_id);
			PP_INDENT();
			PDEBUG("Current is a NON valid do-while loop\n");
			PP_UNINDENT();
			PP_UNMARK();
			PDEBUG("... skipped\n");
			PP_MARK();
		}

		// Destroy no longer used lists
		current_loop->loop_back_edges->destroyList(current_loop->loop_back_edges);
		current_loop->loop_exits->destroyList(current_loop->loop_exits);
		current_loop->loop_instructions->destroyList(current_loop->loop_instructions);

		// Free memory
		freeFunction(current_loop);

		// Proceed with next loop
		loop_item = loop_item->next;
	}
	PP_UNINDENT();

	// Destroy no longer used loop list
	loop_list->destroyList(loop_list);

	// Return the number of modified loops
	return loops_whilified;
}

static inline void internal_checkLoop(ir_method_t * method, t_loop_extended * current_loop) {
	XanList			*work_list;
	XanListItem		*temp_item, *bis_item;
	XanNode			*temp_node;
	t_ir_instruction	*temp_inst, *parent_inst, *matched_inst;
	BasicBlock		*BB_temp;
	JITUINT32		matching_inst;
	JITBOOLEAN		break_search;

	// If current and parent loops share the same header, then the loop to be examined is the parent
	if (NULL != current_loop->parent_loop && current_loop->loop->header_id == current_loop->parent_loop->header_id) {
		current_loop->loop_type = LOOP_NESTED;
		SDEBUG("Unsupported loop: current and parent loops share the same header\n");
		return;
	}

	// Check if the loop is generated by a catcher routine
	if (IRSTARTCATCHER == INST_TYPE(method, current_loop->loop_head)) {
		current_loop->loop_type = LOOP_CATCHER;
		SDEBUG("Unsupported loop: exception catcher\n");
		return;
	}

	// Similarly all loop exits must be branchif or branchifnot...
	temp_item = current_loop->loop_exits->firstItem;
	while (temp_item != NULL) {
		if (IRBRANCHIF == INST_TYPE(method,(t_ir_instruction*)temp_item->data) || IRBRANCHIFNOT == INST_TYPE(method,(t_ir_instruction*)temp_item->data)) {
			temp_item = temp_item->next;
		} else {
			current_loop->loop_type = LOOP_MALFORMED;
			SDEBUG("Unsupported loop: malformed loop exits\n");
			return;
		}
	}

	// ... the same the back edges
	temp_item = current_loop->loop_back_edges->firstItem;
	while (temp_item != NULL) {
		if (IRBRANCHIF == INST_TYPE(method,(t_ir_instruction*)temp_item->data) || IRBRANCHIFNOT == INST_TYPE(method,(t_ir_instruction*)temp_item->data)) {
			temp_item = temp_item->next;
		} else {
			current_loop->loop_type = LOOP_MALFORMED;
			SDEBUG("Unsupported loop: malformed back edges\n");
			return;
		}
	}

	SDEBUG("Look for an instruction (or a consecutive pair) that is either a back edge and a loop exit\n");
	PP_INDENT();
	matching_inst = 0;
	temp_item = current_loop->loop_back_edges->firstItem;
	while (NULL != temp_item) {
		temp_inst = (t_ir_instruction*) temp_item->data;
		if (NULL != current_loop->loop_exits->find(current_loop->loop_exits, temp_inst)) {
			SDEBUG("Found instruction: #%u\n", temp_inst->ID);
			matched_inst = temp_inst;
			matching_inst++;
		} else if (NULL != current_loop->loop_exits->find(current_loop->loop_exits, PREV_INST(method, temp_inst))) {
			SDEBUG("Found a loop exit followed by a back edge: #%u and #%u\n", temp_inst->ID - 1, temp_inst->ID);
			matched_inst = temp_inst;
			matching_inst++;
		} else if (NULL != current_loop->loop_exits->find(current_loop->loop_exits, NEXT_INST(method, temp_inst))) {
			SDEBUG("Found a back edge followed by a loop exit: #%u and #%u\n", temp_inst->ID, temp_inst->ID + 1);
			matched_inst = NEXT_INST(method, temp_inst);
			matching_inst++;
		}
		temp_item = temp_item->next;
	}
	PP_UNINDENT();

	// If no matching instructions, the loop is a while/for one
	if (0 == matching_inst) {
		current_loop->loop_type = LOOP_WHILE;
		SDEBUG("Unsupported loop: this is a while/for loop\n");
		return;
	}

	// If two or more matching instructions, the loop shows an odd combination of consecutive break & continue
	if (2 <= matching_inst) {
		current_loop->loop_type = LOOP_ODD;
		SDEBUG("Unsupported loop: odd combination of internal jumps (break/continue)\n");
		return;
	}

	// If the matched instruction isn't a leaf in the predominance tree, the loop must be a while/for with an odd break & continue
	temp_node = PREDOM_TREE(method)->find(PREDOM_TREE(method), matched_inst);
	assert(NULL != temp_node);
	if (NULL != temp_node->childrens && temp_node->childrens->length(temp_node->childrens) > 0) {
		temp_item = temp_node->childrens->firstItem;
		while (NULL != temp_item) {
			if (NULL != current_loop->loop_instructions->find(current_loop->loop_instructions, (t_ir_instruction*) temp_item->data)) {
				current_loop->loop_type = LOOP_ODD;
				SDEBUG("Unsupported loop: a LE & BE instruction must be a leaf in the loop dominator tree\n");
				return;
			}
			temp_item = temp_item->next;
		}
	}

	SDEBUG("Loop MAY BE a valid do-while, looking for the basic block where condition evaluation starts\n");

	// Clone the loop exits' list into a working list
	work_list = current_loop->loop_exits->cloneList(current_loop->loop_exits);
	assert(work_list->length(work_list) >= 1);

	// Append the list of back edges to the working list
	work_list->appendList(work_list, current_loop->loop_back_edges);
	assert(work_list->length(work_list) >= 2);
	assert(work_list->length(work_list) > current_loop->loop_back_edges->length(current_loop->loop_back_edges));

	// Perform a duplicates' cleanup inside the working list
	work_list->deleteClones(work_list);

	// This isn't the time to break search, yet
	break_search = JITFALSE;

	// Initialise temp_inst to point the previously matched instruction
	temp_inst = matched_inst;

	SDEBUG("Looking for the block\n");
	PP_INDENT();

	// Starting from the previously matched instruction, go back up the dominator tree until neither a BE nor LE is found
	while (JITTRUE) {

		// Get the first instruction in the current Basic Block
		BB_temp = GET_BB(method, temp_inst);
		temp_inst = FIRST_IN_BB(method, BB_temp);
		SDEBUG("Examining block starting at #%u\n", temp_inst->ID);

		// Break if reached the header of the loop
		if (temp_inst == current_loop->loop_head)
			break;

		// Get the corresponding node in the dominator tree, then its (unique) parent
		temp_node = PREDOM_TREE(method)->find(PREDOM_TREE(method), temp_inst);
		assert(NULL != temp_node);
		temp_node = temp_node->parent;
		assert(NULL != temp_node);

		// Get the corresponding instruction, that belongs to a different Basic Block
		parent_inst = (t_ir_instruction*) temp_node->data;
		assert(BB_temp != GET_BB(method, parent_inst));

		// If the instruction is a jump to BE/LE and is dominated by a LE/BE (not examined, yet), then skip to the previous dominator
		while (NULL == work_list->find(work_list, parent_inst) && IS_A_JUMP(method, parent_inst)) {
			BB_temp = GET_BB(method, parent_inst);
			parent_inst = FIRST_IN_BB(method, BB_temp);
			if (parent_inst == current_loop->loop_head)
				break;
			temp_node = PREDOM_TREE(method)->find(PREDOM_TREE(method), parent_inst);
			assert(NULL != temp_node);
			temp_node = temp_node->parent;
			assert(NULL != temp_node);
			parent_inst = (t_ir_instruction*) temp_node->data;
			assert(BB_temp != GET_BB(method, parent_inst));
		}

		// If the instruction is in the working list (BE or LE, then a jump), remove it and continue, else break search since found what needed
		if (NULL != work_list->find(work_list, parent_inst)) {
			PP_INDENT();
			SDEBUG("Instruction is either a loop exit or a back edge\n");
			PP_UNINDENT();
			assert(IS_A_JUMP(method, parent_inst));
			work_list->delete(work_list, parent_inst);
			temp_inst = parent_inst;
		} else {
			PP_INDENT();
			SDEBUG("Found block starting at instruction #%u\n", FIRST_IN_BB(method, GET_BB(method,temp_inst))->ID);
			PP_UNINDENT();
			break;
		}
	}

	PP_UNINDENT();

	// Search inside working list for any node dominated by the found one, then delete what found
	temp_item = work_list->lastItem;
	while (temp_item != NULL) {
		if (IS_A_PREDOM(method, temp_inst, (t_ir_instruction*) temp_item->data)) {
			bis_item = temp_item;
			temp_item = temp_item->prev;
			work_list->deleteItem(work_list, bis_item);
		} else
			temp_item = temp_item->prev;
	}

	// If working list isn't empty, it means than there's some break/continue somewhere else that cannot be handled
	if (work_list->length(work_list) > 0) {
		if (work_list->length(work_list) == 1) {
			current_loop->loop_type = LOOP_DOWHILE_JUMP;
			SDEBUG("Found a (most likely) do-while loop with one internal jump (break/continue)\n");
		} else {
			current_loop->loop_type = LOOP_JUMPS;
			SDEBUG("Unsupported loop: unable to identify the body beacause of some internal jumps (break/continue)\n");
		}
		// Clear working list's memory before the return statement
		work_list->destroyList(work_list);
		return;
	}
	assert(work_list->length(work_list) == 0);

	// Clear working list's memory, since it's no longer used
	work_list->destroyList(work_list);

	SDEBUG("Common dominator for all back edges and loop exits is at instruction #%u\n", temp_inst->ID);

	// Check if the loop is made of only one basic block
	if (GET_BB(method, current_loop->loop_head) == GET_BB(method, matched_inst)) {
		current_loop->loop_type = LOOP_ONE_BLOCK;
		SDEBUG("One-basic-block loop (most likely a do-while)\n");
	}

	// If the found Basic Block is the same as the loop header belongs to the loop is not valid
	if (LOOP_ONE_BLOCK != current_loop->loop_type && GET_BB(method, temp_inst) == GET_BB(method, current_loop->loop_head)) {
		current_loop->loop_type = LOOP_UNSUPPORTED;
		SDEBUG("Unsupported loop: unable to identify the body\n");
		return;
	}

	// Found instruction must dominate all the loop exits...
	temp_item = current_loop->loop_exits->firstItem;
	while (temp_item != NULL) {
		if (IS_A_PREDOM(method, temp_inst, (t_ir_instruction*) temp_item->data)) {
			temp_item = temp_item->next;
		} else {
			current_loop->loop_type = LOOP_WRONG;
			SDEBUG("Unsupported loop: something went wrong identifying loop body (LE-side)\n");
			return;
		}
	}

	// ... and all the back edges
	temp_item = current_loop->loop_back_edges->firstItem;
	while (temp_item != NULL) {
		if (IS_A_PREDOM(method, temp_inst, (t_ir_instruction*) temp_item->data)) {
			temp_item = temp_item->next;
		} else {
			current_loop->loop_type = LOOP_WRONG;
			SDEBUG("Unsupported loop: something went wrong identifying loop body (BE-side)\n");
			return;
		}
	}

	// If execution reached this point, the loop is a do-while (hopefully)
	SDEBUG("Found a do-while loop!\n");
	current_loop->loop_type = LOOP_DOWHILE;
	current_loop->BB_cond_inst = temp_inst;
}

static inline void internal_findLoopCondition(ir_method_t * method, t_loop_extended * current_loop) {
	XanStack		*work_stack;
	t_ir_instruction	*temp_inst;
	ir_item_t		*param;
	JITUINT32		varID;
	JITBOOLEAN		var_miss, shift_inst;

	// Allocate space for a new working stack
	work_stack = xanStackNew(allocFunction, freeFunction, NULL);
	assert(NULL != work_stack);

	// Start from the last instruction in the Basic Block
	temp_inst = LAST_IN_BB(method, GET_BB(method, current_loop->BB_cond_inst));
	assert(IS_A_JUMP(method, temp_inst)); // redundant check
	assert(IRBRANCHIF == INST_TYPE(method, temp_inst) || IRBRANCHIFNOT == INST_TYPE(method, temp_inst)); // more specific, but still redundant

	// Initialise the stack with the ID of the branch variable
	PP_INDENT();
	SDEBUG("Examining instruction #%u\n", temp_inst->ID);
	param = PAR_1(method, temp_inst);
	assert(IROFFSET == param->type);
	PP_INDENT();
	SDEBUG("Pushing variable id #%u to the working stack\n", (JITUINT32) param->value);
	PP_UNINDENT();
	work_stack->push(work_stack, (void*) (JITNUINT) param->value);

	// Now stack cannot be empty
	assert(work_stack->getSize(work_stack) > 0);

	// No variable miss and no need to shift any instruction, yet
	var_miss = JITFALSE;
	shift_inst = JITFALSE;

	// Start searching: iterate till the beginning of the basic block is reached or the working stack is empty
	while (temp_inst != FIRST_IN_BB(method, GET_BB(method, current_loop->BB_cond_inst)) && work_stack->getSize(work_stack) > 0) {

		// Get previous instruction
		if (var_miss == JITFALSE) {
			temp_inst = PREV_INST(method, temp_inst);
			SDEBUG("Examining instruction #%u\n", temp_inst->ID);
		}

		// Pop the top of the stack
		varID = (IR_ITEM_VALUE) (JITNUINT)work_stack->pop(work_stack);
		PP_INDENT();
		SDEBUG("Popped variable id #%u from the working stack\n", varID);

		// Check instruction type and act consequently
		switch (INST_TYPE(method, temp_inst)) {

			// Comparison
			case IRLT:
			case IRGT:
			case IREQ:
			// Logical operations
			case IRAND:
			case IROR:
			case IRXOR:
			// Arithmetic instruction
			case IRADD:
			case IRADDOVF:
			case IRSUB:
			case IRSUBOVF:
			case IRMUL:
			case IRMULOVF:
			case IRDIV:

				assert(IROFFSET == RES_TYPE(method, temp_inst));
				param = RES(method, temp_inst);
				// Following if statement is not true if some optimisation is performed before the execution of the plugin
				if (varID == (JITNUINT) param->value) {
					var_miss = JITFALSE;
					// If first parameter is a variable, then get its ID and push to the stack
					param = PAR_1(method, temp_inst);
					if (IROFFSET == param->type && IS_A_TEMPORARY(method, param->value)) {
						SDEBUG("Pushing variable id %u to the working stack\n", (JITUINT32) param->value);
						work_stack->push(work_stack, (void*) (JITNUINT) param->value);
					}
					// If second parameter is a variable, then get its ID and push to the stack
					param = PAR_2(method, temp_inst);
					if (IROFFSET == param->type && IS_A_TEMPORARY(method, param->value)) {
						SDEBUG("Pushing variable id %u to the working stack\n", (JITUINT32) param->value);
						work_stack->push(work_stack, (void*) (JITNUINT) param->value);
					}
				} else {
					if (0 == work_stack->getSize(work_stack))
						shift_inst = JITTRUE;
					var_miss = JITTRUE;
				}
				break;

			// Conversion
			case IRCONV:
			// Logical operations
			case IRNOT:
			case IRNEG:
			// Load rel
			case IRLOADREL:

				assert(IROFFSET == RES_TYPE(method, temp_inst));
				param = RES(method, temp_inst);
				// Following if statement is not true if some optimisation is performed before the execution of the plugin
				if (varID == (JITNUINT) param->value) {
					var_miss = JITFALSE;
					// If first parameter is a variable (others aren't), then get its ID and push to the stack
					param = PAR_1(method, temp_inst);
					if (IROFFSET == param->type && IS_A_TEMPORARY(method, param->value)) {
						SDEBUG("Pushing variable id %u to the working stack\n", (JITUINT32) param->value);
						work_stack->push(work_stack, (void*) (JITNUINT) param->value);
					}
				} else {
					if (0 == work_stack->getSize(work_stack))
						shift_inst = JITTRUE;
					var_miss = JITTRUE;
				}
				break;

			// Store
			case IRSTORE:

				param = PAR_1(method, temp_inst);
				if (IROFFSET == param->type && varID == (JITNUINT) param->value) {
					var_miss = JITFALSE;
					// If second parameter is a variable (others aren't meaningful), then get its ID and push to the stack
					param = PAR_2(method, temp_inst);
					if (IROFFSET == param->type && IS_A_TEMPORARY(method, param->value)) {
						SDEBUG("Pushing variable id %u to the working stack\n", (JITUINT32) param->value);
						work_stack->push(work_stack, (void*) (JITNUINT) param->value);
					} 
				} else {
					if (0 == work_stack->getSize(work_stack))
						shift_inst = JITTRUE;
					var_miss = JITTRUE;
				}
				break;

			// None of previous
			default:
				if (0 == work_stack->getSize(work_stack))
					shift_inst = JITTRUE;
				var_miss = JITTRUE;
				break;
		}
		PP_UNINDENT();
	}
	PP_UNINDENT();

	// Shift instruction by one, if a miss happened
	if (shift_inst == JITTRUE)
		temp_inst = NEXT_INST(method, temp_inst);

	// Destroy no longer used stack
	work_stack->destroyStack(work_stack);

	// Now temp_inst is the first instruction of condition evaluation
	assert(temp_inst != current_loop->loop_head);

	// If previous instruction is a label, then move temp_inst to it
	// Repeat until inside the BB, but hopefully this step will run only once
	while (temp_inst != FIRST_IN_BB(method, GET_BB(method, current_loop->BB_cond_inst)) && IRLABEL == INST_TYPE(method, PREV_INST(method, temp_inst)))
		temp_inst = PREV_INST(method, temp_inst);

	SDEBUG("... identified the instruction: #%u\n", temp_inst->ID);

	current_loop->loop_cond = temp_inst;
}

static inline void internal_addInstructions(ir_method_t * method, t_loop_extended * current_loop) {
	XanList			*labels_to_delete;
	XanListItem		*list_item;
	XanStack		*work_stack;
	XanHashTable		*copied_map, *labels_map;
	t_ir_instruction	*loop_prehead, *last_cloned, *inst, *temp_inst, *next_inst;
	IR_ITEM_VALUE		label_cond, label_head, label_prehead, label_postprehead, label_old, label_mapped, label_temp;
	void			*lookup_result;

	PP_INDENT();

#ifdef SUPERDEBUG
	SDEBUG("Dumping the method's control flow graph before any change is applied\n");
	IROPTIMIZER_callMethodOptimization(irOptimizer, method, irLib, METHOD_PRINTER);
#endif /* SUPERDEBUG */

	// Assertions
	assert(NULL != current_loop->loop_cond);
	assert(NULL != current_loop->loop_head);

	PDEBUG("Adding a new label, before the first conditional instruction\n");
	PP_INDENT();
	
	if (IRLABEL == INST_TYPE(method, current_loop->loop_cond)) {
		label_cond = PAR_1_VALUE(method, current_loop->loop_cond);
		SDEBUG("Label already exists (%llu)\n", label_cond);
	} else {
		label_cond = NEW_LABEL(method);
		SDEBUG("Creating a new label (%llu)\n", label_cond);
		inst = NEW_INST_BEFORE(method, current_loop->loop_cond);
		INST_TYPE_SET(method, inst, IRLABEL);
		PAR_1_SET(method, inst, label_cond, 0, IRLABELITEM, IRLABELITEM, 0, NULL);
		current_loop->loop_cond = inst;
#ifdef SUPERDEBUG
		PP_UNINDENT();
		SDEBUG("Dumping the graph after the creation of the new label\n");
		PP_INDENT();
		IROPTIMIZER_callMethodOptimization(irOptimizer, method, irLib, METHOD_PRINTER);
#endif /* SUPERDEBUG */
	}
	PP_UNINDENT();

	assert(IRLABEL == INST_TYPE(method, current_loop->loop_cond)); // trivial

	// The pre-header contains redundant branches, in order to keep a clean and immediate view in the control flow graph
	// Such branches will be deleted, if needed, by a specific plugin
	PDEBUG("Adding the pre-header block(s)\n");
	PP_INDENT();

	assert(IRLABEL == INST_TYPE(method, current_loop->loop_head)); // redundant check, but whatever...
	label_head = PAR_1_VALUE(method, current_loop->loop_head);

	SDEBUG("Getting a new label for the first instruction in the pre-header\n");
	label_prehead = NEW_LABEL(method);

	SDEBUG("Making all the header's predecessor outside the loop to point to the pre-header\n");
	inst = GET_PREDECESSOR(method, current_loop->loop_head, NULL);
	while (inst != NULL) {
		if (NULL == current_loop->loop_back_edges->find(current_loop->loop_back_edges, inst)) {
		// that is, if inst is outside the loop then it must be updated to point to the pre-header
			switch (INST_TYPE(method, inst)) {
				case IRBRANCH:
					if (label_head == PAR_1_VALUE(method, inst))
						PAR_1_VALUE_SET(method, inst, label_prehead);
					break;
				case IRBRANCHIF:
				case IRBRANCHIFNOT:
					if (label_head == PAR_2_VALUE(method, inst))
						PAR_2_VALUE_SET(method, inst, label_prehead);
					break;
				default:
					break;
			}
		}
		inst = GET_PREDECESSOR(method, current_loop->loop_head, inst);
	}

	SDEBUG("Adding the new label instruction\n");
	loop_prehead = NEW_INST_BEFORE(method, current_loop->loop_head);
	INST_TYPE_SET(method, loop_prehead, IRLABEL);
	PAR_1_SET(method, loop_prehead, label_prehead, 0, IRLABELITEM, IRLABELITEM, 0, NULL);

	SDEBUG("Getting a new label for the last instruction in the pre-header\n");
	label_postprehead = NEW_LABEL(method);

	SDEBUG("Adding the new label instruction\n");
	inst = NEW_INST_AFTER(method, loop_prehead);
	INST_TYPE_SET(method, inst, IRLABEL);
	PAR_1_SET(method, inst, label_postprehead, 0, IRLABELITEM, IRLABELITEM, 0, NULL);

	SDEBUG("Adding a branch to the condition\n");
	inst = NEW_INST_AFTER(method, inst);
	INST_TYPE_SET(method, inst, IRBRANCH);
	PAR_1_SET(method, inst, label_cond, 0, IRLABELITEM, IRLABELITEM, 0, NULL);

#ifdef SUPERDEBUG
	PP_UNINDENT();
	SDEBUG("Dumping the graph after the creation of the pre-header block\n");
	PP_INDENT();
	IROPTIMIZER_callMethodOptimization(irOptimizer, method, irLib, METHOD_PRINTER);
#endif /* SUPERDEBUG */

	PP_UNINDENT();

	PDEBUG("Copying the loop body into the pre-header\n");

	// Initialise working data structures
	copied_map = xanHashTable_new(LOOP_BODY_SIZE(current_loop), JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
	assert(NULL != copied_map);
	labels_to_delete = xanListNew(allocFunction, freeFunction, NULL);
	assert(NULL != labels_to_delete);
	work_stack = xanStackNew(allocFunction, freeFunction, NULL);
	assert(NULL != work_stack);
	labels_map = xanHashTable_new(LOOP_BODY_SIZE(current_loop), JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
	assert(NULL != labels_map);

	// Initialise the last cloned instruction as the first instruction in the pre-header
	last_cloned = loop_prehead;

	// Insert the first instruction to be copied insied the stack
	assert(IRLABEL == INST_TYPE(method, current_loop->loop_head)); // just checked earlier, but whatever...
	work_stack->push(work_stack, current_loop->loop_head);

	// Loop unrolling is done when the stack is empty
	while (work_stack->getSize(work_stack) > 0) {

		// Get the top of the stack
		inst = (t_ir_instruction*) work_stack->pop(work_stack);

		// Proceed only if the instruction hasn't already been copied
		if (NULL != copied_map->lookup(copied_map, inst))
			continue;

		// Clone the instruction to a new one
		temp_inst = CLONE_INST(method, inst);

		// Move the instruction after the last cloned instruction
		MOVE_INST_AFTER(method, temp_inst, last_cloned);
		last_cloned = temp_inst;

		// Add the instructions to the map of copied instructions
		copied_map->insert(copied_map, inst, last_cloned);

		// If the instruction has a LABELITEM as a parameter rename the label to break dependencies on the original loop
		// A workaround is needed whenever an instruction points to a label whose instruction has not been created yet
		switch (INST_TYPE(method, last_cloned)) {

			// First parameter is a label
			case IRLABEL:
			case IRBRANCH:
			case IRCALLFINALLY:
			case IRSTARTFILTER:
			case IRSTARTFINALLY:
			case IRCALLFILTER:

				assert(IRLABELITEM == PAR_1_TYPE(method, last_cloned));
				assert(IRLABELITEM == PAR_1_INT_TYPE(method, last_cloned));
				label_old = PAR_1_VALUE(method, last_cloned);
				// Nothing to update if current instruction points to the new header of the loop
				if (label_old != label_cond) {
					lookup_result = labels_map->lookup(labels_map, (void*) (JITNUINT) (label_old + 1));
					if (NULL == lookup_result) {
						label_mapped = (JITNUINT) NEW_LABEL(method);
						// Workaround to prevent ILDJIT from reassigning the label ID
						if (NULL == LABEL_AT(method, label_mapped)) {
							temp_inst = NEW_INST(method);
							INST_TYPE_SET(method, temp_inst, IRLABEL);
							PAR_1_SET(method, temp_inst, label_mapped, 0, IRLABELITEM, IRLABELITEM, 0, NULL);
							labels_to_delete->append(labels_to_delete, temp_inst);
						}
						labels_map->insert(labels_map, (void*) (JITNUINT) (label_old + 1), (void*) (JITNUINT) (label_mapped + 1));
					} else {
						label_mapped = ((IR_ITEM_VALUE)(JITNUINT) lookup_result) - 1;
					}
					PAR_1_VALUE_SET(method, last_cloned, label_mapped);
				} else
					PAR_1_VALUE_SET(method, last_cloned, label_postprehead);
				break;

			// Second parameter is a label
			case IRBRANCHIF:
			case IRBRANCHIFNOT:
			case IRENDFILTER:

				assert(IRLABELITEM == PAR_2_TYPE(method, last_cloned));
				assert(IRLABELITEM == PAR_2_INT_TYPE(method, last_cloned));
				label_old = PAR_2_VALUE(method, last_cloned);
				// Nothing to update if current instruction points to the new header of the loop
				if (label_old != label_cond) {
					lookup_result = labels_map->lookup(labels_map, (void*) (JITNUINT) (label_old + 1));
					if (NULL == lookup_result) {
						label_mapped = (JITNUINT) NEW_LABEL(method);
						// Workaround to prevent ILDJIT from reassigning the label ID
						if (NULL == LABEL_AT(method, label_mapped)) {
							temp_inst = NEW_INST(method);
							INST_TYPE_SET(method, temp_inst, IRLABEL);
							PAR_1_SET(method, temp_inst, label_mapped, 0, IRLABELITEM, IRLABELITEM, 0, NULL);
							labels_to_delete->append(labels_to_delete, temp_inst);
						}
						labels_map->insert(labels_map, (void*) (JITNUINT) (label_old + 1), (void*) (JITNUINT) (label_mapped + 1));
					} else {
						label_mapped = ((JITNUINT) lookup_result) - 1;
					}
					PAR_2_VALUE_SET(method, last_cloned, label_mapped);
				} else
					PAR_2_VALUE_SET(method, last_cloned, label_postprehead);
				break;

			// First, second and third parameters are labels
			case IRBRANCHIFPCNOTINRANGE:

				// Consider third parameter, then don't break the switch
				assert(IRLABELITEM == PAR_3_TYPE(method, last_cloned));
				assert(IRLABELITEM == PAR_3_INT_TYPE(method, last_cloned));
				label_old = PAR_3_VALUE(method, last_cloned);
				// Nothing to update if current instruction points to the new header of the loop
				if (label_old != label_cond) {
					lookup_result = labels_map->lookup(labels_map, (void*) (JITNUINT) (label_old + 1));
					if (NULL == lookup_result) {
						label_mapped = (JITNUINT) NEW_LABEL(method);
						// Workaround to prevent ILDJIT from reassigning the label ID
						if (NULL == LABEL_AT(method, label_mapped)) {
							temp_inst = NEW_INST(method);
							INST_TYPE_SET(method, temp_inst, IRLABEL);
							PAR_1_SET(method, temp_inst, label_mapped, 0, IRLABELITEM, IRLABELITEM, 0, NULL);
							labels_to_delete->append(labels_to_delete, temp_inst);
						}
						labels_map->insert(labels_map, (void*) (JITNUINT) (label_old + 1), (void*) (JITNUINT) (label_mapped + 1));
					} else {
						label_mapped = ((JITNUINT) lookup_result) - 1;
					}
					PAR_3_VALUE_SET(method, last_cloned, label_mapped);
				} else
					PAR_3_VALUE_SET(method, last_cloned, label_postprehead);

			// First and second parameters are labels
			case IRENDFINALLY:

				// Consider second parameter
				assert(IRLABELITEM == PAR_2_TYPE(method, last_cloned));
				assert(IRLABELITEM == PAR_2_INT_TYPE(method, last_cloned));
				label_old = PAR_2_VALUE(method, last_cloned);
				// Nothing to update if current instruction points to the new header of the loop
				if (label_old != label_cond) {
					lookup_result = labels_map->lookup(labels_map, (void*) (JITNUINT) (label_old + 1));
					if (NULL == lookup_result) {
						label_mapped = (JITNUINT) NEW_LABEL(method);
						// Workaround to prevent ILDJIT from reassigning the label ID
						if (NULL == LABEL_AT(method, label_mapped)) {
							temp_inst = NEW_INST(method);
							INST_TYPE_SET(method, temp_inst, IRLABEL);
							PAR_1_SET(method, temp_inst, label_mapped, 0, IRLABELITEM, IRLABELITEM, 0, NULL);
							labels_to_delete->append(labels_to_delete, temp_inst);
						}
						labels_map->insert(labels_map, (void*) (JITNUINT) (label_old + 1), (void*) (JITNUINT) (label_mapped + 1));
					} else {
						label_mapped = ((JITNUINT) lookup_result) - 1;
					}
					PAR_2_VALUE_SET(method, last_cloned, label_mapped);
				} else
					PAR_2_VALUE_SET(method, last_cloned, label_postprehead);

				// Consider first parameter
				assert(IRLABELITEM == PAR_1_TYPE(method, last_cloned));
				assert(IRLABELITEM == PAR_1_INT_TYPE(method, last_cloned));
				label_old = PAR_1_VALUE(method, last_cloned);
				// Nothing to update if current instruction points to the new header of the loop
				if (label_old != label_cond) {
					lookup_result = labels_map->lookup(labels_map, (void*) (JITNUINT) (label_old + 1));
					if (NULL == lookup_result) {
						label_mapped = (JITNUINT) NEW_LABEL(method);
						// Workaround to prevent ILDJIT from reassigning the label ID
						if (NULL == LABEL_AT(method, label_mapped)) {
							temp_inst = NEW_INST(method);
							INST_TYPE_SET(method, temp_inst, IRLABEL);
							PAR_1_SET(method, temp_inst, label_mapped, 0, IRLABELITEM, IRLABELITEM, 0, NULL);
							labels_to_delete->append(labels_to_delete, temp_inst);
						}
						labels_map->insert(labels_map, (void*) (JITNUINT) (label_old + 1), (void*) (JITNUINT) (label_mapped + 1));
					} else {
						label_mapped = ((JITNUINT) lookup_result) - 1;
					}
					PAR_1_VALUE_SET(method, last_cloned, label_mapped);
				} else
					PAR_1_VALUE_SET(method, last_cloned, label_postprehead);
				break;

			default:

				break;
		}

		// Initialise to NULL the pointer to the instruction next to the popped one
		next_inst = NULL;

		// Push into the stack the successors of the last examined instruction
		temp_inst = GET_SUCCESSOR(method, inst, NULL);
	
		while (NULL != temp_inst) {
			if (temp_inst == NEXT_INST(method, inst)) {
				if (temp_inst != current_loop->loop_cond) {
					next_inst = temp_inst; // Can be assigned only once
				} else {
					last_cloned = NEW_INST_AFTER(method, last_cloned);
					INST_TYPE_SET(method, last_cloned, IRBRANCH);
					PAR_1_SET(method, last_cloned, label_postprehead, 0, IRLABELITEM, IRLABELITEM, 0, NULL);
				}
			} else if (temp_inst != current_loop->loop_cond) {
				work_stack->push(work_stack, temp_inst); // To be examined
				// Should never cross any back edge
				assert(temp_inst != current_loop->loop_head);
			}
			temp_inst = GET_SUCCESSOR(method, inst, temp_inst);
		}

		// Add explicit branches where an ambigous situation may arise
		if (NULL != next_inst) {
			if (!(IS_A_JUMP(method, inst))) {
				if (IRLABEL == INST_TYPE(method, next_inst)) {
					// Get the label where to branch to
					label_old = PAR_1_VALUE(method, next_inst);
					lookup_result = labels_map->lookup(labels_map, (void*) (JITNUINT) (label_old + 1));
					if (NULL == lookup_result) {
						label_mapped = (JITNUINT) NEW_LABEL(method);
						// Workaround to prevent ILDJIT from reassigning the label ID
						if (NULL == LABEL_AT(method, label_mapped)) {
							temp_inst = NEW_INST(method);
							INST_TYPE_SET(method, temp_inst, IRLABEL);
							PAR_1_SET(method, temp_inst, label_mapped, 0, IRLABELITEM, IRLABELITEM, 0, NULL);
							labels_to_delete->append(labels_to_delete, temp_inst);
						}
						labels_map->insert(labels_map, (void*) (JITNUINT) (label_old + 1), (void*) (JITNUINT) (label_mapped + 1));
					} else {
						label_mapped = ((JITNUINT) lookup_result) - 1;
					}
					// Explicitly branch to the label
					last_cloned = NEW_INST_AFTER(method, last_cloned);
					INST_TYPE_SET(method, last_cloned, IRBRANCH);
					PAR_1_SET(method, last_cloned, label_mapped, 0, IRLABELITEM, IRLABELITEM, 0, NULL);
				} else if (NULL != copied_map->lookup(copied_map, next_inst)) {
					temp_inst = (t_ir_instruction*) copied_map->lookup(copied_map, next_inst);
					// Add a new label before next_inst
					label_temp = NEW_LABEL(method);
					temp_inst = NEW_INST_BEFORE(method, next_inst);
					INST_TYPE_SET(method, temp_inst, IRLABEL);
					PAR_1_SET(method, temp_inst, label_temp, 0, IRLABELITEM, IRLABELITEM, 0, NULL);
					// Explicitly branch to the label
					last_cloned = NEW_INST_AFTER(method, last_cloned);
					INST_TYPE_SET(method, last_cloned, IRBRANCH);
					PAR_1_SET(method, last_cloned, label_temp, 0, IRLABELITEM, IRLABELITEM, 0, NULL);
				}
			}
			// If a next instruction is found, push it as the top of the stack
			work_stack->push(work_stack, next_inst);
		}
	}

	// Delete workaround instructions and destroy list (data deleted by the deleteInstruction())
	list_item = labels_to_delete->lastItem;
	while (NULL != list_item) {
		DEL_INST(method, (t_ir_instruction*) list_item->data);
		list_item = list_item->prev;
	}

	// Destroy other unused data structures
	labels_map->toList(labels_map)->destroyList(labels_map->toList(labels_map));
	labels_map->destroy(labels_map);
	work_stack->destroyStack(work_stack);
	labels_to_delete->destroyList(labels_to_delete);

#ifdef SUPERDEBUG
	SDEBUG("Dumping the method's control flow graph after all the changes are applied\n");
	IROPTIMIZER_callMethodOptimization(irOptimizer, method, irLib, METHOD_PRINTER);
#endif /* SUPERDEBUG */

	PP_UNINDENT();
}
