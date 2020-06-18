/*
 * Copyright (C) 2007 - 2010 Simone Campanoni, Luca Rocchini, Luca Sisler
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
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <ir_optimization_interface.h>
#include <ir_language.h>
#include <iljit-utils.h>
#include <xanlib.h>
#include <compiler_memory_manager.h>

// My headers
#include <optimizer_alias.h>
#include <config.h>
// End

#define INFORMATIONS    "This step compute pointer alias"
#define AUTHOR          "Simone Campanoni, Luca Rocchini, Luca Sisler"

static inline JITUINT64 alias_get_ID_job (void);
char *alias_get_version (void);
static inline void alias_do_job (ir_method_t * method);
char *alias_get_informations (void);
char *alias_get_author (void);
JITUINT64 alias_get_dependences (void);
void alias_shutdown (JITFLOAT32 totalTime);
void alias_init (ir_lib_t * lib, ir_optimizer_t * optimizer, char *outputPrefix);
static inline JITUINT64 alias_get_invalidations (void);

//IR information helper
static inline JITBOOLEAN is_pointer (JITUINT32 internal_type);
static inline JITBOOLEAN instruction_return_pointer (t_ir_instruction *inst, XanList *pointer_variables);
static inline JITUINT32 instruction_to_var (t_ir_instruction * inst);
static inline XanList * computer_variables_may_be_pointer (ir_method_t *method);

// type based analysis helper
static inline XanList *compute_pointer (ir_method_t *method, XanList *pointer_variables);
static inline JITBOOLEAN extract_type_info (ir_method_t * method, JITUINT32 i, TypeDescriptor **cil_type, XanList *pointer_variables);
static inline JITBOOLEAN extract_info_from_flow (ir_method_t * method, JITUINT32 i, TypeDescriptor **cil_type, XanList *pointer_variables);
static inline JITBOOLEAN can_be_max_alias (variable_info *var1, variable_info *var2);
static inline XanHashTable *compute_max_alias (XanList *pointer_info);

// flow based analysis helper
static inline JITUINT32 count_predecessor (ir_method_t * method, t_ir_instruction *inst);
static inline void compute_flow_alias (ir_method_t * method, XanHashTable * max_alias, XanList *pointer_info, XanList *pointer_variables);
static inline void compute_parameters_clique_class (XanHashTable * max_alias, XanList *pointer_info, XanHashTable * in_first_inst, XanList *alias_class_list);
static inline JITBOOLEAN trans (XanHashTable * in, XanHashTable * out, JITUINT32 location, t_ir_instruction * inst, XanHashTable * max_alias, XanList *alias_class_list);

// transfert function helper
static inline JITBOOLEAN merge_pointer_operation_classes (XanHashTable * in, XanHashTable * out, JITUINT32 to_var_id, JITUINT32 from_var_id, JITINT32 constant, XanList *alias_class_list);
static inline JITBOOLEAN merge_max_alias_classes (XanHashTable * in, XanHashTable * out, XanHashTable * max_alias, JITUINT32 to_var_id);

// helper alias information set
static inline JITBOOLEAN merge_predecessor_alias_info (XanHashTable * from, XanHashTable * to, XanList *pointer_info);
static inline XanHashTable *prepare_in_alias_info (XanList *pointer_info);
static inline XanHashTable *prepare_out_alias_info (XanList *pointer_info, XanHashTable *in_alias_info, t_ir_instruction * inst);

// helper for alias class set
static inline unsigned int alias_class_hash (void *element);
static inline int alias_class_equals (void *key1, void *key2);

// alias info writer
static inline void destroy_ir_instr_alias_info (XanHashTable *alias);

// clean up pass
void clean_up (XanHashTable *max_alias, XanList *pointer_info);

static inline alias_class *create_empty_alias_class (JITINT32 location, JITINT32 field, XanList * alias_class_list);
static inline JITBOOLEAN new_alias_class_in_set (XanHashTable * set, JITINT32 location, JITINT32 field, XanList *alias_class_list);
static inline JITBOOLEAN insert_alias_class_in_set (XanHashTable * set, alias_class * el);
static inline JITBOOLEAN merge_alias_class_set (XanHashTable * from, XanHashTable * to);
static inline JITBOOLEAN internal_fetch_cli_type (IR_ITEM_VALUE varID, ir_item_t *item, TypeDescriptor **cli_type);

ir_lib_t *irLib = NULL;
ir_optimizer_t *irOptimizer = NULL;
char *prefix = NULL;

ir_optimization_interface_t plugin_interface = {
	alias_get_ID_job,
	alias_get_dependences,
	alias_init,
	alias_shutdown,
	alias_do_job,
	alias_get_version,
	alias_get_informations,
	alias_get_author,
	alias_get_invalidations
};

void alias_init (ir_lib_t * lib, ir_optimizer_t * optimizer, char *outputPrefix){

	/* Assertions                   */
	assert(lib != NULL);
	assert(optimizer != NULL);
	assert(outputPrefix != NULL);

	irLib = lib;
	irOptimizer = optimizer;
	prefix = outputPrefix;
}

void alias_shutdown (JITFLOAT32 totalTime){
	irLib = NULL;
	irOptimizer = NULL;
	prefix = NULL;
}

static inline JITUINT64 alias_get_ID_job (void){
	return MEMORY_ALIASES_ANALYZER;
}

JITUINT64 alias_get_dependences (void){
	return 0;
}

static inline JITUINT64 alias_get_invalidations (void){
	return INVALIDATE_NONE;
}


static inline unsigned int alias_class_hash (void *element){
	alias_class *class = (alias_class *) element;

	return ((class->field) << 16) | (class->location);
}

static inline int alias_class_equals (void *key1, void *key2){
	if (key1 == NULL || key2 == NULL) {
		return 0;
	}
	alias_class *class1 = (alias_class *) key1;
	alias_class *class2 = (alias_class *) key2;
	return (class1->location == class2->location) && (class1->field == class2->field);
}

/**
 * @brief return list of variables in max_alias relation with variable key
 * @param table: hash table containing max_alias information
 * @param key: variable
 * @return list of variables in max_alias relation with variable key
 */
static inline XanList *max_alias_lookup (XanHashTable *table, JITUINT32 key){
	return (XanList *) table->lookup(table, (void *) (key + 1));
}

/**
 * @brief return list of variables in max_alias relation with variable key if key does not exits the function create an empty relation
 * @param table: hash table containing max_alias information
 * @param key: variable
 * @return list of variables in max_alias relation with variable key
 */
static inline XanList *max_alias_update (XanHashTable *table, JITUINT32 key){
	XanList *it = (XanList *) table->lookup(table, (void *) (key + 1));

	if (it == NULL) {
		it = xanListNew(allocFunction, freeFunction, NULL);
		table->insert(table, (void *) key + 1, (void *) it);
	}
	assert(table->lookup(table, (void *) (key + 1)) != NULL);
	return it;
}

/**
 * @brief return list of alias classes of variable key
 * @param table: hash table containing alias information
 * @param key: variable
 * @return list of alias classes of variable key
 */
static inline XanHashTable *alias_info_lookup (XanHashTable *table, JITUINT32 key){
	return (XanHashTable *) table->lookup(table, (void *) (key + 1));
}

/**
 * @brief return JITTRUE if variable i is a pointer
 * @param method: IR method
 * @param i: variable to check
 * @param cil_type: CIL class ID of variable i
 * @param bin_type: Binary Information of variable i
 * @return JITTRUE if variable i is a pointer
 */
static inline JITBOOLEAN extract_type_info (ir_method_t * method, JITUINT32 i, TypeDescriptor **cil_type, XanList *pointer_variables){
	JITBOOLEAN isOk;
	JITUINT32 num_par = IRMETHOD_getMethodParametersNumber(method);
	JITUINT32 num_local = IRMETHOD_getMethodLocalsNumber(method);
	JITUINT32 internal_type;

	if (i < num_par) {
		(*cil_type) = IRMETHOD_getParameterILType(method, i);
		internal_type = IRMETHOD_getParameterType(method, i);
		isOk = is_pointer(internal_type);
	} else if (i < (num_par + num_local)) {
		(*cil_type) = IRMETHOD_getLocalVariableOfMethod(method, i - num_par)->type_info;
		internal_type = IRMETHOD_getLocalVariableOfMethod(method, i - num_par)->internal_type;
		isOk = is_pointer(internal_type);
	} else{
		isOk = extract_info_from_flow(method, i, cil_type, pointer_variables);
	}

	return isOk;
}

/**
 * @brief return JITTRUE if temporary variable i is a pointer
 * @param method: IR method
 * @param i: variable to check
 * @param cil_type: CIL class ID of variable i
 * @param bin_type: Binary Information of variable i
 * @return JITTRUE if variable i is a pointer
 */
static inline JITBOOLEAN extract_info_from_flow (ir_method_t * method, JITUINT32 i, TypeDescriptor **cil_type, XanList *pointer_variables){
	JITBOOLEAN isOk = JITFALSE;
	JITUINT32 inst_count;
	JITUINT32 num_inst = IRMETHOD_getInstructionsNumber(method);

	/* Initialize the variables		*/
	(*cil_type) = 0;

	for (inst_count = 0; (inst_count < num_inst) && ((*cil_type) == 0); inst_count++) {
		t_ir_instruction *inst = IRMETHOD_getInstructionAtPosition(method, inst_count);
		if (instruction_to_var(inst) == i) {
			// Check if instruction define variable
			isOk = instruction_return_pointer(inst, pointer_variables);
			switch (inst->type) {
			    case IRALLOCA:
				    (*cil_type) = (inst->result).type_infos;
				    break;
			    case IRNEWOBJ:
			    case IRNEWARR:
				    (*cil_type) = (TypeDescriptor *)(JITNUINT)inst->param_1.value.v;
				    break;
			    case IRCALL:
			    case IRVCALL:
			    {
				    ir_method_t *called_method;
				    called_method = IRMETHOD_getIRMethodFromMethodID(method, inst->param_1.value.v);
				    (*cil_type) = IRMETHOD_getResultILType(called_method);
			    }
			    break;
			    case IRLIBRARYCALL:
				    (*cil_type) = (inst->result).type_infos;
				    break;
			    case IRSTORE:
				    if ((inst->param_1).type_infos != NULL) {
					    (*cil_type) = (inst->param_1).type_infos;
				    }
				    break;
			    default:
				    if ((inst->result).type_infos != NULL) {
					    (*cil_type) = (inst->result).type_infos;
				    }
			}
		} else {

			// Check if instruction use variable
			isOk = internal_fetch_cli_type(i, &(inst->param_1), cil_type);
			isOk |= internal_fetch_cli_type(i, &(inst->param_2), cil_type);
			isOk |= internal_fetch_cli_type(i, &(inst->param_3), cil_type);
		}
	}
	return isOk;
}

static inline JITBOOLEAN internal_fetch_cli_type (IR_ITEM_VALUE varID, ir_item_t *item, TypeDescriptor **cli_type){
	JITBOOLEAN isOk;

	isOk = JITFALSE;
	if (     (item->type == IROFFSET)        &&
		 (item->value.v == varID)                ) {
		isOk = is_pointer(item->internal_type);
		if (    (isOk)                                          &&
			(item->type_infos != NULL)      ) {
			(*cli_type) = item->type_infos;
		}
	}

	return isOk;
}

/**
 * @brief return JITTRUE if internal_type is a pointer
 * @param method: internal_type: type to check
 * @return JITTRUE if internal_type is a pointer
 */
static inline JITBOOLEAN is_pointer (JITUINT32 internal_type){
	JITBOOLEAN isAPointer;

	isAPointer = (internal_type == IROBJECT || internal_type == IRTPOINTER || internal_type == IRNINT || internal_type == IRMPOINTER);
	if (!isAPointer) {
		if (sizeof(JITNUINT) == 4) {
			isAPointer = (internal_type == IRINT32);
		} else {
			isAPointer = (internal_type == IRINT64);
		}
	}
	return isAPointer;
}

/**
 * @brief return JITTRUE if var1 and var2 can be in max_alias relation
 * @param var1: variable to check
 * @param var2: variable to check
 * @return JITTRUE if var1 and var2 can be in max_alias relation
 */
static inline JITBOOLEAN can_be_max_alias (variable_info *var1, variable_info *var2){
	JITBOOLEAN isOk;

	if (var1->cil_type == 0 || var2->cil_type == 0) {
		isOk = JITTRUE;
	} else{
		isOk = IRMETHOD_isAssignable(var1->cil_type, var2->cil_type) || IRMETHOD_isAssignable(var2->cil_type, var1->cil_type);
	}
	return isOk;
}


static inline void alias_do_job (ir_method_t * method){
	XanList         *pointer_info;
	XanList         *pointer_variables;
	XanListItem     *item;
	XanHashTable    *max_alias;

	/* Assertions			*/
	assert(method != NULL);
	PDEBUG("ALIAS ANALYZING METHOD %s\n", method->getName(method));

	/* Print the method		*/
	#ifdef PRINTDEBUG
	irOptimizer->callMethodOptimization(irOptimizer, method, METHOD_PRINTER);
	#endif

	/* Initialize the variables	*/
	item = NULL;

	/* Compute the set of variables	*
	* that may be used as pointers	*/
	pointer_variables = computer_variables_may_be_pointer(method);
	assert(pointer_variables != NULL);
	PDEBUG("MEMORY ALIAS:   Pointers\n");
	#ifdef PRINTDEBUG
	item = pointer_variables->first(pointer_variables);
	while (item != NULL) {
		PDEBUG("MEMORY ALIAS:           %d\n", (JITUINT32) item->data);
		item = item->next;
	}
	#endif

	pointer_info = compute_pointer(method, pointer_variables);
	PDEBUG("POINTER COMPUTATION ENDED\n");
	PDEBUG("-----------\n");

	max_alias = compute_max_alias(pointer_info);
	PDEBUG("MAX ALIAS ANALYZING ENDED\n");
	PDEBUG("-----------\n");

	compute_flow_alias(method, max_alias, pointer_info, pointer_variables);
	PDEBUG("FLOW ALIAS ANALYZING ENDED\n");
	PDEBUG("-----------\n");

	/* Free the memory		*/
	clean_up(max_alias, pointer_info);
	pointer_variables->destroyList(pointer_variables);
	PDEBUG("CLEAN UP ENDED\n");
	PDEBUG("-----------\n");

	/* Return			*/
	return;
}

/**
 * @brief compute alias information based on flow
 * @param max_alias: max_alias information of method
 * @param pointer_info: list of pointer variable
 */
void clean_up (XanHashTable *max_alias, XanList *pointer_info){

	XanListItem *current_element = pointer_info->first(pointer_info);

	while (current_element != NULL) {
		variable_info *current_var_info = (variable_info *) (current_element->data);
		JITUINT32 var = current_var_info->id;

		XanList *current_variable = max_alias_lookup(max_alias, var);
		current_variable->destroyList(current_variable);

		current_element = pointer_info->next(pointer_info, current_element);
	}

	max_alias->destroy(max_alias);
	pointer_info->destroyListAndData(pointer_info);
}

/**
 * @brief return max_alias information of method
 * @param method: IR method
 * @return list of pointer in the method
 */
static inline XanList * compute_pointer (ir_method_t *method, XanList *pointer_variables){
	JITUINT32 var;
	TypeDescriptor *cil_type;
	JITUINT32 num_var;
	XanList                 *pointer_info;

	/* Assertions			*/
	assert(method != NULL);
	assert(pointer_variables != NULL);

	num_var = IRMETHOD_getNumberOfVariablesNeededByMethod(method);
	pointer_info = xanListNew(allocFunction, freeFunction, NULL);

	for (var = 0; var < num_var; var++) {

		/* Check if the current variable may be used as a pointer	*/
		if (pointer_variables->find(pointer_variables, (void *) var) == NULL) {
			continue;
		}

		if (extract_type_info(method, var, &cil_type, pointer_variables)) {
			PDEBUG("VARIABLE %d HAS IL_TYPE=%llu\n", var, cil_type);
			variable_info *pointer = allocFunction(sizeof(variable_info));
			pointer->id = var;
			pointer->cil_type = cil_type;
			pointer_info->append(pointer_info, (void *) pointer);
		}
	}

	/* Return			*/
	return pointer_info;
}

/**
 * @brief return max_alias information of method
 * @param pointer_info: list of pointer variable
 * @return max_alias information of method
 */
static inline XanHashTable *compute_max_alias (XanList *pointer_info){
	XanHashTable *max_alias = xanHashTableNew(1, 0, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);

	XanListItem *var1 = pointer_info->first(pointer_info);

	while (var1 != NULL) {
		variable_info *var1_info = var1->data;
		JITUINT32 i = var1_info->id;
		XanList *i_list = max_alias_update(max_alias, i);
		assert(max_alias_lookup(max_alias, i) != NULL);

		XanListItem *var2 = var1->next;
		while (var2 != NULL) {
			variable_info *var2_info = var2->data;
			if (can_be_max_alias(var1_info, var2_info)) {
				JITUINT32 j = var2_info->id;
				i_list->append(i_list, (void *) j);
				XanList *j_list = max_alias_update(max_alias, j);
				j_list->append(j_list, (void *) i);
				PDEBUG("%d AND %d ARE MAX ALIAS\n", i, j);
			}
			var2 = var2->next;
		}
		var1 = var1->next;
	}

	return max_alias;
}

/**
 * @brief compute alias information at first instruction entry
 * @param method: IR method
 * @param max_alias: max_alias information of method
 * @param pointer_info: list of pointer variable of method
 * @param in_first_inst: alias information at first instruction entry data structure
 * @param alias_class_list: list of all alias classes
 */
static inline void compute_parameters_clique_class (XanHashTable * max_alias, XanList *pointer_info, XanHashTable * in_first_inst, XanList *alias_class_list){

	XanList *clique_list = xanListNew(allocFunction, freeFunction, NULL);

	XanListItem *current_element = pointer_info->first(pointer_info);

	while (current_element != NULL) {
		variable_info *current_var_info = (variable_info *) (current_element->data);
		JITUINT32 var = current_var_info->id;
		XanList *merge_max_alias_classes = max_alias_lookup(max_alias, var);

		JITBOOLEAN added = JITFALSE;
		XanListItem *current_clique = clique_list->first(clique_list);
		while (current_clique != NULL) {
			JITBOOLEAN found = JITTRUE;
			XanList *clique = (XanList *) (current_clique->data);
			XanListItem *current_var = clique->first(clique);
			while (current_var != NULL) {
				JITUINT32 selected_var = (JITUINT32) (current_var->data);
				if (merge_max_alias_classes->find(merge_max_alias_classes, (void *) selected_var) == NULL) {
					//No full compatibility
					found = JITFALSE;
					break;
				}
				current_var = clique->next(clique, current_var);
			}
			if (found) {
				clique->append(clique, (void *) var);
			}
			//test for next clique
			added |= found;
			current_clique = clique_list->next(clique_list, current_clique);
		}
		if (!added) {
			//No compatible clique found
			XanList *clique = xanListNew(allocFunction, freeFunction, NULL);
			clique_list->append(clique_list, (void *) clique);
			//append variable to new clique
			clique->append(clique, (void *) var);
			//test predecessor variable for compatibility

			XanListItem *pred_element = pointer_info->prev(pointer_info, current_element);
			while (pred_element != NULL) {
				variable_info *pred_var_info = (variable_info *) (pred_element->data);
				JITUINT32 pred_var = pred_var_info->id;

				XanList *pred_merge_max_alias_classes = max_alias_lookup(max_alias, pred_var);

				JITBOOLEAN found = JITTRUE;
				XanListItem *current_var = clique->first(clique);
				while (current_var != NULL) {
					JITUINT32 selected_var = (JITUINT32) (current_var->data);
					if (pred_merge_max_alias_classes->find(pred_merge_max_alias_classes, (void *) selected_var) == NULL) {
						found = JITFALSE;
						break;
					}
					current_var = clique->next(clique, current_var);
				}
				if (found) {
					clique->append(clique, (void *) pred_var);
				}

				pred_element = pointer_info->prev(pointer_info, pred_element);
			}
		}


		current_element = pointer_info->next(pointer_info, current_element);
	}

	// added alias class per clique and clean up
	JITINT32 count = 0;
	XanListItem *current_clique = clique_list->first(clique_list);
	while (current_clique != NULL) {
		PDEBUG("CLIQUE %d: ", count);
		count++;
		XanList *clique = (XanList *) current_clique->data;

		//create clique alias class
		alias_class *clique_class = create_empty_alias_class(-1 - count, 0, alias_class_list);

		//set alias class for each clique element
		XanListItem *current_var = clique->first(clique);
		while (current_var != NULL) {
			JITUINT32 selected_var = (JITUINT32) current_var->data;
			PDEBUG("%d ", selected_var);
			insert_alias_class_in_set(alias_info_lookup(in_first_inst, selected_var), clique_class);
			current_var = clique->next(clique, current_var);
		}
		PDEBUG("\n");

		//clean up processed clique
		clique->destroyList(clique);

		current_clique = clique_list->next(clique_list, current_clique);
	}

	//clean up clique list
	clique_list->destroyList(clique_list);
}

/**
 * @brief count the predecessors of an instruction
 * @param method: IR method
 * @param inst: instruction to check
 * @return number of predecessors of an instruction
 */
static inline JITUINT32 count_predecessor (ir_method_t * method, t_ir_instruction *inst){
	JITUINT32 count = 0;
	t_ir_instruction *pred = NULL;

	while ((pred = IRMETHOD_getPredecessorInstruction(method, inst, pred)) != NULL) {
		count++;
	}
	return count;
}

/**
 * @brief compute alias information based on flow
 * @param method: IR method
 * @param max_alias: max_alias information of method
 * @param pointer_info: list of pointer variable
 */
static inline void compute_flow_alias (ir_method_t *method, XanHashTable *max_alias, XanList *pointer_info, XanList *pointer_variables){
	XanHashTable            *in_alias_info = xanHashTableNew(1, 0, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
	XanHashTable            *out_alias_info = xanHashTableNew(1, 0, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
	JITUINT32 num_inst = IRMETHOD_getInstructionsNumber(method);
	XanList                 *alias_class_list = xanListNew(allocFunction, freeFunction, NULL);
	JITBOOLEAN              *have_in_info = allocFunction(sizeof(JITBOOLEAN) * num_inst);
	JITBOOLEAN              *have_out_info = allocFunction(sizeof(JITBOOLEAN) * num_inst);
	JITUINT32 inst_count;
	t_ir_instruction        *first_inst;
	XanHashTable            *in_first_inst;

	/* Fetch the first instruction			*/
	first_inst = IRMETHOD_getFirstInstruction(method);

	//build up alias information structure (in & out)
	JITUINT32 completed_count = 0;
	while (completed_count < (num_inst + num_inst)) {
		for (inst_count = 0; inst_count < num_inst; inst_count++) {
			t_ir_instruction        *inst;

			inst = IRMETHOD_getInstructionAtPosition(method, inst_count);
			t_ir_instruction *pred_inst = IRMETHOD_getPredecessorInstruction(method, inst, NULL);
			if (count_predecessor(method, inst) != 1 || inst == first_inst) {
				//instruction has multiple predecessor. allocate new alias information for merging
				if (in_alias_info->lookup(in_alias_info, inst) == NULL) {
					have_in_info[inst_count] = JITTRUE;
					XanHashTable *in_inst_info = prepare_in_alias_info(pointer_info);
					//PDEBUG("%d: MERGE IN %p\n",inst_count,in_inst_info);
					in_alias_info->insert(in_alias_info, inst, in_inst_info);
					completed_count++;
				}
			} else{
				//instruction has one predecessor. share output alias information from predecessor
				XanHashTable *out_inst_alias_info = (XanHashTable *) out_alias_info->lookup(out_alias_info, (void *) pred_inst);
				XanHashTable *current_alias_info = (XanHashTable *) in_alias_info->lookup(in_alias_info, (void *) inst);
				if (out_inst_alias_info != NULL && current_alias_info == NULL) {
					//PDEBUG("%d: IN COPY FROM %p PREDECESSOR IS %d\n",inst_count,alias_info,method->getInstructionID(method,pred_inst));
					have_in_info[inst_count] = JITFALSE;
					in_alias_info->insert(in_alias_info, inst, (void *) out_inst_alias_info);
					completed_count++;
				}
			}
			XanHashTable *in_inst_alias_info = (XanHashTable *) in_alias_info->lookup(in_alias_info, (void *) inst);
			XanHashTable *current_alias_info = (XanHashTable *) out_alias_info->lookup(out_alias_info, (void *) inst);
			if (in_inst_alias_info != NULL && current_alias_info == NULL) {
				if (instruction_return_pointer(inst, pointer_variables)) {

					//instruction returns pointer. allocate new alias information to save changes
					have_out_info[inst_count] = JITTRUE;
					XanHashTable *out_inst_info = prepare_out_alias_info(pointer_info, in_inst_alias_info, inst);
					//PDEBUG("%d: OUT NEW SET %p\n",inst_count,out_inst_info);
					out_alias_info->insert(out_alias_info, inst, out_inst_info);
				} else{

					//instruction doesn't return pointer. share input alias information
					have_out_info[inst_count] = JITFALSE;
					//PDEBUG("%d: OUT COPY FROM %p\n",inst_count,alias_info);
					out_alias_info->insert(out_alias_info, inst, (void *) in_inst_alias_info);
				}
				completed_count++;
			}
		}
	}

	//compute entry alias class using clique covering
	in_first_inst = (XanHashTable *) in_alias_info->lookup(in_alias_info, first_inst);
	compute_parameters_clique_class(max_alias, pointer_info, in_first_inst, alias_class_list);

	//start least fixed point iterations
	JITBOOLEAN modified;
	#ifdef PRINTDEBUG
	JITUINT32 run = 1;
	#endif
	do {
		modified = JITFALSE;
		#ifdef PRINTDEBUG
		PDEBUG("ITERATION %d\n", run++);
		#endif
		JITUINT32 inst_count;
		for (inst_count = 0; inst_count < num_inst; inst_count++) {
			t_ir_instruction *inst = IRMETHOD_getInstructionAtPosition(method, inst_count);
			PDEBUG("COMPUTE INSTRUCTION %d\n", inst_count);
			//load input & output alias information of instruction
			XanHashTable *in_inst_alias_info = (XanHashTable *) in_alias_info->lookup(in_alias_info, inst);
			XanHashTable *out_inst_alias_info = (XanHashTable *) out_alias_info->lookup(out_alias_info, inst);
			if (have_in_info[inst_count]) {
				//merge information from multiple predecessor
				t_ir_instruction *inst2 = NULL;
				while ((inst2 = IRMETHOD_getPredecessorInstruction(method, inst, inst2)) != NULL) {
					XanHashTable *alias_info_out = (XanHashTable *) out_alias_info->lookup(out_alias_info, inst2);
					merge_predecessor_alias_info(alias_info_out, in_inst_alias_info, pointer_info);
				}
				PDEBUG("\tPREDECESSOR MERGED\n");
			}
			if (have_out_info[inst_count]) {
				modified |= trans(in_inst_alias_info, out_inst_alias_info, inst_count, inst, max_alias, alias_class_list);
			}
		}
	} while (modified);


	//copy results sets in t_instruction structure
	for (inst_count = 0; inst_count < num_inst; inst_count++) {

		t_ir_instruction *inst = IRMETHOD_getInstructionAtPosition(method, inst_count);
		XanHashTable *in_inst_alias_info = (XanHashTable *) in_alias_info->lookup(in_alias_info, (void *) inst);

		destroy_ir_instr_alias_info((inst->metadata->alias).alias);

		(inst->metadata->alias).alias = xanHashTableNew(1, 0, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);

		XanListItem *current_element = pointer_info->first(pointer_info);
		while (current_element != NULL) {
			variable_info *current_var_info = (variable_info *) (current_element->data);
			JITUINT32 var = current_var_info->id;

			XanHashTable *var_alias_class_set = alias_info_lookup(in_inst_alias_info, var);
			XanList *current_alias_class_list = var_alias_class_set->toList(var_alias_class_set);
			(inst->metadata->alias).alias->insert((inst->metadata->alias).alias, (void *) (var + 1), current_alias_class_list);

			current_element = pointer_info->next(pointer_info, current_element);
		}
	}

	// clean up alias class information
	alias_class_list->destroyListAndData(alias_class_list);

	// clean up alias information structure (in & out)
	for (inst_count = 0; inst_count < num_inst; inst_count++) {
		t_ir_instruction *inst = IRMETHOD_getInstructionAtPosition(method, inst_count);

		if (inst->type != IRSTOREREL && inst->type != IRSTOREELEM) {
			continue;
		}

		if (have_in_info[inst_count]) {
			XanHashTable *in_inst_info = (XanHashTable *) in_alias_info->lookup(in_alias_info, inst);
			XanListItem *current_element = pointer_info->first(pointer_info);
			while (current_element != NULL) {
				variable_info *current_var_info = (variable_info *) (current_element->data);
				JITUINT32 var = current_var_info->id;

				XanHashTable *current_variable = alias_info_lookup(in_inst_info, var);
				current_variable->destroy(current_variable);

				current_element = pointer_info->next(pointer_info, current_element);
			}
			in_inst_info->destroy(in_inst_info);
		}
		if (have_out_info[inst_count]) {
			XanHashTable *out_inst_info = (XanHashTable *) out_alias_info->lookup(out_alias_info, inst);
			XanHashTable *current_variable = alias_info_lookup(out_inst_info, instruction_to_var(inst));
			current_variable->destroy(current_variable);
			out_inst_info->destroy(out_inst_info);
		}
	}
	in_alias_info->destroy(in_alias_info);
	out_alias_info->destroy(out_alias_info);
	freeFunction(have_in_info);
	freeFunction(have_out_info);

	/* Return				*/
	return;
}

/**
 * @brief return JITTRUE if return type of an instruction is a pointer
 * @param inst: instruction to check
 * @return JITTRUE if return type of an instruction is a pointer
 */
static inline JITBOOLEAN instruction_return_pointer (t_ir_instruction *inst, XanList *pointer_variables){
	JITBOOLEAN isOk;

	isOk = JITFALSE;
	switch (inst->type) {
	    case IRSTORE:
		    if (pointer_variables->find(pointer_variables, (void *) (JITNUINT) (inst->param_1).value.v) != NULL) {
			    isOk = is_pointer(inst->param_1.internal_type);
		    }
		    break;
	    default:
		    if (pointer_variables->find(pointer_variables, (void *) (JITNUINT) (inst->result).value.v) != NULL) {
			    isOk = is_pointer(inst->result.internal_type);
		    }
	}
	return isOk;
}

/**
 * @brief return result variable of an instruction if any
 * @param inst: instruction to check
 * @return result variable of an instruction
 */
static inline JITUINT32 instruction_to_var (t_ir_instruction * inst){
	JITUINT32 to_var;

	switch (inst->type) {
	    case IRSTORE:
		    to_var = inst->param_1.value.v;
		    break;
	    default:
		    to_var = inst->result.value.v;
	}
	return to_var;
}

/**
 * @brief compute out alias information of an out of context instruction
 * @param in: instruction entry alias information
 * @param out: instruction exit alias information
 * @param max_alias: max_alias information of method
 * @param to_var_id: id of instruction result variable
 * @return JITTRUE if the result of instruction is a variable
 */
static inline JITBOOLEAN merge_max_alias_classes (XanHashTable * in, XanHashTable * out, XanHashTable * max_alias, JITUINT32 to_var_id){
	XanListItem     *current_item;
	JITBOOLEAN modified = JITFALSE;
	XanList         *max_alias_to_var;
	JITINT32 count = 0;

	max_alias_to_var = max_alias_lookup(max_alias, to_var_id);
	if (max_alias_to_var != NULL) {
		current_item = max_alias_to_var->first(max_alias_to_var);
		while (current_item != NULL) {
			JITUINT32 it = (JITUINT32) (current_item->data);
			XanHashTable *var_alias = alias_info_lookup(in, it);
			count += var_alias->elementsInside(var_alias);
			current_item = max_alias_to_var->next(max_alias_to_var, current_item);
		}
		XanHashTable *out_inst = alias_info_lookup(out, to_var_id);
		if (count > out_inst->elementsInside(out_inst)) {
			current_item = max_alias_to_var->first(max_alias_to_var);
			while (current_item != NULL) {
				JITUINT32 it = (JITUINT32) (current_item->data);
				XanHashTable *var_alias = alias_info_lookup(in, it);
				modified |= merge_alias_class_set(var_alias, out_inst);
				current_item = max_alias_to_var->next(max_alias_to_var, current_item);
			}
		}
	}

	return modified;
}

/**
 * @brief compute out alias information for pointer increment instruction
 * @param in: instruction entry alias information
 * @param out: instruction exit alias information
 * @param to_var_id: id of instruction result variable
 * @param from_var_id: id of instruction base variable
 * @param costant: increment constant
 * @param alias_class_list: list of all alias classes
 * @return JITTRUE if exit alias information has changed
 */
static inline JITBOOLEAN merge_pointer_operation_classes (XanHashTable * in, XanHashTable * out, JITUINT32 to_var_id, JITUINT32 from_var_id, JITINT32 constant, XanList *alias_class_list){
	JITBOOLEAN modified = JITFALSE;
	XanHashTable *in_inst = alias_info_lookup(in, from_var_id);
	XanHashTable *out_inst = alias_info_lookup(out, to_var_id);

	if (in_inst->elementsInside(in_inst) > out_inst->elementsInside(out_inst)) {
		XanList *in_inst_list = in_inst->toList(in_inst);
		XanListItem *current_item = in_inst_list->first(in_inst_list);
		while (current_item != NULL) {
			alias_class *it = (alias_class *) (current_item->data);
			modified |= new_alias_class_in_set(out_inst, it->location, it->field + constant, alias_class_list);
			current_item = in_inst_list->next(in_inst_list, current_item);
		}
		in_inst_list->destroyList(in_inst_list);
	}
	return modified;
}

/**
 * @brief compute flow analysis transfert function at inst instruction
 * @param in: instruction entry alias information
 * @param out: instruction exit alias information
 * @param inst: instruction to check
 * @param max_alias: max_alias information of method
 * @param alias_class_list: list of all alias classes
 * @return JITTRUE if exit alias information has changed
 */
static inline JITBOOLEAN trans (XanHashTable * in, XanHashTable * out, JITUINT32 location, t_ir_instruction * inst, XanHashTable * max_alias, XanList *alias_class_list){
	JITBOOLEAN modified = JITFALSE;
	JITUINT32 to_var_id = instruction_to_var(inst);

	PDEBUG("\tRESULT VARIABLE IS %d\n", to_var_id);
	switch (inst->type) {
	    case IRSTORE:
		    if (inst->param_2.type == IROFFSET) {
			    PDEBUG("\tINSTRUCTION IS A STORE\n");
			    JITUINT32 from_var_id = inst->param_2.value.v;
			    PDEBUG("\tREAD FROM %d\n", from_var_id);
			    XanHashTable *var_in = alias_info_lookup(in, from_var_id);
			    XanHashTable *var_out = alias_info_lookup(out, to_var_id);
			    if (    (var_in != NULL)                                                        &&
				    (var_out != NULL)                                                       &&
				    (var_in->elementsInside(var_in) > var_out->elementsInside(var_out))     ) {
				    modified |= merge_alias_class_set(var_in, var_out);
			    }
		    } else{
			    PDEBUG("\tINSTRUCTION IS A MANAGED POINTER\n");
			    modified = merge_max_alias_classes(in, out, max_alias, to_var_id);
		    }
		    break;
	    case IRADD:
	    case IRADDOVF:
	    case IRSUB:
	    case IRSUBOVF:
		    /* FIXME: XAN da decommentare
		       if (inst->param_1.type == IROFFSET && inst->param_2.type != IROFFSET){
		        PDEBUG("\tINSTRUCTION IS A POINTER OPERATION\n");
		        JITUINT32 from_var_id = inst->param_1.value;
		        JITINT32 constant = (inst->type == IRADD || inst->type == IRADDOVF) ? inst->param_2.value : -(inst->param_2.value);
		        PDEBUG("\tREAD FROM %d\n", from_var_id);
		        modified = merge_pointer_operation_classes(in, out, to_var_id, from_var_id, constant,alias_class_list);
		       } else if (inst->param_2.type == IROFFSET && inst->param_1.type != IROFFSET){
		        PDEBUG("\tINSTRUCTION IS A POINTER OPERATION\n");
		        JITUINT32 from_var_id = inst->param_2.value;
		        JITINT32 constant = (inst->type == IRADD || inst->type == IRADDOVF) ? inst->param_1.value : -(inst->param_1.value);
		        PDEBUG("\tREAD FROM %d\n", from_var_id);
		        modified = merge_pointer_operation_classes(in, out, to_var_id, from_var_id, constant,alias_class_list);
		       } else{*/
		    PDEBUG("\tINSTRUCTION IS A GENERIC\n");
		    modified = merge_max_alias_classes(in, out, max_alias, to_var_id);
		    //}
		    break;
	    case IRNEWOBJ:
	    case IRALLOCA:
	    case IRNEWARR:
		    PDEBUG("\tINSTRUCTION IS A MEMORY ALLOCATION\n");
		    XanHashTable *out_inst = alias_info_lookup(out, to_var_id);
		    assert(out_inst != NULL);
		    if (out_inst->elementsInside(out_inst) == 0) {
			    modified = new_alias_class_in_set(out_inst, location, 0, alias_class_list);
		    }
		    break;
	    default:
		    PDEBUG("\tINSTRUCTION IS A GENERIC\n");
		    modified = merge_max_alias_classes(in, out, max_alias, to_var_id);
	}
	#ifdef PRINTDEBUG
	if (modified) {
		PDEBUG("\tALIAS INFO MODIFIED\n");
	}
	#endif
	return modified;
}

/**
 * @brief create a new alias class
 *
 * If the class asked is not present within the list given in input, it creates a new one and it returns it.
 * On the other hand, if the class is already present within the list alias_class_list, it returns it.
 *
 * @param location: location attribute
 * @param field: field attribute
 * @param alias_class_list: list of all alias classes
 * @return new alias class
 */
static inline alias_class *create_empty_alias_class (JITINT32 location, JITINT32 field, XanList * alias_class_list){
	alias_class     *new_class;
	XanListItem     *item;

	/* Search the class	*/
	item = alias_class_list->first(alias_class_list);
	while (item != NULL) {
		alias_class     *class;
		class = item->data;
		if (    (class->field == field)         &&
			(class->location == location)   ) {
			return class;
		}
		item = item->next;
	}
	new_class = allocFunction(sizeof(alias_class));
	new_class->location = location;
	new_class->field = field;
	alias_class_list->append(alias_class_list, (void *) new_class);
	return new_class;
}

/**
 * @brief insert new alias class in an alias class set
 * @param set: destination alias class set
 * @param location: location attribute
 * @param field: field attribute
 * @param alias_class_list: list of all alias classes
 * @return JITTRUE if destination alias class set has changed
 */
static inline JITBOOLEAN new_alias_class_in_set (XanHashTable * set, JITINT32 location, JITINT32 field, XanList *alias_class_list){
	alias_class test_class;

	test_class.location = location;
	test_class.field = field;
	alias_class *it = set->lookup(set, &test_class);
	if (it == NULL) {
		it = create_empty_alias_class(location, field, alias_class_list);
	}
	return insert_alias_class_in_set(set, it);
}

/**
 * @brief insert alias class in an alias class set
 * @param set: destination alias class set
 * @param el: alias class to be inserted
 * @return JITTRUE if alias class set has changed
 */
static inline JITBOOLEAN insert_alias_class_in_set (XanHashTable * set, alias_class * el){
	JITBOOLEAN not_found = (set->lookup(set, (void *) el) == NULL);

	if (not_found) {
		set->insert(set, el, el);
	}
	return not_found;
}

/**
 * @brief merge two alias class sets
 * @param from: source alias class set
 * @param to: destination alias class set
 * @return JITTRUE if destination alias class set has changed
 */
static inline JITBOOLEAN merge_alias_class_set (XanHashTable * from, XanHashTable * to){
	JITBOOLEAN modified = JITFALSE;
	XanList *from_list = from->toList(from);
	XanListItem *tuple = from_list->first(from_list);

	while (tuple != NULL) {
		alias_class     *it;
		it = (alias_class *) (tuple->data);
		modified |= insert_alias_class_in_set(to, it);
		tuple = tuple->next;
	}
	from_list->destroyList(from_list);
	return modified;
}

/**
 * @brief merge two alias information into one
 * @param from: source alias information
 * @param to: destination alias information
 * @param pointer_info: list of pointer variable of method
 * @return JITTRUE if destination alias information has changed
 */
static inline JITBOOLEAN merge_predecessor_alias_info (XanHashTable * from, XanHashTable * to, XanList *pointer_info){
	JITBOOLEAN modified = JITFALSE;

	XanListItem *current_element = pointer_info->first(pointer_info);

	while (current_element != NULL) {
		variable_info *current_var = (variable_info *) (current_element->data);
		JITUINT32 var = current_var->id;

		XanHashTable *current_variable_from = alias_info_lookup(from, var);
		XanHashTable *current_variable_to = alias_info_lookup(to, var);

		modified |= merge_alias_class_set(current_variable_from, current_variable_to);

		current_element = current_element->next;
	}

	return modified;
}

static inline XanHashTable *prepare_in_alias_info (XanList *pointer_info){
	XanHashTable *it = xanHashTableNew(1, 0, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
	XanListItem *current_element = pointer_info->first(pointer_info);

	while (current_element != NULL) {
		variable_info *current_var = (variable_info *) (current_element->data);
		JITUINT32 var = current_var->id;
		XanHashTable *current_variable = xanHashTableNew(1, 0, allocFunction, dynamicReallocFunction, freeFunction, alias_class_hash, alias_class_equals);
		it->insert(it, (void *) (var + 1), current_variable);
		current_element = current_element->next;
	}

	return it;
}

static inline XanHashTable *prepare_out_alias_info (XanList *pointer_info, XanHashTable *in_alias_info, t_ir_instruction * inst){
	XanHashTable    *it = xanHashTableNew(1, 0, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
	JITUINT32 to_var = instruction_to_var(inst);
	XanListItem     *current_element;

	current_element = pointer_info->first(pointer_info);
	while (current_element != NULL) {
		variable_info *current_var = (variable_info *) (current_element->data);
		JITUINT32 var = current_var->id;

		if (var != to_var) {
			XanHashTable *current_variable_in = alias_info_lookup(in_alias_info, var);
			it->insert(it, (void*) (var + 1), (void *) current_variable_in);
		} else{
			XanHashTable *current_variable_in = xanHashTableNew(1, 0, allocFunction, dynamicReallocFunction, freeFunction, alias_class_hash, alias_class_equals);
			it->insert(it, (void*) (var + 1), (void *) current_variable_in);
		}

		current_element = pointer_info->next(pointer_info, current_element);
	}

	return it;
}

static inline void destroy_ir_instr_alias_info (XanHashTable *alias){
	if (alias != NULL) {
		XanList *var_in_alias = alias->toList(alias);
		XanListItem *current_element = var_in_alias->first(var_in_alias);
		while (current_element != NULL) {
			XanHashTable *cur_var_alias = (XanHashTable *) (current_element->data);
			cur_var_alias->destroy(cur_var_alias);
			current_element = var_in_alias->next(var_in_alias, current_element);
		}
		var_in_alias->destroyList(var_in_alias);
		alias->destroy(alias);
	}
}

static inline XanList * computer_variables_may_be_pointer (ir_method_t *method){
	XanList         *list;
	JITUINT32 count;
	JITUINT32 max_variables;
	JITUINT32 id;
	JITUINT32 instructionsNumber;

	/* Assertions				*/
	assert(method != NULL);

	/* Allocate the list			*/
	list = xanListNew(allocFunction, freeFunction, NULL);

	/* Fetch the max ID of variables*/
	max_variables = IRMETHOD_getNumberOfVariablesNeededByMethod(method);

	/* Fetch the number of instructions	*/
	instructionsNumber = IRMETHOD_getInstructionsNumber(method);

	/* Fill up the list			*/
	for (count = 0; count < max_variables; count++) {
		JITINT32 added;

		/* Check if the variable escapes	*/
		if (IRMETHOD_isAnEscapedVariable(method, count)) {
			list->append(list, (void *) count);
			continue;
		}

		/* Check if the variable is used as	*
		 * base address				*/
		added = JITFALSE;
		for (id = 0; (id < instructionsNumber) && (!added); id++) {
			t_ir_instruction        *inst;
			inst = IRMETHOD_getInstructionAtPosition(method, id);
			switch (inst->type) {
			    case IRLOADREL:
			    case IRLOADELEM:
			    case IRSTOREELEM:
			    case IRSTOREREL:
			    case IRINITMEMORY:
			    case IRMEMCPY:
				    if (    ((inst->param_1).type == IROFFSET)      &&
					    ((inst->param_1).value.v == count)      ) {
					    list->append(list, (void *) count);
					    added = JITTRUE;
				    }
				    break;
			    case IRNEWOBJ:
			    case IRNEWARR:
			    case IRALLOCA:
				    assert((inst->result).type == IROFFSET);
				    assert((inst->param_1).value.v != 0);
				    if ((inst->result).value.v == count) {
					    list->append(list, (void *) count);
					    added = JITTRUE;
				    }
				    break;
			}
		}
	}

	/* Return				*/
	return list;
}

char *alias_get_version (void){
	return VERSION;
}

char *alias_get_informations (void){
	return INFORMATIONS;
}

char *alias_get_author (void){
	return AUTHOR;
}
