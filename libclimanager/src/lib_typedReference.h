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
#ifndef LIB_TYPEDREFERENCE_H
#define LIB_TYPEDREFERENCE_H


#include <jitsystem.h>
#include <metadata/metadata_types.h>
#include <platform_API.h>

typedef struct t_typedReferenceManager {
    TypeDescriptor          *typedReferenceType;
    TypeDescriptor          *typedReferenceArray;
    FieldDescriptor         *typeField;
    FieldDescriptor         *valueField;
    JITINT32 typeFieldOffset;
    JITINT32 valueFieldOffset;

    TypeDescriptor  *               (*fillILTypedRefType)();
    TypeDescriptor  *               (*fillILTypedRefArray)();
    void (*create_typed_ref)(void* object, void* type, void* value);
    void *          (*getValue)(void *thisTypedRef);
    void *          (*getType)(void *thisTypedRef);
    JITINT32 (*getValueOffset)(struct t_typedReferenceManager *self);
    JITINT32 (*getTypeOffset)(struct t_typedReferenceManager *self);
    void *          (*setValue)(void *thisTypedRef, void *value);
    void *          (*setType)(void *thisTypedRef, void *type);
    void (*copy)(void *dest, void *src);
    void (*initialize)(void);
} t_typedReferenceManager;

void init_typedReferenceManager (t_typedReferenceManager *typedReferenceManager);

#endif
