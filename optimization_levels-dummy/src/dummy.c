/*
 * Copyright (C) 2011  Campanoni Simone
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
#include <ir_optimization_levels_interface.h>
#include <ir_language.h>
#include <iljit-utils.h>
#include <compiler_memory_manager.h>

// My headers
#include <dummy.h>
#include <config.h>
// End

#define INFORMATIONS 	"This is a dummy plugin for the definition of the optimization levels of ILDJIT"
#define	AUTHOR		"Campanoni Simone"

static inline JITUINT32 dummy_optimizeMethodAtLevel (ir_optimizer_t *lib, ir_method_t *method, JITUINT32 optimizationLevel, JITUINT32 state, XanVar *checkPoint);
static inline JITUINT32 dummy_optimizeMethodAOT (ir_optimizer_t *lib, ir_method_t *startMethod, JITUINT32 state, XanVar *checkPoint);
static inline JITUINT32 dummy_optimizeMethodPostAOT (ir_optimizer_t *lib, ir_method_t *method, JITUINT32 state, XanVar *checkPoint);
static inline void dummy_init (ir_lib_t *lib, ir_optimizer_t *optimizer, JITINT8 *outputPrefix);
static inline void dummy_shutdown (JITFLOAT32 totalTime);
static inline char * dummy_get_version (void);
static inline char * dummy_get_information (void);
static inline char * dummy_get_author (void);

ir_lib_t	*irLib		= NULL;
ir_optimizer_t	*irOptimizer	= NULL;
JITINT8	        *prefix		= NULL;

ir_optimization_levels_interface_t plugin_interface = {
	dummy_optimizeMethodAtLevel	,
	dummy_optimizeMethodAOT		,
	dummy_optimizeMethodPostAOT 	,
	dummy_init			,
	dummy_shutdown			,
	dummy_get_version		,
	dummy_get_information		,
	dummy_get_author		,
};

static inline void dummy_init (ir_lib_t *lib, ir_optimizer_t *optimizer, JITINT8 *outputPrefix){

	/* Assertions			*/
	assert(lib != NULL);
	assert(optimizer != NULL);
	assert(outputPrefix != NULL);

	irLib		= lib;
	irOptimizer	= optimizer;
	prefix		= outputPrefix;
}

static inline void dummy_shutdown (JITFLOAT32 totalTime){
        irLib		= NULL;
	irOptimizer	= NULL;
	prefix		= NULL;
}

static inline char * dummy_get_version (void){
	return VERSION;
}

static inline char * dummy_get_information (void){
	return INFORMATIONS;
}

static inline char * dummy_get_author (void){
	return AUTHOR;
}

static inline JITUINT32 dummy_optimizeMethodAtLevel (ir_optimizer_t *lib, ir_method_t *method, JITUINT32 optimizationLevel, JITUINT32 state, XanVar *checkPoint){
	 return JOB_END;
}

static inline JITUINT32 dummy_optimizeMethodAOT (ir_optimizer_t *lib, ir_method_t *startMethod, JITUINT32 state, XanVar *checkPoint){
	 return JOB_END;
}

static inline JITUINT32 dummy_optimizeMethodPostAOT (ir_optimizer_t *lib, ir_method_t *method, JITUINT32 state, XanVar *checkPoint){
	 return JOB_END;
}
