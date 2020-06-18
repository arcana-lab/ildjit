/*
 * Copyright (C) 2006  Campanoni Simone
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
#ifndef LIB_REFLECT_H
#define LIB_REFLECT_H

#include <stdio.h>
#include <xanlib.h>
#include <jitsystem.h>
#include <ir_method.h>
#include <metadata/metadata_types.h>
#include <platform_API.h>

#define CLRFIELD        1
#define CLRASSEMBLY     2
#define CLRPROPERTY     3
#define CLRMETHOD       4
#define CLRCONSTRUCTOR  5
#define CLREVENT        6
#define CLRTYPE         7
#define CLRPARAMETER    8
#define CLRMODULE       9

typedef struct {
    JITINT8 type;
    void                    *ID;
    void                    *object;
} CLRMember;

typedef struct {
    JITINT8 type;
    TypeDescriptor          *ID;
    void                    *object;
} CLRType;

typedef struct {
    JITINT8 type;
    FieldDescriptor         *ID;
    void                    *object;
} CLRField;

typedef struct {
    JITINT8 type;
    BasicAssembly   *ID;
    void                    *object;
} CLRAssembly;

typedef struct {
    JITINT8 type;
    PropertyDescriptor      *ID;
    void                    *object;
} CLRProperty;

typedef struct {
    JITINT8 type;
    MethodDescriptor        *ID;
    void                    *object;
} CLRMethod;

typedef struct {
    JITINT8 type;
    MethodDescriptor                *ID;
    void                    *object;
} CLRConstructor;

typedef struct {
    JITINT8 type;
    EventDescriptor         *ID;
    void                    *object;
} CLREvent;

typedef struct {
    JITINT8 type;
    ParamDescriptor         *ID;
    void                    *object;
} CLRParameter;

typedef struct {
    JITINT8 type;
    BasicModule             *ID;
    void                    *object;
} CLRModule;

typedef struct t_reflectManager {
    TypeDescriptor          *ClrType;
    TypeDescriptor          *ClrAssembly;
    TypeDescriptor          *ClrProperty;
    TypeDescriptor          *ClrMethod;
    TypeDescriptor          *ClrConstructor;
    TypeDescriptor          *ClrField;
    TypeDescriptor          *ClrEvent;
    TypeDescriptor          *ClrParameter;
    TypeDescriptor          *ClrModule;
    FieldDescriptor         *typePrivateDataField;
    FieldDescriptor         *assemblyPrivateDataField;
    FieldDescriptor         *propertyPrivateDataField;
    FieldDescriptor         *methodPrivateDataField;
    FieldDescriptor         *constructorPrivateDataField;
    FieldDescriptor         *fieldPrivateDataField;
    FieldDescriptor         *eventPrivateDataField;
    FieldDescriptor         *parameterPrivateDataField;
    FieldDescriptor         *modulePrivateDataField;
    JITINT32 typePrivateDataFieldOffset;
    JITINT32 assemblyPrivateDataFieldOffset;
    JITINT32 propertyPrivateDataFieldOffset;
    JITINT32 methodPrivateDataFieldOffset;
    JITINT32 constructorPrivateDataFieldOffset;
    JITINT32 fieldPrivateDataFieldOffset;
    JITINT32 eventPrivateDataFieldOffset;
    JITINT32 parameterPrivateDataFieldOffset;
    JITINT32 modulePrivateDataFieldOffset;
    XanHashTable		*clrAssemblies;
    XanHashTable		*clrModule;
    XanHashTable		*clrTypes;
    XanHashTable		*clrProperties;
    XanHashTable		*clrMethods;
    XanHashTable		*clrConstructors;
    XanHashTable		*clrFields;
    XanHashTable		*clrEvents;
    XanHashTable		*clrParameter;


    void *          (*getType)(TypeDescriptor * cilType);
    void *          (*getAssembly)(BasicAssembly * cilAssembly);
    void *          (*getProperty)(PropertyDescriptor * cilProperty);
    void *          (*getMethod)(MethodDescriptor * cilMethod);
    void *          (*getConstructor)(MethodDescriptor * cilConstructor);
    void *          (*getField)(FieldDescriptor * cilMethod);
    void *          (*getEvent)(EventDescriptor * cilMethod);
    void *          (*getParameter)(ParamDescriptor * cilMethod);
    void *          (*getModule)(BasicModule * cilModule);

    ir_symbol_t *   (*getCLRTypeSymbol)(TypeDescriptor * class );

    CLRType *       (*getPrivateDataFromType)(void *object);
    CLRAssembly *   (*getPrivateDataFromAssembly)(void *object);
    CLRProperty *   (*getPrivateDataFromProperty)(void *object);
    CLRMethod *     (*getPrivateDataFromMethod)(void *object);
    CLRConstructor *(*getPrivateDataFromConstructor)(void *object);
    CLRField *      (*getPrivateDataFromField)(void *object);
    CLREvent *      (*getPrivateDataFromEvent)(void *object);
    CLRParameter *  (*getPrivateDataFromParameter)(void *object);
    CLRModule *     (*getPrivateDataFromModule)(void *object);
    void (*initialize)(void);
    void (*destroy)(struct t_reflectManager *self);
} t_reflectManager;

/*
 * Values for "System.Reflection.BindingFlags".
 */
typedef enum {
    BF_Default = 0x00000000,
    BF_IgnoreCase = 0x00000001,
    BF_DeclaredOnly = 0x00000002,
    BF_Instance = 0x00000004,
    BF_Static = 0x00000008,
    BF_Public = 0x00000010,
    BF_NonPublic = 0x00000020,
    BF_FlattenHierarchy = 0x00000040,
    BF_InvokeMethod = 0x00000100,
    BF_CreateInstance = 0x00000200,
    BF_GetField = 0x00000400,
    BF_SetField = 0x00000800,
    BF_GetProperty = 0x00001000,
    BF_SetProperty = 0x00002000,
    BF_PutDispProperty = 0x00004000,
    BF_PutRefDispProperty = 0x00008000,
    BF_ExactBinding = 0x00010000,
    BF_SuppressChangeType = 0x00020000,
    BF_OptionalParamBinding = 0x00040000,
    BF_IgnoreReturn = 0x01000000
} BindingFlags;

void init_reflectManager (t_reflectManager *reflectManager);

#endif
