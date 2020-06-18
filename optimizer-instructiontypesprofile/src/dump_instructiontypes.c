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
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <ir_optimization_interface.h>
#include <ir_language.h>
#include <iljit-utils.h>
#include <compiler_memory_manager.h>

// My headers
#include <optimizer_instructiontypes.h>
#include <dump_instructiontypes.h>
// End

static inline void internal_dump_some_instruction_types (FILE *out, JITBOOLEAN (*dumpType)(JITUINT32 type));
static inline JITBOOLEAN internal_dump_all_instructions (JITUINT32 type);
static inline JITBOOLEAN internal_dump_all_executed_instructions (JITUINT32 type);
static inline JITBOOLEAN internal_dump_math_instructions (JITUINT32 type);
static inline JITBOOLEAN internal_dump_jump_instructions (JITUINT32 type);
static inline JITBOOLEAN internal_dump_compare_instructions (JITUINT32 type);
static inline JITBOOLEAN internal_dump_memory_instructions (JITUINT32 type);
static inline JITBOOLEAN internal_dump_misc_instructions (JITUINT32 type);

void dump_all_executed_instruction_types (char *prefixToUse){
	FILE *out;
	char buf[DIM_BUF];

	/* Open the file				*/
	snprintf(buf, DIM_BUF, "%s_%s_executed_profile", prefixToUse, IRPROGRAM_getProgramName());
	out = fopen(buf, "w");
	if (out == NULL){
		fprintf(stderr, "Error opening file %s\n", buf);
		abort();
	}

	/* Dump all instructions			*/
	internal_dump_some_instruction_types(out, internal_dump_all_executed_instructions);

	/* Close the file				*/
	fclose(out);

	return ;
}

void dump_all_instruction_types (char *prefixToUse){
	FILE *out;
	char buf[DIM_BUF];

	/* Open the file				*/
	snprintf(buf, DIM_BUF, "%s_%s_profile", prefixToUse, IRPROGRAM_getProgramName());
	out = fopen(buf, "w");
	if (out == NULL){
		fprintf(stderr, "Error opening file %s\n", buf);
		abort();
	}

	/* Dump all instructions			*/
	internal_dump_some_instruction_types(out, internal_dump_all_instructions);

	/* Close the file				*/
	fclose(out);

	return ;
}

void dump_grouped_instruction_types (char *prefixToUse){
	FILE *out;
	char buf[DIM_BUF];
	JITUINT32 i;
	JITUINT64 total;
	JITUINT64 maths;
	JITUINT64 compares;
	JITUINT64 jumps;
	JITUINT64 exceptions;
	JITUINT64 memory;
	JITUINT64 convs;
	JITUINT64 misc;

	total	= 0;
	maths = 0;
	compares = 0;
	jumps = 0;
	exceptions = 0;
	memory = 0;
	misc = 0;
	convs = 0;

	snprintf(buf, DIM_BUF, "%s_%s_macroprofile", prefixToUse, IRPROGRAM_getProgramName());
	out = fopen(buf, "w");
	if (out == NULL){
		fprintf(stderr, "Error opening file %s\n", buf);
		abort();
	}
	fprintf(out, "Instructions ");
	fprintf(out, "Memory Math Compare Jump Misc Conversions Exception TOTAL\n");
	fprintf(out, "%s ", (char *)IRPROGRAM_getProgramName());
	for (i=1; i < IREXITNODE; i++){
		total	+= profiled_data[i];
		if (IRMETHOD_isAMathInstructionType(i)){
			maths += profiled_data[i];

		} else if (IRMETHOD_isACompareInstructionType(i)){
			compares += profiled_data[i];

		} else if (IRMETHOD_isAJumpInstructionType(i)){
			jumps += profiled_data[i];

		} else if (IRMETHOD_isAnExceptionInstructionType(i)){
			exceptions += profiled_data[i];

		} else if (IRMETHOD_isAConversionInstructionType(i)){
			convs += profiled_data[i];

		} else if (	(IRMETHOD_isAMemoryAllocationInstructionType(i))	||
				(IRMETHOD_isAMemoryInstructionType(i))			){
			memory += profiled_data[i];

		} else {
			misc += profiled_data[i];
		}
	}
	fprintf(out, "%llu ", memory);
	fprintf(out, "%llu ", maths);
	fprintf(out, "%llu ", compares);
	fprintf(out, "%llu ", jumps);
	fprintf(out, "%llu ", misc);
	fprintf(out, "%llu ", convs);
	fprintf(out, "%llu ", exceptions);
	fprintf(out, "%llu\n", total);
	fclose(out);

	/* Dump memory instructions		*/
	snprintf(buf, DIM_BUF, "%s_%s_memory_profile", prefixToUse, IRPROGRAM_getProgramName());
	out = fopen(buf, "w");
	if (out == NULL){
		fprintf(stderr, "Error opening file %s\n", buf);
		abort();
	}
	internal_dump_some_instruction_types(out, internal_dump_memory_instructions);
	fclose(out);

	/* Dump math instructions		*/
	snprintf(buf, DIM_BUF, "%s_%s_math_profile", prefixToUse, IRPROGRAM_getProgramName());
	out = fopen(buf, "w");
	if (out == NULL){
		fprintf(stderr, "Error opening file %s\n", buf);
		abort();
	}
	internal_dump_some_instruction_types(out, internal_dump_math_instructions);
	fclose(out);

	/* Dump compare instructions		*/
	snprintf(buf, DIM_BUF, "%s_%s_compare_profile", prefixToUse, IRPROGRAM_getProgramName());
	out = fopen(buf, "w");
	if (out == NULL){
		fprintf(stderr, "Error opening file %s\n", buf);
		abort();
	}
	internal_dump_some_instruction_types(out, internal_dump_compare_instructions);
	fclose(out);

	/* Dump jump instructions		*/
	snprintf(buf, DIM_BUF, "%s_%s_jump_profile", prefixToUse, IRPROGRAM_getProgramName());
	out = fopen(buf, "w");
	if (out == NULL){
		fprintf(stderr, "Error opening file %s\n", buf);
		abort();
	}
	internal_dump_some_instruction_types(out, internal_dump_jump_instructions);
	fclose(out);

	/* Dump misc instructions		*/
	snprintf(buf, DIM_BUF, "%s_%s_misc_profile", prefixToUse, IRPROGRAM_getProgramName());
	out = fopen(buf, "w");
	if (out == NULL){
		fprintf(stderr, "Error opening file %s\n", buf);
		abort();
	}
	internal_dump_some_instruction_types(out, internal_dump_misc_instructions);
	fclose(out);

	/* Dump branch instructions		*/
	dump_branch_call_instruction_types(prefixToUse);

	/* Dump memory instructions		*/
	dump_load_store_instruction_types(prefixToUse);

	return ;
}

void dump_load_store_instruction_types (char *prefixToUse){
	FILE *out;
	char buf[DIM_BUF];
	JITUINT32 i;
	JITUINT64 total;
	JITUINT64 loads;
	JITUINT64 stores;

	/* Initialize the variables		*/
	total			= 0;
	loads			= 0;
	stores			= 0;

	/* Open the file			*/
	snprintf(buf, DIM_BUF, "%s_%s_load_store_profile", prefixToUse, IRPROGRAM_getProgramName());
	out = fopen(buf, "w");
	if (out == NULL){
		fprintf(stderr, "Error opening file %s\n", buf);
		abort();
	}

	/* Dump the header of the file		*/
	fprintf(out, "Instructions loads stores TOTAL\n");
	fprintf(out, "%s ", (char *)IRPROGRAM_getProgramName());

	/* Dump the instruction types		*/
	for (i=1; i < IREXITNODE; i++){
		if (IRMETHOD_isAMemoryInstructionType(i)){
			switch (i){
				case IRLOADREL:
	    	    case IRARRAYLENGTH:
					loads		+= profiled_data[i];
					break;
			    case IRSTOREREL:
			    case IRINITMEMORY:
	    		case IRMEMCPY:
					stores		+= profiled_data[i];
					break;
				default:
					abort();
			}
			total += profiled_data[i];
		}
	}

	/* Dump the footnote of the file	*/
	fprintf(out, "%llu %llu %llu\n", loads, stores, total);

	/* Close the file			*/
	fclose(out);

	return ;
}

void dump_branch_call_instruction_types (char *prefixToUse){
	FILE *out;
	char buf[DIM_BUF];
	JITUINT32 i;
	JITUINT64 total;
	JITUINT64 uncond_branches;
	JITUINT64 cond_branches;
	JITUINT64 direct_calls;
	JITUINT64 indirect_calls;

	/* Initialize the variables		*/
	total			= 0;
	uncond_branches		= 0;
	cond_branches		= 0;
	direct_calls		= 0;
	indirect_calls		= 0;

	/* Open the file			*/
	snprintf(buf, DIM_BUF, "%s_%s_branch_vs_call_profile", prefixToUse, IRPROGRAM_getProgramName());
	out = fopen(buf, "w");
	if (out == NULL){
		fprintf(stderr, "Error opening file %s\n", buf);
		abort();
	}

	/* Dump the header of the file		*/
	fprintf(out, "Instructions \"unconditional branches\" \"conditional branches\" \"direct calls\" \"indirect calls\" TOTAL\n");
	fprintf(out, "%s ", (char *)IRPROGRAM_getProgramName());

	/* Dump the instruction types		*/
	for (i=1; i < IREXITNODE; i++){
		if (IRMETHOD_isAJumpInstructionType(i)){
			switch (i){
				case IRBRANCH:
	    			case IRRET:
	    			case IREXIT:
	    			case IRCALLFINALLY:
	    			case IRENDFINALLY:
					uncond_branches	+= profiled_data[i];
					break;
			    	case IRBRANCHIF:
			    	case IRBRANCHIFNOT:
	    			case IRBRANCHIFPCNOTINRANGE:
					cond_branches	+= profiled_data[i];
					break;
	    			case IRCALL:
	    			case IRLIBRARYCALL:
	    			case IRNATIVECALL:
					direct_calls	+= profiled_data[i];
					break;
	    			case IRVCALL:
	    			case IRICALL:
					indirect_calls	+= profiled_data[i];
					break;
	    			case IRSTARTFINALLY:
					break;
				default:
					abort();
			}
			total += profiled_data[i];
		}
	}

	/* Dump the footnote of the file	*/
	fprintf(out, "%llu %llu %llu %llu %llu\n", uncond_branches, cond_branches, direct_calls, indirect_calls, total);

	/* Close the file			*/
	fclose(out);

	return ;
}

static inline void internal_dump_some_instruction_types (FILE *out, JITBOOLEAN (*dumpType)(JITUINT32 type)){
	JITUINT64 total;
	JITUINT32 i;

	/* Dump all instructions		*/
	total	= 0;
	fprintf(out, "Instructions ");
	for (i=1; i < IREXITNODE; i++){
		if ((*dumpType)(i)){
			fprintf(out, "\"%s\" ", IRMETHOD_getInstructionTypeName(i));
		}
	}
	fprintf(out, "TOTAL\n");
	fprintf(out, "%s ", (char *)IRPROGRAM_getProgramName());
	for (i=1; i < IREXITNODE; i++){
		if ((*dumpType)(i)){
			fprintf(out, "%llu ", profiled_data[i]);
			total += profiled_data[i];
		}
	}
	fprintf(out, "%llu\n", total);

	return ;
}

static inline JITBOOLEAN internal_dump_all_executed_instructions (JITUINT32 type){
	if (profiled_data[type] > 0){
		return JITTRUE;
	}
	return JITFALSE;
}

static inline JITBOOLEAN internal_dump_all_instructions (JITUINT32 type){
	return JITTRUE;
}

static inline JITBOOLEAN internal_dump_compare_instructions (JITUINT32 type){
	return IRMETHOD_isACompareInstructionType(type);
}

static inline JITBOOLEAN internal_dump_jump_instructions (JITUINT32 type){
	return IRMETHOD_isAJumpInstructionType(type);
}

static inline JITBOOLEAN internal_dump_math_instructions (JITUINT32 type){
	return IRMETHOD_isAMathInstructionType(type);
}

static inline JITBOOLEAN internal_dump_memory_instructions (JITUINT32 type){
	return IRMETHOD_isAMemoryAllocationInstructionType(type) || IRMETHOD_isAMemoryInstructionType(type);
}

static inline JITBOOLEAN internal_dump_misc_instructions (JITUINT32 type){
	if (	(!IRMETHOD_isAMemoryAllocationInstructionType(type))		&&
		(!IRMETHOD_isAMemoryInstructionType(type))			&&
		(!IRMETHOD_isAMathInstructionType(type))			&&
		(!IRMETHOD_isACompareInstructionType(type))			&&
		(!IRMETHOD_isAJumpInstructionType(type))			&&
		(!IRMETHOD_isAConversionInstructionType(type))			&&
		(!IRMETHOD_isAnExceptionInstructionType(type))			){
		return JITTRUE;
	}
	return JITFALSE;
}
