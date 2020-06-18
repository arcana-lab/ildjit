/*
 * Copyright (C) 2009 - 2010  Campanoni Simone
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
#ifndef LIB_VARARG_H
#define LIB_VARARG_H


#include <jitsystem.h>
#include <metadata/metadata_types.h>
#include <platform_API.h>

typedef struct t_varargManager {
    TypeDescriptor          *TypedReferenceType;
    FieldDescriptor         *argsField;
    FieldDescriptor         *posnField;
    JITINT32 argsFieldOffset;
    JITINT32 posnFieldOffset;

    void (*create_arg_iterator)(void* object, void* args, JITINT32 pos);
    void *          (*getargsField)(void *object);
    void (*setargsField)(void *object, void*args);
    JITINT32 (*getposnField)(void *object);
    void (*setposnField)(void *object, JITINT32 posn);
    void (*initialize)(void);
} t_varargManager;

void init_varargManager (t_varargManager *varargManager);

#endif
