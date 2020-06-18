/*
 * Copyright (C) 2010 - 2012  Campanoni Simone
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
#ifndef IR_OPTIMIZATION_LEVELS_INTERFACE_H
#define IR_OPTIMIZATION_LEVELS_INTERFACE_H

#include <jitsystem.h>
#include <ir_method.h>
#include <ir_optimizer.h>

/**
 * @brief Definition of IR optimization levels
 *
 */
typedef struct {
    JITUINT32 	(*optimizeMethodAtLevel)(ir_optimizer_t *lib, ir_method_t *method, JITUINT32 optimizationLevel, JITUINT32 state, XanVar *checkPoint);
    JITUINT32 	(*optimizeMethodAOT)(ir_optimizer_t *lib, ir_method_t *startMethod, JITUINT32 state, XanVar *checkPoint);
    JITUINT32 	(*optimizeMethodPostAOT)(ir_optimizer_t *lib, ir_method_t *method, JITUINT32 state, XanVar *checkPoint);
    void 		(*init)(ir_optimizer_t *optimizer, JITINT8 *outputPrefix); 	              	/**< This function is called once at startup time					*/
    void 		(*shutdown)(JITFLOAT32 totalTime);                                             	/**< This function is called once during the shutdown of ILDJIT				*/
    char *          (*getVersion)(void);                                                            /**< Return the version of the plugin							*/
    char *          (*getInformation)(void);                                                        /**< Return the information about the optimization step implemented by the plugin	*/
    char *          (*getAuthor)(void);                                                             /**< Return the name of the author of the plugin					*/
    char *          (*getName)(void);                                                               /**< Return the name of the optimizationleveltool					*/
    void 		(*getCompilationFlags)(char *buffer, JITINT32 bufferLength);			/**< Get the compilation flags	 */
    void 		(*getCompilationTime)(char *buffer, JITINT32 bufferLength);			/**< Get the time on when the plugin has been compiled	 */
} ir_optimization_levels_interface_t;

#endif
