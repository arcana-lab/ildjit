/*
 * Copyright (C) 2009  Campanoni Simone
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
#ifndef INTERNAL_CALLS_OBJECT_H
#define INTERNAL_CALLS_OBJECT_H

#include <jitsystem.h>

void *          System_Object_GetType (void *object);
JITINT32        System_Object_GetHashCode (void *obj);
JITBOOLEAN      System_Object_Equals (void *obj1, void *obj2);
void *          System_Object_MemberwiseClone (void *obj);

/**
 * Get the hash code of the given object
 *
 * This is a Mono internal call.
 *
 * @param object the object with the unknown hash code
 *
 * @return the hash code of the given object
 */
JITINT32 System_Object_InternalGetHashCode (void* object);

#endif
