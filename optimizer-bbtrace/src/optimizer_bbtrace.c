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
#include <string.h>

// My headers
#include <optimizer_bbtrace.h>
#include <config.h>
#include <zlib.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

// End

#define INFORMATIONS 	"This is a bbtrace plugin"
#define	AUTHOR		"Yakun Sophia Shao"
#define FILESIZE 50000000000 //50B

static inline JITUINT64 bbtrace_get_ID_job (void);
static inline char * bbtrace_get_version (void);
static inline void bbtrace_do_job (ir_method_t *method);
static inline char * bbtrace_get_informations (void);
static inline char * bbtrace_get_author (void);
static inline JITUINT64 bbtrace_get_dependences (void);
static inline void bbtrace_shutdown (JITFLOAT32 totalExecutionTime);
static inline void bbtrace_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix);
void check_each_method(ir_method_t *method);

static inline JITUINT64 bbtrace_get_invalidations (void);

typedef struct instr_whole{
	ir_instruction_t	*inst;
	JITUINT32			instid;
	JITUINT32			instBBstart;
	JITUINT32			instBBend;
	JITUINT32			instBBlength;
}t_instr_whole;

void print_basic_block(ir_method_t *method, t_instr_whole *instwhole);
void dumper_bbtrace(JITUINT32 methodID, JITUINT32 start, JITUINT32 length, JITUINT32 end);
gzFile fp;
int current_file;
long long unsigned int instruction_counter;
long long unsigned int end_instruction_counter;
struct stat stat_buffer;
char ProgramName[1024];
int temp_counter;

void dumper_bbtrace(JITUINT32 methodID, JITUINT32 start, JITUINT32 length, JITUINT32 end){
	
	char	buf[1024];

	if(instruction_counter < end_instruction_counter ){
		
		temp_counter++;
		if(temp_counter == 1000){
			temp_counter = 0;
			snprintf(buf, 1024, "%s_bbtrace_%d.gz", ProgramName, current_file);
			stat(buf, &stat_buffer);
			if(stat_buffer.st_size > 2000000000){
				gzclose(fp);
				current_file++;
				snprintf(buf, 1024, "%s_bbtrace_%d.gz", ProgramName,current_file );
				fp = gzopen(buf, "w");
			}
		}
		gzprintf(fp, "\n%u,%u,%u,%u", methodID, start, end, length);
	}
	else{
		end_instruction_counter += FILESIZE;
		gzclose(fp);
		current_file++;
		snprintf(buf, 1024, "%s_bbtrace_%d.gz", ProgramName,current_file );
		fp = gzopen(buf, "w");
		gzprintf(fp, "\n%u,%u,%u,%u", methodID, start, end, length);
	}

	instruction_counter += length;
	
}

ir_lib_t	*irLib		= NULL;
ir_optimizer_t	*irOptimizer	= NULL;
char 	        *prefix		= NULL;


ir_optimization_interface_t plugin_interface = {
	bbtrace_get_ID_job	, 
	bbtrace_get_dependences	,
	bbtrace_init		,
	bbtrace_shutdown		,
	bbtrace_do_job		, 
	bbtrace_get_version	,
	bbtrace_get_informations	,
	bbtrace_get_author,
	bbtrace_get_invalidations
};


static inline void bbtrace_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix){

	/* Assertions			*/
	assert(lib != NULL);
	assert(optimizer != NULL);
	assert(outputPrefix != NULL);

	irLib		= lib;
	irOptimizer	= optimizer;
	prefix		= outputPrefix;
	
	fprintf(stdout,"%s", IRPROGRAM_getProgramName());
	fprintf(stderr,"%s", IRPROGRAM_getProgramName());
	snprintf(ProgramName, 1024, "%s", IRPROGRAM_getProgramName());
	
	end_instruction_counter = FILESIZE;
	
	current_file = 0;
	
	char	buf[1024];
	snprintf(buf, 1024, "%s_bbtrace_%d.gz", ProgramName,current_file );
	fp = gzopen(buf, "w");
	instruction_counter = 0;
	temp_counter = 0;
	/* Return			*/
	return ;
}

static inline void bbtrace_shutdown (JITFLOAT32 totalExecutionTime){
    irLib		= NULL;
	irOptimizer	= NULL;
	prefix		= NULL;
	char 		buf[1024];
	FILE *output;
	/*FILE *timestamp;*/
	
	snprintf(buf, 1024, "%s_bbtrace_output", ProgramName);
	output = fopen(buf, "w");
	fprintf(stderr, "The total length instruction is %lld\n", instruction_counter);
	fprintf(output, "The total instruction is %lld\n", instruction_counter);
	fclose(output);
	gzclose(fp);
}

static inline JITUINT64 bbtrace_get_ID_job (void){
	//return BBTRACE;
	return CUSTOM;
}

static inline JITUINT64 bbtrace_get_dependences (void){
	return 0;
}

static inline void bbtrace_do_job (ir_method_t *method){
	
	fprintf(stdout,"%s", IRPROGRAM_getProgramName());
	fprintf(stderr,"%s", IRPROGRAM_getProgramName());
	XanList			*l;
	XanListItem 	*item;
	ir_method_t		*current_method;

	l = xanListNew(allocFunction, freeFunction, NULL);
	l = IRPROGRAM_getIRMethods();
	item = xanList_first(l);
	while(item != NULL){
		current_method = item->data;
		check_each_method(current_method);
		item = item->next;
	}
	l->destroyList(l);
	return;
}
//end of do job
void check_each_method(ir_method_t *method){
	XanList		*l;
	XanListItem *item;
	JITUINT32	c;
	JITUINT32	instID;
	JITUINT32	instBBstart;
	JITUINT32	instBBend;
	ir_instruction_t	*inst;
	t_instr_whole    *instwhole;		
	IRBasicBlock		*basicBlock;

	/*   Variable Initialization    */
	inst		= NULL;
	
	/* Assertions     */
	assert(method != NULL);

	/* Check if the method has a body	*/
	if (!IRMETHOD_hasMethodInstructions(method)){
		return ;
	}	

	/* Put all the instructions in the list */
	l = xanListNew(allocFunction, freeFunction, NULL);
	for (c=0; c< IRMETHOD_getInstructionsNumber(method); c++){
		inst	= IRMETHOD_getInstructionAtPosition(method, c);
		if(inst->type != IRNOP){
			continue;
		}
		IRMETHOD_deleteInstruction(method, inst );
		c--;
	
	}
	IROPTIMIZER_callMethodOptimization(irOptimizer, method, BASIC_BLOCK_IDENTIFIER);
	for (c=0; c< IRMETHOD_getInstructionsNumber(method); c++){
		inst				= IRMETHOD_getInstructionAtPosition(method, c);
		instwhole			= (t_instr_whole *)malloc(sizeof(t_instr_whole));
		instwhole->inst		= inst;
		instwhole->instid	= IRMETHOD_getInstructionPosition(inst);
		basicBlock			= IRMETHOD_getBasicBlockContainingInstruction(method, inst);
		instwhole->instBBstart = basicBlock->startInst;
		instwhole->instBBlength = IRMETHOD_getBasicBlockLength(basicBlock);
		instwhole->instBBend  = basicBlock->endInst;
		l->append(l, instwhole);
	}
	//insert native calls to dynamically print the instructions
	item	= xanList_first(l);

	while(item != NULL){

		//fetch the instruction from the xan list
		instwhole				= item->data;
		instID					= instwhole->instid;
		instBBstart				= instwhole->instBBstart;
		instBBend				= instwhole->instBBend;
		if(instID == instBBstart){
			print_basic_block(method, instwhole);	
		}
		item = item->next;
	}
	//end of while loop to check each instruction in the method	
	return;

}


void print_basic_block(ir_method_t *method, t_instr_whole *instwhole){
	
	JITUINT32		instID;
	JITUINT32		bblength;
	JITUINT32		bbstart;
	JITUINT32		bbend;
	JITUINT32		instType;
	JITNUINT 		methodID;
	
	//JITINT8 			*signature;

	ir_instruction_t	*instPrintBB;
	ir_instruction_t	*inst;

	ir_item_t			*ir_item;
	ir_item_t			*ir_to_dump;

	inst			= instwhole->inst;
	instID			= instwhole->instid;
	bblength		= instwhole->instBBlength;
	bbstart			= instwhole->instBBstart;
	bbend			= instwhole->instBBend;

	methodID 		= IRMETHOD_getMethodID ( method);
	instType		= IRMETHOD_getInstructionType(inst);
	
	if(instType == IRLABEL ||instType == IRSTARTFINALLY || instType == IRSTARTCATCHER || instType == IRSTARTFILTER || (instID == 0 ) ){
		instPrintBB = IRMETHOD_newInstructionAfter(method, inst);
	}
	else{
		instPrintBB = IRMETHOD_newInstructionBefore(method, inst);
	}
	

	IRMETHOD_setInstructionType(method, instPrintBB, IRNATIVECALL);
	
	ir_item				= IRMETHOD_getInstructionParameter1(instPrintBB);
	ir_item->value.v			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type	= IRTYPE;

	ir_item					= IRMETHOD_getInstructionParameter2(instPrintBB);
	ir_item->value.v			= (IR_ITEM_VALUE)(JITNUINT)"dumper_bbtrace";
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;
	
	ir_item					= IRMETHOD_getInstructionParameter3(instPrintBB);
	ir_item->value.v			= (IR_ITEM_VALUE)(JITNUINT)dumper_bbtrace;
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;
	
	ir_item					= IRMETHOD_getInstructionParameter3(instPrintBB);
	ir_item->value.v			= (IR_ITEM_VALUE)(JITNUINT)dumper_bbtrace;
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;
	
	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	
	ir_to_dump->value.v  	= methodID;
	ir_to_dump->type	= IRUINT32;
	ir_to_dump->internal_type = IRUINT32;
	instPrintBB->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instPrintBB->callParameters->append(instPrintBB->callParameters, ir_to_dump);
	
	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	ir_to_dump->value.v  	= bbstart;
	ir_to_dump->type	= IRUINT32;
	ir_to_dump->internal_type = IRUINT32;
	instPrintBB->callParameters->append(instPrintBB->callParameters, ir_to_dump);
	
	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	ir_to_dump->value.v  	= bblength;
	ir_to_dump->type	= IRUINT32;
	ir_to_dump->internal_type = IRUINT32;
	instPrintBB->callParameters->append(instPrintBB->callParameters, ir_to_dump);
	
	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	ir_to_dump->value.v  	= bbend;
	ir_to_dump->type	= IRUINT32;
	ir_to_dump->internal_type = IRUINT32;
	instPrintBB->callParameters->append(instPrintBB->callParameters, ir_to_dump);
		
	return;
}


static inline char * bbtrace_get_version (void){
	return VERSION;
}

static inline char * bbtrace_get_informations (void){
	return INFORMATIONS;
}

static inline char * bbtrace_get_author (void){
	return AUTHOR;
}

static inline JITUINT64 bbtrace_get_invalidations (void){
	return INVALIDATE_ALL;
}







