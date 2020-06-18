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
#ifndef METADATA_TYPES_SPEC_MANAGER_H
#define METADATA_TYPES_SPEC_MANAGER_H

#include <metadata_types.h>
#include <metadata_types_spec.h>

/* TypeSpecDescriptor Accessor Methods */
inline JITBOOLEAN equalsWithMapperTypeSpecDescriptor (TypeSpecDescriptor *type1, TypeDescriptor *owner, XanHashTable *mapper, TypeSpecDescriptor *type2);

inline TypeSpecDescriptor *newTypeSpecDescriptor (ILElemType element_type, void * descriptor, JITBOOLEAN isByRef);
inline TypeSpecDescriptor *replaceTypeSpecDescriptor (TypeSpecDescriptor *type, GenericSpecContainer *container);
inline void destroyTypeSpecDescriptor (TypeSpecDescriptor *type);

inline JITBOOLEAN isClosedTypeSpecDescriptor (TypeSpecDescriptor *type);

inline TypeDescriptor *replaceGenericVarTypeSpecDescriptor (metadataManager_t *manager, TypeSpecDescriptor *type, GenericContainer *container);

/* Pointer Description Accessor Method */
inline JITBOOLEAN equalsWithMapperPointerSpecDescriptor (PointerSpecDescriptor *key1, TypeDescriptor *owner, XanHashTable *mapper, PointerSpecDescriptor *key2);

inline PointerSpecDescriptor *newPointerSpecDescriptor (TypeSpecDescriptor *type);
inline PointerSpecDescriptor *replacePointerSpecDescriptor (PointerSpecDescriptor *pointer, GenericSpecContainer *container);
inline void destroyPointerSpecDescriptor (PointerSpecDescriptor *pointer);

inline JITBOOLEAN isClosedPointerSpecDescriptor (PointerSpecDescriptor *pointer);

inline PointerDescriptor *replaceGenericVarPointerSpecDescriptor (metadataManager_t *manager, PointerSpecDescriptor *pointer, GenericContainer *container);

/* Array descriptor Accessor Method */
inline JITBOOLEAN equalsWithMapperArraySpecDescriptor (ArraySpecDescriptor *key1, TypeDescriptor *owner, XanHashTable *mapper, ArraySpecDescriptor *key2);

inline ArraySpecDescriptor *newArraySpecDescriptor (TypeSpecDescriptor *type, JITUINT32 rank, JITUINT32 numSizes, JITUINT32 *sizes, JITUINT32 numBounds, JITUINT32 *bounds);
inline ArraySpecDescriptor *replaceArraySpecDescriptor (ArraySpecDescriptor *array, GenericSpecContainer *container);
inline void destroyArraySpecDescriptor (ArraySpecDescriptor *array);

inline JITBOOLEAN isClosedArraySpecDescriptor (ArraySpecDescriptor *array);

inline ArrayDescriptor *replaceGenericVarArraySpecDescriptor (metadataManager_t *manager, ArraySpecDescriptor *array, GenericContainer *container);

/* Generic Var descriptor Accessor Method */
inline GenericVarSpecDescriptor *newGenericVarSpecDescriptor (JITUINT32 var);
inline void destroyGenericVarSpecDescriptor (GenericVarSpecDescriptor *GenericVar);

inline JITBOOLEAN isClosedGenericVarSpecDescriptor (GenericVarSpecDescriptor *var);


/* Generic Method Var descriptor Accessor Method */
inline GenericMethodVarSpecDescriptor *newGenericMethodVarSpecDescriptor (JITUINT32 mvar);
inline void destroyGenericMethodVarSpecDescriptor (GenericMethodVarSpecDescriptor *GenericMethodVar);

inline JITBOOLEAN isClosedGenericMethodVarSpecDescriptor (GenericMethodVarSpecDescriptor *mvar);

/* Generic SpecContainer Accessor Method */
inline void destroyGenericSpecContainer (GenericSpecContainer *container);
inline GenericSpecContainer *newGenericSpecContainer (JITUINT32 arg_count);

inline JITBOOLEAN equalsWithMapperGenericSpecContainer (GenericSpecContainer *key1, TypeDescriptor *owner, XanHashTable *mapper, GenericSpecContainer *key2);

inline void insertTypeSpecDescriptorIntoGenericSpecContainer (GenericSpecContainer *self, TypeSpecDescriptor *type, JITUINT32 position);

/* Class descriptor Accessor Method*/
inline void destroyClassSpecDescriptor(ClassSpecDescriptor * class );
inline ClassSpecDescriptor *newClassSpecDescriptor (BasicClass *row, GenericSpecContainer *container);
inline ClassSpecDescriptor *replaceClassSpecDescriptor(ClassSpecDescriptor * class, GenericSpecContainer * container);

inline JITBOOLEAN equalsWithMapperClassSpecDescriptor (ClassSpecDescriptor *key1, TypeDescriptor *owner, XanHashTable *mapper, ClassSpecDescriptor *key2);

inline JITBOOLEAN isClosedClassSpecDescriptor(ClassSpecDescriptor * class );

inline ClassDescriptor *replaceGenericVarClassSpecDescriptor(metadataManager_t * manager, ClassSpecDescriptor * class, GenericContainer * container);

/* Function Pointer descriptor Accessor Method */
inline JITBOOLEAN equalsWithMapperFunctionPointerSpecDescriptor (FunctionPointerSpecDescriptor *key1, TypeDescriptor *owner, XanHashTable *mapper, FunctionPointerSpecDescriptor *key2);

inline FunctionPointerSpecDescriptor *newFunctionPointerSpecDescriptor (JITBOOLEAN hasThis, JITBOOLEAN explicitThis, JITBOOLEAN hasSentinel, JITUINT32 sentinel_index, JITUINT32 calling_convention, JITUINT32 generic_param_count, TypeSpecDescriptor *result, JITUINT32 param_count, TypeSpecDescriptor **params);
inline FunctionPointerSpecDescriptor *replaceFunctionPointerSpecDescriptor (FunctionPointerSpecDescriptor *pointer, GenericSpecContainer *container);
inline void destroyFunctionPointerSpecDescriptor (FunctionPointerSpecDescriptor *pointer);

inline JITBOOLEAN isClosedFunctionPointerSpecDescriptor (FunctionPointerSpecDescriptor *pointer);

inline FunctionPointerDescriptor *replaceGenericVarFunctionPointerSpecDescriptor (metadataManager_t *manager, FunctionPointerSpecDescriptor *pointer, GenericContainer *container);

inline JITBOOLEAN equalsNoVarArgFunctionPointerSpecDescriptor (FunctionPointerSpecDescriptor *method1, FunctionPointerSpecDescriptor *method2);


#endif
