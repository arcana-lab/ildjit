/*
 * Copyright (C) 2008  Campanoni Simone
 *
 * iljit - This is a Just-in-time for the CIL language specified with the ECMA-335
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
#ifndef INTERNAL_CALLS_UTILITIES_H
#define INTERNAL_CALLS_UTILITIES_H

#include <jitsystem.h>
#include <ilmethod.h>

// My headers
#include <ildjit_system.h>
#include <clr_interface.h>
// End

#define LOAD_ERROR_OK                           0
#define LOAD_ERROR_INVALID_NAME                 1
#define LOAD_ERROR_FILE_NOT_FOUND               2
#define LOAD_ERROR_BAD_IMAGE                    3
#define LOAD_ERROR_SECURITY                     4

MethodDescriptor *ILMethodSemGetByType (CLRMember *clrMember, JITINT32 type);
void * InvokeMethod (Method method, void *_this, void *parametersArray);
JITINT32 reflectionStringCmp (JITINT8 *name1, JITINT8 *name2, JITBOOLEAN ignoreCase);
void * GetTypeName (void *object, JITINT16 fullyQualified);
void * GetTypeNamespace (void *object);

#endif
