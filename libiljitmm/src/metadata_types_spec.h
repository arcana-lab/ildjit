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
#ifndef METADATA_TYPES_SPEC_H
#define METADATA_TYPES_SPEC_H

#include <xanlib.h>
#include <decoder.h>
#include <metadata_table_manager.h>
#include <basic_types.h>
#include <jitsystem.h>
#include <ir_language.h>
#include <ecma_constant.h>
#include <metadata_types.h>

typedef struct _GenericSpecContainer {
    struct _TypeSpecDescriptor **paramType;
    JITUINT32 arg_count;

    void (*destroy)(struct _GenericSpecContainer *container);
} GenericSpecContainer;
JITUINT32 hashGenericSpecContainer (struct _GenericSpecContainer *container);
JITINT32 equalsGenericSpecContainer (struct _GenericSpecContainer *key1, struct _GenericSpecContainer *key2);

typedef struct _TypeSpecDescriptor {
    JITUINT32 refCount;
    ILElemType element_type;
    JITBOOLEAN isByRef;
    void *descriptor;
    JITBOOLEAN (*isClosed)(struct _TypeSpecDescriptor *type);
    struct _TypeDescriptor *(*replaceGenericVar)(metadataManager_t *manager, struct _TypeSpecDescriptor *type, struct _GenericContainer *container);

    void (*destroy)(struct _TypeSpecDescriptor *type);
} TypeSpecDescriptor;
JITUINT32 hashTypeSpecDescriptor (struct _TypeSpecDescriptor *type);
JITINT32 equalsTypeSpecDescriptor (struct _TypeSpecDescriptor *key1, struct _TypeSpecDescriptor *key2);


typedef struct _GenericVarSpecDescriptor {
    JITUINT32 var;
    JITBOOLEAN (*isClosed)(struct _GenericVarSpecDescriptor *type);

    void (*destroy)(struct _GenericVarSpecDescriptor *pointer);
} GenericVarSpecDescriptor;
JITUINT32 hashGenericVarSpecDescriptor (struct _GenericVarSpecDescriptor *pointer);
JITINT32 equalsGenericVarSpecDescriptor (struct _GenericVarSpecDescriptor *key1, struct _GenericVarSpecDescriptor *key2);

typedef struct _GenericMethodVarSpecDescriptor {
    JITUINT32 mvar;

    JITBOOLEAN (*isClosed)(struct _GenericMethodVarSpecDescriptor *type);
    void (*destroy)(struct _GenericMethodVarSpecDescriptor *pointer);
} GenericMethodVarSpecDescriptor;
JITUINT32 hashGenericMethodVarSpecDescriptor (struct _GenericMethodVarSpecDescriptor *pointer);
JITINT32 equalsGenericMethodVarSpecDescriptor (struct _GenericMethodVarSpecDescriptor *key1, struct _GenericMethodVarSpecDescriptor *key2);

typedef struct _PointerSpecDescriptor {
    struct _TypeSpecDescriptor *type;

    struct _PointerDescriptor *(*replaceGenericVar)(metadataManager_t *manager, struct _PointerSpecDescriptor *type, struct _GenericContainer *container);
    JITBOOLEAN (*isClosed)(struct _PointerSpecDescriptor *type);
    void (*destroy)(struct _PointerSpecDescriptor *pointer);
} PointerSpecDescriptor;
JITUINT32 hashPointerSpecDescriptor (struct _PointerSpecDescriptor *pointer);
JITINT32 equalsPointerSpecDescriptor (struct _PointerSpecDescriptor *key1, struct _PointerSpecDescriptor *key2);

typedef struct _FunctionPointerSpecDescriptor {
    JITBOOLEAN hasThis;
    JITBOOLEAN explicitThis;
    JITBOOLEAN hasSentinel;
    JITUINT32 sentinel_index;
    JITUINT32 calling_convention;
    JITUINT32 generic_param_count;

    struct _TypeSpecDescriptor      *result;
    JITUINT32 param_count;
    struct _TypeSpecDescriptor      **params;

    struct _FunctionPointerDescriptor  *(*replaceGenericVar)(metadataManager_t *manager, struct _FunctionPointerSpecDescriptor *type, struct _GenericContainer *container);
    JITBOOLEAN (*isClosed)(struct _FunctionPointerSpecDescriptor *type);

    JITBOOLEAN (*equalsNoVarArg)(struct _FunctionPointerSpecDescriptor *method1, struct _FunctionPointerSpecDescriptor *method2);

    void (*destroy)(struct _FunctionPointerSpecDescriptor *pointer);
} FunctionPointerSpecDescriptor;
JITUINT32 hashFunctionPointerSpecDescriptor (struct _FunctionPointerSpecDescriptor *pointer);
JITINT32 equalsFunctionPointerSpecDescriptor (struct _FunctionPointerSpecDescriptor *key1, struct _FunctionPointerSpecDescriptor *key2);

typedef struct _ArraySpecDescriptor {
    struct _TypeSpecDescriptor *type;
    JITUINT32 rank;
    JITUINT32 numSizes;
    JITUINT32       *sizes;
    JITUINT32 numBounds;
    JITUINT32       *bounds;

    struct _ArrayDescriptor *(*replaceGenericVar)(metadataManager_t *manager, struct _ArraySpecDescriptor *type, struct _GenericContainer *container);
    JITBOOLEAN (*isClosed)(struct _ArraySpecDescriptor *type);
    void (*destroy)(struct _ArraySpecDescriptor *array);
} ArraySpecDescriptor;
JITUINT32 hashArraySpecDescriptor (struct _ArraySpecDescriptor *array);
JITINT32 equalsArraySpecDescriptor (struct _ArraySpecDescriptor *key1, struct _ArraySpecDescriptor *key2);


typedef struct _ClassSpecDescriptor {
    BasicClass *row;
    struct _GenericSpecContainer *container;

    struct _ClassDescriptor *(*replaceGenericVar)(metadataManager_t *manager, struct _ClassSpecDescriptor *type, struct _GenericContainer *container);
    JITBOOLEAN (*isClosed)(struct _ClassSpecDescriptor *type);
    void (*destroy)(struct _ClassSpecDescriptor *param);
} ClassSpecDescriptor;
JITUINT32 hashClassSpecDescriptor (struct _ClassSpecDescriptor *param);
JITINT32 equalsClassSpecDescriptor (struct _ClassSpecDescriptor *key1, struct _ClassSpecDescriptor *key2);
#endif
