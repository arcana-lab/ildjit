/*
 * Copyright (C) 2009 - 2012  Simone Campanoni, Luca Rocchini
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
#ifndef METADATA_TYPES_H
#define METADATA_TYPES_H

#include <xanlib.h>
#include <decoder.h>
#include <metadata_table_manager.h>
#include <basic_types.h>
#include <jitsystem.h>
#include <ir_language.h>
#include <ecma_constant.h>
#include <platform_API.h>

typedef struct t_generic_type_checking_rules {
    JITBOOLEAN (*is_IR_ReferenceType)(struct t_generic_type_checking_rules *rules, JITUINT32 typeID);
    JITBOOLEAN (*is_IR_ObjectType)(struct t_generic_type_checking_rules *rules, JITUINT32 typeID);
    JITBOOLEAN (*is_IR_ManagedPointer)(struct t_generic_type_checking_rules *rules, JITUINT32 typeID);
    JITBOOLEAN (*is_IR_UnmanagedPointer)(struct t_generic_type_checking_rules *rules, JITUINT32 typeID);
    JITBOOLEAN (*is_IR_TransientPointer)(struct t_generic_type_checking_rules *rules, JITUINT32 typeID);
    JITBOOLEAN (*is_IR_ValueType)(struct t_generic_type_checking_rules *rules, JITUINT32 typeID);
    JITBOOLEAN (*is_IR_ValidType)(struct t_generic_type_checking_rules *rules, JITUINT32 typeID);
    JITUINT32 (*get_return_IRType_For_Binary_Numeric_operation)(JITUINT32 param1, JITUINT32 param2, JITUINT32 irOP, JITBOOLEAN *verified_code);
    JITUINT32 (*get_return_IRType_For_Shift_operation)(JITUINT32 param1, JITUINT32 param2, JITUINT32 irOp);
    JITUINT32 (*get_return_IRType_For_Unary_Numeric_Operation)(JITUINT32 param);
    JITUINT32 (*get_return_IRType_For_Binary_Comparison)(JITUINT32 param1, JITUINT32 param2, JITUINT32 irOp, JITBOOLEAN *verifiable_code);
    JITUINT32 (*get_return_IRType_For_Overflow_Arythmetic_operation)(JITUINT32 param1, JITUINT32 param2, JITUINT32 irOp, JITBOOLEAN *verified_code);
    JITUINT32 (*get_return_IRType_For_Integer_operation)(JITUINT32 param1, JITUINT32 param2);
    JITUINT32 (*get_conversion_infos_For_Conv_operation)(JITUINT32 fromType, JITUINT32 toType, JITBOOLEAN *verified_code);
    JITUINT32 (*coerce_CLI_Type)(JITUINT32 type_to_coerce);
    JITBOOLEAN (*is_CLI_Type)(JITUINT32 type_to_coerce);
    JITBOOLEAN (*is_unsigned_IRType)(JITUINT32 type);
    JITUINT32 (*to_unsigned_IRType)(JITUINT32 type);
} t_generic_type_checking_rules;

#define GET_BINARY(descriptor) ((t_binary_information *) descriptor->row->binary);

typedef struct _GenericContainer {
    JITUINT32 arg_count;
    ContainerType container_type;
    struct _GenericContainer *parent;
    struct _TypeDescriptor **paramType;
} GenericContainer;
JITUINT32 hashGenericContainer (struct _GenericContainer *container);
JITINT32 equalsGenericContainer (struct _GenericContainer *key1, struct _GenericContainer *key2);

typedef struct _GenericVarDescriptor {
    struct metadataManager_t *manager;
    void *tickets;
    pthread_mutex_t mutex;
    void (*lock)(struct _GenericVarDescriptor *type);
    void (*unlock)(struct _GenericVarDescriptor *type);

    DescriptorMetadata metadata;
    DescriptorMetadataTicket * (*createDescriptorMetadataTicket)(struct _GenericVarDescriptor *var, JITUINT32 type);
    void (*broadcastDescriptorMetadataTicket)(struct _GenericVarDescriptor *var, DescriptorMetadataTicket *ticket, void *data);

    struct _TypeDescriptor *descriptor;
    struct _TypeDescriptor * (*getTypeDescriptor)(struct _GenericVarDescriptor *classDescriptor );
    struct _TypeDescriptor * (*getInstance)(struct _GenericVarDescriptor *type, struct _GenericContainer *container);

    BasicGenericParam *row;
    struct _TypeDescriptor *owner;

    BasicGenericParamAttributes *attributes;

    JITINT8 *name;
    JITINT8 * (*getName)(struct _GenericVarDescriptor *var);

    XanList *constraints;
    XanList *(*getConstraints)(struct _GenericVarDescriptor *var);

    struct _ClassDescriptor * (*getReferenceClass)(struct _GenericVarDescriptor *type);
    JITUINT32 (*getIRType)(struct _GenericVarDescriptor *type);
} GenericVarDescriptor;
JITUINT32 hashGenericVarDescriptor (void *element);
JITINT32 equalsGenericVarDescriptor (void *key1, void *key2);

typedef struct _GenericMethodVarDescriptor {
    struct metadataManager_t *manager;
    void *tickets;
    pthread_mutex_t mutex;
    void (*lock)(struct _GenericMethodVarDescriptor *type);
    void (*unlock)(struct _GenericMethodVarDescriptor *type);

    DescriptorMetadata metadata;
    DescriptorMetadataTicket * (*createDescriptorMetadataTicket)(struct _GenericMethodVarDescriptor *var, JITUINT32 type);
    void (*broadcastDescriptorMetadataTicket)(struct _GenericMethodVarDescriptor *var, DescriptorMetadataTicket *ticket, void *data);

    struct _TypeDescriptor *descriptor;
    struct _TypeDescriptor * (*getTypeDescriptor)(struct _GenericMethodVarDescriptor *classDescriptor );
    struct _TypeDescriptor * (*getInstance)(struct _GenericMethodVarDescriptor *classDescriptor, struct _GenericContainer *container);

    BasicGenericParam *row;
    struct _MethodDescriptor *owner;

    BasicGenericParamAttributes *attributes;

    JITINT8 *name;
    JITINT8 * (*getName)(struct _GenericMethodVarDescriptor *var);

    XanList *constraints;
    XanList *(*getConstraints)(struct _GenericMethodVarDescriptor *var);

    struct _ClassDescriptor * (*getReferenceClass)(struct _GenericMethodVarDescriptor *type);
    JITUINT32 (*getIRType)(struct _GenericMethodVarDescriptor *type);

} GenericMethodVarDescriptor;
JITUINT32 hashGenericMethodVarDescriptor (void *element);
JITINT32 equalsGenericMethodVarDescriptor (void *key1, void *key2);

typedef struct _EventDescriptor {
    struct metadataManager_t *manager;
    void *tickets;
    pthread_mutex_t mutex;
    void (*lock)(struct _EventDescriptor *event);
    void (*unlock)(struct _EventDescriptor *event);

    DescriptorMetadata metadata;
    DescriptorMetadataTicket * (*createDescriptorMetadataTicket)(struct _EventDescriptor *event, JITUINT32 type);
    void (*broadcastDescriptorMetadataTicket)(struct _EventDescriptor *event, DescriptorMetadataTicket *ticket, void *data);

    BasicEvent *row;
    struct _TypeDescriptor *owner;

    BasicEventAttributes *attributes;

    struct _TypeDescriptor *type;
    struct _TypeDescriptor * (*getType)(struct _EventDescriptor *property);

    JITINT8 *name;
    JITINT8 *completeName;
    JITINT8 *(*getName)(struct _EventDescriptor *property);
    JITINT8 *(*getCompleteName)(struct _EventDescriptor *property);

    struct _MethodDescriptor *addon;
    struct _MethodDescriptor * (*getAddOn)(struct _EventDescriptor *property);

    struct _MethodDescriptor *removeon;
    struct _MethodDescriptor * (*getRemoveOn)(struct _EventDescriptor *property);

    struct _MethodDescriptor *fire;
    struct _MethodDescriptor * (*getFire)(struct _EventDescriptor *property);

    struct _MethodDescriptor *other;
    struct _MethodDescriptor * (*getOther)(struct _EventDescriptor *property);

} EventDescriptor;
JITUINT32 hashEventDescriptor (void *element);
JITINT32 equalsEventDescriptor (void *key1, void *key2);


typedef struct _PropertyDescriptor {
    struct metadataManager_t *manager;
    void *tickets;
    pthread_mutex_t mutex;
    void (*lock)(struct _PropertyDescriptor *property);
    void (*unlock)(struct _PropertyDescriptor *property);

    DescriptorMetadata metadata;
    DescriptorMetadataTicket * (*createDescriptorMetadataTicket)(struct _PropertyDescriptor *property, JITUINT32 type);
    void (*broadcastDescriptorMetadataTicket)(struct _PropertyDescriptor *property, DescriptorMetadataTicket *ticket, void *data);

    BasicProperty *row;
    struct _TypeDescriptor *owner;

    BasicPropertyAttributes *attributes;

    struct _TypeDescriptor *type;
    struct _TypeDescriptor * (*getType)(struct _PropertyDescriptor *property);

    JITINT8 *name;
    JITINT8 *completeName;
    JITINT8 *(*getName)(struct _PropertyDescriptor *property);
    JITINT8 *(*getCompleteName)(struct _PropertyDescriptor *property);

    struct _MethodDescriptor *other;
    struct _MethodDescriptor * (*getOther)(struct _PropertyDescriptor *property);

    struct _MethodDescriptor *getter;
    struct _MethodDescriptor * (*getGetter)(struct _PropertyDescriptor *property);

    struct _MethodDescriptor *setter;
    struct _MethodDescriptor * (*getSetter)(struct _PropertyDescriptor *property);

} PropertyDescriptor;
JITUINT32 hashPropertyDescriptor (void *element);
JITINT32 equalsPropertyDescriptor (void *key1, void *key2);

typedef struct  _TypeElem {
    struct _TypeDescriptor *type;
    JITBOOLEAN value;
    struct _TypeElem *next;
} TypeElem;

typedef struct _TypeDescriptor {
    struct  _TypeElem		*compatibility;
    struct metadataManager_t	*manager;
    void				*tickets;
    pthread_mutex_t			mutex;
    ILElemType			element_type;
    JITBOOLEAN			isByRef;
    void				*descriptor;
    struct _BasicTypeAttributes	*attributes;
    struct _TypeDescriptor		*superType;
    void				*vTable;
    JITINT8				*name;
    JITINT8				*completeName;

    void				(*lock)					(struct _TypeDescriptor *type);
    void				(*unlock)				(struct _TypeDescriptor *type);
    DescriptorMetadataTicket *	(*createDescriptorMetadataTicket)	(struct _TypeDescriptor *type_desc, JITUINT32 type);
    void				(*broadcastDescriptorMetadataTicket)	(struct _TypeDescriptor *type_desc, DescriptorMetadataTicket *ticket, void *data);
    struct _GenericContainer *	(*getResolvingContainer)		(struct _TypeDescriptor *type);
    JITBOOLEAN			(*isValueType)				(struct _TypeDescriptor *type);
    JITBOOLEAN			(*isEnum)				(struct _TypeDescriptor *type);
    JITBOOLEAN			(*isDelegate)				(struct _TypeDescriptor *type);
    JITBOOLEAN			(*isNullable)				(struct _TypeDescriptor *type);
    JITBOOLEAN			(*isPrimitive)				(struct _TypeDescriptor *type);
    struct _TypeDescriptor *	(*getSuperType)				(struct _TypeDescriptor *type);
    JITBOOLEAN			(*isSubtypeOf)				(struct _TypeDescriptor *type, struct _TypeDescriptor *superType);
    JITBOOLEAN			(*assignableTo)				(struct _TypeDescriptor *type_to_cast, struct _TypeDescriptor *desired_type);
    XanHashTable *			(*getVTable)				(struct _TypeDescriptor *type, JITUINT32 *vTableLength);
    XanList *			(*getImplementedInterfaces)		(struct _TypeDescriptor *type);
    struct _TypeDescriptor *	(*getEnclosing)				(struct _TypeDescriptor *type);
    XanList *			(*getNestedType)			(struct _TypeDescriptor *type);
    struct _TypeDescriptor *	(*getNestedTypeFromName)		(struct _TypeDescriptor *type, JITINT8 *name);
    struct _MethodDescriptor *	(*getCctor)				(struct _TypeDescriptor *type);
    struct _MethodDescriptor *	(*getFinalizer)				(struct _TypeDescriptor *type);
    XanList *			(*getConstructors)			(struct _TypeDescriptor *type);
    XanList *			(*getDirectDeclaredMethods)		(struct _TypeDescriptor *type);
    XanList *			(*getMethodFromName)			(struct _TypeDescriptor *type, JITINT8 *name);
    struct _MethodDescriptor *	(*implementsMethod)			(struct _TypeDescriptor *type, struct _MethodDescriptor *method);
    XanList *			(*getDirectImplementedFields)		(struct _TypeDescriptor *type);
    struct _FieldDescriptor *	(*getFieldFromName)			(struct _TypeDescriptor *type, JITINT8 *name);
    XanList *			(*getEvents)				(struct _TypeDescriptor *Type);
    struct _EventDescriptor *	(*getEventFromName)			(struct _TypeDescriptor *Type, JITINT8 *name);
    XanList *			(*getProperties)			(struct _TypeDescriptor *Type);
    struct _PropertyDescriptor *	(*getPropertyFromName)			(struct _TypeDescriptor *Type, JITINT8 *name);
    JITUINT32			(*getIRType)				(struct _TypeDescriptor *type);
    JITINT8 *			(*getName)				(struct _TypeDescriptor *type);
    JITINT8 *			(*getCompleteName)			(struct _TypeDescriptor *type);
    JITINT8 *			(*getTypeNameSpace)			(struct _TypeDescriptor *type);
    BasicAssembly *			(*getAssembly)				(struct _TypeDescriptor *type);
    struct _TypeDescriptor *	(*makeArrayDescriptor)			(struct _TypeDescriptor *type, JITUINT32 rank);
    struct _TypeDescriptor *	(*makePointerDescriptor)		(struct _TypeDescriptor *type);
    struct _TypeDescriptor *	(*makeByRef)				(struct _TypeDescriptor *type);
    JITBOOLEAN			(*isGenericTypeDefinition)		(struct _TypeDescriptor *Type);
    JITBOOLEAN			(*containOpenGenericParameter)		(struct _TypeDescriptor *Type);
    JITBOOLEAN			(*isGeneric)				(struct _TypeDescriptor *Type);
    struct _TypeDescriptor *	(*getGenericDefinition)			(struct _TypeDescriptor *type);
    struct _TypeDescriptor *	(*getInstance)				(struct _TypeDescriptor *type, struct _GenericContainer *container);
    XanList *			(*getGenericParameters)			(struct _TypeDescriptor *type);
    struct _ClassDescriptor *	(*getUnderliningClass)			(struct _TypeDescriptor *type);
} TypeDescriptor;

JITUINT32 hashTypeDescriptor (struct _TypeDescriptor *element);
JITINT32 equalsTypeDescriptor (struct _TypeDescriptor *key1, struct _TypeDescriptor *key2);
#define GET_FNPTR(type) ((FunctionPointerDescriptor *) (type->descriptor))
#define GET_PTR(type) ((PointerDescriptor *) (type->descriptor))
#define GET_ARRAY(type) ((ArrayDescriptor *) (type->descriptor))
#define GET_CLASS(type) ((ClassDescriptor *) (type->descriptor))

#define IS_FNPTR_DESCRIPTOR(type)       (type->element_type == ELEMENT_FNPTR)
#define IS_PTR_DESCRIPTOR(type) (type->element_type == ELEMENT_PTR)
#define IS_ARRAY_DESCRIPTOR(type)       (type->element_type == ELEMENT_ARRAY)
#define IS_CLASS_DESCRIPTOR(type)       (type->element_type == ELEMENT_CLASS)

typedef struct _PointerDescriptor {
    struct metadataManager_t *manager;
    void *tickets;
    pthread_mutex_t mutex;
    void (*lock)(struct _PointerDescriptor *type);
    void (*unlock)(struct _PointerDescriptor *type);

    DescriptorMetadata metadata;
    DescriptorMetadataTicket * (*createDescriptorMetadataTicket)(struct _PointerDescriptor *pointer, JITUINT32 type);
    void (*broadcastDescriptorMetadataTicket)(struct _PointerDescriptor *pointer, DescriptorMetadataTicket *ticket, void *data);

    struct _TypeDescriptor *descriptor;
    struct _TypeDescriptor * (*getTypeDescriptor)(struct _PointerDescriptor *classDescriptor );
    struct _TypeDescriptor *byRefDescriptor;
    struct _TypeDescriptor * (*getByRefTypeDescriptor)(struct _PointerDescriptor *classDescriptor  );

    struct _TypeDescriptor *type;

    JITINT8 *name;
    JITINT8 *completeName;
    JITINT8 *(*getName)(struct _PointerDescriptor *type);
    JITINT8 *(*getCompleteName)(struct _PointerDescriptor *type);

    JITBOOLEAN (*containOpenGenericParameter)(struct _PointerDescriptor *pointer);
    struct _TypeDescriptor * (*getGenericDefinition)(struct _PointerDescriptor *type);
    struct _TypeDescriptor * (*getInstance)(struct _PointerDescriptor *type, struct _GenericContainer *container);
} PointerDescriptor;
JITUINT32 hashPointerDescriptor (struct _PointerDescriptor *pointer);
JITINT32 equalsPointerDescriptor (struct _PointerDescriptor *key1, struct _PointerDescriptor *key2);

typedef struct _FunctionPointerDescriptor {
    struct metadataManager_t * manager;
    void *tickets;
    pthread_mutex_t mutex;
    void (*lock)(struct _FunctionPointerDescriptor *type);
    void (*unlock)(struct _FunctionPointerDescriptor *type);

    DescriptorMetadata metadata;
    DescriptorMetadataTicket * (*createDescriptorMetadataTicket)(struct _FunctionPointerDescriptor *pointer, JITUINT32 type);
    void (*broadcastDescriptorMetadataTicket)(struct _FunctionPointerDescriptor *pointer, DescriptorMetadataTicket *ticket, void *data);

    struct _TypeDescriptor *descriptor;
    struct _TypeDescriptor * (*getTypeDescriptor)(struct _FunctionPointerDescriptor *classDescriptor  );
    struct _TypeDescriptor *byRefDescriptor;
    struct _TypeDescriptor * (*getByRefTypeDescriptor)(struct _FunctionPointerDescriptor *classDescriptor  );

    JITBOOLEAN hasThis;
    JITBOOLEAN explicitThis;
    JITBOOLEAN hasSentinel;
    JITUINT32 sentinel_index;
    JITUINT32 calling_convention;
    JITUINT32 generic_param_count;

    struct _TypeDescriptor  *result;
    XanList *params;

    JITINT8 *signatureInString;
    JITINT8 *(*getSignatureInString)(struct _FunctionPointerDescriptor *pointer);

    JITBOOLEAN (*containOpenGenericParameter)(struct _FunctionPointerDescriptor *pointer);
    struct _TypeDescriptor * (*getGenericDefinition)(struct _FunctionPointerDescriptor *type);
    struct _TypeDescriptor * (*getInstance)(struct _FunctionPointerDescriptor *type, struct _GenericContainer *container);
} FunctionPointerDescriptor;
JITUINT32 hashFunctionPointerDescriptor (struct _FunctionPointerDescriptor *pointer);
JITINT32 equalsFunctionPointerDescriptor (struct _FunctionPointerDescriptor *key1, struct _FunctionPointerDescriptor *key2);

typedef struct _ArrayDescriptor {
    struct metadataManager_t *manager;
    void *tickets;
    pthread_mutex_t mutex;
    void (*lock)(struct _ArrayDescriptor *classDescriptor  );
    void (*unlock)(struct _ArrayDescriptor *classDescriptor  );

    DescriptorMetadata metadata;
    DescriptorMetadataTicket * (*createDescriptorMetadataTicket)(struct _ArrayDescriptor *array, JITUINT32 type);
    void (*broadcastDescriptorMetadataTicket)(struct _ArrayDescriptor *array, DescriptorMetadataTicket *ticket, void *data);

    struct _TypeDescriptor *descriptor;
    struct _TypeDescriptor * (*getTypeDescriptor)(struct _ArrayDescriptor *type);
    struct _TypeDescriptor *byRefDescriptor;
    struct _TypeDescriptor * (*getByRefTypeDescriptor)(struct _ArrayDescriptor *type);


    struct _TypeDescriptor *type;
    JITUINT32 rank;
    JITUINT32 numSizes;
    JITUINT32       *sizes;
    JITUINT32 numBounds;
    JITUINT32       *bounds;

    JITINT8 *name;
    JITINT8 *completeName;
    JITINT8 *typeNameSpace;
    JITINT8 *(*getName)(struct _ArrayDescriptor *type);
    JITINT8 *(*getCompleteName)(struct _ArrayDescriptor *type);
    JITINT8 *(*getTypeNameSpace)(struct _ArrayDescriptor *type);

    JITBOOLEAN (*containOpenGenericParameter)(struct _ArrayDescriptor *array);
    struct _TypeDescriptor * (*getGenericDefinition)(struct _ArrayDescriptor *type);
    struct _TypeDescriptor * (*getInstance)(struct _ArrayDescriptor *type, struct _GenericContainer *container);
} ArrayDescriptor;
JITUINT32 hashArrayDescriptor (struct _ArrayDescriptor *array);
JITINT32 equalsArrayDescriptor (struct _ArrayDescriptor *key1, struct _ArrayDescriptor *key2);

typedef struct _VTableSlot {
    struct _MethodDescriptor *declaration;
    struct _MethodDescriptor *body;
    struct _VTableSlot *overriden;
    JITBOOLEAN explicitSlot;
    JITUINT32 index;
} VTableSlot;

typedef struct _ClassDescriptor {
    struct metadataManager_t *manager;
    void *tickets;
    pthread_mutex_t mutex;
    void (*lock)(struct _ClassDescriptor *classDescriptor);
    void (*unlock)(struct _ClassDescriptor *classDescriptor);

    DescriptorMetadata metadata;
    DescriptorMetadataTicket * (*createDescriptorMetadataTicket)(struct _ClassDescriptor *classDescriptor, JITUINT32 type);
    void (*broadcastDescriptorMetadataTicket)(struct _ClassDescriptor *classDescriptor, DescriptorMetadataTicket *ticket, void *data);

    struct _TypeDescriptor *descriptor;
    struct _TypeDescriptor * (*getTypeDescriptor)(struct _ClassDescriptor *classDescriptor);
    struct _TypeDescriptor *byRefDescriptor;
    struct _TypeDescriptor * (*getByRefTypeDescriptor)(struct _ClassDescriptor *classDescriptor);

    BasicClass *row;
    GenericContainer *container;

    GenericContainer *resolving_container;
    GenericContainer * (*getResolvingContainer)(struct _ClassDescriptor *classDescriptor);

    struct _BasicTypeAttributes *attributes;

    struct _TypeDescriptor *superClass;
    struct _TypeDescriptor * (*getSuperClass)(struct _ClassDescriptor *classDescriptor);

    XanList *interfaces;
    XanList * (*getImplementedInterfaces)(struct _ClassDescriptor *classDescriptor);

    struct _TypeDescriptor *enclosing_type;
    struct _TypeDescriptor * (*getEnclosing)(struct _ClassDescriptor        *classDescriptor);
    XanList *nestedClass;
    XanHashTable *nestedClassMap;
    XanList *(*getNestedClass)(struct _ClassDescriptor      *classDescriptor);
    struct _TypeDescriptor *(*getNestedClassFromName)(struct _ClassDescriptor       *classDescriptor, JITINT8 *name);

    XanList *directMethods;
    XanHashTable *methodsMap;
    struct _MethodDescriptor *cctor;
    struct _MethodDescriptor *finalizer;
    XanList *constructors;
    struct _MethodDescriptor *(*getCctor)(struct _ClassDescriptor *type);
    struct _MethodDescriptor *(*getFinalizer)(struct _ClassDescriptor *type);
    XanList *(*getConstructors)(struct _ClassDescriptor *type);
    XanList *(*getDirectDeclaredMethods)(struct _ClassDescriptor *type);
    XanList * (*getMethodFromName)(struct _ClassDescriptor *type, JITINT8 * name);

    XanList *directFields;
    XanHashTable *fieldsMap;
    XanList *(*getDirectImplementedFields)(struct _ClassDescriptor *type);
    struct _FieldDescriptor * (*getFieldFromName)(struct _ClassDescriptor *type, JITINT8 * name);

    JITBOOLEAN (*isValueType)(struct _ClassDescriptor *type);
    JITBOOLEAN (*isEnum)(struct _ClassDescriptor *type);
    JITBOOLEAN (*isDelegate)(struct _ClassDescriptor *type);
    JITBOOLEAN (*isNullable)(struct _ClassDescriptor *type);
    JITBOOLEAN (*isPrimitive)(struct _ClassDescriptor *type);

    JITINT8 *name;
    JITINT8 *typeNameSpace;
    JITINT8 *completeName;
    JITINT8 *(*getName)(struct _ClassDescriptor *type);
    JITINT8 *(*getCompleteName)(struct _ClassDescriptor *type);
    JITINT8 *(*getTypeNameSpace)(struct _ClassDescriptor *type);

    JITBOOLEAN (*implementsInterface)(struct _ClassDescriptor *type, struct _ClassDescriptor *interfaceDescription);
    JITBOOLEAN (*extendsType)(struct _ClassDescriptor *type, struct _ClassDescriptor *supertype);
    JITBOOLEAN (*isSubtypeOf)(struct _ClassDescriptor *type, struct _ClassDescriptor *supertype);
    JITBOOLEAN (*assignableTo)(struct _ClassDescriptor *type_to_cast, struct _ClassDescriptor *desired_type);

    XanList *events;
    XanHashTable *eventsMap;
    XanList * (*getEvents)(struct _ClassDescriptor *classDescriptor);
    struct _EventDescriptor *(*getEventFromName)(struct _ClassDescriptor *classDescriptor, JITINT8 * name);

    XanList *properties;
    XanHashTable *propertiesMap;
    XanList * (*getProperties)(struct _ClassDescriptor *classDescriptor);
    struct _PropertyDescriptor *(*getPropertyFromName)(struct _ClassDescriptor *classDescriptor, JITINT8 * name);

    JITBOOLEAN (*isGenericClassDefinition)(struct _ClassDescriptor *classDescriptor);
    JITBOOLEAN (*containOpenGenericParameter)(struct _ClassDescriptor *classDescriptor);
    JITBOOLEAN (*isGeneric)(struct _ClassDescriptor *classDescriptor);
    struct _TypeDescriptor * (*getGenericDefinition)(struct _ClassDescriptor *classDescriptor);
    struct _TypeDescriptor * (*getInstance)(struct _ClassDescriptor *classDescriptor, GenericContainer *container);

    BasicAssembly *assembly;
    BasicAssembly *(*getAssembly)(struct _ClassDescriptor *classDescriptor);

    JITUINT32 (*getIRType)(struct _ClassDescriptor *type);

    XanList *genericParameters;
    XanList * (*getGenericParameters)(struct _ClassDescriptor *type);
} ClassDescriptor;
JITUINT32 hashClassDescriptor (struct _ClassDescriptor *element);
JITINT32 equalsClassDescriptor (struct _ClassDescriptor *key1, struct _ClassDescriptor *key2);


typedef struct _ParamDescriptor {
    void *tickets;
    pthread_mutex_t mutex;
    void (*lock)(struct _ParamDescriptor *type);
    void (*unlock)(struct _ParamDescriptor *type);

    DescriptorMetadata metadata;
    DescriptorMetadataTicket * (*createDescriptorMetadataTicket)(struct _ParamDescriptor *param, JITUINT32 type);
    void (*broadcastDescriptorMetadataTicket)(struct _ParamDescriptor *param, DescriptorMetadataTicket *ticket, void *data);

    struct _TypeDescriptor *type;

    BasicParam *row;
    BasicParamAttributes *attributes;

    JITINT8 *name;
    JITINT8* (*getName)(struct _ParamDescriptor *param);

    JITINT8 *signatureInString;
    JITINT8* (*getSignatureInString)(struct _ParamDescriptor *param);

    JITUINT32 (*getIRType)(struct _ParamDescriptor *type);
} ParamDescriptor;

typedef struct _LocalDescriptor {
    JITBOOLEAN is_pinned;
    struct _TypeDescriptor *type;

    DescriptorMetadata metadata;
    DescriptorMetadataTicket * (*createDescriptorMetadataTicket)(struct _LocalDescriptor *local, JITUINT32 type);
    void (*broadcastDescriptorMetadataTicket)(struct _LocalDescriptor *local, DescriptorMetadataTicket *ticket, void *data);

    JITUINT32 (*getIRType)(struct _LocalDescriptor *type);
} LocalDescriptor;

typedef struct _ActualMethodDescriptor {
    struct _MethodDescriptor *method;
    struct _FunctionPointerDescriptor *actualSignature;
} ActualMethodDescriptor;

typedef struct _MethodDescriptor {
    struct metadataManager_t *manager;
    void *tickets;
    pthread_mutex_t mutex;
    void (*lock)(struct _MethodDescriptor *method);
    void (*unlock)(struct _MethodDescriptor *method);

    DescriptorMetadata metadata;
    DescriptorMetadataTicket * (*createDescriptorMetadataTicket)(struct _MethodDescriptor *method, JITUINT32 type);
    void (*broadcastDescriptorMetadataTicket)(struct _MethodDescriptor *method, DescriptorMetadataTicket *ticket, void *data);

    BasicMethod *row;
    struct _TypeDescriptor *owner;
    GenericContainer *container;

    GenericContainer *resolving_container;
    GenericContainer * (*getResolvingContainer)(struct _MethodDescriptor *method);

    BasicMethodAttributes *attributes;

    JITBOOLEAN (*requireConservativeOwnerInitialization)(struct _MethodDescriptor *method);

    struct _FunctionPointerDescriptor *signature;
    struct _FunctionPointerDescriptor *(*getFormalSignature)(struct _MethodDescriptor *method);

    XanList * params;
    XanList * (*getParams)(struct _MethodDescriptor *method);
    ParamDescriptor *result;
    struct _ParamDescriptor * (*getResult)(struct _MethodDescriptor *method);

    JITINT8 *name;
    JITINT8 *completeName;
    JITINT8 *signatureInString;
    JITINT8 *internalName;
    JITINT8 *(*getName)(struct _MethodDescriptor *type);
    JITINT8 *(*getCompleteName)(struct _MethodDescriptor *type);
    JITINT8 *(*getSignatureInString)(struct _MethodDescriptor *type);
    JITINT8 *(*getInternalName)(struct _MethodDescriptor *type);

    JITUINT32 (*getOverloadingSequenceID)(struct _MethodDescriptor *type);

    struct _MethodDescriptor *(*getBaseDefinition)(struct _MethodDescriptor *method);

    JITBOOLEAN (*isCallable)(struct _MethodDescriptor *callee, struct _MethodDescriptor *caller);

    JITBOOLEAN (*isCtor)(struct _MethodDescriptor *method);
    JITBOOLEAN (*isCctor)(struct _MethodDescriptor *method);
    JITBOOLEAN (*isFinalizer)(struct _MethodDescriptor *method);

    JITINT8 *importName;
    JITINT8 *(*getImportName)(struct _MethodDescriptor *method);
    JITINT8 *importModule;
    JITINT8 *(*getImportModule)(struct _MethodDescriptor *method);

    struct _PropertyDescriptor *property;
    struct _EventDescriptor *event;
    struct _PropertyDescriptor *(*getProperty)(struct _MethodDescriptor *method);
    struct _EventDescriptor *(*getEvent)(struct _MethodDescriptor *method);

    JITBOOLEAN (*isGenericMethodDefinition)(struct _MethodDescriptor *method);
    JITBOOLEAN (*containOpenGenericParameter)(struct _MethodDescriptor *method);
    JITBOOLEAN (*isGeneric)(struct _MethodDescriptor *method);
    struct _MethodDescriptor *(*getInstance)(struct _MethodDescriptor *method, GenericContainer *container);
    struct _MethodDescriptor * (*getGenericDefinition)(struct _MethodDescriptor *method);

    XanList *genericParameters;
    XanList * (*getGenericParameters)(struct _MethodDescriptor *type);
} MethodDescriptor;

JITUINT32 hashMethodDescriptor (struct _MethodDescriptor *element);
JITINT32 equalsMethodDescriptor (struct _MethodDescriptor *key1, struct _MethodDescriptor *key2);

typedef struct _FieldDescriptor {
    struct metadataManager_t *manager;
    void *tickets;
    pthread_mutex_t mutex;
    void (*lock)(struct _FieldDescriptor *field);
    void (*unlock)(struct _FieldDescriptor *field);

    DescriptorMetadata metadata;
    DescriptorMetadataTicket * (*createDescriptorMetadataTicket)(struct _FieldDescriptor *field, JITUINT32 type);
    void (*broadcastDescriptorMetadataTicket)(struct _FieldDescriptor *field, DescriptorMetadataTicket *ticket, void *data);

    BasicField *row;
    struct _TypeDescriptor *owner;

    BasicFieldAttributes *attributes;

    struct _TypeDescriptor *type;
    struct _TypeDescriptor * (*getType)(struct _FieldDescriptor *field);

    JITINT8 *name;
    JITINT8 *completeName;
    JITINT8 *(*getName)(struct _FieldDescriptor *type);
    JITINT8 *(*getCompleteName)(struct _FieldDescriptor *type);

    JITBOOLEAN (*isAccessible)(struct _FieldDescriptor *field, struct _MethodDescriptor *Caller);
} FieldDescriptor;
JITUINT32 hashFieldDescriptor (void *element);
JITINT32 equalsFieldDescriptor (void *key1, void *key2);


typedef struct metadataManager_t {

    XanHashTable *resolvedTokens;
    XanHashTable *binaryDB;

    XanHashTable *genericVarDescriptors;
    XanHashTable *genericMethodVarDescriptors;
    XanHashTable *propertyDescriptors;
    XanHashTable *eventDescriptors;
    XanHashTable *classDescriptors;
    XanHashTable *pointerDescriptors;
    XanHashTable *arrayDescriptors;
    XanHashTable *functionPointerDescriptors;

    XanHashTable *fieldDescriptors;
    XanHashTable *methodDescriptors;

    XanList *binaries;

    /* temporary Result List */
    XanList *emptyList;

    t_generic_type_checking_rules *generic_type_rules;

    BasicTypeAttributes arrayAttributes;

    struct _ClassDescriptor         *System_Delegate;
    struct _ClassDescriptor         *System_Nullable;
    struct _ClassDescriptor         *System_TypedReference;
    struct _ClassDescriptor         *System_Enum;
    struct _ClassDescriptor         *System_ValueType;
    struct _ClassDescriptor         *System_Void;
    struct _ClassDescriptor         *System_Boolean;
    struct _ClassDescriptor         *System_IntPtr;
    struct _ClassDescriptor         *System_UIntPtr;
    struct _ClassDescriptor         *System_SByte;
    struct _ClassDescriptor         *System_Byte;
    struct _ClassDescriptor         *System_Int16;
    struct _ClassDescriptor         *System_Int32;
    struct _ClassDescriptor         *System_Int64;
    struct _ClassDescriptor         *System_UInt16;
    struct _ClassDescriptor         *System_UInt32;
    struct _ClassDescriptor         *System_UInt64;
    struct _ClassDescriptor         *System_Single;
    struct _ClassDescriptor         *System_Double;
    struct _ClassDescriptor             *System_Char;
    struct _ClassDescriptor             *System_String;
    struct _ClassDescriptor             *System_Array;
    struct _ClassDescriptor             *System_Object;

    struct _TypeDescriptor *(*getTypeDescriptorFromNameAndAssembly)(struct metadataManager_t *self, t_binary_information *binary, JITINT8 *type_name_space, JITINT8 *type_name);
    struct _TypeDescriptor *(*getTypeDescriptorFromName)(struct metadataManager_t *self, JITINT8 *type_name_space, JITINT8 *type_name);
    struct _TypeDescriptor *(*getTypeDescriptorFromIRType)(struct metadataManager_t *self, JITUINT32 IRType);
    struct _TypeDescriptor *(*getTypeDescriptorFromElementType)(struct metadataManager_t *self, JITUINT32 element_type);

    struct _TypeDescriptor *(*makeArrayDescriptor)(struct metadataManager_t *self, struct _TypeDescriptor *type, JITUINT32 rank, JITUINT32 numSizes, JITUINT32 *sizes, JITUINT32 numBounds, JITUINT32 *bounds);
    struct _TypeDescriptor *(*makeFunctionPointerDescriptor)(struct metadataManager_t *self, JITBOOLEAN hasThis, JITBOOLEAN explicitThis, JITBOOLEAN hasSentinel, JITUINT32 sentinel_index, JITUINT32 calling_convention, JITUINT32 generic_param_count, struct _TypeDescriptor *result, XanList *params);

    /* warning: Use only for reflection */
    struct _TypeDescriptor * (*getTypeDescriptorFromTableAndRow)(struct metadataManager_t * self, struct _GenericContainer *container, t_binary_information *binary, JITUINT8 table_id, JITUINT32 row_id);
    struct _MethodDescriptor *(*getMethodDescriptorFromTableAndRow)(struct metadataManager_t *manager, struct _GenericContainer *container, t_binary_information *binary, JITUINT8 table_id, JITUINT32 row_id, struct _FunctionPointerDescriptor **actualSignature);
    /* end warning */
    struct _TypeDescriptor * (*getTypeDescriptorFromToken)(struct metadataManager_t * self, struct _GenericContainer *container, t_binary_information *binary, JITUINT32 token);
    struct _ActualMethodDescriptor (*getMethodDescriptorFromToken)(struct metadataManager_t * self, struct _GenericContainer *container, t_binary_information *binary, JITUINT32 token);
    struct _FieldDescriptor * (*getFieldDescriptorFromToken)(struct metadataManager_t * self, struct _GenericContainer *container, t_binary_information *binary, JITUINT32 token);
    ILConcept (*fetchDescriptor)(struct metadataManager_t *manager, struct _GenericContainer *container, t_binary_information *binary, JITUINT32 token, void **descriptor);

    XanList *(*decodeMethodLocals)(struct metadataManager_t *manager, t_binary_information *binary, JITUINT32 token, struct _GenericContainer *container);

    MethodDescriptor *(*getEntryPointMethod)(struct metadataManager_t * manager, t_binary_information *binary);

    BasicAssembly *(*getAssemblyFromName)(struct metadataManager_t *manager, JITINT8 *name);

    JITINT32 (*getIndexOfTypeDescriptor)(TypeDescriptor *type);

    XanList * (*getMethodsFromAssembly)(struct metadataManager_t *manager, BasicAssembly *assembly);

    void (*initialize)();
    void (*shutdown)(struct metadataManager_t * manager);
} metadataManager_t;

void init_metadataManager (struct metadataManager_t *manager, XanList *binaries);

ArrayDescriptor * METADATA_fromTypeToArray (TypeDescriptor *t);
TypeDescriptor * METADATA_fromArrayToTypeDescriptor (ArrayDescriptor *array);

JITBOOLEAN isValidCallToken (t_binary_information *binary, JITUINT32 token);
JITBOOLEAN isValidFieldToken (t_binary_information *binary, JITUINT32 token);
JITBOOLEAN isValidTypeToken (t_binary_information *binary, JITUINT32 token);

#endif
