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
#include <optimizer_wholetrace.h>
#include <config.h>
#include <zlib.h>
#include <stdlib.h>
#include <xanlib.h>
// End

#define INFORMATIONS 	"This is a wholetrace plugin"
#define	AUTHOR		"Yakun Sophia Shao"
#define SAMPLE_INTERVAL	10000000
#define TOTAL_SAMPLE_FILE 10


static inline JITUINT64 wholetrace_get_ID_job (void);
static inline char * wholetrace_get_version (void);
static inline void wholetrace_do_job (ir_method_t * method);
static inline char * wholetrace_get_informations (void);
static inline char * wholetrace_get_author (void);
static inline JITUINT64 wholetrace_get_dependences (void);
static inline void wholetrace_shutdown (JITFLOAT32 totalExecutionTime);
static inline void wholetrace_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix);
static inline JITUINT64 wholetrace_get_invalidations (void);
void dump_each_method(ir_method_t * method);

/*=============functions used to dynamically dump the code =================*/

void dumper_parnum(JITUINT32 value);
void dumper_print_inst(JITUINT32 value, JITUINT32 instid, JITNUINT methodID);

void dumper_int(unsigned int type, unsigned int size, JITNINT value);
void dumper_uint(unsigned int type, unsigned int size, JITNUINT value );
void dumper_label(unsigned int type, unsigned int size, JITNUINT value );
void dumper_float(unsigned int type, unsigned int size, JITNFLOAT value);
void dumper_valuetype(unsigned int type);
void dumper_var(int varid);

/*=================dump code functions end================================*/

void insert_dumper_par (ir_method_t *method, ir_instruction_t *inst, JITUINT32 parnum, bool isAfter);
void insert_dumper_var (ir_method_t *method, ir_instruction_t *inst, ir_item_t *par, bool isAfter);
void insert_dumper_int (ir_method_t *method, ir_instruction_t *inst, ir_item_t *par, JITUINT32 type, bool isAfter);
void insert_dumper_uint (ir_method_t *method, ir_instruction_t *inst, ir_item_t *par, JITUINT32 type, bool isAfter);
void insert_dumper_label (ir_method_t *method, ir_instruction_t *inst, ir_item_t *par, JITUINT32 type, bool isAfter);
void insert_dumper_float (ir_method_t *method, ir_instruction_t *inst, ir_item_t *par, JITUINT32 type, bool isAfter);
void insert_dumper_valuetype (ir_method_t *method, ir_instruction_t *inst, ir_item_t *par, JITUINT32 type, bool isAfter);

/*=================dump parameters functions================================*/
void insert_dumper_parameter(ir_method_t *method, ir_instruction_t *inst, ir_item_t *par1, bool isAfter);

//statically print functions

void print_data(unsigned int type, unsigned int size);
void print_label(unsigned int type, unsigned int size, unsigned int value);
void print_parameter(unsigned int parid, ir_item_t *par);
void print_instruction(JITNUINT methodid, JITUINT32 instid, JITUINT32 insttype, JITINT32 bblockID, JITINT32 loopID);
JITUINT32 findBasicBlockID (ir_method_t *method, JITUINT32 instID);

ir_lib_t	*irLib		= NULL;
ir_optimizer_t	*irOptimizer	= NULL;
char 	        *prefix		= NULL;

gzFile 	fp;
gzFile 	staticfp;
FILE* method_map;
FILE* loop_inst;
FILE* bb_dependence;

long long unsigned int dynamic_instruction_counter;
long long unsigned int static_instruction_counter;
char ProgramName[1024];

ir_optimization_interface_t plugin_interface = {
	wholetrace_get_ID_job	, 
	wholetrace_get_dependences	,
	wholetrace_init		,
	wholetrace_shutdown		,
	wholetrace_do_job		, 
	wholetrace_get_version	,
	wholetrace_get_informations	,
	wholetrace_get_author,
	wholetrace_get_invalidations
};

static inline void wholetrace_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix){
	/* Assertions			*/
	char	buf[1024];
	assert(lib != NULL);
	assert(optimizer != NULL);
	assert(outputPrefix != NULL);

	irLib		= lib;
	irOptimizer	= optimizer;
	prefix		= outputPrefix;

	dynamic_instruction_counter = 0;
	static_instruction_counter = 0;
	
	snprintf(ProgramName, 1024, "%s", IRPROGRAM_getProgramName());
	
	snprintf(buf, 1024, "%s_dump.gz", ProgramName );
	fp = gzopen(buf, "w");
	snprintf(buf, 1024, "%s_method_map", ProgramName );
	method_map = fopen(buf, "w");
	snprintf(buf, 1024, "%s_loop_inst", ProgramName );
	loop_inst = fopen(buf, "w");
	snprintf(buf, 1024, "%s_bb_dependence", ProgramName );
	bb_dependence = fopen(buf, "w");
	snprintf(buf, 1024, "%s_static_dump.gz", ProgramName );
	staticfp = gzopen(buf, "w");
	/* Return			*/
	return ;
}

static inline void wholetrace_shutdown (JITFLOAT32 totalExecutionTime){
	irLib		= NULL;
	irOptimizer	= NULL;
	prefix		= NULL;
	
	char buf[1024];
	FILE* output;
	gzclose(fp);
    gzclose(staticfp);
	fclose(method_map);
	fclose(loop_inst);
	fclose(bb_dependence);
	snprintf(ProgramName, 1024, "%s", IRPROGRAM_getProgramName());
	snprintf(buf, 1024, "%s_wholetrace_output", ProgramName );
	output = fopen(buf, "w");
	fprintf(stderr, "the total wholetrace executed instruction is %lld\n", dynamic_instruction_counter);
	fprintf(output, "the total wholetrace executed instruction is %lld\n", dynamic_instruction_counter);
	fclose(output);
    
    snprintf(buf, 1024, "%s_statictrace_output", ProgramName );
    output = fopen(buf, "w");
    fprintf(stderr, "the total statictrace instruction is %llu\n", static_instruction_counter);
    fprintf(output, "the total statictrace instruction is %llu\n", static_instruction_counter);
    fclose(output);
}

static inline JITUINT64 wholetrace_get_ID_job (void){
	return CUSTOM;
}


static inline JITUINT64 wholetrace_get_dependences (void){
	return 0;
}

static inline JITUINT64 wholetrace_get_invalidations (void){
	return INVALIDATE_ALL;
}

/*====================dynamically dump code functions ====================*/

void dumper_parnum(JITUINT32 value){
	gzprintf( fp,"\n%d,", value);
}

void dumper_print_inst(JITUINT32 value, JITUINT32 instid, JITNUINT methodID){
    gzprintf( fp, "\n\n0,%u,%d,%u",  methodID, instid, value);
	dynamic_instruction_counter++;
}

void dumper_int(unsigned int type, unsigned int size, JITNINT value){
    gzprintf(fp, "%u,%u,%u", type, size, value);
}

void dumper_uint(unsigned int type, unsigned int size, JITNUINT value ){
    gzprintf(fp, "%u,%u,%u", type, size, value);
}

void dumper_label(unsigned int type, unsigned int size, JITNUINT value ){
    gzprintf(fp, "%u,%u,%u", type, size, value);
}

void dumper_float(unsigned int type, unsigned int size, JITNFLOAT value){
    gzprintf(fp, "%u,%u,%f", type, size, value);
}

void dumper_valuetype(unsigned int type){
    gzprintf( fp, "%u,0,0", type);
}

void dumper_var(int varid){
    gzprintf(fp, "%u,%u,", IROFFSET, varid);
}

/*========================dump code do job ================================*/

static inline void wholetrace_do_job (ir_method_t * method){
	
	XanList			*l;
	XanListItem		*item;
	ir_method_t 	*current_method;

	l = IRPROGRAM_getIRMethods();
	item = xanList_first(l);

	while(item != NULL){
		current_method = item->data;
		dump_each_method(current_method);
		item=item->next;
	}
	xanList_destroyList(l);
	return;

}

void dump_each_method(ir_method_t * method){

	/* Assertions     */
	assert(method != NULL);
	
	/* Check if the method has a body	*/
	if (!IRMETHOD_hasMethodInstructions(method)){
		return ;
	}	
	

	XanList		*l;
	XanListItem *item;
	JITUINT32	c;
	ir_instruction_t	*inst;
	
	ir_instruction_t 	*instPrintInstType;
	
	JITUINT32			instType;
	JITUINT32 			instID;
	
	JITINT8 			*signature;
	IR_ITEM_VALUE			methodID;
	ir_item_t			*ir_to_dump;

	ir_item_t			*par1;
	ir_item_t			*inst_result;

	methodID 	= IRMETHOD_getMethodID (method);
	signature	= IRMETHOD_getMethodName(method);
    
    fprintf(method_map, "M,%llu,%s\n", methodID, signature);
    /*printf("M,%llu,%s\n", methodID, signature);*/
    fprintf(bb_dependence, "M,%llu,%s\n", methodID, signature);
    /*if (strcmp("bb_gemm", signature) != 0 */
    /*&& strcmp("bb_gemm_kernel", signature) != 0 )*/
    /*return;*/
    /*if (strcmp("triad", signature) != 0 )*/
    /**//*if (strstr(signature, "triad") == NULL )*/
    /*return;*/
    /*printf("PASS, M,%llu,%s\n", methodID, signature);*/
    /*if (strcmp("stencil", signature) != 0 */
    /*&& strcmp("stencil_kernel", signature) != 0 )*/
    /*return;*/
	typedef struct instr_whole{
		ir_instruction_t 	*inst;
		JITUINT32			instid;
	}t_instr_whole;

	t_instr_whole		*instwhole;

	/* Assertions     */
	assert(method != NULL);
	
	for (c=0; c< IRMETHOD_getInstructionsNumber(method); c++){
      inst	= IRMETHOD_getInstructionAtPosition(method, c);
      if(inst->type != IRNOP){
          continue;
      }
      IRMETHOD_deleteInstruction(method, inst );
      c--;
	}

    IROPTIMIZER_callMethodOptimization(irOptimizer, method, BASIC_BLOCK_IDENTIFIER);
    
    JITUINT32     basicBlocksNumber;
    basicBlocksNumber   = IRMETHOD_getNumberOfMethodBasicBlocks(method);
    assert(basicBlocksNumber > 0);

    for ( c = 0; c < basicBlocksNumber; c++){
      IRBasicBlock        *basicBlock, *succBasicBlock;
      ir_instruction_t    *succ;
      basicBlock        = IRMETHOD_getBasicBlockAtPosition(method, c);
      assert(basicBlock != NULL);
      assert(basicBlock->startInst <= basicBlock->endInst);
      inst              = IRMETHOD_getInstructionAtPosition(method, basicBlock->endInst);
      assert(inst != NULL);
      succ              = IRMETHOD_getSuccessorInstruction(method, inst, NULL);
      while(succ != NULL){
        
        JITINT32            basicBlockID;
        succBasicBlock       = IRMETHOD_getBasicBlockContainingInstruction(method, succ);
        basicBlockID         = IRMETHOD_getBasicBlockPosition(method, succBasicBlock);
        fprintf(bb_dependence, "%u,%u\n", c, basicBlockID);
        //fprintf(stdout, "%u,%u\n", c, basicBlockID);
        succ                 = IRMETHOD_getSuccessorInstruction(method, inst, succ);
      }
    }
    IROPTIMIZER_callMethodOptimization(irOptimizer, method, LOOP_IDENTIFICATION);
    
    circuit_t          *loop;
    XanListItem     *loop_item;
	if ( method->loop != NULL){
      if (method->loop->len != 0)  {
          
        fprintf(loop_inst, "M,%llu,%s\n", methodID, signature);
        JITINT32            loopID;
        loop_item = xanList_first(method->loop);
        while (loop_item != NULL) {
          
          loop = (circuit_t*) (loop_item->data);
          assert(loop != NULL);
          loopID      = loop->loop_id;
          
          XanList         *instructionsOfLoop;
          XanListItem     *instructionsOfLoopItem;

          instructionsOfLoop 	= IRMETHOD_getCircuitInstructions(method, loop);
          assert(instructionsOfLoop != NULL);
          instructionsOfLoopItem = xanList_first(instructionsOfLoop);
          
          while (instructionsOfLoopItem != NULL){
            inst        = instructionsOfLoopItem->data;
            instID		= IRMETHOD_getInstructionPosition( inst);
            instType	= IRMETHOD_getInstructionType(inst);
            JITINT32            bblockID;
            bblockID    = findBasicBlockID(method, instID);
            //print instruction line 
            fprintf(loop_inst, "0,%llu,",  methodID);
            fprintf(loop_inst, "%u,",  instID);
            fprintf(loop_inst, "%u,",  instType);
            fprintf(loop_inst, "%d,",  bblockID);
            fprintf(loop_inst, "%d\n",  loopID);
            
            instructionsOfLoopItem = instructionsOfLoopItem->next;
          }
          loop_item = loop_item->next;
        }
      }
	}
    
	IROPTIMIZER_callMethodOptimization(irOptimizer, method,  METHOD_PRINTER);
    //statically print instruction trace
	ir_item_t			*par;
	ir_item_t			*result;
    for (c=0; c< IRMETHOD_getInstructionsNumber(method); c++){
      inst	= IRMETHOD_getInstructionAtPosition(method, c);
      instID		= IRMETHOD_getInstructionPosition( inst);
      instType	= IRMETHOD_getInstructionType(inst);
      JITINT32            bblockID;
      bblockID    = findBasicBlockID(method, instID);
      JITINT32            loopID;
      loop = IRMETHOD_getTheMoreNestedCircuitOfInstruction(method, inst);
      if (loop == NULL){
        loopID = -1;
      }
      else{
        loopID = loop->loop_id;
      }
      //print instruction line 
      print_instruction(methodID, instID, instType, bblockID, loopID);
      if (instType == IRMOVE){
        par = IRMETHOD_getInstructionParameter1(inst);
        print_parameter(1, par);
        result = IRMETHOD_getInstructionResult(inst);
        print_parameter(4, result);
      }
      //print parameters, notice that IRSTORE is treated differently here
      switch(IRMETHOD_getInstructionParametersNumber(inst)){
        case 3:
          par = IRMETHOD_getInstructionParameter1(inst);
          print_parameter(1, par);
          par = IRMETHOD_getInstructionParameter2(inst);
          print_parameter(2, par);
          par = IRMETHOD_getInstructionParameter3(inst);
          print_parameter(3, par);
          break;
        case 2:
          par = IRMETHOD_getInstructionParameter1(inst);
          print_parameter(1, par);
          par = IRMETHOD_getInstructionParameter2(inst);
          print_parameter(2, par);
          break;
        case 1:
          par = IRMETHOD_getInstructionParameter1(inst);
          print_parameter(1, par);
          break;
        case 0:
          break;
        default:
          break;
      }
      //call parameters
      if(IRMETHOD_getInstructionCallParametersNumber(inst) > 0) {
        JITUINT32 call_count;
        for (call_count = 0; call_count < IRMETHOD_getInstructionCallParametersNumber(inst); call_count++){
          par = IRMETHOD_getInstructionCallParameter(inst, call_count);
          print_parameter(call_count, par);
        }
      }
      //print results
      if ((IRMETHOD_getInstructionResult(inst)->type != NOPARAM) && (IRMETHOD_getInstructionResult(inst)->type != IRVOID)){
        result = IRMETHOD_getInstructionResult(inst);
        print_parameter(4, result);
      }
    }
    
    //Finish statically profiling
    //Start Dynamic Instruction Trace
	/* Put all the instructions in the list */
    
    l = xanList_new(allocFunction, freeFunction, NULL);
	for (c=0; c< IRMETHOD_getInstructionsNumber(method); c++){
      inst	= IRMETHOD_getInstructionAtPosition(method, c);
      instwhole = (t_instr_whole *)malloc(sizeof(t_instr_whole));
      instwhole->inst 		= inst;
      instwhole->instid		= IRMETHOD_getInstructionPosition( inst);
      xanList_append(l, instwhole);
	}
	
    //insert native calls to dynamically print the instructions
	item	= xanList_first(l);
	while(item != NULL){
      //instruction itself
      //fetch the instruction from the xan list
      instwhole				= item->data;
      inst					= instwhole->inst;
      
      item = item->next;
      
      instType	= IRMETHOD_getInstructionType(inst);
      instID		= instwhole->instid;
      
      if(instType == IRLABEL ||instType == IRSTARTFINALLY || instType == IRSTARTCATCHER || instType == IRSTARTFILTER || (instID == 0) ){
        
          if ((IRMETHOD_getInstructionResult(inst)->type != NOPARAM) && (IRMETHOD_getInstructionResult(inst)->type != IRVOID)){
              inst_result	= IRMETHOD_getInstructionResult(inst);
              insert_dumper_parameter(method, inst, inst_result, 1);
              insert_dumper_par(method, inst, 4, 1);
            }
          
          //print parameters value and varID
          switch(IRMETHOD_getInstructionParametersNumber(inst)){
            case 3: 
              par1	= IRMETHOD_getInstructionParameter3(inst);
              insert_dumper_parameter(method, inst, par1, 1);
              insert_dumper_par(method, inst, 3, 1);
              
            case 2:
              par1	= IRMETHOD_getInstructionParameter2(inst);
              insert_dumper_parameter(method, inst, par1, 1);
              insert_dumper_par(method, inst, 2, 1);
              
            case 1:
              par1	= IRMETHOD_getInstructionParameter1(inst);
              insert_dumper_parameter(method, inst, par1, 1);
              insert_dumper_par(method, inst, 1, 1);
              break;
            case 0:
              break;
            default:
              break;
           }  
        instPrintInstType = IRMETHOD_newInstructionOfTypeAfter(method, inst, IRNATIVECALL);
        
        IRMETHOD_setInstructionParameter1(method, instPrintInstType, IRVOID, 0, IRTYPE, IRTYPE, NULL);
        IRMETHOD_setInstructionParameter2(method, instPrintInstType, (IR_ITEM_VALUE)(JITNUINT)"dumper_print_inst", 0, IRNINT, IRNINT, NULL);
        IRMETHOD_setInstructionParameter3(method, instPrintInstType, (IR_ITEM_VALUE)(JITNUINT)dumper_print_inst, 0, IRNINT, IRNINT, NULL);
        
        ir_to_dump			= allocFunction(sizeof(ir_item_t));
        ir_to_dump->value.v  	= instType;
        ir_to_dump->type	= IRUINT32;
        ir_to_dump->internal_type = IRUINT32;
        IRMETHOD_addInstructionCallParameter(method, instPrintInstType, ir_to_dump);
        
        ir_to_dump			= allocFunction(sizeof(ir_item_t));
        ir_to_dump->value.v  	= instID;
        ir_to_dump->type	= IRUINT32;
        ir_to_dump->internal_type = IRUINT32;
        IRMETHOD_addInstructionCallParameter(method, instPrintInstType, ir_to_dump);
        
        ir_to_dump			= allocFunction(sizeof(ir_item_t));
        ir_to_dump->value.v  	= (IR_ITEM_VALUE)(JITNUINT)methodID;
        ir_to_dump->type	= IRNUINT;
        ir_to_dump->internal_type = IRNUINT;
        IRMETHOD_addInstructionCallParameter(method, instPrintInstType, ir_to_dump);
      }
      else{
        instPrintInstType = IRMETHOD_newInstructionOfTypeBefore(method, inst, IRNATIVECALL);
        IRMETHOD_setInstructionParameter1(method, instPrintInstType, IRVOID, 0, IRTYPE, IRTYPE, NULL);
        IRMETHOD_setInstructionParameter2(method, instPrintInstType, (IR_ITEM_VALUE)(JITNUINT)"dumper_print_inst", 0, IRNINT, IRNINT, NULL);
        IRMETHOD_setInstructionParameter3(method, instPrintInstType, (IR_ITEM_VALUE)(JITNUINT)dumper_print_inst, 0, IRNINT, IRNINT, NULL);
        
        ir_to_dump			= allocFunction(sizeof(ir_item_t));
        ir_to_dump->value.v  	= instType;
        ir_to_dump->type	= IRUINT32;
        ir_to_dump->internal_type = IRUINT32;
        IRMETHOD_addInstructionCallParameter(method, instPrintInstType, ir_to_dump);
        
        ir_to_dump			= allocFunction(sizeof(ir_item_t));
        ir_to_dump->value.v  	= instID;
        ir_to_dump->type	= IRUINT32;
        ir_to_dump->internal_type = IRUINT32;
        IRMETHOD_addInstructionCallParameter(method, instPrintInstType, ir_to_dump);
        
        ir_to_dump			= allocFunction(sizeof(ir_item_t));
        ir_to_dump->value.v  	= (IR_ITEM_VALUE)(JITNUINT)methodID;
        ir_to_dump->type	= IRNUINT;
        ir_to_dump->internal_type = IRNUINT;
        IRMETHOD_addInstructionCallParameter(method, instPrintInstType, ir_to_dump);
        
          //print parameters value and varID
          switch(IRMETHOD_getInstructionParametersNumber(inst)){
            case 3: 
              par1	= IRMETHOD_getInstructionParameter1(inst);
              insert_dumper_par(method, inst, 1, 0);
              insert_dumper_parameter(method, inst, par1, 0);
              
              par1	= IRMETHOD_getInstructionParameter2(inst);
              insert_dumper_par(method, inst, 2, 0);
              insert_dumper_parameter(method, inst, par1, 0);
              
              par1	= IRMETHOD_getInstructionParameter3(inst);
              insert_dumper_par(method, inst, 3, 0);
              insert_dumper_parameter(method, inst, par1, 0);
              break;
            case 2:
              par1	= IRMETHOD_getInstructionParameter1(inst);
              insert_dumper_par(method, inst, 1, 0);
              insert_dumper_parameter(method, inst, par1, 0);
              
              par1	= IRMETHOD_getInstructionParameter2(inst);
              insert_dumper_par(method, inst, 2, 0);
              insert_dumper_parameter(method, inst, par1, 0);
              break;
            case 1:
              par1	= IRMETHOD_getInstructionParameter1(inst);
              insert_dumper_par(method, inst, 1, 0);
              insert_dumper_parameter(method, inst, par1, 0);
              break;
            case 0:
              break;
            default:
              break;
          }
          
          //if call instruction, print call parameters 
          JITUINT32 call_count;
          if(IRMETHOD_getInstructionCallParametersNumber(inst) > 0) {
            for (call_count = 0; call_count < IRMETHOD_getInstructionCallParametersNumber(inst); call_count++){
              par1	= IRMETHOD_getInstructionCallParameter(inst, call_count);
              insert_dumper_par(method, inst, 5+call_count, 0);	
              insert_dumper_parameter(method, inst, par1, 0);
            }
          }
          if ((IRMETHOD_getInstructionResult(inst)->type != NOPARAM) && (IRMETHOD_getInstructionResult(inst)->type != IRVOID)){
            inst_result	= IRMETHOD_getInstructionResult(inst);
            insert_dumper_parameter(method, inst, inst_result, 1);
            insert_dumper_par(method, inst, 4, 1);
          }
          //end of print result  
        }
	}
	//end of while loop to check each instruction in the method	
    
    xanList_destroyList(l);
    
    return;
}
//end of do job


static inline char * wholetrace_get_version (void){
	return VERSION;
}

static inline char * wholetrace_get_informations (void){
	return INFORMATIONS;
}

static inline char * wholetrace_get_author (void){
	return AUTHOR;
}

void insert_dumper_par (ir_method_t *method, ir_instruction_t *inst, JITUINT32 parnum, bool isAfter){
		
	ir_instruction_t 	*instPrint;
	ir_item_t			*ir_to_dump;
	
    if (isAfter){
      instPrint = IRMETHOD_newInstructionOfTypeAfter(method, inst, IRNATIVECALL);
    }
    else{
      instPrint = IRMETHOD_newInstructionOfTypeBefore(method, inst, IRNATIVECALL);
    }
    IRMETHOD_setInstructionParameter1(method, instPrint, IRVOID, 0, IRTYPE, IRTYPE, NULL);
    IRMETHOD_setInstructionParameter2(method, instPrint, (IR_ITEM_VALUE)(JITNUINT)"dumper_parnum", 0, IRNINT, IRNINT, NULL);
    IRMETHOD_setInstructionParameter3(method, instPrint, (IR_ITEM_VALUE)(JITNUINT)dumper_parnum, 0, IRNINT, IRNINT, NULL);
	
	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	ir_to_dump->value.v 	= parnum;
	ir_to_dump->type	= IRUINT32;
	ir_to_dump->internal_type = IRUINT32;
	IRMETHOD_addInstructionCallParameter(method, instPrint, ir_to_dump);
	
	return ;
}

void insert_dumper_int (ir_method_t *method, ir_instruction_t *inst, ir_item_t *par, JITUINT32 type, bool isAfter){
	
	ir_instruction_t 	*instPrint;
	ir_item_t			*ir_to_dump;
    
    if (isAfter){
      instPrint = IRMETHOD_newInstructionOfTypeAfter(method, inst, IRNATIVECALL);
    }
    else{
      instPrint = IRMETHOD_newInstructionOfTypeBefore(method, inst, IRNATIVECALL);
    }
	
	IRMETHOD_setInstructionParameter1(method, instPrint, IRVOID, 0, IRTYPE, IRTYPE, NULL);
	IRMETHOD_setInstructionParameter2(method, instPrint, (IR_ITEM_VALUE)(JITNUINT)"dumper_int", 0, IRNINT, IRNINT, NULL);
	IRMETHOD_setInstructionParameter3(method, instPrint, (IR_ITEM_VALUE)(JITNUINT)dumper_int, 0, IRNINT, IRNINT, NULL);
	
    ir_to_dump			= allocFunction(sizeof(ir_item_t));
	ir_to_dump->value.v 	= type;
	ir_to_dump->type	= IRNUINT;
	ir_to_dump->internal_type = IRNUINT;
	IRMETHOD_addInstructionCallParameter(method, instPrint, ir_to_dump);
	
	
	ir_to_dump					= allocFunction(sizeof(ir_item_t));
	ir_to_dump->value.v 			= IRDATA_getSizeOfType(par);
	ir_to_dump->type			= IRUINT32;
	ir_to_dump->internal_type	= IRUINT32;
	IRMETHOD_addInstructionCallParameter(method, instPrint, ir_to_dump);
	
	ir_to_dump					= allocFunction(sizeof(ir_item_t));
	memcpy(ir_to_dump, par, sizeof(ir_item_t));
	IRMETHOD_addInstructionCallParameter(method, instPrint, ir_to_dump);
	
	return ;
}

void insert_dumper_uint (ir_method_t *method, ir_instruction_t *inst, ir_item_t *par, JITUINT32 type, bool isAfter){

	ir_instruction_t 	*instPrint;
	ir_item_t			*ir_to_dump;
    
    if (isAfter){
      instPrint = IRMETHOD_newInstructionOfTypeAfter(method, inst, IRNATIVECALL);
    }
    else{
      instPrint = IRMETHOD_newInstructionOfTypeBefore(method, inst, IRNATIVECALL);
    }
	
	IRMETHOD_setInstructionParameter1(method, instPrint, IRVOID, 0, IRTYPE, IRTYPE, NULL);
	IRMETHOD_setInstructionParameter2(method, instPrint, (IR_ITEM_VALUE)(JITNUINT)"dumper_uint", 0, IRNINT, IRNINT, NULL);
	IRMETHOD_setInstructionParameter3(method, instPrint, (IR_ITEM_VALUE)(JITNUINT)dumper_uint, 0, IRNINT, IRNINT, NULL);
	
	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	ir_to_dump->value.v 	= type;
	ir_to_dump->type	= IRUINT32;
	ir_to_dump->internal_type = IRUINT32;
	IRMETHOD_addInstructionCallParameter(method, instPrint, ir_to_dump);
	
	ir_to_dump					= allocFunction(sizeof(ir_item_t));
	ir_to_dump->value.v 			= IRDATA_getSizeOfType(par);
	ir_to_dump->type			= IRUINT32;
	ir_to_dump->internal_type	= IRUINT32;
	IRMETHOD_addInstructionCallParameter(method, instPrint, ir_to_dump);
	
	ir_to_dump					= allocFunction(sizeof(ir_item_t));
	memcpy(ir_to_dump, par, sizeof(ir_item_t));
	IRMETHOD_addInstructionCallParameter(method, instPrint, ir_to_dump);
	return ;
}

void insert_dumper_label (ir_method_t *method, ir_instruction_t *inst, ir_item_t *par, JITUINT32 type, bool isAfter){

	ir_instruction_t 	*instPrint;
	ir_item_t			*ir_to_dump;
    
    if (isAfter){
      instPrint = IRMETHOD_newInstructionOfTypeAfter(method, inst, IRNATIVECALL);
    }
    else{
      instPrint = IRMETHOD_newInstructionOfTypeBefore(method, inst, IRNATIVECALL);
    }
	
	IRMETHOD_setInstructionParameter1(method, instPrint, IRVOID, 0, IRTYPE, IRTYPE, NULL);
	IRMETHOD_setInstructionParameter2(method, instPrint, (IR_ITEM_VALUE)(JITNUINT)"dumper_label", 0, IRNINT, IRNINT, NULL);
	IRMETHOD_setInstructionParameter3(method, instPrint, (IR_ITEM_VALUE)(JITNUINT)dumper_label, 0, IRNINT, IRNINT, NULL);
	
	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	ir_to_dump->value.v 	= type;
	ir_to_dump->type	= IRUINT32;
	ir_to_dump->internal_type = IRUINT32;
	IRMETHOD_addInstructionCallParameter(method, instPrint, ir_to_dump);
	
	ir_to_dump					= allocFunction(sizeof(ir_item_t));
	ir_to_dump->value.v 			= IRDATA_getSizeOfType(par);
	ir_to_dump->type			= IRUINT32;
	ir_to_dump->internal_type	= IRUINT32;
	IRMETHOD_addInstructionCallParameter(method, instPrint, ir_to_dump);
	
	ir_to_dump					= allocFunction(sizeof(ir_item_t));
	ir_to_dump->value.v 			= par->value.v;
	ir_to_dump->type			= IRUINT32;
	ir_to_dump->internal_type	= IRUINT32;
	IRMETHOD_addInstructionCallParameter(method, instPrint, ir_to_dump);
	return ;
}

void insert_dumper_float (ir_method_t *method, ir_instruction_t *inst, ir_item_t *par, JITUINT32 type, bool isAfter){

	ir_instruction_t 	*instPrint;
	ir_item_t			*ir_to_dump;
	
    if (isAfter){
      instPrint = IRMETHOD_newInstructionOfTypeAfter(method, inst, IRNATIVECALL);
    }
    else{
      instPrint = IRMETHOD_newInstructionOfTypeBefore(method, inst, IRNATIVECALL);
    }
	
	IRMETHOD_setInstructionParameter1(method, instPrint, IRVOID, 0, IRTYPE, IRTYPE, NULL);
	IRMETHOD_setInstructionParameter2(method, instPrint, (IR_ITEM_VALUE)(JITNUINT)"dumper_float", 0, IRNINT, IRNINT, NULL);
	IRMETHOD_setInstructionParameter3(method, instPrint, (IR_ITEM_VALUE)(JITNUINT)dumper_float, 0, IRNINT, IRNINT, NULL);
	
	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	ir_to_dump->value.v 	= type;
	ir_to_dump->type	= IRUINT32;
	ir_to_dump->internal_type = IRUINT32;
	IRMETHOD_addInstructionCallParameter(method, instPrint, ir_to_dump);
	
	ir_to_dump					= allocFunction(sizeof(ir_item_t));
	ir_to_dump->value.v 			= IRDATA_getSizeOfType(par);
	ir_to_dump->type			= IRUINT32;
	ir_to_dump->internal_type	= IRUINT32;
	IRMETHOD_addInstructionCallParameter(method, instPrint, ir_to_dump);
	
	ir_to_dump					= allocFunction(sizeof(ir_item_t));
	memcpy(ir_to_dump, par, sizeof(ir_item_t));
	IRMETHOD_addInstructionCallParameter(method, instPrint, ir_to_dump);
	return ;
}

void insert_dumper_valuetype (ir_method_t *method, ir_instruction_t *inst, ir_item_t *par, JITUINT32 type, bool isAfter){

	ir_instruction_t 	*instPrint;
	ir_item_t			*ir_to_dump;
    
    if (isAfter){
      instPrint = IRMETHOD_newInstructionOfTypeAfter(method, inst, IRNATIVECALL);
    }
    else{
      instPrint = IRMETHOD_newInstructionOfTypeBefore(method, inst, IRNATIVECALL);
    }
	
	IRMETHOD_setInstructionParameter1(method, instPrint, IRVOID, 0, IRTYPE, IRTYPE, NULL);
	IRMETHOD_setInstructionParameter2(method, instPrint, (IR_ITEM_VALUE)(JITNUINT)"dumper_valuetype", 0, IRNINT, IRNINT, NULL);
	IRMETHOD_setInstructionParameter3(method, instPrint, (IR_ITEM_VALUE)(JITNUINT)dumper_valuetype, 0, IRNINT, IRNINT, NULL);
	
    ir_to_dump			= allocFunction(sizeof(ir_item_t));
	ir_to_dump->value.v 	= type;
	ir_to_dump->type	= IRUINT32;
	ir_to_dump->internal_type = IRUINT32;
	IRMETHOD_addInstructionCallParameter(method, instPrint, ir_to_dump);
	return ;
}

//For registers
void insert_dumper_var (ir_method_t *method, ir_instruction_t *inst, ir_item_t *par, bool isAfter){
	
	ir_instruction_t 	*instPrint;
	ir_item_t			*ir_to_dump;

    if (isAfter){
      instPrint = IRMETHOD_newInstructionOfTypeAfter(method, inst, IRNATIVECALL);
    }
    else{
      instPrint = IRMETHOD_newInstructionOfTypeBefore(method, inst, IRNATIVECALL);
    }
	
	IRMETHOD_setInstructionParameter1(method, instPrint, IRVOID, 0, IRTYPE, IRTYPE, NULL);
	IRMETHOD_setInstructionParameter2(method, instPrint, (IR_ITEM_VALUE)(JITNUINT)"dumper_var", 0, IRNINT, IRNINT, NULL);
	IRMETHOD_setInstructionParameter3(method, instPrint, (IR_ITEM_VALUE)(JITNUINT)dumper_var, 0, IRNINT, IRNINT, NULL);
	
	ir_to_dump					= allocFunction(sizeof(ir_item_t));
	ir_to_dump->value.v 			= par->value.v;
	ir_to_dump->type			= IRUINT32;
	ir_to_dump->internal_type	= IRUINT32;
	IRMETHOD_addInstructionCallParameter(method, instPrint, ir_to_dump);
	
	return ;
}

/*=========================two high level functions of dumping parameters and result =================*/
void insert_dumper_parameter(ir_method_t *method, ir_instruction_t *inst, ir_item_t *par1, bool isAfter){
	
  //ir_item_t				*par1;
  ir_item_t 				localvar;
  //switch parameter type
  switch (par1->type){
					
    case IROFFSET:
      if (!isAfter){
        insert_dumper_var(method, inst, par1, isAfter); 
      }
      switch(par1->internal_type){
        case IRINT8:
        case IRINT16:
        case IRINT32:
        case IRINT64:
        case IRNINT:
          insert_dumper_int(method, inst, par1, par1->internal_type, isAfter);
          break;
        case IRUINT8:
        case IRUINT16:
        case IRUINT32:
        case IRUINT64:
        case IRNUINT:
          insert_dumper_uint(method, inst, par1, par1->internal_type, isAfter);
          break;

        case IRMPOINTER:
        case IRFPOINTER:
        case IROBJECT:
        case IRMETHODENTRYPOINT:
        case IRSIGNATURE:
        case IRCLASSID:

          insert_dumper_label(method, inst, par1, par1->internal_type, isAfter);
          break;
        case IRFLOAT32:
        case IRFLOAT64:
        case IRNFLOAT:
          insert_dumper_float(method, inst, par1, par1->internal_type, isAfter);
          break;

        case IRVALUETYPE:
          insert_dumper_valuetype(method, inst, par1, par1->internal_type, isAfter);
          break;

        case IRVOID:
          break;
        default:
            fprintf(stderr, "unknown iroffset internal type%u\n", par1->internal_type);
            exit(0);
          }
      if (isAfter){
        insert_dumper_var(method, inst, par1, isAfter); 
      }
      break;
                    
    case IRINT8:
    case IRINT16:
    case IRINT32:
    case IRINT64:
    case IRNINT:
      insert_dumper_int(method, inst, par1, par1->type, isAfter);
      break;
    case IRTYPE:
    case IRLABELITEM:
    case IRCLASSID:
    case IRMETHODID:
    case IRMETHODENTRYPOINT:
    case IRMPOINTER:
    case IRFPOINTER:
    case IROBJECT:
    case IRSIGNATURE:
    case IRSTRING:
      insert_dumper_label(method, inst, par1, par1->type, isAfter);
      break;
    
    case IRUINT8:
    case IRUINT16:
    case IRUINT32:
    case IRUINT64:
    case IRNUINT:
      insert_dumper_uint(method, inst, par1, par1->type, isAfter);
      break;
    case IRFLOAT32:
    case IRFLOAT64:
    case IRNFLOAT:
      insert_dumper_float(method, inst, par1, par1->type, isAfter);
      break;
    case IRVOID:
    case NOPARAM:
        break;
    case IRVALUETYPE:
      insert_dumper_valuetype(method, inst, par1, par1->type, isAfter);
      break;
    case IRSYMBOL:
      IRSYMBOL_resolveSymbolFromIRItem (par1, &localvar);
      insert_dumper_label(method, inst, &localvar, par1->type, isAfter);
      break;
      break;
    default:
      fprintf(stderr, "par type missing: %d\n", par1->type);
      exit(0);
  }
}

/*Print Functions to Dump Static Trace*/

void print_instruction(JITNUINT methodid, JITUINT32 instid, JITUINT32 insttype, JITINT32 bblockID, JITINT32 loopID){
  gzprintf( staticfp, "\n0,%u,%d,%u,%d,%d",  methodid, instid, insttype, bblockID, loopID);
  static_instruction_counter++;
}

void print_parameter(unsigned int parid, ir_item_t *par){
  gzprintf(staticfp, "\n%u,", parid);
  unsigned int varid, size;
  ir_item_t 				localvar;
  switch (par->type){
    case IROFFSET:
      varid = par->value.v;
      gzprintf(staticfp, "%u,%u,", IROFFSET, varid);
        switch(par->internal_type){
            case IRINT8:
            case IRINT16:
            case IRINT32:
            case IRINT64:
            case IRNINT:
            case IRUINT8:
            case IRUINT16:
            case IRUINT32:
            case IRUINT64:
            case IRNUINT:
            case IRFLOAT32:
            case IRFLOAT64:
            case IRNFLOAT:
            case IRMPOINTER:
            case IRFPOINTER:
            case IROBJECT:
            case IRMETHODENTRYPOINT:
            case IRSIGNATURE:
            case IRCLASSID:
              size = IRDATA_getSizeOfType(par);
              print_data(par->internal_type, size);
              break;
            
            case IRVALUETYPE:
              print_data(par->internal_type, 0);
              break;
            
            case IRVOID:
                break;
            default:
                fprintf(stderr, "unknown iroffset internal type%u\n", par->internal_type);
                exit(0);
        }
        break;
    
    case IRINT8:
    case IRINT16:
    case IRINT32:
    case IRINT64:
    case IRNINT:
    case IRUINT8:
    case IRUINT16:
    case IRUINT32:
    case IRUINT64:
    case IRNUINT:
    case IRFLOAT32:
    case IRFLOAT64:
    case IRNFLOAT:
    case IRMETHODENTRYPOINT:
    case IRMPOINTER:
    case IRFPOINTER:
    case IROBJECT:
    case IRSIGNATURE:
    case IRCLASSID:
    case IRMETHODID:
    case IRSTRING:
    case IRTYPE:
    
      size = IRDATA_getSizeOfType(par);
      print_data(par->type, size);
      break;
    
    case IRLABELITEM:
      size = IRDATA_getSizeOfType(par);
      print_label(par->type, size, par->value.v);
      break;
    
    case IRVOID:
    case NOPARAM:
        break;
    
    case IRVALUETYPE:
      print_data(par->type, 0);
      break;
    
    case IRSYMBOL:
      IRSYMBOL_resolveSymbolFromIRItem (par, &localvar);
      size = IRDATA_getSizeOfType(&localvar);
      print_data(IRSTRING, size);
      break;
    
    default:
        fprintf(stderr, "par type missing: %d\n", par->type);
        exit(0);
  }
}

void print_data( unsigned int type, unsigned int size){
  gzprintf(staticfp, "%u,%u,", type, size);
}

void print_label( unsigned int type, unsigned int size, unsigned int value){
  gzprintf(staticfp, "%u,%u,%u,", type, size, value);
}

JITUINT32 findBasicBlockID (ir_method_t *method, JITUINT32 instID){
	JITUINT32 count;
	JITUINT32 basicBlocksNumber;

	/* Assertions		*/
	assert(method != NULL);
	assert(instID <= IRMETHOD_getInstructionsNumber(method));

	basicBlocksNumber = IRMETHOD_getNumberOfMethodBasicBlocks(method);
	for (count = 0; count < basicBlocksNumber; count++) {
		IRBasicBlock      *basicBlock;
		basicBlock = IRMETHOD_getBasicBlockAtPosition(method, count);
		assert(basicBlock != NULL);
		if (    (instID >= basicBlock->startInst)       &&
			(instID <= basicBlock->endInst)         ) {
			return count;
		}
	}

	abort();
	return 0;
}

