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
#ifndef EVENT_DESCRIPTOR_H
#define EVENT_DESCRIPTOR_H


#include <metadata_types.h>

inline EventDescriptor *newEventDescriptor (metadataManager_t *manager, BasicEvent *row, TypeDescriptor *owner);

inline TypeDescriptor *getTypeDescriptorFromEvent (EventDescriptor *event);

inline JITINT8 *getNameFromEventDescriptor (EventDescriptor *event);
inline JITINT8 *getCompleteNameFromEventDescriptor (EventDescriptor *event);

inline MethodDescriptor * getOtherFromEvent (EventDescriptor *event);
inline MethodDescriptor *getAddOn (EventDescriptor *event);
inline MethodDescriptor *getRemoveOn (EventDescriptor *event);
inline MethodDescriptor *getFire (EventDescriptor *event);

inline void eventDescriptorLock (EventDescriptor *event);
inline void eventDescriptorUnLock (EventDescriptor *event);

inline DescriptorMetadataTicket *eventDescriptorCreateDescriptorMetadataTicket (EventDescriptor *event, JITUINT32 type);
inline void eventDescriptorBroadcastDescriptorMetadataTicket (EventDescriptor *event,  DescriptorMetadataTicket *ticket, void * data);
#endif
