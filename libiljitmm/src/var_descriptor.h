
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
#ifndef VAR_DESCRIPTOR_H
#define VAR_DESCRIPTOR_H

#include <metadata_types.h>

/* Generic Var Descriptor Accessor Methods */
inline GenericVarDescriptor *newGenericVarDescriptor (metadataManager_t *manager, BasicGenericParam *row, TypeDescriptor *owner);

inline JITINT8 *getNameFromGenericVarDescriptor (GenericVarDescriptor *type);
inline ClassDescriptor *getUnderliningClassDescriptorFromGenericVarDescriptor (GenericVarDescriptor *type);
inline TypeDescriptor *getInstanceFromGenericVarDescriptor (GenericVarDescriptor *type, GenericContainer *container);

inline TypeDescriptor *getSuperTypeFromGenericVarDescriptor (GenericVarDescriptor *type);
inline JITUINT32 getIRTypeFromGenericVarDescriptor (GenericVarDescriptor *type);
inline XanList *getConstraintsFromGenericVarDescriptor (GenericVarDescriptor *type);

inline void genericVarDescriptorLock (GenericVarDescriptor *type);
inline void genericVarDescriptorUnLock (GenericVarDescriptor *type);

inline DescriptorMetadataTicket *genericVarDescriptorCreateDescriptorMetadataTicket (GenericVarDescriptor *type_desc, JITUINT32 type);
inline void genericVarDescriptorBroadcastDescriptorMetadataTicket (GenericVarDescriptor *type,  DescriptorMetadataTicket *ticket, void * data);

/* Generic Method Var Descriptor Accessor Methods */
inline GenericMethodVarDescriptor *newGenericMethodVarDescriptor (metadataManager_t *manager, BasicGenericParam *row, MethodDescriptor *owner);
inline JITINT8 *getNameFromGenericMethodVarDescriptor (GenericMethodVarDescriptor *type);
inline ClassDescriptor *getUnderliningClassDescriptorFromGenericMethodVarDescriptor (GenericMethodVarDescriptor *type);
inline TypeDescriptor *getInstanceFromGenericMethodVarDescriptor (GenericMethodVarDescriptor *type, GenericContainer *container);

inline TypeDescriptor *getSuperTypeFromGenericMethodVarDescriptor (GenericMethodVarDescriptor *type);
inline JITUINT32 getIRTypeFromGenericMethodVarDescriptor (GenericMethodVarDescriptor *type);

inline XanList *getConstraintsFromGenericMethodVarDescriptor (GenericMethodVarDescriptor *type);

inline void genericMethodVarDescriptorLock (GenericMethodVarDescriptor *type);
inline void genericMethodVarDescriptorUnLock (GenericMethodVarDescriptor *type);

inline DescriptorMetadataTicket *genericMethodVarDescriptorCreateDescriptorMetadataTicket (GenericMethodVarDescriptor *type_desc, JITUINT32 type);
inline void genericMethodVarDescriptorBroadcastDescriptorMetadataTicket (GenericMethodVarDescriptor *type,  DescriptorMetadataTicket *ticket, void * data);

#endif
