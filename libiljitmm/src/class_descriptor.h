
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
#ifndef CLASS_DESCRIPTOR_H
#define CLASS_DESCRIPTOR_H

#include <xanlib.h>
#include <metadata_types.h>
#include <metadata_types_spec.h>

inline XanList *getExplictOverrideInformationsFromClassDescriptor(ClassDescriptor * class );
inline XanList *getInterfacesInformationFromClassDescriptor(ClassDescriptor * class, GenericSpecContainer * container);
inline GenericSpecContainer *getSuperClassGenericSpecContainer(ClassDescriptor * class );

/* ClassDescriptor Accessor Method */
inline ClassDescriptor *newClassDescriptor (metadataManager_t *manager, BasicClass *row, GenericContainer *container);

inline TypeDescriptor *getTypeDescriptorFromClassDescriptor(ClassDescriptor * class );
inline TypeDescriptor *getByRefTypeDescriptorFromClassDescriptor(ClassDescriptor * class );

inline JITUINT32 getIRTypeFromClassDescriptor(ClassDescriptor * class );

inline XanList * getImplementedInterfacesFromClassDescriptor(ClassDescriptor * class );
inline TypeDescriptor *getSuperClass(ClassDescriptor * class );

inline JITBOOLEAN implementsInterface (ClassDescriptor *type, ClassDescriptor *interfaceDescription);
inline JITBOOLEAN extendsType (ClassDescriptor *type, ClassDescriptor *supertype);

inline JITBOOLEAN classDescriptorIsSubTypeOf (ClassDescriptor *type, ClassDescriptor *superType);
inline JITBOOLEAN classDescriptorAssignableTo (ClassDescriptor *type_to_cast, ClassDescriptor *desired_type);

inline JITBOOLEAN isNullableClassDescriptor(ClassDescriptor * class );
inline JITBOOLEAN isValueTypeClassDescriptor(ClassDescriptor * class );
inline JITBOOLEAN isEnumClassDescriptor(ClassDescriptor * class );
inline JITBOOLEAN isDelegateClassDescriptor(ClassDescriptor * class );
inline JITBOOLEAN isPrimitiveClassDescriptor(ClassDescriptor * class );

inline TypeDescriptor *getEnclosingClass(ClassDescriptor * class );

inline XanList *getNestedClass(ClassDescriptor * class );
inline TypeDescriptor *getNestedClassFromName(ClassDescriptor * class, JITINT8 * name);

inline JITINT8 *getNameFromClassDescriptor(ClassDescriptor * class );
inline JITINT8 *getTypeNameSpaceFromClassDescriptor(ClassDescriptor * class );
inline JITINT8 *getCompleteNameFromClassDescriptor(ClassDescriptor * class );

inline XanList *getDirectImplementedFieldsClassDescriptor(ClassDescriptor * class );
inline FieldDescriptor *getFieldDescriptorFromNameClassDescriptor(ClassDescriptor * class, JITINT8 * name);

inline XanList *getDirectDeclaredMethodsClassDescriptor(ClassDescriptor * class );
inline XanList *getMethodDescriptorFromNameClassDescriptor(ClassDescriptor * class, JITINT8 * name);
inline XanList *getConstructorsClassDescriptor(ClassDescriptor * class );
inline MethodDescriptor *getCctorClassDescriptor(ClassDescriptor * class );
inline MethodDescriptor *getFinalizerClassDescriptor(ClassDescriptor * class );

inline XanList * getEventsClassDescriptor(ClassDescriptor * class );
inline EventDescriptor *getEventDescriptorFromNameClassDescriptor(ClassDescriptor * class, JITINT8 * name);

inline XanList *getPropertiesClassDescriptor(ClassDescriptor * class );
inline PropertyDescriptor *getPropertyDescriptorFromNameClassDescriptor(ClassDescriptor * class, JITINT8 * name);

inline BasicAssembly *getAssemblyFromClassDescriptor(ClassDescriptor * class );

inline XanList *getGenericParametersFromClassDescriptor (ClassDescriptor *type);
inline JITBOOLEAN isGenericClassDefinition(ClassDescriptor * class );
inline JITBOOLEAN containOpenGenericParameterFromClassDescriptor(ClassDescriptor * class );
inline JITBOOLEAN isGenericClassDescriptor(ClassDescriptor * class );
inline TypeDescriptor *getInstanceFromClassDescriptor(ClassDescriptor * class, GenericContainer * container);
inline TypeDescriptor *getGenericDefinitionFromClassDescriptor(ClassDescriptor * class );
inline GenericContainer *getResolvingContainerFromClassDescriptor(ClassDescriptor * class );

inline void classDescriptorLock(ClassDescriptor * class );
inline void classDescriptorUnLock(ClassDescriptor * class );

inline DescriptorMetadataTicket *classDescriptorCreateDescriptorMetadataTicket(ClassDescriptor * class, JITUINT32 type);
inline void classDescriptorBroadcastDescriptorMetadataTicket(ClassDescriptor * class,  DescriptorMetadataTicket * ticket, void * data);
#endif
