/*
 * Copyright (C) 2008  Roberto Farina, Simone Campanoni
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
   *@file cil_opcodes.h
 *
 * CIL opcodes definition
 */

#ifndef CIL_OPCODES_H
#define CIL_OPCODES_H

/**
 * @def NOP_OPCODE
 *
 * nop opcode
 */
#define NOP_OPCODE                      0x00

/**
 * @def BREAK_OPCODE
 *
 * break opcode
 */
#define BREAK_OPCODE                    0x01

/**
 * @def LDARG0_OPCODE
 *
 * ldarg.0 opcode
 */
#define LDARG_0_OPCODE                  0x02

/**
 * @def LDARG1_OPCODE
 *
 * ldarg.1 opcode
 */
#define LDARG_1_OPCODE                  0x03

/**
 * @def LDARG2_OPCODE
 *
 * ldarg.2 opcode
 */
#define LDARG_2_OPCODE                  0x04

/**
 * @def LDARG3_OPCODE
 *
 * ldarg.3 opcode
 */
#define LDARG_3_OPCODE                  0x05

/**
 * @def LDLOC0_OPCODE
 *
 * ldloc.0 opcode
 */
#define LDLOC_0_OPCODE                  0x06

/**
 * @def LDLOC1_OPCODE
 *
 * ldloc.1 opcode
 */
#define LDLOC_1_OPCODE                  0x07

/**
 * @def LDLOC2_OPCODE
 *
 * ldloc.2 opcode
 */
#define LDLOC_2_OPCODE                  0x08

/**
 * @def LDLOC3_OPCODE
 *
 * ldloc.3 opcode
 */
#define LDLOC_3_OPCODE                  0x09

/**
 * @def STLOC0_OPCODE
 *
 * stloc.0 opcode
 */
#define STLOC_0_OPCODE                  0x0A

/**
 * @def STLOC1_OPCODE
 *
 * stloc.1 opcode
 */
#define STLOC_1_OPCODE                  0x0B

/**
 * @def STLOC2_OPCODE
 *
 * stloc.2 opcode
 */
#define STLOC_2_OPCODE                  0x0C

/**
 * @def STLOC3_OPCODE
 *
 * stloc.3 opcode
 */
#define STLOC_3_OPCODE                  0x0D

/**
 * @def LDARG_S_OPCODE
 *
 * ldarg.s opcode
 */
#define LDARG_S_OPCODE                  0x0E

/**
 * @def LDARGA_S_OPCODE
 *
 * ldarga.s opcode
 */
#define LDARGA_S_OPCODE                 0x0F

/**
 * @def STARG_S_OPCODE
 *
 * starg.s opcode
 */
#define STARG_S_OPCODE                  0x10

/**
 * @def LDLOC_S_OPCODE
 *
 * ldloc.s opcode
 */
#define LDLOC_S_OPCODE                  0x11

/**
 * @def LDLOCA_S_OPCODE
 *
 * ldloca.s opcode
 */
#define LDLOCA_S_OPCODE                 0x12

/**
 * @def STLOC_S_OPCODE
 *
 * stloc.s opcode
 */
#define STLOC_S_OPCODE                  0x13

/**
 * @def LDNULL_OPCODE
 *
 * ldnull opcode
 */
#define LDNULL_OPCODE                   0x14

/**
 * @def LDC_I4_M1_OPCODE
 *
 * ldc.i4.m1 opcode
 */
#define LDC_I4_M1_OPCODE                0x15

/**
 * @def LDC_I4_0_OPCODE
 *
 * ldc.i4.0 opcode
 */
#define LDC_I4_0_OPCODE                 0x16

/**
 * @def LDC_I4_1_OPCODE
 *
 * ldc.i4.1 opcode
 */
#define LDC_I4_1_OPCODE                 0x17

/**
 * @def LDC_I4_2_OPCODE
 *
 * ldc.i4.2 opcode
 */
#define LDC_I4_2_OPCODE                 0x18

/**
 * @def LDC_I4_3_OPCODE
 *
 * ldc.i4.3 opcode
 */
#define LDC_I4_3_OPCODE                 0x19

/**
 * @def LDC_I4_4_OPCODE
 *
 * ldc.i4.4 opcode
 */
#define LDC_I4_4_OPCODE                 0x1A

/**
 * @def LDC_I4_5_OPCODE
 *
 * ldc.i4.5 opcode
 */
#define LDC_I4_5_OPCODE                 0x1B

/**
 * @def LDC_I4_6_OPCODE
 *
 * ldc.i4.6 opcode
 */
#define LDC_I4_6_OPCODE                 0x1C

/**
 * @def LDC_I4_7_OPCODE
 *
 * ldc.i4.7 opcode
 */
#define LDC_I4_7_OPCODE                 0x1D

/**
 * @def LDC_I4_8_OPCODE
 *
 * ldc.i4.8 opcode
 */
#define LDC_I4_8_OPCODE                 0x1E

/**
 * @def LDC_I4_s_OPCODE
 *
 * ldc.i4.s opcode
 */
#define LDC_I4_S_OPCODE                 0x1F

/**
 * @def LDC.I4_OPCODE
 *
 * ldc.i4 opcode
 */
#define LDC_I4_OPCODE                   0x20

/**
 * @def LDC.I8_OPCODE
 *
 * ldc.i8 opcode
 */
#define LDC_I8_OPCODE                   0x21

/**
 * @def LDC.R4_OPCODE
 *
 * ldc.r4 opcode
 */
#define LDC_R4_OPCODE                   0x22

/**
 * @def LDC.R8_OPCODE
 *
 * ldc.r8 opcode
 */
#define LDC_R8_OPCODE                   0x23

/**
 * @def DUP_OPCODE
 *
 * dup opcode
 */
#define DUP_OPCODE                      0x25

/**
 * @def POP_OPCODE
 *
 * pop opcode
 */
#define POP_OPCODE                      0x26

/**
 * @def JMP_OPCODE
 *
 * jmp opcode
 */
#define JMP_OPCODE                      0x27

/**
 * @def CALL_OPCODE
 *
 * call opcode
 */
#define CALL_OPCODE                     0x28
/**
 * @def CALLI_OPCODE
 *
 * calli opcode
 */
#define CALLI_OPCODE                    0x29
/**
 * @def RET_OPCODE
 *
 * ret opcode
 */
#define RET_OPCODE                      0x2A

/**
 * @def BR_S_OPCODE
 *
 * br.s opcode
 */
#define BR_S_OPCODE                     0x2B

/**
 * @def BRFALSE_S_OPCODE
 *
 * brfalse.s opcode
 */
#define BRFALSE_S_OPCODE                0x2C

/**
 * @def JMP_OPCODE
 *
 * jmp opcode
 */
#define BRTRUE_S_OPCODE                 0x2D

/**
 * @def BEQ_S_OPCODE
 *
 * beq.s opcode
 */
#define BEQ_S_OPCODE                    0x2E

/**
 * @def BGE_S_OPCODE
 *
 * bge.s opcode
 */
#define BGE_S_OPCODE                    0x2F

/**
 * @def BGT_S_OPCODE
 *
 * bgt.s opcode
 */
#define BGT_S_OPCODE                    0x30

/**
 * @def BLE_S_OPCODE
 *
 * ble.s opcode
 */
#define BLE_S_OPCODE                    0x31

/**
 * @def BLT_S_OPCODE
 *
 * blt.s opcode
 */
#define BLT_S_OPCODE                    0x32

/**
 * @def BNE_UN_S_OPCODE
 *
 * bne.un.s opcode
 */
#define BNE_UN_S_OPCODE                 0x33

/**
 * @def BGE_UN_S_OPCODE
 *
 * bge.un.s opcode
 */
#define BGE_UN_S_OPCODE                 0x34

/**
 * @def BGT_UN_S_OPCODE
 *
 * bgt.un.s opcode
 */
#define BGT_UN_S_OPCODE                 0x35

/**
 * @def BLE_UN_S_OPCODE
 *
 * ble.un.s opcode
 */
#define BLE_UN_S_OPCODE                 0x36

/**
 * @def BLT_UN_S_OPCODE
 *
 * blt.un.s opcode
 */
#define BLT_UN_S_OPCODE                 0x37

/**
 * @def BR_OPCODE
 *
 * br opcode
 */
#define BR_OPCODE                       0x38

/**
 * @def BRFALSE_OPCODE
 *
 * brfalse opcode
 */
#define BRFALSE_OPCODE                  0x39

/**
 * @def BRTRUE_OPCODE
 *
 * brtrue opcode
 */
#define BRTRUE_OPCODE                   0x3A

/**
 * @def BEQ_OPCODE
 *
 * beq opcode
 */
#define BEQ_OPCODE                      0x3B

/**
 * @def BGE_OPCODE
 *
 * bge opcode
 */
#define BGE_OPCODE                      0x3C

/**
 * @def BGT_OPCODE
 *
 * bgt opcode
 */
#define BGT_OPCODE                      0x3D

/**
 * @def BLE_OPCODE
 *
 * ble opcode
 */
#define BLE_OPCODE                      0x3E

/**
 * @def BLT_OPCODE
 *
 * blt opcode
 */
#define BLT_OPCODE                      0x3F

/**
 * @def BNE_UN_OPCODE
 *
 * bne.un opcode
 */
#define BNE_UN_OPCODE                   0x40

/**
 * @def BGE_UN_OPCODE
 *
 * bge.un opcode
 */
#define BGE_UN_OPCODE                   0x41

/**
 * @def BGT_UN_OPCODE
 *
 * bgt.un opcode
 */
#define BGT_UN_OPCODE                   0x42

/**
 * @def BLE_UN_OPCODE
 *
 * ble.un opcode
 */
#define BLE_UN_OPCODE                   0x43

/**
 * @def BLT_UN_OPCODE
 *
 * blt.un opcode
 */
#define BLT_UN_OPCODE                   0x44

/**
 * @def SWITCH_OPCODE
 *
 * switch opcode
 */
#define SWITCH_OPCODE                   0x45

/**
 * @def LDIND_I1_OPCODE
 *
 * ldind.i1 opcode
 */
#define LDIND_I1_OPCODE                 0x46

/**
 * @def LDIND_U1_OPCODE
 *
 * ldind.u1 opcode
 */
#define LDIND_U1_OPCODE                 0x47

/**
 * @def LDIND_I2_OPCODE
 *
 * ldind.i2 opcode
 */
#define LDIND_I2_OPCODE                 0x48

/**
 * @def LDIND_U2_OPCODE
 *
 * ldind.u2 opcode
 */
#define LDIND_U2_OPCODE                 0x49

/**
 * @def LDIND_I4_OPCODE
 *
 * ldind.i4 opcode
 */
#define LDIND_I4_OPCODE                 0x4A

/**
 * @def LDIND_U4_OPCODE
 *
 * ldind.u4 opcode
 */
#define LDIND_U4_OPCODE                 0x4B

/**
 * @def LDIND_I8_OPCODE
 *
 * ldind.i8 opcode
 */
#define LDIND_I8_OPCODE                 0x4C

/**
 * @def LDIND_I_OPCODE
 *
 * ldind.i opcode
 */
#define LDIND_I_OPCODE                  0x4D

/**
 * @def LDIND_R4_OPCODE
 *
 * ldind.r4 opcode
 */
#define LDIND_R4_OPCODE                 0x4E

/**
 * @def LDIND_R8_OPCODE
 *
 * ldind.r8 opcode
 */
#define LDIND_R8_OPCODE                 0x4F

/**
 * @def LDIND_REF_OPCODE
 *
 * ldind.ref opcode
 */
#define LDIND_REF_OPCODE                0x50

/**
 * @def STIND_REF_OPCODE
 *
 * stind.ref opcode
 */
#define STIND_REF_OPCODE                0x51

/**
 * @def STIND_I1_OPCODE
 *
 * stind.i1 opcode
 */
#define STIND_I1_OPCODE                 0x52

/**
 * @def STIND_I2_OPCODE
 *
 * stind.i2 opcode
 */
#define STIND_I2_OPCODE                 0x53

/**
 * @def STIND_I4_OPCODE
 *
 * stind.i4 opcode
 */
#define STIND_I4_OPCODE                 0x54

/**
 * @def STIND_I8_OPCODE
 *
 * stind.i8 opcode
 */
#define STIND_I8_OPCODE                 0x55

/**
 * @def STIND_R4_OPCODE
 *
 * stind.r4 opcode
 */
#define STIND_R4_OPCODE                 0x56

/**
 * @def STIND_R8_OPCODE
 *
 * stind.r8 opcode
 */
#define STIND_R8_OPCODE                 0x57

/**
 * @def ADD_OPCODE
 *
 * add opcode
 */
#define ADD_OPCODE                      0x58

/**
 * @def SUB_OPCODE
 *
 * sub opcode
 */
#define SUB_OPCODE                      0x59

/**
 * @def MUL_OPCODE
 *
 * mul opcode
 */
#define MUL_OPCODE                      0x5A

/**
 * @def DIV_OPCODE
 *
 * div opcode
 */
#define DIV_OPCODE                      0x5B

/**
 * @def DIV_UN_OPCODE
 *
 * div.un opcode
 */
#define DIV_UN_OPCODE                   0x5C

/**
 * @def REM_OPCODE
 *
 * rem opcode
 */
#define REM_OPCODE                      0x5D

/**
 * @def REM_UN_OPCODE
 *
 * rem.un opcode
 */
#define REM_UN_OPCODE                   0x5E

/**
 * @def AND_OPCODE
 *
 * and opcode
 */
#define AND_OPCODE                      0x5F

/**
 * @def OR_OPCODE
 *
 * or opcode
 */
#define OR_OPCODE                       0x60

/**
 * @def XOR_OPCODE
 *
 * xor opcode
 */
#define XOR_OPCODE                      0x61

/**
 * @def SHL_OPCODE
 *
 * shl opcode
 */
#define SHL_OPCODE                      0x62

/**
 * @def SHR_OPCODE
 *
 * shr opcode
 */
#define SHR_OPCODE                      0x63

/**
 * @def SHR_UN_OPCODE
 *
 * shr.un opcode
 */
#define SHR_UN_OPCODE                   0x64

/**
 * @def NEG_OPCODE
 *
 * neg opcode
 */
#define NEG_OPCODE                      0x65

/**
 * @def NOT_OPCODE
 *
 * not opcode
 */
#define NOT_OPCODE                      0x66

/**
 * @def CONV_I1_OPCODE
 *
 * conv.i1 opcode
 */
#define CONV_I1_OPCODE                  0x67

/**
 * @def CONV_I2_OPCODE
 *
 * conv.i2 opcode
 */
#define CONV_I2_OPCODE                  0x68

/**
 * @def CONV_I4_OPCODE
 *
 * conv.i4 opcode
 */
#define CONV_I4_OPCODE                  0x69

/**
 * @def CONV_I8_OPCODE
 *
 * conv.i8 opcode
 */
#define CONV_I8_OPCODE                  0x6A

/**
 * @def CONV_R4_OPCODE
 *
 * conv.r4 opcode
 */
#define CONV_R4_OPCODE                  0x6B

/**
 * @def CONV_R8_OPCODE
 *
 * conv.r8 opcode
 */
#define CONV_R8_OPCODE                  0x6C

/**
 * @def CONV_U4_OPCODE
 *
 * conv.u4 opcode
 */
#define CONV_U4_OPCODE                  0x6D

/**
 * @def CONV_U8_OPCODE
 *
 * conv.u8 opcode
 */
#define CONV_U8_OPCODE                  0x6E

/**
 * @def CALLVIRT_OPCODE
 *
 * callvirt opcode
 */
#define CALLVIRT_OPCODE                 0x6F

/**
 * @def CPOBJ_OPCODE
 *
 * cpobj opcode
 */
#define CPOBJ_OPCODE                    0x70

/**
 * @def LDOBJ_OPCODE
 *
 * ldobj opcode
 */
#define LDOBJ_OPCODE                    0x71

/**
 * @def LDSTR_OPCODE
 *
 * ldstr opcode
 */
#define LDSTR_OPCODE                    0x72

/**
 * @def NEWOBJ_OPCODE
 *
 * newobj opcode
 */
#define NEWOBJ_OPCODE                   0x73

/**
 * @def CASTCLASS_OPCODE
 *
 * castclass opcode
 */
#define CASTCLASS_OPCODE                0x74

/**
 * @def ISINST_OPCODE
 *
 * isinst opcode
 */
#define ISINST_OPCODE                   0x75

/**
 * @def CONV_R_UN_OPCODE
 *
 * conv.r.un opcode
 */
#define CONV_R_UN_OPCODE                0x76

/**
 * @def UNBOX_OPCODE
 *
 * unbox opcode
 */
#define UNBOX_OPCODE                    0x79

/**
 * @def THROW_OPCODE
 *
 * thorw opcode
 */
#define THROW_OPCODE                    0x7A

/**
 * @def LDFLD_OPCODE
 *
 * ldfld opcode
 */
#define LDFLD_OPCODE                    0x7B

/**
 * @def LDFLDA_OPCODE
 *
 * ldflda opcode
 */
#define LDFLDA_OPCODE                   0x7C

/**
 * @def STFLD_OPCODE
 *
 * stfld opcode
 */
#define STFLD_OPCODE                    0x7D

/**
 * @def LDSFLD_OPCODE
 *
 * ldsfld opcode
 */
#define LDSFLD_OPCODE                   0x7E

/**
 * @def LDSFLDA_OPCODE
 *
 * ldsflda opcode
 */
#define LDSFLDA_OPCODE                  0x7F

/**
 * @def STSFLD_OPCODE
 *
 * stsfld opcode
 */
#define STSFLD_OPCODE                   0x80

/**
 * @def STOBJ_OPCODE
 *
 * stobj opcode
 */
#define STOBJ_OPCODE                    0x81

/**
 * @def CONV_OVF_I1_UN_OPCODE
 *
 * conv.ovf.i1.un opcode
 */
#define CONV_OVF_I1_UN_OPCODE           0x82

/**
 * @def CONV_OVF_I2_UN_OPCODE
 *
 * conv.ovf.i2.un opcode
 */
#define CONV_OVF_I2_UN_OPCODE           0x83
/**
 * @def CONV_OVF_I4_UN_OPCODE
 *
 * conv.ovf.i4.un opcode
 */
#define CONV_OVF_I4_UN_OPCODE           0x84

/**
 * @def CONV_OVF_I8_UN_OPCODE
 *
 * conv.ovf.i8.un opcode
 */
#define CONV_OVF_I8_UN_OPCODE           0x85

/**
 * @def CONV_OVF_U1_UN_OPCODE
 *
 * conv.ovf.u1.un opcode
 */
#define CONV_OVF_U1_UN_OPCODE           0x86

/**
 * @def CONV_OVF_U2_UN_OPCODE
 *
 * conv.ovf.u2.un opcode
 */
#define CONV_OVF_U2_UN_OPCODE           0x87

/**
 * @def CONV_OVF_U4_UN_OPCODE
 *
 * conv.ovf.u4.un opcode
 */
#define CONV_OVF_U4_UN_OPCODE           0x88

/**
 * @def CONV_OVF_U8_UN_OPCODE
 *
 * conv.ovf.u8.un opcode
 */
#define CONV_OVF_U8_UN_OPCODE           0x89

/**
 * @def CONV_OVF_I_UN_OPCODE
 *
 * conv.ovf.i.un opcode
 */
#define CONV_OVF_I_UN_OPCODE            0x8A

/**
 * @def CONV_OVF_U_UN_OPCODE
 *
 * conv.ovf.u.un opcode
 */
#define CONV_OVF_U_UN_OPCODE            0x8B

/**
 * @def BOX_OPCODE
 *
 * box opcode
 */
#define BOX_OPCODE                      0x8C

/**
 * @def NEWARR_OPCODE
 *
 * newarr opcode
 */
#define NEWARR_OPCODE                   0x8D

/**
 * @def LDLEN_OPCODE
 *
 * ldlen opcode
 */
#define LDLEN_OPCODE                    0x8E

/**
 * @def LDELEMA_OPCODE
 *
 * ldelema opcode
 */
#define LDELEMA_OPCODE                  0x8F

/**
 * @def LDELEM_I1_OPCODE
 *
 * ldelem.i1 opcode
 */
#define LDELEM_I1_OPCODE                0x90

/**
 * @def LDELEM_U1_OPCODE
 *
 * ldelem.u1 opcode
 */
#define LDELEM_U1_OPCODE                0x91

/**
 * @def LDELEM_I2_OPCODE
 *
 * ldelem.i2 opcode
 */
#define LDELEM_I2_OPCODE                0x92

/**
 * @def LDELEM_U2_OPCODE
 *
 * ldelem.u2 opcode
 */
#define LDELEM_U2_OPCODE                0x93

/**
 * @def LDELEM_I4_OPCODE
 *
 * ldelem.i4 opcode
 */
#define LDELEM_I4_OPCODE                0x94

/**
 * @def LDELEM_U4_OPCODE
 *
 * ldelem.u4 opcode
 */
#define LDELEM_U4_OPCODE                0x95

/**
 * @def LDELEM_I8_OPCODE
 *
 * ldelem.i8 opcode
 */
#define LDELEM_I8_OPCODE                0x96

/**
 * @def LDELEM_I_OPCODE
 *
 * ldelem.i opcode
 */
#define LDELEM_I_OPCODE                 0x97

/**
 * @def LDELEM_R4_OPCODE
 *
 * ldelem.r4 opcode
 */
#define LDELEM_R4_OPCODE                0x98

/**
 * @def LDELEM_R8_OPCODE
 *
 * ldelem.r8 opcode
 */
#define LDELEM_R8_OPCODE                0x99

/**
 * @def LDELEM_REF_OPCODE
 *
 * ldelem.ref opcode
 */
#define LDELEM_REF_OPCODE               0x9A

/**
 * @def STELEM_I_OPCODE
 *
 * stelem.i opcode
 */
#define STELEM_I_OPCODE                 0x9B

/**
 * @def STELEM_I1_OPCODE
 *
 * stelem.i1 opcode
 */
#define STELEM_I1_OPCODE                0x9C

/**
 * @def STELEM_I2_OPCODE
 *
 * stelem.i2 opcode
 */
#define STELEM_I2_OPCODE                0x9D

/**
 * @def STELEM_I4_OPCODE
 *
 * stelem.i4 opcode
 */
#define STELEM_I4_OPCODE                0x9E

/**
 * @def STELEM_I8_OPCODE
 *
 * stelem.i8 opcode
 */
#define STELEM_I8_OPCODE                0x9F

/**
 * @def STELEM_R4_OPCODE
 *
 * stelem.r4 opcode
 */
#define STELEM_R4_OPCODE                0xA0

/**
 * @def STELEM_R8_OPCODE
 *
 * stelem.r8 opcode
 */
#define STELEM_R8_OPCODE                0xA1

/**
 * @def STELEM_REF_OPCODE
 *
 * stelem.ref opcode
 */
#define STELEM_REF_OPCODE               0xA2

/**
 * @def LDELEM_OPCODE
 *
 * ldelem opcode
 */
#define LDELEM_OPCODE                   0xA3

/**
 * @def STELEM_OPCODE
 *
 * stelem opcode
 */
#define STELEM_OPCODE                   0xA4

/**
 * @def UNBOX_ANY_OPCODE
 *
 * unbox.any opcode
 */
#define UNBOX_ANY_OPCODE                0xA5

/**
 * @def CONV_OVF_I1_OPCODE
 *
 * conv.ovf.i1 opcode
 */
#define CONV_OVF_I1_OPCODE              0xB3

/**
 * @def CONV_OVF_U1_OPCODE
 *
 * conv.ovf.u1 opcode
 */
#define CONV_OVF_U1_OPCODE              0xB4

/**
 * @def CONV_OVF_I2_OPCODE
 *
 * conv.ovf.i2 opcode
 */
#define CONV_OVF_I2_OPCODE              0xB5

/**
 * @def CONV_OVF_U2_OPCODE
 *
 * conv.ovf.u2 opcode
 */
#define CONV_OVF_U2_OPCODE              0xB6

/**
 * @def CONV_OVF_I4_OPCODE
 *
 * conv.ovf.i4 opcode
 */
#define CONV_OVF_I4_OPCODE              0xB7

/**
 * @def CONV_OVF_U4_OPCODE
 *
 * conv.ovf.u4 opcode
 */
#define CONV_OVF_U4_OPCODE              0xB8

/**
 * @def CONV_OVF_I8_OPCODE
 *
 * conv.ovf.i8 opcode
 */
#define CONV_OVF_I8_OPCODE              0xB9

/**
 * @def CONV_OVF_U8_OPCODE
 *
 * conv.ovf.u8 opcode
 */
#define CONV_OVF_U8_OPCODE              0xBA

/**
 * @def REFANYVAL_OPCODE
 *
 * refanyval opcode
 */
#define REFANYVAL_OPCODE                0xC2

/**
 * @def CKFINITE_OPCODE
 *
 * ckfinite opcode
 */
#define CKFINITE_OPCODE                 0xC3

/**
 * @def MKREFANY_OPCODE
 *
 * mkrefany opcode
 */
#define MKREFANY_OPCODE                 0xC6

/**
 * @def LDTOKEN_OPCODE
 *
 * ldtoken opcode
 */
#define LDTOKEN_OPCODE                  0xD0

/**
 * @def CONV_U2_OPCODE
 *
 * conv.u2 opcode
 */
#define CONV_U2_OPCODE                  0xD1

/**
 * @def CONV_U1_OPCODE
 *
 * conv.u1 opcode
 */
#define CONV_U1_OPCODE                  0xD2

/**
 * @def CONV_I_OPCODE
 *
 * conv.i opcode
 */
#define CONV_I_OPCODE                   0xD3

/**
 * @def CONV_OVF_I_OPCODE
 *
 * conv.ovf.i opcode
 */
#define CONV_OVF_I_OPCODE               0xD4

/**
 * @def CONV_OVF_U_OPCODE
 *
 * conv.ovf.u opcode
 */
#define CONV_OVF_U_OPCODE               0xD5

/**
 * @def ADD_OVF_OPCODE
 *
 * add.ovf opcode
 */
#define ADD_OVF_OPCODE                  0xD6

/**
 * @def ADD_OVF_UN_OPCODE
 *
 * add.ovf.un opcode
 */
#define ADD_OVF_UN_OPCODE               0xD7

/**
 * @def MUL_OVF_OPCODE
 *
 * mul.ovf opcode
 */
#define MUL_OVF_OPCODE                  0xD8

/**
 * @def MUL_OVF_UN_OPCODE
 *
 * mul.ovf.un opcode
 */
#define MUL_OVF_UN_OPCODE               0xD9

/**
 * @def SUB_OVF_OPCODE
 *
 * sub.ovf opcode
 */
#define SUB_OVF_OPCODE                  0xDA

/**
 * @def SUB_OVF_UN_OPCODE
 *
 * sub.ovf.un opcode
 */
#define SUB_OVF_UN_OPCODE               0xDB

/**
 * @def ENDFINALLY_OPCODE
 *
 * endfinally opcode
 */
#define ENDFINALLY_OPCODE               0xDC

/**
 * @def LEAVE_OPCODE
 *
 * leave opcode
 */
#define LEAVE_OPCODE                    0xDD

/**
 * @def LEAVE_S_OPCODE
 *
 * leave.s opcode
 */
#define LEAVE_S_OPCODE                  0xDE

/**
 * @def STIND_I_OPCODE
 *
 * stind.i opcode
 */
#define STIND_I_OPCODE                  0xDF

/**
 * @def CONV_U_OPCODE
 *
 * conv.u opcode
 */
#define CONV_U_OPCODE                   0xE0

/**
 * @def PREFIXREF_OPCODE
 *
 * prefixref opcode
 */
#define PREFIXREF_OPCODE                0xFF

/* 0xFE XX opcodes */

/**
 * @def FE_OPCODE
 *
 * 0xFE XX opcodes prefix
 */
#define FE_OPCODE                       0xFE

/**
 * @def ARGLIST_OPCODE
 *
 * arglist opcode
 */
#define ARGLIST_OPCODE                  0x00

/**
 * @def CEQ_OPCODE
 *
 * ceq opcode
 */
#define CEQ_OPCODE                      0x01

/**
 * @def CGT_OPCODE
 *
 * cgt opcode
 */
#define CGT_OPCODE                      0x02

/**
 * @def CGT_UN_OPCODE
 *
 * cgt.un opcode
 */
#define CGT_UN_OPCODE                   0x03

/**
 * @def CLT_OPCODE
 *
 * clt opcode
 */
#define CLT_OPCODE                      0x04

/**
 * @def CLT_UN_OPCODE
 *
 * clt.un opcode
 */
#define CLT_UN_OPCODE                   0x05

/**
 * @def LDFTN_OPCODE
 *
 * ldftn opcode
 */
#define LDFTN_OPCODE                    0x06

/**
 * @def LDVIRTFTN_OPCODE
 *
 * ldvirtftn opcode
 */
#define LDVIRTFTN_OPCODE                0x07

/**
 * @def LDARG_OPCODE
 *
 * ldarg opcode
 */
#define LDARG_OPCODE                    0x09

/**
 * @def LDARGA_OPCODE
 *
 * ldarga opcode
 */
#define LDARGA_OPCODE                   0x0A

/**
 * @def STARG_OPCODE
 *
 * starg opcode
 */
#define STARG_OPCODE                    0x0B

/**
 * @def LDLOC_OPCODE
 *
 * ldloc opcode
 */
#define LDLOC_OPCODE                    0x0C

/**
 * @def LDLOCA_OPCODE
 *
 * ldloca opcode
 */
#define LDLOCA_OPCODE                   0x0D

/**
 * @def STLOC_OPCODE
 *
 * stloc opcode
 */
#define STLOC_OPCODE                    0x0E

/**
 * @def LDCALLOC_OPCODE
 *
 * ldcalloc opcode
 */
#define LDCALLOC_OPCODE                 0x0F

/**
 * @def ENDFILTER_OPCODE
 *
 * endfilter opcode
 */
#define ENDFILTER_OPCODE                0x11

/**
 * @def UNALIGNED_OPCODE
 *
 * unaligned opcode
 */
#define UNALIGNED_OPCODE                0x12

/**
 * @def VOLATILE_OPCODE
 *
 * volatile opcode
 */
#define VOLATILE_OPCODE                 0x13

/**
 * @def TAIL_OPCODE
 *
 * tail opcode
 */
#define TAIL_OPCODE                     0x14

/**
 * @def INITOBJ_OPCODE
 *
 * initobj opcode
 */
#define INITOBJ_OPCODE                  0x15

/**
 * @def CONSTRAINED_OPCODE
 *
 * constrained opcode
 */
#define CONSTRAINED_OPCODE              0x16

/**
 * @def CPBLK_OPCODE
 *
 * cpblk opcode
 */
#define CPBLK_OPCODE                    0x17

/**
 * @def INITBLK_OPCODE
 *
 * initblk opcode
 */
#define INITBLK_OPCODE                  0x18

/**
 * @def NO_OPCODE
 *
 * no opcode
 */
#define NO_OPCODE                       0x19

/**
 * @def RETHROW_OPCODE
 *
 * rethrow opcode
 */
#define RETHROW_OPCODE                  0x1A

/**
 * @def SIZEOF_OPCODE
 *
 * sizeof opcode
 */
#define SIZEOF_OPCODE                   0x1C

/**
 * @def REFANYTYPE_OPCODE
 *
 * refanytype opcode
 */
#define REFANYTYPE_OPCODE               0x1D

/**
 * @def READONLY_OPCODE
 *
 * readonly opcode
 */
#define READONLY_OPCODE                 0x1E

#endif /* CIL_OPCODES_H */
