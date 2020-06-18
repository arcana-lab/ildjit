
/*
 * Copyright (C) 2009  Simone Campanoni, Luca Rocchini
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


// My headers
#ifndef TYPE_DESCRIPTOR_H
#define TYPE_DESCRIPTOR_H

#include <xanlib.h>
#include <metadata_types.h>
#include <metadata_types_spec.h>

#define TYPE_COMP_SIZE 20

typedef struct {
    MethodDescriptor *declaration;
    MethodDescriptor *body;
}  ExplicitOverrideInformation;

typedef struct {
    MethodDescriptor *declaration;
    TypeDescriptor *owner;
    FunctionPointerSpecDescriptor *signature;
} MethodInformation;

typedef struct {
    TypeDescriptor *interfaceDescription;
    GenericSpecContainer *container;
} InterfaceInformation;

typedef struct _VTable {
    XanHashTable *slots; //vtable slot
    JITUINT32 length;
    /* following field is used to compute vtable*/
    XanList *interfacesMethods;     //set of method defined in interface
    XanList *interfacesSet;         // set of implemenent interface
    XanList *methodsList;
    XanHashTable *genericParameterMap;
} VTable;

inline TypeDescriptor *newTypeDescriptor (metadataManager_t *manager, ILElemType element_type, void * descriptor, JITBOOLEAN isByRef);

inline XanHashTable *getVTableFromTypeDescriptor (TypeDescriptor *type, JITUINT32 *vTableLength);

inline JITINT8 *getNameFromTypeDescriptor (TypeDescriptor *type);
inline JITINT8 *getCompleteNameFromTypeDescriptor (TypeDescriptor *type);
inline JITINT8 *getTypeNameSpaceFromTypeDescriptor (TypeDescriptor *type);

inline BasicAssembly *getAssemblyFromTypeDescriptor (TypeDescriptor *type);
inline ClassDescriptor *getUnderliningClassDescriptorFromTypeDescriptor (TypeDescriptor *type);

inline JITBOOLEAN isValueTypeTypeDescriptor (TypeDescriptor *type);
inline JITBOOLEAN isEnumTypeDescriptor (TypeDescriptor *type);
inline JITBOOLEAN isDelegateTypeDescriptor (TypeDescriptor *type);
inline JITBOOLEAN isNullableTypeDescriptor (TypeDescriptor *type);
inline JITBOOLEAN isPrimitiveTypeDescriptor (TypeDescriptor *type);
inline TypeDescriptor *getSuperTypeFromTypeDescriptor (TypeDescriptor *type);
inline MethodDescriptor *typeDescriptorImplementsMethod (TypeDescriptor *type, MethodDescriptor *method);

inline JITUINT32 getIRTypeFromTypeDescriptor (TypeDescriptor *type);

inline JITBOOLEAN typeDescriptorAssignableTo (TypeDescriptor *type_to_cast, TypeDescriptor *desired_type);
inline JITBOOLEAN typeDescriptorIsSubtypeOf (TypeDescriptor *type, TypeDescriptor *superType);

inline TypeDescriptor *makeArrayDescriptor (TypeDescriptor *type, JITUINT32 rank);
inline TypeDescriptor *makePointerDescriptor (TypeDescriptor *type);
inline TypeDescriptor *makeByRef (TypeDescriptor *type);

inline JITBOOLEAN isGenericTypeDefinition (TypeDescriptor *type);
inline JITBOOLEAN containOpenGenericParameterFromTypeDescriptor (TypeDescriptor *type);
inline JITBOOLEAN isGenericTypeDescriptor (TypeDescriptor *type);
inline TypeDescriptor *getGenericDefinitionFromTypeDescriptor (TypeDescriptor *type);
inline TypeDescriptor *getInstanceFromTypeDescriptor (TypeDescriptor *type, GenericContainer *container);
inline XanList *getGenericParametersFromTypeDescriptor (TypeDescriptor *type);

inline void typeDescriptorLock (TypeDescriptor *type);
inline void typeDescriptorUnLock (TypeDescriptor *type);

inline DescriptorMetadataTicket *typeDescriptorCreateDescriptorMetadataTicket (TypeDescriptor *type_desc, JITUINT32 type);
inline void typeDescriptorBroadcastDescriptorMetadataTicket (TypeDescriptor *type,  DescriptorMetadataTicket *ticket, void * data);

inline TypeDescriptor *getEnclosingType (TypeDescriptor *type);
inline GenericContainer *getResolvingContainerFromTypeDescriptor(TypeDescriptor * class );

inline XanList * getImplementedInterfacesFromTypeDescriptor (TypeDescriptor *type);

inline XanList *getDirectImplementedFieldsTypeDescriptor (TypeDescriptor *type);
inline FieldDescriptor *getFieldDescriptorFromNameTypeDescriptor (TypeDescriptor *type, JITINT8 * name);

inline XanList *getDirectDeclaredMethodsTypeDescriptor (TypeDescriptor *type);
inline XanList *getMethodDescriptorFromNameTypeDescriptor (TypeDescriptor *type, JITINT8 * name);
inline XanList *getConstructorsTypeDescriptor (TypeDescriptor *type);
inline MethodDescriptor *getCctorTypeDescriptor (TypeDescriptor *type);
inline MethodDescriptor *getFinalizerTypeDescriptor (TypeDescriptor *type);

inline XanList * getEventsTypeDescriptor (TypeDescriptor *type);
inline EventDescriptor *getEventDescriptorFromNameTypeDescriptor (TypeDescriptor *type, JITINT8 * name);

inline XanList *getPropertiesTypeDescriptor (TypeDescriptor *type);
inline PropertyDescriptor *getPropertyDescriptorFromNameTypeDescriptor (TypeDescriptor *type, JITINT8 * name);

inline XanList *getNestedType (TypeDescriptor *type);
inline TypeDescriptor *getNestedTypeFromName (TypeDescriptor *type, JITINT8 *name);

#endif
