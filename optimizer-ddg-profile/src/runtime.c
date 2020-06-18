/*
 * Copyright (C) 2012  Campanoni Simone
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
#include <assert.h>
#include <ir_optimization_interface.h>
#include <iljit-utils.h>
#include <jitsystem.h>
#include <ir_language.h>
#include <compiler_memory_manager.h>
#include <platform_API.h>
#include <chiara.h>

// My headers
#include <optimizer_ddg_profile.h>
#include <code_injector.h>
// End

static inline void internal_print_cycles (void);

extern XanHashTable				*loopNamesTable;
extern XanHashTable				*methodNamesTable;
extern XanHashTable				*loopProfileTable;

static JITBOOLEAN			disableDumper = JITFALSE;
static BZFILE 				*internalOutputFile;

void RUNTIME_init (BZFILE *compressedOutputFile){
	internalOutputFile	= compressedOutputFile;
	disableDumper 		= JITTRUE;

	return ;
}

void dump_start_loop (JITINT8 *loopName){
	JITINT32	loopID;
	JITINT32 	errorCode;
	JITINT8         buf[DIM_BUF];

	/* Check if we need to enable the dumper.
	 */
	if (	(disableDumper)							&&
		(xanHashTable_lookup(loopProfileTable, loopName) != NULL)	){
		disableDumper	= JITFALSE;
	}

	if (disableDumper){
		return ;
	}
	internal_print_cycles();

	loopID	= (JITINT32)(JITNUINT)xanHashTable_lookup(loopNamesTable, loopName);
	assert(loopID > 0);

	SNPRINTF(buf, DIM_BUF, "LOOP %u\n", loopID);
	BZ2_bzWrite(&errorCode, internalOutputFile, buf, STRLEN(buf));
	if (errorCode != BZ_OK){
		abort();
	}

	return ;
}

void dump_call_instruction (JITUINT32 instructionID){
	JITINT32 	errorCode;
	JITINT8         buf[DIM_BUF];

	if (disableDumper){
		return ;
	}
	internal_print_cycles();

	SNPRINTF(buf, DIM_BUF, "C %u\n", instructionID);
	BZ2_bzWrite(&errorCode, internalOutputFile, buf, STRLEN(buf));
	if (errorCode != BZ_OK){
		abort();
	}

	return ;
}

void dump_os_instruction (void){
	JITINT32 	errorCode;
	JITINT8         buf[DIM_BUF];

	if (disableDumper){
		return ;
	}
	internal_print_cycles();

	SNPRINTF(buf, DIM_BUF, "OS\n");
	BZ2_bzWrite(&errorCode, internalOutputFile, buf, STRLEN(buf));
	if (errorCode != BZ_OK){
		abort();
	}

	return ;
}

void dump_end_loop (void){
	JITINT32 	errorCode;
	JITINT8         buf[DIM_BUF];

	if (disableDumper){
		return ;
	}
	internal_print_cycles();

	SNPRINTF(buf, DIM_BUF, "ENDLOOP\n");
	BZ2_bzWrite(&errorCode, internalOutputFile, buf, STRLEN(buf));
	if (errorCode != BZ_OK){
		abort();
	}

	return ;
}

void dump_start_method (char *methodName){
	JITINT32 	errorCode;
	JITINT8         buf[DIM_BUF];

	if (disableDumper){
		return ;
	}

	SNPRINTF(buf, DIM_BUF, "METHOD %u\n", (JITUINT32)(JITNUINT)xanHashTable_lookup(methodNamesTable, methodName));
	BZ2_bzWrite(&errorCode, internalOutputFile, buf, STRLEN(buf));
	if (errorCode != BZ_OK){
		abort();
	}

	return ;
}

void dump_end_method (void){
	JITINT32 	errorCode;
	JITINT8         buf[DIM_BUF];

	if (disableDumper){
		return ;
	}

	SNPRINTF(buf, DIM_BUF, "ENDMETHOD\n");
	BZ2_bzWrite(&errorCode, internalOutputFile, buf, STRLEN(buf));
	if (errorCode != BZ_OK){
		abort();
	}

	return ;
}

void dump_start_loop_iteration (void){
	JITINT32 	errorCode;
	JITINT8         buf[DIM_BUF];

	if (disableDumper){
		return ;
	}
	internal_print_cycles();

	SNPRINTF(buf, DIM_BUF, "ITER\n");
	BZ2_bzWrite(&errorCode, internalOutputFile, buf, STRLEN(buf));
	if (errorCode != BZ_OK){
		abort();
	}

	return ;
}

void dump_memory_instruction (JITUINT32 instructionPosition, void *input1, JITUINT32 input1Size, void *input2, JITUINT32 input2Size, void *output, JITUINT32 outputSize){
	JITINT32 	errorCode;
	JITINT8         buf[DIM_BUF];

	if (disableDumper){
		return ;
	}
	internal_print_cycles();

	/* Dump the instruction ID.
	 */
	SNPRINTF(buf, DIM_BUF, "M %u ", instructionPosition);
	BZ2_bzWrite(&errorCode, internalOutputFile, buf, STRLEN(buf));
	if (errorCode != BZ_OK){
		abort();
	}

	/* Dump the input memory ranges.
	 */
	if (	(input1 != NULL)	&&
		(input1Size > 0)	){
		SNPRINTF(buf, DIM_BUF, "%p - %p", input1, (void *)(((JITNUINT) input1) + input1Size - 1));
		BZ2_bzWrite(&errorCode, internalOutputFile, buf, STRLEN(buf));
		if (errorCode != BZ_OK){
			abort();
		}
	}
	if (	(input2 != NULL)	&&
		(input2Size > 0)	){
		SNPRINTF(buf, DIM_BUF, " %p - %p", input2, (void *)(((JITNUINT) input2) + input2Size - 1));
		BZ2_bzWrite(&errorCode, internalOutputFile, buf, STRLEN(buf));
		if (errorCode != BZ_OK){
			abort();
		}
	}

	/* Dump the separator between input and output.
	 */
	SNPRINTF(buf, DIM_BUF, " , ");
	BZ2_bzWrite(&errorCode, internalOutputFile, buf, STRLEN(buf));
	if (errorCode != BZ_OK){
		abort();
	}

	/* Dump the output memory range.
	 */
	if (	(output != NULL)	&&
		(outputSize > 0)	){
		SNPRINTF(buf, DIM_BUF, "%p - %p", output, (void *)(((JITNUINT) output) + outputSize - 1));
		BZ2_bzWrite(&errorCode, internalOutputFile, buf, STRLEN(buf));
		if (errorCode != BZ_OK){
			abort();
		}
	}

	/* Dump the terminator.
	 */
	SNPRINTF(buf, DIM_BUF, "\n");
	BZ2_bzWrite(&errorCode, internalOutputFile, buf, STRLEN(buf));
	if (errorCode != BZ_OK){
		abort();
	}

	return ;
}

static inline void internal_print_cycles (void){
	if (disableDumper){
		return ;
	}

	if (executedCycles > 0){
//		gzprintf(internalOutputFile, "C %llu\n", executedCycles);
		executedCycles	= 0;
	}
}
