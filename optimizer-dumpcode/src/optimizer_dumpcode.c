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
#include <optimizer_dumpcode.h>
#include <config.h>
#include <zlib.h>
// End

#define INFORMATIONS 	"This is a dumpcode plugin"
#define	AUTHOR		"Yakun Sophia Shao"


static inline JITUINT64 dumpcode_get_ID_job (void);
static inline char * dumpcode_get_version (void);
static inline void dumpcode_do_job (ir_method_t * method);
static inline char * dumpcode_get_informations (void);
static inline char * dumpcode_get_author (void);
static inline JITUINT64 dumpcode_get_dependences (void);
static inline void dumpcode_shutdown (void);
static inline void dumpcode_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix);
static inline JITUINT64 dumpcode_get_invalidations (void);

/*==============function used to statically dump the code ====================*/
void print_instruction(ir_method_t *method, t_ir_instruction *inst);
void print_result(ir_method_t *method, t_ir_instruction *inst);
void print_IR_type(JITUINT32 type);

void print_IR_Parameter_value(ir_method_t *method, t_ir_instruction *inst, JITUINT16 ID);
void print_IR_FirstParameter(ir_method_t *method, t_ir_instruction *inst);
void print_IR_SecondParameter(ir_method_t *method, t_ir_instruction *inst);
void print_IR_ThirdParameter(ir_method_t *method, t_ir_instruction *inst);

void print_parameters(ir_method_t *method, t_ir_instruction *inst);
void print_call_parameters(ir_method_t *method, t_ir_instruction *inst);
void print_IR_call_parameter_value(ir_method_t *method, t_ir_instruction *inst, JITUINT16 ID);

/*=============functions used to dynamically dump the code =================*/

void dumper_before(JITUINT32 value);
void dumper_after(void);
void dumper_print_inst(JITUINT32 value, JITUINT32 instid, char * signature, JITNUINT methodID);

void dumper_int8(JITINT8 value);
void dumper_int16(JITINT16 value);
void dumper_int32(JITINT32 value);
void dumper_int64(JITINT64 value);
void dumper_uint8(JITUINT8 value);
void dumper_uint16(JITUINT16 value);
void dumper_uint32(JITUINT32 value);
void dumper_uint64(JITUINT64 value);
void dumper_float32(JITFLOAT32 value);
void dumper_float64(JITFLOAT64 value);


void dumper_irtype_64(JITUINT32 value);
void dumper_irtype_32(JITUINT32 value);
void dumper_irtype_16(JITUINT32 value);
void dumper_irtype_8(JITUINT32 value);
void dumper_irtype_object(JITUINT32 value);
void dumper_irtype_valuetype(JITINT32 value);



void dumper_var_int8(IR_ITEM_VALUE varid, JITINT8 value);
void dumper_var_int16(IR_ITEM_VALUE varid, JITINT16 value);
void dumper_var_int32(IR_ITEM_VALUE varid, JITINT32 value);
void dumper_var_int64(IR_ITEM_VALUE varid, JITINT64 value);

void dumper_var_uint8(IR_ITEM_VALUE varid, JITUINT8 value);
void dumper_var_uint16(IR_ITEM_VALUE varid, JITUINT16 value);
void dumper_var_uint32(IR_ITEM_VALUE varid, JITUINT32 value);
void dumper_var_uint64(IR_ITEM_VALUE varid, JITUINT64 value);

void dumper_var_float32(IR_ITEM_VALUE varid, JITFLOAT32 value);
void dumper_var_float64(IR_ITEM_VALUE varid, JITFLOAT64 value);
void dumper_var_pointer(IR_ITEM_VALUE varid, JITNUINT value);
void dumper_var_valuetype(IR_ITEM_VALUE varid, JITINT32 value);

void dumper_label(JITUINT64 value);
void dumper_classid(int value);
void dumper_methodid(int value);
void dumper_pointer(JITNUINT value);
/*=================dump code functions end================================*/

t_ir_instruction * insert_dumper_par (ir_method_t *method, t_ir_instruction *inst, JITUINT32 parnum);

t_ir_instruction * insert_dumper_var_int8 (ir_method_t *method, t_ir_instruction *beforeInst, ir_item_t *par);
t_ir_instruction * insert_dumper_var_int16 (ir_method_t *method, t_ir_instruction *beforeInst, ir_item_t *par);
t_ir_instruction * insert_dumper_var_int32 (ir_method_t *method, t_ir_instruction *beforeInst, ir_item_t *par);
t_ir_instruction * insert_dumper_var_int64 (ir_method_t *method, t_ir_instruction *beforeInst, ir_item_t *par);
t_ir_instruction * insert_dumper_var_uint8 (ir_method_t *method, t_ir_instruction *beforeInst, ir_item_t *par);
t_ir_instruction * insert_dumper_var_uint16 (ir_method_t *method, t_ir_instruction *beforeInst, ir_item_t *par);
t_ir_instruction * insert_dumper_var_uint32 (ir_method_t *method, t_ir_instruction *beforeInst, ir_item_t *par);
t_ir_instruction * insert_dumper_var_uint64 (ir_method_t *method, t_ir_instruction *beforeInst, ir_item_t *par);
t_ir_instruction * insert_dumper_var_float32 (ir_method_t *method, t_ir_instruction *beforeInst, ir_item_t *par);
t_ir_instruction * insert_dumper_var_float64 (ir_method_t *method, t_ir_instruction *beforeInst, ir_item_t *par);
t_ir_instruction * insert_dumper_var_pointer (ir_method_t *method, t_ir_instruction *beforeInst, ir_item_t *par);
t_ir_instruction * insert_dumper_var_valuetype (ir_method_t *method, t_ir_instruction *beforeInst, ir_item_t *par);


t_ir_instruction * insert_dumper_int8 (ir_method_t *method, t_ir_instruction *inst, ir_item_t *par);
t_ir_instruction * insert_dumper_int16 (ir_method_t *method, t_ir_instruction *inst, ir_item_t *par);
t_ir_instruction * insert_dumper_int32 (ir_method_t *method, t_ir_instruction *inst, ir_item_t *par);
t_ir_instruction * insert_dumper_int64 (ir_method_t *method, t_ir_instruction *inst, ir_item_t *par);
t_ir_instruction * insert_dumper_uint8 (ir_method_t *method, t_ir_instruction *inst, ir_item_t *par);
t_ir_instruction * insert_dumper_uint16 (ir_method_t *method, t_ir_instruction *inst, ir_item_t *par);
t_ir_instruction * insert_dumper_uint32 (ir_method_t *method, t_ir_instruction *inst, ir_item_t *par);
t_ir_instruction * insert_dumper_uint64 (ir_method_t *method, t_ir_instruction *inst, ir_item_t *par);
t_ir_instruction * insert_dumper_float32 (ir_method_t *method, t_ir_instruction *inst, ir_item_t *par);
t_ir_instruction * insert_dumper_float64 (ir_method_t *method, t_ir_instruction *inst, ir_item_t *par);

t_ir_instruction * insert_dumper_var_result_int8 (ir_method_t *method, t_ir_instruction *beforeInst, ir_item_t *par);
t_ir_instruction * insert_dumper_var_result_int16 (ir_method_t *method, t_ir_instruction *beforeInst, ir_item_t *par);
t_ir_instruction * insert_dumper_var_result_int32 (ir_method_t *method, t_ir_instruction *beforeInst, ir_item_t *par);
t_ir_instruction * insert_dumper_var_result_int64 (ir_method_t *method, t_ir_instruction *beforeInst, ir_item_t *par);
t_ir_instruction * insert_dumper_var_result_uint8 (ir_method_t *method, t_ir_instruction *beforeInst, ir_item_t *par);
t_ir_instruction * insert_dumper_var_result_uint16 (ir_method_t *method, t_ir_instruction *beforeInst, ir_item_t *par);
t_ir_instruction * insert_dumper_var_result_uint32 (ir_method_t *method, t_ir_instruction *beforeInst, ir_item_t *par);
t_ir_instruction * insert_dumper_var_result_uint64 (ir_method_t *method, t_ir_instruction *beforeInst, ir_item_t *par);
t_ir_instruction * insert_dumper_var_result_float32 (ir_method_t *method, t_ir_instruction *beforeInst, ir_item_t *par);
t_ir_instruction * insert_dumper_var_result_float64 (ir_method_t *method, t_ir_instruction *beforeInst, ir_item_t *par);
t_ir_instruction * insert_dumper_var_result_pointer (ir_method_t *method, t_ir_instruction *beforeInst, ir_item_t *par);
t_ir_instruction * insert_dumper_var_result_valuetype (ir_method_t *method, t_ir_instruction *beforeInst, ir_item_t *par);


t_ir_instruction * insert_dumper_result_int8 (ir_method_t *method, t_ir_instruction *inst, ir_item_t *par);
t_ir_instruction * insert_dumper_result_int16 (ir_method_t *method, t_ir_instruction *inst, ir_item_t *par);
t_ir_instruction * insert_dumper_result_int32 (ir_method_t *method, t_ir_instruction *inst, ir_item_t *par);
t_ir_instruction * insert_dumper_result_int64 (ir_method_t *method, t_ir_instruction *inst, ir_item_t *par);
t_ir_instruction * insert_dumper_result_uint8 (ir_method_t *method, t_ir_instruction *inst, ir_item_t *par);
t_ir_instruction * insert_dumper_result_uint16 (ir_method_t *method, t_ir_instruction *inst, ir_item_t *par);
t_ir_instruction * insert_dumper_result_uint32 (ir_method_t *method, t_ir_instruction *inst, ir_item_t *par);
t_ir_instruction * insert_dumper_result_uint64 (ir_method_t *method, t_ir_instruction *inst, ir_item_t *par);
t_ir_instruction * insert_dumper_result_float32 (ir_method_t *method, t_ir_instruction *inst, ir_item_t *par);
t_ir_instruction * insert_dumper_result_float64 (ir_method_t *method, t_ir_instruction *inst, ir_item_t *par);

t_ir_instruction * insert_dumper_label (ir_method_t *method, t_ir_instruction *inst, ir_item_t *par);
t_ir_instruction * insert_dumper_method (ir_method_t *method, t_ir_instruction *inst, ir_item_t *par);
t_ir_instruction * insert_dumper_class (ir_method_t *method, t_ir_instruction *inst, ir_item_t *par);
t_ir_instruction * insert_dumper_pointer (ir_method_t *method, t_ir_instruction *inst, ir_item_t *par);


t_ir_instruction * insert_dumper_result_label (ir_method_t *method, t_ir_instruction *inst, ir_item_t *par);
t_ir_instruction * insert_dumper_result_method (ir_method_t *method, t_ir_instruction *inst, ir_item_t *par);
t_ir_instruction * insert_dumper_result_class (ir_method_t *method, t_ir_instruction *inst, ir_item_t *par);
t_ir_instruction * insert_dumper_result_pointer (ir_method_t *method, t_ir_instruction *inst, ir_item_t *par);


t_ir_instruction * insert_dumper_irtype_8 (ir_method_t *method, t_ir_instruction *beforeInst);
t_ir_instruction * insert_dumper_irtype_16 (ir_method_t *method, t_ir_instruction *beforeInst);
t_ir_instruction * insert_dumper_irtype_32 (ir_method_t *method, t_ir_instruction *beforeInst);
t_ir_instruction * insert_dumper_irtype_64 (ir_method_t *method, t_ir_instruction *beforeInst);
t_ir_instruction * insert_dumper_irtype_object (ir_method_t *method, t_ir_instruction *beforeInst);
t_ir_instruction * insert_dumper_irtype_valuetype(ir_method_t *method, t_ir_instruction *beforeInst, ir_item_t *par);

t_ir_instruction * insert_dumper_result_irtype_8 (ir_method_t *method, t_ir_instruction *beforeInst);
t_ir_instruction * insert_dumper_result_irtype_16 (ir_method_t *method, t_ir_instruction *beforeInst);
t_ir_instruction * insert_dumper_result_irtype_32 (ir_method_t *method, t_ir_instruction *beforeInst);
t_ir_instruction * insert_dumper_result_irtype_64 (ir_method_t *method, t_ir_instruction *beforeInst);
t_ir_instruction * insert_dumper_result_irtype_object (ir_method_t *method, t_ir_instruction *beforeInst);
t_ir_instruction * insert_dumper_result_irtype_valuetype (ir_method_t *method, t_ir_instruction *beforeInst, ir_item_t *par);



/*=================dump parameters functions================================*/
void insert_dumper_parameter_1(ir_method_t *method, t_ir_instruction *inst);
void insert_dumper_parameter_2(ir_method_t *method, t_ir_instruction *inst);
void insert_dumper_parameter_3(ir_method_t *method, t_ir_instruction *inst);
void insert_dumper_result(ir_method_t *method, t_ir_instruction *inst);
void insert_dumper_call_parameter(ir_method_t *method, t_ir_instruction *inst, JITUINT32 call_count);

ir_lib_t	*irLib		= NULL;
ir_optimizer_t	*irOptimizer	= NULL;
char 	        *prefix		= NULL;

gzFile 	fp;
ir_optimization_interface_t plugin_interface = {
	dumpcode_get_ID_job	, 
	dumpcode_get_dependences	,
	dumpcode_init		,
	dumpcode_shutdown		,
	dumpcode_do_job		, 
	dumpcode_get_version	,
	dumpcode_get_informations	,
	dumpcode_get_author,
	dumpcode_get_invalidations
};


static inline void dumpcode_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix){
	char	buf[1024];

	/* Assertions			*/
	assert(lib != NULL);
	assert(optimizer != NULL);
	assert(outputPrefix != NULL);

	irLib		= lib;
	irOptimizer	= optimizer;
	prefix		= outputPrefix;

	snprintf(buf, 1024, "%s_dump.gz", lib->getProgramName());
	fp=gzopen(buf, "w");

	/* Return			*/
	return ;
}

static inline void dumpcode_shutdown (void){
        irLib		= NULL;
	irOptimizer	= NULL;
	prefix		= NULL;
	gzclose(fp);
}

static inline JITUINT64 dumpcode_get_ID_job (void){
	return DUMPCODE;
}


static inline JITUINT64 dumpcode_get_dependences (void){
	return 0;
}

/*====================dynamically dump code functions ====================*/

void dumper_before(JITUINT32 value){
	gzprintf( fp,"\n%d\t", value);
}

void dumper_print_inst(JITUINT32 value, JITUINT32 instid, char * signature, JITNUINT methodID){
	gzprintf( fp, "\n\n0\t%s\t%u\t%d\t", signature, methodID, instid);
	switch(value){
		case IRADD:
			gzprintf( fp,  "add ");
			break;
		case IRADDOVF:
			gzprintf( fp,  "add_ovf ");
			break;
		case IRAND:
			gzprintf( fp,  "and ");
			break;
		case IRALLOCA:
			gzprintf( fp,  "alloca ");
			break;
		case IRBRANCH:
			gzprintf( fp,  "branch ");
			break;
		case IRBRANCHIF:
			gzprintf( fp,  "branch_if ");
			break;
		case IRBRANCHIFPCNOTINRANGE:
			gzprintf( fp,  "branch_if_pc_not_in_range ");
			break;
		case IRBRANCHIFNOT:
			gzprintf( fp,  "branch_if_not ");
			break;
		case IRCALL:
			gzprintf( fp,  "call ");
			break;
		case IRCALLFILTER:
			gzprintf( fp,  "call_filter ");
			break;
		case IRCALLFINALLY:
			gzprintf( fp,  "call_finally ");
			break;
		case IRCHECKNULL:
			gzprintf( fp,  "check_null ");
			break;
		case IRCONV:
			gzprintf( fp,  "conv ");
			break;
		case IRDIV:
			gzprintf( fp,  "div ");
			break;
		case IRENDFINALLY:
			gzprintf( fp,  "end_finally ");
			break;
		case IRENDFILTER:
			gzprintf( fp,  "end_filter ");
			break;
		case IREQ:
			gzprintf( fp,  "eq ");
			break;
		case IRGETADDRESS:
			gzprintf( fp,  "get_address ");
			break;
		case IRGT:
			gzprintf( fp,  "gt ");
			break;
		case IRICALL:
			gzprintf( fp,  "internal_call ");
			break;
		case IRINITMEMORY:
			gzprintf( fp,  "init_memory ");
			break;
		case IRMEMCPY:
			gzprintf( fp,  "memcpy ");
			break;
		case IRISNAN:
			gzprintf( fp,  "is_not_a_number ");
			break;
		case IRISINF:
			gzprintf( fp,  "is_infinite ");
			break;
		case IRLABEL:
			gzprintf( fp,  "label ");
			break;
		case IRLOADELEM:
			gzprintf( fp,  "load_elem ");
			break;
		case IRLOADREL:
			gzprintf( fp,  "load_rel ");
			break;
		case IRLT:
			gzprintf( fp,  "lt ");
			break;
		case IRMUL:
			gzprintf( fp,  "mul ");
			break;
		case IRMULOVF:
			gzprintf( fp,  "mul_ovf ");
			break;
		case IRNATIVECALL:
			gzprintf( fp,  "native_call ");
			break;
		case IRLIBRARYCALL:
			gzprintf( fp,  "lib_call ");
			break;
		case IRNEG:
			gzprintf( fp,  "neg ");
			break;
		case IRNEWOBJ:
			gzprintf( fp,  "new_object ");
			break;
		case IRNEWARR:
			gzprintf( fp,  "new_array ");
			break;
		case IRNOP:
			gzprintf( fp,  "nop ");
			break;
		case IRNOT:
			gzprintf( fp,  "not ");
			break;
		case IROR:
			gzprintf( fp,  "or ");
			break;
		case IRREM:
			gzprintf( fp,  "rem ");
			break;
		case IRRET:
			gzprintf( fp,  "ret ");
			break;
		case IRSHL:
			gzprintf( fp,  "shl ");
			break;
		case IRSHR:
			gzprintf( fp,  "shr ");
			break;
		case IRSTARTCATCHER:
			gzprintf( fp,  "start_catcher ");
			break;
		case IRSTARTFILTER:
			gzprintf( fp,  "start_filter ");
			break;
		case IRSTARTFINALLY:
			gzprintf( fp,  "start_finally ");
			break;
		case IRSTORE:
			gzprintf( fp,  "store ");
			break;
		case IRSTOREREL:
			gzprintf( fp,  "store_rel ");
			break;
		case IRSTOREELEM:
			gzprintf( fp,  "store_elem ");
			break;
		case IRSUB:
			gzprintf( fp,  "sub ");
			break;
		case IRSUBOVF:
			gzprintf( fp,  "sub_ovf ");
			break;
		case IRTHROW:
			gzprintf( fp,  "throw ");
			break;
		case IRVCALL:
			gzprintf( fp,  "virtual_call ");
			break;
		case IRXOR:
			gzprintf( fp,  "xor ");
			break;
		case IREXITNODE:
			gzprintf( fp,  "STOP ");
			break;
		default:
			gzprintf( fp, "Error!!!\n");
			//abort();
			break;
			}

		//gzprintf( fp,"\n");
}


void dumper_after(void){
	gzprintf( fp, "\n4\t");
}

void dumper_irtype_valuetype(JITINT32 value){
	gzprintf( fp,"irtype\t%d\t%d", value, value);
}

void dumper_irtype_object(JITUINT32 value){
	gzprintf( fp,"irtype\t%u\t%u", value, value);
}

void dumper_irtype_64(JITUINT32 value){
	gzprintf( fp,"irtype\t%u\t%u", value, value);
}

void dumper_irtype_32(JITUINT32 value){
	gzprintf( fp,"irtype\t%u\t%u", value, value);
}

void dumper_irtype_16(JITUINT32 value){
	gzprintf( fp,"irtype\t%u\t%u", value, value);
}

void dumper_irtype_8(JITUINT32 value){
	gzprintf( fp,"irtype\t%u\t%u", value, value);
}


void dumper_int8(JITINT8 value){
	gzprintf( fp, "int8\t8\t%d", value);	
}

void dumper_int16(JITINT16 value){
	gzprintf( fp, "int16\t16\t%d", value);	
}

void dumper_int32(JITINT32 value){
	gzprintf( fp, "int32\t32\t%d", value);	
}

void dumper_int64(JITINT64 value){
	gzprintf( fp, "int64\t64\t%lld", value);	
}

void dumper_uint8(JITUINT8 value){
	gzprintf( fp, "uint8\t8\t%u", value);	
}

void dumper_uint16(JITUINT16 value){
	gzprintf( fp, "uint16\t16\t%u", value);	
}

void dumper_uint32(JITUINT32 value){
	gzprintf( fp, "uint32\t32\t%u", value);	
}

void dumper_uint64(JITUINT64 value){
	gzprintf( fp, "uint64\t64\t%llu", value);	
}

void dumper_float32(JITFLOAT32 value){
	gzprintf( fp, "float32\t32\t%f", value);
}

void dumper_float64(JITFLOAT64 value){
	gzprintf( fp, "float64\t64\t%lf", value);

}

void dumper_var_int8(IR_ITEM_VALUE varid, JITINT8 value){
	gzprintf( fp, "var\t%llu\t8\t%d", varid, value);
}

void dumper_var_int16(IR_ITEM_VALUE varid, JITINT16 value){
	gzprintf( fp, "var\t%llu\t16\t%d", varid, value);
}

void dumper_var_int32(IR_ITEM_VALUE varid, JITINT32 value){
	gzprintf( fp, "var\t%llu\t32\t%d", varid, value);
}

void dumper_var_int64(IR_ITEM_VALUE varid, JITINT64 value){
	gzprintf( fp, "var\t%llu\t64\t%lld", varid, value);
}

void dumper_var_uint8(IR_ITEM_VALUE varid, JITUINT8 value){
	gzprintf( fp, "var\t%llu\t8\t%u", varid, value);
}

void dumper_var_uint16(IR_ITEM_VALUE varid, JITUINT16 value){
	gzprintf( fp, "var\t%llu\t16\t%u", varid, value);
}

void dumper_var_uint32(IR_ITEM_VALUE varid, JITUINT32 value){
	gzprintf( fp, "var\t%llu\t32\t%u", varid, value);
}

void dumper_var_uint64(IR_ITEM_VALUE varid, JITUINT64 value){
	gzprintf( fp, "var\t%llu\t64\t%llu", varid, value);
}

void dumper_var_float32(IR_ITEM_VALUE varid, JITFLOAT32 value){
	gzprintf( fp, "var\t%llu\t32\t%f", varid, value);
}

void dumper_var_float64(IR_ITEM_VALUE varid, JITFLOAT64 value){
	gzprintf( fp, "var\t%llu\t64\t%f", varid, value);
}

void dumper_var_pointer(IR_ITEM_VALUE varid, JITNUINT value){
	gzprintf( fp, "var\t%llu\t64\t%u", varid, value);
}

void dumper_var_valuetype(IR_ITEM_VALUE varid, JITINT32 value){
	gzprintf( fp, "var\t%llu\t64\t%d", varid, value);
}


void dumper_label(JITUINT64 value){
	gzprintf( fp, "label\t%llu", value);
}

void dumper_classid(int value){
	gzprintf( fp, "class\t%d", value);
}

void dumper_methodid(int value){
	gzprintf( fp, "method\t%d", value);
}

void dumper_pointer(JITNUINT value){
	gzprintf( fp, "pointer\t%u", value);
}
/*========================dump code do job ================================*/

static inline void dumpcode_do_job (ir_method_t * method){
	//gzprintf( fp,"hello!!!\n");
	//return;
	XanList		*l;
	XanListItem *item;
	JITUINT32	c;
	t_ir_instruction	*inst;
	

	typedef struct instr_whole{
		t_ir_instruction 	*inst;
		JITUINT32			instid;
	}t_instr_whole;

	t_instr_whole		*instwhole;

	/* Assertions     */
	assert(method != NULL);
	
	/* Put all the instructions in the list */
	l = xanListNew(allocFunction, freeFunction, NULL);
	for (c=0; c< IRMETHOD_getInstructionsNumber(method); c++){
		inst	= IRMETHOD_getInstructionAtPosition(method, c);
		instwhole = (t_instr_whole *)malloc(sizeof(t_instr_whole));
		instwhole->inst 		= inst;
		instwhole->instid		= IRMETHOD_getInstructionPosition( inst);
		//statically print the instruction
		//print_instruction(method, instwhole->inst);
		//gzprintf( fp, "%u \n", instwhole->instid);
		//end statically print the instructions
		l->append(l, instwhole);
		//l->insert(l, instwhole);
	}

	//insert native calls to dynamically print the instructions
	item	= l->first(l);
	while(item != NULL){
		//instruction itself
		t_ir_instruction 	*inst;
		
		//instruction to print the operation code
		//instruction to print "Par"
		//instructions to print two parameters
		//instruction to print "result"
		//instruction to print one result
		t_ir_instruction 	*instPrintInstType;
		t_ir_instruction	*instPrintBefore;
		t_ir_instruction  	*instBefore1;
		t_ir_instruction	*instPrintAfter;
		t_ir_instruction	*instAfter;
		
		
		//store instruction type, used to print the instruction operation, the value
		//store the instruction id;
		//store the method signature;
		//store the method id
		//temp use to give value to native call instruction parameters
		//temp use to transfer parameters to dumping functions
		JITUINT32			instType;
		JITUINT32 			instID;
		JITINT8 			*signature;
		JITNUINT			methodID;
		ir_item_t			*ir_item;
		ir_item_t			*ir_to_dump;

		ir_item_t			*par1;
		ir_item_t			*inst_result;

		//fetch the instruction from the xan list
		instwhole				= item->data;
		inst					= instwhole->inst;
		
		//print instruction operations
		instType	= IRMETHOD_getInstructionType(inst);
		//instID		= IRMETHOD_getInstructionPosition( inst);
		instID		= instwhole->instid;
		signature	= IRMETHOD_getSignatureInString(method);
		methodID 	= IRMETHOD_getMethodID (irLib, method);
		if (signature == NULL){
			signature	= IRMETHOD_getMethodName(method);
			if(signature == NULL){
				signature	= (JITINT8 *)"UNKNOWN_METHOD";
			}
		}

		for(c = 0; c <= STRLEN(signature); c++){
			if(signature[c] == ' ')
				signature[c] = '_';
		
		}
		instPrintInstType = IRMETHOD_newInstructionBefore(method, inst);
		IRMETHOD_setInstructionType(method, instPrintInstType, IRNATIVECALL);
		
		ir_item				= method->getInstrPar1(instPrintInstType);
		ir_item->value			= IRVOID;
		ir_item->type			= IRTYPE;
		ir_item->internal_type	= IRTYPE;

		ir_item					= method->getInstrPar2(instPrintInstType);
		ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_print_inst";
		ir_item->type			= IRNINT;
		ir_item->internal_type	= IRNINT;
			
		ir_item					= method->getInstrPar3(instPrintInstType);
		ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_print_inst;
		ir_item->type			= IRNINT;
		ir_item->internal_type	= IRNINT;
		
		ir_to_dump			= allocFunction(sizeof(ir_item_t));
		ir_to_dump->value  	= instType;
		ir_to_dump->type	= IRUINT32;
		ir_to_dump->internal_type = IRUINT32;
		instPrintInstType->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
		instPrintInstType->callParameters->append(instPrintInstType->callParameters, ir_to_dump);
		
		ir_to_dump			= allocFunction(sizeof(ir_item_t));
		ir_to_dump->value  	= instID;
		ir_to_dump->type	= IRUINT32;
		ir_to_dump->internal_type = IRUINT32;
		instPrintInstType->callParameters->append(instPrintInstType->callParameters, ir_to_dump);
		
		ir_to_dump			= allocFunction(sizeof(ir_item_t));
		ir_to_dump->value  	= (IR_ITEM_VALUE)(JITNUINT)signature;
		ir_to_dump->type	= IRSTRING;
		ir_to_dump->internal_type = IRSTRING;
		instPrintInstType->callParameters->append(instPrintInstType->callParameters, ir_to_dump);

		ir_to_dump			= allocFunction(sizeof(ir_item_t));
		ir_to_dump->value  	= (IR_ITEM_VALUE)(JITNUINT)methodID;
		ir_to_dump->type	= IRNUINT;
		ir_to_dump->internal_type = IRNUINT;
		instPrintInstType->callParameters->append(instPrintInstType->callParameters, ir_to_dump);
	   //END print instruction type

		//end print "Par"
		if (instType == IRSTORE){
			par1					= method->getInstrPar2(inst);
			instPrintBefore 		= insert_dumper_par(method, inst, 1);	
				switch (par1->type){
					
					case IRINT8:
						instBefore1		= insert_dumper_int8(method, inst, par1);
						break;
					case IRINT16:
						instBefore1		= insert_dumper_int16(method, inst, par1);
						break;
					case IRINT32:
		//			case IRNINT:
						instBefore1		= insert_dumper_int32(method, inst, par1);
						break;
					case IRINT64:
					case IRNINT:
						instBefore1		= insert_dumper_int64(method, inst, par1);
						break;

					case IRUINT8:
						instBefore1		= insert_dumper_uint8(method, inst, par1);
						break;
					case IRUINT16:
						instBefore1		= insert_dumper_uint16(method, inst, par1);
						break;

					case IRUINT32:
					//case IRNUINT:
						instBefore1		= insert_dumper_uint32(method, inst, par1);
						break;
					
					case IRUINT64:
					case IRNUINT:
						instBefore1		= insert_dumper_uint64(method, inst, par1);
						break;
				
					case IRFLOAT32:
					//case IRNFLOAT:
						instBefore1		= insert_dumper_float32(method, inst, par1);
						break;
					
					case IRFLOAT64:
					case IRNFLOAT:
						instBefore1		= insert_dumper_float64(method, inst, par1);
						break;
					
					case IRTYPE:
						switch (par1->value){
								
							case IRINT8:
							case IRUINT8:
								instBefore1		= insert_dumper_irtype_8(method, inst);
								break;
							case IRINT16:
							case IRUINT16:
								instBefore1		= insert_dumper_irtype_16(method, inst);
								break;
							case IRINT32:
				//			case IRNINT:
							case IRUINT32:
				//			case IRNUINT:
							case IRFLOAT32:
				//			case IRNFLOAT:
								instBefore1		= insert_dumper_irtype_32(method, inst);
								break;
							case IRINT64:
							case IRUINT64:
							case IRFLOAT64:
							case IRNINT:
							case IRNUINT:
							case IRNFLOAT:
				
								instBefore1		= insert_dumper_irtype_64(method, inst);
								break;

							case IROBJECT:
								instBefore1		= insert_dumper_irtype_object(method, inst);
								break;
							case IRVALUETYPE:
								instBefore1		= insert_dumper_irtype_valuetype(method, inst, par1);
								break;

							default: 
								break;
							}
						break;
					
					case IROFFSET:
						switch(par1->internal_type){
						
							case IRINT8:
								instBefore1	= insert_dumper_var_int8(method, inst, par1);
								break;	
							case IRINT16:
								instBefore1	= insert_dumper_var_int16(method, inst, par1);
								break;	
							case IRINT32:
							//case IRNINT:
								instBefore1	= insert_dumper_var_int32(method, inst, par1);
								break;	
							case IRINT64:
							case IRNINT:
								instBefore1	= insert_dumper_var_int64(method, inst, par1);
								break;	

							case IRUINT8:
								instBefore1	= insert_dumper_var_uint8(method, inst, par1);
								break;	
							case IRUINT16:
								instBefore1	= insert_dumper_var_uint16(method, inst, par1);
								break;	
							case IRUINT32:
							//case IRNUINT:
								instBefore1	= insert_dumper_var_uint32(method, inst, par1);
								break;	
							case IRUINT64:
							case IRNUINT:
								instBefore1	= insert_dumper_var_uint64(method, inst, par1);
								break;	
							case IRFLOAT32:
							//case IRNFLOAT:
								instBefore1	= insert_dumper_var_float32(method, inst, par1);
								break;	
						
							case IRFLOAT64:
							case IRNFLOAT:
								instBefore1	= insert_dumper_var_float64(method, inst, par1);
								break;

							case IRMPOINTER:
							case IRFPOINTER:
							case IROBJECT:
							case IRMETHODENTRYPOINT:
								instBefore1	= insert_dumper_var_pointer (method, inst, par1);
								break;
							case IRVALUETYPE:
								instBefore1 = insert_dumper_var_valuetype (method, inst, par1);
								break;
							default:
								break;
						}
					 default:
						break;
					}
				//print result var


			inst_result			= method->getInstrPar1(inst);
			//switch parameter type
			switch (inst_result->type){
				
				case IRINT8:
					instAfter		= insert_dumper_result_int8(method, inst, inst_result);
					break;
				case IRINT16:
					instAfter		= insert_dumper_result_int16(method, inst, inst_result);
					break;
				case IRINT32:
				//case IRNINT:
					instAfter		= insert_dumper_result_int32(method, inst, inst_result);
					break;
				case IRINT64:
				case IRNINT :
					instAfter		= insert_dumper_result_int64(method, inst, inst_result);
					break;

				case IRUINT8:
					instAfter		= insert_dumper_result_uint8(method, inst, inst_result);
					break;
				case IRUINT16:
					instAfter		= insert_dumper_result_uint16(method, inst, inst_result);
					break;

				case IRUINT32:
				//case IRNUINT:
					instAfter		= insert_dumper_result_uint32(method, inst, inst_result);
					break;
					
				case IRUINT64:
				case IRNUINT :
					instAfter		= insert_dumper_result_uint64(method, inst, inst_result);
					break;
				
				case IRFLOAT32:
				//case IRNFLOAT:
					instAfter		= insert_dumper_result_float32(method, inst, inst_result);
					break;
					
				case IRFLOAT64:
				case IRNFLOAT:
					instAfter		= insert_dumper_result_float64(method, inst, inst_result);
					break;
				
				
					case IRTYPE:
						switch (inst_result->value){
								
							case IRINT8:
							case IRUINT8:
								instAfter		= insert_dumper_result_irtype_8(method, inst);
								break;
							case IRINT16:
							case IRUINT16:
								instAfter		= insert_dumper_result_irtype_16(method, inst);
								break;
							case IRINT32:
				//			case IRNINT:
							case IRUINT32:
				//			case IRNUINT:
							case IRFLOAT32:
				//			case IRNFLOAT:
								instAfter		= insert_dumper_result_irtype_32(method, inst);
								break;
							case IRINT64:
							case IRUINT64:
							case IRFLOAT64:
							case IRNINT:
							case IRNUINT:
							case IRNFLOAT:
								instAfter		= insert_dumper_result_irtype_64(method, inst);
								break;

							case IROBJECT:
								instAfter		= insert_dumper_result_irtype_object(method, inst);
								break;
							case IRVALUETYPE:
								instAfter		= insert_dumper_result_irtype_valuetype(method, inst, inst_result);
								break;
							default: 
								break;
							}
						break;
					
					case IROFFSET:
						switch(inst_result->internal_type){
						
							case IRINT8:
								instAfter	= insert_dumper_var_result_int8(method, inst, inst_result);
								break;	
							case IRINT16:
								instAfter	= insert_dumper_var_result_int16(method, inst, inst_result);
								break;	
							case IRINT32:
							//case IRNINT:
								instAfter	= insert_dumper_var_result_int32(method, inst, inst_result);
								break;	
							case IRINT64:
							case IRNINT :
								instAfter	= insert_dumper_var_result_int64(method, inst, inst_result);
								break;	

							case IRUINT8:
								instAfter	= insert_dumper_var_result_uint8(method, inst, inst_result);
								break;	
							case IRUINT16:
								instAfter	= insert_dumper_var_result_uint16(method, inst, inst_result);
								break;	
							case IRUINT32:
							//case IRNUINT:
								instAfter	= insert_dumper_var_result_uint32(method, inst, inst_result);
								break;	
							case IRUINT64:
							case IRNUINT :
								instAfter	= insert_dumper_var_result_uint64(method, inst, inst_result);
								break;	

							case IRFLOAT32:
							//case IRNFLOAT:
								instAfter	= insert_dumper_var_result_float32(method, inst, inst_result);
								break;	
						
							case IRFLOAT64:
							case IRNFLOAT:
								instAfter	= insert_dumper_var_result_float64(method, inst, inst_result);
								break;	
							case IRMPOINTER:
							case IRFPOINTER:
							case IROBJECT:
							case IRMETHODENTRYPOINT:
								instAfter	= insert_dumper_var_result_pointer (method, inst, inst_result);
								break;
							case IRVALUETYPE:
								instAfter = insert_dumper_var_result_valuetype (method, inst, inst_result);
								break;
							default:
								break;
							}
						//end switch var internal type
						break;		
				default:
					break;
					}


					//print "result"
			assert(inst->type == IRSTORE);
			instPrintAfter = IRMETHOD_newInstructionAfter(method, inst);
			IRMETHOD_setInstructionType(method, instPrintAfter, IRNATIVECALL);
		
			ir_item				= method->getInstrPar1(instPrintAfter);
			ir_item->value			= IRVOID;
			ir_item->type			= IRTYPE;
			ir_item->internal_type	= IRTYPE;

			ir_item				= method->getInstrPar2(instPrintAfter);
			ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_after";
			ir_item->type			= IRNUINT;
			ir_item->internal_type		= IRNUINT;
		
			ir_item				= method->getInstrPar3(instPrintAfter);
			ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_after;
			ir_item->type			= IRNUINT; 
			ir_item->internal_type		= IRNUINT;
		}   
		
		else {
		//print parameters value and varID
			switch(IRMETHOD_getInstructionParametersNumber(inst)){
			
				case 3: 
					insert_dumper_parameter_1(method, inst);
					insert_dumper_parameter_2(method, inst);
					insert_dumper_parameter_3(method, inst);
					break;
				case 2:
					insert_dumper_parameter_1(method, inst);
					insert_dumper_parameter_2(method, inst);
					break;
				case 1:
					insert_dumper_parameter_1(method, inst);
					break;
				case 0:
					break;
				default:
					abort();
			
			}
			//if call instruction, print call parameters 
			JITUINT32 call_count;

			if(IRMETHOD_getInstructionCallParametersNumber(inst) > 0) {
				for (call_count = 0; call_count < IRMETHOD_getInstructionCallParametersNumber(inst); call_count++){
					insert_dumper_call_parameter(method, inst, call_count);
				}
			
			}
			
			insert_dumper_result(method, inst);
		}
		//end of print result  
		item = item->next;
	}
	//end of while loop to check each instruction in the method	
	return;
}
//end of do job


static inline char * dumpcode_get_version (void){
	return VERSION;
}

static inline char * dumpcode_get_informations (void){
	return INFORMATIONS;
}

static inline char * dumpcode_get_author (void){
	return AUTHOR;
}
/*=====================statically print the instruction trace ==============================*/
/*==========================================================================================*/
/*==========================================================================================*/
/*==========================================================================================*/
/*==========================================================================================*/

void print_instruction(ir_method_t *method, t_ir_instruction *inst){

	JITUINT32	instType;

	//Assertions
	assert(method!=NULL);
	assert(inst != NULL);
	
	
	//print format: ID) result = instruction(par1, par2, par3..);
	instType	= IRMETHOD_getInstructionType(inst);

	//print instruction ID
	gzprintf( fp, "%d)", IRMETHOD_getInstructionPosition( inst));

	//print result:
	print_result(method, inst);
	
	//print the instruction name
	switch(instType){
	
		case IRADD:
			gzprintf( fp,  "add(");
			break;
		case IRADDOVF:
			gzprintf( fp,  "add_ovf(");
			break;
		case IRAND:
			gzprintf( fp,  "and(");
			break;
		case IRALLOCA:
			gzprintf( fp,  "alloca(");
			break;
		case IRBRANCH:
			gzprintf( fp,  "branch(");
			break;
		case IRBRANCHIF:
			gzprintf( fp,  "branch_if(");
			break;
		case IRBRANCHIFPCNOTINRANGE:
			gzprintf( fp,  "branch_if_pc_not_in_range(");
			break;
		case IRBRANCHIFNOT:
			gzprintf( fp,  "branch_if_not(");
			break;
		case IRCALL:
			gzprintf( fp,  "call(");
			break;
		case IRCALLFILTER:
			gzprintf( fp,  "call_filter(");
			break;
		case IRCALLFINALLY:
			gzprintf( fp,  "call_finally(");
			break;
		case IRCHECKNULL:
			gzprintf( fp,  "check_null(");
			break;
		case IRCONV:
			gzprintf( fp,  "conv(");
			break;
		case IRDIV:
			gzprintf( fp,  "div(");
			break;
		case IRENDFINALLY:
			gzprintf( fp,  "end_finally(");
			break;
		case IRENDFILTER:
			gzprintf( fp,  "end_filter(");
			break;
		case IREQ:
			gzprintf( fp,  "eq(");
			break;
		case IRGETADDRESS:
			gzprintf( fp,  "get_address(");
			break;
		case IRGT:
			gzprintf( fp,  "gt(");
			break;
		case IRICALL:
			gzprintf( fp,  "internal_call(");
			break;
		case IRINITMEMORY:
			gzprintf( fp,  "init_memory(");
			break;
		case IRMEMCPY:
			gzprintf( fp,  "memcpy(");
			break;
		case IRISNAN:
			gzprintf( fp,  "is_not_a_number(");
			break;
		case IRISINF:
			gzprintf( fp,  "is_infinite(");
			break;
		case IRLABEL:
			gzprintf( fp,  "label(");
			break;
		case IRLOADELEM:
			gzprintf( fp,  "load_elem(");
			break;
		case IRLOADREL:
			gzprintf( fp,  "load_rel(");
			break;
		case IRLT:
			gzprintf( fp,  "lt(");
			break;
		case IRMUL:
			gzprintf( fp,  "mul(");
			break;
		case IRMULOVF:
			gzprintf( fp,  "mul_ovf(");
			break;
		case IRNATIVECALL:
			gzprintf( fp,  "native_call(");
			break;
		case IRLIBRARYCALL:
			gzprintf( fp,  "lib_call(");
			break;
		case IRNEG:
			gzprintf( fp,  "neg(");
			break;
		case IRNEWOBJ:
			gzprintf( fp,  "new_object(");
			break;
		case IRNEWARR:
			gzprintf( fp,  "new_array(");
			break;
		case IRNOP:
			gzprintf( fp,  "nop(");
			break;
		case IRNOT:
			gzprintf( fp,  "not(");
			break;
		case IROR:
			gzprintf( fp,  "or(");
			break;
		case IRREM:
			gzprintf( fp,  "rem(");
			break;
		case IRRET:
			gzprintf( fp,  "ret(");
			break;
		case IRSHL:
			gzprintf( fp,  "shl(");
			break;
		case IRSHR:
			gzprintf( fp,  "shr(");
			break;
		case IRSTARTCATCHER:
			gzprintf( fp,  "start_catcher(");
			break;
		case IRSTARTFILTER:
			gzprintf( fp,  "start_filter(");
			break;
		case IRSTARTFINALLY:
			gzprintf( fp,  "start_finally(");
			break;
		case IRSTORE:
			gzprintf( fp,  "store(");
			break;
		case IRSTOREREL:
			gzprintf( fp,  "store_rel(");
			break;
		case IRSTOREELEM:
			gzprintf( fp,  "store_elem(");
			break;
		case IRSUB:
			gzprintf( fp,  "sub(");
			break;
		case IRSUBOVF:
			gzprintf( fp,  "sub_ovf(");
			break;
		case IRTHROW:
			gzprintf( fp,  "throw(");
			break;
		case IRVCALL:
			gzprintf( fp,  "virtual_call(");
			break;
		case IRXOR:
			gzprintf( fp,  "xor(");
			break;
		case IREXITNODE:
			gzprintf( fp,  "STOP(");
			break;
		default:
			gzprintf( fp, "Error!!!\n");
			abort();
	
	}

	//print parameters
	print_parameters(method, inst);
	print_call_parameters(method, inst);
	gzprintf( fp, ")");

}

void print_call_parameters(ir_method_t *method, t_ir_instruction *inst){

	JITUINT32  count;

	//assertions
	assert(method != NULL);
	assert(inst != NULL);

	if(IRMETHOD_getInstructionCallParametersNumber(inst) > 0){
		gzprintf( fp, "\n Call parameters: ");
		for (count = 0; count < IRMETHOD_getInstructionCallParametersNumber(inst); count++){
			print_IR_type(method->getCallParameterType(inst, count));
			print_IR_call_parameter_value(method, inst, count);

			if(method->getCallParameterInternalType(inst, count) != method->getCallParameterType(inst, count)){
				gzprintf( fp, "[ ");
				print_IR_type(method->getCallParameterInternalType(inst, count));
				gzprintf( fp, "], ");
			
			}
		
		}
	
	}
	
}

void print_IR_call_parameter_value(ir_method_t *method, t_ir_instruction *inst, JITUINT16 ID){

	JITUINT32	type;

	/* Assertions		*/
	assert(inst != NULL);
	assert(method != NULL);

	type	= method->getCallParameterType(inst, ID);
	switch(type){
		case IRINT8:
		case IRINT16:
		case IRINT32:
		//case IRNINT:
			gzprintf( fp, "%d ", (JITINT32) method->getCallParameterValue(inst, ID));
			break;
		case IRINT64:
		case IRNINT :
			gzprintf( fp, "%lld ", method->getCallParameterValue(inst, ID));
			break;
		case IRUINT8:
		case IRUINT16:
		case IRUINT32:
		//case IRNUINT:
			gzprintf( fp, "%u ", (JITUINT32)method->getCallParameterValue(inst, ID));
			break;
		case IRUINT64:
		case IRNUINT :
		case IROFFSET:
		case IRLABELITEM:
			gzprintf( fp, "%llu ", method->getCallParameterValue(inst, ID));
			break;
		case IRFLOAT32:
		case IRFLOAT64:
		case IRNFLOAT:
			gzprintf( fp, "%f ", method->getCallParameterFvalue(inst, ID));
			break;
		case IRMPOINTER:
		case IRTPOINTER:
		case IROBJECT:
		case IRMETHODENTRYPOINT:
			gzprintf( fp, "%p ", (void *)(JITNUINT)method->getCallParameterValue(inst, ID));
			break;
		case IRMETHODID:
			break;
		case IRCLASSID:
			break;
		case IRTYPE:
			print_IR_type(method->getCallParameterValue(inst, ID));
			break;
		case IRVOID:
			break;
		default:
	//		abort();
			break;
		}

}

void print_parameters(ir_method_t *method, t_ir_instruction *inst){
	
	//assertions
	assert(method!=NULL);
	assert(inst !=NULL);

	switch(IRMETHOD_getInstructionParametersNumber(inst)){

		case 3:	
			print_IR_FirstParameter(method, inst);
			print_IR_SecondParameter(method, inst);
			print_IR_ThirdParameter(method, inst);
			break;
		case 2:
			print_IR_FirstParameter(method, inst);
			print_IR_SecondParameter(method, inst);
			break;
		case 1:
			print_IR_FirstParameter(method, inst);
			break;
		case 0:
			break;
		default:
			abort();
	}
}

void print_IR_Parameter_value(ir_method_t *method, t_ir_instruction *inst, JITUINT16 ID){

	JITUINT32	type;
	IR_ITEM_VALUE	value;
	IR_ITEM_FVALUE	fvalue;

	/* Assertions		*/
	assert(inst != NULL);
	assert(method != NULL);

	switch(ID){
		case 3:
			type	= method->getInstrPar3Type(inst);
			value	= method->getInstrPar3Value(inst);
			fvalue	= method->getInstrPar3Fvalue(inst);
			break;
		case 2:
			type	= method->getInstrPar2Type(inst);
			value	= method->getInstrPar2Value(inst);
			fvalue	= method->getInstrPar2Fvalue(inst);
			break;
		case 1:
			type	= method->getInstrPar1Type(inst);
			value	= method->getInstrPar1Value(inst);
			fvalue	= method->getInstrPar1Fvalue(inst);
			break;
		default:
			abort();
	}
	switch(type){
		case IRINT8:
		case IRINT16:
		case IRINT32:
		//case IRNINT:
			gzprintf( fp, "%d ", (JITINT32)value);
			break;
		case IRINT64:
		case IRNINT :
			gzprintf( fp, "%lld ", value);
			break;
		case IRUINT8:
		case IRUINT16:
		case IRUINT32:
		//case IRNUINT:
			gzprintf( fp, "%u ", (JITUINT32)value);
			break;
		case IRUINT64:
		case IRNUINT :
		case IROFFSET:
		case IRLABELITEM:
			gzprintf( fp, "%llu ", value);
			break;
		case IRFLOAT32:
		case IRFLOAT64:
		case IRNFLOAT:
			gzprintf( fp, "%f ", fvalue);
			break;
		case IRMPOINTER:
		case IRFPOINTER:
		case IROBJECT:
		case IRMETHODENTRYPOINT:
			gzprintf( fp, "%p ", (void *)(JITNUINT)value);
			break;
		case IRMETHODID:
			break;
		case IRCLASSID:
			break;
		case IRTYPE:
			print_IR_type(value);
			break;
		case IRVOID:
			break;
		case IRTYPEDREF:
			gzprintf( fp, "TypedRef ");
			break;
		case IRSTRING:
			gzprintf( fp, "irstring: %s", (char *) (JITNUINT) value);
			break;
		default:
			gzprintf( fp, "NOT found\n");
			break;
		}

}
void print_IR_FirstParameter(ir_method_t *method, t_ir_instruction *inst){

	//assertions
	assert(method != NULL);
	assert(inst != NULL);

	print_IR_type(method->getInstrPar1Type(inst));
	print_IR_Parameter_value(method, inst, 1);

	//if par is IRVariable 
	if(method->getInstrPar1IntType(inst) != method->getInstrPar1Type(inst)){
		gzprintf( fp, "[ ");
		print_IR_type(method->getInstrPar1IntType(inst));
		gzprintf( fp, "]");
	}
	gzprintf( fp, ", ");

}
	

void print_IR_SecondParameter(ir_method_t *method, t_ir_instruction *inst){

	//assertions
	assert(method != NULL);
	assert(inst != NULL);

	print_IR_type(method->getInstrPar2Type(inst));
	print_IR_Parameter_value(method, inst, 2);

	//if par is IRVariable 
	if(method->getInstrPar2IntType(inst) != method->getInstrPar2Type(inst)){
		gzprintf( fp, "[ ");
		print_IR_type(method->getInstrPar2IntType(inst));
		gzprintf( fp, "]");
	}
	gzprintf( fp, ", ");
}

void print_IR_ThirdParameter(ir_method_t *method, t_ir_instruction *inst){

	//assertions
	assert(method != NULL);
	assert(inst != NULL);

	print_IR_type(method->getInstrPar3Type(inst));
	print_IR_Parameter_value(method, inst, 3);

	//if par is IRVariable 
	if(method->getInstrPar3IntType(inst) != method->getInstrPar3Type(inst)){
		gzprintf( fp, "[ ");
		print_IR_type(method->getInstrPar3IntType(inst));
		gzprintf( fp, "]");
	}
	gzprintf( fp, ", ");
}


void print_result(ir_method_t *method, t_ir_instruction *inst){


	//assertions
	assert(method != NULL);
	assert(inst != NULL);


	if ((method->getInstrResult(inst)->type != NOPARAM) && (method->getInstrResult(inst)->type != IRVOID)){
		
		print_IR_type(method->getInstrResult(inst)->type);
		gzprintf( fp, "%llu [ ", method->getInstrResult(inst)->value);
		print_IR_type(method->getInstrResult(inst)->internal_type);
		gzprintf( fp, " ] = ");
		
	}
	

}


void print_IR_type(JITUINT32 type){

	switch (type){
		case IRINT8:
			gzprintf( fp, "INT8 ");
			break;
		case IRINT16:
			gzprintf( fp, "INT16 ");
			break;
		case IRINT32:
			gzprintf( fp, "INT32 ");
			break;
		case IRINT64:
			gzprintf( fp, "INT64 ");
			break;
		case IRNINT :
			gzprintf( fp, "NINT ");
			break;
		case IRUINT8:
			gzprintf( fp, "UINT8 ");
			break;
		case IRUINT16:
			gzprintf( fp, "UINT16 ");
			break;
		case IRUINT32:
			gzprintf( fp, "UINT32 ");
			break;
		case IRUINT64:
			gzprintf( fp, "UINT64 ");
			break;
		case IRNUINT :
			gzprintf( fp, "NUINT ");
			break;
		case IRFLOAT32:
			gzprintf( fp, "FLOAT32 ");
			break;
		case IRFLOAT64:
			gzprintf( fp, "FLOAT64 ");
			break;
		case IRNFLOAT:
			gzprintf( fp, "NFLOAT ");
			break;
		case IRMPOINTER:
			gzprintf( fp, "MPOINTER ");
			break;
		case IRTPOINTER:
			gzprintf( fp, "TPOINTER ");
			break;
		case IROBJECT:
			gzprintf( fp, "OBJECT ");
			break;
		case IRVALUETYPE:
			gzprintf( fp, "VALUETYPE ");
			break;
		case IRTYPE:
			gzprintf( fp, "TYPE ");
			break;
		case IRCLASSID:
			gzprintf( fp, "CLASSID ");
			break;
		case IRMETHODID:
			gzprintf( fp, "METHODID ");
			break;
		case IRMETHODENTRYPOINT:
			gzprintf( fp, "METHODENTRYPOINT ");
			break;
		case IRLABELITEM:
			gzprintf( fp, "LABELITEM ");
			break;
		case IRVOID:
			gzprintf( fp, "VOID ");
			break;
		case IRTYPEDREF:
			gzprintf( fp, "TYPEDREF ");
			break;
		case IROFFSET:
			gzprintf( fp, "var ");
			break;
		case NOPARAM:
			break;
		default:
		//	abort();
			break;
	}

}

t_ir_instruction * insert_dumper_var_int8 (ir_method_t *method, t_ir_instruction *beforeInst, ir_item_t *par){
	t_ir_instruction	*instBefore1;
	ir_item_t		*ir_item;
	ir_item_t		*ir_to_dump;
	
	assert(par->internal_type == IRINT8);

	instBefore1		= IRMETHOD_newInstructionBefore(method, beforeInst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);

	ir_item				= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type		= IRTYPE;

	ir_item				= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_var_int8";
	ir_item->type			= IRNINT;
	ir_item->internal_type		= IRNINT;

	ir_item				= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_var_int8;
	ir_item->type			= IRNINT;
	ir_item->internal_type		= IRNINT;

	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	ir_to_dump->value 		= par->value;
	ir_to_dump->type		= IRUINT64;
	ir_to_dump->internal_type	= IRUINT64;
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);
	
	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	memcpy(ir_to_dump, par, sizeof(ir_item_t));
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);

	assert(instBefore1->callParameters->length(instBefore1->callParameters) == 2);

	return instBefore1;
}


t_ir_instruction * insert_dumper_var_int16 (ir_method_t *method, t_ir_instruction *beforeInst, ir_item_t *par){
	t_ir_instruction	*instBefore1;
	ir_item_t		*ir_item;
	ir_item_t		*ir_to_dump;
	
	assert(par->internal_type == IRINT16);

	instBefore1		= IRMETHOD_newInstructionBefore(method, beforeInst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);

	ir_item				= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type		= IRTYPE;

	ir_item				= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_var_int16";
	ir_item->type			= IRNINT;
	ir_item->internal_type		= IRNINT;

	ir_item				= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_var_int16;
	ir_item->type			= IRNINT;
	ir_item->internal_type		= IRNINT;

	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	ir_to_dump->value 		= par->value;
	ir_to_dump->type		= IRUINT64;
	ir_to_dump->internal_type	= IRUINT64;
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);
	
	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	memcpy(ir_to_dump, par, sizeof(ir_item_t));
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);

	assert(instBefore1->callParameters->length(instBefore1->callParameters) == 2);

	return instBefore1;
}


t_ir_instruction * insert_dumper_var_int32 (ir_method_t *method, t_ir_instruction *beforeInst, ir_item_t *par){
	t_ir_instruction	*instBefore1;
	ir_item_t		*ir_item;
	ir_item_t		*ir_to_dump;
	
	assert(par->internal_type == IRINT32);

	instBefore1		= IRMETHOD_newInstructionBefore(method, beforeInst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);

	ir_item				= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type		= IRTYPE;

	ir_item				= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_var_int32";
	ir_item->type			= IRNINT;
	ir_item->internal_type		= IRNINT;

	ir_item				= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_var_int32;
	ir_item->type			= IRNINT;
	ir_item->internal_type		= IRNINT;

	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	ir_to_dump->value 		= par->value;
	ir_to_dump->type		= IRUINT64;
	ir_to_dump->internal_type	= IRUINT64;
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);

	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	memcpy(ir_to_dump, par, sizeof(ir_item_t));
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);

	assert(instBefore1->callParameters->length(instBefore1->callParameters) == 2);

	return instBefore1;
}


t_ir_instruction * insert_dumper_var_int64 (ir_method_t *method, t_ir_instruction *beforeInst, ir_item_t *par){
	t_ir_instruction	*instBefore1;
	ir_item_t		*ir_item;
	ir_item_t		*ir_to_dump;
	
	assert(par->internal_type == IRINT64);

	instBefore1		= IRMETHOD_newInstructionBefore(method, beforeInst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);

	ir_item				= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type		= IRTYPE;

	ir_item				= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_var_int64";
	ir_item->type			= IRNINT;
	ir_item->internal_type		= IRNINT;

	ir_item				= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_var_int64;
	ir_item->type			= IRNINT;
	ir_item->internal_type		= IRNINT;

	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	ir_to_dump->value 		= par->value;
	ir_to_dump->type		= IRUINT64;
	ir_to_dump->internal_type	= IRUINT64;
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);
	
	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	memcpy(ir_to_dump, par, sizeof(ir_item_t));
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);

	assert(instBefore1->callParameters->length(instBefore1->callParameters) == 2);

	return instBefore1;
}


t_ir_instruction * insert_dumper_var_uint8 (ir_method_t *method, t_ir_instruction *beforeInst, ir_item_t *par){
	t_ir_instruction	*instBefore1;
	ir_item_t		*ir_item;
	ir_item_t		*ir_to_dump;
	
	assert(par->internal_type == IRUINT8);

	instBefore1		= IRMETHOD_newInstructionBefore(method, beforeInst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);

	ir_item				= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type		= IRTYPE;

	ir_item				= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_var_uint8";
	ir_item->type			= IRNINT;
	ir_item->internal_type		= IRNINT;

	ir_item				= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_var_uint8;
	ir_item->type			= IRNINT;
	ir_item->internal_type		= IRNINT;

	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	ir_to_dump->value 		= par->value;
	ir_to_dump->type		= IRUINT64;
	ir_to_dump->internal_type	= IRUINT64;
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);
	
	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	memcpy(ir_to_dump, par, sizeof(ir_item_t));
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);

	assert(instBefore1->callParameters->length(instBefore1->callParameters) == 2);

	return instBefore1;
}


t_ir_instruction * insert_dumper_var_uint16 (ir_method_t *method, t_ir_instruction *beforeInst, ir_item_t *par){
	t_ir_instruction	*instBefore1;
	ir_item_t		*ir_item;
	ir_item_t		*ir_to_dump;
	
	assert(par->internal_type == IRUINT16);

	instBefore1		= IRMETHOD_newInstructionBefore(method, beforeInst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);

	ir_item				= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type		= IRTYPE;

	ir_item				= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_var_uint16";
	ir_item->type			= IRNINT;
	ir_item->internal_type		= IRNINT;

	ir_item				= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_var_uint16;
	ir_item->type			= IRNINT;
	ir_item->internal_type		= IRNINT;

	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	ir_to_dump->value 		= par->value;
	ir_to_dump->type		= IRUINT64;
	ir_to_dump->internal_type	= IRUINT64;
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);
	
	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	memcpy(ir_to_dump, par, sizeof(ir_item_t));
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);

	assert(instBefore1->callParameters->length(instBefore1->callParameters) == 2);

	return instBefore1;
}


t_ir_instruction * insert_dumper_var_uint32 (ir_method_t *method, t_ir_instruction *beforeInst, ir_item_t *par){
	t_ir_instruction	*instBefore1;
	ir_item_t		*ir_item;
	ir_item_t		*ir_to_dump;
	
	assert(par->internal_type == IRUINT32);

	instBefore1		= IRMETHOD_newInstructionBefore(method, beforeInst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);

	ir_item				= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type		= IRTYPE;

	ir_item				= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_var_uint32";
	ir_item->type			= IRNINT;
	ir_item->internal_type		= IRNINT;

	ir_item				= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_var_uint32;
	ir_item->type			= IRNINT;
	ir_item->internal_type		= IRNINT;

	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	ir_to_dump->value 		= par->value;
	ir_to_dump->type		= IRUINT64;
	ir_to_dump->internal_type	= IRUINT64;
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);
	
	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	memcpy(ir_to_dump, par, sizeof(ir_item_t));
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);

	assert(instBefore1->callParameters->length(instBefore1->callParameters) == 2);

	return instBefore1;
}



t_ir_instruction * insert_dumper_var_uint64 (ir_method_t *method, t_ir_instruction *beforeInst, ir_item_t *par){
	t_ir_instruction	*instBefore1;
	ir_item_t		*ir_item;
	ir_item_t		*ir_to_dump;
	
	assert(par->internal_type == IRUINT64);

	instBefore1		= IRMETHOD_newInstructionBefore(method, beforeInst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);

	ir_item				= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type		= IRTYPE;

	ir_item				= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_var_uint64";
	ir_item->type			= IRNINT;
	ir_item->internal_type		= IRNINT;

	ir_item				= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_var_uint64;
	ir_item->type			= IRNINT;
	ir_item->internal_type		= IRNINT;

	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	ir_to_dump->value 		= par->value;
	ir_to_dump->type		= IRUINT64;
	ir_to_dump->internal_type	= IRUINT64;
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);
	
	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	memcpy(ir_to_dump, par, sizeof(ir_item_t));
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);

	assert(instBefore1->callParameters->length(instBefore1->callParameters) == 2);

	return instBefore1;
}


t_ir_instruction * insert_dumper_var_float32 (ir_method_t *method, t_ir_instruction *beforeInst, ir_item_t *par){

	t_ir_instruction	*instBefore1;
	ir_item_t		*ir_item;
	ir_item_t		*ir_to_dump;
	
	assert(par->internal_type == IRFLOAT32);
	
	instBefore1		= IRMETHOD_newInstructionBefore(method, beforeInst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);
	
	ir_item					= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type	= IRTYPE;
				
	ir_item					= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_var_float32";
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;

				
	ir_item					= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_var_float32;
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;
				
	ir_to_dump					= allocFunction(sizeof(ir_item_t));
	ir_to_dump->value 			= par->value;
	ir_to_dump->type			= IRUINT64;
	ir_to_dump->internal_type	= IRUINT64;
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);
	
	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	memcpy(ir_to_dump, par, sizeof(ir_item_t));
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);

	assert(instBefore1->callParameters->length(instBefore1->callParameters) == 2);

	return instBefore1;
}

t_ir_instruction * insert_dumper_var_float64 (ir_method_t *method, t_ir_instruction *beforeInst, ir_item_t *par){

	t_ir_instruction	*instBefore1;
	ir_item_t		*ir_item;
	ir_item_t		*ir_to_dump;
	
	assert(par->internal_type == IRFLOAT64);
	
	instBefore1		= IRMETHOD_newInstructionBefore(method, beforeInst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);
	
	ir_item					= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type	= IRTYPE;
				
	ir_item					= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_var_float64";
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;

				
	ir_item					= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_var_float64;
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;
				
	ir_to_dump					= allocFunction(sizeof(ir_item_t));
	ir_to_dump->value 			= par->value;
	ir_to_dump->type			= IRUINT64;
	ir_to_dump->internal_type	= IRUINT64;
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);
	
	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	memcpy(ir_to_dump, par, sizeof(ir_item_t));
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);

	assert(instBefore1->callParameters->length(instBefore1->callParameters) == 2);

	return instBefore1;
}


t_ir_instruction * insert_dumper_var_result_int8 (ir_method_t *method, t_ir_instruction *beforeInst, ir_item_t *par){
	t_ir_instruction	*instBefore1;
	ir_item_t		*ir_item;
	ir_item_t		*ir_to_dump;
	
	assert(par->internal_type == IRINT8);

	instBefore1		= IRMETHOD_newInstructionAfter(method, beforeInst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);

	ir_item				= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type		= IRTYPE;

	ir_item				= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_var_int8";
	ir_item->type			= IRNINT;
	ir_item->internal_type		= IRNINT;

	ir_item				= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_var_int8;
	ir_item->type			= IRNINT;
	ir_item->internal_type		= IRNINT;

	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	ir_to_dump->value 		= par->value;
	ir_to_dump->type		= IRUINT64;
	ir_to_dump->internal_type	= IRUINT64;
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);
	
	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	memcpy(ir_to_dump, par, sizeof(ir_item_t));
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);

	assert(instBefore1->callParameters->length(instBefore1->callParameters) == 2);

	return instBefore1;
}


t_ir_instruction * insert_dumper_var_result_int16 (ir_method_t *method, t_ir_instruction *beforeInst, ir_item_t *par){
	t_ir_instruction	*instBefore1;
	ir_item_t		*ir_item;
	ir_item_t		*ir_to_dump;
	
	assert(par->internal_type == IRINT16);

	instBefore1		= IRMETHOD_newInstructionAfter(method, beforeInst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);

	ir_item				= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type		= IRTYPE;

	ir_item				= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_var_int16";
	ir_item->type			= IRNINT;
	ir_item->internal_type		= IRNINT;

	ir_item				= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_var_int16;
	ir_item->type			= IRNINT;
	ir_item->internal_type		= IRNINT;

	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	ir_to_dump->value 		= par->value;
	ir_to_dump->type		= IRUINT64;
	ir_to_dump->internal_type	= IRUINT64;
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);
	
	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	memcpy(ir_to_dump, par, sizeof(ir_item_t));
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);

	assert(instBefore1->callParameters->length(instBefore1->callParameters) == 2);

	return instBefore1;
}


t_ir_instruction * insert_dumper_var_result_int32 (ir_method_t *method, t_ir_instruction *beforeInst, ir_item_t *par){
	t_ir_instruction	*instBefore1;
	ir_item_t		*ir_item;
	ir_item_t		*ir_to_dump;
	
	assert(par->internal_type == IRINT32);

	instBefore1		= IRMETHOD_newInstructionAfter(method, beforeInst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);

	ir_item				= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type		= IRTYPE;

	ir_item				= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_var_int32";
	ir_item->type			= IRNINT;
	ir_item->internal_type		= IRNINT;

	ir_item				= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_var_int32;
	ir_item->type			= IRNINT;
	ir_item->internal_type		= IRNINT;

	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	ir_to_dump->value 		= par->value;
	ir_to_dump->type		= IRUINT64;
	ir_to_dump->internal_type	= IRUINT64;
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);
	
	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	memcpy(ir_to_dump, par, sizeof(ir_item_t));
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);

	assert(instBefore1->callParameters->length(instBefore1->callParameters) == 2);

	return instBefore1;
}


t_ir_instruction * insert_dumper_var_result_int64 (ir_method_t *method, t_ir_instruction *beforeInst, ir_item_t *par){
	t_ir_instruction	*instBefore1;
	ir_item_t		*ir_item;
	ir_item_t		*ir_to_dump;
	
	assert(par->internal_type == IRINT64);

	instBefore1		= IRMETHOD_newInstructionAfter(method, beforeInst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);

	ir_item				= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type		= IRTYPE;

	ir_item				= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_var_int64";
	ir_item->type			= IRNINT;
	ir_item->internal_type		= IRNINT;

	ir_item				= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_var_int64;
	ir_item->type			= IRNINT;
	ir_item->internal_type		= IRNINT;

	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	ir_to_dump->value 		= par->value;
	ir_to_dump->type		= IRUINT64;
	ir_to_dump->internal_type	= IRUINT64;
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);
	
	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	memcpy(ir_to_dump, par, sizeof(ir_item_t));
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);

	assert(instBefore1->callParameters->length(instBefore1->callParameters) == 2);

	return instBefore1;
}


t_ir_instruction * insert_dumper_var_result_uint8 (ir_method_t *method, t_ir_instruction *beforeInst, ir_item_t *par){
	t_ir_instruction	*instBefore1;
	ir_item_t		*ir_item;
	ir_item_t		*ir_to_dump;
	
	assert(par->internal_type == IRUINT8);

	instBefore1		= IRMETHOD_newInstructionAfter(method, beforeInst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);

	ir_item				= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type		= IRTYPE;

	ir_item				= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_var_uint8";
	ir_item->type			= IRNINT;
	ir_item->internal_type		= IRNINT;

	ir_item				= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_var_uint8;
	ir_item->type			= IRNINT;
	ir_item->internal_type		= IRNINT;

	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	ir_to_dump->value 		= par->value;
	ir_to_dump->type		= IRUINT64;
	ir_to_dump->internal_type	= IRUINT64;
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);
	
	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	memcpy(ir_to_dump, par, sizeof(ir_item_t));
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);

	assert(instBefore1->callParameters->length(instBefore1->callParameters) == 2);

	return instBefore1;
}


t_ir_instruction * insert_dumper_var_result_uint16 (ir_method_t *method, t_ir_instruction *beforeInst, ir_item_t *par){
	t_ir_instruction	*instBefore1;
	ir_item_t		*ir_item;
	ir_item_t		*ir_to_dump;
	
	assert(par->internal_type == IRUINT16);

	instBefore1		= IRMETHOD_newInstructionAfter(method, beforeInst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);

	ir_item				= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type		= IRTYPE;

	ir_item				= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_var_uint16";
	ir_item->type			= IRNINT;
	ir_item->internal_type		= IRNINT;

	ir_item				= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_var_uint16;
	ir_item->type			= IRNINT;
	ir_item->internal_type		= IRNINT;

	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	ir_to_dump->value 		= par->value;
	ir_to_dump->type		= IRUINT64;
	ir_to_dump->internal_type	= IRUINT64;
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);
	
	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	memcpy(ir_to_dump, par, sizeof(ir_item_t));
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);

	assert(instBefore1->callParameters->length(instBefore1->callParameters) == 2);

	return instBefore1;
}


t_ir_instruction * insert_dumper_var_result_uint32 (ir_method_t *method, t_ir_instruction *beforeInst, ir_item_t *par){
	t_ir_instruction	*instBefore1;
	ir_item_t		*ir_item;
	ir_item_t		*ir_to_dump;
	
	assert(par->internal_type == IRUINT32);

	instBefore1		= IRMETHOD_newInstructionAfter(method, beforeInst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);

	ir_item				= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type		= IRTYPE;

	ir_item				= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_var_uint32";
	ir_item->type			= IRNINT;
	ir_item->internal_type		= IRNINT;

	ir_item				= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_var_uint32;
	ir_item->type			= IRNINT;
	ir_item->internal_type		= IRNINT;

	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	ir_to_dump->value 		= par->value;
	ir_to_dump->type		= IRUINT64;
	ir_to_dump->internal_type	= IRUINT64;
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);
	
	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	memcpy(ir_to_dump, par, sizeof(ir_item_t));
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);

	assert(instBefore1->callParameters->length(instBefore1->callParameters) == 2);

	return instBefore1;
}



t_ir_instruction * insert_dumper_var_result_uint64 (ir_method_t *method, t_ir_instruction *beforeInst, ir_item_t *par){
	t_ir_instruction	*instBefore1;
	ir_item_t		*ir_item;
	ir_item_t		*ir_to_dump;
	
	assert(par->internal_type == IRUINT64);

	instBefore1		= IRMETHOD_newInstructionAfter(method, beforeInst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);

	ir_item				= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type		= IRTYPE;

	ir_item				= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_var_uint64";
	ir_item->type			= IRNINT;
	ir_item->internal_type		= IRNINT;

	ir_item				= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_var_uint64;
	ir_item->type			= IRNINT;
	ir_item->internal_type		= IRNINT;

	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	ir_to_dump->value 		= par->value;
	ir_to_dump->type		= IRUINT64;
	ir_to_dump->internal_type	= IRUINT64;
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);
	
	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	memcpy(ir_to_dump, par, sizeof(ir_item_t));
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);

	assert(instBefore1->callParameters->length(instBefore1->callParameters) == 2);

	return instBefore1;
}


t_ir_instruction * insert_dumper_var_result_float32 (ir_method_t *method, t_ir_instruction *beforeInst, ir_item_t *par){

	t_ir_instruction	*instBefore1;
	ir_item_t		*ir_item;
	ir_item_t		*ir_to_dump;
	
	assert(par->internal_type == IRFLOAT32);
	
	instBefore1		= IRMETHOD_newInstructionAfter(method, beforeInst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);
	
	ir_item					= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type	= IRTYPE;
				
	ir_item					= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_var_float32";
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;

				
	ir_item					= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_var_float32;
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;
				
	ir_to_dump					= allocFunction(sizeof(ir_item_t));
	ir_to_dump->value 			= par->value;
	ir_to_dump->type			= IRUINT64;
	ir_to_dump->internal_type	= IRUINT64;
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);
	
	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	memcpy(ir_to_dump, par, sizeof(ir_item_t));
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);

	assert(instBefore1->callParameters->length(instBefore1->callParameters) == 2);

	return instBefore1;
}

t_ir_instruction * insert_dumper_var_result_float64 (ir_method_t *method, t_ir_instruction *beforeInst, ir_item_t *par){

	t_ir_instruction	*instBefore1;
	ir_item_t		*ir_item;
	ir_item_t		*ir_to_dump;
	
	assert(par->internal_type == IRFLOAT64);
	
	instBefore1		= IRMETHOD_newInstructionAfter(method, beforeInst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);
	
	ir_item					= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type	= IRTYPE;
				
	ir_item					= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_var_float64";
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;

				
	ir_item					= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_var_float64;
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;
				
	ir_to_dump					= allocFunction(sizeof(ir_item_t));
	ir_to_dump->value 			= par->value;
	ir_to_dump->type			= IRUINT64;
	ir_to_dump->internal_type	= IRUINT64;
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);
	
	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	memcpy(ir_to_dump, par, sizeof(ir_item_t));
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);

	assert(instBefore1->callParameters->length(instBefore1->callParameters) == 2);

	return instBefore1;
}

t_ir_instruction * insert_dumper_int8 (ir_method_t *method, t_ir_instruction *inst, ir_item_t *par){
	t_ir_instruction 		*instBefore1;
	ir_item_t 				*ir_item;
	ir_item_t				*ir_to_dump;


	instBefore1		= IRMETHOD_newInstructionBefore(method, inst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);

	ir_item					= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type	= IRTYPE;

	ir_item					= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_int8";
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;
	                                                                             

	ir_item					= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_int8;
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;

	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	memcpy(ir_to_dump, par, sizeof(ir_item_t));
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);

	return instBefore1;
}


t_ir_instruction * insert_dumper_int16 (ir_method_t *method, t_ir_instruction *inst, ir_item_t *par){
	t_ir_instruction 		*instBefore1;
	ir_item_t 				*ir_item;
	ir_item_t				*ir_to_dump;


	instBefore1		= IRMETHOD_newInstructionBefore(method, inst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);

	ir_item					= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type	= IRTYPE;

	ir_item					= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_int16";
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;
	                                                                             

	ir_item					= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_int16;
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;

	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	memcpy(ir_to_dump, par, sizeof(ir_item_t));
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);

	return instBefore1;
}


t_ir_instruction * insert_dumper_int32 (ir_method_t *method, t_ir_instruction *inst, ir_item_t *par){
	t_ir_instruction 		*instBefore1;
	ir_item_t 				*ir_item;
	ir_item_t				*ir_to_dump;


	instBefore1		= IRMETHOD_newInstructionBefore(method, inst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);

	ir_item					= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type	= IRTYPE;

	ir_item					= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_int32";
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;
	                                                                             

	ir_item					= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_int32;
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;

	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	memcpy(ir_to_dump, par, sizeof(ir_item_t));
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);

	return instBefore1;
}


t_ir_instruction * insert_dumper_int64 (ir_method_t *method, t_ir_instruction *inst, ir_item_t *par){
	t_ir_instruction 		*instBefore1;
	ir_item_t 				*ir_item;
	ir_item_t				*ir_to_dump;


	instBefore1		= IRMETHOD_newInstructionBefore(method, inst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);

	ir_item					= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type	= IRTYPE;

	ir_item					= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_int64";
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;
	                                                                             

	ir_item					= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_int64;
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;

	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	memcpy(ir_to_dump, par, sizeof(ir_item_t));
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);

	return instBefore1;
}


t_ir_instruction * insert_dumper_uint8 (ir_method_t *method, t_ir_instruction *inst, ir_item_t *par){
	t_ir_instruction 		*instBefore1;
	ir_item_t 				*ir_item;
	ir_item_t				*ir_to_dump;


	instBefore1		= IRMETHOD_newInstructionBefore(method, inst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);

	ir_item					= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type	= IRTYPE;

	ir_item					= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_uint8";
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;
	                                                                             

	ir_item					= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_uint8;
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;

	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	memcpy(ir_to_dump, par, sizeof(ir_item_t));
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);

	return instBefore1;
}


t_ir_instruction * insert_dumper_uint16 (ir_method_t *method, t_ir_instruction *inst, ir_item_t *par){
	t_ir_instruction 		*instBefore1;
	ir_item_t 				*ir_item;
	ir_item_t				*ir_to_dump;


	instBefore1		= IRMETHOD_newInstructionBefore(method, inst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);

	ir_item					= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type	= IRTYPE;

	ir_item					= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_uint16";
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;
	                                                                             

	ir_item					= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_uint16;
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;

	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	memcpy(ir_to_dump, par, sizeof(ir_item_t));
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);

	return instBefore1;
}


t_ir_instruction * insert_dumper_uint32 (ir_method_t *method, t_ir_instruction *inst, ir_item_t *par){
	t_ir_instruction 		*instBefore1;
	ir_item_t 				*ir_item;
	ir_item_t				*ir_to_dump;


	instBefore1		= IRMETHOD_newInstructionBefore(method, inst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);

	ir_item					= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type	= IRTYPE;

	ir_item					= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_uint32";
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;
	                                                                             

	ir_item					= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_uint32;
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;

	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	memcpy(ir_to_dump, par, sizeof(ir_item_t));
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);

	return instBefore1;
}


t_ir_instruction * insert_dumper_uint64 (ir_method_t *method, t_ir_instruction *inst, ir_item_t *par){
	t_ir_instruction 		*instBefore1;
	ir_item_t 				*ir_item;
	ir_item_t				*ir_to_dump;


	instBefore1		= IRMETHOD_newInstructionBefore(method, inst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);

	ir_item					= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type	= IRTYPE;

	ir_item					= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_uint64";
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;
	                                                                             

	ir_item					= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_uint64;
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;

	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	memcpy(ir_to_dump, par, sizeof(ir_item_t));
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);

	return instBefore1;
}

t_ir_instruction * insert_dumper_float32 (ir_method_t *method, t_ir_instruction *inst, ir_item_t *par){
	t_ir_instruction 		*instBefore1;
	ir_item_t 				*ir_item;
	ir_item_t				*ir_to_dump;


	instBefore1		= IRMETHOD_newInstructionBefore(method, inst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);

	ir_item					= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type	= IRTYPE;

	ir_item					= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_float32";
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;
	                                                                             

	ir_item					= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_float32;
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;

	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	memcpy(ir_to_dump, par, sizeof(ir_item_t));
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);

	return instBefore1;
}


t_ir_instruction * insert_dumper_float64 (ir_method_t *method, t_ir_instruction *inst, ir_item_t *par){
	t_ir_instruction 		*instBefore1;
	ir_item_t 				*ir_item;
	ir_item_t				*ir_to_dump;


	instBefore1		= IRMETHOD_newInstructionBefore(method, inst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);

	ir_item					= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type	= IRTYPE;

	ir_item					= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_float64";
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;
	                                                                             

	ir_item					= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_float64;
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;

	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	memcpy(ir_to_dump, par, sizeof(ir_item_t));
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);

	return instBefore1;
}

t_ir_instruction * insert_dumper_result_int8 (ir_method_t *method, t_ir_instruction *inst, ir_item_t *par){
	t_ir_instruction 		*instBefore1;
	ir_item_t 				*ir_item;
	ir_item_t				*ir_to_dump;


	instBefore1		= IRMETHOD_newInstructionAfter(method, inst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);

	ir_item					= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type	= IRTYPE;

	ir_item					= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_int8";
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;
	                                                                             

	ir_item					= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_int8;
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;

	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	memcpy(ir_to_dump, par, sizeof(ir_item_t));
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);

	return instBefore1;
}


t_ir_instruction * insert_dumper_result_int16 (ir_method_t *method, t_ir_instruction *inst, ir_item_t *par){
	t_ir_instruction 		*instBefore1;
	ir_item_t 				*ir_item;
	ir_item_t				*ir_to_dump;


	instBefore1		= IRMETHOD_newInstructionAfter(method, inst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);

	ir_item					= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type	= IRTYPE;

	ir_item					= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_int16";
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;
	                                                                             

	ir_item					= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_int16;
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;

	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	memcpy(ir_to_dump, par, sizeof(ir_item_t));
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);

	return instBefore1;
}


t_ir_instruction * insert_dumper_result_int32 (ir_method_t *method, t_ir_instruction *inst, ir_item_t *par){
	t_ir_instruction 		*instBefore1;
	ir_item_t 				*ir_item;
	ir_item_t				*ir_to_dump;


	instBefore1		= IRMETHOD_newInstructionAfter(method, inst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);

	ir_item					= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type	= IRTYPE;

	ir_item					= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_int32";
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;
	                                                                             

	ir_item					= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_int32;
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;

	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	memcpy(ir_to_dump, par, sizeof(ir_item_t));
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);

	return instBefore1;
}


t_ir_instruction * insert_dumper_result_int64 (ir_method_t *method, t_ir_instruction *inst, ir_item_t *par){
	t_ir_instruction 		*instBefore1;
	ir_item_t 				*ir_item;
	ir_item_t				*ir_to_dump;


	instBefore1		= IRMETHOD_newInstructionAfter(method, inst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);

	ir_item					= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type	= IRTYPE;

	ir_item					= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_int64";
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;
	                                                                             

	ir_item					= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_int64;
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;

	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	memcpy(ir_to_dump, par, sizeof(ir_item_t));
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);

	return instBefore1;
}


t_ir_instruction * insert_dumper_result_uint8 (ir_method_t *method, t_ir_instruction *inst, ir_item_t *par){
	t_ir_instruction 		*instBefore1;
	ir_item_t 				*ir_item;
	ir_item_t				*ir_to_dump;


	instBefore1		= IRMETHOD_newInstructionAfter(method, inst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);

	ir_item					= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type	= IRTYPE;

	ir_item					= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_uint8";
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;
	                                                                             

	ir_item					= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_uint8;
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;

	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	memcpy(ir_to_dump, par, sizeof(ir_item_t));
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);

	return instBefore1;
}


t_ir_instruction * insert_dumper_result_uint16 (ir_method_t *method, t_ir_instruction *inst, ir_item_t *par){
	t_ir_instruction 		*instBefore1;
	ir_item_t 				*ir_item;
	ir_item_t				*ir_to_dump;


	instBefore1		= IRMETHOD_newInstructionAfter(method, inst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);

	ir_item					= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type	= IRTYPE;

	ir_item					= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_uint16";
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;
	                                                                             

	ir_item					= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_uint16;
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;

	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	memcpy(ir_to_dump, par, sizeof(ir_item_t));
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);

	return instBefore1;
}


t_ir_instruction * insert_dumper_result_uint32 (ir_method_t *method, t_ir_instruction *inst, ir_item_t *par){
	t_ir_instruction 		*instBefore1;
	ir_item_t 				*ir_item;
	ir_item_t				*ir_to_dump;


	instBefore1		= IRMETHOD_newInstructionAfter(method, inst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);

	ir_item					= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type	= IRTYPE;

	ir_item					= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_uint32";
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;
	                                                                             

	ir_item					= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_uint32;
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;

	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	memcpy(ir_to_dump, par, sizeof(ir_item_t));
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);

	return instBefore1;
}


t_ir_instruction * insert_dumper_result_uint64 (ir_method_t *method, t_ir_instruction *inst, ir_item_t *par){
	t_ir_instruction 		*instBefore1;
	ir_item_t 				*ir_item;
	ir_item_t				*ir_to_dump;


	instBefore1		= IRMETHOD_newInstructionAfter(method, inst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);

	ir_item					= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type	= IRTYPE;

	ir_item					= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_uint64";
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;
	                                                                             

	ir_item					= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_uint64;
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;

	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	memcpy(ir_to_dump, par, sizeof(ir_item_t));
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);

	return instBefore1;
}

t_ir_instruction * insert_dumper_result_float32 (ir_method_t *method, t_ir_instruction *inst, ir_item_t *par){
	t_ir_instruction 		*instBefore1;
	ir_item_t 				*ir_item;
	ir_item_t				*ir_to_dump;


	instBefore1		= IRMETHOD_newInstructionAfter(method, inst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);

	ir_item					= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type	= IRTYPE;

	ir_item					= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_float32";
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;
	                                                                             

	ir_item					= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_float32;
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;

	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	memcpy(ir_to_dump, par, sizeof(ir_item_t));
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);

	return instBefore1;
}


t_ir_instruction * insert_dumper_result_float64 (ir_method_t *method, t_ir_instruction *inst, ir_item_t *par){
	t_ir_instruction 		*instBefore1;
	ir_item_t 				*ir_item;
	ir_item_t				*ir_to_dump;


	instBefore1		= IRMETHOD_newInstructionAfter(method, inst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);

	ir_item					= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type	= IRTYPE;

	ir_item					= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_float64";
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;
	                                                                             

	ir_item					= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_float64;
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;

	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	memcpy(ir_to_dump, par, sizeof(ir_item_t));
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);

	return instBefore1;
}


t_ir_instruction * insert_dumper_label (ir_method_t *method, t_ir_instruction *inst, ir_item_t *par){
	t_ir_instruction 		*instBefore1;
	ir_item_t 				*ir_item;
	ir_item_t				*ir_to_dump;
	
	
	instBefore1		= IRMETHOD_newInstructionBefore(method, inst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);
																				  
	ir_item					= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type	= IRTYPE;
																				  
	ir_item					= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_label";
	ir_item->type			= IRNUINT;
	ir_item->internal_type	= IRNUINT;
																				  
																				  
	ir_item					= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_label;
	ir_item->type			= IRNUINT;
	ir_item->internal_type	= IRNUINT;
																				  
	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	ir_to_dump->value 			= par->value;
	ir_to_dump->type			= IRUINT64;
	ir_to_dump->internal_type	= IRUINT64;
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);
	
	return instBefore1;
}


t_ir_instruction * insert_dumper_method (ir_method_t *method, t_ir_instruction *inst, ir_item_t *par){
	t_ir_instruction 		*instBefore1;
	ir_item_t 				*ir_item;
	ir_item_t				*ir_to_dump;
	
	instBefore1		= IRMETHOD_newInstructionBefore(method, inst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);
																				  
	ir_item					= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type	= IRTYPE;
																				  
	ir_item					= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_methodid";
	ir_item->type			= IRNUINT;
	ir_item->internal_type	= IRNUINT;
																				  
																				  
	ir_item					= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_methodid;
	ir_item->type			= IRNUINT;
	ir_item->internal_type	= IRNUINT;
																				  
	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	ir_to_dump->value 			= par->value;
	ir_to_dump->type			= IRNINT;
	ir_to_dump->internal_type	= IRNINT;
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);
	
	return instBefore1;
}

t_ir_instruction * insert_dumper_class (ir_method_t *method, t_ir_instruction *inst, ir_item_t *par){
	t_ir_instruction 		*instBefore1;
	ir_item_t 				*ir_item;
	ir_item_t				*ir_to_dump;
	
	instBefore1		= IRMETHOD_newInstructionBefore(method, inst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);
																				  
	ir_item					= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type	= IRTYPE;
																				  
	ir_item					= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_classid";
	ir_item->type			= IRNUINT;
	ir_item->internal_type	= IRNUINT;
																				  
																				  
	ir_item					= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_classid;
	ir_item->type			= IRNUINT;
	ir_item->internal_type	= IRNUINT;
																				  
	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	ir_to_dump->value 			= par->value;
	ir_to_dump->type			= IRNINT;
	ir_to_dump->internal_type	= IRNINT;
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);
	
	return instBefore1;
}
	
t_ir_instruction * insert_dumper_result_label (ir_method_t *method, t_ir_instruction *inst, ir_item_t *par){
	t_ir_instruction 		*instBefore1;
	ir_item_t 				*ir_item;
	ir_item_t				*ir_to_dump;
	
	
	instBefore1		= IRMETHOD_newInstructionAfter(method, inst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);
																				  
	ir_item					= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type	= IRTYPE;
																				  
	ir_item					= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_label";
	ir_item->type			= IRNUINT;
	ir_item->internal_type	= IRNUINT;
																				  
																				  
	ir_item					= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_label;
	ir_item->type			= IRNUINT;
	ir_item->internal_type	= IRNUINT;
																				  
	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	ir_to_dump->value 			= par->value;
	ir_to_dump->type			= IRUINT64;
	ir_to_dump->internal_type	= IRUINT64;
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);
	
	return instBefore1;
}


t_ir_instruction * insert_dumper_result_method (ir_method_t *method, t_ir_instruction *inst, ir_item_t *par){
	t_ir_instruction 		*instBefore1;
	ir_item_t 				*ir_item;
	ir_item_t				*ir_to_dump;
	
	instBefore1		= IRMETHOD_newInstructionAfter(method, inst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);
																				  
	ir_item					= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type	= IRTYPE;
																				  
	ir_item					= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_methodid";
	ir_item->type			= IRNUINT;
	ir_item->internal_type	= IRNUINT;
																				  
																				  
	ir_item					= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_methodid;
	ir_item->type			= IRNUINT;
	ir_item->internal_type	= IRNUINT;
																				  
	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	ir_to_dump->value 			= par->value;
	ir_to_dump->type			= IRNINT;
	ir_to_dump->internal_type	= IRNINT;
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);
	
	return instBefore1;
}

t_ir_instruction * insert_dumper_result_class (ir_method_t *method, t_ir_instruction *inst, ir_item_t *par){
	t_ir_instruction 		*instBefore1;
	ir_item_t 				*ir_item;
	ir_item_t				*ir_to_dump;
	
	instBefore1		= IRMETHOD_newInstructionAfter(method, inst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);
																				  
	ir_item					= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type	= IRTYPE;
																				  
	ir_item					= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_classid";
	ir_item->type			= IRNUINT;
	ir_item->internal_type	= IRNUINT;
																				  
																				  
	ir_item					= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_classid;
	ir_item->type			= IRNUINT;
	ir_item->internal_type	= IRNUINT;
																				  
	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	ir_to_dump->value 			= par->value;
	ir_to_dump->type			= IRNINT;
	ir_to_dump->internal_type	= IRNINT;
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);
	
	return instBefore1;
}
	

t_ir_instruction * insert_dumper_pointer (ir_method_t *method, t_ir_instruction *inst, ir_item_t *par){
	t_ir_instruction 		*instBefore1;
	ir_item_t 				*ir_item;
	ir_item_t				*ir_to_dump;
	
	instBefore1		= IRMETHOD_newInstructionBefore(method, inst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);
																				  
	ir_item					= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type	= IRTYPE;
																				  
	ir_item					= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_pointer";
	ir_item->type			= IRNUINT;
	ir_item->internal_type	= IRNUINT;
																				  
																				  
	ir_item					= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_pointer;
	ir_item->type			= IRNUINT;
	ir_item->internal_type	= IRNUINT;
																				  
	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	ir_to_dump->value 			= (JITNUINT)par->value;
	ir_to_dump->type			= IRNUINT;
	ir_to_dump->internal_type	= IRNUINT;
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);
	
	return instBefore1;
}
	
t_ir_instruction * insert_dumper_result_pointer (ir_method_t *method, t_ir_instruction *inst, ir_item_t *par){
	t_ir_instruction 		*instBefore1;
	ir_item_t 				*ir_item;
	ir_item_t				*ir_to_dump;
	
	instBefore1		= IRMETHOD_newInstructionAfter(method, inst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);
																				  
	ir_item					= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type	= IRTYPE;
																				  
	ir_item					= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_pointer";
	ir_item->type			= IRNUINT;
	ir_item->internal_type	= IRNUINT;
																				  
																				  
	ir_item					= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_pointer;
	ir_item->type			= IRNUINT;
	ir_item->internal_type	= IRNUINT;
																				  
	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	ir_to_dump->value 			= (JITNUINT)par->value;
	ir_to_dump->type			= IRNUINT;
	ir_to_dump->internal_type	= IRNUINT;
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);
	
	return instBefore1;
}
	
t_ir_instruction * insert_dumper_irtype_valuetype (ir_method_t *method, t_ir_instruction *inst, ir_item_t *par){

	t_ir_instruction	*instBefore1;
	ir_item_t		*ir_item;
	ir_item_t		*ir_to_dump;
	
	
	instBefore1		= IRMETHOD_newInstructionBefore(method, inst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);
	
	ir_item					= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type	= IRTYPE;
				
	ir_item					= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_irtype_valuetype";
	ir_item->type			= IRNUINT;
	ir_item->internal_type	= IRNUINT;

				
	ir_item					= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_irtype_valuetype;
	ir_item->type			= IRNUINT;
	ir_item->internal_type	= IRNUINT;
				
	ir_to_dump					= allocFunction(sizeof(ir_item_t));
	ir_to_dump->value 			= IRDATA_getSizeOfType(irLib, par)*8;
	ir_to_dump->type			= IRINT32;
	ir_to_dump->internal_type	= IRINT32;
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);
	
	return instBefore1;
}

t_ir_instruction * insert_dumper_result_irtype_valuetype (ir_method_t *method, t_ir_instruction *inst, ir_item_t *par){

	t_ir_instruction	*instBefore1;
	ir_item_t		*ir_item;
	ir_item_t		*ir_to_dump;
	
	
	instBefore1		= IRMETHOD_newInstructionAfter(method, inst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);
	
	ir_item					= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type	= IRTYPE;
				
	ir_item					= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_irtype_valuetype";
	ir_item->type			= IRNUINT;
	ir_item->internal_type	= IRNUINT;

				
	ir_item					= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_irtype_valuetype;
	ir_item->type			= IRNUINT;
	ir_item->internal_type	= IRNUINT;
				
	ir_to_dump					= allocFunction(sizeof(ir_item_t));
	ir_to_dump->value 			= IRDATA_getSizeOfType(irLib, par)*8;
	ir_to_dump->type			= IRINT32;
	ir_to_dump->internal_type	= IRINT32;
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);
	
	return instBefore1;
}


t_ir_instruction * insert_dumper_var_pointer (ir_method_t *method, t_ir_instruction *beforeInst, ir_item_t *par){

	t_ir_instruction	*instBefore1;
	ir_item_t		*ir_item;
	ir_item_t		*ir_to_dump;
	
	
	instBefore1		= IRMETHOD_newInstructionBefore(method, beforeInst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);
	
	ir_item					= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type	= IRTYPE;
				
	ir_item					= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_var_pointer";
	ir_item->type			= IRNUINT;
	ir_item->internal_type	= IRNUINT;

				
	ir_item					= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_var_pointer;
	ir_item->type			= IRNUINT;
	ir_item->internal_type	= IRNUINT;
				
	ir_to_dump					= allocFunction(sizeof(ir_item_t));
	ir_to_dump->value 			= par->value;
	ir_to_dump->type			= IRUINT64;
	ir_to_dump->internal_type	= IRUINT64;
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);
	
	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	memcpy(ir_to_dump, par, sizeof(ir_item_t));
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);
	return instBefore1;
}

t_ir_instruction * insert_dumper_var_valuetype (ir_method_t *method, t_ir_instruction *beforeInst, ir_item_t *par){

	t_ir_instruction	*instBefore1;
	ir_item_t		*ir_item;
	ir_item_t		*ir_to_dump;
	
	
	instBefore1		= IRMETHOD_newInstructionBefore(method, beforeInst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);
	
	ir_item					= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type	= IRTYPE;
				
	ir_item					= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_var_valuetype";
	ir_item->type			= IRNUINT;
	ir_item->internal_type	= IRNUINT;

				
	ir_item					= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_var_valuetype;
	ir_item->type			= IRNUINT;
	ir_item->internal_type	= IRNUINT;
				
	ir_to_dump					= allocFunction(sizeof(ir_item_t));
	ir_to_dump->value 			= par->value;
	ir_to_dump->type			= IRUINT64;
	ir_to_dump->internal_type	= IRUINT64;
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);
	
	ir_to_dump			= allocFunction(sizeof(ir_item_t));
	//memcpy(ir_to_dump, par, sizeof(ir_item_t));
	ir_to_dump->value 			= IRDATA_getSizeOfType(irLib,par )*8;
	ir_to_dump->type		= IRINT32;
	ir_to_dump->internal_type	= IRINT32;
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);
	return instBefore1;
}

t_ir_instruction * insert_dumper_var_result_valuetype (ir_method_t *method, t_ir_instruction *beforeInst, ir_item_t *par){

	t_ir_instruction	*instBefore1;
	ir_item_t		*ir_item;
	ir_item_t		*ir_to_dump;
	
	
	instBefore1		= IRMETHOD_newInstructionAfter(method, beforeInst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);
	
	ir_item					= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type	= IRTYPE;
				
	ir_item					= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_var_valuetype";
	ir_item->type			= IRNUINT;
	ir_item->internal_type	= IRNUINT;

				
	ir_item					= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_var_valuetype;
	ir_item->type			= IRNUINT;
	ir_item->internal_type	= IRNUINT;
				
	ir_to_dump					= allocFunction(sizeof(ir_item_t));
	ir_to_dump->value 			= par->value;
	ir_to_dump->type			= IRUINT64;
	ir_to_dump->internal_type	= IRUINT64;
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);

	ir_to_dump					= allocFunction(sizeof(ir_item_t));
	//memcpy(ir_to_dump, par, sizeof(ir_item_t));
	ir_to_dump->value 			= IRDATA_getSizeOfType(irLib,par )*8;
	ir_to_dump->type		= IRINT32;
	ir_to_dump->internal_type	= IRINT32;
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);

	return instBefore1;
}







t_ir_instruction * insert_dumper_var_result_pointer (ir_method_t *method, t_ir_instruction *beforeInst, ir_item_t *par){

	t_ir_instruction	*instBefore1;
	ir_item_t		*ir_item;
	ir_item_t		*ir_to_dump;
	
	
	instBefore1		= IRMETHOD_newInstructionAfter(method, beforeInst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);
	
	ir_item					= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type	= IRTYPE;
				
	ir_item					= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_var_pointer";
	ir_item->type			= IRNUINT;
	ir_item->internal_type	= IRNUINT;

				
	ir_item					= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_var_pointer;
	ir_item->type			= IRNUINT;
	ir_item->internal_type	= IRNUINT;
				
	ir_to_dump					= allocFunction(sizeof(ir_item_t));
	ir_to_dump->value 			= par->value;
	ir_to_dump->type			= IRUINT64;
	ir_to_dump->internal_type	= IRUINT64;
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);

	ir_to_dump					= allocFunction(sizeof(ir_item_t));
	memcpy(ir_to_dump, par, sizeof(ir_item_t));

	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);
	return instBefore1;
}

t_ir_instruction * insert_dumper_par (ir_method_t *method, t_ir_instruction *inst, JITUINT32 parnum){
		
		t_ir_instruction 	*instPrintBefore;
		ir_item_t			*ir_item;
		ir_item_t			*ir_to_dump;
		
		instPrintBefore = IRMETHOD_newInstructionBefore(method, inst);
		IRMETHOD_setInstructionType(method, instPrintBefore, IRNATIVECALL);
	
		ir_item					= method->getInstrPar1(instPrintBefore);
		ir_item->value			= IRVOID;
		ir_item->type			= IRTYPE;
		ir_item->internal_type	= IRTYPE;
                                                                                              
		ir_item					= method->getInstrPar2(instPrintBefore);
		ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_before";
		ir_item->type			= IRNUINT;     //which type??????
		ir_item->internal_type	= IRNUINT;
		
		ir_item					= method->getInstrPar3(instPrintBefore);
		ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_before;
		ir_item->type			= IRNUINT;
		ir_item->internal_type	= IRNUINT;
	
		ir_to_dump			= allocFunction(sizeof(ir_item_t));
		//if (instType == IRSTORE){
		//	ir_to_dump->value  	= IRMETHOD_getInstructionParametersNumber(inst)-1;
		//}
		//else {
		//	ir_to_dump->value  	= IRMETHOD_getInstructionParametersNumber(inst);
		//}
		ir_to_dump->value 	= parnum;
		ir_to_dump->type	= IRUINT32;
		ir_to_dump->internal_type = IRUINT32;
		instPrintBefore->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
		instPrintBefore->callParameters->append(instPrintBefore->callParameters, ir_to_dump);

		return instPrintBefore;

}

t_ir_instruction * insert_dumper_irtype_64 (ir_method_t *method, t_ir_instruction *beforeInst){

	t_ir_instruction	*instBefore1;
	ir_item_t		*ir_item;
	ir_item_t		*ir_to_dump;
	
	//assert(par->internal_type == IRFLOAT64);
	
	instBefore1		= IRMETHOD_newInstructionBefore(method, beforeInst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);
	
	ir_item					= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type	= IRTYPE;
				
	ir_item					= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_irtype_64";
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;

				
	ir_item					= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_irtype_64;
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;
				
	ir_to_dump					= allocFunction(sizeof(ir_item_t));
	ir_to_dump->value 			= 64;
	ir_to_dump->type			= IRUINT32;
	ir_to_dump->internal_type	= IRUINT32;
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);
	

	return instBefore1;
}


t_ir_instruction * insert_dumper_irtype_32 (ir_method_t *method, t_ir_instruction *beforeInst){

	t_ir_instruction	*instBefore1;
	ir_item_t		*ir_item;
	ir_item_t		*ir_to_dump;
	
	//assert(par->internal_type == IRFLOAT64);
	
	instBefore1		= IRMETHOD_newInstructionBefore(method, beforeInst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);
	
	ir_item					= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type	= IRTYPE;
				
	ir_item					= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_irtype_32";
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;

				
	ir_item					= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_irtype_32;
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;
				
	ir_to_dump					= allocFunction(sizeof(ir_item_t));
	ir_to_dump->value 			= 32;
	ir_to_dump->type			= IRUINT32;
	ir_to_dump->internal_type	= IRUINT32;
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);
	

	return instBefore1;
}


t_ir_instruction * insert_dumper_irtype_16 (ir_method_t *method, t_ir_instruction *beforeInst){

	t_ir_instruction	*instBefore1;
	ir_item_t		*ir_item;
	ir_item_t		*ir_to_dump;
	
	//assert(par->internal_type == IRFLOAT64);
	
	instBefore1		= IRMETHOD_newInstructionBefore(method, beforeInst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);
	
	ir_item					= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type	= IRTYPE;
				
	ir_item					= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_irtype_16";
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;

				
	ir_item					= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_irtype_16;
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;
				
	ir_to_dump					= allocFunction(sizeof(ir_item_t));
	ir_to_dump->value 			= 16;
	ir_to_dump->type			= IRUINT32;
	ir_to_dump->internal_type	= IRUINT32;
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);
	

	return instBefore1;
}


t_ir_instruction * insert_dumper_irtype_8 (ir_method_t *method, t_ir_instruction *beforeInst){

	t_ir_instruction	*instBefore1;
	ir_item_t		*ir_item;
	ir_item_t		*ir_to_dump;
	
	//assert(par->internal_type == IRFLOAT64);
	
	instBefore1		= IRMETHOD_newInstructionBefore(method, beforeInst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);
	
	ir_item					= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type	= IRTYPE;
				
	ir_item					= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_irtype_8";
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;

				
	ir_item					= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_irtype_8;
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;
				
	ir_to_dump					= allocFunction(sizeof(ir_item_t));
	ir_to_dump->value 			= 8;
	ir_to_dump->type			= IRUINT32;
	ir_to_dump->internal_type	= IRUINT32;
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);
	

	return instBefore1;
}


t_ir_instruction * insert_dumper_result_irtype_64 (ir_method_t *method, t_ir_instruction *beforeInst){

	t_ir_instruction	*instBefore1;
	ir_item_t		*ir_item;
	ir_item_t		*ir_to_dump;
	
	//assert(par->internal_type == IRFLOAT64);
	
	instBefore1		= IRMETHOD_newInstructionAfter(method, beforeInst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);
	
	ir_item					= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type	= IRTYPE;
				
	ir_item					= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_irtype_64";
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;

				
	ir_item					= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_irtype_64;
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;
				
	ir_to_dump					= allocFunction(sizeof(ir_item_t));
	ir_to_dump->value 			= 64;
	ir_to_dump->type			= IRUINT32;
	ir_to_dump->internal_type	= IRUINT32;
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);
	

	return instBefore1;
}


t_ir_instruction * insert_dumper_result_irtype_32 (ir_method_t *method, t_ir_instruction *beforeInst){

	t_ir_instruction	*instBefore1;
	ir_item_t		*ir_item;
	ir_item_t		*ir_to_dump;
	
	//assert(par->internal_type == IRFLOAT64);
	
	instBefore1		= IRMETHOD_newInstructionAfter(method, beforeInst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);
	
	ir_item					= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type	= IRTYPE;
				
	ir_item					= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_irtype_32";
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;

				
	ir_item					= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_irtype_32;
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;
				
	ir_to_dump					= allocFunction(sizeof(ir_item_t));
	ir_to_dump->value 			= 32;
	ir_to_dump->type			= IRUINT32;
	ir_to_dump->internal_type	= IRUINT32;
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);
	

	return instBefore1;
}


t_ir_instruction * insert_dumper_result_irtype_16 (ir_method_t *method, t_ir_instruction *beforeInst){

	t_ir_instruction	*instBefore1;
	ir_item_t		*ir_item;
	ir_item_t		*ir_to_dump;
	
	//assert(par->internal_type == IRFLOAT64);
	
	instBefore1		= IRMETHOD_newInstructionAfter(method, beforeInst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);
	
	ir_item					= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type	= IRTYPE;
				
	ir_item					= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_irtype_16";
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;

				
	ir_item					= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_irtype_16;
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;
				
	ir_to_dump					= allocFunction(sizeof(ir_item_t));
	ir_to_dump->value 			= 16;
	ir_to_dump->type			= IRUINT32;
	ir_to_dump->internal_type	= IRUINT32;
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);
	

	return instBefore1;
}


t_ir_instruction * insert_dumper_result_irtype_8 (ir_method_t *method, t_ir_instruction *beforeInst){

	t_ir_instruction	*instBefore1;
	ir_item_t		*ir_item;
	ir_item_t		*ir_to_dump;
	
	//assert(par->internal_type == IRFLOAT64);
	
	instBefore1		= IRMETHOD_newInstructionAfter(method, beforeInst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);
	
	ir_item					= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type	= IRTYPE;
				
	ir_item					= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_irtype_8";
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;

				
	ir_item					= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_irtype_8;
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;
				
	ir_to_dump					= allocFunction(sizeof(ir_item_t));
	ir_to_dump->value 			= 8;
	ir_to_dump->type			= IRUINT32;
	ir_to_dump->internal_type	= IRUINT32;
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);
	

	return instBefore1;
}


t_ir_instruction * insert_dumper_result_irtype_object (ir_method_t *method, t_ir_instruction *beforeInst){

	t_ir_instruction	*instBefore1;
	ir_item_t		*ir_item;
	ir_item_t		*ir_to_dump;
	
	//assert(par->internal_type == IRFLOAT64);
	
	instBefore1		= IRMETHOD_newInstructionAfter(method, beforeInst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);
	
	ir_item					= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type	= IRTYPE;
				
	ir_item					= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_irtype_object";
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;

				
	ir_item					= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_irtype_object;
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;
				
	ir_to_dump					= allocFunction(sizeof(ir_item_t));
	ir_to_dump->value 			= sizeof(JITNUINT)*8;
	ir_to_dump->type			= IRUINT32;
	ir_to_dump->internal_type	= IRUINT32;
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);
	

	return instBefore1;
}

t_ir_instruction * insert_dumper_irtype_object (ir_method_t *method, t_ir_instruction *beforeInst){

	t_ir_instruction	*instBefore1;
	ir_item_t		*ir_item;
	ir_item_t		*ir_to_dump;
	
	//assert(par->internal_type == IRFLOAT64);
	
	instBefore1		= IRMETHOD_newInstructionBefore(method, beforeInst);
	IRMETHOD_setInstructionType(method, instBefore1, IRNATIVECALL);
	
	ir_item					= method->getInstrPar1(instBefore1);
	ir_item->value			= IRVOID;
	ir_item->type			= IRTYPE;
	ir_item->internal_type	= IRTYPE;
				
	ir_item					= method->getInstrPar2(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_irtype_object";
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;

				
	ir_item					= method->getInstrPar3(instBefore1);
	ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_irtype_object;
	ir_item->type			= IRNINT;
	ir_item->internal_type	= IRNINT;
				
	ir_to_dump					= allocFunction(sizeof(ir_item_t));
	ir_to_dump->value 			= sizeof(JITNUINT)*8;
	ir_to_dump->type			= IRUINT32;
	ir_to_dump->internal_type	= IRUINT32;
	instBefore1->callParameters =  xanListNew(allocFunction, freeFunction, NULL);
	instBefore1->callParameters->append(instBefore1->callParameters, ir_to_dump);
	

	return instBefore1;
}

void insert_dumper_parameter_1(ir_method_t *method, t_ir_instruction *inst){
	
	ir_item_t				*par1;
	t_ir_instruction 		*instPrintBefore;
	t_ir_instruction 		*instBefore1;
	
	par1 					= method->getInstrPar1(inst);

	instPrintBefore 		= insert_dumper_par(method, inst,1 );	
	//switch parameter type
	switch (par1->type){
					
		case IRINT8:
			instBefore1		= insert_dumper_int8(method, inst, par1);
			break;
		case IRINT16:
			instBefore1		= insert_dumper_int16(method, inst, par1);
			break;
		case IRINT32:
		//case IRNINT:
			instBefore1		= insert_dumper_int32(method, inst, par1);
			break;
		case IRINT64:
		case IRNINT:
			instBefore1		= insert_dumper_int64(method, inst, par1);
			break;
		case IRUINT8:
			instBefore1		= insert_dumper_uint8(method, inst, par1);
			break;
		case IRUINT16:
			instBefore1		= insert_dumper_uint16(method, inst, par1);
			break;

		case IRUINT32:
			//case IRNUINT:
			instBefore1		= insert_dumper_uint32(method, inst, par1);
			break;
			
		case IRUINT64:
		case IRNUINT:
			instBefore1		= insert_dumper_uint64(method, inst, par1);
			break;
			
		case IRFLOAT32:
			//case IRNFLOAT:
			instBefore1		= insert_dumper_float32(method, inst, par1);
			break;
			
		case IRFLOAT64:
		case IRNFLOAT:
			instBefore1		= insert_dumper_float64(method, inst, par1);
			break;
		case IRTYPE:
			switch (par1->value){
					
				case IRINT8:
				case IRUINT8:
					instBefore1		= insert_dumper_irtype_8(method, inst);
					break;
				case IRINT16:
				case IRUINT16:
					instBefore1		= insert_dumper_irtype_16(method, inst);
					break;
				case IRINT32:
			//		case IRNINT:
				case IRUINT32:
			//		case IRNUINT:
				case IRFLOAT32:
			//		case IRNFLOAT:
					instBefore1		= insert_dumper_irtype_32(method, inst);
					break;
				case IRINT64:
				case IRUINT64:
				case IRFLOAT64:
				case IRNINT:
				case IRNUINT:
				case IRNFLOAT:
					instBefore1		= insert_dumper_irtype_64(method, inst);
					break;

				case IROBJECT:
					instBefore1		= insert_dumper_irtype_object(method, inst);
					break;
				case IRVALUETYPE:
					instBefore1		= insert_dumper_irtype_valuetype(method, inst, par1);
					break;
				default: 
					break;
				}
			break;
			

		case IROFFSET:
			switch(par1->internal_type){
			
				case IRINT8:
					instBefore1	= insert_dumper_var_int8(method, inst, par1);
					break;	
				case IRINT16:
					instBefore1	= insert_dumper_var_int16(method, inst, par1);
					break;	
				case IRINT32:
				//case IRNINT:
					instBefore1	= insert_dumper_var_int32(method, inst, par1);
					break;	
				case IRINT64:
				case IRNINT:
					instBefore1	= insert_dumper_var_int64(method, inst, par1);
					break;	

				case IRUINT8:
					instBefore1	= insert_dumper_var_uint8(method, inst, par1);
					break;	
				case IRUINT16:
					instBefore1	= insert_dumper_var_uint16(method, inst, par1);
					break;	
				case IRUINT32:
				//case IRNUINT:
					instBefore1	= insert_dumper_var_uint32(method, inst, par1);
					break;	
				case IRUINT64:
				case IRNUINT:
					instBefore1	= insert_dumper_var_uint64(method, inst, par1);
					break;	
				case IRFLOAT32:
				//case IRNFLOAT:
					instBefore1	= insert_dumper_var_float32(method, inst, par1);
					break;	
			
				case IRFLOAT64:
				case IRNFLOAT:
					instBefore1	= insert_dumper_var_float64(method, inst, par1);
					break;	
				case IRMPOINTER:
				case IRFPOINTER:
				case IROBJECT:
				case IRMETHODENTRYPOINT:
					instBefore1	= insert_dumper_var_pointer (method, inst, par1);
					break;
				case IRVALUETYPE:
					instBefore1 = insert_dumper_var_valuetype (method, inst, par1);
					break;
				default:
					break;
				}
			//end switch var internal type
			break;		

		case IRLABELITEM:
			instBefore1 = insert_dumper_label(method, inst, par1);
			break;		

		case IRCLASSID:
			instBefore1 = insert_dumper_class(method, inst, par1);
			break;		

		case IRMETHODID:
			instBefore1		=insert_dumper_method (method, inst, par1);
			break;
		case IRMPOINTER:
		case IRFPOINTER:
		case IROBJECT:
		case IRMETHODENTRYPOINT:
			instBefore1		=insert_dumper_pointer (method, inst, par1);
			break;
			
		default:
			break;
		}

}


void insert_dumper_parameter_2(ir_method_t *method, t_ir_instruction *inst){
	
	ir_item_t				*par1;
	t_ir_instruction 		*instPrintBefore;
	t_ir_instruction 		*instBefore1;
	
	par1 					= method->getInstrPar2(inst);

	instPrintBefore 		= insert_dumper_par(method, inst,2 );	
	//switch parameter type
	switch (par1->type){
					
		case IRINT8:
			instBefore1		= insert_dumper_int8(method, inst, par1);
			break;
		case IRINT16:
			instBefore1		= insert_dumper_int16(method, inst, par1);
			break;
		case IRINT32:
		//case IRNINT:
			instBefore1		= insert_dumper_int32(method, inst, par1);
			break;
		case IRINT64:
		case IRNINT:
			instBefore1		= insert_dumper_int64(method, inst, par1);
			break;
		case IRUINT8:
			instBefore1		= insert_dumper_uint8(method, inst, par1);
			break;
		case IRUINT16:
			instBefore1		= insert_dumper_uint16(method, inst, par1);
			break;

		case IRUINT32:
			//case IRNUINT:
			instBefore1		= insert_dumper_uint32(method, inst, par1);
			break;
			
		case IRUINT64:
		case IRNUINT:
			instBefore1		= insert_dumper_uint64(method, inst, par1);
			break;
			
		case IRFLOAT32:
			//case IRNFLOAT:
			instBefore1		= insert_dumper_float32(method, inst, par1);
			break;
			
		case IRFLOAT64:
		case IRNFLOAT:
			instBefore1		= insert_dumper_float64(method, inst, par1);
			break;
		case IRTYPE:
			switch (par1->value){
					
				case IRINT8:
				case IRUINT8:
					instBefore1		= insert_dumper_irtype_8(method, inst);
					break;
				case IRINT16:
				case IRUINT16:
					instBefore1		= insert_dumper_irtype_16(method, inst);
					break;
				case IRINT32:
			//		case IRNINT:
				case IRUINT32:
			//		case IRNUINT:
				case IRFLOAT32:
			//		case IRNFLOAT:
					instBefore1		= insert_dumper_irtype_32(method, inst);
					break;
				case IRINT64:
				case IRUINT64:
				case IRFLOAT64:
				case IRNINT:
				case IRNUINT:
				case IRNFLOAT:
					instBefore1		= insert_dumper_irtype_64(method, inst);
					break;

				case IROBJECT:
					instBefore1		= insert_dumper_irtype_object(method, inst);
					break;
				case IRVALUETYPE:
					instBefore1		= insert_dumper_irtype_valuetype(method, inst, par1);
					break;
				default: 
					break;
				}
			break;
			

		case IROFFSET:
			switch(par1->internal_type){
			
				case IRINT8:
					instBefore1	= insert_dumper_var_int8(method, inst, par1);
					break;	
				case IRINT16:
					instBefore1	= insert_dumper_var_int16(method, inst, par1);
					break;	
				case IRINT32:
				//case IRNINT:
					instBefore1	= insert_dumper_var_int32(method, inst, par1);
					break;	
				case IRINT64:
				case IRNINT:
					instBefore1	= insert_dumper_var_int64(method, inst, par1);
					break;	

				case IRUINT8:
					instBefore1	= insert_dumper_var_uint8(method, inst, par1);
					break;	
				case IRUINT16:
					instBefore1	= insert_dumper_var_uint16(method, inst, par1);
					break;	
				case IRUINT32:
				//case IRNUINT:
					instBefore1	= insert_dumper_var_uint32(method, inst, par1);
					break;	
				case IRUINT64:
				case IRNUINT:
					instBefore1	= insert_dumper_var_uint64(method, inst, par1);
					break;	
				case IRFLOAT32:
				//case IRNFLOAT:
					instBefore1	= insert_dumper_var_float32(method, inst, par1);
					break;	
			
				case IRFLOAT64:
				case IRNFLOAT:
					instBefore1	= insert_dumper_var_float64(method, inst, par1);
					break;	
				case IRMPOINTER:
				case IRFPOINTER:
				case IROBJECT:
				case IRMETHODENTRYPOINT:
					instBefore1	= insert_dumper_var_pointer (method, inst, par1);
					break;
				case IRVALUETYPE:
					instBefore1 = insert_dumper_var_valuetype (method, inst, par1);
					break;
				default:
					break;
				}
			//end switch var internal type
			break;		

		case IRLABELITEM:
			instBefore1 = insert_dumper_label(method, inst, par1);
			break;		

		case IRCLASSID:
			instBefore1 = insert_dumper_class(method, inst, par1);
			break;		

		case IRMETHODID:
			instBefore1		=insert_dumper_method (method, inst, par1);
			break;
		case IRMPOINTER:
		case IRFPOINTER:
		case IROBJECT:
		case IRMETHODENTRYPOINT:
			instBefore1		=insert_dumper_pointer (method, inst, par1);
			break;
			
		default:
			break;
		}

}

void insert_dumper_parameter_3(ir_method_t *method, t_ir_instruction *inst){
	
	ir_item_t				*par1;
	t_ir_instruction 		*instPrintBefore;
	t_ir_instruction 		*instBefore1;
	
	par1 					= method->getInstrPar3(inst);

	instPrintBefore 		= insert_dumper_par(method, inst,3 );	
	//switch parameter type
	switch (par1->type){
					
		case IRINT8:
			instBefore1		= insert_dumper_int8(method, inst, par1);
			break;
		case IRINT16:
			instBefore1		= insert_dumper_int16(method, inst, par1);
			break;
		case IRINT32:
		//case IRNINT:
			instBefore1		= insert_dumper_int32(method, inst, par1);
			break;
		case IRINT64:
		case IRNINT:
			instBefore1		= insert_dumper_int64(method, inst, par1);
			break;
		case IRUINT8:
			instBefore1		= insert_dumper_uint8(method, inst, par1);
			break;
		case IRUINT16:
			instBefore1		= insert_dumper_uint16(method, inst, par1);
			break;

		case IRUINT32:
			//case IRNUINT:
			instBefore1		= insert_dumper_uint32(method, inst, par1);
			break;
			
		case IRUINT64:
		case IRNUINT:
			instBefore1		= insert_dumper_uint64(method, inst, par1);
			break;
			
		case IRFLOAT32:
			//case IRNFLOAT:
			instBefore1		= insert_dumper_float32(method, inst, par1);
			break;
			
		case IRFLOAT64:
		case IRNFLOAT:
			instBefore1		= insert_dumper_float64(method, inst, par1);
			break;
		case IRTYPE:
			switch (par1->value){
					
				case IRINT8:
				case IRUINT8:
					instBefore1		= insert_dumper_irtype_8(method, inst);
					break;
				case IRINT16:
				case IRUINT16:
					instBefore1		= insert_dumper_irtype_16(method, inst);
					break;
				case IRINT32:
			//		case IRNINT:
				case IRUINT32:
			//		case IRNUINT:
				case IRFLOAT32:
			//		case IRNFLOAT:
					instBefore1		= insert_dumper_irtype_32(method, inst);
					break;
				case IRINT64:
				case IRUINT64:
				case IRFLOAT64:
				case IRNINT:
				case IRNUINT:
				case IRNFLOAT:
					instBefore1		= insert_dumper_irtype_64(method, inst);
					break;

				case IROBJECT:
					instBefore1		= insert_dumper_irtype_object(method, inst);
					break;
				case IRVALUETYPE:
					instBefore1		= insert_dumper_irtype_valuetype(method, inst, par1);
					break;
				default: 
					break;
				}
			break;
			

		case IROFFSET:
			switch(par1->internal_type){
			
				case IRINT8:
					instBefore1	= insert_dumper_var_int8(method, inst, par1);
					break;	
				case IRINT16:
					instBefore1	= insert_dumper_var_int16(method, inst, par1);
					break;	
				case IRINT32:
				//case IRNINT:
					instBefore1	= insert_dumper_var_int32(method, inst, par1);
					break;	
				case IRINT64:
				case IRNINT:
					instBefore1	= insert_dumper_var_int64(method, inst, par1);
					break;	

				case IRUINT8:
					instBefore1	= insert_dumper_var_uint8(method, inst, par1);
					break;	
				case IRUINT16:
					instBefore1	= insert_dumper_var_uint16(method, inst, par1);
					break;	
				case IRUINT32:
				//case IRNUINT:
					instBefore1	= insert_dumper_var_uint32(method, inst, par1);
					break;	
				case IRUINT64:
				case IRNUINT:
					instBefore1	= insert_dumper_var_uint64(method, inst, par1);
					break;	
				case IRFLOAT32:
				//case IRNFLOAT:
					instBefore1	= insert_dumper_var_float32(method, inst, par1);
					break;	
			
				case IRFLOAT64:
				case IRNFLOAT:
					instBefore1	= insert_dumper_var_float64(method, inst, par1);
					break;	
				case IRMPOINTER:
				case IRFPOINTER:
				case IROBJECT:
				case IRMETHODENTRYPOINT:
					instBefore1	= insert_dumper_var_pointer (method, inst, par1);
					break;
				case IRVALUETYPE:
					instBefore1 = insert_dumper_var_valuetype (method, inst, par1);
					break;
				default:
					break;
				}
			//end switch var internal type
			break;		

		case IRLABELITEM:
			instBefore1 = insert_dumper_label(method, inst, par1);
			break;		

		case IRCLASSID:
			instBefore1 = insert_dumper_class(method, inst, par1);
			break;		

		case IRMETHODID:
			instBefore1		=insert_dumper_method (method, inst, par1);
			break;
		case IRMPOINTER:
		case IRFPOINTER:
		case IROBJECT:
		case IRMETHODENTRYPOINT:
			instBefore1		=insert_dumper_pointer (method, inst, par1);
			break;
			
		default:
			break;
		}

}


void insert_dumper_result(ir_method_t *method, t_ir_instruction *inst){
	ir_item_t				*inst_result;
	ir_item_t				*ir_item;
	t_ir_instruction 		*instPrintAfter;
	t_ir_instruction 		*instAfter;
	

	if ((method->getInstrResult(inst)->type != NOPARAM) && (method->getInstrResult(inst)->type != IRVOID)){
	
		inst_result			= method->getInstrResult(inst);
		//switch parameter type
		switch (inst_result->type){
			
			case IRINT8:
				instAfter		= insert_dumper_result_int8(method, inst, inst_result);
				break;
			case IRINT16:
				instAfter		= insert_dumper_result_int16(method, inst, inst_result);
				break;
			case IRINT32:
			//case IRNINT:
				instAfter		= insert_dumper_result_int32(method, inst, inst_result);
				break;
			case IRINT64:
			case IRNINT :
				instAfter		= insert_dumper_result_int64(method, inst, inst_result);
				break;

			case IRUINT8:
				instAfter		= insert_dumper_result_uint8(method, inst, inst_result);
				break;
			case IRUINT16:
				instAfter		= insert_dumper_result_uint16(method, inst, inst_result);
				break;

			case IRUINT32:
				//case IRNUINT:
				instAfter		= insert_dumper_result_uint32(method, inst, inst_result);
				break;
				
			case IRUINT64:
			case IRNUINT :
				instAfter		= insert_dumper_result_uint64(method, inst, inst_result);
				break;
			
			case IRFLOAT32:
			//case IRNFLOAT:
				instAfter		= insert_dumper_result_float32(method, inst, inst_result);
				break;
				
			case IRFLOAT64:
			case IRNFLOAT:
				instAfter		= insert_dumper_result_float64(method, inst, inst_result);
				break;
				
			case IRTYPE:
				switch (inst_result->value){
						
					case IRINT8:
					case IRUINT8:
						instAfter		= insert_dumper_result_irtype_8(method, inst);
						break;
					case IRINT16:
					case IRUINT16:
						instAfter		= insert_dumper_result_irtype_16(method, inst);
						break;
					case IRINT32:
					//case IRNINT:
					case IRUINT32:
					//case IRNUINT:
					case IRFLOAT32:
					//case IRNFLOAT:
						instAfter		= insert_dumper_result_irtype_32(method, inst);
						break;
					case IRINT64:
					case IRUINT64:
					case IRFLOAT64:
					case IRNINT:
					case IRNUINT:
					case IRNFLOAT:
						instAfter		= insert_dumper_result_irtype_64(method, inst);
						break;

					case IROBJECT:
						instAfter		= insert_dumper_result_irtype_object(method, inst);
						break;
					case IRVALUETYPE:
						instAfter		= insert_dumper_result_irtype_valuetype(method, inst, inst_result);
						break;
					default: 
						break;
					}
				break;
				
			case IROFFSET:
				switch(inst_result->internal_type){
				
					case IRINT8:
						instAfter	= insert_dumper_var_result_int8(method, inst, inst_result);
						break;	
					case IRINT16:
						instAfter	= insert_dumper_var_result_int16(method, inst, inst_result);
						break;	
					case IRINT32:
					//case IRNINT:
						instAfter	= insert_dumper_var_result_int32(method, inst, inst_result);
						break;	
					case IRINT64:
					case IRNINT :
						instAfter	= insert_dumper_var_result_int64(method, inst, inst_result);
						break;	

					case IRUINT8:
						instAfter	= insert_dumper_var_result_uint8(method, inst, inst_result);
						break;	
					case IRUINT16:
						instAfter	= insert_dumper_var_result_uint16(method, inst, inst_result);
						break;	
					case IRUINT32:
					//case IRNUINT:
						instAfter	= insert_dumper_var_result_uint32(method, inst, inst_result);
						break;	
					case IRUINT64:
					case IRNUINT :
						instAfter	= insert_dumper_var_result_uint64(method, inst, inst_result);
						break;	
					case IRFLOAT32:
					//case IRNFLOAT:
						instAfter	= insert_dumper_var_result_float32(method, inst, inst_result);
						break;	
				
					case IRFLOAT64:
					case IRNFLOAT:
						instAfter	= insert_dumper_var_result_float64(method, inst, inst_result);
						break;	
					case IRMPOINTER:
					case IRFPOINTER:
					case IROBJECT:
					case IRMETHODENTRYPOINT:
						instAfter	= insert_dumper_var_result_pointer (method, inst, inst_result);
						break;
					case IRVALUETYPE:
						instAfter = insert_dumper_var_result_valuetype (method, inst, inst_result);
						break;
					default:
						break;
					}
				//end switch var internal type
				break;		

			case IRLABELITEM:
				instAfter = insert_dumper_result_label(method, inst, inst_result);
				break;		

			case IRCLASSID:
				instAfter = insert_dumper_result_class(method, inst, inst_result);
				break;		

			case IRMETHODID:
				instAfter		=insert_dumper_result_method (method, inst, inst_result);
				break;
			case IRMPOINTER:
			case IRFPOINTER:
			case IROBJECT:
			case IRMETHODENTRYPOINT:
				instAfter		=insert_dumper_result_pointer (method, inst, inst_result);
				break;
			default:
				break;
		}
		//end of switch to print result value
		//instAfter->callParameters->destroyList(instAfter->callParameters);
	
		instPrintAfter = IRMETHOD_newInstructionAfter(method, inst);
		IRMETHOD_setInstructionType(method, instPrintAfter, IRNATIVECALL);
		ir_item				= method->getInstrPar1(instPrintAfter);
		ir_item->value			= IRVOID;
		ir_item->type			= IRTYPE;
		ir_item->internal_type		= IRTYPE;

		ir_item				= method->getInstrPar2(instPrintAfter);
		ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)"dumper_after";
		ir_item->type			= IRNUINT;
		ir_item->internal_type		= IRNUINT;
	
		ir_item				= method->getInstrPar3(instPrintAfter);
		ir_item->value			= (IR_ITEM_VALUE)(JITNUINT)dumper_after;
		ir_item->type			= IRNUINT; 
		ir_item->internal_type		= IRNUINT;
	}
}


void insert_dumper_call_parameter(ir_method_t *method, t_ir_instruction *inst, JITUINT32 call_count){
	
	ir_item_t				*par1;
	t_ir_instruction 		*instPrintBefore;
	t_ir_instruction 		*instBefore1;
	
	par1 					= IRMETHOD_getInstructionCallParameter(inst, call_count);

	instPrintBefore 		= insert_dumper_par(method, inst, 5+call_count );	
	//switch parameter type
	switch (par1->type){
					
		case IRINT8:
			instBefore1		= insert_dumper_int8(method, inst, par1);
			break;
		case IRINT16:
			instBefore1		= insert_dumper_int16(method, inst, par1);
			break;
		case IRINT32:
		//case IRNINT:
			instBefore1		= insert_dumper_int32(method, inst, par1);
			break;
		case IRINT64:
		case IRNINT:
			instBefore1		= insert_dumper_int64(method, inst, par1);
			break;
		case IRUINT8:
			instBefore1		= insert_dumper_uint8(method, inst, par1);
			break;
		case IRUINT16:
			instBefore1		= insert_dumper_uint16(method, inst, par1);
			break;

		case IRUINT32:
			//case IRNUINT:
			instBefore1		= insert_dumper_uint32(method, inst, par1);
			break;
			
		case IRUINT64:
		case IRNUINT:
			instBefore1		= insert_dumper_uint64(method, inst, par1);
			break;
			
		case IRFLOAT32:
			//case IRNFLOAT:
			instBefore1		= insert_dumper_float32(method, inst, par1);
			break;
			
		case IRFLOAT64:
		case IRNFLOAT:
			instBefore1		= insert_dumper_float64(method, inst, par1);
			break;
		case IRTYPE:
			switch (par1->value){
					
				case IRINT8:
				case IRUINT8:
					instBefore1		= insert_dumper_irtype_8(method, inst);
					break;
				case IRINT16:
				case IRUINT16:
					instBefore1		= insert_dumper_irtype_16(method, inst);
					break;
				case IRINT32:
			//		case IRNINT:
				case IRUINT32:
			//		case IRNUINT:
				case IRFLOAT32:
			//		case IRNFLOAT:
					instBefore1		= insert_dumper_irtype_32(method, inst);
					break;
				case IRINT64:
				case IRUINT64:
				case IRFLOAT64:
				case IRNINT:
				case IRNUINT:
				case IRNFLOAT:
					instBefore1		= insert_dumper_irtype_64(method, inst);
					break;

				case IROBJECT:
					instBefore1		= insert_dumper_irtype_object(method, inst);
					break;
				case IRVALUETYPE:
					instBefore1		= insert_dumper_irtype_valuetype(method, inst, par1);
					break;
				default: 
					break;
				}
			break;
			

		case IROFFSET:
			switch(par1->internal_type){
			
				case IRINT8:
					instBefore1	= insert_dumper_var_int8(method, inst, par1);
					break;	
				case IRINT16:
					instBefore1	= insert_dumper_var_int16(method, inst, par1);
					break;	
				case IRINT32:
				//case IRNINT:
					instBefore1	= insert_dumper_var_int32(method, inst, par1);
					break;	
				case IRINT64:
				case IRNINT:
					instBefore1	= insert_dumper_var_int64(method, inst, par1);
					break;	

				case IRUINT8:
					instBefore1	= insert_dumper_var_uint8(method, inst, par1);
					break;	
				case IRUINT16:
					instBefore1	= insert_dumper_var_uint16(method, inst, par1);
					break;	
				case IRUINT32:
				//case IRNUINT:
					instBefore1	= insert_dumper_var_uint32(method, inst, par1);
					break;	
				case IRUINT64:
				case IRNUINT:
					instBefore1	= insert_dumper_var_uint64(method, inst, par1);
					break;	
				case IRFLOAT32:
				//case IRNFLOAT:
					instBefore1	= insert_dumper_var_float32(method, inst, par1);
					break;	
			
				case IRFLOAT64:
				case IRNFLOAT:
					instBefore1	= insert_dumper_var_float64(method, inst, par1);
					break;	
				case IRMPOINTER:
				case IRFPOINTER:
				case IROBJECT:
				case IRMETHODENTRYPOINT:
					instBefore1	= insert_dumper_var_pointer (method, inst, par1);
					break;
				case IRVALUETYPE:
					instBefore1 = insert_dumper_var_valuetype (method, inst, par1);
					break;
				default:
					break;
				}
			//end switch var internal type
			break;		

		case IRLABELITEM:
			instBefore1 = insert_dumper_label(method, inst, par1);
			break;		

		case IRCLASSID:
			instBefore1 = insert_dumper_class(method, inst, par1);
			break;		

		case IRMETHODID:
			instBefore1		=insert_dumper_method (method, inst, par1);
			break;
		case IRMPOINTER:
		case IRFPOINTER:
		case IROBJECT:
		case IRMETHODENTRYPOINT:
			instBefore1		=insert_dumper_pointer (method, inst, par1);
			break;
			
		default:
			break;
		}

}

static inline JITUINT64 dumpcode_get_invalidations (void){
	return INVALIDATE_ALL;
}
