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
#ifndef INTERNAL_CALLS_REFLECTION_H
#define INTERNAL_CALLS_REFLECTION_H

#include <jitsystem.h>
#include <lib_reflect.h>
/****************************************************************************************************************************
*                                               REFLECTION								    *
* if a method has to return an IntPtr, in PnetLib implementation, it return a CLRMember * ,in ildjit implementation,	    *
* otherwise when the return type is a Class in PnetLib implementation it return a void* in ildjit			    *
****************************************************************************************************************************/
void *          System_Reflection_Assembly_GetExecutingAssembly (void);
void *          System_Reflection_Assembly_GetEntryAssembly (void);
void *          System_Reflection_Assembly_GetEntryPoint (void *_this);
void *          System_Reflection_Assembly_GetCallingAssembly (void);
void *          System_Reflection_Assembly_GetFullName (void *object);
void *          System_Reflection_Assembly_GetManifestResourceStream (void *_this, void *stringName);
void *          System_Reflection_Assembly_GetExportedTypes (void *_this);
void *          System_Reflection_Assembly_GetType (void *_this, void *stringName, JITBOOLEAN throwOnError, JITBOOLEAN ignoreCase);
void *          System_Reflection_Assembly_GetTypes (void *_this);
void *          System_Reflection_Assembly_LoadFromName (void *name, JITINT32 * error, void *object);
void *          System_Reflection_Assembly_LoadFromFile (void *name, JITINT32 * error, void *object);
void *          System_Reflection_Assembly_GetSatellitePath (void *_this, void *fileNameString);
void *          System_Reflection_Assembly_GetModuleInternal (void *_this, void *nameString);

void *          System_Reflection_ClrType_GetClrGUID (void *_this);
void *          System_Reflection_ClrType_GetClrModule (void *_this);
void *          System_Reflection_ClrType_GetClrNestedDeclaringType (void *_this);
void *          System_Reflection_ClrType_GetClrAssembly (void *_this);
JITBOOLEAN              System_Reflection_ClrType_IsClrNestedType (void *_this);
void *          System_Reflection_ClrType_GetClrBaseType (void *_this);
void *          System_Reflection_ClrType_GetClrName (void *object);
void *          System_Reflection_ClrType_GetClrNamespace (void *object);
void *          System_Reflection_ClrType_GetElementType (void *object);
JITBOOLEAN      System_Reflection_ClrType_IsSubclassOf (void *object, void *otherObject);
void *          System_Reflection_ClrType_GetClrFullName (void *object);
void *          System_Reflection_ClrType_GetInterfaces (void *object);
JITINT32        System_Reflection_ClrType_GetClrArrayRank (void *_this);
JITINT32        System_Reflection_ClrType_GetAttributeFlagsImpl (void *_this);
void *          System_Reflection_ClrType_GetMemberImpl (void *_this, void *name, JITINT32 memberTypes, JITINT32 bindingAttrs, void *binder, JITINT32 callConv, void *typesArray, void *modifiersArray);
void *          System_Reflection_ClrType_GetMembersImpl (void *_this, JITINT32 memberTypes, JITINT32 bindingAttrs, void *arrayType, void *nameString);

void *          System_Reflection_ClrMethod_Invoke (void *_this, void *object, JITINT32 invokeAttr, void *binder, void *parametersArray, void *culture);
void *          System_Reflection_ClrMethod_GetBaseDefinition (void *_this);

void *          System_Reflection_ClrConstructor_Invoke (void *_this, JITINT32 invokeAttr, void *binder, void *parametersArray, void *culture);

void *          System_Reflection_ClrField_GetValueInternal (void *_this, void *object);
void *          System_Reflection_ClrField_GetFieldType (JITNINT item); //add

void *          System_Reflection_ClrHelpers_GetCustomAttributes (JITNINT item, JITNINT type, JITBOOLEAN inherit);
void * 		System_Reflection_ClrHelpers_GetDeclaringType (JITNINT item);
void *          System_Reflection_ClrHelpers_GetName (JITNINT item);
void *          System_Reflection_ClrHelpers_GetParameterType (JITNINT item, JITINT32 num);
JITINT32        System_Reflection_ClrHelpers_GetNumParameters (JITNINT item);
JITINT32        System_Reflection_ClrHelpers_GetMemberAttrs (JITNINT item);
JITINT32        System_Reflection_ClrHelpers_GetCallConv (JITNINT item);
JITINT32        System_Reflection_ClrHelpers_GetImplAttrs (JITNINT item);
void *          System_Reflection_ClrHelpers_GetSemantics (JITNINT itemPrivate, JITINT32 type, JITBOOLEAN nonPublic);
JITBOOLEAN              System_Reflection_ClrHelpers_HasSemantics (JITNINT item, JITINT32 type, JITBOOLEAN nonPublic);
JITBOOLEAN              System_Reflection_ClrHelpers_CanAccess (JITNINT item);

void *          System_Reflection_Module_GetType (void *_this, void *name, JITBOOLEAN throwOnError, JITBOOLEAN ignoreCase);
void *          System_Reflection_Module_GetTypes (void *_this);
void *          System_Reflection_Module_GetModuleType (void *_this);
void *          System_Reflection_Module_GetAssembly (void *_this);

void *          System_Reflection_ClrParameter_GetParamAttrs (JITNINT item);
void *          System_Reflection_ClrParameter_GetParamName (JITNINT item);

void *          System_Reflection_ClrProperty_GetPropertyType (JITNINT item);

JITINT32        System_Reflection_ClrResourceStream_ResourceRead (JITNINT handle, JITINT64 position, void * buffer, JITINT32 offset, JITINT32 count);

void *          System_Reflection_MethodBase_GetMethodFromHandle (void *handle);
void *          System_Reflection_MethodBase_GetCurrentMethod (void);
void *		System_RuntimeMethodHandle_GetFunctionPointer (void *runtimeMethodHandle);

/****************************************************************************************************************************
*                                               TYPE									    *
****************************************************************************************************************************/
void *          System_Type_GetTypeFromHandle (void *handle);
void *          System_Type_GetType (void *nameString, JITBOOLEAN throwOnError, JITBOOLEAN ignoreCase);

/****************************************************************************************************************************
*                                               MODULE 									    *
****************************************************************************************************************************/
void 		System_ModuleHandle_ResolveMethodHandle_Int32 (void *clrModule, JITINT32 token, void *runtimeMethodHandle);

#endif
