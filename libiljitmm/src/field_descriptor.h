
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
#ifndef FIELD_DESCRIPTOR_H
#define FIELD_DESCRIPTOR_H

#include <metadata_types.h>

/* FieldDescriptor Accessor Method */
inline FieldDescriptor *newFieldDescriptor (metadataManager_t *manager, BasicField *row, TypeDescriptor *owner);

inline TypeDescriptor *getTypeDescriptorFromField (FieldDescriptor *field);

inline JITINT8 *getNameFromFieldDescriptor (FieldDescriptor *field);
inline JITINT8 *getCompleteNameFromFieldDescriptor (FieldDescriptor *field);

inline JITBOOLEAN isFieldAccessible (FieldDescriptor *field, MethodDescriptor *caller);

inline void fieldDescriptorLock (FieldDescriptor *field);
inline void fieldDescriptorUnLock (FieldDescriptor *field);

inline DescriptorMetadataTicket *fieldDescriptorCreateDescriptorMetadataTicket (FieldDescriptor *field, JITUINT32 type);
inline void fieldDescriptorBroadcastDescriptorMetadataTicket (FieldDescriptor *field,  DescriptorMetadataTicket *ticket, void * data);

#endif
