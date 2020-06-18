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
/**
 * @file ir_optimization_interface.h
 */
#ifndef CLR_INTERFACE_H
#define CLR_INTERFACE_H

#include <jitsystem.h>
#include <ilmethod.h>
#include <ildjit_system.h>

#define METHOD_BEGIN(system, methodName) {										\
	if ((system->IRVM).behavior.tracer) {										\
		switch (((system->IRVM).behavior).tracerOptions){							\
			case 1:												\
				break ;											\
			default :											\
				fprintf(stderr, "Thread %ld: Enter to the method NATIVE:%s\n", gettid(), methodName);	\
		}													\
	}														\
}

#define METHOD_END(system, methodName)   { 										\
	if ((system->IRVM).behavior.tracer) {										\
		switch (((system->IRVM).behavior).tracerOptions){							\
			case 1:												\
				break ;											\
			default :											\
				fprintf(stderr, "Thread %ld: Exit from the method NATIVE:%s\n", gettid(), methodName);	\
		}													\
	}														\
}

/**
 * @brief CLR plugin interface
 *
 * This structure includes each method that each plugin has to implement.
 */
typedef struct {
    void 		(*init) (t_system *ildjitSystem);				/**< This function is called once at startup time					*/
    char *		(*getName) (void);						/**< Return the name of the plugin							*/
    JITBOOLEAN	(*buildMethod) (Method method);					/**< Map method to its implementation provided by the CLR. Return JITTRUE if it has been mapped; JITFALSE otherwise.*/
    JITBOOLEAN	(*getNameAndFunctionPointerOfMethod) (JITINT8 *signatureInString, JITINT8 **cFunctionName, void **cFunctionPointer);
    char *          (*getVersion)(void);                                           	/**< Return the version of the plugin							*/
    char *          (*getInformation)(void);                                       	/**< Return the information about the optimization step implemented by the plugin	*/
    char *          (*getAuthor)(void);                                            	/**< Return the author name of the plugin						*/
    void 		(*getCompilationFlags)(char *buffer, JITINT32 bufferLength);	/**< Get the compilation flags	 							*/
    void 		(*getCompilationTime)(char *buffer, JITINT32 bufferLength);	/**< Get the time on when the plugin has been compiled	 				*/
    void 		(*shutdown)(void);		                                /**< This function is called once during the shutdown of ILDJIT				*/
} clr_interface_t;

#endif
