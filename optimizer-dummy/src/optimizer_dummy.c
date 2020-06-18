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
#include <assert.h>
#include <ir_optimization_interface.h>
#include <iljit-utils.h>
#include <jitsystem.h>
#include <ir_language.h>
#include <compiler_memory_manager.h>
#include <platform_API.h>

// My headers
#include <optimizer_dummy.h>
#include <config.h>
// End

#define INFORMATIONS 	"This is a dummy plugin"
#define	AUTHOR		"Simone Campanoni"
#define JOB            	CUSTOM

static inline void dummy_start_execution (void);
static inline JITUINT64 dummy_get_ID_job (void);
static inline char * dummy_get_version (void);
static inline void dummy_do_job (ir_method_t * method);
static inline char * dummy_get_informations (void);
static inline char * dummy_get_author (void);
static inline JITUINT64 dummy_get_dependences (void);
static inline void dummy_shutdown (JITFLOAT32 totalTime);
static inline void dummy_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix);
static inline JITUINT64 dummy_get_invalidations (void);
static inline void dummy_getCompilationTime (char *buffer, JITINT32 bufferLength);
static inline void dummy_getCompilationFlags (char *buffer, JITINT32 bufferLength);

ir_lib_t	*irLib		= NULL;
ir_optimizer_t	*irOptimizer	= NULL;
char 	        *prefix		= NULL;

ir_optimization_interface_t plugin_interface = {
	dummy_get_ID_job	, 
	dummy_get_dependences	,
	dummy_init		,
	dummy_shutdown		,
	dummy_do_job		, 
	dummy_get_version	,
	dummy_get_informations	,
	dummy_get_author	,
	dummy_get_invalidations	,
	dummy_start_execution	,
	dummy_getCompilationFlags,
	dummy_getCompilationTime
};

static inline void dummy_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix){

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

static inline JITUINT64 dummy_get_ID_job (void){
	return JOB;
}

static inline JITUINT64 dummy_get_dependences (void){
	return 0;
}

static inline void dummy_do_job (ir_method_t * method){

	/* Assertions				*/
	assert(method != NULL);

	/* Return				*/
	return;
}

static inline char * dummy_get_version (void){
	return VERSION;
}

static inline char * dummy_get_informations (void){
	return INFORMATIONS;
}

static inline char * dummy_get_author (void){
	return AUTHOR;
}

static inline JITUINT64 dummy_get_invalidations (void){
	return INVALIDATE_ALL;
}

static inline void dummy_start_execution (void){
	return ;
}

static inline void dummy_getCompilationFlags (char *buffer, JITINT32 bufferLength){

	/* Assertions				*/
	assert(buffer != NULL);

	snprintf(buffer, sizeof(char) * bufferLength, " ");
	#ifdef DEBUG
	strncat(buffer, "DEBUG ", sizeof(char) * bufferLength);
	#endif
	#ifdef PRINTDEBUG
	strncat(buffer, "PRINTDEBUG ", sizeof(char) * bufferLength);
	#endif
	#ifdef PROFILE
	strncat(buffer, "PROFILE ", sizeof(char) * bufferLength);
	#endif
}

static inline void dummy_getCompilationTime (char *buffer, JITINT32 bufferLength){

	/* Assertions				*/
	assert(buffer != NULL);

	snprintf(buffer, sizeof(char) * bufferLength, "%s %s", __DATE__, __TIME__);

	/* Return				*/
	return;
}
