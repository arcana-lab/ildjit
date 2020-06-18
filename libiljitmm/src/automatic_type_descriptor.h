
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
#ifndef AUTOMATIC_TYPE_DESCRIPTOR_H
#define AUTOMATIC_TYPE_DESCRIPTOR_H

#include <metadata_types.h>

/* Pointer Description Accessor Method */
inline PointerDescriptor *newPointerDescriptor (metadataManager_t *manager, TypeDescriptor *type);

inline TypeDescriptor *getTypeDescriptorFromPointerDescriptor(PointerDescriptor * class );
inline TypeDescriptor *getByRefTypeDescriptorFromPointerDescriptor(PointerDescriptor * class );

inline JITINT8 *getNameFromPointerDescriptor (PointerDescriptor *pointer);
inline JITINT8 *getCompleteNameFromPointerDescriptor (PointerDescriptor *pointer);

inline JITBOOLEAN containOpenGenericParameterFromPointerDescriptor (PointerDescriptor *pointer);

inline void pointerDescriptorLock (PointerDescriptor *type);
inline void pointerDescriptorUnLock (PointerDescriptor *type);

inline DescriptorMetadataTicket *pointerDescriptorCreateDescriptorMetadataTicket (PointerDescriptor * pointer, JITUINT32 type);
inline void pointerDescriptorBroadcastDescriptorMetadataTicket (PointerDescriptor * pointer,  DescriptorMetadataTicket *ticket, void * data);

inline TypeDescriptor *getInstanceFromPointerDescriptor (PointerDescriptor *type, GenericContainer *container);
inline TypeDescriptor *getGenericDefinitionFromPointerDescriptor (PointerDescriptor *type);

/* Array Descriptor Accessor Method */
inline ArrayDescriptor *newArrayDescriptor (metadataManager_t *manager, TypeDescriptor *type, JITUINT32 rank, JITUINT32 numSizes, JITUINT32 *sizes, JITUINT32 numBounds, JITUINT32 *bounds);

inline TypeDescriptor *getTypeDescriptorFromArrayDescriptor(ArrayDescriptor * class );
inline TypeDescriptor *getByRefTypeDescriptorFromArrayDescriptor(ArrayDescriptor * class );

inline JITBOOLEAN ArrayDescriptorAssignableTo (ArrayDescriptor *type_to_cast, ArrayDescriptor *desired_type);


inline JITINT8 *getTypeNameSpaceFromArrayDescriptor (ArrayDescriptor *array);
inline JITINT8 *getNameFromArrayDescriptor (ArrayDescriptor *array);
inline JITINT8 *getCompleteNameFromArrayDescriptor (ArrayDescriptor *array);
inline BasicAssembly *getAssemblyFromArrayDescriptor (ArrayDescriptor *array);
inline JITBOOLEAN containOpenGenericParameterFromArrayDescriptor (ArrayDescriptor *array);

inline void arrayDescriptorLock (ArrayDescriptor *type);
inline void arrayDescriptorUnLock (ArrayDescriptor *type);

inline DescriptorMetadataTicket *arrayDescriptorCreateDescriptorMetadataTicket (ArrayDescriptor * array, JITUINT32 type);
inline void arrayDescriptorBroadcastDescriptorMetadataTicket (ArrayDescriptor * array,  DescriptorMetadataTicket *ticket, void * data);

inline TypeDescriptor *getInstanceFromArrayDescriptor (ArrayDescriptor *type, GenericContainer *container);
inline TypeDescriptor *getGenericDefinitionFromArrayDescriptor (ArrayDescriptor *type);

/* Function Pointer Descriptor Accessor Method */
inline FunctionPointerDescriptor *newFunctionPointerDescriptor (metadataManager_t *manager, JITBOOLEAN hasThis, JITBOOLEAN explicitThis, JITBOOLEAN hasSentinel, JITUINT32 sentinel_index, JITUINT32 calling_convention, JITUINT32 generic_param_count, TypeDescriptor *result, XanList *params);
inline FunctionPointerDescriptor *newUnCachedFunctionPointerDescriptor (metadataManager_t *manager, JITBOOLEAN hasThis, JITBOOLEAN explicitThis, JITBOOLEAN hasSentinel, JITUINT32 sentinel_index, JITUINT32 calling_convention, JITUINT32 generic_param_count, TypeDescriptor *result, XanList *params);

inline TypeDescriptor *getTypeDescriptorFromFunctionPointerDescriptor(FunctionPointerDescriptor * class );
inline TypeDescriptor *getByRefTypeDescriptorFromFunctionPointerDescriptor(FunctionPointerDescriptor * class );
inline JITINT8 *getSignatureInStringFromFunctionPointerDescriptor (FunctionPointerDescriptor *pointer);

inline JITBOOLEAN containOpenGenericParameterFromFunctionPointerDescriptor (FunctionPointerDescriptor *pointer);

inline void functionPointerDescriptorLock (FunctionPointerDescriptor *type);
inline void functionPointerDescriptorUnLock (FunctionPointerDescriptor *type);

inline DescriptorMetadataTicket *functionPointerDescriptorCreateDescriptorMetadataTicket (FunctionPointerDescriptor *pointer, JITUINT32 type);
inline void functionPointerDescriptorBroadcastDescriptorMetadataTicket (FunctionPointerDescriptor *pointer,  DescriptorMetadataTicket *ticket, void * data);

inline TypeDescriptor *getInstanceFromFunctionPointerDescriptor (FunctionPointerDescriptor *type, GenericContainer *container);
inline TypeDescriptor *getGenericDefinitionFromFunctionPointerDescriptor (FunctionPointerDescriptor *type);
#endif
