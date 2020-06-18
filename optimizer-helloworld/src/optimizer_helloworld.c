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
#include <optimizer_helloworld.h>
#include <config.h>
// End

#define INFORMATIONS 	"This is the HelloWorld plugin"
#define	AUTHOR		"Campanoni Simone"

static inline JITUINT64 helloworld_get_ID_job (void);
static inline char * helloworld_get_version (void);
static inline void helloworld_do_job (ir_method_t * method);
static inline char * helloworld_get_informations (void);
static inline char * helloworld_get_author (void);
static inline JITUINT64 helloworld_get_dependences (void);
static inline void helloworld_shutdown (JITFLOAT32 totalTime);
static inline void helloworld_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix);
static inline JITUINT64 helloworld_get_invalidations (void);

ir_optimization_interface_t plugin_interface = {
	helloworld_get_ID_job	, 
	helloworld_get_dependences	,
	helloworld_init		,
	helloworld_shutdown		,
	helloworld_do_job		, 
	helloworld_get_version	,
	helloworld_get_informations	,
	helloworld_get_author,
	helloworld_get_invalidations
};

static inline void helloworld_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix){
	printf("Hello world! init\n");
}

static inline void helloworld_shutdown (JITFLOAT32 totalTime){
	printf("Hello world! shutdown\n");
}

static inline JITUINT64 helloworld_get_ID_job (void){
	return CUSTOM;
}

static inline JITUINT64 helloworld_get_dependences (void){
	return 0;
}

static inline void helloworld_do_job (ir_method_t * method){
	printf("Hello world! do_job\n");
}

static inline char * helloworld_get_version (void){
	return VERSION;
}

static inline char * helloworld_get_informations (void){
	return INFORMATIONS;
}

static inline char * helloworld_get_author (void){
	return AUTHOR;
}

static inline JITUINT64 helloworld_get_invalidations (void){
	return INVALIDATE_ALL;
}
