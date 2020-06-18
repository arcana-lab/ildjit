/*
 * Copyright (C) 2008 - 2012  Campanoni Simone
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
/**
 * @file ir_optimization_interface.h
 */
#ifndef IR_OPTIMIZATION_INTERFACE_H
#define IR_OPTIMIZATION_INTERFACE_H

#include <jitsystem.h>
#include <ir_method.h>
#include <ir_optimizer.h>
#include <codetool_types.h>

/**
 * @def INVALIDATE_NONE
 * @ingroup Codetools
 * @brief Invalidate no analysis codetools
 *
 * Returning this from an optimization plugin signifies that all prior analysis is preserved and no codetools need to be rerun after this plugin.
 */
#define INVALIDATE_NONE                         0x0

/**
 * @def INVALIDATE_ALL
 * @ingroup Codetools
 * @brief Invalidate all analysis codetools
 *
 * Returning this from an optimization plugin signifies that the plugin invalidates all types of codetool.
 * If another codetool was run beforehand, it will need to be run again for its results to be valid.
 */
#define INVALIDATE_ALL                          JITMAXUINT64

/**
 * @brief Plugin methods
 *
 * This structure includes each method that each plugin has to implement.
 */
typedef struct {
    JITUINT64 	(*get_job_kind)(void);                                     	/**< Get the ID of the kind of the work that the plugin can do	*/
    JITUINT64 	(*get_dependences)(void);                                       /**< Get the ID of each plugins to call before the current	*
	                                                                                 *  The return value is a bitmap where for example, if the      *
	                                                                                 *  current method needs the liveness analysis already done	*
	                                                                                 *  before it will call, it return a bitmap where the bit	*
	                                                                                 *  0x1 << LIVENESS is set					*/

    void 		(*init)(ir_lib_t *irLib, ir_optimizer_t *optimizer, char *outputPrefix);   /**< This function is called once at startup time					*/
    void 		(*shutdown)(JITFLOAT32 totalTime);                                         /**< This function is called once during the shutdown of ILDJIT				*/
    void 		(*do_job)(ir_method_t *method);                                            /**< Call the plugin to make its optimization step					*/
    char *          (*get_version)(void);                                           /**< Return the version of the plugin							*/
    char *          (*get_information)(void);                                       /**< Return the information about the optimization step implemented by the plugin	*/
    char *          (*get_author)(void);                                            /**< Return the author name of the plugin						*/
    JITUINT64 	(*get_invalidations)(void);					/**< Return the IDs of the codetools that the current one invalidates		*/
    void		(*startExecution) (void);					/**< Start the execution of the program.			*/
    void 		(*getCompilationFlags)(char *buffer, JITINT32 bufferLength);	/**< Get the compilation flags	 */
    void 		(*getCompilationTime)(char *buffer, JITINT32 bufferLength);	/**< Get the time on when the plugin has been compiled	 */
} ir_optimization_interface_t;

#endif
