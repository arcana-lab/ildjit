/*
 * Copyright (C) 2006  Campanoni Simone
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

/**
   *@file ecma_constant.h
 */

#ifndef ECMA_CONSTANT
#define ECMA_CONSTANT

#define COMIMAGE_FLAGS_32BITREQUIRED            0x00000002
#define COMIMAGE_FLAGS_STRONGNAMESIGNED         0x00000008
#define ECMA_SIGNATURE                          0x424A5342

#define STRING_STREAM_HEAP_SIZE_EXTENDED_BIT    0x00
#define GUID_STREAM_HEAP_SIZE_EXTENDED_BIT      0x01
#define BLOB_STREAM_HEAP_SIZE_EXTENDED_BIT      0x02

/*****************************************************************************************************************************
                                                TABLES
*****************************************************************************************************************************/

/**
 * @def MODULE_TABLE
 *
 * Module table identifier
 */
#define MODULE_TABLE                            0x00

/**
 * @def TYPE_REF_TABLE
 *
 * TypeRef table identifier
 */
#define TYPE_REF_TABLE                          0x01

/**
 * @def TYPE_DEF_TABLE
 *
 * TypeDef table identifier
 */
#define TYPE_DEF_TABLE                          0x02

/**
 * @def FIELD_TABLE
 *
 * Field table identifier
 */
#define FIELD_TABLE                             0x04

/**
 * @def METHOD_DEF_TABLE
 *
 * MethodDef table identifier
 */
#define METHOD_DEF_TABLE                        0x06

/**
 * @def PARAM_TABLE
 *
 * Param table identifier
 */
#define PARAM_TABLE                             0x08

/**
 * @def INTERFACE_IMPL_TABLE
 *
 * InterfaceImpl table identifier
 */
#define INTERFACE_IMPL_TABLE                    0x09

/**
 * @def MEMBER_REF_TABLE
 *
 * MemberRef table identifier
 */
#define MEMBER_REF_TABLE                        0x0A

/**
 * @def CONSTANT_TABLE
 *
 * Constant table identifier
 */
#define CONSTANT_TABLE                          0x0B

/**
 * @def CUSTOM_ATTRIBUTE_TABLE
 *
 * CustomAttribute table identifier
 */
#define CUSTOM_ATTRIBUTE_TABLE                  0x0C

/**
 * @def FIELD_MARSHAL_TABLE
 *
 * FieldMarshal table identifier
 */
#define FIELD_MARSHAL_TABLE                     0x0D

/**
 * @def DECL_SECURITY_TABLE
 *
 * DeclSecurity table identifier
 */
#define DECL_SECURITY_TABLE                     0x0E

/**
 * @def CLASS_LAYOUT_TABLE
 *
 * ClassLayout table identifier
 */
#define CLASS_LAYOUT_TABLE                      0x0F

/**
 * @def FIELD_LAYOUT_TABLE
 *
 * FieldLayout table identifier
 */
#define FIELD_LAYOUT_TABLE                      0x10

/**
 * @def STANDALONE_SIG_TABLE
 *
 * StandAloneSig table identifier
 */
#define STANDALONE_SIG_TABLE                    0x11

/**
 * @def EVENT_MAP_TABLE
 *
 * EventMap table identifier
 */
#define EVENT_MAP_TABLE                         0x12

/**
 * @def EVENT_TABLE
 *
 * Event table identifier
 */
#define EVENT_TABLE                             0x14

/**
 * @def PROPERTY_MAP_TABLE
 *
 * PropertyMap table identifier
 */
#define PROPERTY_MAP_TABLE                      0x15

/**
 * @def PROPERTY_TABLE
 *
 * Property table identifier
 */
#define PROPERTY_TABLE                          0x17

/**
 * @def METHOD_SEMANTICS_TABLE
 *
 * MethodSemantics table identifier
 */
#define METHOD_SEMANTICS_TABLE                  0x18

/**
 * @def METHOD_IMPL_TABLE
 *
 * MethodImpl table identifier
 */
#define METHOD_IMPL_TABLE                       0x19

/**
 * @def MODULE_REF_TABLE
 *
 * ModuleRef table identifier
 */
#define MODULE_REF_TABLE                        0x1A

/**
 * @def TYPE_SPEC_TABLE
 *
 * TypeSpec table identifier
 */
#define TYPE_SPEC_TABLE                         0x1B

/**
 * @def IMPL_MAP_TABLE
 *
 * ImplMap table identifier
 */
#define IMPL_MAP_TABLE                          0x1C

/**
 * @def FIELD_RVA_TABLE
 *
 * FieldRVA table identifier
 */
#define FIELD_RVA_TABLE                         0x1D

/**
 * @def ASSEMBLY_TABLE
 *
 * Assembly table identifier
 */
#define ASSEMBLY_TABLE                          0x20

/**
 * @def ASSEMBLY_REF_TABLE
 *
 * AssemblyRef table identifier
 */
#define ASSEMBLY_REF_TABLE                      0x23

/**
 * @def FILE_TABLE
 *
 * File table identifier
 */
#define FILE_TABLE                              0x26

/**
 * @def EXPORTED_TYPE_TABLE
 *
 * ExportedType table identifier
 */
#define EXPORTED_TYPE_TABLE                     0x27

/**
 * @def MANIFEST_RESOURCE_TABLE
 *
 * ManifestResource table identifier
 */
#define MANIFEST_RESOURCE_TABLE                 0x28

/**
 * @def NESTED_CLASS_TABLE
 *
 * NestedClass table identifier
 */
#define NESTED_CLASS_TABLE                      0x29

/**
 * @def GENERIC_PARAM_TABLE
 *
 * GenericParam table identifier
 */
#define GENERIC_PARAM_TABLE                     0x2A

/**
 * @def GENERIC_PARAM_CONSTRAINT_TABLE
 *
 * GenericParamConstraint table identifier
 */
#define GENERIC_PARAM_CONSTRAINT_TABLE                  0x2C

/**
 * @def METHOD_SPEC_TABLE
 *
 * MethodSpec table identifier
 */
#define METHOD_SPEC_TABLE                       0x2B

/*****************************************************************************************************************************
                                                TOKENS
*****************************************************************************************************************************/
#define TYPE_DEF_OR_REF_TOKEN_SIZE_BOUND        16384
#define TYPE_OR_METHOD_DEF_TOKEN_SIZE_BOUND     32768
#define METHOD_DEF_OR_REF_TOKEN_SIZE_BOUND      32768

/*****************************************************************************************************************************
                                                ELEMENT TYPE
*****************************************************************************************************************************/
#define ELEMENT_TYPE_END                        0x00
#define ELEMENT_TYPE_VOID                       0x01
#define ELEMENT_TYPE_BOOLEAN                    0x02
#define ELEMENT_TYPE_CHAR                       0x03
#define ELEMENT_TYPE_I1                         0x04
#define ELEMENT_TYPE_U1                         0x05
#define ELEMENT_TYPE_I2                         0x06
#define ELEMENT_TYPE_U2                         0x07
#define ELEMENT_TYPE_I4                         0x08
#define ELEMENT_TYPE_U4                         0x09
#define ELEMENT_TYPE_I8                         0x0A
#define ELEMENT_TYPE_U8                         0x0B
#define ELEMENT_TYPE_R4                         0x0C
#define ELEMENT_TYPE_R8                         0x0D
#define ELEMENT_TYPE_STRING                     0x0E
#define ELEMENT_TYPE_PTR                        0x0F
#define ELEMENT_TYPE_BYREF                      0x10
#define ELEMENT_TYPE_VALUETYPE                  0x11
#define ELEMENT_TYPE_CLASS                      0x12
#define ELEMENT_TYPE_VAR                        0x13
#define ELEMENT_TYPE_ARRAY                      0x14
#define ELEMENT_TYPE_GENERICINST                0x15
#define ELEMENT_TYPE_TYPEDBYREF                 0x16
#define ELEMENT_TYPE_I                          0x18
#define ELEMENT_TYPE_U                          0x19
#define ELEMENT_TYPE_FNPTR                      0x1B
#define ELEMENT_TYPE_OBJECT                     0x1C
#define ELEMENT_TYPE_SZARRAY                    0x1D
#define ELEMENT_TYPE_MVAR                       0x1E
#define ELEMENT_TYPE_CMOD_REQD                  0x1F
#define ELEMENT_TYPE_CMOD_OPT                   0x20
#define ELEMENT_TYPE_SENTINEL                   0x41
#define ELEMENT_TYPE_PINNED                     0x45

/*****************************************************************************************************************************
                        TYPE ACCESSIBILITY CONSTRAINTS - T_ROW_TYPE_DEF_TABLE - VisibilityMask
*****************************************************************************************************************************/
#define NOT_PUBLIC_TYPE                 0x00000000
#define PUBLIC_TYPE                     0x00000001
#define NESTED_PUBLIC_TYPE              0x00000002
#define NESTED_PRIVATE_TYPE             0x00000003
#define NESTED_FAMILY_TYPE              0x00000004
#define NESTED_ASSEMBLY_TYPE            0x00000005
#define NESTED_FAM_AND_ASSEM_TYPE       0x00000006
#define NESTED_FAM_OR_ASSEM_TYPE        0x00000007

/*****************************************************************************************************************************
                        CLASS LAYOUT ATTRIBUTES - T_ROW_TYPE_DEF_TABLE - LayoutMask
*****************************************************************************************************************************/
#define LAYOUT_MASK                     0x00000018
#define AUTOLAYOUT                      0x00000000
#define SEQUENTIAL_LAYOUT               0x00000008
#define EXPLICIT_LAYOUT                 0x00000010

/*****************************************************************************************************************************
                        CLASS SEMANTIC ATTRIBUTES - T_ROW_TYPE_DEF_TABLE - ClassSemanticMask
*****************************************************************************************************************************/
#define CLASS_TYPE                      0x00000000
#define INTERFACE_TYPE                  0x00000020

/*****************************************************************************************************************************
                        SPECIAL SEMANTICS - T_ROW_TYPE_DEF_TABLE
*****************************************************************************************************************************/
#define ABSTRACT_TYPE                   0x00000080
#define SEALED_TYPE                     0x00000100
#define SPECIAL_NAME                    0x00000400

/*******************************************************
                        CLASS INITIALIZATION ATTRIBUTES - T_ROW_TYPE_DEF_TABLE
*******************************************************/
#define BEFORE_FIELD_INIT               0x00100000

/*****************************************************************************************************************************
                        METHOD ACCESSIBILITY CONSTRAINTS - T_ROW_METHOD_DEF_TABLE - memberAccessMask
*****************************************************************************************************************************/
#define COMPILER_CONTROLLED_METHOD      0x0000
#define PRIVATE_METHOD                  0x0001
#define FAM_AND_ASSEM_METHOD            0x0002
#define ASSEM_METHOD                    0x0003
#define FAMILY_METHOD                   0x0004
#define FAM_OR_ASSEM_METHOD             0x0005
#define PUBLIC_METHOD                   0x0006

/*****************************************************************************************************************************
                        METHOD ACCESSIBILITY CONSTRAINTS - T_ROW_FIELD_TABLE - fieldAccessMask
*****************************************************************************************************************************/
#define COMPILER_CONTROLLED_FIELD       0x0000
#define PRIVATE_FIELD                   0x0001
#define FAM_AND_ASSEM_FIELD             0x0002
#define ASSEM_FIELD                     0x0003
#define FAMILY_FIELD                    0x0004
#define FAM_OR_ASSEM_FIELD              0x0005
#define PUBLIC_FIELD                    0x0006

/*****************************************************************************************************************************
                                          T_ROW_MANIFEST_TABLE - VisibilityMask
*****************************************************************************************************************************/
#define PUBLIC_MANIFEST                 0x0001
#define PRIVATE_MANIFEST                0x0002

/*****************************************************************************************************************************
                                          T_ROW_FILE_TABLE - VisibilityMask
*****************************************************************************************************************************/
#define CONTAINS_METADATA_FILE          0x0000
#define CONTAINS_NO_METADATA_FILE       0x0001

/*****************************************************************************************************************************
                                          T_GENERIC_PARAM_TABLE - FlagsMask
*****************************************************************************************************************************/
#define VARIANCE_MASK           0x0003
#define NONE    0x0000
#define COVARIANT       0x001
#define CONTRAVARIANT 0x002
#define SPECIAL_CONSTRAINT_MASK 0x001C
#define REFERENCE_TYPE_CONSTRAINT 0x0004
#define NOT_NULLABLE_VALUE_TYPE_CONSTRAINT 0x0008
#define DEFAULT_CONSTRUCTOR_CONSTRAINT 0x0010

/*****************************************************************************************************************************
                MACRO USED FOR DECODING : StandAloneMethodSig; MethodDefSig; MethodRefSig
*****************************************************************************************************************************/

#define HASTHIS         0x20
#define EXPLICITTHIS    0x40
#define DEFAULT         0x0
#define VARARG          0x5
#define C_CALL          0x1
#define CLI_STDCALL     0x2
#define THISCALL        0x3
#define CLI_FASTCALL    0x4
#define SENTINEL        0x41
#define GENERIC         0x10

/*****************************************************************************************************************************
                MACRO USED FOR DECODING : MethodSpec
*****************************************************************************************************************************/

#define GENERICINST             0x0A

/*****************************************************************************************************************************
                MACRO USED FOR DECODING : fieldSig
*****************************************************************************************************************************/

#define FIELD           0x6

/*****************************************************************************************************************************
                                        MACRO USE FOR DECODIG: memberTypes
*****************************************************************************************************************************/

#define MEMBER_TYPES_CONSTRUCTOR        0x0001
#define MEMBER_TYPES_EVENT              0x0002
#define MEMBER_TYPES_FIELD              0x0004
#define MEMBER_TYPES_METHOD             0x0008
#define MEMBER_TYPES_PROPERTY           0x0010
#define MEMBER_TYPES_TYPEINFO           0x0020
#define MEMBER_TYPES_CUSTOM             0x0040
#define MEMBER_TYPES_NESTEDTYPE         0x0080
#define MEMBER_TYPES_ALL                0x00BF

/******************************************************************************************************************************
                                                Kind of complex types
******************************************************************************************************************************/

#define IL_TYPE_COMPLEX_BYREF                   1
#define IL_TYPE_COMPLEX_PTR                     2
#define IL_TYPE_COMPLEX_ARRAY                   3
#define IL_TYPE_COMPLEX_ARRAY_CONTINUE          4
#define IL_TYPE_COMPLEX_CMOD_REQD               6
#define IL_TYPE_COMPLEX_CMOD_OPT                7
#define IL_TYPE_COMPLEX_PROPERTY                8
#define IL_TYPE_COMPLEX_SENTINEL                9
#define IL_TYPE_COMPLEX_PINNED                  10
#define IL_TYPE_COMPLEX_LOCALS                  11
#define IL_TYPE_COMPLEX_WITH                    12
#define IL_TYPE_COMPLEX_MVAR                    13
#define IL_TYPE_COMPLEX_VAR                     14
#define IL_TYPE_COMPLEX_METHOD                  16
#define IL_TYPE_COMPLEX_METHOD_SENTINEL         1

#endif /* ECMA_CONSTANT */


