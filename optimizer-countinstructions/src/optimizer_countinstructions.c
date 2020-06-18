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

// My headers
#include <optimizer_countinstructions.h>
#include <config.h>
// End

#define INFORMATIONS 	"This plugin counts the number of static instructions of methods compiled"
#define	AUTHOR		"Simone Campanoni"

static inline JITUINT64 countinstructions_get_ID_job (void);
static inline char * countinstructions_get_version (void);
static inline void countinstructions_do_job (ir_method_t * method);
static inline char * countinstructions_get_informations (void);
static inline char * countinstructions_get_author (void);
static inline JITUINT64 countinstructions_get_dependences (void);
static inline void countinstructions_shutdown (JITFLOAT32 totalTime);
static inline void countinstructions_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix);
static inline JITUINT64 countinstructions_get_invalidations (void);

static JITUINT64 insts;

ir_optimization_interface_t plugin_interface = {
	countinstructions_get_ID_job	, 
	countinstructions_get_dependences	,
	countinstructions_init		,
	countinstructions_shutdown		,
	countinstructions_do_job		, 
	countinstructions_get_version	,
	countinstructions_get_informations	,
	countinstructions_get_author,
	countinstructions_get_invalidations
};

static inline void countinstructions_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix){
	insts = 0;
}

static inline void countinstructions_shutdown (JITFLOAT32 totalTime){
	printf("Static instructions: %llu\n", insts);
}

static inline JITUINT64 countinstructions_get_ID_job (void){
	return CUSTOM;
}

static inline JITUINT64 countinstructions_get_dependences (void){
	return 0;
}

static inline void countinstructions_do_job (ir_method_t * method){
	insts += IRMETHOD_getInstructionsNumber(method);
}

static inline char * countinstructions_get_version (void){
	return VERSION;
}

static inline char * countinstructions_get_informations (void){
	return INFORMATIONS;
}

static inline char * countinstructions_get_author (void){
	return AUTHOR;
}

static inline JITUINT64 countinstructions_get_invalidations (void){
	return INVALIDATE_ALL;
}
