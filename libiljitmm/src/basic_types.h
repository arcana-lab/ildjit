
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
#ifndef BASIC_TYPES_H
#define BASIC_TYPES_H

#include <xanlib.h>
#include <decoder.h>
#include <metadata_table_manager.h>
#include <jitsystem.h>
#include <ecma_constant.h>

typedef enum _ILConcept {
    ILTYPE,
    ILFIELD,
    ILMETHOD,
    ILPROPERTY,
    ILEVENT
} ILConcept;

typedef enum _ILElemType {
    ELEMENT_ARRAY,
    ELEMENT_CLASS,
    ELEMENT_FNPTR,
    ELEMENT_PTR,
    ELEMENT_VAR,
    ELEMENT_MVAR
} ILElemType;

typedef enum _MethodSemantics {
    STANDARD,
    SETTER,
    GETTER,
    OTHER,
    ADD_ON,
    REMOVE_ON,
    FIRE
} MethodSemantics;

typedef enum _MethodAssociation {
    DEFAULT_ASSOCIATION,
    EVENT_ASSOCIATION,
    PROPERTY_ASSOCIATION
} MethodAssociation;

typedef t_row_type_def_table BasicClass;
typedef t_row_method_def_table BasicMethod;
typedef t_row_field_table BasicField;
typedef t_row_generic_param_table BasicGenericParam;

typedef t_row_assembly_table BasicAssembly;
typedef t_row_property_table BasicProperty;
typedef t_row_event_table BasicEvent;
typedef t_row_param_table BasicParam;
typedef t_row_module_table BasicModule;

typedef struct _BasicGenericParamAttributes {
    JITBOOLEAN isCovariant;
    JITBOOLEAN isContravariant;
    JITBOOLEAN isReference;
    JITBOOLEAN isValueType;
    JITBOOLEAN hasDefaultConstructor;
    JITUINT32 number;
} BasicGenericParamAttributes;

typedef struct _BasicMethodAttributes {
    JITUINT32 rva;
    JITUINT32 accessMask;
    JITBOOLEAN is_internal_call;
    JITBOOLEAN is_native;
    JITBOOLEAN is_cil;
    JITBOOLEAN is_provided_by_the_runtime;
    JITBOOLEAN is_abstract;
    JITBOOLEAN is_pinvoke;
    JITBOOLEAN is_virtual;
    JITBOOLEAN is_static;
    JITBOOLEAN is_special_name;
    JITBOOLEAN need_new_slot;
    JITBOOLEAN override;
    JITBOOLEAN is_vararg;
    JITBOOLEAN has_this;
    JITBOOLEAN explicit_this;
    JITUINT32 param_count;
    JITUINT32 calling_convention;
    MethodSemantics semantics;
    MethodAssociation association;
} BasicMethodAttributes;

typedef struct _BasicTypeAttributes {
    JITUINT32 visibility;
    JITUINT32 layout;
    JITBOOLEAN auto_layout;
    JITBOOLEAN sequential_layout;
    JITBOOLEAN explicit_layout;
    JITBOOLEAN isAbstract;
    JITBOOLEAN isNested;
    JITBOOLEAN isSealed;
    JITBOOLEAN isInterface;
    JITBOOLEAN isSerializable;
    JITBOOLEAN allowsRelaxedInitialization;
    JITUINT32 param_count;
    JITUINT32 packing_size;
    JITUINT32 class_size;
    JITUINT32 flags;
} BasicTypeAttributes;

typedef struct _BasicFieldAttributes {
    JITUINT32 accessMask;
    JITBOOLEAN is_static;
    JITBOOLEAN is_init_only;
    JITBOOLEAN is_literal;
    JITBOOLEAN is_serializable;
    JITBOOLEAN is_special;
    JITBOOLEAN is_pinvoke;
    JITBOOLEAN is_rt_special_name;
    JITBOOLEAN has_marshal_information;
    JITBOOLEAN has_default;
    JITBOOLEAN has_rva;
    JITUINT32 rva;
    JITBOOLEAN has_offset;
    JITUINT32 offset;
} BasicFieldAttributes;

typedef struct _BasicPropertyAttributes {
    JITBOOLEAN is_special_name;
    JITBOOLEAN hasDefault;
} BasicPropertyAttributes;

typedef struct _BasicEventAttributes {
    JITBOOLEAN is_special_name;
} BasicEventAttributes;

typedef struct _BasicParamAttributes {
    JITBOOLEAN is_in;
    JITBOOLEAN is_out;
    JITBOOLEAN is_optional;
    JITBOOLEAN has_default;
    JITBOOLEAN has_field_marshal;
} BasicParamAttributes;

typedef enum _ContainerType {
    CLASS_LEVEL,
    METHOD_LEVEL
} ContainerType;

#define TRUNCATE                1
#define NOP                     2
#define TRUNCATE_TO_ZERO        3
#define SIGN_EXTENDED           4
#define ZERO_EXTENDED           5
#define INVALID_CONVERSION      6
#define TO_FLOAT                7
#define CHANGE_PRECISION        8
#define STOP_GC_TRACKING        9

#endif
