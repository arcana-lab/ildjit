/*
 * Copyright (C) 2008  Ceriani Marco
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; with#include <iljit-utils.h>out even the implied warranty of
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
#include <optimizer_ddg.h>

iSet_t * newVarInfo(t_ir_instruction * inst, ir_lib_t * irlib){
	iSet_t * node;
	node = allocFunction( sizeof(iSet_t));
	node->inst = inst;
	node->next = NULL;
	return node;
}

iSet_t * addElementToSet(iSet_t * info, iSet_t ** setptr, ir_lib_t * irlib){
	iSet_t * tmp;

	if (*setptr == NULL || (*setptr)->inst < info->inst){
		info->next = *setptr;
		*setptr = info;
		return info;
	} else if ((*setptr)->inst == info->inst){
		freeFunction( info);
		return *setptr;
	}

	tmp = *setptr;
	while(tmp->next != NULL){
		if(tmp->next->inst < info->inst){
			break;
		}
		tmp = tmp->next;
	}
	if(tmp->inst == info->inst){
		freeFunction( info);
		return tmp;
	} else {
		info->next = tmp->next;
		tmp->next = info;
		return info;
	}
}

void add_to_set(t_ir_instruction * inst, iSet_t ** set_ptr, ir_lib_t * irlib) {
	iSet_t * tmp;
	iSet_t * new_info;

	if (*set_ptr == NULL || (*set_ptr)->inst < inst){
		/* Add to head */
		new_info = newVarInfo(inst, irlib);
		new_info->next = *set_ptr;
		*set_ptr = new_info;
		return;
	} else if ((*set_ptr)->inst == inst){
		/* Already in head*/
		return;
	}

	/* Find position */
	tmp = *set_ptr;
	while(tmp->next != NULL){
		if(tmp->next->inst < inst){
			break;
		}
		tmp = tmp->next;
	}
	if(tmp->inst == inst){
		return;
	} else {
		new_info = newVarInfo(inst, irlib);
		new_info->next = tmp->next;
		tmp->next = new_info;
		return;
	}
}

void free_set(iSet_t ** set, ir_lib_t * irlib){
	iSet_t * tmp = *set;
	iSet_t * next;

	while(tmp != NULL){
		next = tmp->next;
		freeFunction( tmp);
		tmp = next;
	}
	*set = NULL;
}

/***************************************************************************/

void print_set(ir_method_t * method, iSet_t * set){
	iSet_t * tmp = set;
	JITUINT32 id;

	while(tmp != NULL){
		id = method->getInstructionID(method, tmp->inst);
		fprintf(stderr, "%d ", id);
		tmp = tmp->next;
	}
	fprintf(stderr, "\n");
}

void print_all_sets(ir_method_t * method, deps_t * deps){

	JITUINT32 var;
	JITUINT32 inst;
	JITBOOLEAN skip;

	for (inst = 0; inst < method->getInstructionsNumber(method); inst++){
		if (deps[inst].uses == NULL && deps[inst].uses == NULL){
			continue;
		}
		// Skip instructions without content.
		skip = 1;
		for(var = 0; var < method->getMaxVariables(method) + 1; var ++)
		{
			if(deps[inst].uses[var] != NULL || deps[inst].defs[var] != NULL)
			{
				skip = 0;
			}
		}
		if (skip)
			continue;

		fprintf(stderr, "\nList of Instruction %d\n", inst);
		assert(deps[inst].uses != NULL);
		assert(deps[inst].defs != NULL);
		for(var = 0; var < method->getMaxVariables(method) + 1; var ++){
			if(deps[inst].uses[var] != NULL){
				fprintf(stderr, "\tvar %d) U: ", var);
				print_set(method, deps[inst].uses[var]);
			}
			if(deps[inst].defs[var] != NULL){
				fprintf(stderr, "\tvar %d) D: ", var);
				print_set(method, deps[inst].defs[var]);
			}
		}
	}
	fprintf(stderr, "\n");

}

/***************************************************************************/

JITBOOLEAN merge_iSets(deps_t * deps, JITUINT32 sets, JITUINT32 newSets, ir_method_t *method, JITUINT64 def){

	iSet_t *iter;
	iSet_t *dest_iter;
	iSet_t *new_info;
	ir_lib_t *irlib;
	JITUINT32 num_vars;
	JITUINT64 var;
	JITBOOLEAN addedInfo;

	irlib = (ir_lib_t *)method->sys;
	assert(deps[newSets].uses != NULL);
	assert(deps[newSets].defs != NULL);
	assert(deps[sets].uses != NULL);
	assert(deps[sets].defs != NULL);

	num_vars = method->getMaxVariables(method) + 1;
	addedInfo = 0;
	for(var = 0; var < num_vars; var++)
	{

		/* Merge uses */
		iter		= deps[sets].uses[var];
		dest_iter	= deps[newSets].uses[var];
		while( iter != NULL)
		{
			if (deps[newSets].uses[var] == NULL || iter->inst > deps[newSets].uses[var]->inst)
			{	// Add to head
				new_info = newVarInfo(iter->inst, irlib);
				new_info->next = deps[newSets].uses[var];
				deps[newSets].uses[var] = new_info;
				dest_iter = new_info;
				if (var != def)
				{
					addedInfo = 1;
				}
			} 
			else if (iter->inst < deps[newSets].uses[var]->inst)
			{

				while(dest_iter->next != NULL && dest_iter->next->inst >= iter->inst)
				{
					dest_iter = dest_iter->next;
				}
				if(dest_iter->inst != iter->inst)
				{
					new_info = newVarInfo(iter->inst, irlib);
					new_info->next = dest_iter->next;
					dest_iter->next = new_info;
					if (var != def)
					{
						addedInfo = 1;
					}
				}
			}
			iter = iter->next;
		}

		/* Merge defs */
		iter		= deps[sets].defs[var];
		dest_iter	= deps[newSets].defs[var];
		while( iter != NULL)
		{
			if (deps[newSets].defs[var] == NULL || iter->inst > deps[newSets].defs[var]->inst)
			{	// Add to head
				new_info = newVarInfo(iter->inst, irlib);
				new_info->next = deps[newSets].defs[var];
				deps[newSets].defs[var] = new_info;
				dest_iter = new_info;
				if (var != def)
				{
					addedInfo = 1;
				}
			} 
			else if (iter->inst < deps[newSets].defs[var]->inst)
			{
				while(dest_iter->next != NULL && dest_iter->next->inst >= iter->inst)
				{
					dest_iter = dest_iter->next;
				}
				if(dest_iter->inst != iter->inst)
				{
					new_info = newVarInfo(iter->inst, irlib);
					new_info->next = dest_iter->next;
					dest_iter->next = new_info;
					if (var != def)
					{
						addedInfo = 1;
					}
				}
			}
			iter = iter->next;
		}
	}
	return addedInfo;
}

/***************************************************************************/

void appendToWl(JITUINT32 id, iWorkList_t * fifo_ptr, ir_lib_t * irlib) {

	assert(fifo_ptr != NULL);
	idSet_t * new_info;

	new_info = allocFunction( sizeof(idSet_t));
	new_info->instID = id;

	if (fifo_ptr->head == NULL) {
		fifo_ptr->head = new_info;
		fifo_ptr->tail = new_info;
	} else {
		assert(fifo_ptr->tail != NULL);
		fifo_ptr->tail->next = new_info;
		fifo_ptr->tail = new_info;
	}

}

void pushToWl(JITUINT32 id, iWorkList_t * fifo_ptr, ir_lib_t * irlib) {

	assert(fifo_ptr != NULL);
	idSet_t * new_info;

	new_info = allocFunction( sizeof(idSet_t));
	new_info->instID = id;

	new_info->next = fifo_ptr->head;
	fifo_ptr->head = new_info;

	if (fifo_ptr->tail == NULL) {
		fifo_ptr->tail = new_info;
	}
}

JITBOOLEAN wlEmpty(iWorkList_t * fifo_ptr) {

	if (fifo_ptr->head == NULL){
		return 1;
	} else {
		return 0;
	}
}


JITUINT32 wlGet(iWorkList_t * fifo_ptr, ir_lib_t * irlib) {

	assert(fifo_ptr != NULL);
	assert(fifo_ptr->head != NULL);
	idSet_t * node;
	JITUINT32 id;

	node = fifo_ptr->head;
	fifo_ptr->head = fifo_ptr->head->next;
	id = node->instID;
	freeFunction( node);

	if (node == fifo_ptr->tail) {
		fifo_ptr->tail = NULL;
	}

	return id;
}

