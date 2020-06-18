/*
 * Copyright (C) 2012 Campanoni Simone
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
#ifndef ILDJIT_H
#define ILDJIT_H

#include <ir_method.h>

/**
 * \defgroup SystemAPI_Exception Exception
 * \ingroup SystemAPI
 */

/**
 * \defgroup SystemAPI_CompilationScheme Compilation scheme
 * \ingroup SystemAPI
 */

/**
 * \ingroup SystemAPI_CompilationScheme
 * @brief Get the compilation scheme currently in use
 *
 * Return the compilation scheme currently in use.
 *
 * The compilation scheme is defined by the command line (e.g., --static) and it does not change till the end of the execution.
 *
 * @return Compilation scheme currently in use
 */
compilation_scheme_t ILDJIT_compilationSchemeInUse (void);

/**
 * \ingroup SystemAPI_CompilationScheme
 * @brief Check whether a given compilation scheme is static
 *
 * If the given compilation scheme @c cs is a static compilation scheme, then JITTRUE is returned.
 * Otherwise JITFALSE is returned.
 *
 * @param cs Compilation scheme to consider
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN ILDJIT_isAStaticCompilationScheme (compilation_scheme_t cs);

/**
 * \ingroup SystemAPI_CompilationScheme
 * @brief Check whether a given compilation scheme is dynamic
 *
 * If the given compilation scheme @c cs is a dynamic compilation scheme, then JITTRUE is returned.
 * Otherwise JITFALSE is returned.
 *
 * @param cs Compilation scheme to consider
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN ILDJIT_isADynamicCompilationScheme (compilation_scheme_t cs);

/**
 * \ingroup SystemAPI_Exception
 *
 * Throw an already built exception.
 */
void * ILDJIT_getCurrentThreadException (void);

/**
 * \ingroup SystemAPI_Exception
 *
 * Throw the exception @c exceptionObject
 */
void ILDJIT_throwException (void *exceptionObject);

/**
 * \ingroup SystemAPI_Exception
 *
 * Throw an exception that is an instance of the class @c typeNameSpace.typeName
 */
void ILDJIT_throwExceptionWithName (JITINT8 *typeNameSpace, JITINT8 *typeName);

/**
 * \ingroup SystemAPI
 *
 * Shutdown the system
 */
void ILDJIT_quit (JITINT32 exitCode);

#endif
