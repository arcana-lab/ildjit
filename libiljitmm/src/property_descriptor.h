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
#ifndef PROPERTY_DESCRIPTOR_H
#define PROPERTY_DESCRIPTOR_H


#include <metadata_types.h>


inline PropertyDescriptor *newPropertyDescriptor (metadataManager_t *manager, BasicProperty *row, TypeDescriptor *owner);

inline TypeDescriptor *getTypeDescriptorFromProperty (PropertyDescriptor *property);

inline JITINT8 *getNameFromPropertyDescriptor (PropertyDescriptor *property);
inline JITINT8 *getCompleteNameFromPropertyDescriptor (PropertyDescriptor *property);

inline MethodDescriptor *getOtherFromProperty (PropertyDescriptor *property);
inline MethodDescriptor *getGetter (PropertyDescriptor *property);
inline MethodDescriptor *getSetter (PropertyDescriptor *property);

inline void propertyDescriptorLock (PropertyDescriptor *property);
inline void propertyDescriptorUnLock (PropertyDescriptor *property);

inline DescriptorMetadataTicket *propertyDescriptorCreateDescriptorMetadataTicket (PropertyDescriptor *property, JITUINT32 type);
inline void propertyDescriptorBroadcastDescriptorMetadataTicket (PropertyDescriptor *property,  DescriptorMetadataTicket *ticket, void * data);

#endif
