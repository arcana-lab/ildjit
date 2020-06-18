/*
 * Copyright (C) 2005 - 2012  Campanoni Simone, Di Biagio Andrea, Farina Roberto, Tartara Michele
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
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <ir_language.h>
#include <error_codes.h>
#include <metadata/metadata_types.h>
#include <ecma_constant.h>
#include <cil_opcodes.h>
#include <compiler_memory_manager.h>
#include <ir_virtual_machine.h>
#include <garbage_collector.h>
#include <cil_stack.h>
#include <platform_API.h>
#include <ir_optimization_interface.h>

// My headers
#include <lib_compilerNativeCall.h>
#include <runtime.h>
#include <cil_ir_translator.h>
#include <iljit.h>
#include <general_tools.h>
#include <system_manager.h>
// End

#define DIM_BUF 1024

/* unaligned prefix values */
#define UNALIGNED_DONOTCHECK_ALIGNMENT  9
#define UNALIGNED_DEFAULT_ALIGNMENT     0
#define UNALIGNED_BYTE_ALIGNMENT        1
#define UNALIGNED_DOUBLEBYTE_ALIGNMENT  2
#define UNALIGNED_QUADBYTE_ALIGNMENT    4

static inline JITBOOLEAN internal_isValueType (TypeDescriptor *ilType);
static inline JITINT8 convertEnumIntoInt32 (ir_item_t *item);
static inline void print_stack (t_stack *stack, JITUINT32 parameters, JITUINT32 locals);
static inline void print_stack_element (t_stack *stack, JITUINT32 elementID);
static inline JITINT16 create_call_IO (CLIManager_t *cliManager, ir_instruction_t *inst, Method ilMethod, t_stack *stack, JITBOOLEAN created, Method creatorMethod, ir_signature_t *signature_of_the_jump_method, JITUINT32 bytes_read, CILStack cilStack, JITUINT32 *current_label_ID, XanList *labels);
static inline JITINT16 decode_locals_signature (CLIManager_t *cliManager, JITUINT32 local_var_sig_tok, GenericContainer *container, t_binary_information *binary, t_stack *stack, Method method);
static inline void print_labels (XanList *labels);
static inline void labels_decrement_counter (XanList *labels, JITUINT32 value);
static inline void invalidate_blocks_starting_at_label (XanList *labels, t_label *label);
static inline void invalidate_An_Handler (XanList *labels, t_try_handler *current_handler);
static inline void invalidate_A_block (XanList *labels, t_try_block *block);
static inline JITBOOLEAN coerce_operands_for_binary_operation (CLIManager_t *cliManager, Method method, JITUINT32 bytes_read, t_stack *stack);
static inline JITBOOLEAN coerce_operands_for_binary_operation_to_unsigned_types (CLIManager_t *cliManager, Method method, JITUINT32 bytes_read, t_stack *stack, JITBOOLEAN force_stackable_type);
static inline JITBOOLEAN _perform_conversion (CLIManager_t *cliManager, Method method, JITUINT32 bytes_read, t_stack *stack, JITUINT32 toType, JITBOOLEAN with_overflow_check, ir_instruction_t *before);
static inline JITBOOLEAN _translate_binary_operation (Method method, JITUINT32 bytes_read, t_stack *stack, JITUINT32 IRop, JITUINT32 return_internal_type);
static inline JITBOOLEAN _translate_arithmetic_operation (CLIManager_t *cliManager, Method method, JITUINT32 bytes_read, t_stack *stack, JITUINT32 IRop);
static inline JITBOOLEAN _translate_arithmetic_operation_un (CLIManager_t *cliManager, Method method, JITUINT32 bytes_read, t_stack *stack, JITUINT32 IRop);
static inline t_label * fetch_label (XanList *labels, JITUINT32 ir_position);
static inline ir_instruction_t * find_insert_point (Method method, JITUINT32 insert_point);
static inline t_label * find_label_at_position (XanList *labels, JITUINT32 bytes_offset);
static inline JITINT16 decode_exceptions (CLIManager_t *cliManager, CILStack cilStack, GenericContainer *container, t_binary_information *binary, JITUINT32 body_size, XanList *labels, JITUINT32 *current_label_ID, Method method, t_stack *stack);
static inline t_try_block * find_inner_label (XanList *blocks, JITUINT32 current_offset);
static inline t_try_block * find_next_inner_label (XanList *blocks, t_try_block *inner_block);
static inline t_label * get_label_by_bytes_offset (XanList *labels, JITUINT32 bytes_offset);
static inline JITINT16 cil_insert_label (CILStack cilStack, XanList *labels, t_label *label, t_stack *stack, JITINT32 position);
static inline JITINT16 translate_cil_dup (Method method, JITUINT32 bytes_read, t_stack *stack);
static inline JITINT16 translate_cil_endfinally (Method method, JITUINT32 bytes_read, XanStack *finallyLabels);
static inline JITINT32 internal_check_ir (ir_method_t *method);
static inline void initialize_local_variables (CLIManager_t *cliManager, Method method, t_stack *stack);
static inline ir_item_t * make_new_IR_param ();
static inline void make_ir_infos (ir_item_t * ir, ir_item_t * stack);

/**
 * @brief Store a value into a new variable
 *
 * ---------
 * | varID |           \
 * ---------    --------\    ---------
 * | value |    --------/    | varID |
 * ---------           /     ---------
 *    .                          .
 *    .                          .
 *    .                          .
 */
static inline JITINT16 translate_cil_move (Method method, JITUINT32 bytes_read, t_stack *stack);
static inline JITINT16 translate_cil_switch (CLIManager_t *cliManager, CILStack cilStack, Method method, JITUINT32 *bytes_read, t_stack *stack, JITUINT32 *current_label_ID, XanList *labels, JITINT8 *body_stream);
static inline JITINT16 translate_cil_ret (CLIManager_t *cliManager, Method method, ir_method_t *irMethod, CILStack cilStack, JITUINT32 *current_label_ID, JITUINT32 bytes_read, XanList *labels, t_stack *stack);
static inline JITINT16 translate_cil_arglist (CLIManager_t *cliManager, CILStack cilStack, Method method, JITUINT32 bytes_read, t_stack *stack, JITUINT32 *current_label_ID, XanList *labels);
static inline JITINT16 translate_cil_branch (Method method, CILStack cilStack, JITUINT32 *current_label_ID, JITUINT32 bytes_read, JITINT32 jump_offset, XanList *labels, JITUINT32 inst_size, t_stack *stack);
static inline JITINT8 translate_cil_sizeof (CLIManager_t *cliManager, Method method, JITUINT32 bytes_read, t_stack *stack, GenericContainer *container, t_binary_information *binary, JITUINT32 token);
static inline JITINT16 translate_cil_brfalse (Method method, CILStack cilStack, JITUINT32 *current_label_ID, JITUINT32 bytes_read, JITINT32 jump_offset, XanList *labels, JITUINT32 inst_size, t_stack *stack);
static inline JITINT16 translate_cil_brtrue (Method method, CILStack cilStack, JITUINT32 *current_label_ID, JITUINT32 bytes_read, JITINT32 jump_offset, XanList *labels, JITUINT32 inst_size, t_stack *stack);
static inline JITINT16 translate_cil_bge (CLIManager_t *cliManager, CILStack cilStack, Method method, JITUINT32 *current_label_ID, JITUINT32 bytes_read, JITINT32 jump_offset, XanList *labels, JITUINT32 inst_size, t_stack *stack, JITBOOLEAN unordered);
static inline JITINT16 translate_cil_blt (CLIManager_t *cliManager, CILStack cilStack, Method method, JITUINT32 *current_label_ID, JITUINT32 bytes_read, JITINT32 jump_offset, XanList *labels, JITUINT32 inst_size, t_stack *stack, JITBOOLEAN unordered);
static inline JITINT16 translate_cil_beq (CLIManager_t *cliManager, CILStack cilStack,  Method method, JITUINT32 *current_label_ID, JITUINT32 bytes_read, JITINT32 jump_offset, XanList *labels, JITUINT32 inst_size, t_stack *stack);
static inline JITINT16 translate_cil_bneq (CLIManager_t *cliManager, CILStack cilStack, Method method, JITUINT32 *current_label_ID, JITUINT32 bytes_read, JITINT32 jump_offset, XanList *labels, JITUINT32 inst_size, t_stack *stack);
static inline JITINT16 translate_cil_conv (CLIManager_t *cliManager, Method method, JITUINT32 bytes_read, t_stack *stack, JITUINT32 type, JITBOOLEAN ovf, JITBOOLEAN force_unsigned_check, JITBOOLEAN force_stackable_type);
static inline JITINT16 translate_cil_ldlen (CLIManager_t *cliManager, t_running_garbage_collector *gc, Method method, CILStack cilStack, JITUINT32 bytes_read, t_stack *stack, JITUINT32 *current_label_ID, XanList *labels);
static inline JITINT16 translate_cil_load_fconstant (CLIManager_t *cliManager, Method method, JITUINT32 bytes_read, IR_ITEM_FVALUE value, JITUINT32 type, t_stack *stack);
static inline JITINT16 translate_cil_load_constant (CLIManager_t *cliManager, Method method, JITUINT32 bytes_read, IR_ITEM_VALUE value, JITUINT32 type, t_stack *stack);
static inline JITINT16 translate_cil_checknull (Method method, JITUINT32 bytes_read, t_stack *stack);
static inline JITINT16 translate_cil_ldcalloc (CILStack cilStack, Method method, JITUINT32 bytes_read, t_stack *stack, JITUINT32 *current_label_ID, XanList *labels);
static inline JITINT16 translate_cil_call (t_system *system, CILStack cilStack, GenericContainer *container, t_binary_information *binary, Method method, JITUINT32 token, JITUINT32 bytes_read, t_stack *stack, JITBOOLEAN isTail, JITUINT32 *current_label_ID, XanList *labels, JITBOOLEAN created);
static inline JITINT16 translate_cil_calli (CLIManager_t *cliManager, CILStack cilStack, GenericContainer *container, t_binary_information *binary, Method method, JITUINT32 token, JITUINT32 bytes_read, t_stack *stack, JITBOOLEAN isTail, JITUINT32 *current_label_ID, XanList *labels);
static inline JITINT16 translate_cil_call_by_methodID (CLIManager_t *cliManager, Method method, MethodDescriptor *methodID, JITUINT32 bytes_read, t_stack *stack, JITBOOLEAN created, FunctionPointerDescriptor *signature_of_the_jump_method, CILStack cilStack, JITUINT32 *current_label_ID, XanList *labels);
static inline JITINT16 translate_cil_castclass (t_system *system, Method method, CILStack cilStack, JITUINT32 token, GenericContainer *container, t_binary_information *binary, XanList *labels, JITUINT32 *current_label_ID, JITUINT32 bytes_read, t_stack *stack);
static inline JITINT16 translate_cil_ldstr (CLIManager_t *cliManager, Method method, CILStack cilStack, JITUINT32 token, JITUINT32 bytes_read, t_stack *stack, t_binary_information *binary, JITUINT32 *current_label_ID, XanList *labels);
static inline JITINT16 translate_cil_leave (Method method, CILStack cilStack, JITUINT32 *current_label_ID, JITUINT32 bytes_read, JITINT32 jump_offset, XanList *labels, JITUINT32 inst_size, t_stack *stack);
static inline JITINT16 translate_cil_newobj (t_system *system, CILStack cilStack, GenericContainer *container, t_binary_information *current_binary, Method method, JITUINT32 bytes_read, JITUINT32 token, t_stack *stack, JITUINT32 *current_label_ID, XanList *labels);
static inline JITINT16 translate_cil_initobj (t_system *system, CILStack cilStack, GenericContainer *container, t_binary_information *current_binary, Method method, JITUINT32 bytes_read, JITUINT32 token, t_stack *stack, JITUINT32 *current_label_ID, XanList *labels);
static inline JITINT16 translate_cil_box (t_system *system, CILStack cilStack, GenericContainer *container, t_binary_information *current_binary, Method method, JITUINT32 bytes_read, JITUINT32 token, t_stack *stack, JITUINT32 *current_label_ID, XanList *labels);
static inline JITINT16 translate_cil_unbox (t_system *system, CILStack cilStack, GenericContainer *container, t_binary_information *current_binary, Method method, JITUINT32 bytes_read, JITUINT32 token, t_stack *stack, JITUINT32 *current_label_ID, XanList *labels);
static inline JITINT16 translate_cil_unbox_any (t_system *system, CILStack cilStack, GenericContainer *container, t_binary_information *current_binary, Method method, JITUINT32 bytes_read, JITUINT32 token, t_stack *stack, JITUINT32 *current_label_ID, XanList *labels);
static inline JITINT16 translate_cil_bgt (CLIManager_t *cliManager, CILStack cilStack, Method method, JITUINT32 *current_label_ID, JITUINT32 bytes_read, JITINT32 jump_offset, XanList *labels, JITUINT32 inst_size, t_stack *stack, JITBOOLEAN unordered);
static inline JITINT16 translate_cil_throw (CLIManager_t *cliManager, Method method, CILStack cilStack, JITUINT32 *current_label_ID, XanList *labels, JITUINT32 bytes_read, t_stack *stack);
static inline JITINT16 translate_cil_rethrow (CLIManager_t *cliManager, Method method, CILStack cilStack, JITUINT32 *current_label_ID, XanList *labels, JITUINT32 bytes_read, t_stack *stack, IR_ITEM_VALUE *exceptionVarID);
static inline JITINT16 translate_cil_refanyval (t_system *system, Method method, CILStack cilStack, JITUINT32 *current_label_ID, XanList *labels, JITUINT32 bytes_read, t_stack *stack, GenericContainer *container, t_binary_information *binary, JITUINT32 token);
static inline JITINT16 translate_cil_newarr (t_system *system, CILStack cilStack, GenericContainer *container, t_binary_information *current_binary, Method method, JITUINT32 bytes_read, JITUINT32 token, t_stack *stack, JITUINT32 *current_label_ID, XanList *labels);
static inline JITINT16 translate_cil_ldelema (t_system *system, CILStack cilStack, GenericContainer *container, t_binary_information *current_binary, Method method, JITUINT32 bytes_read, JITUINT32 token, t_stack *stack, JITUINT32 *current_label_ID, XanList *labels);
static inline JITINT16 translate_cil_vcall (t_system *system, CILStack cilStack, GenericContainer *container, t_binary_information *binary, Method method, JITUINT32 token, JITUINT32 bytes_read, t_stack *stack, JITBOOLEAN isTail, TypeDescriptor *constrained, JITUINT32 *current_label_ID, XanList *labels);
static inline JITINT16 translate_cil_ldind (t_system *system, Method method, JITUINT32 bytes_read, t_stack *stack, JITUINT32 type, JITUINT32 target_type);
static inline JITINT16 translate_cil_stind (CLIManager_t *cliManager, Method method, CILStack cilStack, JITUINT32 bytes_read, t_stack *stack, JITUINT32 *current_label_ID, XanList *labels, JITUINT32 type);
static inline JITINT16 translate_cil_stsfld (Method method, CILStack cilStack, JITUINT32 token, JITUINT32 bytes_read, t_stack *stack, GenericContainer *container, t_binary_information *binary, t_system *system, JITUINT32 *current_label_ID, XanList *labels);
static inline JITINT16 translate_cil_ldftn (t_system *system, Method method, CILStack cilStack, JITUINT32 bytes_read, t_stack *stack, GenericContainer *container, t_binary_information *binary, JITUINT32 token);
static inline JITINT16 translate_cil_ldvirtftn (t_system *system, Method method, CILStack cilStack, JITUINT32 bytes_read, t_stack *stack, GenericContainer *container, t_binary_information *binary, JITUINT32 token, JITUINT32 *current_label_ID, XanList *labels);
static inline JITINT16 translate_cil_stobj (CLIManager_t *cliManager, Method method, CILStack cilStack, JITUINT32 token, JITUINT32 bytes_read, t_stack *stack, GenericContainer *container, t_binary_information *binary, t_system *system, JITUINT32 *current_label_ID, XanList *labels, JITUINT8 alignment);
static inline JITINT16 translate_cil_ldobj (Method method, CILStack cilStack, JITUINT32 token, JITUINT32 bytes_read, t_stack *stack, GenericContainer *container, t_binary_information *binary, t_system *system, JITUINT32 *current_label_ID, XanList *labels);
static inline JITINT16 translate_cil_ble (CLIManager_t *cliManager, CILStack cilStack, Method method, JITUINT32 *current_label_ID, JITUINT32 bytes_read, JITINT32 jump_offset, XanList *labels, JITUINT32 inst_size, t_stack *stack, JITBOOLEAN unordered);
static inline JITINT16 translate_instruction_with_binary_logical_operator (t_system *system, Method method, JITUINT32 bytes_read, t_stack *stack, JITUINT32 logical_operator);
static inline JITINT16 translate_cil_ldfld (t_system *system, Method method, CILStack cilStack, JITUINT32 *current_label_ID, XanList *labels, JITUINT32 bytes_read, GenericContainer *container, t_binary_information *binary, JITUINT32 token, t_stack *stack);
static inline JITINT16 translate_cil_ldflda (t_system *system, Method method, CILStack cilStack, JITUINT32 *current_label_ID, XanList *labels, JITUINT32 bytes_read, GenericContainer *container, t_binary_information *binary, JITUINT32 token, t_stack *stack);
static inline JITINT16 translate_cil_ldsflda (t_system *system, Method method, CILStack cilStack, JITUINT32 *current_label_ID, XanList *labels, JITUINT32 bytes_read, GenericContainer *container, t_binary_information *binary, JITUINT32 token, t_stack *stack);
static inline JITINT16 translate_cil_stfld (Method method, CILStack cilStack, JITUINT32 bytes_read, GenericContainer *container, t_binary_information *binary, JITUINT32 token, t_stack *stack, JITUINT32 *current_label_ID, XanList *labels, t_system *system);
static inline JITINT16 translate_cil_shl (t_system *system, Method method, JITUINT32 bytes_read, t_stack *stack);
static inline JITINT16 translate_cil_shr (t_system *system, Method method, JITUINT32 bytes_read, t_stack *stack, JITBOOLEAN unsigned_check);
static inline JITINT16 translate_cil_not (Method method, JITUINT32 bytes_read, t_stack *stack);
static inline JITINT16 translate_cil_neg (Method method, JITUINT32 bytes_read, t_stack *stack);
static inline JITINT16 translate_cil_ldsfld (Method method, CILStack cilStack, JITUINT32 token, JITUINT32 bytes_read, t_stack *stack, GenericContainer *container, t_binary_information *binary, t_system *system, JITUINT32 *current_label_ID, XanList *labels);
static inline JITINT16 translate_cil_ldarga (t_system *system, Method method, JITUINT32 bytes_read, JITUINT16 num, t_stack *stack);
static inline JITINT16 translate_cil_ldarg (CLIManager_t *cliManager, Method method, JITUINT32 bytes_read, JITUINT32 arg_num, t_stack *stack);
static inline JITINT16 translate_cil_starg (CLIManager_t *cliManager, Method method, JITUINT32 bytes_read, JITUINT32 arg_num, t_stack *stack);
static inline JITINT16 translate_cil_ldloc (t_system *system, Method method, JITUINT32 bytes_read, JITUINT32 loc_num, t_stack *stack);
static inline JITINT16 translate_cil_ldloca (t_system *system, Method method, JITUINT32 bytes_read, JITUINT16 num, t_stack *stack, JITBOOLEAN localsInit, JITUINT32 *current_label_ID, XanList *labels);
static inline JITINT16 translate_cil_stloc (CLIManager_t *cliManager, Method method, JITUINT32 bytes_read, JITUINT32 loc_num, t_stack *stack);
static inline JITINT16 translate_cil_ldnull (CLIManager_t *cliManager, Method method, JITUINT32 bytes_read, t_stack *stack);
static inline JITINT16 translate_cil_ldtoken (t_system *system, Method method, GenericContainer *container, t_binary_information *binary, t_stack *stack, JITUINT32 bytes_read, JITUINT32 token, JITUINT32 *current_label_ID, XanList *labels);
static inline JITINT16 translate_cil_eq (CLIManager_t *cliManager, Method method, JITUINT32 bytes_read, t_stack *stack);
static inline JITINT16 translate_cil_gt (CLIManager_t *cliManager, Method method, JITUINT32 bytes_read, t_stack *stack);
static inline JITINT16 translate_cil_lt (CLIManager_t *cliManager, Method method, JITUINT32 bytes_read, t_stack *stack);
static inline JITINT16 translate_cil_Ckfinite (CLIManager_t *cliManager, Method method, CILStack cilStack, JITUINT32 *current_label_ID, JITUINT32 bytes_read, JITINT32 jump_offset, XanList *labels, t_system *system, JITUINT32 inst_size, t_stack *stack);
static inline JITINT16 make_irnewobj (CLIManager_t *cliManager, Method method, CILStack cilStack, JITUINT32 bytes_read, TypeDescriptor *classID, JITUINT32 overSize, t_stack *stack, JITUINT32 *current_label_ID, XanList *labels, ir_instruction_t *before);
static inline JITINT16 make_irmemcpy (Method method, CILStack cilStack, JITUINT32 bytes_read, t_stack *stack, JITUINT32 *current_label_ID, XanList *labels, JITINT32 memsize, JITUINT8 alignment, ir_instruction_t *before);
static inline JITINT16 make_irnewValueType (CLIManager_t *cliManager, Method method, JITUINT32 bytes_read, TypeDescriptor *value_type, t_stack *stack, ir_instruction_t *before);
static inline JITINT16 make_stelem (t_system *system, Method method, CILStack cilStack, JITUINT32 *current_label_ID, XanList *labels, JITUINT32 bytes_read, TypeDescriptor *classLocated, t_stack *stack);
static inline JITINT16 translate_cil_stelem (t_system *system, CILStack cilStack, GenericContainer *container, t_binary_information *current_binary, Method method, JITUINT32 bytes_read, JITUINT32 token, t_stack *stack, JITUINT32 *current_label_ID, XanList *labels);
static inline JITINT16 make_ldelem (t_system *system, Method method, CILStack cilStack, JITUINT32 *current_label_ID, XanList *labels, JITUINT32 bytes_read, TypeDescriptor *classLocated, t_stack *stack);
static inline JITINT16 translate_cil_ldelem (t_system *system, CILStack cilStack, GenericContainer *container, t_binary_information *current_binary, Method method, JITUINT32 bytes_read, JITUINT32 token, t_stack *stack, JITUINT32 *current_label_ID, XanList *labels);
static inline JITINT16 translate_cil_compare (CLIManager_t *cliManager, Method method, CILStack cilStack, JITUINT32 bytes_read, JITUINT32 type, t_stack *stack, JITUINT32 *current_label_ID, XanList *labels, JITBOOLEAN unordered_check, JITBOOLEAN fix_all_labels, JITBOOLEAN force_stackable_type);
static inline JITINT16 translate_cil_cpblk (t_system *system, CILStack cilStack, Method method, JITUINT32 bytes_read, t_stack *stack, JITUINT32 *current_label_ID, XanList *labels, JITUINT8 alignment);
static inline JITINT16 translate_cil_initblk (t_system *system, CILStack cilStack, Method method, JITUINT32 bytes_read, t_stack *stack, JITUINT32 *current_label_ID, XanList *labels, JITUINT8 alignment);
/**
 *
 * Insert a IRLABEL instruction (if it is needed) in the instructions list at position (bytes_read + bytes_offset) and set the parameter param to point to that label (where the IR instruction where the param is fill has size equal to inst_size); insert a label item into the labels list and increment by one the current_label_ID (if it is needed).
 *
 * @param method		The current method
 * @param labels Labels		list of the current method
 * @param bytes_offset		Number of bytes to jump from the current position
 * @param inst_size		Size in bytes of the current branch instruction
 * @param bytes_read		Bytes read from the first instruction of the current method
 * @param current_label_ID	Current label ID free
 * @param param			Parameter of the current branch instruction
 * @param stack			Current stack of the method
 */
static inline JITINT16 add_label_inst (Method method, CILStack cilStack, XanList *labels, JITINT32 bytes_offset, JITUINT32 inst_size, JITUINT32 bytes_read, JITUINT32 *current_label_ID, ir_item_t *param, t_stack *stack);
static inline ir_instruction_t * insert_label_before (Method method, ir_instruction_t *inst, JITUINT32 current_label_ID, JITUINT32 bytes_offset, JITBOOLEAN *label_already_assigned);

static inline JITINT16 internal_translate_ldlen (CLIManager_t *cliManager, t_running_garbage_collector *gc, Method method, JITUINT32 bytes_read, t_stack *stack);
static inline JITINT16 perform_leave (Method method, CILStack cilStack, JITUINT32 *current_label_ID, JITUINT32 bytes_read, JITINT32 jump_offset, XanList *labels, JITUINT32 inst_size, t_stack *stack, t_try_block *source_block, t_try_block *target_block, t_try_handler *source_handler, t_try_handler *target_handler);
static inline JITINT16 make_catcher_instructions (t_system *system, Method method, CILStack cilStack, JITUINT32 *current_label_ID, XanList *labels, JITUINT32 bytes_read, t_stack *stack, IR_ITEM_VALUE *exceptionVarID);
static inline JITINT16 make_catcher_initialization_instructions (Method method, CILStack cilStack, JITUINT32 *current_label_ID, XanList *labels, JITUINT32 bytes_read, t_stack *stack, IR_ITEM_VALUE *exceptionVarID);
static inline void make_blocks_instructions (t_system *system, Method method, XanList *blocks, JITUINT32 *current_label_ID, JITUINT32 exit_label, JITUINT32 bytes_read, t_stack *stack);

/**
 *
 * @param before if it is not NULL, the IR instructions will be inserted before the specified instruction.
 *               If it is NULL, they will be put at the end of the current method.
 */
static inline JITINT16 translate_ncall (Method method, JITINT8 *function_name, JITUINT32 result, XanList *params, JITUINT32 bytes_read, JITUINT32 result_variable_ID, ir_instruction_t *before);

/* functions used to test exception conditions */
static inline JITUINT32 translate_Test_Null_Reference (CLIManager_t *cliManager, Method method, CILStack cilStack, JITUINT32 bytes_read, t_stack *stack);
static inline JITUINT32 translate_Test_Array_Type_Mismatch (t_system *system, Method method, CILStack cilStack, JITUINT32 *current_label_ID, XanList *labels, JITUINT32 bytes_read, t_stack *stack, TypeDescriptor *classLocated);
static inline JITUINT32 translate_Test_Array_Index_Out_Of_Range (t_system *system, Method method, CILStack cilStack, JITUINT32 *current_label_ID, XanList *labels, JITUINT32 bytes_read, t_stack *stack);
static inline JITUINT32 translate_Test_OutOfMemory (CLIManager_t *cliManager, Method method, CILStack cilStack, JITUINT32 *current_label_ID, XanList *labels, JITUINT32 bytes_read, t_stack *stack, ir_instruction_t *before);
static inline JITUINT32 translate_Test_Overflow (t_system *system, Method method, CILStack cilStack, JITUINT32 *current_label_ID, XanList *labels, JITUINT32 bytes_read, t_stack *stack);
static inline JITUINT32 translate_Test_Type_Load_with_Type_Token (t_system *system, Method method, CILStack cilStack, JITUINT32 *current_label_ID, XanList *labels, JITUINT32 bytes_read, t_stack *stack, t_binary_information *binary, JITUINT32 token);
static inline JITUINT32 translate_Test_Missing_Method (t_system *system, Method method, CILStack cilStack, JITUINT32 *current_label_ID, XanList *labels, JITUINT32 bytes_read, t_stack *stack, t_binary_information *binary, JITUINT32 token);
static inline JITUINT32 translate_Test_Method_Access (t_system *system, Method method, CILStack cilStack, JITUINT32 *current_label_ID, XanList *labels, JITUINT32 bytes_read, t_stack *stack, MethodDescriptor *callee);
static inline JITUINT32 translate_Test_Invalid_Operation (t_system *system, Method method, CILStack cilStack, JITUINT32 *current_label_ID, XanList *labels, JITUINT32 bytes_read, t_stack *stack, MethodDescriptor *ctor);
static inline JITUINT32 translate_Test_Missing_Field (t_system *system, Method method, CILStack cilStack, JITUINT32 *current_label_ID, XanList *labels, JITUINT32 bytes_read, t_stack *stack, t_binary_information *binary, JITUINT32 token);
static inline JITUINT32 translate_Test_Null_Reference_With_Static_Field_Check (CLIManager_t *cliManager, Method method, CILStack cilStack, JITUINT32 *current_label_ID, XanList *labels, JITUINT32 bytes_read, t_stack *stack, FieldDescriptor *fieldID);
static inline JITUINT32 translate_Test_Field_Access (t_system *system, Method method, CILStack cilStack, JITUINT32 *current_label_ID, XanList *labels, JITUINT32 bytes_read, t_stack *stack, FieldDescriptor *fieldLocated);
static inline JITUINT32 translate_Test_Cast_Class (t_system *system, Method method, CILStack cilStack, JITUINT32 *current_label_ID, XanList *labels, JITUINT32 bytes_read, t_stack *stack, GenericContainer *container, t_binary_information *binary, JITUINT32 token, JITBOOLEAN can_throw_exception);
static inline JITUINT32 translate_Test_Valid_Value_Address (t_system *system, Method method, CILStack cilStack, JITUINT32 *current_label_ID, XanList *labels, JITUINT32 bytes_read, t_stack *stack, JITUINT32 type);
static inline JITUINT32 _translate_throw_CIL_Exception (CLIManager_t *cliManager, Method method, CILStack cilStack, JITUINT32 *current_label_ID, XanList *labels, JITUINT32 bytes_read, t_stack *stack, TypeDescriptor *type);
static inline JITINT32 internal_isStackCorrect (CLIManager_t *cliManager, Method method, t_stack *stack);
static inline JITINT16 make_isSubType (t_system *system, Method method, JITUINT32 *current_label_ID, JITUINT32 bytes_read, t_stack *stack, TypeDescriptor *type_of_the_token);
static inline t_stack * internal_insertLabels (Method method, XanList *labels, JITBOOLEAN *is_invalid_basic_block, JITUINT32 bytes_read, t_stack *stack, CILStack cilStack, XanStack *finallyLabels, IR_ITEM_VALUE *exceptionVarID);

/**
 * @brief This function translates a CIL method in a method written the IR language
 *
 * @param system The t_system variable describing the running system
 * @param method The CIL method to be translated. It usually comes from a pipeline
 *
 * @return If the given method was a native call, it returns the constant value NATIVE_CALL.
 */
JITINT16 translate_method_from_cil_to_ir (t_system *system, Method method) {
    t_binary_information            *current_binary;        /**< Binary where the method is	*/
    t_stack                         *stack;
    MethodDescriptor                *methodMetadata;
    XanListItem                     *item;
    GenericContainer                *container;
    XanList                                 *labels;
    CILStack cilStack;
    JITBOOLEAN are_more_sections;
    JITUINT32 entry_point_address;
    JITUINT32 body_size;
    JITUINT32 count;
    JITINT16 error;
    JITUINT32 IRParam;
    JITINT8 buffer[DIM_BUF];
    char method_type;
    JITINT8 first_byte;
    JITINT8                         *body_stream;
    JITUINT32 current_label_ID;
    JITBOOLEAN is_invalid_basic_block;
    JITBOOLEAN localsInit;
    JITUINT8 unaligned_prefix_value;
    ir_method_t                     *irMethod;
    CLIManager_t                    *cliManager;
    t_running_garbage_collector     *gc;
    XanStack                        *finallyLabels;
    IR_ITEM_VALUE			exceptionVarID;

    /*
            If the unaligned_prefix_value is 0, no unaligned prefix has been found;
            the next instruction works considering the default alignment to IRNUINT
            If the unaligned prefix is found, it must have one of the value in 1, 2 or 4
            and the next instruction must check for addresses to be aligned to the indicated value
     */

    /* Assertion.
     */
    assert(method != NULL);
    assert(method->getState(method) == CIL_STATE);
    assert(method->getBinary(method) != NULL);
    assert(method->getName(method) != NULL);
    PDEBUG("CILIR: Method to translate in IR language\n");
    PDEBUG("CILIR:		Binary				= %s\n", method->getBinary(method)->name);
    PDEBUG("CILIR:		Name				= %s\n", method->getFullName(method));

    /* Init the variables.
     */
    current_binary = NULL;
    stack = NULL;
    cilStack = NULL;
    body_stream = NULL;
    are_more_sections = 0;
    entry_point_address = 0;
    body_size = 0;
    count = 0;
    error = 0;
    current_label_ID = 0;
    is_invalid_basic_block = 0;
    localsInit = 0;
    unaligned_prefix_value = 0;
    exceptionVarID	= -1;

    /* Allocate the list of labels		*/
    labels = xanList_new(allocFunction, freeFunction, NULL);

    /* Fetch the metadata			*/
    methodMetadata = method->getID(method);
    assert(methodMetadata != NULL);
    container = method->getContainer(method);

    /* Check if the method is available in	*
     * CIL bytecode				*/
    if (!method->isCilImplemented(method)) {
        SNPRINTF(buffer, sizeof(buffer), "'%s' does not to be compiled", methodMetadata->getSignatureInString(methodMetadata));
        print_err((char *) buffer, 0);
        return 0;
    }

    /* Fetch the cli manager		*/
    cliManager = &(system->cliManager);

    /* Fetch the garbage collector		*/
    gc = &((system->garbage_collectors).gc);

    /* Create the stack			*/
    PDEBUG("CILIR: Make the CIL stack\n");
    cilStack = newCILStack();
    assert(cilStack != NULL);
    stack = cilStack->newStack(cilStack);
    assert(stack != NULL);

    /* Fetch the binary                     */
    current_binary = method->getBinary(method);
    assert(current_binary != NULL);

    /* Fetch the IR method			*/
    irMethod = method->getIRMethod(method);
    assert(irMethod != NULL);

    /* Lock the mutex of the binary         */
    PLATFORM_lockMutex(&((current_binary->binary).mutex));

    entry_point_address = convert_virtualaddress_to_realaddress(current_binary->sections, methodMetadata->attributes->rva);

    /* Seek to the begin of the file	*/
    if (unroll_file( &(current_binary->binary) ) != 0) {
        PLATFORM_unlockMutex(&((current_binary->binary).mutex));
        return NO_SEEK_POSSIBLE;
    }

    /* Seek to the entry point address	*/
    PDEBUG("CILIR:	Seek to the entry point\n");
    PDEBUG("CILIR:		Entry point: %u\n", entry_point_address);
    PDEBUG("CILIR:		Current offset: %llu\n", (*(current_binary->binary.reader))->offset);
    if (seek_within_file(&(current_binary->binary), buffer, (*(current_binary->binary.reader))->offset, entry_point_address) != 0 ) {
        PLATFORM_unlockMutex(&((current_binary->binary).mutex));
        return NO_SEEK_POSSIBLE;
    }
    assert((*(current_binary->binary.reader))->offset == entry_point_address);
    PDEBUG("CILIR:		File offset= 0x%llX\n", (*(current_binary->binary.reader))->offset);

    /* Decode the entry point address	*/
    PDEBUG("CILIR:	Read the first byte of the method header\n");
    if (il_read(&first_byte, 1, &(current_binary->binary)) != 0) {
        PLATFORM_unlockMutex(&((current_binary->binary).mutex));
        return -1;
    }
    PDEBUG("CILIR:		File offset= 0x%llX\n", (*(current_binary->binary.reader))->offset);

    /* Decode the method header			*/
    method_type = (first_byte & 0x03);
    if (method_type == 0x03) {
        /* Fat method				*/
        JITUINT32 max_stack;
        JITUINT32 local_var_sig_tok;
#ifdef PRINTDEBUG
        JITUINT32 size;
#endif
        PDEBUG("CILIR:		Fat method\n");

        /* Alignment check			*/
        assert((entry_point_address % 4) == 0);

        /* Reading 12 bytes, the fat header */
        buffer[0] = first_byte;
        if (il_read(buffer + 1, 11, &(current_binary->binary)) != 0) {
            PLATFORM_unlockMutex(&((current_binary->binary).mutex));
            return NO_READ_POSSIBLE;
        }
        PDEBUG("CILIR:			Flags\n");
        if ((buffer[0] & 0x8) == 0x8) {
            PDEBUG("CILIR:				More sections\n");
            are_more_sections = 1;
        } else {
            PDEBUG("CILIR:				There aren't more sections\n");
        }
        if ((buffer[0] & 0x10) == 0x10) {
            PDEBUG("CILIR:				Call default contructor on all local variables\n");
            localsInit = 1;
        }
#ifdef PRINTDEBUG
        size = (JITUINT32) ((buffer[1] >> 0x4) & 0xF);
        PDEBUG("CILIR:				Size header	= %d Bytes\n", size * 4);
#endif
        read_from_buffer(buffer, 2, 2, &max_stack);
        PDEBUG("CILIR:				Max stack       = %d items\n", max_stack);
        read_from_buffer(buffer, 4, 4, &body_size);
        PDEBUG("CILIR:				Body size       = %d Bytes\n", body_size);
        read_from_buffer(buffer, 8, 4, &local_var_sig_tok);
        PDEBUG("CILIR:				LocalVarSigTok  = 0x%X\n", local_var_sig_tok);

        /* Decode the local signature		*/
        if (local_var_sig_tok != 0x0) {
            PDEBUG("CILIR:			Decode the signature of the local variables\n");
            decode_locals_signature(cliManager, local_var_sig_tok, container, current_binary, stack, method);
        }

        /* Decode the method data section	*/
        if (are_more_sections == 1) {
            /* Note that the function `decode_exceptions` doesn't modify the (method->MaxVariables) variable
             * and doesn't modify the `stack` element. */
            PDEBUG("CILIR:			Decode the exceptions header\n");
            decode_exceptions(cliManager, cilStack, container, current_binary, body_size, labels, &current_label_ID, method, stack);
        }

    } else if (method_type == 0x02) {
        /* Tiny method				*/
        PDEBUG("CILIR:			Tiny method\n");

        body_size = ((first_byte >> 0x2) & 0x3F);
        PDEBUG("CILIR:			Body size	= %d\n", body_size);
    } else {
        snprintf((char *) buffer, sizeof(buffer), "CILIR: ERROR = Method type %d is not known. ", method_type);
        print_err((char *) buffer, 0);
        PLATFORM_unlockMutex(&((current_binary->binary).mutex));
        abort();
    }
    assert(body_size > 0);

    XanList *params = methodMetadata->getParams(methodMetadata);

    /* Set the type of each parameters	*/
    for (count = 0, IRParam = NOPARAM; count < method->getParametersNumber(method); count++) {
        JITUINT32 currentParam;
        stack->adjustSize(stack);
        IRParam = IRMETHOD_getParameterType(irMethod, count);
        assert(IRParam != NOPARAM);
        (stack->stack[count]).value.v = count;
        (stack->stack[count]).type = IROFFSET;
        (stack->stack[count]).internal_type = IRParam;

        /* retrieve the current IL parameter informations       */
        if (    (count >= (methodMetadata->attributes->has_this))               &&
                (count < (params->len + methodMetadata->attributes->has_this))  ) {
            if (methodMetadata->attributes->has_this) {
                currentParam = count - 1;
            } else {
                currentParam = count;
            }
            assert(currentParam >= 0);
            assert(currentParam < xanList_length(params));

            /* Fetch the parameter CIL type				*/
            ParamDescriptor *param = (ParamDescriptor *) xanList_getElementFromPositionNumber(params, currentParam)->data;
            assert(param != NULL);

            /* Set the stack CIL type				*/
            (stack->stack[count]).type_infos = param->type;
            assert((stack->stack[count]).type_infos != NULL);
            assert((stack->stack[count]).type == IROFFSET);

        } else {

            /* Set the stack CIL type				*/
            (stack->stack[count]).type_infos = (cliManager->metadataManager)->getTypeDescriptorFromIRType(cliManager->metadataManager, IRParam);
        }
    }

    /* Translate the body			*/
    PDEBUG("CILIR:	Body\n");
    JITBOOLEAN exit_condition;
    JITUINT32 opcode_counter;
    JITUINT32 bytes_read;                   /* Bytes read of the body	*/
    JITUINT32 old_bytes_read;               /* Bytes read of the body	*/
    JITUINT32 new_token;

    opcode_counter = 0;
    exit_condition = 0;
    bytes_read = 0;
    old_bytes_read = 0;
    new_token = 0;

    stack->top = method->getMaxVariables(method);
#ifdef PRINTDEBUG
    PDEBUG("CILIR:		Parameters number	= %u\n", method->getParametersNumber(method));
    PDEBUG("CILIR:		Locals number		= %u\n", method->getLocalsNumber(method));
    PDEBUG("CILIR:		Var count               = %u\n", method->getMaxVariables(method));
    PDEBUG("CILIR:		Top of the stack	= %u\n", stack->top);
    print_stack(stack, method->getParametersNumber(method), method->getLocalsNumber(method));
#endif

    /* Compute the root set			*/
    for (count = 0; count < (method->getLocalsNumber(method) + method->getParametersNumber(method)); count++) {
        stack->adjustSize(stack);
        assert((stack->stack[count]).type == IROFFSET);
        if (    ((stack->stack[count]).internal_type == IROBJECT)       ||
                ((stack->stack[count]).internal_type == IRTPOINTER)     ||
                ((stack->stack[count]).internal_type == IRNINT) 	||
                ((stack->stack[count]).internal_type == IRMPOINTER)     ) {
            method->addVariableToRootSet(method, count);
        }
    }

    /* Initialize the local variables	*/
    initialize_local_variables(cliManager, method, stack);

    /* Read the body of the method          */
    body_stream = allocMemory(body_size + 1);
    if (il_read(body_stream, body_size, &(current_binary->binary)) != 0) {
        PLATFORM_unlockMutex(&((current_binary->binary).mutex));
        abort();
    }
    PLATFORM_unlockMutex(&((current_binary->binary).mutex));

    /* Allocate the stack of finally labels */
    finallyLabels = xanStack_new(allocFunction, freeFunction, NULL);
    assert(finallyLabels != NULL);

    /* Now, we are ready to start with the	*
     * code translation from CIL to IR.     */
    assert(internal_isStackCorrect(cliManager, method, stack));
    while (!exit_condition) {
        JITUINT8 opcode[2];
        JITUINT32 address;
        ir_instruction_t        *instruction;
        TypeDescriptor          *class_located;

        /* Initialize the variables			*/
        class_located = NULL;
        instruction = NULL;
        address = 0;

        /* Remember the bytes read till now		*/
        old_bytes_read = bytes_read;

        /* Check if I have to insert a label		*/
        stack = internal_insertLabels(method, labels, &is_invalid_basic_block, bytes_read, stack, cilStack, finallyLabels, &exceptionVarID);
        assert(stack != NULL);

        /* Set the second byte of the opcode to zero	*/
        opcode[1] = 0;

        /* Read OPCODE of the next instruction		*/
        opcode[0] = body_stream[bytes_read];
        bytes_read += 1;
        opcode_counter++;

        /* Adjust the size of the stack			*/
        stack->adjustSize(stack);

        JITUINT8 paramUInt8;
        JITINT8 paramInt8;
        JITUINT16 paramUInt16;
        JITINT32 paramInt32;
        JITUINT32 paramUInt32;
        JITINT64 paramInt64;
        JITFLOAT32 paramFloat32;
        JITFLOAT64 paramFloat64;

        assert(internal_isStackCorrect(cliManager, method, stack) == JITTRUE);
        switch ((JITUINT16) opcode[0]) {
            case NOP_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	nop\n", opcode_counter);
                break;
            case BREAK_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	break\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                print_err("CILIR: break instruction is not implemented. ", 0);
                abort();
                break;
            case LDARG_0_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	ldarg.0\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_ldarg(cliManager, method, bytes_read, 0, stack);
                break;
            case LDARG_1_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	ldarg.1\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_ldarg(cliManager, method, bytes_read, 1, stack);
                break;
            case LDARG_2_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	ldarg.2\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_ldarg(cliManager, method, bytes_read, 2, stack);
                break;
            case LDARG_3_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	ldarg.3\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_ldarg(cliManager, method, bytes_read, 3, stack);
                break;
            case LDLOC_0_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	ldloc.0\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_ldloc(system, method, bytes_read, 0, stack);
                break;
            case LDLOC_1_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	ldloc.1\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_ldloc(system, method, bytes_read, 1, stack);
                break;
            case LDLOC_2_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	ldloc.2\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_ldloc(system, method, bytes_read, 2, stack);
                break;
            case LDLOC_3_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	ldloc.3\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_ldloc(system, method, bytes_read, 3, stack);
                break;
            case STLOC_0_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	stloc.0\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_stloc(cliManager, method, bytes_read, 0, stack);
                break;
            case STLOC_1_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	stloc.1\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_stloc(cliManager, method, bytes_read, 1, stack);
                break;
            case STLOC_2_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	stloc.2\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_stloc(cliManager, method, bytes_read, 2, stack);
                break;
            case STLOC_3_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	stloc.3\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_stloc(cliManager, method, bytes_read, 3, stack);
                break;
            case LDARG_S_OPCODE:
                memcpy(&paramUInt8, body_stream + bytes_read, 1);
                bytes_read += 1;
                PDEBUG("CILIR:<CIL Inst %d>	ldarg.s		%d\n", opcode_counter, paramUInt8);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_ldarg(cliManager, method, bytes_read, paramUInt8, stack);
                break;
            case LDARGA_S_OPCODE:
                memcpy(&paramUInt8, body_stream + bytes_read, 1);
                bytes_read += 1;
                PDEBUG("CILIR:<CIL Inst %d>	ldarga.s	0x%X\n", opcode_counter, paramUInt8);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_ldarga(system, method, bytes_read, paramUInt8, stack);
                break;
            case STARG_S_OPCODE:
                memcpy(&paramUInt8, body_stream + bytes_read, 1);
                bytes_read += 1;
                PDEBUG("CILIR:<CIL Inst %d>	starg.s		%d\n", opcode_counter, paramUInt8);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_starg(cliManager, method, bytes_read, paramUInt8, stack);
                break;
            case LDLOC_S_OPCODE:
                memcpy(&paramUInt8, body_stream + bytes_read, 1);
                bytes_read += 1;
                PDEBUG("CILIR:<CIL Inst %d>	ldloc.s		%d\n", opcode_counter, paramUInt8);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_ldloc(system, method, bytes_read, paramUInt8, stack);
                break;
            case LDLOCA_S_OPCODE:
                memcpy(&paramUInt8, body_stream + bytes_read, 1);
                bytes_read += 1;
                PDEBUG("CILIR:<CIL Inst %d>	ldloca.s	0x%X\n", opcode_counter, paramUInt8);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_ldloca(system, method, bytes_read, paramUInt8, stack, localsInit, &current_label_ID, labels);
                break;
            case STLOC_S_OPCODE:
                memcpy(&paramUInt8, body_stream + bytes_read, 1);
                bytes_read += 1;
                PDEBUG("CILIR:<CIL Inst %d>	stloc.s		%d\n", opcode_counter, paramUInt8);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_stloc(cliManager, method, bytes_read, paramUInt8, stack);
                break;
            case LDNULL_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	ldnull\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_ldnull(cliManager, method, bytes_read, stack);
                break;
            case LDC_I4_M1_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	ldc.i4.M1\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_load_constant(cliManager, method, bytes_read, (JITINT32) -1, IRINT32, stack);
                break;
            case LDC_I4_0_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	ldc.i4.0\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_load_constant(cliManager, method, bytes_read, (JITINT32) 0, IRINT32, stack);
                break;
            case LDC_I4_1_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	ldc.i4.1\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_load_constant(cliManager, method, bytes_read, (JITINT32) 1, IRINT32, stack);
                break;
            case LDC_I4_2_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	ldc.i4.2\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_load_constant(cliManager, method, bytes_read, (JITINT32) 2, IRINT32, stack);
                break;
            case LDC_I4_3_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	ldc.i4.3\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_load_constant(cliManager, method, bytes_read, (JITINT32) 3, IRINT32, stack);
                break;
            case LDC_I4_4_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	ldc.i4.4\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_load_constant(cliManager, method, bytes_read, (JITINT32) 4, IRINT32, stack);
                break;
            case LDC_I4_5_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	ldc.i4.5\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_load_constant(cliManager, method, bytes_read, (JITINT32) 5, IRINT32, stack);
                break;
            case LDC_I4_6_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	ldc.i4.6\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_load_constant(cliManager, method, bytes_read, (JITINT32) 6, IRINT32, stack);
                break;
            case LDC_I4_7_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	ldc.i4.7\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_load_constant(cliManager, method, bytes_read, (JITINT32) 7, IRINT32, stack);
                break;
            case LDC_I4_8_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	ldc.i4.8\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_load_constant(cliManager, method, bytes_read, (JITINT32) 8, IRINT32, stack);
                break;
            case LDC_I4_S_OPCODE:
                memcpy(&paramInt8, body_stream + bytes_read, 1);
                bytes_read += 1;
                PDEBUG("CILIR:<CIL Inst %d>	ldc.i4.s	%d\n", opcode_counter, paramInt8);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_load_constant(cliManager, method, bytes_read, paramInt8, IRINT32, stack);
                break;
            case LDC_I4_OPCODE:
                memcpy(&paramInt32, body_stream + bytes_read, 4);
                bytes_read += 4;
                PDEBUG("CILIR:<CIL Inst %d>	ldc.i4		%d\n", opcode_counter, paramInt32);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_load_constant(cliManager, method, bytes_read, paramInt32, IRINT32, stack);
                break;
            case LDC_I8_OPCODE:
                memcpy(&paramInt64, body_stream + bytes_read, 8);
                bytes_read += 8;
                PDEBUG("CILIR:<CIL Inst %d>	ldc.i8		%lld\n", opcode_counter, paramInt64);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_load_constant(cliManager, method, bytes_read, paramInt64, IRINT64, stack);
                break;
            case LDC_R4_OPCODE:
                memcpy(&paramFloat32, body_stream + bytes_read, 4);
                bytes_read += 4;
                PDEBUG("CILIR:<CIL Inst %d>	ldc.r4		%f\n", opcode_counter, paramFloat32);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_load_fconstant(cliManager, method, bytes_read, paramFloat32, IRFLOAT32, stack);
                break;
            case LDC_R8_OPCODE:
                memcpy(&paramFloat64, body_stream + bytes_read, 8);
                bytes_read += 8;
                PDEBUG("CILIR:<CIL Inst %d>	ldc.r8		%.8f\n", opcode_counter, paramFloat64);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_load_fconstant(cliManager, method, bytes_read, paramFloat64, IRFLOAT64, stack);
                break;
            case DUP_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	dup\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_dup(method, bytes_read, stack);
                break;
            case POP_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	pop\n", opcode_counter);
                (stack->top)--;
                break;
            case JMP_OPCODE:      //FIXME todo
                memcpy(&new_token, body_stream + bytes_read, 4);
                bytes_read += 4;
                PDEBUG("CILIR:<CIL Inst %d>	jmp		0x%X\n", opcode_counter, new_token);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                print_err("CILIR: jmp instruction is not implemented. ", 0);
                abort();
                break;
            case CALL_OPCODE:
                memcpy(&new_token, body_stream + bytes_read, 4);
                bytes_read += 4;
                PDEBUG("CILIR:<CIL Inst %d>	call		0x%X\n", opcode_counter, new_token);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_call(system, cilStack, container, current_binary, method, new_token, bytes_read, stack, 0, &current_label_ID, labels, 0);
                break;
            case CALLI_OPCODE:
                memcpy(&new_token, body_stream + bytes_read, 4);
                bytes_read += 4;
                PDEBUG("CILIR:<CIL Inst %d>	calli		0x%X\n", opcode_counter, new_token);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_calli(cliManager, cilStack, container,  current_binary, method, new_token, bytes_read, stack, JITFALSE, &current_label_ID, labels);
                break;
            case RET_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	Ret\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_ret(cliManager, method, irMethod, cilStack, &current_label_ID, bytes_read, labels, stack);
                break;
            case BR_S_OPCODE:
                memcpy(&paramInt8, body_stream + bytes_read, 1);
                bytes_read += 1;
                PDEBUG("CILIR:<CIL Inst %d>	br.s		%d\n", opcode_counter, paramInt8);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_branch(method, cilStack, &current_label_ID, bytes_read, paramInt8, labels, 2, stack);
                break;
            case BRFALSE_S_OPCODE:
                memcpy(&paramInt8, body_stream + bytes_read, 1);
                bytes_read += 1;
                PDEBUG("CILIR:<CIL Inst %d>	brfalse.s	%d\n", opcode_counter, paramInt8);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_brfalse(method, cilStack, &current_label_ID, bytes_read, paramInt8, labels, 2, stack);
                break;
            case BRTRUE_S_OPCODE:
                memcpy(&paramInt8, body_stream + bytes_read, 1);
                bytes_read += 1;
                PDEBUG("CILIR:<CIL Inst %d>	brtrue.s	%d\n", opcode_counter, paramInt8);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_brtrue(method, cilStack, &current_label_ID, bytes_read, paramInt8, labels, 2, stack);
                break;
            case BEQ_S_OPCODE:
                memcpy(&paramInt8, body_stream + bytes_read, 1);
                bytes_read += 1;
                PDEBUG("CILIR:<CIL Inst %d>	beq.s		%d\n", opcode_counter, paramInt8);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_beq(cliManager, cilStack, method, &current_label_ID, bytes_read, paramInt8, labels, 2, stack);
                break;
            case BGE_S_OPCODE:
                memcpy(&paramInt8, body_stream + bytes_read, 1);
                bytes_read += 1;
                PDEBUG("CILIR:<CIL Inst %d>	bge.s		%d\n", opcode_counter, paramInt8);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_bge(cliManager, cilStack, method, &current_label_ID, bytes_read, paramInt8, labels, 2, stack, 0);
                break;
            case BGT_S_OPCODE:
                memcpy(&paramInt8, body_stream + bytes_read, 1);
                bytes_read += 1;
                PDEBUG("CILIR:<CIL Inst %d>	bgt.s		%d\n", opcode_counter, paramInt8);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_bgt(cliManager, cilStack, method, &current_label_ID, bytes_read, paramInt8, labels, 2, stack, 0);
                break;
            case BLE_S_OPCODE:
                memcpy(&paramInt8, body_stream + bytes_read, 1);
                bytes_read += 1;
                PDEBUG("CILIR:<CIL Inst %d>	ble.s		%d\n", opcode_counter, paramInt8);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_ble(cliManager, cilStack, method, &current_label_ID, bytes_read, paramInt8, labels, 2, stack, 0);
                break;
            case BLT_S_OPCODE:
                memcpy(&paramInt8, body_stream + bytes_read, 1);
                bytes_read += 1;
                PDEBUG("CILIR:<CIL Inst %d>	blt.s		%d\n", opcode_counter, paramInt8);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_blt(cliManager, cilStack, method, &current_label_ID, bytes_read, paramInt8, labels, 2, stack, 0);
                break;
            case BNE_UN_S_OPCODE:
                memcpy(buffer, body_stream + bytes_read, 1);
                bytes_read += 1;
                address = buffer[0];
                PDEBUG("CILIR:<CIL Inst %d>	bne.un.s	%d\n", opcode_counter, address);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_bneq(cliManager, cilStack, method, &current_label_ID, bytes_read, address, labels, 2, stack);
                break;
            case BGE_UN_S_OPCODE:
                memcpy(buffer, body_stream + bytes_read, 1);
                bytes_read += 1;
                PDEBUG("CILIR:<CIL Inst %d>	bge.un.s	%d\n", opcode_counter, buffer[0]);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_bge(cliManager, cilStack, method, &current_label_ID, bytes_read, (JITINT8) buffer[0], labels, 2, stack, 1);
                break;
            case BGT_UN_S_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	bgt.un.s\n", opcode_counter);
                memcpy(buffer, body_stream + bytes_read, 1);
                bytes_read += 1;
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_bgt(cliManager, cilStack, method, &current_label_ID, bytes_read, (JITINT8) buffer[0], labels, 2, stack, 1);

                break;
            case BLE_UN_S_OPCODE:
                memcpy(buffer, body_stream + bytes_read, 1);
                bytes_read += 1;
                PDEBUG("CILIR:<CIL Inst %d>	ble.un.s	%d\n", opcode_counter, buffer[0]);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_ble(cliManager, cilStack, method, &current_label_ID, bytes_read, (JITINT8) buffer[0], labels, 2, stack, 1);
                break;
            case BLT_UN_S_OPCODE:
                memcpy(buffer, body_stream + bytes_read, 1);
                bytes_read += 1;
                PDEBUG("CILIR:<CIL Inst %d>	blt.un.s	%d\n", opcode_counter, buffer[0]);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_blt(cliManager, cilStack, method, &current_label_ID, bytes_read, (JITINT8) buffer[0], labels, 2, stack, 1);
                break;
            case BR_OPCODE:
                memcpy(&paramInt32, body_stream + bytes_read, 4);
                bytes_read += 4;
                PDEBUG("CILIR:<CIL Inst %d>	br		%d\n", opcode_counter, paramInt32);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_branch( method, cilStack, &current_label_ID, bytes_read, paramInt32, labels, 5, stack);
                break;
            case BRFALSE_OPCODE:
                memcpy(&paramInt32, body_stream + bytes_read, 4);
                bytes_read += 4;
                PDEBUG("CILIR:<CIL Inst %d>	brfalse		%d\n", opcode_counter, paramInt32);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_brfalse(method, cilStack, &current_label_ID, bytes_read, paramInt32, labels, 5, stack);
                break;
            case BRTRUE_OPCODE:
                memcpy(&paramInt32, body_stream + bytes_read, 4);
                bytes_read += 4;
                PDEBUG("CILIR:<CIL Inst %d>	brinst | brtrue	%d\n", opcode_counter, paramInt32);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_brtrue(method, cilStack, &current_label_ID, bytes_read, paramInt32, labels, 5, stack);
                break;
            case BEQ_OPCODE:
                memcpy(&paramInt32, body_stream + bytes_read, 4);
                bytes_read += 4;
                PDEBUG("CILIR:<CIL Inst %d>	beq		%d\n", opcode_counter, paramInt32);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_beq(cliManager, cilStack, method, &current_label_ID, bytes_read, paramInt32, labels, 5, stack);
                break;
            case BGE_OPCODE:
                memcpy(&paramInt32, body_stream + bytes_read, 4);
                bytes_read += 4;
                PDEBUG("CILIR:<CIL Inst %d>	bge		%d\n", opcode_counter, paramInt32);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_bge(cliManager, cilStack, method, &current_label_ID, bytes_read, paramInt32, labels, 5, stack, 0);
                break;
            case BGT_OPCODE:
                memcpy(&paramInt32, body_stream + bytes_read, 4);
                bytes_read += 4;
                PDEBUG("CILIR:<CIL Inst %d>	bgt		%d\n", opcode_counter, paramInt32);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_bgt(cliManager, cilStack, method, &current_label_ID, bytes_read, paramInt32, labels, 5, stack, 0);
                break;
            case BLE_OPCODE:
                memcpy(&paramInt32, body_stream + bytes_read, 4);
                bytes_read += 4;
                PDEBUG("CILIR:<CIL Inst %d>	ble		%d\n", opcode_counter, paramInt32);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_ble(cliManager, cilStack, method, &current_label_ID, bytes_read, paramInt32, labels, 5, stack, 0);
                break;
            case BLT_OPCODE:
                memcpy(&paramInt32, body_stream + bytes_read, 4);
                bytes_read += 4;
                PDEBUG("CILIR:<CIL Inst %d>	blt		%d\n", opcode_counter, paramInt32);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_blt(cliManager, cilStack, method, &current_label_ID, bytes_read, paramInt32, labels, 5, stack, 0);
                break;
            case BNE_UN_OPCODE:
                memcpy(&address, body_stream + bytes_read, 4);
                bytes_read += 4;
                PDEBUG("CILIR:<CIL Inst %d>	bne.un		%d\n", opcode_counter, address);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_bneq(cliManager, cilStack, method, &current_label_ID, bytes_read, address, labels, 5, stack);
                break;
            case BGE_UN_OPCODE:
                memcpy(&paramUInt32, body_stream + bytes_read, 4);
                bytes_read += 4;
                PDEBUG("CILIR:<CIL Inst %d>	bge.un		%d\n", opcode_counter, paramUInt32);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_bge(cliManager, cilStack, method, &current_label_ID, bytes_read, paramUInt32, labels, 5, stack, 1);
                break;
            case BGT_UN_OPCODE:
                memcpy(&paramUInt32, body_stream + bytes_read, 4);
                bytes_read += 4;
                PDEBUG("CILIR:<CIL Inst %d>	bgt.un		%d\n", opcode_counter, paramUInt32);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_bgt(cliManager, cilStack, method, &current_label_ID, bytes_read, paramUInt32, labels, 5, stack, 1);
                break;
            case BLE_UN_OPCODE:
                memcpy(&paramUInt32, body_stream + bytes_read, 4);
                bytes_read += 4;
                PDEBUG("CILIR:<CIL Inst %d>	ble.un		%d\n", opcode_counter, paramUInt32);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_ble(cliManager, cilStack, method, &current_label_ID, bytes_read, paramUInt32, labels, 5, stack, 1);
                break;
            case BLT_UN_OPCODE:
                memcpy(&paramUInt32, body_stream + bytes_read, 4);
                bytes_read += 4;
                PDEBUG("CILIR:<CIL Inst %d>	blt.un		%d\n", opcode_counter, paramUInt32);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_blt(cliManager, cilStack, method, &current_label_ID, bytes_read, paramUInt32, labels, 5, stack, 1);
                break;
            case SWITCH_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	switch\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_switch(cliManager, cilStack, method, &bytes_read, stack, &current_label_ID, labels, body_stream);
                break;
            case LDIND_I1_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	ldind.i1\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_ldind(system, method, bytes_read, stack, IRINT8, IRINT32);
                break;
            case LDIND_U1_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	ldind.u1\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_ldind(system, method, bytes_read, stack, IRUINT8, IRINT32);
                break;
            case LDIND_I2_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	ldind.i2\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_ldind(system, method, bytes_read, stack, IRINT16, IRINT32);
                break;
            case LDIND_U2_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	ldind.u2\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_ldind(system, method, bytes_read, stack, IRUINT16, IRINT32);
                break;
            case LDIND_I4_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	ldind.i4\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_ldind(system, method, bytes_read, stack, IRINT32, IRINT32);
                break;
            case LDIND_U4_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	ldind.u4\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_ldind(system, method, bytes_read, stack, IRUINT32, IRINT32);
                break;
            case LDIND_I8_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	ldind.i8\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_ldind(system, method, bytes_read, stack, IRINT64, IRINT64);
                break;
            case LDIND_I_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	ldind.i\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_ldind(system, method, bytes_read, stack, IRNINT, IRNINT);
                break;
            case LDIND_R4_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	ldind.r4\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_ldind(system, method, bytes_read, stack, IRFLOAT32, IRFLOAT32);
                break;
            case LDIND_R8_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	ldind.r8\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_ldind(system, method, bytes_read, stack, IRFLOAT64, IRFLOAT64);
                break;
            case LDIND_REF_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	ldind.ref\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_ldind(system, method, bytes_read, stack, IROBJECT, IROBJECT);
                break;
            case STIND_REF_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	stind.ref\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_stind(cliManager, method, cilStack, bytes_read, stack, &current_label_ID, labels, IROBJECT);
                break;
            case STIND_I1_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	stind.i1\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_stind(cliManager, method, cilStack, bytes_read, stack, &current_label_ID, labels, IRINT8);
                break;
            case STIND_I2_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	stind.i2\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_stind(cliManager, method, cilStack, bytes_read, stack, &current_label_ID, labels, IRINT16);
                break;
            case STIND_I4_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	stind.i4\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_stind(cliManager, method, cilStack, bytes_read, stack, &current_label_ID, labels, IRINT32);
                break;
            case STIND_I8_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	stind.i8\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_stind(cliManager, method, cilStack, bytes_read, stack, &current_label_ID, labels, IRINT64);
                break;
            case STIND_R4_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	stind.r4\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_stind(cliManager, method, cilStack, bytes_read, stack, &current_label_ID, labels, IRFLOAT32);
                break;
            case STIND_R8_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	stind.r8\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_stind(cliManager, method, cilStack, bytes_read, stack, &current_label_ID, labels, IRFLOAT64);
                break;
            case ADD_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	add\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = _translate_arithmetic_operation(cliManager, method, bytes_read, stack, IRADD);
                break;
            case SUB_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	sub\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = _translate_arithmetic_operation(cliManager, method, bytes_read, stack, IRSUB);
                break;
            case MUL_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	mul\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = _translate_arithmetic_operation(cliManager, method, bytes_read, stack, IRMUL);
                break;
            case DIV_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	Div\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = _translate_arithmetic_operation(cliManager, method, bytes_read, stack, IRDIV);
                break;
            case DIV_UN_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	div.un\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = _translate_arithmetic_operation_un(cliManager, method, bytes_read, stack, IRDIV);
                break;
            case REM_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	rem\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = _translate_arithmetic_operation(cliManager, method, bytes_read, stack, IRREM);
                break;
            case REM_UN_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	rem.un\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = _translate_arithmetic_operation_un(cliManager, method, bytes_read, stack, IRREM);
                break;
            case AND_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	and\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_instruction_with_binary_logical_operator(system, method, bytes_read, stack, IRAND);
                break;
            case OR_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	or\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_instruction_with_binary_logical_operator(system, method, bytes_read, stack, IROR);
                break;
            case XOR_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	xor\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_instruction_with_binary_logical_operator(system, method, bytes_read, stack, IRXOR);
                break;
            case SHL_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	shl\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_shl(system, method, bytes_read, stack);
                break;
            case SHR_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	shr\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_shr(system, method, bytes_read, stack, 0);
                break;
            case SHR_UN_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	shr.un\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_shr(system, method, bytes_read, stack, 1);
                break;
            case NEG_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	neg\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_neg(method, bytes_read, stack);
                break;
            case NOT_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	not\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_not(method, bytes_read, stack);
                break;
            case CONV_I1_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	conv.i1\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_conv(cliManager, method, bytes_read, stack, IRINT8, 0, 0, 1);
                break;
            case CONV_I2_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	conv.i2\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_conv(cliManager, method, bytes_read, stack, IRINT16, 0, 0, 1);
                break;
            case CONV_I4_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	conv.i4\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_conv(cliManager, method, bytes_read, stack, IRINT32, 0, 0, 1);
                break;
            case CONV_I8_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	conv.i8\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_conv(cliManager, method, bytes_read, stack, IRINT64, 0, 0, 1);
                break;
            case CONV_R4_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	conv.r4\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_conv(cliManager, method, bytes_read, stack, IRFLOAT32, 0, 0, 1);
                break;
            case CONV_R8_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	conv.r8\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_conv(cliManager, method, bytes_read, stack, IRFLOAT64, 0, 0, 1);
                break;
            case CONV_U4_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	conv.u4\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_conv(cliManager, method, bytes_read, stack, IRUINT32, 0, JITTRUE, JITTRUE);
                break;
            case CONV_U8_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	conv.u8\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_conv(cliManager, method, bytes_read, stack, IRUINT64, 0, JITTRUE, JITTRUE);
                break;
            case CALLVIRT_OPCODE:
                memcpy(&new_token, body_stream + bytes_read, 4);
                bytes_read += 4;
                PDEBUG("CILIR:<CIL Inst %d>	callvirt	0x%X\n", opcode_counter, new_token);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_vcall(system, cilStack, container, current_binary, method, new_token, bytes_read, stack, 0, NULL,  &current_label_ID, labels);
                break;
            case CPOBJ_OPCODE:     //FIXME todo
                memcpy(&new_token, body_stream + bytes_read, 4);
                bytes_read += 4;
                PDEBUG("CILIR:<CIL Inst %d>	cpobj		0x%X\n", opcode_counter, new_token);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                print_err("CILIR: cpobj instruction is not implemented. ", 0);
                abort();
                break;
            case LDOBJ_OPCODE:
                memcpy(&new_token, body_stream + bytes_read, 4);
                bytes_read += 4;
                PDEBUG("CILIR:<CIL Inst %d>	ldobj		0x%X\n", opcode_counter, new_token);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_ldobj(method, cilStack, new_token, bytes_read, stack, container, current_binary, system, &current_label_ID, labels);
                break;
            case LDSTR_OPCODE:
                memcpy(&new_token, body_stream + bytes_read, 4);
                bytes_read += 4;
                PDEBUG("CILIR:<CIL Inst %d>	ldstr		0x%X\n", opcode_counter, new_token);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_ldstr(cliManager, method, cilStack, new_token, bytes_read, stack, current_binary, &current_label_ID, labels);
                break;
            case NEWOBJ_OPCODE:
                memcpy(&new_token, body_stream + bytes_read, 4);
                bytes_read += 4;
                PDEBUG("CILIR:<CIL Inst %d>	newobj		0x%X\n", opcode_counter, new_token);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_newobj(system, cilStack, container, current_binary, method, bytes_read, new_token, stack, &current_label_ID, labels);
                break;
            case CASTCLASS_OPCODE:
                memcpy(&new_token, body_stream + bytes_read, 4);
                bytes_read += 4;
                PDEBUG("CILIR:<CIL Inst %d>	castclass	0x%X\n", opcode_counter, new_token);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_castclass(system, method, cilStack, new_token, container, current_binary, labels, &current_label_ID, bytes_read, stack);
                break;
            case ISINST_OPCODE:
                memcpy(&new_token, body_stream + bytes_read, 4);
                bytes_read += 4;
                PDEBUG("CILIR:<CIL Inst %d>	isinst		0x%X\n", opcode_counter, new_token);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_Test_Cast_Class(system, method, cilStack, &current_label_ID, labels, bytes_read, stack, container, current_binary, new_token, 0);
                break;
            case CONV_R_UN_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	conv.r.un\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_conv(cliManager, method, bytes_read, stack, IRFLOAT64, 0, JITTRUE, JITTRUE);
                break;
            case UNBOX_OPCODE:
                memcpy(&new_token, body_stream + bytes_read, 4);
                bytes_read += 4;
                PDEBUG("CILIR:<CIL Inst %d>	unbox		0x%X\n", opcode_counter, new_token);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_unbox(system, cilStack, container, current_binary, method, bytes_read, new_token, stack, &current_label_ID, labels);
                break;
            case THROW_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	throw\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_throw(cliManager, method, cilStack, &current_label_ID, labels, bytes_read, stack);
                break;
            case LDFLD_OPCODE:
                memcpy(&new_token, body_stream + bytes_read, 4);
                bytes_read += 4;
                PDEBUG("CILIR:<CIL Inst %d>	ldfld		0x%X\n", opcode_counter, new_token);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_ldfld(system, method, cilStack, &current_label_ID, labels, bytes_read, container, current_binary, new_token, stack);
                break;
            case LDFLDA_OPCODE:
                memcpy(&new_token, body_stream + bytes_read, 4);
                bytes_read += 4;
                PDEBUG("CILIR:<CIL Inst %d>	ldflda		0x%X\n", opcode_counter, new_token);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_ldflda(system, method, cilStack, &current_label_ID, labels, bytes_read, container, current_binary, new_token, stack);
                break;
            case STFLD_OPCODE:
                memcpy(&new_token, body_stream + bytes_read, 4);
                bytes_read += 4;
                PDEBUG("CILIR:<CIL Inst %d>	stfld		0x%X\n", opcode_counter, new_token);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_stfld(method, cilStack, bytes_read, container, current_binary, new_token, stack, &current_label_ID, labels, system);
                break;
            case LDSFLD_OPCODE:
                memcpy(&new_token, body_stream + bytes_read, 4);
                bytes_read += 4;
                PDEBUG("CILIR:<CIL Inst %d>	ldsfld		0x%X\n", opcode_counter, new_token);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_ldsfld(method, cilStack, new_token, bytes_read, stack, container, current_binary, system, &current_label_ID, labels);
                break;
            case LDSFLDA_OPCODE:
                memcpy(&new_token, body_stream + bytes_read, 4);
                bytes_read += 4;
                PDEBUG("CILIR:<CIL Inst %d>	ldsflda		0x%X\n", opcode_counter, new_token);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_ldsflda(system, method, cilStack, &current_label_ID, labels, bytes_read, container, current_binary, new_token, stack);
                break;
            case STSFLD_OPCODE:
                memcpy(&new_token, body_stream + bytes_read, 4);
                bytes_read += 4;
                PDEBUG("CILIR:<CIL Inst %d>	stsfld		0x%X\n", opcode_counter, new_token);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_stsfld(method, cilStack, new_token, bytes_read, stack, container, current_binary, system, &current_label_ID, labels);
                break;
            case STOBJ_OPCODE:
                memcpy(&new_token, body_stream + bytes_read, 4);
                bytes_read += 4;
                PDEBUG("CILIR:<CIL Inst %d>	stobj		0x%X\n", opcode_counter, new_token);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_stobj(cliManager, method, cilStack, new_token, bytes_read, stack, container, current_binary, system, &current_label_ID, labels, unaligned_prefix_value);
                /* Reset the unaligned prefix value */
                if (unaligned_prefix_value != 0) {
                    unaligned_prefix_value = 0;
                }
                break;
            case CONV_OVF_I1_UN_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	conv.ovf.i1.un\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_conv(cliManager, method, bytes_read, stack, IRINT8, 1, 1, 1);
                break;
            case CONV_OVF_I2_UN_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	conv.ovf.i2.un\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_conv(cliManager, method, bytes_read, stack, IRINT16, 1, 1, 1);
                break;
            case CONV_OVF_I4_UN_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	conv.ovf.i4.un\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_conv(cliManager, method, bytes_read, stack, IRINT32, 1, 1, 1);
                break;
            case CONV_OVF_I8_UN_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	conv.ovf.i8.un\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_conv(cliManager, method, bytes_read, stack, IRINT64, 1, 1, 1);
                break;
            case CONV_OVF_U1_UN_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	conv.ovf.u1.un\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_conv(cliManager, method, bytes_read, stack, IRUINT8, 1, 1, 1);
                break;
            case CONV_OVF_U2_UN_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	conv.ovf.u2.un\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_conv(cliManager, method, bytes_read, stack, IRUINT16, 1, 1, 1);
                break;
            case CONV_OVF_U4_UN_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	conv.ovf.u4.un\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_conv(cliManager, method, bytes_read, stack, IRUINT32, 1, 1, 1);
                break;
            case CONV_OVF_U8_UN_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	conv.ovf.u8.un\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_conv(cliManager, method, bytes_read, stack, IRUINT64, 1, 1, 1);
                break;
            case CONV_OVF_I_UN_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	conv.ovf.i.un\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_conv(cliManager, method, bytes_read, stack, IRNINT, 1, 1, 1);
                break;
            case CONV_OVF_U_UN_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	conv.ovf.u.un\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_conv(cliManager, method, bytes_read, stack, IRNUINT, 1, 1, 1);
                break;
            case BOX_OPCODE:
                memcpy(&new_token, body_stream + bytes_read, 4);
                bytes_read += 4;
                PDEBUG("CILIR:<CIL Inst %d>	box		0x%X\n", opcode_counter, new_token);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_box(system, cilStack, container, current_binary, method, bytes_read, new_token, stack, &current_label_ID, labels);
                break;
            case NEWARR_OPCODE:
                memcpy(&new_token, body_stream + bytes_read, 4);
                bytes_read += 4;
                PDEBUG("CILIR:<CIL Inst %d>	newarr		0x%X\n", opcode_counter, new_token);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_newarr(system, cilStack, container, current_binary, method, bytes_read, new_token, stack, &current_label_ID, labels);
                break;
            case LDLEN_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	ldlen\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_ldlen(cliManager, gc, method, cilStack, bytes_read, stack, &current_label_ID, labels);
                break;
            case LDELEMA_OPCODE:
                memcpy(&new_token, body_stream + bytes_read, 4);
                bytes_read += 4;
                PDEBUG("CILIR:<CIL Inst %d>	ldelema		0x%X\n", opcode_counter, new_token);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_ldelema(system, cilStack, container, current_binary, method, bytes_read, new_token, stack, &current_label_ID, labels);
                break;
            case LDELEM_I1_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	ldelem.i1\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                class_located = (system->cliManager).metadataManager->getTypeDescriptorFromElementType(system->cliManager.metadataManager, ELEMENT_TYPE_I1);
                assert(class_located != NULL);
                is_invalid_basic_block = make_ldelem(system, method, cilStack, &current_label_ID, labels, bytes_read, class_located, stack);
                break;
            case LDELEM_U1_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	ldelem.u1\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                class_located = (system->cliManager).metadataManager->getTypeDescriptorFromElementType(system->cliManager.metadataManager, ELEMENT_TYPE_U1);
                assert(class_located != NULL);
                is_invalid_basic_block = make_ldelem(system, method, cilStack, &current_label_ID, labels, bytes_read, class_located, stack);
                break;
            case LDELEM_I2_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	ldelem.i2\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                class_located = (system->cliManager).metadataManager->getTypeDescriptorFromElementType(system->cliManager.metadataManager, ELEMENT_TYPE_I2);
                assert(class_located != NULL);
                is_invalid_basic_block = make_ldelem(system, method, cilStack, &current_label_ID, labels, bytes_read, class_located, stack);
                break;
            case LDELEM_U2_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	ldelem.u2\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                class_located = (system->cliManager).metadataManager->getTypeDescriptorFromElementType(system->cliManager.metadataManager, ELEMENT_TYPE_U2);
                assert(class_located != NULL);
                is_invalid_basic_block = make_ldelem(system, method, cilStack, &current_label_ID, labels, bytes_read, class_located, stack);
                break;
            case LDELEM_I4_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	ldelem.i4\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                class_located = (system->cliManager).metadataManager->getTypeDescriptorFromElementType(system->cliManager.metadataManager, ELEMENT_TYPE_I4);
                assert(class_located != NULL);
                is_invalid_basic_block = make_ldelem(system, method, cilStack, &current_label_ID, labels, bytes_read, class_located, stack);
                break;
            case LDELEM_U4_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	ldelem.u4\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                class_located = (system->cliManager).metadataManager->getTypeDescriptorFromElementType(system->cliManager.metadataManager, ELEMENT_TYPE_U4);
                assert(class_located != NULL);
                is_invalid_basic_block = make_ldelem(system, method, cilStack, &current_label_ID, labels, bytes_read, class_located, stack);
                break;
            case LDELEM_I8_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	ldelem.i8\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                class_located = (system->cliManager).metadataManager->getTypeDescriptorFromElementType(system->cliManager.metadataManager, ELEMENT_TYPE_I8);
                assert(class_located != NULL);
                is_invalid_basic_block = make_ldelem(system, method, cilStack, &current_label_ID, labels, bytes_read, class_located, stack);
                break;
            case LDELEM_I_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	ldelem.i\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                class_located = (system->cliManager).metadataManager->getTypeDescriptorFromElementType(system->cliManager.metadataManager, ELEMENT_TYPE_I);
                assert(class_located != NULL);
                is_invalid_basic_block = make_ldelem(system, method, cilStack, &current_label_ID, labels, bytes_read, class_located, stack);
                break;
            case LDELEM_R4_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	ldelem.r4\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                class_located = (system->cliManager).metadataManager->getTypeDescriptorFromElementType(system->cliManager.metadataManager, ELEMENT_TYPE_R4);
                assert(class_located != NULL);
                is_invalid_basic_block = make_ldelem(system, method, cilStack, &current_label_ID, labels, bytes_read, class_located, stack);
                break;
            case LDELEM_R8_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	ldelem.r8\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                class_located = (system->cliManager).metadataManager->getTypeDescriptorFromElementType(system->cliManager.metadataManager, ELEMENT_TYPE_R8);
                assert(class_located != NULL);
                is_invalid_basic_block = make_ldelem(system, method, cilStack, &current_label_ID, labels, bytes_read, class_located, stack);
                break;
            case LDELEM_REF_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	ldelem.ref\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = make_ldelem(system, method, cilStack, &current_label_ID, labels, bytes_read, NULL, stack);
                break;
            case STELEM_I_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	stelem.i\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                class_located = (system->cliManager).metadataManager->getTypeDescriptorFromElementType(system->cliManager.metadataManager, ELEMENT_TYPE_I);
                assert(class_located != NULL);
                is_invalid_basic_block = make_stelem(system, method, cilStack, &current_label_ID, labels, bytes_read, class_located, stack);
                break;
            case STELEM_I1_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	stelem.i1\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                class_located = (system->cliManager).metadataManager->getTypeDescriptorFromElementType(system->cliManager.metadataManager, ELEMENT_TYPE_I1);
                assert(class_located != NULL);
                is_invalid_basic_block = make_stelem(system, method, cilStack, &current_label_ID, labels, bytes_read, class_located, stack);
                break;
            case STELEM_I2_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	stelem.i2\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                class_located = (system->cliManager).metadataManager->getTypeDescriptorFromElementType(system->cliManager.metadataManager, ELEMENT_TYPE_I2);
                assert(class_located != NULL);
                is_invalid_basic_block = make_stelem(system, method, cilStack, &current_label_ID, labels, bytes_read, class_located, stack);
                break;
            case STELEM_I4_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	stelem.i4\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                class_located = (system->cliManager).metadataManager->getTypeDescriptorFromElementType(system->cliManager.metadataManager, ELEMENT_TYPE_I4);
                assert(class_located != NULL);
                is_invalid_basic_block = make_stelem(system, method, cilStack, &current_label_ID, labels, bytes_read, class_located, stack);
                break;
            case STELEM_I8_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	stelem.i8\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                class_located = (system->cliManager).metadataManager->getTypeDescriptorFromElementType(system->cliManager.metadataManager, ELEMENT_TYPE_I8);
                assert(class_located != NULL);
                is_invalid_basic_block = make_stelem(system, method, cilStack, &current_label_ID, labels, bytes_read, class_located, stack);
                break;
            case STELEM_R4_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	stelem.r4\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                class_located = (system->cliManager).metadataManager->getTypeDescriptorFromElementType(system->cliManager.metadataManager, ELEMENT_TYPE_R4);
                assert(class_located != NULL);
                is_invalid_basic_block = make_stelem(system, method, cilStack, &current_label_ID, labels, bytes_read, class_located, stack);
                break;
            case STELEM_R8_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	stelem.r8\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                class_located = (system->cliManager).metadataManager->getTypeDescriptorFromElementType(system->cliManager.metadataManager, ELEMENT_TYPE_R8);
                assert(class_located != NULL);
                is_invalid_basic_block = make_stelem(system, method, cilStack, &current_label_ID, labels, bytes_read, class_located, stack);
                break;
            case STELEM_REF_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	stelem.ref\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = make_stelem(system, method, cilStack, &current_label_ID, labels, bytes_read, NULL, stack);
                break;
            case LDELEM_OPCODE:
                memcpy(&new_token, body_stream + bytes_read, 4);
                bytes_read += 4;
                PDEBUG("CILIR:<CIL Inst %d>	ldelem		0x%X\n", opcode_counter, new_token);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_ldelem(system, cilStack, container, current_binary, method, bytes_read, new_token, stack, &current_label_ID, labels);
                break;
            case STELEM_OPCODE:
                memcpy(&new_token, body_stream + bytes_read, 4);
                bytes_read += 4;
                PDEBUG("CILIR:<CIL Inst %d>	stelem		0x%X\n", opcode_counter, new_token);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_stelem(system, cilStack, container, current_binary, method, bytes_read, new_token, stack, &current_label_ID, labels);
                break;
            case UNBOX_ANY_OPCODE:
                memcpy(&new_token, body_stream + bytes_read, 4);
                bytes_read += 4;
                PDEBUG("CILIR:<CIL Inst %d>	unbox.any	0x%X\n", opcode_counter, new_token);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_unbox_any(system, cilStack, container, current_binary, method, bytes_read, new_token, stack, &current_label_ID, labels);
                break;
            case CONV_OVF_I1_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	conv.ovf.i1\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_conv(&(system->cliManager), method, bytes_read, stack, IRINT8, 1, 0, 1);
                break;
            case CONV_OVF_U1_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	conv.ovf.u1\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_conv(&(system->cliManager), method, bytes_read, stack, IRUINT8, 1, JITTRUE, JITTRUE);
                break;
            case CONV_OVF_I2_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	conv.ovf.i2\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_conv(&(system->cliManager), method, bytes_read, stack, IRINT16, 1, 0, 1);
                break;
            case CONV_OVF_U2_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	conv.ovf.u2\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_conv(&(system->cliManager), method, bytes_read, stack, IRUINT16, 1, JITTRUE, JITTRUE);
                break;
            case CONV_OVF_I4_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	conv.ovf.i4\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_conv(&(system->cliManager), method, bytes_read, stack, IRINT32, 1, 0, 1);
                break;
            case CONV_OVF_U4_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	conv.ovf.u4\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_conv(&(system->cliManager), method, bytes_read, stack, IRUINT32, 1, JITTRUE, JITTRUE);
                break;
            case CONV_OVF_I8_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	conv.ovf.i8\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_conv(&(system->cliManager), method, bytes_read, stack, IRINT64, 1, 0, 1);
                break;
            case CONV_OVF_U8_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	conv.ovf.u8\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_conv(&(system->cliManager), method, bytes_read, stack, IRUINT64, 1, JITTRUE, JITTRUE);
                break;
            case REFANYVAL_OPCODE:
                memcpy(&new_token, body_stream + bytes_read, 4);
                bytes_read += 4;
                PDEBUG("CILIR:<CIL Inst %d>	refanyval	0x%X\n", opcode_counter, new_token);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_refanyval(system, method, cilStack, &current_label_ID, labels, bytes_read, stack, container, current_binary, new_token);
                break;
            case CKFINITE_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	ckfinite\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                read_from_buffer(buffer, 0, 4, &paramInt32);
                is_invalid_basic_block = translate_cil_Ckfinite(cliManager, method, cilStack, &current_label_ID, bytes_read, paramInt32, labels, system, 1, stack);
                break;
            case MKREFANY_OPCODE:     //FIXME todo
                memcpy(&new_token, body_stream + bytes_read, 4);
                bytes_read += 4;
                PDEBUG("CILIR:<CIL Inst %d>	mkrefany	0x%X\n", opcode_counter, new_token);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                print_err("CILIR: mkrefany instruction is not implemented. ", 0);
                abort();
                break;
            case LDTOKEN_OPCODE:
                memcpy(&new_token, body_stream + bytes_read, 4);
                bytes_read += 4;
                PDEBUG("CILIR:<CIL Inst %d>	ldtoken		0x%X\n", opcode_counter, new_token);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_ldtoken(system, method, container, current_binary, stack, bytes_read, new_token, &current_label_ID, labels);
                break;
            case CONV_U2_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	conv.u2\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_conv(&(system->cliManager), method, bytes_read, stack, IRUINT16, 0, JITTRUE, JITTRUE);
                break;
            case CONV_U1_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	conv.u1\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_conv(&(system->cliManager), method, bytes_read, stack, IRUINT8, 0, JITTRUE, JITTRUE);
                break;
            case CONV_I_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	conv.i\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_conv(&(system->cliManager), method, bytes_read, stack, IRNINT, 0, 0, 1);
                break;
            case CONV_OVF_I_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	conv.ovf.i\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_conv(&(system->cliManager), method, bytes_read, stack, IRNUINT, 1, 0, 1);
                break;
            case CONV_OVF_U_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	conv.ovf.u\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_conv(&(system->cliManager), method, bytes_read, stack, IRNINT, 1, JITTRUE, JITTRUE);
                break;
            case ADD_OVF_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	add.ovf\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = _translate_arithmetic_operation(&(system->cliManager), method, bytes_read, stack, IRADDOVF);
                break;
            case ADD_OVF_UN_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	add.ovf.un\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = _translate_arithmetic_operation_un(&(system->cliManager), method, bytes_read, stack, IRADDOVF);
                break;
            case MUL_OVF_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	mul.ovf\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = _translate_arithmetic_operation(&(system->cliManager), method, bytes_read, stack, IRMULOVF);
                break;
            case MUL_OVF_UN_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	mul.ovf.un\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = _translate_arithmetic_operation_un(&(system->cliManager), method, bytes_read, stack, IRMULOVF);
                break;
            case SUB_OVF_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	sub.ovf\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = _translate_arithmetic_operation(&(system->cliManager), method, bytes_read, stack, IRSUBOVF);
                break;
            case SUB_OVF_UN_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	sub.ovf.un\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = _translate_arithmetic_operation_un(&(system->cliManager), method, bytes_read, stack, IRSUBOVF);
                break;
            case ENDFINALLY_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	endfault | endfinally\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_endfinally(method, bytes_read, finallyLabels);
                break;
            case LEAVE_OPCODE:
                memcpy(&paramUInt32, body_stream + bytes_read, 4);
                bytes_read += 4;
                PDEBUG("CILIR:<CIL Inst %d>	leave		%d\n", opcode_counter, paramUInt32);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_leave(method, cilStack, &current_label_ID, bytes_read, paramUInt32, labels, 5, stack);
                break;
            case LEAVE_S_OPCODE:
                memcpy(&paramInt8, body_stream + bytes_read, 1);
                bytes_read += 1;
                PDEBUG("CILIR:<CIL Inst %d>	leave.s		%d\n", opcode_counter, paramInt8);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_leave(method, cilStack, &current_label_ID, bytes_read, paramInt8, labels, 2, stack);
                break;
            case STIND_I_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	stind.i\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_stind(cliManager, method, cilStack, bytes_read, stack,  &current_label_ID, labels, IRNINT);
                break;
            case CONV_U_OPCODE:
                PDEBUG("CILIR:<CIL Inst %d>	conv.u\n", opcode_counter);
                if (is_invalid_basic_block == 1) {
                    break;
                }
                is_invalid_basic_block = translate_cil_conv(&(system->cliManager), method, bytes_read, stack, IRNUINT, 0, JITTRUE, JITTRUE);
                break;
            case FE_OPCODE:
                memcpy(buffer, body_stream + bytes_read, 1);
                bytes_read += 1;
                switch (buffer[0]) {
                    case ARGLIST_OPCODE:
                        PDEBUG("CILIR:<CIL Inst %d>	arglist\n", opcode_counter);
                        if (is_invalid_basic_block == 1) {
                            break;
                        }
                        is_invalid_basic_block = translate_cil_arglist(cliManager, cilStack, method, bytes_read, stack, &current_label_ID, labels);
                        break;
                    case CEQ_OPCODE:
                        PDEBUG("CILIR:<CIL Inst %d>	Ceq\n", opcode_counter);
                        if (is_invalid_basic_block == 1) {
                            break;
                        }
                        is_invalid_basic_block = translate_cil_compare(cliManager, method, cilStack, bytes_read, IREQ, stack, &current_label_ID, labels, 0, 0, JITTRUE);
                        break;
                    case CGT_OPCODE:
                        PDEBUG("CILIR:<CIL Inst %d>	Cgt\n", opcode_counter);
                        if (is_invalid_basic_block == 1) {
                            break;
                        }
                        is_invalid_basic_block = translate_cil_compare(cliManager, method, cilStack, bytes_read, IRGT, stack, &current_label_ID, labels, 0, 0, JITTRUE);
                        break;
                    case CGT_UN_OPCODE:
                        PDEBUG("CILIR:<CIL Inst %d>	Cgt.un\n", opcode_counter);
                        if (is_invalid_basic_block == 1) {
                            break;
                        }
                        is_invalid_basic_block = translate_cil_compare(cliManager, method, cilStack, bytes_read, IRGT, stack, &current_label_ID, labels, 1, 0, JITTRUE);
                        break;
                    case CLT_OPCODE:
                        PDEBUG("CILIR:<CIL Inst %d>	Clt\n", opcode_counter);
                        if (is_invalid_basic_block == 1) {
                            break;
                        }
                        is_invalid_basic_block = translate_cil_compare(cliManager, method, cilStack, bytes_read, IRLT, stack, &current_label_ID, labels,  0, 0, JITTRUE);
                        break;
                    case CLT_UN_OPCODE:
                        PDEBUG("CILIR:<CIL Inst %d>	clt.un\n", opcode_counter);
                        if (is_invalid_basic_block == 1) {
                            break;
                        }
                        is_invalid_basic_block = translate_cil_compare(cliManager, method, cilStack, bytes_read, IRLT, stack, &current_label_ID, labels, 1, 0, JITTRUE);
                        break;
                    case LDFTN_OPCODE:
                        memcpy(&new_token, body_stream + bytes_read, 4);
                        bytes_read += 4;
                        PDEBUG("CILIR:<CIL Inst %d>	ldftn	0x%X\n", opcode_counter, new_token);
                        is_invalid_basic_block = translate_cil_ldftn(system, method, cilStack, bytes_read, stack, container, current_binary, new_token);
                        break;
                    case LDVIRTFTN_OPCODE:
                        memcpy(&new_token, body_stream + bytes_read, 4);
                        bytes_read += 4;
                        PDEBUG("CILIR:<CIL Inst %d>	ldvirtftn	0x%X\n", opcode_counter, new_token);
                        is_invalid_basic_block = translate_cil_ldvirtftn(system, method, cilStack, bytes_read, stack, container, current_binary, new_token, &current_label_ID, labels);
                        break;
                        break;
                    case LDARG_OPCODE:
                        memcpy(&paramUInt16, body_stream + bytes_read, 2);
                        bytes_read += 2;
                        PDEBUG("CILIR:<CIL Inst %d>	ldarg		%d\n", opcode_counter, paramUInt16);
                        if (is_invalid_basic_block == 1) {
                            break;
                        }
                        is_invalid_basic_block = translate_cil_ldarg(cliManager, method, bytes_read, paramUInt16, stack);
                        break;
                    case LDARGA_OPCODE:
                        memcpy(&paramUInt16, body_stream + bytes_read, 2);
                        bytes_read += 2;
                        PDEBUG("CILIR:<CIL Inst %d>	ldarga	0x%X\n", opcode_counter, paramUInt16);
                        if (is_invalid_basic_block == 1) {
                            break;
                        }
                        is_invalid_basic_block = translate_cil_ldarga(system, method, bytes_read, paramUInt16, stack);
                        break;
                    case STARG_OPCODE:
                        memcpy(&paramUInt16, body_stream + bytes_read, 2);
                        bytes_read += 2;
                        PDEBUG("CILIR:<CIL Inst %d>	starg		%d\n", opcode_counter, paramUInt16);
                        if (is_invalid_basic_block == 1) {
                            break;
                        }
                        is_invalid_basic_block = translate_cil_starg(cliManager, method, bytes_read, paramUInt16, stack);
                        break;
                    case LDLOC_OPCODE:
                        memcpy(&paramUInt16, body_stream + bytes_read, 2);
                        bytes_read += 2;
                        PDEBUG("CILIR:<CIL Inst %d>	ldloc		%d\n", opcode_counter, paramUInt16);
                        if (is_invalid_basic_block == 1) {
                            break;
                        }
                        is_invalid_basic_block = translate_cil_ldloc(system, method, bytes_read, paramUInt16, stack);
                        break;
                    case LDLOCA_OPCODE:
                        memcpy(&paramUInt16, body_stream + bytes_read, 2);
                        bytes_read += 2;
                        PDEBUG("CILIR:<CIL Inst %d>	ldloca	0x%X\n", opcode_counter, paramUInt16);
                        if (is_invalid_basic_block == 1) {
                            break;
                        }
                        is_invalid_basic_block = translate_cil_ldloca(system, method, bytes_read, paramUInt16, stack, localsInit, &current_label_ID, labels);
                        break;
                    case STLOC_OPCODE:
                        memcpy(&paramUInt16, body_stream + bytes_read, 2);
                        bytes_read += 2;
                        PDEBUG("CILIR:<CIL Inst %d>	stloc		%d\n", opcode_counter, paramUInt16);
                        if (is_invalid_basic_block == 1) {
                            break;
                        }
                        is_invalid_basic_block = translate_cil_stloc(cliManager, method, bytes_read, paramUInt16, stack);
                        break;
                    case LDCALLOC_OPCODE:
                        PDEBUG("CILIR:<CIL Inst %d>	ldcalloc\n", opcode_counter);
                        if (is_invalid_basic_block == 1) {
                            break;
                        }
                        is_invalid_basic_block = translate_cil_ldcalloc(cilStack, method, bytes_read, stack, &current_label_ID, labels);
                        break;
                    case ENDFILTER_OPCODE:
                        PDEBUG("CILIR:<CIL Inst %d>	Endfilter\n", opcode_counter);
                        if (is_invalid_basic_block == 1) {
                            break;
                        }
                        instruction = method->newIRInstr(method);
                        instruction->type = IRENDFILTER;
                        (stack->top)--;
                        memcpy(&(instruction->param_1), &(stack->stack[stack->top]), sizeof(ir_item_t));
                        assert((instruction->param_1).internal_type == IRINT32);
                        instruction->byte_offset = bytes_read;
                        break;
                    case UNALIGNED_OPCODE:
                        memcpy(buffer, body_stream + bytes_read, 1);
                        bytes_read += 1;
                        PDEBUG("CILIR:PREFIX	unaligned	0x%X\n", buffer[0]);
                        unaligned_prefix_value = buffer[0];
                        if (is_invalid_basic_block == 1) {
                            break;
                        }
                        break;
                    case VOLATILE_OPCODE:         //FIXME todo
                        PDEBUG("CILIR:PREFIX	volatile\n");
                        if (is_invalid_basic_block == 1) {
                            break;
                        }
                        print_err("ILDJIT: Warning: CILIR: volatile instruction is not implemented. ", 0);
                        break;
                    case TAIL_OPCODE:
                        memcpy(buffer, body_stream + bytes_read, 1);
                        bytes_read += 1;
                        switch (buffer[0]) {
                            case 0x28:
                                memcpy(&new_token, body_stream + bytes_read, 4);
                                bytes_read += 4;
                                PDEBUG("CILIR:<CIL Inst %d>	tail call	0x%X\n", opcode_counter, new_token);
                                if (is_invalid_basic_block == 1) {
                                    break;
                                }
                                is_invalid_basic_block = translate_cil_call(system, cilStack, container, current_binary, method, new_token, bytes_read, stack, 1, &current_label_ID, labels, 0);
                                break;
                            case 0x29:
                                memcpy(&new_token, body_stream + bytes_read, 4);
                                bytes_read += 4;
                                PDEBUG("CILIR:<CIL Inst %d>	tail calli	0x%X\n", opcode_counter, new_token);
                                if (is_invalid_basic_block == 1) {
                                    break;
                                }
                                is_invalid_basic_block = translate_cil_calli(cliManager, cilStack, container, current_binary, method, new_token, bytes_read, stack, JITTRUE, &current_label_ID, labels);
                                break;
                            case 0x6F:
                                memcpy(&new_token, body_stream + bytes_read, 4);
                                bytes_read += 4;
                                PDEBUG("CILIR:<CIL Inst %d>	tail callvirt 0x%X\n", opcode_counter, new_token);
                                if (is_invalid_basic_block == 1) {
                                    break;
                                }
                                is_invalid_basic_block = translate_cil_vcall(system, cilStack, container, current_binary, method, new_token, bytes_read, stack, 1, NULL, &current_label_ID, labels);
                                break;
                            default:
                                snprintf((char *) buffer, sizeof(buffer), "CILIR: ERROR = <CIL Inst %d>	OPCODE 0x%X"" is not known. ", opcode_counter, (JITUINT16) buffer[0]);
                                print_err((char *) buffer, 0);
                                abort();
                        }
                        break;
                    case INITOBJ_OPCODE:
                        memcpy(&new_token, body_stream + bytes_read, 4);
                        bytes_read += 4;
                        PDEBUG("CILIR: initobj	0x%X\n", new_token);
                        if (is_invalid_basic_block == 1) {
                            break;
                        }
                        is_invalid_basic_block = translate_cil_initobj(system, cilStack, container, current_binary, method, bytes_read, new_token, stack, &current_label_ID, labels);
                        break;
                    case CONSTRAINED_OPCODE:
                        memcpy(&new_token, body_stream + bytes_read, 4);
                        bytes_read += 5;
                        PDEBUG("CILIR:PREFIX	constrained     0x%X\n", new_token);
                        class_located = (system->cliManager).metadataManager->getTypeDescriptorFromToken(system->cliManager.metadataManager, container, current_binary, new_token);
                        memcpy(&new_token, body_stream + bytes_read, 4);
                        bytes_read += 4;
                        PDEBUG("CILIR:<CIL Inst %d>	constrained callvirt 0x%X\n", opcode_counter, new_token);
                        if (is_invalid_basic_block == 1) {
                            break;
                        }
                        is_invalid_basic_block = translate_cil_vcall(system, cilStack, container, current_binary, method, new_token, bytes_read, stack, 0, class_located, &current_label_ID, labels);
                        break;
                    case CPBLK_OPCODE:
                        PDEBUG("CILIR:PREFIX	cpblk\n");
                        if (is_invalid_basic_block == 1) {
                            break;
                        }
                        is_invalid_basic_block = translate_cil_cpblk(system, cilStack, method, bytes_read, stack, &current_label_ID, labels, unaligned_prefix_value);
                        if (unaligned_prefix_value != 0) {
                            unaligned_prefix_value = 0;
                        }
                        break;
                    case INITBLK_OPCODE:
                        PDEBUG("CILIR:PREFIX	initblk\n");
                        if (is_invalid_basic_block == 1) {
                            break;
                        }
                        is_invalid_basic_block = translate_cil_initblk(system, cilStack, method, bytes_read, stack, &current_label_ID, labels, unaligned_prefix_value);
                        if (unaligned_prefix_value != 0) {
                            unaligned_prefix_value = 0;
                        }
                        break;
                    case NO_OPCODE:         //FIXME todo
                        memcpy(buffer, body_stream + bytes_read, 1);
                        bytes_read += 1;
                        PDEBUG("CILIR:PREFIX	no      0x%X\n", buffer[0]);
                        if (is_invalid_basic_block == 1) {
                            break;
                        }
                        print_err("CILIR: no instruction is not implemented. ", 0);
                        abort();
                        break;
                    case RETHROW_OPCODE:
                        PDEBUG("CILIR:PREFIX	rethrow\n");
                        if (is_invalid_basic_block == 1) {
                            break;
                        }
                        is_invalid_basic_block = translate_cil_rethrow(cliManager, method, cilStack, &current_label_ID, labels, bytes_read, stack, &exceptionVarID);
                        break;
                    case SIZEOF_OPCODE:
                        PDEBUG("CILIR:PREFIX	sizeof\n");
                        if (is_invalid_basic_block == 1) {
                            break;
                        }
                        memcpy(&new_token, body_stream + bytes_read, 4);
                        bytes_read += 4;
                        is_invalid_basic_block = translate_cil_sizeof(cliManager, method, bytes_read, stack, container, current_binary, new_token);
                        break;
                    case REFANYTYPE_OPCODE:         //FIXME todo
                        print_err("CILIR: refanytype instruction is not implemented. ", 0);
                        abort();
                        break;
                    case READONLY_OPCODE:         //FIXME todo
                        PDEBUG("CILIR:PREFIX	readonly\n");
                        if (is_invalid_basic_block == 1) {
                            break;
                        }
                        print_err("CILIR: readonly instruction is not implemented. ", 0);
                        abort();
                        break;
                    default:
                        snprintf((char *) buffer, sizeof(buffer), "CILIR: ERROR = <CIL Inst %d>	OPCODE 0x%X 0x%X is not known. ", opcode_counter, (JITUINT16) opcode[0], (JITUINT16) buffer[0]);
                        print_err((char *) buffer, 0);
                        abort();
                }
                break;
            case PREFIXREF_OPCODE:     //FIXME todo
                PDEBUG("CILIR:PREFIX	prefixref\n");
                print_err("CILIR: prefixref instruction is not implemented. ", 0);
                abort();
                break;
            default:
                snprintf((char *) buffer, sizeof(buffer), "CILIR: ERROR = <CIL Inst %d>	OPCODE 0x%X is not known. ", opcode_counter, (JITUINT16) opcode[0]);
                print_err((char *) buffer, 0);
                abort();
        }
        assert(internal_isStackCorrect(cliManager, method, stack) == JITTRUE);

#ifdef PRINTDEBUG
        PDEBUG("CILIR: Instruction translated\n");
        if (is_invalid_basic_block == 1) {
            PDEBUG("CILIR: INSTRUCTION NOT TRANSLATED BECAUSE THE BASIC BLOCK WAS INVALIDATED \n");
        }
        PDEBUG("CILIR:	Stack->top		= %d\n", stack->top);
        PDEBUG("CILIR:	Locals number		= %d\n", method->getLocalsNumber(method));
        PDEBUG("CILIR:	Parameters		= %d\n", method->getParametersNumber(method));
        PDEBUG("CILIR:	Max locals number	= %d\n", method->getMaxVariables(method));
        PDEBUG("CILIR:	Var count               = %d\n", method->getMaxVariables(method));
        PDEBUG("CILIR:	Current label ID	= \"L%d\"\n", current_label_ID);
        print_stack(stack, method->getParametersNumber(method), method->getLocalsNumber(method));
        assert(method->getParametersNumber(method) + method->getLocalsNumber(method) <= stack->top );
#endif

        /* Decrement the counter of each label  */
        PDEBUG("CILIR:	Decrement the counter of each label\n");
        labels_decrement_counter(labels, bytes_read - old_bytes_read);

#ifdef PRINTDEBUG
        PDEBUG("CILIR: Print the labels\n");
        print_labels(labels);
#endif

        assert(opcode[1] == 0);

        /* Check if the body is finished	*/
        PDEBUG("CILIR:	Bytes read of the body	= %d\n", bytes_read);
        PDEBUG("CILIR:	Bytes of the body	= %d\n", body_size);
        if (bytes_read >= body_size) {
            if (bytes_read > body_size) {
                print_err("CILIR: WARNING: We have read more bytes than the decleared size of the method. ", 0);
                abort();
            }

            /* verify if it's necessary to construct a catcher block */
            /* we shall define a catcher block. A catcher block starts with
             * a IRSTARTCATCHER instruction.
             * The main goal of the catcher is to manager properly each
             * possible exception that could be
             * raised during the method execution. In order, to build
             * properly the catcher, we must use the informations regarding
             * the try_blocks and the try_handlers: more precisely,
             * we have to `slide` the try_blocks structure (that actually is
             * implemented as a hierarchy of blocks) and, for each try_block
             * and handler we'll find, a proper "branch code" must be
             * defined. ACTUALLY every method defines a catcher block even if its code doesn't
             * contains any form of protected block. This is mandatory because
             * it's possible that an uncaught exception object may cause the end
             * of the execution by walking-up the stack and exiting the main.
             * This ensure that all the thrown exception objects will be at last constructed */
            make_catcher_instructions(system, method, cilStack, &current_label_ID, labels, bytes_read, stack, &exceptionVarID);

            /* Set the exit condition		*/
            exit_condition = JITTRUE;
        }
    }

    /* Final Assertions			*/
#ifdef DEBUG
    PDEBUG("CILIR: End body\n");
    PDEBUG("CILIR:	        Bytes read = %d\n", bytes_read);
    if (body_size != 0) {
        PDEBUG("CILIR:	        Body size = %d\n", body_size);
        if (bytes_read != body_size) {
            PDEBUG("CILIR:	        WARNING= Body size is not long as described in the header\n");
        }
    }
    assert(((system->cliManager).methods).container != NULL);
    assert(method != NULL);
#endif

    /* Update the total number of CIL	*
     * opcodes decoded by ILDJIT		*/
    if ((system->IRVM).behavior.profiler >= 2) {
        ((system->program).cil_opcodes_decoded) += opcode_counter;
    }
    assert(internal_check_ir(method->getIRMethod(method)));

    /* Free the memory			*/
    cilStack->destroy(cilStack);
    freeMemory(body_stream);
    item = xanList_first(labels);
    while (item != NULL) {
        t_label *label;
        label = item->data;
        assert(label != NULL);
        if (label->starting_blocks != NULL) {
            xanList_destroyListAndData(label->starting_blocks);
            label->starting_blocks = NULL;
        }
        item = item->next;
    }
    xanList_destroyListAndData(labels);
    xanStack_destroyStack(finallyLabels);

    return error;
}

static inline JITINT32 internal_check_ir (ir_method_t *method) {
    JITINT32 b;
    ir_instruction_t        *inst;

    IRMETHOD_lock(method);
    for (b = 0; b < IRMETHOD_getInstructionsNumber(method); b++) {
        inst = IRMETHOD_getInstructionAtPosition(method, b);
        if ((inst->param_1).internal_type == IROFFSET) {
            return JITFALSE;
        }
        if ((inst->param_2).internal_type == IROFFSET) {
            return JITFALSE;
        }
        if ((inst->param_3).internal_type == IROFFSET) {
            return JITFALSE;
        }
    }
    IRMETHOD_unlock(method);
    return JITTRUE;
}

static inline ir_instruction_t * find_insert_point (Method method, JITUINT32 insert_point) {
    ir_instruction_t        *inst;
    ir_method_t             *irMethod;

    /* Assertions		*/
    assert(method != NULL);
    assert(insert_point > 0);

    PDEBUG("CILIR:		Search the entry point for the insert point %d\n", insert_point);

    irMethod = method->getIRMethod(method);
    IRMETHOD_lock(irMethod);
    inst = IRMETHOD_getNextInstruction(irMethod, NULL);
    assert(inst != NULL);

    while (inst != NULL) {
        PDEBUG("CILIR:		Inst->byte_offset = %d\n", inst->byte_offset);
        if (inst->byte_offset >= insert_point) {
            PDEBUG("CILIR:			Found\n");
            IRMETHOD_unlock(irMethod);
            return inst;
        }
        inst = IRMETHOD_getNextInstruction(irMethod, inst);
    }
    IRMETHOD_unlock(irMethod);

    PDEBUG("CILIR:		Not found\n");
    return NULL;
}

static inline t_label * find_label_at_position (XanList *labels, JITUINT32 bytes_offset) {
    XanListItem     *item;
    t_label *label;

    item = xanList_first(labels);
    while (item != NULL) {
        label = (t_label *) item->data;
        assert(label != NULL);
        if (label->byte_offset == bytes_offset) {
            return label;
        }
        item = item->next;
    }

    return NULL;
}

static inline void labels_decrement_counter (XanList *labels, JITUINT32 value) {
    XanListItem     *item;
    t_label *label;

    PDEBUG("CILIR: Decrement labels counter\n");

    item = xanList_first(labels);

    while (item != NULL) {
        label = (t_label *) item->data;
        if (label->counter > 0) {
            (label->counter) -= value;
            if (label->counter < 0) {
                label->counter = 0;
            }
        }
        item = item->next;
    }
}

static inline t_label * fetch_label (XanList *labels, JITUINT32 ir_position) {
    XanListItem     *item;
    t_label *label;

    /* Check if exist another label already inserted in ir_position	*/
    item = xanList_first(labels);

    while (item != NULL) {
        label = (t_label *) item->data;
        if (label->ir_position == ir_position) {
            return NULL;
        }
        item = item->next;
    }

    item = xanList_first(labels);

    while (item != NULL) {
        label = (t_label *) item->data;
        if (label->counter == 0 && label->ir_position == -1) {
            return label;
        }
        item = item->next;
    }

    return NULL;
}

static inline void print_labels (XanList *labels) {
#ifdef PRINTDEBUG
    XanListItem     *item;
    t_label *label;

    /* Assertions			*/
    assert(labels != NULL);

    PDEBUG("CILIR: Print labels\n");
    item = xanList_first(labels);
    while (item != NULL) {
        label = (t_label *) item->data;
        PDEBUG("CILIR:		Label \"L%d\"\n", label->ID);
        PDEBUG("CILIR:			Counter         = %d\n", label->counter);
        PDEBUG("CILIR:			Bytes offset	= %d\n", label->byte_offset);
        PDEBUG("CILIR:			IR position	= %d\n", label->ir_position);
        if (label->type == DEFAULT_LABEL_TYPE) {
            PDEBUG("CILIR:			type		= %s\n", "DEFAULT_LABEL_TYPE");
        } else {
            PDEBUG("CILIR:			type		= %s\n", "EXCEPTION_HANDLING_LABEL_TYPE");
            XanListItem         *current_block;
            current_block = xanList_first(label->starting_blocks);
            while (current_block != NULL) {
                t_try_block     *current_try_block;
                current_try_block = (t_try_block *) current_block->data;
                PDEBUG("CILIR:			TRY_BLOCK_START[ \"L%d\" , \"L%d\"] \n", current_try_block->try_start_label_ID, current_try_block->try_end_label_ID );
                current_block = current_block->next;
            }
            if (label->starting_handler != NULL) {
                if ((label->starting_handler)->type == EXCEPTION_TYPE) {
                    PDEBUG("CILIR:		CATCH_HANDLER_START[ \"L%d\" , \"L%d\"] \n", (label->starting_handler)->handler_start_label_ID, (label->starting_handler)->handler_end_label_ID );
                }
                if ((label->starting_handler)->type == FILTER_TYPE) {
                    PDEBUG("CILIR:		FILTER_HANDLER_START[ \"L%d\" , \"L%d\"] \n", (label->starting_handler)->filter_start_label_ID, (label->starting_handler)->handler_end_label_ID );
                }
                if ((label->starting_handler)->type == FINALLY_TYPE) {
                    PDEBUG("CILIR:		FINALLY_HANDLER_START[ \"L%d\" , \"L%d\"] \n", (label->starting_handler)->handler_start_label_ID, (label->starting_handler)->handler_end_label_ID );
                }
                if ((label->starting_handler)->type == FAULT_TYPE) {
                    PDEBUG("CILIR:		FAULT_HANDLER_START[ \"L%d\" , \"L%d\"] \n", (label->starting_handler)->handler_start_label_ID, (label->starting_handler)->handler_end_label_ID );
                }
                PDEBUG("CILIR:			PARENT_BLOCK :: [ \"L%d\" , \"L%d\"] \n", ((label->starting_handler)->owner_block)->try_start_label_ID, ((label->starting_handler)->owner_block)->try_end_label_ID );
            }
            if (label->ending_handler != NULL) {
                if ((label->ending_handler)->type == EXCEPTION_TYPE) {
                    PDEBUG("CILIR:		CATCH_HANDLER_END[ \"L%d\" , \"L%d\"] \n", (label->ending_handler)->handler_start_label_ID, (label->ending_handler)->handler_end_label_ID );
                }
                if ((label->ending_handler)->type == FILTER_TYPE) {
                    PDEBUG("CILIR:		FILTER_HANDLER_END[ \"L%d\" , \"L%d\"] \n", (label->ending_handler)->filter_start_label_ID, (label->ending_handler)->handler_end_label_ID );
                }
                if ((label->ending_handler)->type == FINALLY_TYPE) {
                    PDEBUG("CILIR:		FINALLY_HANDLER_END[ \"L%d\" , \"L%d\"] \n", (label->ending_handler)->handler_start_label_ID, (label->ending_handler)->handler_end_label_ID );
                }
                if ((label->ending_handler)->type == FAULT_TYPE) {
                    PDEBUG("CILIR:		FAULT_HANDLER_END[ \"L%d\" , \"L%d\"] \n", (label->ending_handler)->handler_start_label_ID, (label->ending_handler)->handler_end_label_ID );
                }
                PDEBUG("CILIR:		PARENT_BLOCK :: [ \"L%d\" , \"L%d\"] \n", ((label->ending_handler)->owner_block)->try_start_label_ID, ((label->ending_handler)->owner_block)->try_end_label_ID );
            }
            if (label->ending_block != NULL) {
                PDEBUG("CILIR:		TRY_BLOCK_END[ \"L%d\" , \"L%d\"] \n", (label->ending_block)->try_start_label_ID, (label->ending_block)->try_end_label_ID );
            }
        }
        item = item->next;
    }
#endif
}

static inline void print_stack (t_stack *stack, JITUINT32 parameters, JITUINT32 locals) {
    JITUINT32 count;

    /* Assertions		*/
    assert(stack != NULL);

    PDEBUG("CILIR: Print stack. Top = %d, %d Parameters, %d Locals\n", stack->top, parameters, locals);
    PDEBUG("CILIR: ----------------  Start parameters\n");
    for (count = 0; count < parameters; count++) {
        print_stack_element(stack, count);
    }
    assert(count <= stack->top);
    PDEBUG("CILIR: ----------------  End parameters\n");
    PDEBUG("CILIR: ----------------  Start locals\n");
    for (; count < locals + parameters; count++) {
        print_stack_element(stack, count);
    }
    assert(count <= stack->top);
    PDEBUG("CILIR: ----------------  End locals\n");
    PDEBUG("CILIR: ----------------  Start dynamic stack\n");
    for (; count < stack->top; count++) {
        print_stack_element(stack, count);
    }
    PDEBUG("CILIR: ----------------  End dynamic stack\n");
}

static inline void print_stack_element (t_stack *stack, JITUINT32 elementID) {
    char buf[1024];

    /* Assertions		*/
    assert(stack != NULL);

    switch ((stack->stack[elementID]).type) {
        case IROFFSET:
            PDEBUG("CILIR:          <%d> Variable %lld		", elementID
                   , (stack->stack[elementID]).value.v);
            break;
        case IRINT32:
            PDEBUG("CILIR:          <%d> INT32 Constant %lld	", elementID
                   , (stack->stack[elementID]).value.v);
            break;
        case IRINT16:
            PDEBUG("CILIR:          <%d> INT16 Constant %lld	", elementID
                   , (stack->stack[elementID]).value.v);
            break;
        case IRINT64:
            PDEBUG("CILIR:          <%d> INT64 Constant %lld	", elementID
                   , (stack->stack[elementID]).value.v);
            break;
        case IRNINT:
            PDEBUG("CILIR:          <%d> NINT Constant %lld	", elementID
                   , (stack->stack[elementID]).value.v);
            break;
        case IRUINT32:
            PDEBUG("CILIR:          <%d> UINT32 Constant %lld	", elementID
                   , (stack->stack[elementID]).value.v);
            break;
        case IRUINT64:
            PDEBUG("CILIR:          <%d> UINT64 Constant %lld	", elementID
                   , (stack->stack[elementID]).value.v);
            break;
        case IRNUINT:
            PDEBUG("CILIR:          <%d> NUINT Constant %lld	", elementID
                   , (stack->stack[elementID]).value.v);
            break;
        case IRMPOINTER:
            PDEBUG("CILIR:          <%d> MPOINTER 0x%llX		", elementID
                   , (stack->stack[elementID]).value.v);
            break;
        case IRTPOINTER:
            PDEBUG("CILIR:          <%d> TPOINTER 0x%llX		", elementID
                   , (stack->stack[elementID]).value.v);
            break;
        case IROBJECT:
            PDEBUG("CILIR:          <%d> OBJECT 0x%llX		", elementID
                   , (stack->stack[elementID]).value.v);
            break;
        case IRFLOAT32:
            PDEBUG("CILIR:          <%d> FLOAT32 Constant %f	", elementID
                   , (JITFLOAT32) (stack->stack[elementID]).value.f);
            break;
        case IRFLOAT64:
            PDEBUG("CILIR:          <%d> FLOAT64 Constant %f	", elementID
                   , (JITFLOAT32) (stack->stack[elementID]).value.f);
            break;
        case NOPARAM:
            PDEBUG("CILIR:          <%d>				", elementID);
            break;
        case IRCLASSID:
            PDEBUG("CILIR:          <%d> CLASSID 0x%p		", elementID
                   , (void *) (JITNUINT) (stack->stack[elementID]).value.v);
            break;
        case IRMETHODID:
            PDEBUG("CILIR:          <%d> METHODID 0x%p		", elementID, (void *) (JITNUINT) (stack->stack[elementID]).value.v);
            break;
        case IRVALUETYPE:
            assert((stack->stack[elementID]).type_infos != NULL);
            PDEBUG("CILIR:          <%d> VALUETYPE		", elementID);
            break;
        default:
            snprintf(buf, sizeof(buf), "CILIR: Type %d of the stack element is not known. ", ((stack->stack[elementID]).type));
            print_err(buf, 0);
            abort();
    }

    fprintf(stderr, "[ ");
    IRMETHOD_dumpIRType((stack->stack[elementID]).internal_type, stderr);
    fprintf(stderr, "]\n");
}

static inline JITINT16 decode_locals_signature (CLIManager_t *cliManager, JITUINT32 local_var_sig_tok, GenericContainer *container, t_binary_information *binary, t_stack *stack, Method method) {
    JITUINT32 	row;
    JITUINT32 	count;
    JITUINT32 	numParams;
    XanList 	*locals;

    /* Assertions.
     */
    assert(local_var_sig_tok != 0x0);
    assert(method != NULL);
    assert(stack != NULL);
    assert(binary != NULL);
    PDEBUG("CILIR: decode_locals_signature: Start\n");
    PDEBUG("CILIR: decode_locals_signature:         Decode the LocalVarSig\n");

    /* Fetch the number of local variables.
     */
    numParams = method->getParametersNumber(method);

    /* Fetch the row.
     */
    row = (local_var_sig_tok & 0x00FFFFFF);
    assert(row > 0);

    /* Fetch the local variables.
     */
    locals 	= cliManager->metadataManager->decodeMethodLocals(cliManager->metadataManager, binary, row, container);

    /* Make space on the stack for local variables.
     */
    stack->top = numParams + xanList_length(locals);
    stack->adjustSize(stack);

    /* Decode each local variable.
     */
    PDEBUG("CILIR: decode_locals_signature:		Translate each local from CIL to IR type\n");
    count = 0;
    XanListItem *item = xanList_first(locals);
    while (item != NULL) {
        LocalDescriptor *current_local;
        JITUINT32 	local_IRType;

        /* Fetch the local.
         */
        current_local 		= (LocalDescriptor *) item->data;
        assert(current_local != NULL);
        local_IRType 		= current_local->getIRType(current_local);

        /* Insert the current local internal type into the local list.
         */
        ir_local_t *local = method->insertLocal(method, local_IRType, current_local->type);
        (stack->stack[count + numParams]).type_infos = current_local->type;

        if (local_IRType == IRVALUETYPE) {
            (cliManager->layoutManager).layoutType(&(cliManager->layoutManager), current_local->type);
        }
        (stack->stack[count + numParams]).value.v = local->var_number;
        (stack->stack[count + numParams]).type = IROFFSET;
        (stack->stack[count + numParams]).internal_type = local_IRType;

        /* Fetch the next element.
         */
        item = item->next;
        count++;
    }
    assert(xanList_length(locals) == method->getLocalsNumber(method));

    /* Free the memory.
     */
    xanList_destroyListAndData(locals);

    /* Check the local variables.
     */
#ifdef DEBUG
    for (count = 0; count < method->getLocalsNumber(method); count++) {
        assert((stack->stack[count + numParams]).type == IROFFSET);
        assert((stack->stack[count + numParams]).value.v == (count + numParams));
    }
#endif

    PDEBUG("CILIR: decode_locals_signature: Exit\n");
    return 0;
}

static inline ir_instruction_t * insert_label_before (Method method, ir_instruction_t *inst, JITUINT32 current_label_ID, JITUINT32 bytes_offset, JITBOOLEAN *label_already_assigned) {
    ir_instruction_t        *instruction;
    ir_instruction_t        *instr_before;
    ir_method_t             *irMethod;

    /* Assertions		*/
    assert(method != NULL);
    assert(inst != NULL);
    assert(label_already_assigned != NULL);

    /* Initialize the variables			*/
    instruction = NULL;
    instr_before = NULL;
    (*label_already_assigned) = 0;

    /* Fetch the IR method				*/
    irMethod = method->getIRMethod(method);
    assert(irMethod != NULL);

    /* Fetch the instruction before the one given	*
     * as input.					*/
    IRMETHOD_lock(irMethod);
    instr_before = IRMETHOD_getPrevInstruction(irMethod, inst);
    IRMETHOD_unlock(irMethod);

    /* Test if the previous instruction is a label  */
    if (    (instr_before != NULL)          &&
            (instr_before->type == IRLABEL) ) {
        (*label_already_assigned) = 1;
        return instr_before;
    }

    /* we have to insert a new label before         */
    instruction = method->newIRInstrBefore(method, inst);
    instruction->type = IRLABEL;
    (instruction->param_1).value.v = current_label_ID;
    (instruction->param_1).type = IRLABELITEM;
    (instruction->param_1).internal_type = IRLABELITEM;
    instruction->byte_offset = bytes_offset;

    /* Return					*/
    return instruction;
}

static inline JITINT16 create_call_IO (CLIManager_t *cliManager, ir_instruction_t *inst, Method ilMethod, t_stack *stack, JITBOOLEAN created, Method creatorMethod, ir_signature_t *signature_of_the_jump_method, JITUINT32 bytes_read, CILStack cilStack, JITUINT32 *current_label_ID, XanList *labels) {
    JITUINT32 		type;
    JITUINT32 		param_size;
    JITUINT32 		count;
    ir_item_t               *param;
    JITUINT32 		stack_position_of_the_created_object;
    JITUINT32 		current_param_index;
    JITUINT32 		object_position;
    ir_signature_t          *formal_signature;
    ir_signature_t          *sign;
    MethodDescriptor        *method;
    ILLayout_manager        *layoutManager;
    ir_method_t 		*creatorIRMethod;
    TypeDescriptor          *created_object_type;
    JITBOOLEAN 		isInternalCall;
    JITUINT32 		stackIndex;
    JITUINT32 		destVar;
    ir_instruction_t	*extraInst;
    ir_instruction_t	*extraInst2;
    JITBOOLEAN 		isVarargCall;
    JITNINT 		num_params_to_generate;
    TypeDescriptor          *arrayType;

    /* Assertions				*/
    assert(cliManager != NULL);
    assert(inst != NULL);
    assert(stack != NULL);
    assert(ilMethod != NULL);
    PDEBUG("CILIR: create_call_IO: Start\n");

    /* Init the variables			*/
    count = 0;
    current_param_index = 0;
    inst->callParameters = NULL;
    sign = NULL;
    isVarargCall = JITFALSE;

    /* Get layout manager */
    layoutManager = &(cliManager->layoutManager);

    /* Fetch the IR Method	*/
    creatorIRMethod = creatorMethod->getIRMethod(creatorMethod);

    /* Allocate the call parameters list	*/
    if (inst->callParameters != NULL) {
        xanList_deleteAndFreeElements(inst->callParameters);
    }
    inst->callParameters = xanList_new(sharedAllocFunction, freeFunction, NULL);
    assert(inst->callParameters != NULL);

    /* Get the signatures	*/
    formal_signature = ilMethod->getSignature(ilMethod);
    assert(formal_signature != NULL);

    if (signature_of_the_jump_method == NULL) {

        /* No actual signature. Use the formal signature	*/
        sign = formal_signature;
    } else {

        /* We have an actual signature. Let's use it */
        sign = signature_of_the_jump_method;
    }
    assert(sign != NULL);
    assert(sign->resultType == formal_signature->resultType);

    if (inst->type != IRNATIVECALL) {
        num_params_to_generate = formal_signature->parametersNumber;
    } else {
        num_params_to_generate = sign->parametersNumber;
    }

    /* Fetch the metadata of the method	*/
    method = ilMethod->getID(ilMethod);
    assert(method != NULL);
    assert(method->name != NULL);
    param_size = sign->parametersNumber;
    stack_position_of_the_created_object = -1;
    created_object_type = NULL;
    PDEBUG("CILIR: create_call_IO:	Parameters number	= %d\n", param_size);

    /* verify if the object is the first element of the stack */
    if (created == 1) {
        PDEBUG("CILIR: create_call_IO:	created flag asserted\n");
        object_position = stack->top - 1;
        stack_position_of_the_created_object = (stack->stack[object_position]).value.v;
        created_object_type = (stack->stack[object_position]).type_infos;
    } else {
        PDEBUG("CILIR: create_call_IO:	created flag NOT asserted\n");
        object_position = stack->top - param_size;
    }

    /* Check if we are creating the I/O for an internal call */
    if (method->attributes->is_internal_call) {
        isInternalCall = JITTRUE;
    } else {
        isInternalCall = JITFALSE;
    }

    /* Create the parameters		*/
#ifdef PRINTDEBUG
    if (num_params_to_generate > 0) {
        PDEBUG("CILIR: create_call_IO:	Create the %ld parameters\n", num_params_to_generate);
    } else {
        PDEBUG("CILIR: create_call_IO:	No parameters\n");
    }

#endif
    if (method->attributes->is_vararg) {
        assert(formal_signature != sign);
        isVarargCall = JITTRUE;
        if (inst->type == IRNATIVECALL) {
            IRMETHOD_setInstructionAsVarArg(creatorIRMethod, inst, JITTRUE);
        }

        /* Generate formal parameters and an array to hold vararg ones	*/
    } else {
        isVarargCall = JITFALSE;

        /* Only parameters of the formal signature exist	*/
    }

    /* Convert all the parameters to the type expected by the signature */
    if (param_size > 0) {
        JITINT32 i;
        ir_item_t *auxStack;
        auxStack = allocMemory(sizeof(ir_item_t) * param_size);
        assert(auxStack != NULL);
        /* Pop all the parameters */
        i = current_param_index;
        while (i < param_size) {
            stack->pop(stack, &auxStack[i]);
            i++;
        }
        i--;

        /* Place this into the right position */
        if (created) {
            ir_item_t thisElement;
            unsigned int j;

            thisElement = auxStack[0];
            for (j = 1; j < param_size; j++) {
                auxStack[j - 1] = auxStack[j];
            }
            auxStack[param_size - 1] = thisElement;
        }

        /* Push the parameters back on the stack and convert them */
        while (i >= (JITINT32) current_param_index) {
            stack->push(stack, &auxStack[i]);
            JITINT32 toType = sign->parameterTypes[(param_size - 1) - i + current_param_index];   //Remember that parameters are on the auxStack in reverse order
            if (     (stack->stack[stack->top - 1].internal_type != toType)    &&
                     (toType != IRVALUETYPE)                                 ) {
                _perform_conversion(cliManager, creatorMethod, bytes_read, stack, toType, JITFALSE, inst);
            }
            i--;
        }
        freeMemory(auxStack);
    }

    while (current_param_index < num_params_to_generate) {
        if (	(isVarargCall)				 		&&
                (inst->type != IRNATIVECALL)				&&
                (current_param_index == num_params_to_generate - 1)	) {

            /* In case of a vararg call the vararg parameters are have to be passed inside
               an array, that will be the only parameter after the last one defined by the
               formal signature.
               This array will be an array of TypedReference.
               Each TypedReference contains a RuntimeTypeHandle (enclosing the CLRType of the parameter)
               and a pointer to the (eventually boxed) actual value of the parameter */

            /* Generate the array to hold the vararg parameters after the last formal param */
            JITINT32 num_vararg_params;
            JITINT32 vararg_param_index;
            JITINT32 paramArray;
            ir_item_t       *arrayAllocated;

            num_vararg_params = sign->parametersNumber - (formal_signature->parametersNumber - 1);

            arrayType = (cliManager->CLR).typedReferenceManager.fillILTypedRefType();

            extraInst = creatorMethod->newIRInstrBefore(creatorMethod, inst);

            extraInst->type = IRNEWARR;
            /* Type of the array */
            (extraInst->param_1).value.v = (JITNUINT) arrayType;
            (extraInst->param_1).type = IRCLASSID;
            (extraInst->param_1).internal_type = IRCLASSID;
            /* Size of the array */
            (extraInst->param_2).value.v = num_vararg_params;
            (extraInst->param_2).type = IRINT32;
            (extraInst->param_2).internal_type = IRINT32;
            /* Return value */
            paramArray = creatorMethod->incMaxVariablesReturnOld(creatorMethod);
            (extraInst->result).value.v = paramArray;
            (extraInst->result).type = IROFFSET;
            (extraInst->result).internal_type = IROBJECT;
            (extraInst->result).type_infos = arrayType;
            arrayAllocated = &(extraInst->result);

            vararg_param_index = current_param_index;
            while (vararg_param_index < param_size) {
                ir_item_t dereferencedTypedRef;
                TypeDescriptor *rthType;
                TypeDescriptor *elementType;
                ir_symbol_t *clrType;
                JITBOOLEAN needsBoxing = JITFALSE;
                ir_item_t *value_on_the_stack = NULL;
                ir_item_t *typedRefAddr = NULL;
                ir_item_t *valueAddr = NULL;

                assert((stack->top - 1 - (param_size - vararg_param_index)) >= 0);

                /* Prepare the TypedReference (and leave it on the stack) */
                make_irnewValueType(cliManager, creatorMethod, bytes_read, arrayType, stack, inst);

                /* Define the ILType of the current element to store in the array */
                elementType = sign->ilParamsTypes[vararg_param_index];
                if (elementType == NULL) {
                    JITINT32 irType;

                    irType = sign->parameterTypes[vararg_param_index];

                    /* We have no complete information about the type. Let's obtain them out from the IRType */

                    elementType = cliManager->metadataManager->getTypeDescriptorFromIRType(cliManager->metadataManager, irType);

                }
                /*  We need to box it into an object
                 * (unless it already is an object)
                 */
                if (elementType->getIRType(elementType) != IROBJECT) {
                    needsBoxing = JITTRUE;
                }

                /* The value we want to store */
                value_on_the_stack = &(stack->stack[(stack->top) - 1 - (param_size - vararg_param_index)]);
                if (needsBoxing) {
                    /* create the new object (and leave it on the top of the stack): */
                    make_irnewobj(cliManager, creatorMethod, cilStack, bytes_read, elementType, 0, stack, current_label_ID, labels, inst);

                    if (value_on_the_stack->type == IROFFSET) {
                        ILLayout                *layoutInfos;

                        /* load the address of the valueType stored into value_on_the_stack */
                        extraInst = creatorMethod->newIRInstrBefore(creatorMethod, inst);
                        extraInst->type = IRGETADDRESS;
                        memcpy(&(extraInst->param_1), value_on_the_stack, sizeof(ir_item_t));
                        assert((extraInst->param_1).internal_type != NOPARAM);

                        /* Clean the top of the stack           */
                        stack->cleanTop(stack);

                        stack->stack[stack->top].value.v = creatorMethod->incMaxVariablesReturnOld(creatorMethod);
                        stack->stack[stack->top].type = IROFFSET;
                        stack->stack[stack->top].internal_type = IRMPOINTER;
                        if (value_on_the_stack->type_infos != NULL) {
                            stack->stack[stack->top].type_infos = value_on_the_stack->type_infos->makePointerDescriptor(value_on_the_stack->type_infos);
                        } else {
                            stack->stack[stack->top].type_infos = NULL;
                        }
                        memcpy(&(extraInst->result), &(stack->stack[stack->top]), sizeof(ir_item_t));
                        (stack->top)++;
                        extraInst->byte_offset = bytes_read;
                        PDEBUG("CILIR:			Insert a IRGETADDRESS instruction\n");

#ifdef PRINTDEBUG
                        PDEBUG("CILIR: cil_box: Print the stack after the IRGETADDRESS \n");
                        print_stack(stack, creatorMethod->getParametersNumber(creatorMethod), creatorMethod->getLocalsNumber(creatorMethod));
#endif

                        /* Copy the created object on top of stack */
                        memcpy(&(stack->stack[stack->top]), &(stack->stack[stack->top - 2]), sizeof(ir_item_t));
                        (stack->top)++;

                        /* Copy the just calculated address on top of the stack */
                        memcpy(&(stack->stack[stack->top]), &(stack->stack[stack->top - 2]), sizeof(ir_item_t));
                        (stack->top)++;

                        /* retrieve the layout informations of the value on the stack */
                        assert(layoutManager->layoutType != NULL);
                        assert(elementType != NULL);

                        layoutInfos = layoutManager->layoutType(layoutManager, elementType);
                        assert(layoutInfos != NULL);
                        /* create an IRMEMCPY instruction (copy from the address of the ValueType to the object)*/
                        make_irmemcpy(creatorMethod, cilStack, bytes_read, stack, current_label_ID, labels, layoutInfos->typeSize, UNALIGNED_DONOTCHECK_ALIGNMENT, inst);

#ifdef PRINTDEBUG
                        PDEBUG("CILIR: cil_box: Print the stack after the IRMEMCPY -- still OBJECT & MPOINTER \n");
                        print_stack(stack, creatorMethod->getParametersNumber(creatorMethod), creatorMethod->getLocalsNumber(creatorMethod));
#endif

                        /* pop the computed address from the stack (thus leaving the object on top)*/
                        (stack->top)--;

#ifdef PRINTDEBUG
                        PDEBUG("CILIR: cil_box: Print the stack after the IRMEMCPY --- ONLY OBJECT \n");
                        print_stack(stack, creatorMethod->getParametersNumber(creatorMethod), creatorMethod->getLocalsNumber(creatorMethod));
#endif

                    } else {
                        assert(value_on_the_stack->internal_type != IRVALUETYPE);

                        /* Make the IR instruction		*/
                        extraInst = creatorMethod->newIRInstrBefore(creatorMethod, inst);
                        assert(extraInst != NULL);
                        extraInst->type = IRSTOREREL;
                        /* The base address of the object: */
                        memcpy(&(extraInst->param_1), &(stack->stack[stack->top - 1]), sizeof(ir_item_t));
                        assert((extraInst->param_1).internal_type == IROBJECT);
                        /* The offset (=0, we want to store at the beginning of the object): */
                        (extraInst->param_2).value.v = 0;
                        (extraInst->param_2).type = IRINT32;
                        (extraInst->param_2).internal_type = IRINT32;
                        (extraInst->param_2).type_infos = NULL;
                        /* The value to store: */
                        memcpy(&(extraInst->param_3), value_on_the_stack, sizeof(ir_item_t));
                    }
                }

                /* Define the addresses of the TypedReference and of the value to store */
                if (needsBoxing) {
                    typedRefAddr = &(stack->stack[stack->top - 2]);
                    valueAddr = &(stack->stack[(stack->top) - 1]);
                } else {
                    typedRefAddr = &(stack->stack[(stack->top) - 1]);
                    valueAddr = value_on_the_stack;
                }
                assert(typedRefAddr != NULL);
                assert(valueAddr != NULL);

                /* Set the "value" field of the TypedReference  */
                extraInst = creatorMethod->newIRInstrBefore(creatorMethod, inst);
                extraInst->type = IRSTOREREL;
                /* Base address of the TypedReference:          */
                memcpy(&(extraInst->param_1), typedRefAddr, sizeof(ir_item_t));
                /* Offset of the "value" field:                 */
                (extraInst->param_2).value.v = (JITINT32) ((cliManager->CLR).typedReferenceManager.getValueOffset(&(((cliManager->CLR).typedReferenceManager))));
                (extraInst->param_2).type = IRINT32;
                (extraInst->param_2).internal_type = IRINT32;
                /* Value to store:                              */
                memcpy(&(extraInst->param_3), valueAddr, sizeof(ir_item_t));

                if (needsBoxing) {
                    /*
                     * Remove the Object we created, that contains the value from the stack.
                     * It is not needed anymore
                     */
                    (stack->top)--;
                }

                /* Prepare the RuntimeTypeHandle value type (and leave it on the stack) */
                rthType = (cliManager->CLR).runtimeHandleManager.fillILRuntimeHandle(&((cliManager->CLR).runtimeHandleManager), SYSTEM_RUNTIMETYPEHANDLE);
                make_irnewValueType(cliManager, creatorMethod, bytes_read, rthType, stack, inst);

                /* Set the "value" field of the RuntimeTypeHandle */
                extraInst = creatorMethod->newIRInstrBefore(creatorMethod, inst);
                extraInst->type = IRSTOREREL;
                /* Base address of the RuntimeTypeHandle: */
                memcpy(&(extraInst->param_1), &(stack->stack[stack->top - 1]), sizeof(ir_item_t));
                /* Offset of the "value" field: */
                (extraInst->param_2).value.v = (JITINT32) (cliManager->CLR).runtimeHandleManager.getRuntimeTypeHandleValueOffset(&((cliManager->CLR).runtimeHandleManager));
                (extraInst->param_2).type = IRINT32;
                (extraInst->param_2).internal_type = IRINT32;

                /* Find and store in the RuntimeTypeHandle the CLRType corresponding to the given ILType */
                clrType = (cliManager->CLR).reflectManager.getCLRTypeSymbol(elementType);
                (extraInst->param_3).value.v = (JITNUINT) clrType;
                (extraInst->param_3).type = IRSYMBOL;
                (extraInst->param_3).internal_type = IRNINT;

                /* Store the RuntimeTypeHandle in the "type" field of the TypedReference */
                extraInst = creatorMethod->newIRInstrBefore(creatorMethod, inst);
                extraInst->type = IRSTOREREL;
                /* Base address of the TypedReference: */
                memcpy(&(extraInst->param_1), &(stack->stack[stack->top - 2]), sizeof(ir_item_t));
                /* Offset of the "type" field: */
                (extraInst->param_2).value.v = (JITINT32) (cliManager->CLR).typedReferenceManager.getTypeOffset(&(((cliManager->CLR).typedReferenceManager)));
                (extraInst->param_2).type = IRINT32;
                (extraInst->param_2).internal_type = IRINT32;
                /* Value to store: */
                memcpy(&(extraInst->param_3), &(stack->stack[stack->top - 1]), sizeof(ir_item_t));

                /* Remove the RuntimeTypeHandle from the stack */
                (stack->top)--;

                /* Dereference the TypedReference: */
                extraInst = creatorMethod->newIRInstrBefore(creatorMethod, inst);
                extraInst->type = IRLOADREL;
                /* Address to dereference: */
                memcpy(&(extraInst->param_1), &(stack->stack[(stack->top) - 1]), sizeof(ir_item_t));
                /* Offset (=0, we want to dereference exactly param_1): */
                (extraInst->param_2).value.v = 0;
                (extraInst->param_2).type = IRINT32;
                (extraInst->param_2).internal_type = IRINT32;

                /* Prepare the return value: */
                memset(&dereferencedTypedRef, 0, sizeof(ir_item_t));
                dereferencedTypedRef.value.v = creatorMethod->incMaxVariablesReturnOld(creatorMethod);
                dereferencedTypedRef.type = IROFFSET;
                dereferencedTypedRef.internal_type = IRVALUETYPE;
                dereferencedTypedRef.type_infos = arrayType;

                /*set the return value: */
                memcpy(&(extraInst->result), &dereferencedTypedRef, sizeof(ir_item_t));

                /* Store the newly-created TypedReference into the array */
                extraInst = creatorMethod->newIRInstrBefore(creatorMethod, inst);
                extraInst->type = IRSTOREREL;

                /* The element to put into the array	*/
                memcpy(&(extraInst->param_3), &dereferencedTypedRef, sizeof(ir_item_t));
#ifdef DEBUG
                if ((extraInst->param_3).internal_type == IRVALUETYPE) {
                    assert((extraInst->param_3).type_infos != NULL);
                }
#endif

                /* Set the offset of the store.
                 */
                (extraInst->param_2).value.v = (vararg_param_index - current_param_index) * IRDATA_getSizeOfType(&dereferencedTypedRef);
                (extraInst->param_2).type = IRINT32;
                (extraInst->param_2).internal_type = IRINT32;

                /* Set the array pointer	*/
                memcpy(&(extraInst->param_1), arrayAllocated, sizeof(ir_item_t));

                /* Remove the TypedReference from the CIL stack */
                (stack->top)--;

                /* Step to the next param */
                vararg_param_index++;
            }

            /* Create the parameter		*/
            param = make_new_IR_param();
            memcpy(param, arrayAllocated, sizeof(ir_item_t));
            extraInst2 = IRMETHOD_newInstructionAfter(creatorIRMethod, inst);
            extraInst2->type = IRFREEOBJ;
            IRMETHOD_cpInstructionParameter1(extraInst2, arrayAllocated);
        } else {

            /* Fetch the type		*/
            type = sign->parameterTypes[current_param_index];
            assert(type != NOPARAM);

            /* Create the parameter		*/
            param = make_new_IR_param();

            /* Check if we have a special case to handle	*/
            if (isInternalCall && (type == IRVALUETYPE)) {

                /*
                 * We are calling an internal call an it needs a value type as
                 * parameter: we must do a parameter conversion
                 */
                PDEBUG("CILIR: create_call_IO:	Internal call with a VALUETYPE parameter\n");

                /* We a variable to hold value type address     */
                destVar = creatorMethod->incMaxVariablesReturnOld(creatorMethod);

                /* Get the value type to "convert"              */
                stackIndex = stack->top - (param_size - count);

                /*
                 * Generate an instruction to get the value type address
                 * and put it in the new variable
                 */
                extraInst = creatorMethod->newIRInstrBefore(creatorMethod, inst);
                extraInst->type = IRGETADDRESS;
                memcpy(&(extraInst->param_1), &(stack->stack[stackIndex]), sizeof(ir_item_t));
                extraInst->result.value.v = destVar;
                extraInst->result.type = IROFFSET;
                extraInst->result.internal_type = IRMPOINTER;
                assert((extraInst->param_1).internal_type != NOPARAM);

                /*
                 * The native call parameter is the address of the value
                 * type, not the value type itself
                 */
                memcpy(param, &(extraInst->result), sizeof(ir_item_t));

            } else {

                /* Normal call, copy parameter from the stack */
                PDEBUG("CILIR: create_call_IO:	Normal call, copy parameter from the stack\n");
                memcpy(param, &(stack->stack[(stack->top) - (param_size - count)]), sizeof(ir_item_t));
            }
        }

        /* Insert the parameter inside the list	*/
        assert(inst->callParameters != NULL);
        xanList_append(inst->callParameters, param);
        count++;

        /* Retrieve the next parameter index	*/
        current_param_index++;
    }

    /* Deallocates the parameters		*/
    if (param_size > 0) {
        PDEBUG("CILIR: create_call_IO:	Deallocate %d parameters from the stack\n", param_size);
        (stack->top) = (stack->top) - param_size;
    }

    /* Allocate the result			*/
    switch (sign->resultType) {
        case IRVOID:
            PDEBUG("CILIR: create_call_IO:	Result is IRVOID\n");
            (inst->result).type = IRVOID;
            (inst->result).internal_type = (inst->result).type;
            break;
        default:
            PDEBUG("CILIR: create_call_IO:	Result is %s\n", IRMETHOD_getIRTypeName(sign->resultType));

            /* Clean the top of the stack           */
            stack->cleanTop(stack);

            (stack->stack[stack->top]).value.v = creatorMethod->incMaxVariablesReturnOld(creatorMethod);
            (stack->stack[stack->top]).type = IROFFSET;
            (stack->stack[stack->top]).internal_type = sign->resultType;
            (stack->stack[stack->top]).type_infos = method->getResult(method)->type;

            /*
             * The callee method is an internal call and return a
             * value type, we must insert an extra instruction to
             * copy the value type from the heap to the current
             * method stack
             */
            if (isInternalCall && (sign->resultType == IRVALUETYPE)) {
                ir_item_t       *paramPointer;

                /* Get the value type size                      */
                layoutManager->layoutType(layoutManager, (stack->stack[stack->top]).type_infos);

                /* Make a new variable to hold the call return	*
                 * value and complete call instruction          *
                 * generation					*/
                param = make_new_IR_param();
                paramPointer = make_new_IR_param();
                memcpy(param, &(stack->stack[stack->top]), sizeof(ir_item_t));
                (paramPointer->value).v = creatorMethod->incMaxVariablesReturnOld(creatorMethod);
                paramPointer->type = IROFFSET;
                paramPointer->internal_type = IRMPOINTER;
                if (param->type_infos != NULL) {
                    paramPointer->type_infos = param->type_infos->makePointerDescriptor(param->type_infos);
                }

                /* Set the result of the call as void		*/
                (inst->result).type = IRVOID;
                (inst->result).internal_type = (inst->result).type;

                /* Generate an instruction to get its address		*
                 * and a variable to hold it			        */
                extraInst = creatorMethod->newIRInstrBefore(creatorMethod, inst);
                extraInst->type = IRGETADDRESS;
                memcpy(&(extraInst->param_1), param, sizeof(ir_item_t));
                memcpy(&(extraInst->result), paramPointer, sizeof(ir_item_t));
                assert((extraInst->param_1).internal_type != NOPARAM);

                /* Add the value type allocated within the stack	*
                 * by the caller					*/
                xanList_append(inst->callParameters, paramPointer);

                /* Free the memory					*/
                freeFunction(param);

            } else {

                /* Normal call                                          */
                memcpy(&(inst->result), &(stack->stack[stack->top]), sizeof(ir_item_t));
                assert(((inst->result).type == IROFFSET) || ((inst->result).type == (inst->result).internal_type));
            }
            (stack->top)++;

            /* Convert the result to the appropriate stackable type */
            JITINT32 toType;
            toType = (cliManager->metadataManager->generic_type_rules)->coerce_CLI_Type(stack->stack[stack->top - 1].internal_type);
            if (stack->stack[stack->top - 1].internal_type != toType) {
                _perform_conversion(cliManager, creatorMethod, bytes_read, stack, toType, JITFALSE, NULL);
            }
    }
    assert(((inst->result).type == IROFFSET) || ((inst->result).type == (inst->result).internal_type));

    /* Check if we have to restore the object on	*
     * the top of the stack				*/
    if (created) {

        /* Clean the top of the stack           */
        stack->cleanTop(stack);

        /* We shall restore the object created on the top of the stack */
        PDEBUG("CILIR: create_call_IO:	Restore the object on top of the stack\n");
        assert(stack_position_of_the_created_object != -1);
        (stack->stack[stack->top]).value.v = stack_position_of_the_created_object;
        (stack->stack[stack->top]).type = IROFFSET;
        (stack->stack[stack->top]).internal_type = IROBJECT;
        (stack->stack[stack->top]).type_infos = created_object_type;
        (stack->top)++;
    }

    PDEBUG("CILIR: create_call_IO: Exit\n");
    return 0;
}

static inline JITINT16 add_label_inst (Method method, CILStack cilStack, XanList *labels, JITINT32 bytes_offset, JITUINT32 inst_size, JITUINT32 bytes_read, JITUINT32 *current_label_ID, ir_item_t *param, t_stack *stack) {
    JITUINT32 insert_point;
    t_label                 *label;
    ir_instruction_t        *inst;
    ir_instruction_t        *new_inst;

    /* Assertions		*/
    assert(method != NULL);
    assert(cilStack != NULL);
    assert(labels != NULL);
    assert(current_label_ID != NULL);
    assert(param != NULL);
    assert(stack != NULL);

    /* Add the label	*/
    PDEBUG("CILIR: add_label_inst: Start\n");
    PDEBUG("CILIR: add_label_inst:		Bytes offset of the Jump        = %d bytes\n", bytes_offset);
    PDEBUG("CILIR: add_label_inst:		Instruction size                = %d bytes\n", inst_size);
    PDEBUG("CILIR: add_label_inst:		Current bytes position          = %d bytes\n", bytes_read);
    if (bytes_offset > 0) {
        PDEBUG("CILIR: add_label_inst:			Jump forward\n");
        label = get_label_by_bytes_offset(labels, bytes_read + bytes_offset);
        if (label == NULL) {
            PDEBUG("CILIR: add_label_inst:				Label not exist\n");
            label = (t_label *) allocMemory(sizeof(t_label));
            label->ID = (*current_label_ID);
            label->counter = bytes_offset + inst_size;                      // Because the instruction
            // has size equal to inst_size bytes
            label->ir_position = -1;
            label->byte_offset = bytes_read + bytes_offset;
            label->type = DEFAULT_LABEL_TYPE;
            label->starting_blocks = xanList_new(allocFunction, freeFunction, NULL);
            label->starting_handler = NULL;
            label->ending_block = NULL;
            label->ending_handler = NULL;
            cil_insert_label(cilStack, labels, label, stack, -1);
            (*current_label_ID)++;
            assert(IRMETHOD_getLabelInstruction(method->getIRMethod(method), label->ID) == NULL);
            PDEBUG("CILIR: add_label_inst:				A new label %d is inserted\n", label->ID);
        } else {
            PDEBUG("CILIR: add_label_inst:				Label exist\n");
        }
        (param->value).v = label->ID;
        param->type = IRLABELITEM;
        param->internal_type = IRLABELITEM;
    } else {
        PDEBUG("CILIR: add_label_inst:			Jump backward\n");
        insert_point = bytes_read + bytes_offset;
        PDEBUG("CILIR: add_label_inst:				The insert point is after the %d bytes offset\n", insert_point);
        insert_point++;
        inst = find_insert_point(method, insert_point);
        assert(inst != NULL);
        PDEBUG("CILIR: add_label_inst:				The insert point is in the instruction with Bytes offset = %d\n", inst->byte_offset);
        if (inst->type == IRLABEL) {
            PDEBUG("CILIR: add_label_inst:					The insert point is a label\n");
            (param->value).v = (inst->param_1).value.v;
            param->type = (inst->param_1).type;
            param->internal_type = (inst->param_1).internal_type;
            assert(param->internal_type == IRLABELITEM);
            assert(param->type == IRLABELITEM);
        } else {
            JITBOOLEAN label_already_assigned;
            PDEBUG("CILIR: add_label_inst:					The insert point isn't a label\n");
            new_inst = insert_label_before(method, inst, *current_label_ID, bytes_read + bytes_offset, &label_already_assigned);

            /* test if we inserted a new label or the label was already inserted */
            /* `label_already_assigned` is set, then the label was already inserted */
            if (!label_already_assigned) {
                (*current_label_ID)++;
            }
            (param->value).v = (new_inst->param_1).value.v;
            param->type = (new_inst->param_1).type;
            param->internal_type = (new_inst->param_1).internal_type;
            assert(param->internal_type == IRLABELITEM);
            assert(param->type == IRLABELITEM);
        }
    }

    /* Return			*/
    return 0;
}

static inline JITINT16 decode_exceptions (CLIManager_t *cliManager, CILStack cilStack, GenericContainer *container, t_binary_information *binary, JITUINT32 body_size, XanList *labels, JITUINT32 *current_label_ID, Method method, t_stack *stack) {
    JITUINT32 bytes_read;
    JITUINT32 count;
    JITUINT32 data_size;
    JITINT64 offset;
    JITUINT32 pad;
    JITBOOLEAN is_fat;
    JITINT8 buffer[DIM_BUF];

    /* Assertions		*/
    assert(binary           != NULL);
    assert(cilStack         != NULL);
    assert(body_size        != 0);
    assert(stack            != NULL);

    /* Init the variables	*/
    bytes_read = 0;
    is_fat = 0;

    /* Check the current position		*/
    offset  = ftell((*(binary->binary.reader))->stream);
    pad     = (offset + body_size) % 4;

    if (pad != 0) {
        pad = (4 - pad);
    }
    PDEBUG("CILIR: DECODE_EXCEPTIONS: Body size	        = %d\n", body_size);
    PDEBUG("CILIR: DECODE_EXCEPTIONS: Current file offset   = %llX (%lld)\n", offset, offset);
    PDEBUG("CILIR: DECODE_EXCEPTIONS: Pad                   = %d\n", pad);

    /* Seek to the end of the method body	*/
    PDEBUG("CILIR: DECODE_EXCEPTIONS: Seek %d bytes forward\n", body_size + pad);
    if (fseek((*(binary->binary.reader))->stream, body_size + pad, SEEK_CUR) != 0) {
        print_err("CILIR: DECODE_EXCEPTIONS: ERROR= Fseek return an error. ", errno);
        return NO_SEEK_POSSIBLE;
    }
    bytes_read += pad;

    /* Read the first byte of the header	*/
    if (il_read(buffer, 1, &(binary->binary)) != 0) {
        return NO_READ_POSSIBLE;
    }
    bytes_read += 1;

    PDEBUG("CILIR: DECODE_EXCEPTIONS: First byte    = 0x%X\n", buffer[0]);
    if ((buffer[0] & 0x1) != 0) {
        PDEBUG("CILIR: DECODE_EXCEPTIONS:       Exception handling data\n");
    }
    if ((buffer[0] & 0x40) != 0) {
        PDEBUG("CILIR: DECODE_EXCEPTIONS:       Fat variety\n");
        is_fat = 1;
    } else {
        PDEBUG("CILIR: DECODE_EXCEPTIONS:       Tiny variety\n");
        is_fat = 0;
    }
    if ((buffer[0] & 0x80) != 0) {
        PDEBUG("CILIR: DECODE_EXCEPTIONS:       There is another section\n");
        abort();
    }

    /* Reusing the buffer, cleaning		*/
    memset(buffer, '\0', 1024);

    /* Decode the signature			*/
    if (is_fat == 1) {
        PDEBUG("CILIR: DECODE_EXCEPTIONS: Fat signature\n");
        /* Read next 3 bytes of the header	*/
        if (il_read(buffer, 3, &(binary->binary)) != 0) {
            return NO_READ_POSSIBLE;
        }
        bytes_read += 3;
        read_from_buffer(buffer, 0, 4, &data_size);
        PDEBUG("CILIR: DECODE_EXCEPTIONS:       read data size %d\n", data_size);
        assert(data_size > 0);
        assert(((data_size - 4) % 24) == 0);

    } else {

        /* Data size				*/
        PDEBUG("CILIR: DECODE_EXCEPTIONS: Not a fat signature\n");

        /* Read the first byte of the header	*/
        if (il_read(buffer, 3, &(binary->binary)) != 0) {
            return NO_READ_POSSIBLE;
        }
        bytes_read += 3;
        read_from_buffer(buffer, 0, 1, &data_size);
        PDEBUG("CILIR: DECODE_EXCEPTIONS:       read data size %d\n", data_size);
#ifdef DEBUG
        JITUINT16 test_short_word;
        read_from_buffer(buffer, 0, 2, &test_short_word);
        assert(data_size == (test_short_word & 0x00FF));
#endif
        assert(data_size > 0);
        assert(buffer[1] == 0);
        assert(buffer[2] == 0);
        assert(((data_size - 4) % 12) == 0);
    }

    /* Clauses		*/
    PDEBUG("CILIR: DECODE_EXCEPTIONS: Data size = %d\n", data_size);
    data_size -= 4;
    for (count = 0; data_size > 0; count++) {
        XanListItem             *item;
        t_try_block     *try_block;
        t_try_handler   *try_handler;
        t_label         *try_handler_start_label;
        t_label         *try_handler_end_label;
        t_label         *try_filter_start_label;
        t_label         *try_block_start_label;
        t_label         *try_block_end_label;
        JITUINT32 class_token_found;

        /*	INITIALIZATION OF THE LOCAL VARIABLES */
        item = NULL;
        try_block = NULL;
        try_handler = NULL;
        class_token_found = -1;
        PDEBUG("CILIR: DECODE_EXCEPTIONS:       %d Clause\n", count);

        /* Allocate the try_block	*/
        try_block = (t_try_block *) allocMemory(sizeof(t_try_block));
        assert(try_block != NULL);

        /* Allocate the try_handler for the try-block	*/
        try_handler = (t_try_handler *) allocMemory(sizeof(t_try_handler));
        assert(try_handler != NULL);

        /* Initialize the try-block */
        try_block->begin_offset = -1;
        try_block->length = -1;
        try_block->inner_try_blocks = NULL;
        try_block->catch_handlers = NULL;
        try_block->finally_handlers = NULL;
        try_block->stack_to_restore = NULL;
        try_block->parent_block = NULL;
        try_block->try_start_label_ID = -1;
        try_block->try_end_label_ID = -1;

        /* Initialize the try-handler */
        try_handler->type = -1;
        try_handler->handler_begin_offset = -1;
        try_handler->handler_length = -1;
        try_handler->handler_start_label_ID = -1;
        try_handler->handler_end_label_ID = -1;
        try_handler->filter_start_label_ID = -1;
        try_handler->class = NULL;
        try_handler->filter_offset = -1;
        try_handler->owner_block = try_block;
        try_handler->end_finally_successor_label_ID = -1;
        try_handler->end_filter_or_finally_inst = NULL;

        if (is_fat == 0) {

            /* Read a clause	*/
            if (il_read(buffer, 12, &(binary->binary)) != 0) {
                return NO_READ_POSSIBLE;
            }
            bytes_read += 12;

            /* Flags		*/
            PDEBUG("CILIR: DECODE_EXCEPTIONS:               Flag                    = 0x%X\n", buffer[0]);
            if (buffer[0] == 0x0) {
                PDEBUG("CILIR: DECODE_EXCEPTIONS:               Flag = Exception\n");
                try_handler->type = EXCEPTION_TYPE;
            } else if (buffer[0] == 0x1) {
                PDEBUG("CILIR: DECODE_EXCEPTIONS:               Flag = Filter and handler clause\n");
                try_handler->type = FILTER_TYPE;
            } else if (buffer[0] == 0x2) {
                PDEBUG("CILIR: DECODE_EXCEPTIONS:		Flag = Finally\n");
                try_handler->type = FINALLY_TYPE;
            } else if (buffer[0] == 0x4) {
                PDEBUG("CILIR: DECODE_EXCEPTIONS:		Flag = Fault\n");
                try_handler->type = FAULT_TYPE;
            } else {
                print_err("CILIR: DECODE_EXCEPTIONS: ERROR = Flags of the exceptions clause are not correct. ", 0);
                try_handler->type = -1;
                abort();
            }

            /* Try offset		*/
            read_from_buffer(buffer, 2, 2, &(try_block->begin_offset));
            PDEBUG("CILIR: DECODE_EXCEPTIONS:       Try bytes offset        = %d\n", try_block->begin_offset );

            /* Try length		*/
            read_from_buffer(buffer, 4, 2, &(try_block->length));
            try_block->length = try_block->length & 0x00FF;
            PDEBUG("CILIR: DECODE_EXCEPTIONS:       Try length in bytes	= %d\n", try_block->length);

            /* Handler offset	*/
            read_from_buffer(buffer, 5, 2, &(try_handler->handler_begin_offset));
            PDEBUG("CILIR: DECODE_EXCEPTIONS:       Handler bytes offset	= %d\n", try_handler->handler_begin_offset);

            /* Handler length	*/
            read_from_buffer(buffer, 7, 2, &(try_handler->handler_length));
            try_handler->handler_length = try_handler->handler_length & 0x00FF;
            PDEBUG("CILIR: DECODE_EXCEPTIONS:       Handler length in bytes	= %d\n", try_handler->handler_length);
#if DEBUG
            JITUINT8 testChar;
            read_from_buffer(buffer, 7, 1, &(testChar));
            assert(try_handler->handler_length == (JITUINT16) testChar);
#endif

            /* Class token		*/
            read_from_buffer(buffer, 8, 4, &class_token_found);
            PDEBUG("CILIR: DECODE_EXCEPTIONS:       Class Token		= 0x%X\n", class_token_found);

        } else {

            /* fat variety */
            /* Read a clause	*/
            if (il_read(buffer, 24, &(binary->binary)) != 0) {
                return NO_READ_POSSIBLE;
            }
            bytes_read += 24;

            /* Flags		*/
            JITUINT32 decode_exceptions_flag;
            read_from_buffer(buffer, 0, 4, &decode_exceptions_flag);
            PDEBUG("CILIR: DECODE_EXCEPTIONS:               "
                   "Flag                    = 0x%X\n"
                   , decode_exceptions_flag);
            if (decode_exceptions_flag == 0x0000) {
                PDEBUG("CILIR: DECODE_EXCEPTIONS:               "
                       "Flag = Exception\n");
                try_handler->type = EXCEPTION_TYPE;
            } else if (decode_exceptions_flag == 0x0001) {
                PDEBUG("CILIR: DECODE_EXCEPTIONS:               "
                       "Flag = Filter and handler clause\n");
                try_handler->type = FILTER_TYPE;
            } else if (decode_exceptions_flag == 0x0002) {
                PDEBUG("CILIR: DECODE_EXCEPTIONS:		"
                       "Flag = Finally\n");
                try_handler->type = FINALLY_TYPE;
            } else if (decode_exceptions_flag == 0x0004) {
                PDEBUG("CILIR: DECODE_EXCEPTIONS:		"
                       "Flag = Fault\n");
                try_handler->type = FAULT_TYPE;
            } else {
                print_err("CILIR: DECODE_EXCEPTIONS: ERROR = Flags of the exceptions clause are not correct. ", 0);
                try_handler->type = -1;
                abort();
            }

            /* Try offset		*/
            read_from_buffer(buffer, 4, 4, &(try_block->begin_offset));
            PDEBUG("CILIR: DECODE_EXCEPTIONS:               Try bytes offset        = %d\n",
                   try_block->begin_offset );

            /* Try length		*/
            read_from_buffer(buffer, 8, 4, &(try_block->length));
            PDEBUG("CILIR: DECODE_EXCEPTIONS:               Try length in bytes	= %d\n",
                   try_block->length );

            /* Handler offset	*/
            read_from_buffer(buffer, 12, 4, &(try_handler->handler_begin_offset));
            PDEBUG("CILIR: DECODE_EXCEPTIONS:               Handler bytes offset	= %d\n",
                   try_handler->handler_begin_offset);

            /* Handler length	*/
            read_from_buffer(buffer, 16, 4, &(try_handler->handler_length));
            PDEBUG("CILIR: DECODE_EXCEPTIONS:               Handler length in bytes	= %d\n",
                   try_handler->handler_length );

            /* Class token		*/
            read_from_buffer(buffer, 20, 4, &(class_token_found));
            PDEBUG("CILIR: DECODE_EXCEPTIONS:               Class Token		= 0x%X\n",
                   class_token_found);
        }

        /* the local variable class_token_found represent the Meta data token for
         * a type-based exception handler or the Offset in method body for filter-based
         * exception handler. */
        if (try_handler->type == EXCEPTION_TYPE) {

            /* WE HAVE TO RETRIEVE THE ILClassID value
             * associated to the token "class_token_found" */

            /* At first we have to define a new local variable called tempMethod.
             * We shall record all the infos taken from the metadata streams into this
             * structure. Once retrieved, the infos should reveal the correct ILClassID
             * for the type we are searching for. */

            TypeDescriptor *exception_class = cliManager->metadataManager->getTypeDescriptorFromToken(cliManager->metadataManager, container, binary, class_token_found);

            /* the ClassID is recorded into the try_handler structure */
            try_handler->class = exception_class;

            PDEBUG("DECODE_EXCEPTION: EXCEPTION_TYPE : Class = %s\n", exception_class->getCompleteName(exception_class));
        } else if (try_handler->type == FILTER_TYPE) {

            /* the class_token_found represent the CIL offset in the current function
             * at which starts the filter procedure. In this case we simply memorize
             * the value of class_token_found into the try_handler structure. */
            try_handler->filter_offset = class_token_found;

            /* The class_token_found must be a correct offset value */
            assert(class_token_found);
        }

        /* Search the labels	*/
        try_handler_start_label = NULL;
        try_handler_end_label = NULL;
        try_filter_start_label = NULL;
        try_block_start_label = NULL;
        try_block_end_label = NULL;

        item = xanList_first(labels);
        while (item != NULL) {
            t_label *tmp;
            tmp = (t_label *) item->data;

            /* VERIFY IF WE HAVE ENCOUNTERED A LABEL THAT POINTS TO THE START OF
             * AN HANDLER SECTION */
            if (tmp->counter == try_handler->handler_begin_offset) {
                try_handler_start_label = tmp;
                if (    try_block_start_label == NULL
                        || try_block_end_label == NULL
                        || try_handler_end_label == NULL ) {
                    item = item->next;
                    continue;
                } else {
                    if (try_handler->type != FILTER_TYPE) {
                        break;
                    }
                    if (try_filter_start_label == NULL) {
                        item = item->next;
                        continue;
                    }
                }
            } else if (       tmp->counter
                              == (    (try_handler->handler_begin_offset)
                                      + (try_handler->handler_length)
                                 )       ) {
                try_handler_end_label = tmp;
                if (    try_block_start_label == NULL
                        || try_block_end_label == NULL
                        || try_handler_start_label == NULL ) {
                    item = item->next;
                    continue;
                } else {
                    if (try_handler->type != FILTER_TYPE) {
                        break;
                    }
                    if (try_filter_start_label == NULL) {
                        item = item->next;
                        continue;
                    }
                }
            } else if ( tmp->counter == try_block->begin_offset) {
                try_block_start_label = tmp;
                if (    try_block_end_label == NULL
                        || try_handler_end_label == NULL
                        || try_handler_start_label == NULL ) {
                    item = item->next;
                    continue;
                } else {
                    if (try_handler->type != FILTER_TYPE) {
                        break;
                    }
                    if (try_filter_start_label == NULL) {
                        item = item->next;
                        continue;
                    }
                }
            } else if (       tmp->counter
                              == (    (try_block->begin_offset)
                                      + (try_block->length)   ) ) {
                try_block_end_label = tmp;
                if (    try_block_start_label == NULL
                        || try_handler_end_label == NULL
                        || try_handler_start_label == NULL ) {
                    item = item->next;
                    continue;
                } else {
                    if (try_handler->type != FILTER_TYPE) {
                        break;
                    }
                    if (try_filter_start_label == NULL) {
                        item = item->next;
                        continue;
                    }
                }
            }
            /* IF THE NEXT LABEL MARKS THE ENTRY-POINT
             * FOR THE CURRENT FILTER BLOCK */
            else if (       try_handler->type == FILTER_TYPE
                            && (    tmp->counter
                                    == try_handler->filter_offset) ) {
                try_filter_start_label = tmp;
                if (    try_block_start_label   == NULL
                        || try_handler_end_label == NULL
                        || try_handler_start_label == NULL
                        || try_block_end_label  == NULL ) {
                    item = item->next;
                    continue;
                } else {
                    break;
                }
            }

            item = item->next;
        }

        if (try_handler_start_label == NULL) {

            /* Insert the label	*/
            try_handler_start_label = (t_label *) allocMemory(sizeof(t_label));
            try_handler_start_label->ID = (*current_label_ID);
            try_handler_start_label->counter = (try_handler->handler_begin_offset);
            try_handler_start_label->ir_position = -1;
            try_handler_start_label->cil_position = -1;
            try_handler_start_label->byte_offset = (try_handler->handler_begin_offset);
            try_handler_start_label->stack = NULL;
            try_handler_start_label->type = EXCEPTION_HANDLING_LABEL_TYPE;
            try_handler_start_label->starting_blocks = xanList_new(allocFunction, freeFunction, NULL);
            try_handler_start_label->starting_handler = try_handler;
            try_handler_start_label->ending_handler = NULL;
            try_handler_start_label->ending_block = NULL;
            assert(IRMETHOD_getLabelInstruction(method->getIRMethod(method), try_handler_start_label->ID) == NULL);

            cil_insert_label(cilStack, labels, try_handler_start_label, stack, -1);
            PDEBUG("CILIR: DECODE_EXCEPTIONS:	Insert an handler_start label \"L%d\"\n", *current_label_ID);
            (*current_label_ID)++;

        } else {
            assert(try_handler_start_label->starting_handler == NULL);
            try_handler_start_label->type = EXCEPTION_HANDLING_LABEL_TYPE;
            try_handler_start_label->starting_handler = try_handler;
        }

        if (try_handler_end_label == NULL) {

            /* Insert the label	*/
            try_handler_end_label = (t_label *) allocMemory(sizeof(t_label));
            try_handler_end_label->ID = *current_label_ID;
            try_handler_end_label->counter = (try_handler->handler_begin_offset) + (try_handler->handler_length);
            try_handler_end_label->ir_position = -1;
            try_handler_end_label->cil_position = -1;
            try_handler_end_label->byte_offset = (try_handler->handler_begin_offset) + (try_handler->handler_length);
            try_handler_end_label->stack = NULL;
            try_handler_end_label->type = EXCEPTION_HANDLING_LABEL_TYPE;
            try_handler_end_label->starting_blocks = xanList_new(allocFunction, freeFunction, NULL);
            try_handler_end_label->starting_handler = NULL;
            try_handler_end_label->ending_handler = try_handler;
            try_handler_end_label->ending_block = NULL;

            cil_insert_label(cilStack, labels, try_handler_end_label, stack, -1);
            PDEBUG("CILIR: DECODE_EXCEPTIONS:	Insert an handler_end "
                   "label \"L%d\"\n", *current_label_ID);
            (*current_label_ID)++;
            assert(IRMETHOD_getLabelInstruction(method->getIRMethod(method), try_handler_end_label->ID) == NULL);
        } else {
            assert(try_handler_end_label->ending_handler == NULL);
            try_handler_end_label->type = EXCEPTION_HANDLING_LABEL_TYPE;
            try_handler_end_label->ending_handler = try_handler;
        }

        if (try_block_start_label == NULL) {

            /* Insert the label	*/
            try_block_start_label = (t_label *) allocMemory(sizeof(t_label));
            try_block_start_label->ID = *current_label_ID;
            try_block_start_label->counter = (try_block->begin_offset);
            try_block_start_label->ir_position = -1;
            try_block_start_label->cil_position = -1;
            try_block_start_label->byte_offset = (try_block->begin_offset);
            try_block_start_label->stack = NULL;
            try_block_start_label->type = EXCEPTION_HANDLING_LABEL_TYPE;
            try_block_start_label->starting_blocks = xanList_new(allocFunction, freeFunction, NULL);
            assert(try_block_start_label->starting_blocks != NULL);
            xanList_append(try_block_start_label->starting_blocks, try_block);

            try_block_start_label->starting_handler = NULL;
            try_block_start_label->ending_handler = NULL;
            try_block_start_label->ending_block = NULL;

            cil_insert_label(cilStack, labels, try_block_start_label, stack, -1);
            PDEBUG("CILIR: DECODE_EXCEPTIONS:	Insert a try start "
                   "label \"L%d\"\n", *current_label_ID);
            (*current_label_ID)++;
            assert(IRMETHOD_getLabelInstruction(method->getIRMethod(method), try_block_start_label->ID) == NULL);
        } else {
            XanListItem             *current_block;
            PDEBUG("CILIR: DECODE_EXCEPTIONS:	"
                   "TRY_BLOCK_START_LABEL already present \n");
            PDEBUG("CILIR: DECODE_EXCEPTIONS:	"
                   "TRY_BLOCK_START_LABEL \"l%d\" \n",
                   try_block_start_label->ID);
            if (xanList_length(try_block_start_label->starting_blocks) > 0 ) {
                PDEBUG("CILIR: DECODE_EXCEPTIONS:	starting_block != NULL \n");
                assert(try_block_start_label->type == EXCEPTION_HANDLING_LABEL_TYPE);

                current_block = xanList_first(try_block_start_label->starting_blocks);
                assert(current_block != NULL);
                while (current_block != NULL) {

                    t_try_block     *current_try_block =
                        (t_try_block *) current_block->data;

                    assert(current_try_block != NULL);
                    PDEBUG( "CILIR: DECODE_EXCEPTIONS:	"
                            "current_try_block->length (%d)== "
                            "try_block->length (%d)\n"
                            , current_try_block->length, try_block->length);

                    if (current_try_block->length == try_block->length) {
                        /* THE BLOCK IS ALREADY INSERTED */
                        PDEBUG("BLOCK ALREADY INSERTED! \n");
                        break;
                    }

                    current_block = current_block->next;
                }

                /* THE BLOCK WAS NEVER INSERTED */
                if (current_block == NULL) {
                    PDEBUG("BLOCK WAS NEVER INSERTED! \n");
                    xanList_append(try_block_start_label->starting_blocks, try_block);
                }
            } else {
                try_block_start_label->type = EXCEPTION_HANDLING_LABEL_TYPE;
                xanList_append(try_block_start_label->starting_blocks, try_block);
            }
        }

        if (try_block_end_label == NULL) {
            if (    ((try_block->begin_offset) + (try_block->length))
                    == (try_handler->handler_begin_offset) ) {
                assert(try_handler_start_label != NULL);
                try_handler_start_label->ending_block = try_block;
            } else {

                /* Insert the label	*/
                try_block_end_label = (t_label *) allocMemory(sizeof(t_label));
                try_block_end_label->ID = *current_label_ID;
                try_block_end_label->counter = (try_block->begin_offset) + (try_block->length);
                try_block_end_label->ir_position = -1;
                try_block_end_label->cil_position = -1;
                try_block_end_label->byte_offset = (try_block->begin_offset) + (try_block->length);
                try_block_end_label->stack = NULL;
                try_block_end_label->type = EXCEPTION_HANDLING_LABEL_TYPE;
                try_block_end_label->starting_blocks = xanList_new(allocFunction, freeFunction, NULL);
                try_block_end_label->starting_handler = NULL;
                try_block_end_label->ending_handler = NULL;
                try_block_end_label->ending_block = try_block;
                cil_insert_label(cilStack, labels, try_block_end_label, stack, -1);
                PDEBUG("CILIR: DECODE_EXCEPTIONS:	Insert a try end label \"L%d\"\n", *current_label_ID);
                (*current_label_ID)++;
                assert(IRMETHOD_getLabelInstruction(method->getIRMethod(method), try_block_end_label->ID) == NULL);
            }
        } else {
            try_block_end_label->type = EXCEPTION_HANDLING_LABEL_TYPE;
            PDEBUG("%p  --- %p \n", try_block_end_label->ending_block, try_block);
            if (try_block_end_label->ending_block == NULL) {
                try_block_end_label->ending_block = try_block;
            } else {
                assert(try_block_end_label->ending_block->begin_offset == try_block->begin_offset);
                assert(try_block_end_label->ending_block->length == try_block->length);
            }
        }

        if (try_filter_start_label == NULL && try_handler->type == FILTER_TYPE) {
            if (try_handler->filter_offset ==
                    ((try_block->begin_offset) + (try_block->length))) {
                assert(try_block_end_label != NULL);
                try_block_end_label->type = EXCEPTION_HANDLING_LABEL_TYPE;
                try_block_end_label->starting_handler = try_handler;
            } else {
                /* Insert the label	*/
                try_filter_start_label = (t_label *) allocMemory(sizeof(t_label));
                try_filter_start_label->ID = *current_label_ID;
                try_filter_start_label->counter = try_handler->filter_offset;
                try_filter_start_label->ir_position = -1;
                try_filter_start_label->cil_position = -1;
                try_filter_start_label->byte_offset = try_handler->filter_offset;
                try_filter_start_label->stack = NULL;
                try_filter_start_label->type = EXCEPTION_HANDLING_LABEL_TYPE;
                try_filter_start_label->starting_blocks = xanList_new(allocFunction, freeFunction, NULL);
                try_filter_start_label->starting_handler = try_handler;
                try_filter_start_label->ending_handler = NULL;
                try_filter_start_label->ending_block = NULL;

                cil_insert_label(cilStack, labels, try_filter_start_label, stack, -1);
                PDEBUG("CILIR: DECODE_EXCEPTIONS:		Insert a start filter \
						label \"L%d\"\n", *current_label_ID);
                (*current_label_ID)++;
                assert(IRMETHOD_getLabelInstruction(method->getIRMethod(method), try_filter_start_label->ID) == NULL);
            }
        } else {
            if (try_handler->type == FILTER_TYPE) {
                assert(try_filter_start_label != NULL);
                try_filter_start_label->type = EXCEPTION_HANDLING_LABEL_TYPE;
                try_filter_start_label->starting_handler = try_handler;
            }
        }

        /* Set the label-IDs into the handler and try blocks	*/
        try_handler->handler_start_label_ID = try_handler_start_label->ID;
        try_handler->handler_end_label_ID = try_handler_end_label->ID;
        try_block->try_start_label_ID = try_block_start_label->ID;
        if (try_block_end_label != NULL) {
            try_block->try_end_label_ID = try_block_end_label->ID;
        } else {
            try_block->try_end_label_ID = try_handler_start_label->ID;
        }
        if (try_handler->type == FILTER_TYPE) {
            if (try_filter_start_label != NULL) {
                try_handler->filter_start_label_ID = try_filter_start_label->ID;
            } else {
                try_handler->filter_start_label_ID = try_block_end_label->ID;
            }
        }

        /* update the Method inserting the try-block infos */
        PDEBUG("CILIR: DECODE_EXCEPTIONS:		updating method handlers \n");
        method->insertAnHandler(method, try_block, try_handler);

        /* GO TO THE NEXT SECTION */
        if (is_fat == 0) {
            data_size -= 12;
        } else {
            data_size -= 24;
        }
    }

    /* Seek the file		*/
    PDEBUG("CILIR: DECODE_EXCEPTIONS: Seek %d bytes backward\n", body_size + bytes_read);
    if (fseek((*(binary->binary.reader))->stream, (JITINT32) -(body_size + bytes_read), SEEK_CUR) != 0) {
        print_err("CILIR: DECODE_EXCEPTIONS: ERROR= Fseek return an error. ", errno);
        return NO_SEEK_POSSIBLE;
    }

    /* Return			*/
    return 0;
}

static inline t_try_block * find_inner_label (XanList *blocks, JITUINT32 current_offset) {
    XanListItem             *item;
    t_try_block     *block;
    t_try_block     *champion_block;

    /* Assertions		*/
    assert(blocks != NULL);

    champion_block = NULL;
    item =xanList_first(blocks);
    while (item != NULL) {
        block = (t_try_block *) item->data;
        assert(block != NULL);
        if ((block->begin_offset <= current_offset) && ((block->length + block->begin_offset) >= current_offset)) {
            if (champion_block == NULL) {
                champion_block = block;
            } else if ((champion_block->begin_offset + champion_block->length) > (block->begin_offset + block->length)) {
                champion_block = block;
            }
        }
        item = item->next;
    }

    if (champion_block == NULL) {
        return NULL;
    }
    return champion_block;
}

static inline t_try_block * find_next_inner_label (XanList *blocks, t_try_block *inner_block) {
    XanListItem             *item;
    t_try_block     *block;
    t_try_block     *champion_block;

    /* Assertions			*/
    assert(blocks != NULL);

    /* Initialize the variables	*/
    champion_block = NULL;
    item = NULL;
    block = NULL;

    item =xanList_first(blocks);
    while (item != NULL) {
        block = (t_try_block *) item->data;
        assert(inner_block != NULL);
        assert(block != NULL);
        if (    (block->begin_offset <= inner_block->begin_offset)                                              &&
                ((block->length + block->begin_offset) >= (inner_block->length + inner_block->begin_offset))    &&
                (block != inner_block)                                                                          ) {
            /* The block enclose the inner_block	*/
            if (champion_block == NULL) {
                champion_block = block;
            } else if ((champion_block->begin_offset + champion_block->length) > (block->begin_offset + block->length)) {
                champion_block = block;
            }
        }
        item = item->next;
    }

    if (champion_block == NULL) {
        return NULL;
    }
    return champion_block;
}

static inline JITINT16 cil_insert_label (CILStack cilStack, XanList *labels, t_label *label, t_stack *stack, JITINT32 position) {

    /* Assertions		*/
    assert(cilStack != NULL);
    assert(labels != NULL);
    assert(label != NULL);
    assert(stack != NULL);
    PDEBUG("CILIR: cil_insert_label: Start\n");
    PDEBUG("CILIR: cil_insert_label:        Insert the label \"L%d\"\n", label->ID);

    /* Copy the stack	*/
    label->stack = cilStack->cloneStack(cilStack, stack);
    assert(label->stack != NULL);

    /* Insert the label	*/
    if (position < 0) {
        xanList_append(labels, (void *) label);
    } else {
        XanListItem *item = xanList_getElementFromPositionNumber(labels, position + 1);
        xanList_insertBefore(labels, item, (void *) label);
    }

    /* Return		*/
    PDEBUG("CILIR: cil_insert_label: Exit\n");
    return 0;
}

static inline t_label * get_label_by_bytes_offset (XanList *labels, JITUINT32 bytes_offset) {
    XanListItem     *item;
    t_label *label;

    item = xanList_first(labels);
    while (item != NULL) {
        label = (t_label *) item->data;
        if (label->byte_offset == bytes_offset) {
            return label;
        }
        item = item->next;
    }
    return NULL;
}

/***************************************************************************************************************************
*                                       TRANSLATE FUNCTION
***************************************************************************************************************************/
static inline JITINT16 translate_cil_ret (CLIManager_t *cliManager, Method method, ir_method_t *irMethod, CILStack cilStack, JITUINT32 *current_label_ID, JITUINT32 bytes_read, XanList *labels, t_stack *stack) {
    ir_instruction_t *instruction;
    JITUINT32 return_type;

    /* Assertions			*/
    assert(method != NULL);
    assert(cilStack != NULL);
    assert(current_label_ID != NULL);
    assert(labels != NULL);
    assert(stack != NULL);

    return_type = IRMETHOD_getResultType(irMethod);
    instruction = method->newIRInstr(method);
    instruction->type = IRRET;
    if (return_type != IRVOID) {
        if (return_type != (stack->stack[(stack->top) - 1]).internal_type) {
            _perform_conversion(cliManager, method, bytes_read, stack, return_type, JITFALSE, instruction);
        }
        memcpy(&(instruction->param_1), &(stack->stack[(stack->top) - 1]), sizeof(ir_item_t));
        (stack->top)--;
    }
    instruction->byte_offset = bytes_read;

    return 0;
}

static inline JITINT16 translate_cil_branch (Method method, CILStack cilStack, JITUINT32 *current_label_ID, JITUINT32 bytes_read, JITINT32 jump_offset, XanList *labels, JITUINT32 inst_size, t_stack *stack) {
    ir_instruction_t *instruction;

    /* Assertions			*/
    assert(method != NULL);
    assert(cilStack != NULL);
    assert(current_label_ID != NULL);
    assert(labels != NULL);
    assert(stack != NULL);

    /* Check if the jump_offset is 0 */
    if (jump_offset == 0) {
        PDEBUG("CILIR: CIL_BRANCH: Jump to the next instruction\n");
        return 0;
    }

    /* Create the instruction */
    instruction = method->newIRInstr(method);
    instruction->type = IRBRANCH;
    (instruction->param_1).value.v = *current_label_ID;
    (instruction->param_1).type = IRLABELITEM;
    (instruction->param_1).internal_type = IRLABELITEM;
    instruction->byte_offset = bytes_read;

    /* Create the label		*/
    add_label_inst(method, cilStack, labels, jump_offset, inst_size, bytes_read, current_label_ID, &(instruction->param_1), stack);
    assert((instruction->param_1).type == IRLABELITEM);

    /* Insert the instruction	*/
    PDEBUG("CILIR: CIL_BRANCH: Insert a IR instruction\n");
    return 0;
}

/**
 * Test whether the type represented by the token is identical to the type stored in the TypedReference on the stack
 *
 * @return 0 if everything is correct (the two types are identical), 1 otherwise
 */
static inline JITINT16 translate_Test_Type_Identical_to_TypedRef (t_system *system, Method method, CILStack cilStack, JITUINT32 *current_label_ID, XanList *labels, JITUINT32 bytes_read, t_stack *stack, GenericContainer *container, t_binary_information *binary, JITUINT32 token) {
    TypeDescriptor  *type_of_the_token;
    ir_instruction_t        *inst;
    ir_item_t rth;                  /**< The item representing the RuntimeTypeHandle */
    ir_item_t clrType;              /**< The item representing the CLRType */
    ir_item_t clsID;                /** The item representing the ClassID extracted from the CLRType */
    ir_item_t cmpResult;            /** The item representing the comparison result */
    JITUINT32 branch_label_ID;      /** The number of the label we need to jump to */

#ifdef DEBUG
    JITUINT32 stackTop;
#endif

    /* Assertions							*/
    assert(system != NULL);
    assert(method != NULL);
    assert(binary != NULL);
    assert(stack != NULL);

    /* Check if runtime checks are enabled			*/
    if (!(system->cliManager.CLR).runtimeChecks) {

        /* The runtime checks have been disabled		*/
        return 0;
    }

    /* Fetch the stack top						*/
#ifdef DEBUG
    stackTop = stack->top;
#endif

    /* Extract the RuntimeTypeHandle from the TypedReference	*/
    inst = method->newIRInstr(method);
    inst->type = IRLOADREL;

    /* Base address of the TypedReference:                          */
    memcpy(&(inst->param_1), &(stack->stack[stack->top - 1]), sizeof(ir_item_t));

    /* Offset of the "type" field:                                  */
    (inst->param_2).value.v = (JITINT32) (system->cliManager).CLR.typedReferenceManager.getTypeOffset(&(((system->cliManager).CLR.typedReferenceManager)));
    (inst->param_2).type = IRINT32;
    (inst->param_2).internal_type = IRINT32;

    /* Where to put the result (on a new temporary): */
    memset(&rth, 0, sizeof(ir_item_t));
    rth.value.v = method->incMaxVariablesReturnOld(method);
    rth.type = IROFFSET;
    rth.internal_type = IRNINT;

    /*set the return value: */
    memcpy(&(inst->result), &rth, sizeof(ir_item_t));

    /* extract the CLRType from the RuntimeTypeHandle */
    inst = method->newIRInstr(method);
    inst->type = IRLOADREL;
    /* Base address of the RuntimeTypeHandle: */
    memcpy(&(inst->param_1), &(rth), sizeof(ir_item_t));
    /* Offset of the "value" field (that contains the CLRType): */
    (inst->param_2).value.v = (JITINT32) (system->cliManager).CLR.runtimeHandleManager.getRuntimeTypeHandleValueOffset(&((system->cliManager).CLR.runtimeHandleManager));
    (inst->param_2).type = IRINT32;
    (inst->param_2).internal_type = IRINT32;
    /* Where to put the result (on a new temporary): */
    memset(&clrType, 0, sizeof(ir_item_t));
    clrType.value.v = method->incMaxVariablesReturnOld(method);
    clrType.type = IROFFSET;
    clrType.internal_type = IRNINT;
    memcpy(&(inst->result), &clrType, sizeof(ir_item_t));

    /* extract the the ClassID from the CLRType */
    inst = method->newIRInstr(method);
    inst->type = IRLOADREL;
    /* Base address of the CLRType: */
    memcpy(&(inst->param_1), &(clrType), sizeof(ir_item_t));
    /* Offset of the "ID" field: */
    (inst->param_2).value.v = (JITINT32) offsetof(CLRType, ID);
    (inst->param_2).type = IRINT32;
    (inst->param_2).internal_type = IRINT32;
    /* Where to put the result (on a new temporary): */
    memset(&clsID, 0, sizeof(ir_item_t));
    clsID.value.v = method->incMaxVariablesReturnOld(method);
    clsID.type = IROFFSET;
    clsID.internal_type = IRCLASSID;
    memcpy(&(inst->result), &clsID, sizeof(ir_item_t));

    /* Extract the type from the token	*/
    /* fill the 'type_of_the_token' structure, obtaining the info through the method informations gathered by get_entry_point */
    type_of_the_token = (system->cliManager).metadataManager->getTypeDescriptorFromToken(system->cliManager.metadataManager, container, binary, token);
    assert(type_of_the_token != NULL);

    /* Check whether the two types are identical */
    inst = method->newIRInstr(method);
    inst->type = IREQ;
    /* Load the classID from the TypedReference */
    memcpy(&(inst->param_1), &clsID, sizeof(ir_item_t));
    /* Load the classID from the token */
    (inst->param_2).value.v = (JITNUINT) type_of_the_token;
    (inst->param_2).type = IRCLASSID;
    (inst->param_2).internal_type = IRCLASSID;
    /* The result of the comparison: */
    memset(&cmpResult, 0, sizeof(ir_item_t));
    cmpResult.value.v = method->incMaxVariablesReturnOld(method);
    cmpResult.type = IROFFSET;
    cmpResult.internal_type = IRINT32;
    memcpy(&(inst->result), &cmpResult, sizeof(ir_item_t));
    assert((inst->param_1).internal_type == (inst->param_2).internal_type);

    /* Branch if the result of the compare is true	*/
    inst = method->newIRInstr(method);
    inst->type = IRBRANCHIF;
    inst->byte_offset = bytes_read;
    /* Condition (the result of the compare): */
    memcpy(&(inst->param_1), &cmpResult, sizeof(ir_item_t));
    assert((inst->param_1).type == IROFFSET);
    assert((inst->param_1).internal_type == IRINT32 );
    /* The label we want to jump to */
    (inst->param_2).value.v = (*current_label_ID);
    branch_label_ID = (inst->param_2).value.v;
    (*current_label_ID)++;
    (inst->param_2).type = IRLABELITEM;
    (inst->param_2).internal_type = IRLABELITEM;

    /* translate the THROW System.InvalidCastException */
    /* the unbox instruction must raise a System.InvalidCastException */
    _translate_throw_CIL_Exception(&(system->cliManager), method, cilStack, current_label_ID, labels, bytes_read, stack, (system->exception_system)._System_InvalidCastException_ID);

    /* create a label instruction */
    inst = method->newIRInstr(method);
    inst->type = IRLABEL;
    inst->byte_offset = bytes_read;
    (inst->param_1).value.v = branch_label_ID;
    (inst->param_1).type = IRLABELITEM;
    (inst->param_1).internal_type = IRLABELITEM;

    /* Check the stack top				*/
#ifdef DEBUG
    assert(stackTop == stack->top);
#endif

    /* Return					*/
    return 0;
}

static inline JITINT16 translate_cil_refanyval (t_system *system, Method method, CILStack cilStack, JITUINT32 *current_label_ID, XanList *labels, JITUINT32 bytes_read, t_stack *stack, GenericContainer *container, t_binary_information *binary, JITUINT32 token) {
    ir_instruction_t        *inst; /** The IR instruction to be generated */

    /* Assertions                           */
    assert(system != NULL);
    assert(method != NULL);
    assert(cilStack != NULL);
    assert(current_label_ID != NULL);
    assert(labels != NULL);
    assert(stack != NULL);
    assert(binary != NULL);

    /* test if the token is valid type - Does not modify the stack */
    if (translate_Test_Type_Load_with_Type_Token(system, method, cilStack, current_label_ID, labels, bytes_read, stack, binary, token) == 1) {
        PDEBUG("CILIR: refanyval: EXITING type load error. invalid type-token detected \n");
        return 1;
    }

    /* Get the address of the TypedReference */
    inst = method->newIRInstr(method);
    inst->type = IRGETADDRESS;
    /* The TypedReference: */
    memcpy(&(inst->param_1), &(stack->stack[stack->top - 1]), sizeof(ir_item_t));
    assert((inst->param_1).internal_type != NOPARAM);

    stack->cleanTop(stack);

    /* Store the address on the stack */
    stack->stack[stack->top].value.v = method->incMaxVariablesReturnOld(method);
    stack->stack[stack->top].type = IROFFSET;
    stack->stack[stack->top].internal_type = IROBJECT;
    if ((inst->param_1).type_infos != NULL) {
        stack->stack[stack->top].type_infos = (inst->param_1).type_infos->makePointerDescriptor((inst->param_1).type_infos);
    } else {
        stack->stack[stack->top].type_infos = NULL;
    }
    memcpy(&(inst->result), &(stack->stack[stack->top]), sizeof(ir_item_t));
    (stack->top)++;

    /* Generate the instructions to test whether the token is identical to the type stored in the TypedRef on the stack
     * Does not modify the stack
     */
    translate_Test_Type_Identical_to_TypedRef(system, method, cilStack, current_label_ID, labels, bytes_read, stack, container, binary, token);

    /* All checks passed: now extract the value from the TypedReference */
    inst = method->newIRInstr(method);
    inst->type = IRLOADREL;
    /* Base address of the TypedReference: */
    memcpy(&(inst->param_1), &(stack->stack[stack->top - 1]), sizeof(ir_item_t));
    /* Offset of the "value" field: */
    (inst->param_2).value.v = (JITINT32) (system->cliManager).CLR.typedReferenceManager.getValueOffset(&(((system->cliManager).CLR.typedReferenceManager)));
    (inst->param_2).type = IRINT32;
    (inst->param_2).internal_type = IRINT32;
    /* Where to put the result (on the stack, where the TypedReference was): */
    stack->stack[stack->top - 2].value.v = method->incMaxVariablesReturnOld(method);
    stack->stack[stack->top - 2].type = IROFFSET;
    stack->stack[stack->top - 2].internal_type = IROBJECT;
    memcpy(&(inst->result), &(stack->stack[stack->top - 2]), sizeof(ir_item_t));

    /* Remove the address of the TypedReference from the stack */
    (stack->top)--;

    return 0;
}

static inline JITINT16 translate_cil_Ckfinite (CLIManager_t *cliManager, Method method, CILStack cilStack, JITUINT32 *current_label_ID, JITUINT32 bytes_read, JITINT32 jump_offset, XanList *labels, t_system *system, JITUINT32 inst_size, t_stack *stack) {
    ir_instruction_t        *instruction;
    t_label                 *label_l1;
    t_label                 *label_l2;
    JITBOOLEAN found;

#ifdef DEBUG
    JITUINT32 stackTop;
#endif

    /* Assertions           */
    assert(cliManager != NULL);
    assert(method != NULL);
    assert(cilStack != NULL);
    assert(current_label_ID != NULL);
    assert(labels != NULL);
    assert(system != NULL);
    assert(stack != NULL);

    /* initialize the locals */
    instruction = NULL;
    label_l1 = NULL;
    label_l2 = NULL;
    found = 0;

    /* Check if runtime checks are enabled			*/
    if (!(cliManager->CLR).runtimeChecks) {

        /* The runtime checks have been disabled		*/
        return 0;
    }

    /* Fetch the stack top						*/
#ifdef DEBUG
    stackTop = stack->top;
#endif

    /* test if the value on the top of the stack is NaN */
    instruction = method->newIRInstr(method);
    instruction->type = IRISNAN;
    memcpy(&(instruction->param_1), &(stack->stack[stack->top - 1]), sizeof(ir_item_t));
    assert( ((instruction->param_1).internal_type == IRNFLOAT)
            || ((instruction->param_1).internal_type == IRFLOAT32)
            || ((instruction->param_1).internal_type == IRFLOAT64));

    /* Clean the top of the stack           */
    stack->cleanTop(stack);

    stack->stack[stack->top].value.v = method->incMaxVariablesReturnOld(method);
    stack->stack[stack->top].type = IROFFSET;
    stack->stack[stack->top].internal_type = IRINT32;
    stack->stack[stack->top].type_infos = NULL;
    memcpy(&(instruction->result), &(stack->stack[stack->top]), sizeof(ir_item_t));
    (stack->top)++;
    instruction->byte_offset = bytes_read;

    /* translate the branch instruction */
    instruction = method->newIRInstr(method);
    instruction->type = IRBRANCHIF;
    instruction->byte_offset = bytes_read;
    (stack->top)--;
    memcpy(&(instruction->param_1), &(stack->stack[stack->top]), sizeof(ir_item_t));
    assert((instruction->param_1).internal_type == IRINT32);
    label_l1 = (t_label *) allocMemory(sizeof(t_label));
    label_l1->ID = (*current_label_ID);
    label_l1->counter = 0;
    label_l1->ir_position = method->getInstructionsNumber(method) - 1;
    label_l1->byte_offset = bytes_read;
    label_l1->type = DEFAULT_LABEL_TYPE;
    cil_insert_label(cilStack, labels, label_l1, stack, -1);
    (*current_label_ID)++;
    assert(IRMETHOD_getLabelInstruction(method->getIRMethod(method), label_l1->ID) == NULL);
    (instruction->param_2).value.v = label_l1->ID;
    (instruction->param_2).type = IRLABELITEM;
    (instruction->param_2).internal_type = IRLABELITEM;

    /* check if the number is infinite */
    instruction = method->newIRInstr(method);
    instruction->type = IRISINF;
    memcpy(&(instruction->param_1), &(stack->stack[stack->top - 1]), sizeof(ir_item_t));
    assert( ((instruction->param_1).internal_type == IRNFLOAT)
            || ((instruction->param_1).internal_type == IRFLOAT32)
            || ((instruction->param_1).internal_type == IRFLOAT64));

    /* Clean the top of the stack           */
    stack->cleanTop(stack);

    stack->stack[stack->top].value.v = method->incMaxVariablesReturnOld(method);
    stack->stack[stack->top].type = IROFFSET;
    stack->stack[stack->top].internal_type = IRINT32;
    stack->stack[stack->top].type_infos = NULL;
    memcpy(&(instruction->result), &(stack->stack[stack->top]), sizeof(ir_item_t));
    (stack->top)++;
    instruction->byte_offset = bytes_read;

    /* translate the branch instruction */
    instruction = method->newIRInstr(method);
    instruction->type = IRBRANCHIFNOT;
    instruction->byte_offset = bytes_read;
    (stack->top)--;
    memcpy(&(instruction->param_1), &(stack->stack[stack->top]), sizeof(ir_item_t));
    assert((instruction->param_1).internal_type == IRINT32);
    label_l2 = find_label_at_position(labels, bytes_read);
    if (label_l2 == NULL) {
        label_l2 = (t_label *) allocMemory(sizeof(t_label));
        label_l2->ID = (*current_label_ID);
        label_l2->counter = 0;
        label_l2->ir_position = method->getInstructionsNumber(method) - 1;
        label_l2->byte_offset = bytes_read;
        label_l2->type = DEFAULT_LABEL_TYPE;
        cil_insert_label(cilStack, labels, label_l2, stack, -1);
        (*current_label_ID)++;
        assert(IRMETHOD_getLabelInstruction(method->getIRMethod(method), label_l2->ID) == NULL);
    } else {
        found = 1;
    }

    (instruction->param_2).value.v = label_l2->ID;
    (instruction->param_2).type = IRLABELITEM;
    (instruction->param_2).internal_type = IRLABELITEM;

    /* fix-up the label l1	*/
    instruction = method->newIRInstr(method);
    instruction->type = IRLABEL;
    instruction->byte_offset = bytes_read;
    (instruction->param_1).value.v = label_l1->ID;
    (instruction->param_1).type = IRLABELITEM;
    (instruction->param_1).internal_type = IRLABELITEM;

    /* throw the System.ArithmeticException (value is NaN or infinity) */
    _translate_throw_CIL_Exception(&(system->cliManager), method, cilStack, current_label_ID, labels, bytes_read, stack, (system->exception_system)._System_ArithmeticException_ID);

    /* Upon exiting the function, we must assure that all the labels are constructed properly.
     * In particular we must verify if the label l2 need to be created */
    if (found == 0) {
        /* fix-up the label l2 */
        instruction = method->newIRInstr(method);
        instruction->type = IRLABEL;
        instruction->byte_offset = bytes_read;
        (instruction->param_1).value.v = label_l2->ID;
        (instruction->param_1).type = IRLABELITEM;
        (instruction->param_1).internal_type = IRLABELITEM;

        /* restore the correct stack */
        assert(label_l2->stack != NULL);
        stack = label_l2->stack;
    }

    /* Check the stack top				*/
#ifdef DEBUG
    assert(stackTop == stack->top);
#endif

    /* Return				*/
    return 0;
}

static inline JITINT16 translate_cil_leave (Method method, CILStack cilStack, JITUINT32 *current_label_ID, JITUINT32 bytes_read, JITINT32 jump_offset, XanList *labels, JITUINT32 inst_size, t_stack *stack) {
    t_try_block             *source_block;
    t_try_block             *target_block;
    t_try_handler           *source_handler;
    t_try_handler           *target_handler;
    JITINT16 result;

    /* Assertions		*/
    assert(method != NULL);
    assert(cilStack != NULL);
    assert(current_label_ID != NULL);
    PDEBUG("CILIR: translate_cil_leave: Start\n");

    source_block = method->findTheOwnerTryBlock(method, NULL, (bytes_read - inst_size));
    PDEBUG("CILIR: translate_cil_leave:     OWNER TRY BLOCK IS [\"L%d\" , \"L%d\"] \n", source_block->try_start_label_ID, source_block->try_end_label_ID);
    target_block = method->findTheOwnerTryBlock(method, NULL, (bytes_read + jump_offset));
#ifdef PRINTDEBUG
    if (target_block != NULL) {
        PDEBUG("CILIR: OWNER TRY BLOCK IS [\"L%d\" , \"L%d\"] \n", target_block->try_start_label_ID, target_block->try_end_label_ID);
    }

#endif
    source_handler = method->findOwnerHandler(source_block, (bytes_read - inst_size));
    target_handler = method->findOwnerHandler(target_block, (bytes_read + jump_offset));

    PDEBUG("CILIR: translate_cil_leave:     Call the function \"perform_leave\"\n");
    result = perform_leave(method, cilStack, current_label_ID, bytes_read, jump_offset, labels, inst_size, stack, source_block, target_block, source_handler, target_handler );

    /* Return		*/
    return result;
}

static inline JITINT16 perform_leave (Method method, CILStack cilStack, JITUINT32 *current_label_ID, JITUINT32 bytes_read, JITINT32 jump_offset, XanList *labels, JITUINT32 inst_size, t_stack *stack, t_try_block *source_block, t_try_block *target_block, t_try_handler *source_handler, t_try_handler *target_handler) {
    t_try_block *old_source_block;

    /* Assertions		*/
    assert(method != NULL);

#ifdef PRINTDEBUG
    if (source_block != NULL) {
        PDEBUG("CILIR: perform_leave : source_block = [\"L%d\", \"L%d\"]\n"
               , source_block->try_start_label_ID
               , source_block->try_end_label_ID);
    } else {
        PDEBUG("CILIR: perform_leave : source_block is NULL\n" );
    }

    if (target_block != NULL) {
        PDEBUG("CILIR: perform_leave : target_block = [\"L%d\", \"L%d\"]\n"
               , target_block->try_start_label_ID
               , target_block->try_end_label_ID);
    } else {
        PDEBUG("CILIR: perform_leave : target_block is NULL\n" );
    }

    if (source_handler != NULL) {
        PDEBUG("CILIR: perform_leave : source_handler = [\"L%d\", \"L%d\"]\n"
               , source_handler->handler_start_label_ID
               , source_handler->handler_end_label_ID);
    } else {
        PDEBUG("CILIR: perform_leave : source_handler is NULL\n" );
    }

    if (target_handler != NULL) {
        PDEBUG("CILIR: perform_leave : target_handler = [\"L%d\", \"L%d\"]\n"
               , target_handler->handler_start_label_ID
               , target_handler->handler_end_label_ID);
    } else {
        PDEBUG("CILIR: perform_leave : target_handler is NULL\n" );
    }
#endif

    if (source_block == target_block) {
        PDEBUG("source_block == target_block \n");
        if ( (source_handler == NULL) && (target_handler == NULL)) {
            PDEBUG(" (source_handler == NULL) && (target_handler == NULL) \n");
            if (source_block == NULL) {
                JITUINT32 old_top;

                PDEBUG("source_block == NULL \n");

                /* WE MUST EMPTY THE STACK_TRACE */
                old_top = stack->top;

                (stack->top) = ( method->getParametersNumber(method)
                                 + method->getLocalsNumber(method) );

                /* THE BEHAVIOUR IS SIMILAR TO THE branch INSTRUCTION BEHAVIOR */
                translate_cil_branch( method, cilStack, current_label_ID, bytes_read, jump_offset, labels, inst_size, stack);

                /* RESTORE THE OLD STACK */
                stack->top = old_top;
            } else {
                PDEBUG("source_block != NULL \n");
                /* WE MUST EMPTY THE STACK_TRACE. TO DO THIS WE USE THE STACK_TRACE
                 * RECORDED INTO THE TRY-BLOCK ELEMENT */
                translate_cil_branch( method, cilStack, current_label_ID, bytes_read, jump_offset, labels, inst_size, source_block->stack_to_restore);
            }
        } else {
            PDEBUG("! ((source_handler == NULL) && (target_handler == NULL)) \n");
            if (target_handler != NULL) {
                PDEBUG("target_handler != NULL \n");
                assert(target_block != NULL);

                /* If the target is within a filter or handler,
                 * the source shall be within the same filter or handler */
                if (    (target_handler->type == FILTER_TYPE)
                        || (target_handler->type == EXCEPTION_TYPE) ) {
                    assert(target_handler == source_handler);
                }
            }

            /* WE MUST EMPTY THE STACK_TRACE. TO DO THIS WE USE THE STACK_TRACE
             * RECORDED INTO THE TRY-BLOCK ELEMENT */
            translate_cil_branch( method, cilStack, current_label_ID, bytes_read, jump_offset, labels, inst_size, source_block->stack_to_restore);
        }
        return 0;
    }

    /*      AT FIRST I HAVE TO SELECT THE PARENT BLOCK AND EXECUTE HIS FINALLY HANDLERS.
     *	TO DO THIS I HAVE TO RESTORE THE STACK MEMORIZED INTO THE STRUCTURE OF THE
     *	SOURCE_BLOCK. */

    PDEBUG("source_block != target_block \n");

    /*	RESTORE THE STACK */
    assert(source_block->stack_to_restore != NULL);

    /*	EXECUTE THE FINALLY HANDLERS ASSOCIATED WITH THE SOURCE_BLOCK.
     *	WE SHALL DO IN THIS WAY BECAUSE WE ARE EXITING THE PROTECTED-BLOCK.
     *	WE MUST NOT EXECUTE THE FAULT HANDLERS BECAUSE AN EXCEPTION HASN'T
     *	OCCURRED */
    if (source_block->finally_handlers != NULL) {

        /*	WE MUST EXECUTE ONLY THE FINALLY HANDLERS */
        XanListItem     *current_finally_or_fault_handler;
        t_try_handler   *current_finally_or_fault_try_handler;

        PDEBUG("source_block->finally_handlers != NULL \n");
        current_finally_or_fault_handler = xanList_first(source_block->finally_handlers);
        assert(current_finally_or_fault_handler != NULL);
        while (current_finally_or_fault_handler != NULL) {
            JITUINT16 current_type;
            current_finally_or_fault_try_handler = current_finally_or_fault_handler->data;
            assert(current_finally_or_fault_try_handler != NULL);

            /* VERIFY IF THE CURRENT HANDLER IS A FINALLY OR A FAULT HANDLER */
            current_type = current_finally_or_fault_try_handler->type;

            assert(current_type != EXCEPTION_TYPE);
            assert(current_type != FILTER_TYPE);
            assert(current_type == FAULT_TYPE || current_type == FINALLY_TYPE);

            if (current_type == FAULT_TYPE) {
                current_finally_or_fault_handler = current_finally_or_fault_handler->next;
                /* analyze the next handler */
                continue;
            } else {
                ir_instruction_t        *instruction;
                t_label                 *label_dfa;

                /*	WE HAVE A FINALLY TYPE.
                 *	WE MUST DEFINE A IRCALLFINALLY INSTRUCTION	*/
                /*	IRCALLFINALLY == call_finally(IRLABELITEM ID)	*/
                PDEBUG("CILIR: CIL_LEAVE:       Jump to the finally clause\n");
                instruction = method->newIRInstr(method);
                instruction->type = IRCALLFINALLY;
                (instruction->param_1).value.v = current_finally_or_fault_try_handler->handler_start_label_ID;
                (instruction->param_1).type = IRLABELITEM;
                (instruction->param_1).internal_type = IRLABELITEM;
                instruction->byte_offset = bytes_read;
                PDEBUG("CILIR: CIL_LEAVE:	Insert a IRCALLFINALLY instruction\n");

                /* Fix up the data-flow label				*/
                instruction = method->newIRInstr(method);
                instruction->type = IRLABEL;
                instruction->byte_offset = bytes_read;

                /* making the label item ... */
                label_dfa = (t_label *) allocMemory(sizeof(t_label));
                label_dfa->ID = (*current_label_ID);
                label_dfa->counter = 0;
                label_dfa->ir_position = method->getInstructionsNumber(method) - 1;
                label_dfa->byte_offset = bytes_read;
                label_dfa->type = DEFAULT_LABEL_TYPE;
                cil_insert_label(cilStack, labels, label_dfa, stack, -1);
                (*current_label_ID)++;
                assert(IRMETHOD_getLabelInstruction(method->getIRMethod(method), label_dfa->ID) == NULL);
                (instruction->param_1).value.v = label_dfa->ID;
                (instruction->param_1).type = IRLABELITEM;
                (instruction->param_1).internal_type = IRLABELITEM;

                /* WE MUST MEMORIZE THIS INFORMATION WITHIN THE HANDLER STRUCTURE IN ORDER
                 * TO ENABLE SPECIFIC DATA-FLOW CONTROLS */
                current_finally_or_fault_try_handler->end_finally_successor_label_ID = label_dfa->ID;
            }

            current_finally_or_fault_handler = current_finally_or_fault_handler->next;
        }
    }

    /*	ONCE EXECUTED ALL THE FINALLY HANDLERS FOR THE BLOCK, WE HAVE TO CHECK
     *	SOME CONDITIONS. AT FIRST WE HAVE TO VERIFY IF THE PARENT OF THE CURRENT BLOCK
     *	IS THE TARGET BLOCK. IF IT ISN'T SO WE HAVE TWO ALTERNATIVES:
     *	1.	THE TARGET BLOCK IS AN INNER TRY-BLOCK OF THE PARENT OF THE CURRENT BLOCK
     *		(THE TARGET IS REACHABLE FROM THE PARENT).
     *	2.	THE PARENT BLOCK IS `NULL` AND SO WE HAVE REACHED THE ROOT OF THE PROTECTED
     *		BLOCK TREE STRUCTURE. IN THIS CASE WE HAVE TO STOP THE SEARCH AND RECOVER
     *		THE STACK-TRACE ASSOCIATED WITH THE TARGET_BLOCK. THEN WE HAVE TO EXECUTE A
     *		BRANCH TO TARGET.
     */

    old_source_block = source_block;
    source_block = source_block->parent_block;

    if (source_block != NULL && target_block != NULL) {
        if (method->isAnInnerTryBlock(source_block, target_block)) {
            /* WE MUST EMPTY THE STACK_TRACE */
            JITUINT32 old_top;

            old_top = stack->top;

            (stack->top) = (method->getParametersNumber(method)
                            + method->getLocalsNumber(method) );

            /* THE BEHAVIOUR IS SIMILAR TO THE branch INSTRUCTION BEHAVIOR */
            translate_cil_branch( method, cilStack, current_label_ID, bytes_read,
                                  jump_offset, labels, inst_size, stack);

            stack->top = old_top;
        } else {
            JITINT16	result;
            result	= perform_leave(method, cilStack, current_label_ID, bytes_read, jump_offset, labels, inst_size, stack, source_block, target_block, NULL, target_handler);
            return result;
        }

    } else if (source_block != NULL && target_block == NULL) {
        JITINT16	result;
        result 	= perform_leave(method, cilStack, current_label_ID, bytes_read, jump_offset, labels, inst_size, stack, source_block, target_block, NULL, target_handler);
        return result;

    } else {

        /*	(source_block == NULL)	*/
        /*	WE ARE IN THE second CONDITION :
         *	--> THE PARENT BLOCK IS NULL BECAUSE
         *	WE REACHED THE ROOT OF THE `PROTECTED-BLOCKS TREE` */
        if (target_block == NULL) {
            /* WE MUST EMPTY THE STACK_TRACE */
            JITUINT32 old_top;

            old_top = stack->top;

            (stack->top) = ( method->getParametersNumber(method) + method->getLocalsNumber(method) );
            /* THE BEHAVIOUR IS SIMILAR TO THE branch INSTRUCTION BEHAVIOR */
            translate_cil_branch( method, cilStack, current_label_ID, bytes_read, jump_offset, labels, inst_size, stack);

            stack->top = old_top;
        } else {
            if (target_block->stack_to_restore == NULL) {
                translate_cil_branch( method, cilStack, current_label_ID, bytes_read,
                                      jump_offset, labels, inst_size, old_source_block->stack_to_restore);
            } else {
                translate_cil_branch( method, cilStack, current_label_ID, bytes_read,
                                      jump_offset, labels, inst_size, target_block->stack_to_restore);
            }
        }
    }

    return 0;
}

static inline JITINT16 translate_cil_call_by_methodID (CLIManager_t *cliManager, Method method, MethodDescriptor *methodID, JITUINT32 bytes_read, t_stack *stack, JITBOOLEAN created, FunctionPointerDescriptor *signature_of_the_jump_method, CILStack cilStack, JITUINT32 *current_label_ID, XanList *labels) {
    Method 			newMethod;
    ir_method_t		*irMethod;
    ir_instruction_t        *instruction;
    JITUINT32 		call_type;
    ir_signature_t          *actual_ir_signature;

    /* Assertions			*/
    assert(cliManager != NULL);
    assert(method != NULL);
    assert(methodID != NULL);
    assert(stack != NULL);

    /* Fetch the IR method of the caller.
     */
    irMethod	= method->getIRMethod(method);

    /* Fetch the callee method.
     * If the method has not been created yet, then it will be created here.
     */
    PDEBUG("CILIR: cil_call_by_methodID: Search the method\n");
    newMethod 	= (cliManager->methods).fetchOrCreateMethod(&(cliManager->methods), methodID, JITFALSE);
    assert(newMethod != NULL);
    PDEBUG("CILIR: cil_call_by_methodID:	Method to call	= %s\n", IRMETHOD_getSignatureInString(newMethod->getIRMethod(newMethod)));

    /* Prepare the actual ir_signature (if possible).
     */
    if (signature_of_the_jump_method == NULL) {

        /* Fetch the signature of the method.
         */
        actual_ir_signature 	= NULL;

    } else {

        /* We have an actual signature. Let's decode and use it.
         */
        actual_ir_signature 	= (ir_signature_t *) allocMemory(sizeof(ir_signature_t));
        (cliManager->translator).translateActualMethodSignatureToIR(cliManager, methodID,  signature_of_the_jump_method, actual_ir_signature);
    }

    /* Check if the callee is provided in machine code.
     */
    if (!(newMethod->IRMethod).ID->attributes->is_pinvoke) {
        if (!newMethod->isIrImplemented(newMethod)) {

            /* The jump method found is a native method	*/
            PDEBUG("\n\n%s\n\n", newMethod->getName(newMethod));
            PDEBUG("CILIR: cil_call_by_methodID:		The IR method is a native call\n");
            call_type = IRLIBRARYCALL;

        } else {

            /* The jump method found is normal method	*/
            PDEBUG("CILIR: cil_call_by_methodID:		The IR method is a normal call\n");
            call_type = IRCALL;
        }
        assert((call_type == IRCALL) || (call_type == IRLIBRARYCALL));
        assert(((call_type == IRLIBRARYCALL) && (!newMethod->isIrImplemented(newMethod))) || (call_type == IRCALL));

        /* Create the IR instruction	*/
        instruction 		= method->newIRInstr(method);
        instruction->type 	= call_type;
        (instruction->param_1).value.v = (JITNUINT) newMethod;
        (instruction->param_1).type = IRMETHODID;
        (instruction->param_1).internal_type = IRMETHODID;
        (instruction->param_2).value.v = 0;
        (instruction->param_2).type = IRINT32;
        (instruction->param_2).internal_type = IRINT32;
        if (actual_ir_signature != NULL) {
            (instruction->param_3).value.v = (JITNUINT) actual_ir_signature;
            (instruction->param_3).type = IRSIGNATURE;
            (instruction->param_3).internal_type = IRSIGNATURE;
        }

    } else {
        PDEBUG("CILIR: cil_call_by_methodID:		PInvoke\n");
        instruction	= (cliManager->CLR).pinvokeManager.addPinvokeInstruction(&((cliManager->CLR).pinvokeManager), irMethod, newMethod, NULL);

    }
    assert(instruction != NULL);
    instruction->byte_offset = bytes_read;

    /* Print the stack			*/
    PDEBUG("CILIR: cil_call_by_methodID:	CIL stack state before making I/O of the call\n");
#ifdef PRINTDEBUG
    print_stack(stack, method->getParametersNumber(method), method->getLocalsNumber(method));
#endif

    /* Create the IO of the IRCALL instruction	*/
    PDEBUG("CILIR: cil_call_by_methodID:	Create the I/O of the IR call instruction\n");
    if (create_call_IO(cliManager, instruction, newMethod, stack, created, method, actual_ir_signature, bytes_read, cilStack, current_label_ID, labels) != 0) {
        print_err("CILIR: ERROR during creating the IO of the method. ", 0);
        abort();
    }
    assert(((instruction->result).type == IROFFSET) || ((instruction->result).type == (instruction->result).internal_type));

    PDEBUG("CILIR: cil_call_by_methodID: Exit\n");
    return 0;
}

/**
 * \brief Perform the first phase of a call
 *
 * Check the validity of input parameters, create a temporary method and test if the method is missing
 * \param tempMethod After the execution contains the pointer to the created temporary method
 * \return 1 if the translate_Test_Missing_Method check fails, 0 otherwise
 */
static inline JITINT16 translate_cil_call_setup (t_system *system, CILStack cilStack, GenericContainer *container, t_binary_information *binary, Method method, JITUINT32 token, JITUINT32 bytes_read, t_stack *stack, JITBOOLEAN isTail, JITUINT32 *current_label_ID, XanList *labels, ActualMethodDescriptor *tempMethod) {

    /* Assertions			*/
    assert(system != NULL);
    assert(cilStack != NULL);
    assert(binary != NULL);
    assert(method != NULL);
    assert(stack != NULL);
    assert(token != 0);
    assert(labels != NULL);
    PDEBUG("CILIR: translate_cil_call_setup: Start\n");

    /* test if the method is missing */
    PDEBUG("CILIR: translate_cil_call_setup:        Test missing method\n");
    if (translate_Test_Missing_Method(system, method, cilStack, current_label_ID, labels, bytes_read, stack, binary, token) == 1) {

        PDEBUG("CILIR : translate_cil_call_setup : EXITING : missing method-token FOUND \n");
        return 1;
    }

    /* Fetch the metadata of the jump method */
    PDEBUG("CILIR: translate_cil_call_setup:        Fetch the metadata\n");

    (*tempMethod) = (system->cliManager).metadataManager->getMethodDescriptorFromToken(system->cliManager.metadataManager, container, binary, token);

    /* Return the result value			*/
    PDEBUG("CILIR: translate_cil_call_setup: Exit\n");
    return 0;
}

static inline JITINT16 translate_cil_call (t_system *system, CILStack cilStack, GenericContainer *container, t_binary_information *binary, Method method, JITUINT32 token, JITUINT32 bytes_read, t_stack *stack, JITBOOLEAN isTail, JITUINT32 *current_label_ID, XanList *labels, JITBOOLEAN created) {
    ActualMethodDescriptor tempMethod;
    JITINT16 result;

    /* Setup the function call */
    if (translate_cil_call_setup(system, cilStack, container, binary, method, token, bytes_read, stack, isTail, current_label_ID, labels, &tempMethod) == 1) {
        return 1;
    }

    /*	System.MethodAccessException can be thrown when there is an invalid attempt
     *	to access a private or protected method inside a class		*/
    PDEBUG("CILIR: translate_cil_call:      Test method access\n");
    if (translate_Test_Method_Access(system, method, cilStack, current_label_ID, labels, bytes_read, stack, tempMethod.method) == 1) {
        PDEBUG("CILIR : cil_call : EXITING : trying to access a private or protected method FOUND \n");
        return 1;
    }

    /* Check whether we must do a conservative type initialization */
    if ((tempMethod.method)->requireConservativeOwnerInitialization(tempMethod.method)) {
        STATICMEMORY_fetchStaticObject(&(system->staticMemoryManager), method, (tempMethod.method)->owner);
    }

    /* Call the translate_cil_call_by_methodID	*/
    PDEBUG("CILIR: translate_cil_call:      Translate the CIL call\n");
    result = translate_cil_call_by_methodID(&(system->cliManager), method, tempMethod.method, bytes_read, stack, created, tempMethod.actualSignature, cilStack, current_label_ID, labels);

    /* Return the result value			*/
    PDEBUG("CILIR: translate_cil_call: Exit\n");
    return result;
}

static inline JITINT16 translate_cil_ldstr (CLIManager_t *cliManager, Method method, CILStack cilStack, JITUINT32 token, JITUINT32 bytes_read, t_stack *stack, t_binary_information *binary, JITUINT32 *current_label_ID, XanList *labels) {
    JITUINT32 row;
    JITINT16 result;
    ir_instruction_t        *instruction;
    ir_symbol_t             *object;

    /* Assertions			*/
    assert(method != NULL);
    assert(stack != NULL);
    assert(cilStack != NULL);
    assert(labels != NULL);
    assert(binary != NULL);
    assert(((token >> 24) & 0x000000FF) == 0x70);
    PDEBUG("CILIR: cil_ldstr: Start\n");

    /* Initialize the variables	*/
    result = 0;

    /* Fetch the string literal	*/
    PDEBUG("CILIR: cil_ldstr:       Search the string from the US stream of %s\n", binary->name);
    row = (token & 0x00FFFFFF);
    PDEBUG("CILIR: cil_ldstr:               Row                     = %u\n", row);
    object = (cliManager->CLR).stringManager.getUserStringSymbol(binary, row);
    assert(object != NULL);

    /* Clean the top of the stack           */
    stack->cleanTop(stack);

    /* Load the object from the US row	*/
    instruction = method->newIRInstr(method);
    instruction->type = IRMOVE;
    (instruction->result).value.v = method->incMaxVariablesReturnOld(method);
    (instruction->result).type = IROFFSET;
    (instruction->result).internal_type = IROBJECT;
    (instruction->result).type_infos = (cliManager->CLR).stringManager.fillILStringType();
    (instruction->param_1).value.v = (JITNUINT) object;
    (instruction->param_1).type = IRSYMBOL;
    (instruction->param_1).internal_type = IROBJECT;
    (instruction->param_1).type_infos = (cliManager->CLR).stringManager.fillILStringType();
    instruction->byte_offset = bytes_read;
    memcpy(&(stack->stack[stack->top]), &(instruction->result), sizeof(ir_item_t));
    (stack->top)++;

    /* Return				*/
    PDEBUG("CILIR: cil_ldstr: Exit\n");
    return result;
}

static inline JITINT16 translate_cil_rethrow (CLIManager_t *cliManager, Method method, CILStack cilStack, JITUINT32 *current_label_ID, XanList *labels, JITUINT32 bytes_read, t_stack *stack, IR_ITEM_VALUE *exceptionVarID) {
    ir_instruction_t	*catchInst;
    ir_method_t		*irMethod;

    /* Assertions				*/
    assert(method != NULL);
    assert(cilStack != NULL);
    assert(current_label_ID != NULL);
    assert(labels != NULL);
    assert(stack != NULL);

    /* Clean the top of the stack           */
    stack->cleanTop(stack);

    /* Fetch the IR method			*/
    irMethod	= method->getIRMethod(method);

    /* Fetch the catcher instruction	*/
    catchInst	= IRMETHOD_getCatcherInstruction(irMethod);

    /* An exception object describing the
     * exception previously raised is pushed newly onto the evaluation stack as the first
     * item. Once loaded, the exception is finally thrown and the behaviour is similar to
     * a normal throw instruction */
    if (catchInst == NULL) {
        if ((*exceptionVarID) == -1) {
            (*exceptionVarID)			= method->incMaxVariablesReturnOld(method);
        }
        assert((*exceptionVarID) != -1);
        stack->stack[stack->top].value.v 	= (*exceptionVarID);
        stack->stack[stack->top].type 		= IROFFSET;
        stack->stack[stack->top].internal_type 	= IROBJECT;
        stack->stack[(stack->top)].type_infos 	= NULL;

    } else {

        memcpy(&(stack->stack[stack->top]), &(catchInst->result), sizeof(ir_item_t));
    }
    (stack->top)++;

    /* at this point the exception object is loaded on the stack. The behaviour of the
     * rethrow instruction from this point on, is equivalent to the throw behaviour */
    translate_cil_throw(cliManager, method, cilStack, current_label_ID, labels, bytes_read, stack);

    return 0;
}

static inline JITINT16 translate_cil_throw (CLIManager_t *cliManager, Method method, CILStack cilStack, JITUINT32 *current_label_ID, XanList *labels, JITUINT32 bytes_read, t_stack *stack) {
    ir_instruction_t        *instruction;
    XanList                 *icall_params;
    ir_item_t               *current_stack_item;

    /* Assertions					*/
    assert(method != NULL);
    assert(cilStack != NULL);
    assert(stack != NULL);
    assert(labels != NULL);

    /* Initialize the variables			*/
    instruction = NULL;

    /*	TO CREATE THE THROW INSTRUCTION WE MUST RETRIEVE THE EXCEPTION OBJECT FROM THE STACK.
     *	AT FIRST WE HAVE TO CHECK IF THE EXCEPTION OBJECT IS A NULL REFERENCE. IF THIS IS THE CASE
     *	WE MUST THROW A System.NullReferenceException.
     *	THE CURRENT TOP OF THE STACK CONTAINS THE EXCEPTION OBJECT. WE SHALL RETRIEVE IT AND
     *	CHECK IF IT IS NULL. */
    translate_Test_Null_Reference(cliManager, method, cilStack, bytes_read, stack);

    /* at this point we are shure that the control code was generated */
    PDEBUG("CILIR: throw:   save the current stack trace \n");
    icall_params = xanList_new(sharedAllocFunction, freeFunction, NULL);
    assert(icall_params != NULL);
    current_stack_item = make_new_IR_param();
    memcpy(current_stack_item, &(stack->stack[(stack->top) - 1]), sizeof(ir_item_t));
    assert(current_stack_item->internal_type == IROBJECT);
    xanList_append(icall_params, current_stack_item);
    assert(icall_params != NULL);
    translate_ncall(method, (JITINT8 *) "setCurrentThreadException", IRVOID, icall_params, bytes_read, 0, NULL);
    translate_ncall(method, (JITINT8 *) "updateTheStackTrace", IRVOID, NULL, bytes_read, 0, NULL);

    /* THROW THE EXCEPTION OBJECT */
    instruction = method->newIRInstr(method);
    instruction->type = IRTHROW;
    (stack->top)--;
    memcpy(&(instruction->param_1), &(stack->stack[stack->top]), sizeof(ir_item_t));
    assert((instruction->param_1).internal_type == IROBJECT);
    assert((instruction->param_1).type == IROFFSET);
    instruction->byte_offset = bytes_read;

    /* AT THIS POINT THE EXCEPTION OBJECT IS THROWN AND IT'S "POPPED" FROM THE STACK */
    return 0;
}

static inline JITINT16 translate_cil_ldind (t_system *system, Method method, JITUINT32 bytes_read, t_stack *stack, JITUINT32 type, JITUINT32 target_type) {
    ir_instruction_t        *instruction;

    /* Assertions		*/
    assert(method != NULL);
    assert(stack != NULL);

    instruction = method->newIRInstr(method);
    instruction->type = IRLOADREL;
    (stack->top)--;
    memcpy(&(instruction->param_1), &(stack->stack[stack->top]), sizeof(ir_item_t));
    (instruction->param_2).value.v            = 0;
    (instruction->param_2).type             = IRINT32;
    (instruction->param_2).internal_type    = IRINT32;

    /* Clean the top of the stack           */
    stack->cleanTop(stack);

    (stack->stack[stack->top]).value.v = method->incMaxVariablesReturnOld(method);
    (stack->stack[stack->top]).type = IROFFSET;
    (stack->stack[stack->top]).internal_type = type;
    (stack->stack[stack->top]).type_infos = (system->cliManager).metadataManager->getTypeDescriptorFromIRType(system->cliManager.metadataManager, type);
    memcpy(&(instruction->result), &(stack->stack[stack->top]), sizeof(ir_item_t));
    instruction->byte_offset = bytes_read;
    (stack->top)++;
    if (type == target_type) {
        return 0;
    }

    /* Translate the value just pushed on the stack	*/
    translate_cil_conv(&(system->cliManager), method, bytes_read, stack, target_type, 0, 0, 1);

    return 0;
}

static inline JITBOOLEAN coerce_operands_for_binary_operation (CLIManager_t *cliManager, Method method, JITUINT32 bytes_read, t_stack *stack) {
    ir_item_t second_operand;
    ir_item_t first_operand;
    JITUINT32 coerced_type;

    /* assertions */
    assert(method != NULL);
    assert(stack != NULL);

    /* Check if the operands are floating point numbers     */
    if (    ((stack->stack[stack->top - 1]).internal_type == IRFLOAT32)     ||
            ((stack->stack[stack->top - 1]).internal_type == IRFLOAT64)     ||
            ((stack->stack[stack->top - 1]).internal_type == IRNFLOAT)      ) {

        /* copy the operands informations               */
        memcpy(&second_operand, &(stack->stack[stack->top - 1]), sizeof(ir_item_t));
        memcpy(&first_operand, &(stack->stack[stack->top - 2]), sizeof(ir_item_t));
        (stack->top)--;
        (stack->top)--;

        /* Check if they are different                  */
        if (first_operand.internal_type != second_operand.internal_type) {
            if (first_operand.internal_type == IRFLOAT64) {

                /* execute an explicit internal-type conversion */
                stack->push(stack, &second_operand);
                translate_cil_conv(cliManager, method, bytes_read, stack, IRFLOAT64, 0, 0, 1);
                memcpy(&second_operand, &(stack->stack[stack->top - 1]), sizeof(ir_item_t));
                assert(second_operand.internal_type == IRFLOAT64);
                (stack->top)--;
            } else if (second_operand.internal_type == IRFLOAT64) {

                /* execute an explicit internal-type conversion */
                stack->push(stack, &first_operand);
                translate_cil_conv(cliManager, method, bytes_read, stack, IRFLOAT64, 0, 0, 1);
                memcpy(&first_operand, &(stack->stack[stack->top - 1]), sizeof(ir_item_t));
                assert(first_operand.internal_type == IRFLOAT64);
                (stack->top)--;

            } else if (first_operand.internal_type == IRNFLOAT) {

                /* execute an explicit internal-type conversion */
                stack->push(stack, &second_operand);
                translate_cil_conv(cliManager, method, bytes_read, stack, IRNFLOAT, 0, 0, 1);
                memcpy(&second_operand, &(stack->stack[stack->top - 1]), sizeof(ir_item_t));
                assert(second_operand.internal_type == IRNFLOAT);
                (stack->top)--;
            } else if (second_operand.internal_type == IRNFLOAT) {

                /* execute an explicit internal-type conversion */
                stack->push(stack, &first_operand);
                translate_cil_conv(cliManager, method, bytes_read, stack, IRNFLOAT, 0, 0, 1);
                memcpy(&first_operand, &(stack->stack[stack->top - 1]), sizeof(ir_item_t));
                assert(first_operand.internal_type == IRNFLOAT);
                (stack->top)--;
            } else {
                abort();
            }
        }
        stack->push(stack, &first_operand);
        stack->push(stack, &second_operand);
    } else {
        JITUINT32 stack_type;

        /* copy the second operand informations */
        (stack->top)--;
        memcpy(&second_operand, &(stack->stack[stack->top]), sizeof(ir_item_t));

        /* retrieve the correct type informations for the first operand */
        stack_type      = (stack->stack[stack->top - 1]).internal_type;
        coerced_type    = (cliManager->metadataManager->generic_type_rules)->coerce_CLI_Type(stack_type);

        /* test if we have to perform an explicit internal-type conversion */
        if (coerced_type != stack_type) {

            /* execute an explicit internal-type conversion */
            translate_cil_conv(cliManager, method, bytes_read, stack, coerced_type, 0, 0, 1);
        }

        /* Clean the top of the stack           */
        stack->cleanTop(stack);

        /* once converted the type of the first operand, we have to reload the
         * second operand on the top of the stack */
        memcpy(&(stack->stack[stack->top]), &second_operand, sizeof(ir_item_t));
        (stack->top)++;

        /* retrieve the correct type informations for the second operand */
        stack_type = (stack->stack[stack->top - 1]).internal_type;
        coerced_type = ((cliManager->metadataManager)->generic_type_rules)->coerce_CLI_Type(stack_type);

        /* test if we have to perform an explicit internal-type conversion */
        if (coerced_type != stack_type) {

            /* execute an explicit internal-type conversion */
            translate_cil_conv(cliManager, method, bytes_read, stack, coerced_type, 0, 0, 1);
        }

        /* Ensure that the two operands are the same	*/
        if (stack->stack[stack->top - 1].internal_type != stack->stack[stack->top - 2].internal_type) {

            /* Convert the value on top of the stack.	*
             * Notice that it does not matter which value 	*
             * be converted (ensured by ECMA 335)		*/
            translate_cil_conv(cliManager, method, bytes_read, stack, stack->stack[stack->top - 2].internal_type, JITFALSE, JITFALSE, JITFALSE);
        }
    }

    return 0;
}

static inline JITBOOLEAN coerce_operands_for_binary_operation_to_unsigned_types (CLIManager_t *cliManager, Method method, JITUINT32 bytes_read, t_stack *stack, JITBOOLEAN force_stackable_type) {
    ir_item_t second_operand;
    JITUINT32 coerced_type;

    /* assertions */
    assert(method != NULL);
    assert(stack != NULL);
    assert(stack->top >= ((method->getParametersNumber(method) + method->getLocalsNumber(method) + 2)));

    /* copy the second operand informations */
    (stack->top)--;
    memcpy(&second_operand, &(stack->stack[(stack->top)]), sizeof(ir_item_t));

    /* retrieve the correct type informations for the first operand */
    coerced_type = (cliManager->metadataManager->generic_type_rules)->to_unsigned_IRType((stack->stack[stack->top - 1]).internal_type);

    /* test if we have to perform an explicit internal-type conversion */
    if (coerced_type != (stack->stack[stack->top - 1]).internal_type) {

        /* execute an explicit internal-type conversion */
        translate_cil_conv(cliManager, method, bytes_read, stack, coerced_type, 0, 0, force_stackable_type);
    }

    /* once converted the type of the first operand, we have to reload the
     * second operand on the top of the stack */
    memcpy(&(stack->stack[(stack->top)]), &second_operand, sizeof(ir_item_t));
    (stack->top)++;

    /* retrieve the correct type informations for the second operand */
    coerced_type = (cliManager->metadataManager->generic_type_rules)->to_unsigned_IRType((stack->stack[stack->top - 1]).internal_type);

    /* test if we have to perform an explicit internal-type conversion */
    if (coerced_type != (stack->stack[stack->top - 1]).internal_type) {

        /* execute an explicit internal-type conversion */
        translate_cil_conv(cliManager, method, bytes_read, stack, coerced_type, 0, 0, force_stackable_type);
    }

    /* Ensure that the two operands are the same	*/
    if (stack->stack[stack->top - 1].internal_type != stack->stack[stack->top - 2].internal_type) {

        /* Convert the value on top of the stack.	*
         * Notice that it does not matter which value 	*
         * be converted (ensured by ECMA 335)		*/
        translate_cil_conv(cliManager, method, bytes_read, stack, stack->stack[stack->top - 2].internal_type, JITFALSE, JITFALSE, JITFALSE);
    }

    /* once converted all the types, we can go on :) */
    return 0;
}

static inline JITINT16 translate_cil_conv (CLIManager_t *cliManager, Method method, JITUINT32 bytes_read, t_stack *stack, JITUINT32 type, JITBOOLEAN ovf, JITBOOLEAN force_unsigned_check, JITBOOLEAN force_stackable_type) {
    JITUINT32 	toType;
    JITUINT32 	from_type;
    JITBOOLEAN 	verified_code;

    /* Assertions			*/
    assert(method != NULL);
    assert(stack != NULL);
    assert(type != NOPARAM);
    assert(stack->top >= ((method->getParametersNumber(method) + method->getLocalsNumber(method) + 1)));
    PDEBUG("CILIR: translate_cil_conv: Start\n");

    /* initialize the ``from_type'' value with the current internal value */
    from_type = (stack->stack[stack->top - 1]).internal_type;
    assert(from_type != NOPARAM);

    /* iinitialize the ``verified_code'' local variable */
    verified_code = 1;

    /* Delete aliases				*/
    switch (from_type) {
        case IRMPOINTER:
        case IRFPOINTER:
            from_type = IRNINT;
            break;
    }
    switch (type) {
        case IRMPOINTER:
            type = IRNINT;
            break;
    }
    switch ((stack->stack[stack->top - 1]).internal_type) {
        case IRMPOINTER:
            _perform_conversion(cliManager, method, bytes_read, stack, IRNINT, 0, NULL);
            assert((stack->stack[stack->top - 1]).internal_type == IRNINT);
            break;
    }

    if (from_type != type) {
        if (    (force_unsigned_check == 1)
                && (!(cliManager->metadataManager->generic_type_rules)->is_unsigned_IRType((stack->stack[stack->top - 1]).internal_type)) ) {
            JITUINT32 unsigned_Type;

            /* retrieve the unsigned type */
            unsigned_Type = (cliManager->metadataManager->generic_type_rules)->to_unsigned_IRType((stack->stack[stack->top - 1]).internal_type);

            /* verify if the unsigned_type is NOPARAM */
            assert(unsigned_Type != NOPARAM);

            /* convert the value to the related unsigned type */
            _perform_conversion(cliManager, method, bytes_read, stack, unsigned_Type, 0, NULL);
        }

        /* check if the current type is a stackable CLI type */
        if ( !(cliManager->metadataManager->generic_type_rules)->is_CLI_Type((stack->stack[stack->top - 1]).internal_type) ) {
            PDEBUG("CILIR: translate_cil_conv: Coercin type from %u to %u \n", (stack->stack[stack->top - 1]).internal_type, from_type);

            /* we must convert the value on the top of the stack into
             * a valid CLI stackable type */
            from_type = (cliManager->metadataManager->generic_type_rules)->coerce_CLI_Type(from_type);
        }

        /* Compute the result type	*/
        PDEBUG("CILIR: translate_cil_conv:      From type		= %u\n", from_type);
        PDEBUG("CILIR: translate_cil_conv:      Request to type		= %u\n", type);
        if ((cliManager->metadataManager->generic_type_rules)->get_conversion_infos_For_Conv_operation(from_type, type, &verified_code) == INVALID_CONVERSION ) {
            char 	buf[DIM_BUF];
            snprintf(buf, DIM_BUF, "CILIR: translate_cil_conv:      Invalid conversion found in method %s!\nThe conversion from type %s to type %s is not allowed by the ECMA-335 standard. ", (char *)IRMETHOD_getSignatureInString(method->getIRMethod(method)), (char *) IRMETHOD_getIRTypeName(from_type), (char *) IRMETHOD_getIRTypeName(type));
            print_err(buf, 0);
            abort();
        }

        /* Convertion allowed	*/
        if (    (from_type != (stack->stack[stack->top - 1]).internal_type)     ||
                (from_type != type)                                             ) {
            PDEBUG("CILIR: translate_cil_conv:      Make an explicit conversion\n");

            /* perform the conversion to the type specified as parameter of the conv */
            _perform_conversion(cliManager, method, bytes_read, stack, type, ovf, NULL);
        }
    }

    /* Check if we need to ensure	*
     * that the target type is      *
     * stackable			*/
    if (force_stackable_type == 1) {

        /* Before returning the result on the stack, we have to convert the result into a valid
         * stackable item */
        PDEBUG("CILIR: translate_cil_conv: converting the result in a valid stackable item\n");
        toType = (cliManager->metadataManager->generic_type_rules)->coerce_CLI_Type(type);
        assert(toType != NOPARAM);
        if (type != toType) {
            _perform_conversion(cliManager, method, bytes_read, stack, toType, 0, NULL);
        }
    }

    /* Check if we need to perform	*
     * a conversion from pointer to	*
     * an integer			*/
    if (	(IRMETHOD_isAPointerType((stack->stack[stack->top - 1]).internal_type))	&&
            (IRMETHOD_isAnIntType(type))						) {
        _perform_conversion(cliManager, method, bytes_read, stack, type, JITFALSE, NULL);
    }

    /* Check if we need to perform	*
     * a conversion from an integer	*
     * to a pointer 		*/
    if (	(IRMETHOD_isAnIntType((stack->stack[stack->top - 1]).internal_type))	&&
            (IRMETHOD_isAPointerType(type))						) {
        _perform_conversion(cliManager, method, bytes_read, stack, type, JITFALSE, NULL);
    }

    /* Return			*/
    PDEBUG("CILIR: translate_cil_conv: Exit\n");
    return 0;
}

static inline JITINT16 translate_cil_stind (CLIManager_t *cliManager, Method method, CILStack cilStack, JITUINT32 bytes_read, t_stack *stack, JITUINT32 *current_label_ID, XanList *labels, JITUINT32 type) {
    ir_instruction_t        *instruction;

    /* Assertions		*/
    assert(method != NULL);
    assert(stack != NULL);
    assert(cilStack != NULL);
    assert(labels != NULL);

    /* Ensure that the value to be stored has the correct type */
    if (type != (stack->stack[stack->top - 1]).internal_type) {
        _perform_conversion(cliManager, method, bytes_read, stack, type, JITFALSE, NULL);
    }

    /* translate the IRSTOREREL instruction */
    instruction = method->newIRInstr(method);
    instruction->type = IRSTOREREL;
    (stack->top)--;
    memcpy(&(instruction->param_3), &(stack->stack[stack->top]), sizeof(ir_item_t));
    (instruction->param_2).value.v = 0;
    (instruction->param_2).type = IRINT32;
    (instruction->param_2).internal_type = IRINT32;
    (stack->top)--;
    memcpy(&(instruction->param_1), &(stack->stack[stack->top]), sizeof(ir_item_t));
    instruction->byte_offset = bytes_read;

    return 0;
}

static inline JITBOOLEAN _translate_arithmetic_operation_un (CLIManager_t *cliManager, Method method, JITUINT32 bytes_read, t_stack *stack, JITUINT32 IRop) {
    JITUINT32 return_type;
    JITUINT32 un_return_type;
    JITUINT32 coerced_first_operand;
    JITUINT32 coerced_second_operand;

    PDEBUG("CILIR: _translate_arithmetic_operation_un: Start\n");

    /* Assertions		*/
    assert(method != NULL);
    assert(stack != NULL);
    assert( (IRop == IRREM)
            ||  (IRop == IRADDOVF)
            ||  (IRop == IRSUBOVF)
            ||  (IRop == IRMULOVF)
            ||  (IRop == IRDIV) );

    un_return_type = NOPARAM;
    coerced_first_operand = NOPARAM;
    coerced_second_operand = NOPARAM;

    /* initialize the coerced operand types */
    coerced_first_operand = (cliManager->metadataManager->generic_type_rules)->coerce_CLI_Type((stack->stack[stack->top - 2]).internal_type);
    coerced_second_operand = (cliManager->metadataManager->generic_type_rules)->coerce_CLI_Type((stack->stack[stack->top - 1]).internal_type);
    assert(coerced_first_operand != NOPARAM);
    assert(coerced_second_operand != NOPARAM);

    /* initialize the return_type variable */
    return_type = (cliManager->metadataManager->generic_type_rules)->get_return_IRType_For_Integer_operation(coerced_first_operand, coerced_second_operand);
    assert(return_type != NOPARAM);

    /* retrieve the unsigned return type */
    un_return_type = (cliManager->metadataManager->generic_type_rules)->to_unsigned_IRType(return_type);
    assert(un_return_type != NOPARAM);

    /* Coerce the two operands to be unsigned values */
    coerce_operands_for_binary_operation_to_unsigned_types(cliManager, method, bytes_read, stack, JITFALSE);

    /* translate the binary operation */
    _translate_binary_operation(method, bytes_read, stack, IRop, un_return_type);

    /* Make an explicit conversion from ``un_return_type'' to ``return_type''       */
    translate_cil_conv(cliManager, method, bytes_read, stack, return_type, 0, 0, 1);

    /* Return									*/
    PDEBUG("CILIR: _translate_arithmetic_operation_un: End\n");
    return 0;
}

static inline JITBOOLEAN _translate_arithmetic_operation (CLIManager_t *cliManager, Method method, JITUINT32 bytes_read, t_stack *stack, JITUINT32 IRop) {
    JITUINT32 		return_internal_type;
    JITBOOLEAN 		verified_code;
    JITUINT16 		type1;
    JITUINT16 		type2;

    /* Assertions		*/
    assert(method != NULL);
    assert(stack != NULL);
    assert( (IRop == IRADD)
            || (IRop == IRADDOVF)
            || (IRop == IRSUB)
            || (IRop == IRSUBOVF)
            || (IRop == IRMUL)
            || (IRop == IRMULOVF)
            || (IRop == IRDIV)
            || (IRop == IRREM));
    PDEBUG("CILIR: translate_arithmetic_operation (%u) : Start\n", IRop);

    /* initialize the values */
    return_internal_type = NOPARAM;
    verified_code = 1;

    /* Print the stack			*/
#ifdef PRINTDEBUG
    PDEBUG("CILIR: translate_arithmetic_operation (%u) :    Stack before the compare instruction\n", IRop);
    print_stack(stack, method->getParametersNumber(method), method->getLocalsNumber(method));
#endif

    /* at first we have to coerce the values - of the two operands
    * on the top of the stack - to be valid stackable CLI types */
    coerce_operands_for_binary_operation(cliManager, method, bytes_read, stack);

    /* Fetch the input types					*/
    type1 = (stack->stack[stack->top - 1]).internal_type;
    type2 = (stack->stack[stack->top - 2]).internal_type;

    /* Adapt the types						*/
    switch (type1) {
        case IROBJECT:
            type1 = IRNINT;
            break;
    }
    switch (type2) {
        case IROBJECT:
            type2 = IRNINT;
            break;
    }

    /* retrieve the internal type for the value that will be returned on the stack
     * by the add instruction */
    if (!(  (IRop == IRADDOVF)
            || (IRop == IRSUBOVF)
            || (IRop == IRMULOVF) )) {
        return_internal_type = (cliManager->metadataManager->generic_type_rules)->get_return_IRType_For_Binary_Numeric_operation(type2, type1, IRop, &verified_code);
        assert(return_internal_type != NOPARAM);
    } else {
        return_internal_type = (cliManager->metadataManager->generic_type_rules)->get_return_IRType_For_Overflow_Arythmetic_operation(type2, type1, IRop, &verified_code);
        assert(return_internal_type != NOPARAM);
    }

    /* Make the operands uniform with the return type	*/
    if (return_internal_type != (stack->stack[stack->top - 1]).internal_type) {
        translate_cil_conv(cliManager, method, bytes_read, stack, return_internal_type, JITFALSE, JITFALSE, JITFALSE);
    }
    if (return_internal_type != (stack->stack[stack->top - 2]).internal_type) {
        ir_item_t	temp;

        /* Pop the first operand				*/
        (stack->top)--;
        memcpy(&temp, &(stack->stack[stack->top]), sizeof(ir_item_t));

        /* Convert the second operand				*/
        translate_cil_conv(cliManager, method, bytes_read, stack, return_internal_type, JITFALSE, JITFALSE, JITFALSE);

        /* Push back the first operand to the stack		*/
        memcpy(&(stack->stack[stack->top]), &temp, sizeof(ir_item_t));
        (stack->top)++;
    }

    /* Translate the binary operation			*/
    _translate_binary_operation(method, bytes_read, stack, IRop, return_internal_type);

    /* Return						*/
    PDEBUG("CILIR: translate_arithmetic_operation (%u) : End\n", IRop);
    return 0;
}

static inline JITINT16 translate_cil_brfalse (Method method, CILStack cilStack, JITUINT32 *current_label_ID, JITUINT32 bytes_read, JITINT32 jump_offset, XanList *labels, JITUINT32 inst_size, t_stack *stack) {
    ir_instruction_t *instruction;

    /* Assertions			*/
    assert(method != NULL);
    assert(cilStack != NULL);
    assert(current_label_ID != NULL);
    assert(labels != NULL);
    assert(stack != NULL);

    /* Check if the jump_offset is 0*/
    if (jump_offset == 0) {
        PDEBUG("CILIR: CIL_BRANCH: Jump to the next instruction\n");
        return 0;
    }

    /* Create the instruction	*/
    instruction = method->newIRInstr(method);
    instruction->type = IRBRANCHIFNOT;
    instruction->byte_offset = bytes_read;
    (stack->top)--;
    memcpy(&(instruction->param_1), &(stack->stack[(stack->top)]), sizeof(ir_item_t));
    add_label_inst(method, cilStack, labels, jump_offset, inst_size, bytes_read, current_label_ID, &(instruction->param_2), stack);

    /* Return			*/
    return 0;
}

static inline JITINT16 translate_cil_brtrue (Method method, CILStack cilStack, JITUINT32 *current_label_ID, JITUINT32 bytes_read, JITINT32 jump_offset, XanList *labels, JITUINT32 inst_size, t_stack *stack) {
    ir_instruction_t *instruction;

    /* Assertions			*/
    assert(method != NULL);
    assert(current_label_ID != NULL);
    assert(labels != NULL);
    assert(stack != NULL);
    assert(cilStack != NULL);

    /* Check if the jump_offset is 0*/
    if (jump_offset == 0) {
        PDEBUG("CILIR: CIL_BRANCH: Jump to the next instruction\n");
        return 0;
    }

    /* Create the instruction	*/
    instruction = method->newIRInstr(method);
    instruction->type = IRBRANCHIF;
    instruction->byte_offset = bytes_read;
    (stack->top)--;
    memcpy(&(instruction->param_1), &(stack->stack[(stack->top)]), sizeof(ir_item_t));
    add_label_inst(method, cilStack, labels, jump_offset, inst_size, bytes_read, current_label_ID, &(instruction->param_2), stack);

    /* Return			*/
    return 0;
}

static inline JITINT16 translate_cil_bneq (CLIManager_t *cliManager, CILStack cilStack, Method method, JITUINT32 *current_label_ID, JITUINT32 bytes_read, JITINT32 jump_offset, XanList *labels, JITUINT32 inst_size, t_stack *stack) {

    /* Assertions		*/
    assert(cilStack != NULL);
    assert(method != NULL);
    assert(current_label_ID != NULL);
    assert(stack != NULL);

    translate_cil_compare(cliManager, method, cilStack, bytes_read, IREQ, stack, current_label_ID, labels, 0, 1, JITFALSE);

    /* Insert the IRBRANCHIFNOT instruction	*/
    translate_cil_brfalse(method, cilStack, current_label_ID, bytes_read, jump_offset, labels, inst_size, stack);

    return 0;
}

static inline JITINT16 translate_cil_beq (CLIManager_t *cliManager, CILStack cilStack,  Method method, JITUINT32 *current_label_ID, JITUINT32 bytes_read, JITINT32 jump_offset, XanList *labels, JITUINT32 inst_size, t_stack *stack) {

    /* Assertions				*/
    assert(cilStack != NULL);
    assert(method != NULL);
    assert(current_label_ID != NULL);
    assert(labels != NULL);
    assert(stack != NULL);

    /* Compare the values			*/
    translate_cil_compare(cliManager, method, cilStack, bytes_read, IREQ, stack, current_label_ID, labels, JITFALSE, JITTRUE, JITFALSE);

    /* Insert the IRBRANCHIF instruction	*/
    translate_cil_brtrue(method, cilStack, current_label_ID, bytes_read, jump_offset, labels, inst_size, stack);

    return 0;
}

static inline JITINT16 translate_cil_bgt (CLIManager_t *cliManager, CILStack cilStack, Method method, JITUINT32 *current_label_ID, JITUINT32 bytes_read, JITINT32 jump_offset, XanList *labels, JITUINT32 inst_size, t_stack *stack, JITBOOLEAN unordered) {

    /* Assertions		*/
    assert(method != NULL);
    assert(current_label_ID != NULL);
    assert(stack != NULL);
    assert(cilStack != NULL);

    translate_cil_compare(cliManager, method, cilStack, bytes_read, IRGT, stack, current_label_ID, labels, unordered, 1, JITFALSE);

    /* Insert the IRBRANCHIF instruction	*/
    translate_cil_brtrue(method, cilStack, current_label_ID, bytes_read, jump_offset, labels, inst_size, stack);

    return 0;
}

static inline JITINT16 translate_cil_blt (CLIManager_t *cliManager, CILStack cilStack, Method method, JITUINT32 *current_label_ID, JITUINT32 bytes_read, JITINT32 jump_offset, XanList *labels, JITUINT32 inst_size, t_stack *stack, JITBOOLEAN unordered) {

    /* Assertions				*/
    assert(method != NULL);
    assert(cilStack != NULL);
    assert(current_label_ID != NULL);
    assert(stack != NULL);
    assert(stack->top >= (method->getParametersNumber(method) + method->getLocalsNumber(method) + 2));
    PDEBUG("CILIR: translate_cil_blt: Start\n");

    /* Print the stack			*/
#ifdef PRINTDEBUG
    PDEBUG("CILIR: translate_cil_blt:       Stack before the compare instruction\n");
    print_stack(stack, method->getParametersNumber(method), method->getLocalsNumber(method));
#endif

    /* Compute the condition to jump	*/
    translate_cil_compare(cliManager, method, cilStack, bytes_read, IRLT, stack, current_label_ID, labels, unordered, 1, JITFALSE);

    /* Print the stack			*/
#ifdef PRINTDEBUG
    PDEBUG("CILIR: translate_cil_blt:       Stack after the compare instruction\n");
    print_stack(stack, method->getParametersNumber(method), method->getLocalsNumber(method));
#endif

    /* Insert the IRBRANCHIF instruction	*/
    translate_cil_brtrue(method, cilStack, current_label_ID, bytes_read, jump_offset, labels, inst_size, stack);

    /* Print the stack			*/
#ifdef PRINTDEBUG
    PDEBUG("CILIR: translate_cil_blt:       Stack after the brtrue instruction\n");
    print_stack(stack, method->getParametersNumber(method), method->getLocalsNumber(method));
#endif

    /* Return				*/
    return 0;
}

static inline JITINT16 translate_cil_ble (CLIManager_t *cliManager, CILStack cilStack, Method method, JITUINT32 *current_label_ID, JITUINT32 bytes_read, JITINT32 jump_offset, XanList *labels, JITUINT32 inst_size, t_stack *stack, JITBOOLEAN unordered) {

    /* Assertions		*/
    assert(method != NULL);
    assert(cilStack != NULL);
    assert(current_label_ID != NULL);
    assert(stack != NULL);
    assert(stack->top >= ((method->getParametersNumber(method) + method->getLocalsNumber(method) + 2)));

    /* Check the offset	*/
    if (jump_offset == 0) {
        (stack->top)--;
        return 0;
    }

    if (     (       ((stack->stack[(stack->top) - 1]).internal_type == IRNFLOAT)
                     || ((stack->stack[(stack->top) - 1]).internal_type == IRFLOAT32)
                     || ((stack->stack[(stack->top) - 1]).internal_type == IRFLOAT64) )
             &&
             (       ((stack->stack[(stack->top) - 2]).internal_type == IRNFLOAT)
                     || ((stack->stack[(stack->top) - 2]).internal_type == IRFLOAT32)
                     || ((stack->stack[(stack->top) - 2]).internal_type == IRFLOAT64)) ) {
        if (unordered == 1) {
            unordered = 0;
        } else {
            unordered = 1;
        }
    }

    translate_cil_compare(cliManager, method, cilStack, bytes_read, IRGT, stack, current_label_ID, labels, unordered, 1, JITFALSE);

    translate_cil_brfalse(method, cilStack, current_label_ID, bytes_read, jump_offset, labels, inst_size, stack);

    return 0;
}

static inline JITINT16 translate_cil_bge (CLIManager_t *cliManager, CILStack cilStack, Method method, JITUINT32 *current_label_ID, JITUINT32 bytes_read, JITINT32 jump_offset, XanList *labels, JITUINT32 inst_size, t_stack *stack, JITBOOLEAN unordered) {

    /* Assertions		*/
    assert(cilStack != NULL);
    assert(method != NULL);
    assert(current_label_ID != NULL);
    assert(stack != NULL);

    if (unordered == 0) {
        if (     (       ((stack->stack[(stack->top) - 1]).internal_type == IRNFLOAT)
                         || ((stack->stack[(stack->top) - 1]).internal_type == IRFLOAT32)
                         || ((stack->stack[(stack->top) - 1]).internal_type == IRFLOAT64) )
                 &&
                 (       ((stack->stack[(stack->top) - 2]).internal_type == IRNFLOAT)
                         || ((stack->stack[(stack->top) - 2]).internal_type == IRFLOAT32)
                         || ((stack->stack[(stack->top) - 2]).internal_type == IRFLOAT64)) ) {
            unordered = 1;
        }
    }

    translate_cil_compare(cliManager, method, cilStack, bytes_read, IRLT, stack, current_label_ID, labels, unordered, 1, JITFALSE);

    translate_cil_brfalse(method, cilStack, current_label_ID, bytes_read, jump_offset, labels, inst_size, stack);

    return 0;
}

static inline JITINT16 translate_cil_shl (t_system *system, Method method, JITUINT32 bytes_read, t_stack *stack) {
    JITUINT32 return_internal_type;

    /* Assertions		*/
    assert(method != NULL);
    assert(stack != NULL);
    assert(system != NULL);

    PDEBUG("CILIR: translate_cil_shl: Start\n");

    /* initialize the values */
    return_internal_type = NOPARAM;

    /* at first we have to coerce the values - of the two operands
    * on the top of the stack - to be valid stackable CLI types */
    coerce_operands_for_binary_operation(&(system->cliManager), method, bytes_read, stack);

    /* retrieve the internal type for the value that will be returned on the stack
     * by the add instruction */
    return_internal_type = (system->cliManager.metadataManager->generic_type_rules)->get_return_IRType_For_Shift_operation((stack->stack[stack->top - 2]).internal_type, (stack->stack[stack->top - 1]).internal_type, IRSHL);
    assert(return_internal_type != NOPARAM);
    _translate_binary_operation(method, bytes_read, stack, IRSHL, return_internal_type);

    /* Return							*/
    PDEBUG("CILIR: translate_cil_shl: End\n");
    return 0;
}

static inline JITINT16 translate_cil_not (Method method, JITUINT32 bytes_read, t_stack *stack) {
    ir_instruction_t        *instruction;

    instruction = method->newIRInstr(method);
    instruction->type = IRNOT;
    (stack->top)--;
    memcpy(&(instruction->param_1), &(stack->stack[stack->top]), sizeof(ir_item_t));

    /* Clean the top of the stack           */
    stack->cleanTop(stack);

    stack->stack[stack->top].value.v = method->incMaxVariablesReturnOld(method);
    stack->stack[stack->top].type = IROFFSET;
    stack->stack[stack->top].internal_type = (instruction->param_1).internal_type;
    assert(stack->stack[stack->top].internal_type != NOPARAM);
    memcpy(&(instruction->result), &(stack->stack[stack->top]), sizeof(ir_item_t));
    (stack->top)++;
    instruction->byte_offset = bytes_read;

    /* Return				*/
    return 0;
}

static inline JITINT16 translate_cil_neg (Method method, JITUINT32 bytes_read, t_stack *stack) {
    ir_instruction_t        *instruction;

    /* Assertions				*/
    assert(method != NULL);
    assert(stack != NULL);

    /* Add the IRNEG instruction		*/
    instruction = method->newIRInstr(method);
    instruction->type = IRNEG;
    instruction->byte_offset = bytes_read;
    (stack->top)--;
    memcpy(&(instruction->param_1), &(stack->stack[stack->top]), sizeof(ir_item_t));
    stack->cleanTop(stack);
    stack->stack[stack->top].value.v = method->incMaxVariablesReturnOld(method);
    stack->stack[stack->top].type = IROFFSET;
    stack->stack[stack->top].internal_type = (instruction->param_1).internal_type;
    (stack->stack[stack->top]).type_infos = (instruction->param_1).type_infos;
    memcpy(&(instruction->result), &(stack->stack[stack->top]), sizeof(ir_item_t));
    (stack->top)++;

    /* Return				*/
    return 0;
}

static inline JITINT16 translate_cil_shr (t_system *system, Method method, JITUINT32 bytes_read, t_stack *stack, JITBOOLEAN unsigned_check) {
    JITUINT32 signed_return_type;
    JITUINT32 un_return_type;
    JITUINT32 coerced_first_operand;
    JITUINT32 coerced_second_operand;

    /* Assertions		*/
    assert(method != NULL);
    assert(stack != NULL);
    assert(system != NULL);
    PDEBUG("CILIR: translate_cil_shr: Start\n");

    /* initialize variables */
    un_return_type = NOPARAM;
    coerced_first_operand = NOPARAM;
    coerced_second_operand = NOPARAM;
    signed_return_type = NOPARAM;

    /* initialize the coerced operand types */
    coerced_first_operand   = ((system->cliManager).metadataManager->generic_type_rules)->coerce_CLI_Type((stack->stack[stack->top - 2]).internal_type);
    coerced_second_operand  = ((system->cliManager).metadataManager->generic_type_rules)->coerce_CLI_Type((stack->stack[stack->top - 1]).internal_type);
    assert(coerced_first_operand != NOPARAM);
    assert(coerced_second_operand != NOPARAM);

    /* initialize the return_type variable */
    /* retrieve the internal type for the value that will be returned on the stack
     * by the add instruction */
    signed_return_type      = ((system->cliManager).metadataManager->generic_type_rules)->get_return_IRType_For_Shift_operation(coerced_first_operand, coerced_second_operand, IRSHR);
    assert(signed_return_type != NOPARAM);

    if (unsigned_check == 1) {

        /* retrieve the unsigned return type */
        un_return_type = ((system->cliManager).metadataManager->generic_type_rules)->to_unsigned_IRType(signed_return_type);
        assert(un_return_type != NOPARAM);
        PDEBUG("CILIR: translate_cil_shr: coercing the two operands to be unsigned...\n");

        /* Coerce the two operands to be unsigned values */
        coerce_operands_for_binary_operation_to_unsigned_types(&(system->cliManager), method, bytes_read, stack, JITFALSE);

        assert(stack->stack[stack->top - 1].internal_type == stack->stack[stack->top - 2].internal_type);
        _translate_binary_operation(method, bytes_read, stack, IRSHR, un_return_type);

        /* Make an explicit conversion from ``un_return_type'' to ``signed_return_type'' */
        translate_cil_conv(&(system->cliManager), method, bytes_read, stack, signed_return_type, 0, 0, 1);
    } else {
        PDEBUG("CILIR: translate_cil_shr: coercing operands to be valid CLI types...\n");

        /* we have to coerce the values - of the two operands
         * on the top of the stack - to be valid stackable CLI types */
        coerce_operands_for_binary_operation(&(system->cliManager), method, bytes_read, stack);

        assert(stack->stack[stack->top - 1].internal_type == stack->stack[stack->top - 2].internal_type);
        _translate_binary_operation(method, bytes_read, stack, IRSHR, signed_return_type);
    }

    PDEBUG("CILIR: translate_cil_shr: End\n");
    return 0;
}

static inline JITINT16 translate_cil_ldobj (Method method, CILStack cilStack, JITUINT32 token, JITUINT32 bytes_read, t_stack *stack, GenericContainer *container, t_binary_information *binary, t_system *system, JITUINT32 *current_label_ID, XanList *labels) {
    TypeDescriptor                  *type_located;
    ir_instruction_t        *instruction;
    JITUINT32 irType;

    /* Assertions		*/
    assert(method != NULL);
    assert(stack != NULL);
    assert(system != NULL);
    assert(cilStack != NULL);
    assert(binary != NULL);
    assert( (stack->stack[stack->top - 1].internal_type == IRNINT)      ||
            (stack->stack[stack->top - 1].internal_type == IRMPOINTER)      ||
            (stack->stack[stack->top - 1].internal_type == IROBJECT)        ||
            (stack->stack[stack->top - 1].internal_type == IRNINT)          );

    /* initialize the local variables */
    instruction = NULL;

    /* test if the token is valid */
    if (translate_Test_Type_Load_with_Type_Token(system, method, cilStack, current_label_ID, labels, bytes_read, stack, binary, token) == 1) {
        PDEBUG("CILIR: ldobj: EXITING type load error. invalid type-token detected \n");
        return 1;
    }

    /* Fetch the type informations	from the metadata addressed by the token */
    type_located = (system->cliManager).metadataManager->getTypeDescriptorFromToken(system->cliManager.metadataManager, container, binary, token);

    /* evaluate the post-conditions of the fetching process */
    assert(type_located != NULL);

    /* test if the type identified by the token is a ValueType	*/
    if (!type_located->isValueType(type_located)) {

        /* If type_located is a reference type, the ldobj instruction
         * has the same effect as ldind.ref. */
        return translate_cil_ldind(system, method, bytes_read, stack, IROBJECT, IROBJECT);
    }

    /* retrieve the layout informations about `type_located` */
    assert(system != NULL);
    assert((system->cliManager).layoutManager.layoutType != NULL);

    irType = type_located->getIRType(type_located);
    (system->cliManager).layoutManager.layoutType(&((system->cliManager).layoutManager), type_located);

    /* set on the stack the value of the valuetype */
    instruction = method->newIRInstr(method);
    instruction->type = IRLOADREL;
    memcpy(&(instruction->param_1), &(stack->stack[stack->top - 1]), sizeof(ir_item_t));
    (instruction->param_2).value.v = 0;
    (instruction->param_2).type = IRINT32;
    (instruction->param_2).internal_type = IRINT32;
    (instruction->result).value.v = method->incMaxVariablesReturnOld(method);
    (instruction->result).type = IROFFSET;
    (instruction->result).internal_type = irType;
    (instruction->result).type_infos = type_located;

    /* Clean the top of the stack           */
    memcpy(&(stack->stack[stack->top - 1]), &(instruction->result), sizeof(ir_item_t));

#ifdef PRINTDEBUG
    PDEBUG("CILIR: ldobj: Print the stack once translated the ir_load_rel instruction \n");
    print_stack(stack, method->getParametersNumber(method), method->getLocalsNumber(method));
#endif

    return 0;
}

static inline JITINT16 translate_cil_stobj (CLIManager_t *cliManager, Method method, CILStack cilStack, JITUINT32 token, JITUINT32 bytes_read, t_stack *stack, GenericContainer *container, t_binary_information *binary, t_system *system, JITUINT32 *current_label_ID, XanList *labels, JITUINT8 alignment) {
    TypeDescriptor                  *type_located;
    ir_instruction_t        *instruction;
    ILLayout                *layout_infos;

    /* Assertions		*/
    assert(method != NULL);
    assert(stack != NULL);
    assert(system != NULL);
    assert(cilStack != NULL);
    assert(binary != NULL);

    /* initialize the local variables */
    instruction = NULL;

    /* thest if the token is valid */
    if (translate_Test_Type_Load_with_Type_Token(system, method, cilStack, current_label_ID, labels, bytes_read, stack, binary, token) == 1) {
        PDEBUG("CILIR: stobj: EXITING type load error. invalid type-token detected \n");
        return 1;
    }

    /* Fetch the type informations	from the metadata addressed by the token */
    type_located = (system->cliManager).metadataManager->getTypeDescriptorFromToken(system->cliManager.metadataManager, container, binary, token);

    /* evaluate the post-conditions of the fetching process */
    assert(type_located != NULL);

    /* test if the type identified by the token is a ValueType	*/
    if (!type_located->isValueType(type_located)) {
        /* If type_located is a reference type, the stobj instruction
         * has the same effect as stind.ref. */
        return translate_cil_stind(cliManager, method, cilStack, bytes_read, stack, current_label_ID, labels, IROBJECT);
    }

    /* retrieve the layout informations about `type_located` */
    assert(system != NULL);
    assert((system->cliManager).layoutManager.layoutType != NULL);

    layout_infos = (system->cliManager).layoutManager.layoutType(&((system->cliManager).layoutManager), type_located);
    assert(layout_infos != NULL);

#ifdef PRINTDEBUG
    PDEBUG("BEFORE THE IRGETADDRESS \n");
    print_stack(stack, method->getParametersNumber(method), method->getLocalsNumber(method));
#endif

    /* Create the IRGETADDRESS of the valuetype on the stack */
    instruction = method->newIRInstr(method);
    instruction->type = IRGETADDRESS;
    (stack->top)--;
    memcpy(&(instruction->param_1), &(stack->stack[stack->top]), sizeof(ir_item_t));
    stack->stack[stack->top].value.v = method->incMaxVariablesReturnOld(method);
    stack->stack[stack->top].type = IROFFSET;
    stack->stack[stack->top].internal_type = IRMPOINTER;
    if ((instruction->param_1).type_infos != NULL) {
        stack->stack[stack->top].type_infos = (instruction->param_1).type_infos->makePointerDescriptor((instruction->param_1).type_infos);
    } else {
        stack->stack[stack->top].type_infos = NULL;
    }
    memcpy(&(instruction->result), &(stack->stack[stack->top]), sizeof(ir_item_t));
    (stack->top)++;
    instruction->byte_offset = bytes_read;
    assert((instruction->param_1).internal_type != NOPARAM);
    PDEBUG("CILIR:			Insert a IR instruction\n");

#ifdef PRINTDEBUG
    PDEBUG("AFTER THE IRGETADDRESS AND BEFORE ENTERING make_irmemcpy \n");
    print_stack(stack, method->getParametersNumber(method), method->getLocalsNumber(method));
#endif

    /* create an IRMEMCPY instruction */
    make_irmemcpy(method, cilStack, bytes_read, stack, current_label_ID, labels, layout_infos->typeSize, UNALIGNED_DONOTCHECK_ALIGNMENT, NULL);

#ifdef PRINTDEBUG
    PDEBUG("AFTER make_irmemcpy \n");
    print_stack(stack, method->getParametersNumber(method), method->getLocalsNumber(method));
#endif

    return 0;
}

static inline JITINT16 translate_cil_stsfld (Method method, CILStack cilStack, JITUINT32 token, JITUINT32 bytes_read, t_stack *stack, GenericContainer *container, t_binary_information *binary, t_system *system, JITUINT32 *current_label_ID, XanList *labels) {
    ir_instruction_t        *instruction;
    FieldDescriptor         *fieldLocated;
    TypeDescriptor          *classID;
    JITINT32 fieldOffset;
    ir_symbol_t              *staticObject;
    JITINT32 irFieldType;

    /* Assertions		*/
    assert(method != NULL);
    assert(stack != NULL);
    assert(binary != NULL);
    assert(system != NULL);
    assert(cilStack != NULL);
    assert(current_label_ID != NULL);
    assert(labels != NULL);
    PDEBUG("CILIR: translate_cil_stsfld: Start\n");

    /* Initialize the variables					*/
    fieldLocated = NULL;

    /* Print the stack						*/
#ifdef PRINTDEBUG
    PDEBUG("CILIR: translate_cil_stsfld:    Stack before the Missing field test\n");
    print_stack(stack, method->getParametersNumber(method), method->getLocalsNumber(method));
#endif

    /* Test if the field exists					*/
    if (translate_Test_Missing_Field(system, method, cilStack, current_label_ID, labels, bytes_read, stack, binary, token) == 1) {
        PDEBUG("CILIR: cil_stsfld: EXITING missing field-token found \n");
        return 1;
    }

    /* Print the stack						*/
#ifdef PRINTDEBUG
    PDEBUG("CILIR: translate_cil_stsfld:    Stack after the Missing field test\n");
    print_stack(stack, method->getParametersNumber(method), method->getLocalsNumber(method));
#endif

    /* retrieve the fieldID and the binary associated with the field */
    fieldLocated = (system->cliManager).metadataManager->getFieldDescriptorFromToken(system->cliManager.metadataManager, container, binary, token);
    assert(fieldLocated != NULL);
    PDEBUG("CILIR: translate_cil_stsfld:    Field name = \"%s\"\n", fieldLocated->getCompleteName(fieldLocated));

    /* test if the current method has the correct rights to access the field */
    PDEBUG("CILIR: translate_cil_stsfld:    Translate Test_Field_Access\n");
    if (translate_Test_Field_Access(system, method, cilStack, current_label_ID, labels, bytes_read, stack, fieldLocated) == 1) {
        PDEBUG("CILIR: translate_cil_stsfld:    Invalid field access detected \n");
        return 1;
    }

    /* Prepare the parameters for the IR instruction	*/
    /* Fetch the class of the field	*/
    PDEBUG("CILIR: translate_cil_stsfld:    Fetch the class ID of the field\n");
    classID = fieldLocated->owner;

    /* Fetch the static object	*/
    staticObject = STATICMEMORY_fetchStaticObjectSymbol(&(system->staticMemoryManager), method, classID);
    assert(staticObject != NULL);

    /* Fetch the field offset */
    fieldOffset = (system->garbage_collectors).gc.fetchStaticFieldOffset(fieldLocated);

    /* Fetch the IR field type	*/
    TypeDescriptor *resolved_type = fieldLocated->getType(fieldLocated);
    irFieldType = resolved_type->getIRType(resolved_type);

    /* Make the conversion from the stackable type to the expected type (irFieldType) if they are different	*/
    if (irFieldType != ((stack->stack[stack->top - 1]).internal_type)) {
        assert((stack->stack[stack->top - 1]).internal_type != IRVALUETYPE);
        _perform_conversion(&(system->cliManager), method, bytes_read, stack, irFieldType, JITFALSE, NULL);
    }

    /* Create the IR instruction						*/
    instruction = method->newIRInstr(method);
    instruction->type = IRSTOREREL;
    instruction->byte_offset = bytes_read;

    /* Memory base address							*/
    (instruction->param_1).value.v = (JITNUINT) staticObject;
    (instruction->param_1).type = IRSYMBOL;
    (instruction->param_1).internal_type = IRMPOINTER;
    (instruction->param_1).type_infos = classID;

    /* Memory offset							*/
    (instruction->param_2).value.v = fieldOffset;
    (instruction->param_2).type = IRINT32;
    (instruction->param_2).internal_type = IRINT32;

    /* Value to store							*/
    (stack->top)--;
    (stack->stack[(stack->top)]).type_infos = resolved_type;
    memcpy(&(instruction->param_3), &(stack->stack[(stack->top)]), sizeof(ir_item_t));

    PDEBUG("CILIR: translate_cil_stsfld: Exit\n");
    return 0;
}

static inline JITINT16 translate_cil_ldsfld (Method method, CILStack cilStack, JITUINT32 token, JITUINT32 bytes_read, t_stack *stack, GenericContainer *container, t_binary_information *binary, t_system *system, JITUINT32 *current_label_ID, XanList *labels) {
    ir_instruction_t        *instruction;
    FieldDescriptor         *fieldLocated;
    TypeDescriptor          *classID;
    JITINT32 field_offset;
    ir_symbol_t             *staticObject;

    /* Assertions		*/
    assert(system != NULL);
    assert(method != NULL);
    assert(stack != NULL);
    assert(cilStack != NULL);
    assert(binary != NULL);
    PDEBUG("CILIR: cil_ldsfld: Start\n");

    /* initialize the local variables */
    instruction = NULL;
    fieldLocated = NULL;

    if (translate_Test_Missing_Field(system, method, cilStack, current_label_ID, labels, bytes_read, stack, binary, token) == 1) {
        PDEBUG("CILIR: cil_ldsfld: EXITING missing field-token found \n");
        return 1;
    }

    /* retrieve the fieldID and the binary associated with the field */
    fieldLocated = (system->cliManager).metadataManager->getFieldDescriptorFromToken(system->cliManager.metadataManager, container, binary, token);
    assert(fieldLocated != NULL);

    PDEBUG("CILIR : translate_cil_ldsfld : FIELD_NAME = \"%s\"\n", fieldLocated->getCompleteName(fieldLocated));

    PDEBUG("CILIR : translate_cil_ldsfld : translate_Test_Field_Access \n");
    /* test if the current method has the correct rights to access the field */
    if (translate_Test_Field_Access(system, method, cilStack, current_label_ID, labels, bytes_read, stack, fieldLocated) == 1) {
        PDEBUG("CILIR : translate_cil_ldsfld : EXITING invalid field access detected \n");
        return 1;
    }

    /* Fetch the class ID of the field	*/
    PDEBUG("CILIR : translate_cil_ldsfld : Fetch the class ID of the field\n");
    classID = fieldLocated->owner;
    assert(classID != NULL);

    /* Fetch the static object	*/
    staticObject = STATICMEMORY_fetchStaticObjectSymbol(&(system->staticMemoryManager), method, classID);
    assert(staticObject != NULL);

    /* Fetch the field informations	*/
    PDEBUG("CILIR : translate_cil_ldsfld : Fetch the stati field offset\n");
    field_offset = (system->garbage_collectors).gc.fetchStaticFieldOffset(fieldLocated);

    TypeDescriptor *resolved_type = fieldLocated->getType(fieldLocated);

    /* Create the IR instruction	*/
    instruction = method->newIRInstr(method);
    instruction->type = IRLOADREL;
    (instruction->param_1).value.v = (JITNUINT) staticObject;
    (instruction->param_1).type = IRSYMBOL;
    (instruction->param_1).internal_type = IRMPOINTER;
    (instruction->param_1).type_infos = classID;
    (instruction->param_2).value.v = field_offset;
    (instruction->param_2).type = IRINT32;
    (instruction->param_2).internal_type = IRINT32;
    (instruction->result).value.v = method->incMaxVariablesReturnOld(method);
    (instruction->result).type = IROFFSET;
    (instruction->result).internal_type = resolved_type->getIRType(resolved_type);
    (instruction->result).type_infos = resolved_type;
    instruction->byte_offset = bytes_read;

    /* Clean the top of the stack           */
    stack->cleanTop(stack);

    /* Push the result                       */
    stack->push(stack, &(instruction->result));

    /* Convert the result to a stackable type */
    JITINT32 toType;
    toType = ((system->cliManager).metadataManager->generic_type_rules)->coerce_CLI_Type(stack->stack[stack->top - 1].internal_type);
    if (stack->stack[stack->top - 1].internal_type != toType) {
        _perform_conversion(&(system->cliManager), method, bytes_read, stack, toType, JITFALSE, NULL);
    }

    return 0;
}

static inline JITINT16 translate_cil_ldarga (t_system *system, Method method, JITUINT32 bytes_read, JITUINT16 num, t_stack *stack) {
    ir_instruction_t        *instruction;

    /* Assertions		*/
    assert(method != NULL);
    assert(stack != NULL);
    assert(method->getParametersNumber(method) > 0);

    /* Clean the top of the stack           */
    stack->cleanTop(stack);

    /* Load the argument		*/
    memcpy(&(stack->stack[stack->top]), &(stack->stack[num]), sizeof(ir_item_t));
    (stack->top)++;

    /* Create the IRGETADDRESS of the argument	*/
    instruction = method->newIRInstr(method);
    instruction->type = IRGETADDRESS;
    (stack->top)--;
    memcpy(&(instruction->param_1), &(stack->stack[stack->top]), sizeof(ir_item_t));

    /* Clean the top of the stack           */
    stack->cleanTop(stack);

    stack->stack[stack->top].value.v = method->incMaxVariablesReturnOld(method);
    stack->stack[stack->top].type = IROFFSET;
    stack->stack[stack->top].internal_type = IRMPOINTER;
    if ((instruction->param_1).type_infos != NULL) {
        stack->stack[stack->top].type_infos = (instruction->param_1).type_infos->makePointerDescriptor((instruction->param_1).type_infos);
    }
    memcpy(&(instruction->result), &(stack->stack[stack->top]), sizeof(ir_item_t));
    (stack->top)++;
    instruction->byte_offset = bytes_read;
    assert((instruction->param_1).internal_type != NOPARAM);
    PDEBUG("CILIR:			Insert a IR instruction\n");

    return 0;
}

static inline JITINT16 translate_cil_ldloca (t_system *system, Method method, JITUINT32 bytes_read, JITUINT16 num, t_stack *stack, JITBOOLEAN localsInit, JITUINT32 *current_label_ID, XanList *labels) {
    ir_instruction_t        *instruction;
    JITUINT32 parametersNumber;

    /* Assertions		*/
    assert(method != NULL);
    assert(stack != NULL);
    assert(method->getLocalsNumber(method) > 0);
    assert(current_label_ID != NULL);
    assert(labels != NULL);
    assert(num < method->getLocalsNumber(method));

    //BEGIN TODO UNCOMMENT ONLY IF THE System.VerificationException TYPE IS IMPLEMENTED IN PNETLIB
    /* verify if the flag localsinit is set for current method */
    /*	if (localsInit == 0)
            {
                    _translate_throw_CIL_Exception(&(system->cliManager), method, current_label_ID,
                                    labels, bytes_read, stack,
                                    , (system->exception_system)._System_VerificationException_ID);

                    return 1;
       } */
    //END TODO

    /* Fetch the number of parameters of the*
     * method				*/
    parametersNumber = method->getParametersNumber(method);

    /* Clean the top of the stack           */
    stack->cleanTop(stack);

    /* Load the argument			*/
    assert(stack->top < stack->max_stack_size);
    assert((num + parametersNumber) < stack->max_stack_size);
    memcpy(&(stack->stack[stack->top]), &(stack->stack[num + parametersNumber]), sizeof(ir_item_t));
    (stack->top)++;

    /* Get the address of the argument	*/
    instruction = method->newIRInstr(method);
    instruction->type = IRGETADDRESS;
    (stack->top)--;
    memcpy(&(instruction->param_1), &(stack->stack[stack->top]), sizeof(ir_item_t));

    /* Clean the top of the stack           */
    stack->cleanTop(stack);

    stack->stack[stack->top].value.v = method->incMaxVariablesReturnOld(method);
    stack->stack[stack->top].type = IROFFSET;
    stack->stack[stack->top].internal_type = IRMPOINTER;
    if ((instruction->param_1).type_infos != NULL) {
        stack->stack[stack->top].type_infos = (instruction->param_1).type_infos->makePointerDescriptor((instruction->param_1).type_infos);
    }
    memcpy(&(instruction->result), &(stack->stack[stack->top]), sizeof(ir_item_t));
    (stack->top)++;
    instruction->byte_offset = bytes_read;

    /* Return				*/
    return 0;
}

static inline JITINT16 translate_cil_initobj (t_system *system, CILStack cilStack, GenericContainer *container, t_binary_information *current_binary, Method method, JITUINT32 bytes_read, JITUINT32 token, t_stack *stack, JITUINT32 *current_label_ID, XanList *labels) {
    TypeDescriptor          *type_located;
    ir_instruction_t        *instruction;
    ILLayout                *layout_infos;

    /* Assertions		*/
    assert(method != NULL);
    assert(cilStack != NULL);
    assert(stack != NULL);
    assert(system != NULL);
    assert(current_binary != NULL);
    assert(current_label_ID != NULL);
    assert(labels != NULL);
    assert( ((stack->stack[stack->top - 1]).internal_type    == IRNINT)
            || ((stack->stack[stack->top - 1]).internal_type == IRMPOINTER)
            || ((stack->stack[stack->top - 1]).internal_type == IROBJECT)
            || ((stack->stack[stack->top - 1]).internal_type == IRNINT));

    /* thest if the token is valid */
    if (translate_Test_Type_Load_with_Type_Token(system, method, cilStack, current_label_ID, labels, bytes_read, stack, current_binary, token) == 1) {
        PDEBUG("CILIR: initobj: EXITING type load error. invalid type-token detected \n");
        return 1;
    }

    /* retrieve the type addressed by the metadata token */
    type_located = (system->cliManager).metadataManager->getTypeDescriptorFromToken(system->cliManager.metadataManager, container, current_binary, token);
    assert(type_located != NULL);

    /* preconditions */
    assert(method->getIRMethod(method) != NULL);

    if (type_located->isValueType(type_located)) {
        assert((system->cliManager).layoutManager.layoutType != NULL);

        /* initialize the local variables       */
        instruction = NULL;

        /* retrieve the layout informations about the value type */
        layout_infos = (system->cliManager).layoutManager.layoutType(&((system->cliManager).layoutManager), type_located);
        assert(layout_infos != NULL);

        /* Make the IR instruction		*/
        instruction = method->newIRInstr(method);
        assert(instruction != NULL);
        instruction->type = IRINITMEMORY;
        memcpy(&(instruction->param_1), &(stack->stack[stack->top - 1]), sizeof(ir_item_t));
        (instruction->param_2).value.v = 0;
        (instruction->param_2).type = IRINT32;
        (instruction->param_2).internal_type = IRINT32;
        (instruction->param_3).value.v = layout_infos->typeSize;
        (instruction->param_3).type = IRINT32;
        (instruction->param_3).internal_type = IRINT32;

        PDEBUG("CILIR: translate_cil_initobj: Insert the IRINITMEMORY instruction inside the IR method\n");

        /* postconditions */
        (stack->top)--;
    } else {
        translate_cil_ldnull(&(system->cliManager), method, bytes_read, stack);
        translate_cil_stind(&(system->cliManager), method, cilStack, bytes_read, stack, current_label_ID, labels, IROBJECT);
    }
    return 0;
}

static inline JITINT16 translate_cil_newobj (t_system *system, CILStack cilStack, GenericContainer *container, t_binary_information *current_binary, Method method, JITUINT32 bytes_read, JITUINT32 token, t_stack *stack, JITUINT32 *current_label_ID, XanList *labels) {
    ActualMethodDescriptor tempMethod;
    TypeDescriptor  *typeFound;
    JITBOOLEAN isValuetype;
    ir_item_t string;
    JITUINT32 coerced_type;
#ifdef  DEBUG
    JITUINT32 stack_size;
#endif

    /* Assertions		*/
    assert(method != NULL);
    assert(method->getIRMethod(method) != NULL);
    assert(stack != NULL);
    assert(cilStack != NULL);
    assert(system != NULL);
    assert(current_binary != NULL);
    assert(internal_isStackCorrect(&(system->cliManager), method, stack));

#ifdef  DEBUG
    stack_size = stack->top;
#endif
    if (translate_Test_Missing_Method(system, method, cilStack, current_label_ID, labels, bytes_read, stack, current_binary, token) == 1) {
        PDEBUG("CILIR : cil_newobj : EXITING : missing method-token FOUND \n");
        return 1;
    }
#ifdef  DEBUG
    assert(stack->top == stack_size);
#endif

    /* Fetch the type information	*/
    PDEBUG("CILIR: NEWOBJ: Create the tempMethod to fetch informations about the class\n");
    tempMethod      = (system->cliManager).metadataManager->getMethodDescriptorFromToken(system->cliManager.metadataManager, container, current_binary, token);

    /* Fetch the binary where the   *
     * type is stored		*/
    if (translate_Test_Invalid_Operation(system, method, cilStack, current_label_ID, labels, bytes_read, stack, tempMethod.method) == 1) {
        PDEBUG("CILIR: cil_newobj: Exiting with an invalid operation found \n");
        return 1;
    }

    /* Create the IRNEWOBJ instruction	*/
#ifdef PRINTDEBUG
    PDEBUG("CILIR: cil_newobj: Create the obj\n");
    PDEBUG("CILIR: cil_newobj: Print the stack before entering in make_irnewobj \n");
    print_stack(stack, method->getParametersNumber(method), method->getLocalsNumber(method));
#endif

    /* retrieve the metadata informations about the ILType of the element */
    typeFound = (tempMethod.method)->owner;
    assert(typeFound != NULL);

    /* test if the type just found is a valueType */
    isValuetype	= internal_isValueType(typeFound);
    PDEBUG("CILIR: cil_newobj: isValueType ? %d \n", isValuetype);

    //Get the ILType of the "String" class.
    TypeDescriptor *stringIlType = (system->cliManager).CLR.stringManager.fillILStringType();
    memset(&string, 0, sizeof(ir_item_t));
    string.type = NOPARAM;

    if (typeFound == stringIlType) {
        ir_instruction_t *inst;

        //Allocate some space for the string variable
        string.value.v = method->incMaxVariablesReturnOld(method);
        string.type = IROFFSET;
        string.internal_type = IROBJECT;
        string.type_infos = stringIlType;

        //Get the address of the string variable and put it on top of the stack
        stack->cleanTop(stack);

        inst = method->newIRInstr(method);
        inst->type = IRGETADDRESS;
        memcpy(&(inst->param_1), &(string), sizeof(ir_item_t));

        stack->stack[stack->top].value.v = method->incMaxVariablesReturnOld(method);
        stack->stack[stack->top].type = IROFFSET;
        stack->stack[stack->top].internal_type = IRMPOINTER;
        if ((inst->param_1).type_infos != NULL) {
            stack->stack[stack->top].type_infos = (inst->param_1).type_infos->makePointerDescriptor((inst->param_1).type_infos);
        }
        memcpy(&(inst->result), &(stack->stack[stack->top]), sizeof(ir_item_t));

        (stack->top)++;
        inst->byte_offset = bytes_read;

    } else {
        if (!isValuetype) {
            make_irnewobj(&(system->cliManager), method, cilStack, bytes_read, typeFound, 0, stack, current_label_ID, labels, NULL);

            /* Print the stack			*/
#ifdef PRINTDEBUG
            PDEBUG("CILIR: cil_newobj: Print the stack after entering in make_irnewobj \n");
            print_stack(stack, method->getParametersNumber(method), method->getLocalsNumber(method));
#endif
        } else {
            make_irnewValueType(&(system->cliManager), method, bytes_read, typeFound, stack, NULL);
        }
    }

    /* Add the call to the .ctor method	*/
    PDEBUG("CILIR: cil_newobj: Create the IR call instruction to the .ctor method\n");
    translate_cil_call(system, cilStack, container, current_binary, method, token, bytes_read, stack, 0, current_label_ID, labels, 1);

    if (typeFound == stringIlType) {
        assert(string.type != NOPARAM);

        /* Put the resulting string on the stack */
        memcpy(&(stack->stack[stack->top - 1]), &(string), sizeof(ir_item_t));

    } else if (isValuetype) {
        ir_instruction_t        *instruction;

        /* Print the stack			*/
#ifdef PRINTDEBUG
        PDEBUG("CILIR: cil_newobj: Print the stack before entering in ir_load_rel \n");
        print_stack(stack, method->getParametersNumber(method), method->getLocalsNumber(method));
#endif

        (stack->top)--;
        instruction = method->newIRInstr(method);
        instruction->type = IRLOADREL;
        memcpy(&(instruction->param_1), &(stack->stack[stack->top]), sizeof(ir_item_t));
        (instruction->param_2).value.v = 0;
        (instruction->param_2).type = IRINT32;
        (instruction->param_2).internal_type = IRINT32;
        stack->stack[stack->top].value.v = method->incMaxVariablesReturnOld(method);
        stack->stack[stack->top].type = IROFFSET;
        stack->stack[stack->top].internal_type = typeFound->getIRType(typeFound);
        stack->stack[stack->top].type_infos	= typeFound;
        (instruction->result).value.v = stack->stack[stack->top].value.v;
        (instruction->result).type = stack->stack[stack->top].type;
        (instruction->result).internal_type = stack->stack[stack->top].internal_type;
        make_ir_infos(&(instruction->result), &(stack->stack[stack->top]));
        instruction->byte_offset = bytes_read;
        (stack->top)++;

        /* Print the stack			*/
#ifdef PRINTDEBUG
        PDEBUG("CILIR: cil_newobj: Print the stack once translated the ir_load_rel instruction \n");
        print_stack(stack, method->getParametersNumber(method), method->getLocalsNumber(method));
#endif
    }

    /* convert - if necessary - the value to a CLI stackable element type */
    coerced_type = ((system->cliManager).metadataManager->generic_type_rules)->coerce_CLI_Type(stack->stack[stack->top - 1].internal_type);
    if (stack->stack[stack->top - 1].internal_type != coerced_type) {
        translate_cil_conv(&(system->cliManager), method, bytes_read, stack, coerced_type, JITFALSE, JITFALSE, JITTRUE);
    }
    assert(internal_isStackCorrect(&(system->cliManager), method, stack));

    return 0;
}

static inline JITINT16 translate_cil_unbox (t_system *system, CILStack cilStack, GenericContainer *container, t_binary_information *current_binary, Method method, JITUINT32 bytes_read, JITUINT32 token, t_stack *stack, JITUINT32 *current_label_ID, XanList *labels) {
    TypeDescriptor          *valueType;
    ir_item_t newVar;

    /* Assertions		*/
    assert(method != NULL);
    assert(stack != NULL);
    assert(system != NULL);
    assert(current_binary != NULL);
    assert(cilStack != NULL);

    PDEBUG("CILIR: UNBOX: Retrieve the boxed valueType\n");

    /* thest if the token is valid */
    if (translate_Test_Type_Load_with_Type_Token(system, method, cilStack, current_label_ID, labels, bytes_read, stack, current_binary, token) == 1) {
        PDEBUG("CILIR: UNBOX: EXITING type load error. invalid type-token detected \n");
        return 1;
    }

    /* Fetch the type informations	from the metadata addressed by the token */
    valueType = (system->cliManager).metadataManager->getTypeDescriptorFromToken(system->cliManager.metadataManager, container, current_binary, token);

    /* evaluate the post-conditions of the fetching process */
    assert(valueType != NULL);

    /* test if the type identified by the token is a subtype of System.ValueType	*/
    if (!valueType->isValueType(valueType)) {

        /* the unbox instruction must raise a System.InvalidCastException */
        _translate_throw_CIL_Exception(&(system->cliManager), method, cilStack, current_label_ID, labels, bytes_read, stack, (system->exception_system)._System_InvalidCastException_ID);

        return 1;
    }

    translate_Test_Cast_Class(system, method, cilStack, current_label_ID, labels, bytes_read, stack, container, current_binary, token, 1);

    /* Make the IR instruction	*/
    memset(&newVar, 0, sizeof(ir_item_t));
    newVar.value.v = method->incMaxVariablesReturnOld(method);
    newVar.type = IROFFSET;
    newVar.internal_type = stack->stack[stack->top - 1].internal_type;
    stack->push(stack, &newVar);
    translate_cil_move( method, bytes_read, stack);

#ifdef PRINTDEBUG
    PDEBUG("CILIR: cil_unbox: Print the stack after the IRMOVE -- IRMPOINTER on the top\n");
    print_stack(stack, method->getParametersNumber(method), method->getLocalsNumber(method));
#endif

    return 0;
}

static inline JITINT16 translate_cil_unbox_any (t_system *system, CILStack cilStack, GenericContainer *container, t_binary_information *current_binary, Method method, JITUINT32 bytes_read, JITUINT32 token, t_stack *stack, JITUINT32 *current_label_ID, XanList *labels) {
    TypeDescriptor          *type;

    /* Assertions		*/
    assert(method != NULL);
    assert(stack != NULL);
    assert(system != NULL);
    assert(current_binary != NULL);
    assert(cilStack != NULL);

    PDEBUG("CILIR: UNBOX.ANY: Retrieve the boxed\n");

    /* thest if the token is valid */
    if (translate_Test_Type_Load_with_Type_Token(system, method, cilStack, current_label_ID, labels, bytes_read, stack, current_binary, token) == 1) {
        PDEBUG("CILIR: UNBOX.ANY: EXITING type load error. invalid type-token detected \n");
        return 1;
    }

    /* Fetch the type informations	from the metadata addressed by the token */
    type = (system->cliManager).metadataManager->getTypeDescriptorFromToken(system->cliManager.metadataManager, container, current_binary, token);

    /* evaluate the post-conditions of the fetching process */
    assert(type != NULL);

    if (type->isValueType(type)) {
        if (translate_cil_unbox(system, cilStack, container, current_binary, method, bytes_read, token, stack, current_label_ID, labels) == 1) {
            return 1;
        }
        if (translate_cil_ldobj(method, cilStack, token, bytes_read, stack, container, current_binary, system, current_label_ID, labels) == 1) {
            return 1;
        }
    } else if (translate_Test_Cast_Class(system, method, cilStack, current_label_ID, labels, bytes_read, stack, container, current_binary, token, 1) == 1) {
        return 1;
    }

    return 0;
}

static inline JITINT16 translate_cil_box (t_system *system, CILStack cilStack, GenericContainer *container, t_binary_information *current_binary, Method method, JITUINT32 bytes_read, JITUINT32 token, t_stack *stack, JITUINT32 *current_label_ID, XanList *labels) {
    TypeDescriptor  *valueType;
    ir_item_t value_on_the_stack;
    ir_instruction_t        *instruction;
    ILLayout                *layoutInfos;

    /* Assertions		*/
    assert(method != NULL);
    assert(stack != NULL);
    assert(cilStack != NULL);
    assert(system != NULL);
    assert(current_binary != NULL);


    /* Create the IRNEWOBJ instruction	*/
    PDEBUG("CILIR: BOX: Create the box object\n");

    /* thest if the token is valid */
    if (translate_Test_Type_Load_with_Type_Token(system, method, cilStack, current_label_ID, labels, bytes_read, stack, current_binary, token) == 1) {
        PDEBUG("CILIR: BOX: EXITING type load error. invalid type-token detected \n");
        return 1;
    }

    /* Fetch the type informations	from the metadata addressed by the token */
    valueType = (system->cliManager).metadataManager->getTypeDescriptorFromToken(system->cliManager.metadataManager, container, current_binary, token);


    /* evaluate the post-conditions of the fetching process */
    assert(valueType != NULL);

    /* test if the type identified by the token is a subtype of System.ValueType	*/
    if (!valueType->isValueType(valueType)) {

        /* the box instruction does nothing. See ECMA specification */
        return 0;
    }

    //NOTE SUPPORT TO System.Nullable<T> types not yet implemented by pnetlib!

#ifdef PRINTDEBUG
    PDEBUG("CILIR: cil_box: Print the stack before executing the instruction \n");
    print_stack(stack, method->getParametersNumber(method), method->getLocalsNumber(method));
#endif

    /* pop the value-type from the stack */
    (stack->top)--;

    /* make a copy of the value on the stack */
#ifdef DEBUG
    if (    ((stack->stack[stack->top]).internal_type == IRVALUETYPE)       ||
            ((stack->stack[stack->top]).internal_type == IRVALUETYPE)       ) {
        assert((stack->stack[stack->top]).type_infos != NULL);
    }

#endif
    value_on_the_stack.value.v = stack->stack[stack->top].value.v;
    value_on_the_stack.value.f = stack->stack[stack->top].value.f;
    value_on_the_stack.internal_type = stack->stack[stack->top].internal_type;
    make_ir_infos(&value_on_the_stack, &(stack->stack[stack->top]));
    value_on_the_stack.type = stack->stack[stack->top].type;
#ifdef DEBUG
    if (value_on_the_stack.internal_type == IRVALUETYPE) {
        assert(value_on_the_stack.type_infos != NULL);
    }
#endif

    /* create the new object */
    make_irnewobj(&(system->cliManager), method, cilStack, bytes_read, valueType, 0, stack, current_label_ID, labels, NULL);

#ifdef PRINTDEBUG
    PDEBUG("CILIR: cil_box: Print the stack once created the new object \n");
    print_stack(stack, method->getParametersNumber(method), method->getLocalsNumber(method));
#endif

    /* retrieve the layout informations of the value on the stack */
    assert((system->cliManager).layoutManager.layoutType != NULL);
    assert(valueType != NULL);

    layoutInfos = (system->cliManager).layoutManager.layoutType(&((system->cliManager).layoutManager), valueType);
    assert(layoutInfos != NULL);

    PDEBUG("BOXATO! -- recuperate le info di layout \n");

    if (value_on_the_stack.type == IROFFSET) {

        /* load the address of the valueType stored into value_on_the_stack */
        instruction = method->newIRInstr(method);
        instruction->type = IRGETADDRESS;
        memcpy(&(instruction->param_1), &value_on_the_stack, sizeof(ir_item_t));

        /* Clean the top of the stack           */
        stack->cleanTop(stack);

        /* Push the address			*/
        stack->stack[stack->top].value.v = method->incMaxVariablesReturnOld(method);
        stack->stack[stack->top].type = IROFFSET;
        stack->stack[stack->top].internal_type = IRMPOINTER;
        if ((instruction->param_1).type_infos != NULL) {
            stack->stack[stack->top].type_infos = (instruction->param_1).type_infos->makePointerDescriptor((instruction->param_1).type_infos);
        }

        memcpy(&(instruction->result), &(stack->stack[stack->top]), sizeof(ir_item_t));
        (stack->top)++;
        instruction->byte_offset = bytes_read;
        PDEBUG("CILIR:			Insert a IRGETADDRESS instruction\n");

#ifdef PRINTDEBUG
        PDEBUG("CILIR: cil_box: Print the stack after the IRGETADDRESS \n");
        print_stack(stack, method->getParametersNumber(method), method->getLocalsNumber(method));
#endif

        /* reload the values on the stack */
        stack->stack[stack->top].value.v = stack->stack[stack->top - 2].value.v;
        stack->stack[stack->top].value.f = stack->stack[stack->top - 2].value.f;
        stack->stack[stack->top].type = stack->stack[stack->top - 2].type;
        stack->stack[stack->top].internal_type = stack->stack[stack->top - 2].internal_type;
        make_ir_infos(&(stack->stack[stack->top]), &(stack->stack[stack->top - 2]));
        (stack->top)++;
        stack->stack[stack->top].value.v = stack->stack[stack->top - 2].value.v;
        stack->stack[stack->top].value.f = stack->stack[stack->top - 2].value.f;
        stack->stack[stack->top].type = stack->stack[stack->top - 2].type;
        stack->stack[stack->top].internal_type = stack->stack[stack->top - 2].internal_type;
        make_ir_infos(&(stack->stack[stack->top]), &(stack->stack[stack->top - 2]));
        (stack->top)++;

        /* create an IRMEMCPY instruction */
        make_irmemcpy(method, cilStack, bytes_read, stack, current_label_ID, labels, layoutInfos->typeSize, UNALIGNED_DONOTCHECK_ALIGNMENT, NULL);

#ifdef PRINTDEBUG
        PDEBUG("CILIR: cil_box: Print the stack after the IRMEMCPY -- still OBJECT & MPOINTER \n");
        print_stack(stack, method->getParametersNumber(method), method->getLocalsNumber(method));
#endif

        /* pop the value from the stack */
        (stack->top)--;

#ifdef PRINTDEBUG
        PDEBUG("CILIR: cil_box: Print the stack after the IRMEMCPY --- ONLY OBJECT \n");
        print_stack(stack, method->getParametersNumber(method), method->getLocalsNumber(method));
#endif
    } else {
#ifdef PRINTDEBUG
        PDEBUG("CILIR: cil_box: OBJ \n");
        print_stack(stack, method->getParametersNumber(method), method->getLocalsNumber(method));
#endif
        assert(value_on_the_stack.internal_type != IRVALUETYPE);

        /* Make the IR instruction		*/
        instruction = method->newIRInstr(method);
        assert(instruction != NULL);
        instruction->type = IRSTOREREL;
        memcpy(&(instruction->param_1), &(stack->stack[stack->top - 1]), sizeof(ir_item_t));
        assert((instruction->param_1).internal_type == IROBJECT);
        (instruction->param_2).value.v = 0;
        (instruction->param_2).type = IRINT32;
        (instruction->param_2).internal_type = IRINT32;
        (instruction->param_2).type_infos = NULL;
        memcpy(&(instruction->param_3), &value_on_the_stack, sizeof(ir_item_t));

        PDEBUG("CILIR: translate_cil_box: Insert the IRINITMEMORY instruction inside the IR method\n");
    }

    /* Print the stack			*/
#ifdef PRINTDEBUG
    PDEBUG("CILIR: cil_box: Print the stack after the IRINITMEMORY --- ONLY OBJECT \n");
    print_stack(stack, method->getParametersNumber(method), method->getLocalsNumber(method));
#endif

    /* Return				*/
    return 0;
}

/**
 * @brief Create and initialize a new ValueType
 *
 * @param before if it is not NULL, the IR instructions to create the valuetype will be inserted before the specified instruction.
 *               If it is NULL, they will be put at the end of the current method.
 */
static inline JITINT16 make_irnewValueType (CLIManager_t *cliManager, Method method, JITUINT32 bytes_read, TypeDescriptor *value_type, t_stack *stack, ir_instruction_t *before) {
    ir_instruction_t        *instruction;
    ILLayout                *layoutInfos;

    /* Assertions				*/
    assert(cliManager != NULL);
    assert(method != NULL);
    assert(stack != NULL);
    assert(value_type != NULL);
    assert(method->getIRMethod(method) != NULL);

    /* initialize the local variables       */
    instruction = NULL;

    /* retrieve the layout informations for the valuetype */
    layoutInfos = (cliManager->layoutManager).layoutType(&(cliManager->layoutManager), value_type);
    assert(layoutInfos != NULL);

    /* Clean the top of the stack           */
    stack->cleanTop(stack);

    /* Make the IR instruction		*/
    if (before == NULL) {
        /* Append the new instruction at the end of the method */
        instruction = method->newIRInstr(method);
    } else {
        /* Insert the new instruction before the specified one */
        instruction = method->newIRInstrBefore(method, before);
    }

    assert(instruction != NULL);
    instruction->type = IRGETADDRESS;
    (instruction->param_1).value.v = method->incMaxVariablesReturnOld(method);
    (instruction->param_1).type = IROFFSET;
    (instruction->param_1).internal_type = value_type->getIRType(value_type);
    (instruction->param_1).type_infos = layoutInfos->type;
    stack->stack[stack->top].value.v = method->incMaxVariablesReturnOld(method);
    stack->stack[stack->top].type = IROFFSET;
    stack->stack[stack->top].internal_type = IROBJECT;
    stack->stack[stack->top].type_infos = layoutInfos->type;
    instruction->byte_offset = bytes_read;
    memcpy(&(instruction->result), &(stack->stack[stack->top]), sizeof(ir_item_t));
    assert((instruction->result).type == IROFFSET);
    (stack->top)++;

    if (before == NULL) {

        /* Append the new instruction at the end of the method */
        instruction = method->newIRInstr(method);
    } else {

        /* Insert the new instruction before the specified one */
        instruction = method->newIRInstrBefore(method, before);
    }
    assert(instruction != NULL);
    instruction->type = IRINITMEMORY;
    memcpy(&(instruction->param_1), &(stack->stack[stack->top - 1]), sizeof(ir_item_t));
    (instruction->param_2).value.v = 0;
    (instruction->param_2).type = IRINT32;
    (instruction->param_2).internal_type = IRINT32;
    (instruction->param_3).value.v = layoutInfos->typeSize;
    (instruction->param_3).type = IRINT32;
    (instruction->param_3).internal_type = IRINT32;
    instruction->byte_offset = bytes_read;

    /* Return				*/
    return 0;
}

/**
 * @brief Create an instruction to copy a value from the address on stack->top-1 to the address on stack->top-2
 *
 * @param before if it is not NULL, the IR instruction to perform the memcpy will be inserted before the specified instruction.
 *               If it is NULL, it will be put at the end of the current method.
 */
static inline JITINT16 make_irmemcpy (Method method, CILStack cilStack, JITUINT32 bytes_read, t_stack *stack, JITUINT32 *current_label_ID, XanList *labels, JITINT32 memsize, JITUINT8 alignment, ir_instruction_t *before) {
    ir_instruction_t        *instruction;

    /* Assertions				*/
    assert(method != NULL);
    assert(stack != NULL);
    assert(method->getIRMethod(method) != NULL);
    assert(current_label_ID != NULL);
    assert(labels != NULL);
    assert(memsize >= 0);

#if 0
    /* Checking the aliognment of the parameters if not disabled */
    if (alignment != UNALIGNED_DONOTCHECK_ALIGNMENT) {
        /* Test if the first address has a correct alignment  */
        (stack->stack[(stack->top)]).value.v = (stack->stack[(stack->top) - 1]).value.v;
        (stack->stack[(stack->top)]).value.f = (stack->stack[(stack->top) - 1]).value.f;
        (stack->stack[(stack->top)]).type = (stack->stack[(stack->top) - 1]).type;
        (stack->stack[(stack->top)]).internal_type = (stack->stack[(stack->top) - 1]).internal_type;
        (stack->top)++;
        /* Test if the target address in which we should store the value is correctly aligned */
        switch (alignment) {
            case UNALIGNED_DEFAULT_ALIGNMENT:
                translate_Test_Valid_Value_Address(system, method, cilStack, current_label_ID, labels, bytes_read, stack, IRNUINT);
                break;
            case UNALIGNED_BYTE_ALIGNMENT:
                translate_Test_Valid_Value_Address(system, method, cilStack, current_label_ID, labels, bytes_read, stack, IRINT8);
                break;
            case UNALIGNED_DOUBLEBYTE_ALIGNMENT:
                translate_Test_Valid_Value_Address(system, method, cilStack, current_label_ID, labels, bytes_read, stack, IRINT16);
                break;
            case UNALIGNED_QUADBYTE_ALIGNMENT:
                translate_Test_Valid_Value_Address(system, method, cilStack, current_label_ID, labels, bytes_read, stack, IRINT32);
                break;
            default:
                print_err("CILIR: ERROR = invalid alignment for first address of irmemcpy. ", 0);
                pthread_mutex_unlock(&((binary->binary).mutex));
                abort();
        }
        /* Pop the reloaded address from the stack */
        (stack->top)--;
        /* Test if the second address has a correct alignment  */
        (stack->stack[(stack->top)]).value.v = (stack->stack[(stack->top) - 2]).value.v;
        (stack->stack[(stack->top)]).value.f = (stack->stack[(stack->top) - 2]).value.f;
        (stack->stack[(stack->top)]).type = (stack->stack[(stack->top) - 2]).type;
        (stack->stack[(stack->top)]).internal_type = (stack->stack[(stack->top) - 2]).internal_type;
        (stack->top)++;
        /* Test if the target address in which we should store the value is correctly aligned */
        switch (alignment) {
            case UNALIGNED_DEFAULT_ALIGNMENT:
                translate_Test_Valid_Value_Address(system, method, cilStack, current_label_ID, labels, bytes_read, stack, IRNUINT);
                break;
            case UNALIGNED_BYTE_ALIGNMENT:
                translate_Test_Valid_Value_Address(system, method, cilStack, current_label_ID, labels, bytes_read, stack, IRINT8);
                break;
            case UNALIGNED_DOUBLEBYTE_ALIGNMENT:
                translate_Test_Valid_Value_Address(system, method, cilStack, current_label_ID, labels, bytes_read, stack, IRINT16);
                break;
            case UNALIGNED_QUADBYTE_ALIGNMENT:
                translate_Test_Valid_Value_Address(system, method, cilStack, current_label_ID, labels, bytes_read, stack, IRINT32);
                break;
            default:
                print_err("CILIR: ERROR = invalid alignment for second address of irmemcpy. ", 0);
                pthread_mutex_unlock(&((binary->binary).mutex));
                abort();
        }
        /* Pop the reloaded address from the stack */
        (stack->top)--;
    }
#endif
    /* Call the IRMEMCPY instruction in order to set the memory area of the object */
    /* Insert the new instruction before the specified one (if any) */
    instruction = method->newIRInstrBefore(method, before);
    instruction->type = IRMEMCPY;
    (stack->top)--;
    (instruction->param_2).value.v = stack->stack[stack->top].value.v;
    (instruction->param_2).type = stack->stack[stack->top].type;
    (instruction->param_2).internal_type = stack->stack[stack->top].internal_type;
    make_ir_infos(&(instruction->param_2), &(stack->stack[stack->top]));
    assert( ((instruction->param_2).internal_type == IRMPOINTER) || ((instruction->param_2).internal_type == IRTPOINTER) || ((instruction->param_2).internal_type == IROBJECT) || ((instruction->param_2).internal_type == IRNINT) );
    (stack->top)--;
    (instruction->param_1).value.v = stack->stack[stack->top].value.v;
    (instruction->param_1).type = stack->stack[stack->top].type;
    (instruction->param_1).internal_type = stack->stack[stack->top].internal_type;
    make_ir_infos(&(instruction->param_1), &(stack->stack[stack->top]));
    assert( ((instruction->param_1).internal_type == IRMPOINTER)
            || ((instruction->param_1).internal_type == IRTPOINTER)
            || ((instruction->param_1).internal_type == IROBJECT)
            || ((instruction->param_1).internal_type == IRNINT)
            || ((instruction->param_1).internal_type == IRNINT) );
    (instruction->param_3).value.v = memsize;
    (instruction->param_3).type = IRINT32;
    (instruction->param_3).internal_type = IRINT32;
    instruction->byte_offset = bytes_read;
    PDEBUG("CILIR:			Insert a IRMEMCPY instruction\n");

    return 0;
}

/**
 * @brief Create a new Object
 *
 * @param before if it is not NULL, the IR instructions to create the object will be inserted before the specified instruction.
 *               If it is NULL, they will be put at the end of the method.
 */
static inline JITINT16 make_irnewobj (CLIManager_t *cliManager, Method method, CILStack cilStack, JITUINT32 bytes_read, TypeDescriptor *classID, JITUINT32 overSize, t_stack *stack, JITUINT32 *current_label_ID, XanList *labels, ir_instruction_t *before) {
    ir_instruction_t        *instruction;

    /* Assertions				*/
    assert(method != NULL);
    assert(cilStack != NULL);
    assert(stack != NULL);
    assert(classID != NULL);
    assert(method->getIRMethod(method) != NULL);
    assert(labels != NULL);

    /* initialize the local variables       */
    instruction = NULL;

    /* Layout the type			*/
    (cliManager->layoutManager).layoutType(&(cliManager->layoutManager), classID);

    /* Clean the top of the stack           */
    stack->cleanTop(stack);

    /* Insert the new instruction before the specified one (if any) */
    instruction = method->newIRInstrBefore(method, before);
    instruction->type = IRNEWOBJ;
    (instruction->param_1).value.v = (JITNUINT) classID;
    (instruction->param_1).type = IRCLASSID;
    (instruction->param_1).internal_type = IRCLASSID;
    (instruction->param_2).value.v = overSize;
    (instruction->param_2).type = IRUINT32;
    (instruction->param_2).internal_type = IRUINT32;
    stack->stack[stack->top].value.v = method->incMaxVariablesReturnOld(method);
    stack->stack[stack->top].type = IROFFSET;
    stack->stack[stack->top].internal_type = IROBJECT;
    stack->stack[stack->top].type_infos = classID;
    memcpy(&(instruction->result), &(stack->stack[stack->top]), sizeof(ir_item_t));
    assert((instruction->result).type == IROFFSET);
    instruction->byte_offset = bytes_read;
    (stack->top)++;
    PDEBUG("CILIR: make_irnewobj: Insert the IRNEWOBJ instruction inside the IR method\n");

    /* Refresh the root set of the method	*/
    assert((instruction->result).type == IROFFSET);
    assert((instruction->result).internal_type == IROBJECT);
    method->addVariableToRootSet(method, (instruction->result).value.v);

    /* Return                                       */
    return 0;
}

static inline JITINT16 translate_cil_move (Method method, JITUINT32 bytes_read, t_stack *stack) {
    ir_instruction_t        *instruction;

    /* Assertions					*/
    assert(method != NULL);
    assert(stack != NULL);
    assert(stack->top > 1);
    assert(stack->top > (method->getParametersNumber(method) + method->getLocalsNumber(method)));
    assert((stack->stack[stack->top - 1]).type == IROFFSET);
    assert(method->getParametersNumber(method) >= 0);

    /* insert the move instruction               */
    instruction = method->newIRInstr(method);
    (stack->top)--;
    instruction->type = IRMOVE;
    memcpy(&(instruction->result), &(stack->stack[stack->top]), sizeof(ir_item_t));
    (stack->top)--;
    memcpy(&(instruction->param_1), &(stack->stack[stack->top]), sizeof(ir_item_t));
    instruction->byte_offset = bytes_read;
    assert((instruction->result).type == IROFFSET);
    assert((instruction->result).value.v < method->getMaxVariables(method));
    assert((instruction->result).internal_type == (instruction->param_1).internal_type);

    /* Reload the new variable on top of the stack	*/
    memcpy(&(stack->stack[stack->top]), &(instruction->result), sizeof(ir_item_t));
    (stack->top)++;

    /* Refresh the root set				*/
    if ((instruction->result).internal_type == IROBJECT) {
        method->addVariableToRootSet(method, (instruction->result).value.v);
    }
    assert((stack->stack[stack->top - 1]).type == IROFFSET);
    assert(stack->top >= (method->getParametersNumber(method) + method->getLocalsNumber(method)));

    /* Return					*/
    return 0;
}

static inline JITINT16 translate_cil_starg (CLIManager_t *cliManager, Method method, JITUINT32 bytes_read, JITUINT32 arg_num, t_stack *stack) {
    ir_item_t var;

    memset(&var, 0, sizeof(ir_item_t));
    /* Assertions					*/
    assert(method != NULL);
    assert(stack != NULL);
    assert(method->getParametersNumber(method) > 0);

    var.value.v = arg_num;
    var.type = IROFFSET;
    var.internal_type = (stack->stack[var.value.v]).internal_type;
    make_ir_infos(&var, &(stack->stack[var.value.v]));

    /* Perform the necessary conversions			*/
    if (stack->stack[stack->top - 1].internal_type != var.internal_type) {
        translate_cil_conv(cliManager, method, bytes_read, stack, var.internal_type, JITFALSE, JITFALSE, JITFALSE);
    }

    /* Store the top of the stack 				*/
    stack->push(stack, &var);
    translate_cil_move(method, bytes_read, stack);
    (stack->top)--;

    /* Return					*/
    return 0;
}

static inline JITINT16 translate_cil_ldarg (CLIManager_t *cliManager, Method method, JITUINT32 bytes_read, JITUINT32 arg_num, t_stack *stack) {
    char buf[DIM_BUF];
    ir_item_t var;
    JITINT32 toType;

    /* Assertions					*/
    assert(method != NULL);
    assert(stack != NULL);
    assert(cliManager != NULL);
    assert(method->getParametersNumber(method) > 0);
    PDEBUG("CILIR: translate_cil_ldarg: Start\n");

    /* Check if the argument number is correct	*/
    if (arg_num > method->getParametersNumber(method)) {
        snprintf(buf, sizeof(char) * 1024, "CILIR: translate_cil_ldarg: ERROR = The argument %u not exist. ", arg_num);
        print_err(buf, 0);
        abort();
    }
    assert(method->getIRMethod(method) != NULL);
    assert(((method->getIRMethod(method))->signature).parameterTypes != NULL);

    /* Push the variable to store			*/
    memcpy(&var, &(stack->stack[arg_num]), sizeof(ir_item_t));
    stack->push(stack, &var);

    /* Push the destination variable		*/
    var.value.v = method->incMaxVariablesReturnOld(method);
    stack->push(stack, &var);

    /* Insert the move instruction               	*/
    translate_cil_move( method, bytes_read, stack);

    /* verify that the internal_type is correct */
    assert((stack->stack[stack->top - 1]).internal_type != NOPARAM);
    if ((stack->stack[stack->top - 1]).internal_type == IRVALUETYPE) {

        /* precoditions */
        assert((stack->stack[stack->top - 1]).type_infos != NULL);
        return 0;
    }

    /* Convert the result to the appropriate stackable type */
    toType = (cliManager->metadataManager->generic_type_rules)->coerce_CLI_Type(stack->stack[stack->top - 1].internal_type);
    if (stack->stack[stack->top - 1].internal_type != toType) {
        _perform_conversion(cliManager, method, bytes_read, stack, toType, JITFALSE, NULL);
    }

    /* Return						*/
    PDEBUG("CILIR: translate_cil_ldarg: End\n");
    return 0;
}

static inline JITINT16 translate_cil_ldloc (t_system *system, Method method, JITUINT32 bytes_read, JITUINT32 loc_num, t_stack *stack) {
    JITUINT32 stack_position;
    JITINT32 toType;
    JITINT32 fromType;
    ir_item_t var;

    /* Assertions			*/
    assert(method != NULL);
    assert(stack != NULL);
    PDEBUG("CILIR: translate_cil_ldloc: Start\n");

    /* Check if the local exist	*/
    if (loc_num > method->getLocalsNumber(method)) {
        char buf[DIM_BUF];
        snprintf(buf, sizeof(char) * 1024, "CILIR: translate_cil_ldloc: ERROR = The locals %u not exist. ", loc_num);
        print_err(buf, 0);
        abort();
    }

    /* set the stack position value */
    stack_position = method->getParametersNumber(method) + loc_num;

    /* insert the IRLOAD instruction */
    memcpy(&var, &(stack->stack[stack_position]), sizeof(ir_item_t));
    stack->push(stack, &var);

    memcpy(&var, &(stack->stack[stack_position]), sizeof(ir_item_t));
    var.value.v = method->incMaxVariablesReturnOld(method);
    stack->push(stack, &var);
    translate_cil_move( method, bytes_read, stack);

    /* verify that the internal_type is correct */
    assert((stack->stack[stack->top - 1]).internal_type != NOPARAM);
    if ((stack->stack[stack->top - 1]).internal_type == IRVALUETYPE) {

        /* Return				*/
        assert((stack->stack[stack->top - 1]).type_infos != NULL);
        return 0;
    }

    /* The value that is on top of the stack will remain there after this opcode, so it has to be stackable */
    toType  = ((system->cliManager).metadataManager->generic_type_rules)->coerce_CLI_Type(stack->stack[stack->top - 1].internal_type);

    fromType        = stack->stack[stack->top - 1].internal_type;

    /* Perform the conversion			*/
    if (fromType != toType) {
        _perform_conversion(&(system->cliManager), method, bytes_read, stack, toType, JITFALSE, NULL);
    }

    /* Return				*/
    PDEBUG("CILIR: translate_cil_ldloc: End\n");
    return 0;
}

static inline JITINT16 translate_cil_stloc (CLIManager_t *cliManager, Method method, JITUINT32 bytes_read, JITUINT32 loc_num, t_stack *stack) {
    ir_item_t var;
    ir_item_t localVar;
    JITUINT32 varID;
    JITUINT32 valueType;

    /* Assertions			*/
    assert(method != NULL);
    assert(stack != NULL);
    PDEBUG("CILIR: translate_cil_stloc: Start\n");

    /* Print the stack		*/
#ifdef PRINTDEBUG
    PDEBUG("CILIR: translate_cil_stloc:     Stack before translating the instruction\n");
    print_stack(stack, method->getParametersNumber(method), method->getLocalsNumber(method));
#endif

    /* Compute the ID of the        *
     * variable where I will store	*
     * the value			*/
    varID = method->getParametersNumber(method) + loc_num;
#ifdef DEBUG
    assert((stack->stack[varID]).value.v == varID);
    assert((stack->stack[varID]).type == IROFFSET);
    assert((stack->stack[varID]).internal_type != NOPARAM);
    if ((stack->stack[varID]).internal_type == IRVALUETYPE) {
        assert((stack->stack[varID]).type_infos != NULL);
    }

#endif
    PDEBUG("CILIR: translate_cil_stloc:	Store the value into the variable %u\n", varID);

    /* Fetch the variable		*/
    memcpy(&localVar, &(stack->stack[varID]), sizeof(ir_item_t));

    /* Fetch the type				*/
    valueType = (stack->stack[stack->top - 1]).internal_type;
    assert(valueType != NOPARAM);

    /* Propagate the CIL type from the local to the	*
     * temporary					*/
    if (stack->stack[stack->top - 1].type_infos == NULL) {
        stack->stack[stack->top - 1].type_infos = stack->stack[varID].type_infos;
    }

    /* Delete aliases				*/
    switch (valueType) {
        case IRMPOINTER:
            valueType = IRNINT;
            break;
    }
    switch (localVar.internal_type) {
        case IRMPOINTER:
            localVar.internal_type = IRNINT;
            break;
    }

    if (    (valueType != localVar.internal_type)      &&
            (!(
                 (valueType == IRINT32) &&
                 (localVar.internal_type == IRVALUETYPE) &&
                 (localVar.type_infos)->isEnum(localVar.type_infos)
             ))) {

        /* explicitly convert value to a '(stack->stack[varID]).internal_type' value */
        translate_cil_conv(cliManager, method, bytes_read, stack, localVar.internal_type, 0, 0, 0);
        assert((stack->stack[(stack->top) - 1]).internal_type == localVar.internal_type);
    }

    /* Perform the necessary conversions			*/
    memcpy(&var, &(stack->stack[varID]), sizeof(ir_item_t));
    if (stack->stack[stack->top - 1].internal_type != var.internal_type) {
        translate_cil_conv(cliManager, method, bytes_read, stack, var.internal_type, JITFALSE, JITFALSE, JITFALSE);
    }

    /* Store the top of the stack to the local variable	*/
    stack->push(stack, &var);
    translate_cil_move(method, bytes_read, stack);
    (stack->top)--;

    /* Return			*/
    PDEBUG("CILIR: translate_cil_stloc: Exit\n");
    return 0;
}

static inline JITINT16 translate_cil_ldelema (t_system *system, CILStack cilStack, GenericContainer *container, t_binary_information *current_binary, Method method, JITUINT32 bytes_read, JITUINT32 token, t_stack *stack, JITUINT32 *current_label_ID, XanList *labels) {
    ir_instruction_t        *instruction;
    ir_instruction_t        *instruction2;
    ir_instruction_t        *instructionConv;
    ir_instruction_t	*instructionSizeOf;
    TypeDescriptor          *arrayType;
    CLIManager_t            *cliManager;
    ir_item_t		arraySlotType;

    /* Assertions				*/
    assert(system != NULL);
    assert(cilStack != NULL);
    assert(current_binary != NULL);
    assert(method != NULL);
    assert(stack != NULL);

    /* Initialize the variables		*/
    instruction = NULL;
    arrayType = NULL;

    /* Fetch the CLI manager		*/
    cliManager = &(system->cliManager);

    /* Fetch the array type informations	*/
    arrayType               = cliManager->metadataManager->getTypeDescriptorFromToken(cliManager->metadataManager, container, current_binary, token);
    assert(arrayType != NULL);

    /* Clean the top of the stack           */
    stack->cleanTop(stack);

    /* retrieve the array object from the stack */
    memcpy(&(stack->stack[(stack->top)]), &(stack->stack[(stack->top) - 2]), sizeof(ir_item_t));
    (stack->top)++;

    translate_Test_Null_Reference(cliManager, method, cilStack, bytes_read, stack);

    /* Clean the top of the stack           */
    stack->cleanTop(stack);

    /* retrieve the index value from the stack */
    memcpy(&(stack->stack[(stack->top)]), &(stack->stack[(stack->top) - 2]), sizeof(ir_item_t));
    (stack->top)++;

    translate_Test_Array_Index_Out_Of_Range(system, method, cilStack, current_label_ID, labels, bytes_read, stack);

    /* pop the elements used only for testing from the stack */
    (stack->top)--;

    /* test if the type mismatch the array type */
    translate_Test_Array_Type_Mismatch(system, method, cilStack, current_label_ID, labels, bytes_read, stack, arrayType);
    (stack->top)--;

    /* we have to check if the array is an array of valueTypes or not */
    memset(&arraySlotType, 0, sizeof(ir_item_t));
    arraySlotType.type		= IRTYPE;
    arraySlotType.internal_type	= arraySlotType.type;
    arraySlotType.type_infos	= arrayType;
    if (arrayType->isValueType(arrayType)) {

        /* before starting with the load instruction, we have to retrieve the
         * layout informations about the arrayType. These informations will be used
         * in order to discover if the array contains valuetypes or reference types */
        assert((system->cliManager).layoutManager.layoutType != NULL);
        (system->cliManager).layoutManager.layoutType(&((system->cliManager).layoutManager), arrayType);
        arraySlotType.value.v		= IRVALUETYPE;

    } else {
        arraySlotType.value.v		= IRMPOINTER;
    }

    /* Fetch the size of each array element	*/
    instructionSizeOf 				= method->newIRInstr(method);
    instructionSizeOf->type				= IRSIZEOF;
    memcpy(&(instructionSizeOf->param_1), &arraySlotType, sizeof(ir_item_t));
    (instructionSizeOf->result).value.v 		= method->incMaxVariablesReturnOld(method);
    (instructionSizeOf->result).type 		= IROFFSET;
    (instructionSizeOf->result).internal_type 	= IRINT32;

    /* Compute the offset			*/
    (stack->top)--;
    instruction = method->newIRInstr(method);
    instruction->type = IRMUL;
    memcpy(&(instruction->param_2), &(instructionSizeOf->result), sizeof(ir_item_t));
    memcpy(&(instruction->param_1), &(stack->stack[(stack->top)]), sizeof(ir_item_t));
    (instruction->result).value.v = method->incMaxVariablesReturnOld(method);
    (instruction->result).type = IROFFSET;
    (instruction->result).internal_type = IRINT32;
    assert((instruction->param_1).internal_type == (instruction->param_2).internal_type);

    /* Convert the offset to be a pointer	*/
    instructionConv = method->newIRInstr(method);
    instructionConv->type = IRCONV;
    memcpy(&(instructionConv->param_1), &(instruction->result), sizeof(ir_item_t));
    (instructionConv->param_2).value.v = stack->stack[stack->top - 1].internal_type;
    (instructionConv->param_2).type = IRTYPE;
    (instructionConv->param_2).internal_type = IRTYPE;
    (instructionConv->result).value.v = method->incMaxVariablesReturnOld(method);
    (instructionConv->result).type = IROFFSET;
    (instructionConv->result).internal_type = (instructionConv->param_2).value.v;
    assert((instructionConv->param_1).internal_type != IRVALUETYPE);

    /* Compute the address	*/
    (stack->top)--;
    instruction2 = method->newIRInstr(method);
    instruction2->type = IRADD;
    memcpy(&(instruction2->param_1), &(stack->stack[(stack->top)]), sizeof(ir_item_t));
    memcpy(&(instruction2->param_2), &(instructionConv->result), sizeof(ir_item_t));
    memcpy(&(instruction2->result), &(instruction2->param_1), sizeof(ir_item_t));
    (instruction2->result).value.v = method->incMaxVariablesReturnOld(method);
    (instruction2->result).type = IROFFSET;
    assert((instruction2->param_1).internal_type == (instruction2->param_2).internal_type);

    /* Push the address on  *
     * top of the stack	*/
    memcpy(&(stack->stack[stack->top]), &(instruction2->result), sizeof(ir_item_t));
    (stack->top)++;

    /* Return		*/
    return 0;
}

static inline JITINT16 translate_cil_newarr (t_system *system, CILStack cilStack, GenericContainer *container, t_binary_information *current_binary, Method method, JITUINT32 bytes_read, JITUINT32 token, t_stack *stack, JITUINT32 *current_label_ID, XanList *labels) {
    ir_instruction_t        *instruction;
    TypeDescriptor          *arrayType;

#ifdef DEBUG
    JITUINT32 stackTop;
#endif

    /* Assertions				*/
    assert(system != NULL);
    assert(current_binary != NULL);
    assert(method != NULL);
    assert(stack != NULL);
    assert(current_label_ID != NULL);
    assert(cilStack != NULL);
    assert(labels != NULL);

    /* Initialize the variables		*/
    instruction = NULL;

    /* Fetch the array type informations	*/
    arrayType = (system->cliManager).metadataManager->getTypeDescriptorFromToken(system->cliManager.metadataManager, container, current_binary, token);
    assert(arrayType != NULL);

    /* test whether the size of the array is < 0 or not. If the size is <0 we shall throw a
     * System.OverflowException             */
#ifdef DEBUG
    stackTop = stack->top;
#endif
    translate_Test_Overflow(system, method, cilStack, current_label_ID, labels, bytes_read, stack);
#ifdef DEBUG
    assert(stackTop == stack->top);
#endif

    /* Layout the type			*/
    (system->cliManager).layoutManager.layoutType(&((system->cliManager).layoutManager), arrayType);

    /* Make the IR instruction		*/
    PDEBUG("CILIR: NEWARR: Create the IRNEWARR instruction\n");
    instruction = method->newIRInstr(method);
    (stack->top)--;
    instruction->type = IRNEWARR;
    instruction->byte_offset = bytes_read;
    (instruction->param_1).value.v = (JITNUINT) arrayType;
    (instruction->param_1).type = IRCLASSID;
    (instruction->param_1).internal_type = IRCLASSID;
    memcpy(&(instruction->param_2), &(stack->stack[stack->top]), sizeof(ir_item_t));

    /* Clean the top of the stack           */
    stack->cleanTop(stack);

    stack->stack[stack->top].value.v = method->incMaxVariablesReturnOld(method);
    stack->stack[stack->top].type = IROFFSET;
    stack->stack[stack->top].internal_type = IROBJECT;
    stack->stack[stack->top].type_infos = arrayType;
    memcpy(&(instruction->result), &(stack->stack[stack->top]), sizeof(ir_item_t));
    assert((instruction->result).internal_type == IROBJECT);
    (stack->top)++;

    /* Refresh the root set of the method	*/
    assert((instruction->result).type == IROFFSET);
    assert((instruction->result).internal_type == IROBJECT);
    method->addVariableToRootSet(method, (instruction->result).value.v);

    /* Print the stack			*/
#ifdef PRINTDEBUG
    print_stack(stack, method->getParametersNumber(method), method->getLocalsNumber(method));
#endif

    /* At this point we have to verify if the object reference is a NULL reference.
     * This should verify only if the garbage collector has returned a NULL pointer because
     * the memory allocator is OUT OF MEMORY	*/
    if ((system->IRVM).behavior.debugExecution) {
        translate_Test_OutOfMemory(&(system->cliManager), method, cilStack, current_label_ID, labels, bytes_read, stack, NULL);
    }

    /* Return				*/
    return 0;
}

static inline JITINT16 translate_cil_ldtoken (t_system *system, Method method, GenericContainer *container, t_binary_information *binary, t_stack *stack, JITUINT32 bytes_read, JITUINT32 token, JITUINT32 *current_label_ID, XanList *labels) {
    char buffer[1024];
    ir_symbol_t             *item;
    void                    *fetch_descriptor;
    ILLayout                *Runtime_Handle;
    ir_instruction_t        *instruction;

    /* The System.Runtime*Handle manager */
    RuntimeHandleManager* rhManager;

    /* Offset of the value field inside a System.Runtime*Handle */
    JITINT32 valueOffset;

    /* Assertions			*/
    assert(system != NULL);
    assert(binary != NULL);
    assert(stack != NULL);
    assert(method != NULL);
    assert(labels != NULL);
    assert(current_label_ID != NULL);
    PDEBUG("CILIR: ldtoken: Start\n");

    /* Initialize the variables	*/
    item = NULL;
    Runtime_Handle = NULL;
    instruction = NULL;

    /* Get the System.Runtime*Handle manager */
    rhManager = &((system->cliManager).CLR.runtimeHandleManager);

    ILConcept decoded_concept = (system->cliManager).metadataManager->fetchDescriptor(system->cliManager.metadataManager, container, binary, token, &fetch_descriptor);
    switch (decoded_concept) {
        case ILTYPE:
            item = (system->cliManager).CLR.reflectManager.getCLRTypeSymbol((TypeDescriptor *) fetch_descriptor);
            Runtime_Handle = (system->cliManager).layoutManager.getRuntimeTypeHandleLayout(&((system->cliManager).layoutManager));
            assert(Runtime_Handle != NULL);
            assert(Runtime_Handle->type != NULL);
            valueOffset = rhManager->getRuntimeTypeHandleValueOffset(rhManager);
            break;
        case ILMETHOD:
            item = (system->cliManager).translator.getMethodDescriptorSymbol(&(system->cliManager), (MethodDescriptor *) fetch_descriptor);
            Runtime_Handle = (system->cliManager).layoutManager.getRuntimeMethodHandleLayout(&((system->cliManager).layoutManager));
            valueOffset = rhManager->getRuntimeMethodHandleValueOffset(rhManager);
            assert(Runtime_Handle != NULL);
            assert(Runtime_Handle->type != NULL);
            break;
        case ILFIELD:
            item = (system->cliManager).translator.getFieldDescriptorSymbol(&(system->cliManager), (FieldDescriptor *) fetch_descriptor);;
            Runtime_Handle = (system->cliManager).layoutManager.getRuntimeFieldHandleLayout(&((system->cliManager).layoutManager));
            valueOffset = rhManager->getRuntimeFieldHandleValueOffset(rhManager);
            assert(Runtime_Handle != NULL);
            assert(Runtime_Handle->type != NULL);
            break;
        default:
            snprintf(buffer, sizeof(buffer), "CILIR: ldtoken: ERROR= %d metadata table not known", decoded_concept);
            print_err(buffer, 0);
            abort();
    }
    assert(Runtime_Handle != NULL);
    assert(Runtime_Handle->type != NULL);
    assert(item != NULL);

    /* Create the new ValueType 		*/
    PDEBUG("CILIR: ldtoken:         Item to store	= %p\n", item);
    make_irnewValueType(&(system->cliManager), method, bytes_read, Runtime_Handle->type, stack, NULL);

    /* Print the stack			*/
#ifdef PRINTDEBUG
    print_stack(stack, method->getParametersNumber(method), method->getLocalsNumber(method));
#endif

    /* Make the IR instruction		*/
    instruction = method->newIRInstr(method);
    assert(instruction != NULL);
    instruction->type = IRSTOREREL;
    memcpy(&(instruction->param_1), &(stack->stack[stack->top - 1]), sizeof(ir_item_t));
    (instruction->param_2).value.v = valueOffset;
    (instruction->param_2).type = IRINT32;
    (instruction->param_2).internal_type = IRINT32;
    (instruction->param_2).type_infos = NULL;
    (instruction->param_3).value.v = (JITNUINT) item;
    (instruction->param_3).type = IRSYMBOL;
    (instruction->param_3).internal_type = IRNINT;
    instruction->byte_offset = bytes_read;

    /* set on the stack the value of the valuetype */
    (stack->top)--;
    instruction = method->newIRInstr(method);
    instruction->type = IRLOADREL;
    memcpy(&(instruction->param_1), &(stack->stack[stack->top]), sizeof(ir_item_t));
    (instruction->param_2).value.v = 0;
    (instruction->param_2).type = IRINT32;
    (instruction->param_2).internal_type = IRINT32;
    stack->stack[stack->top].value.v = method->incMaxVariablesReturnOld(method);
    stack->stack[stack->top].type = IROFFSET;
    stack->stack[stack->top].internal_type = IRVALUETYPE;
    stack->stack[stack->top].type_infos	= Runtime_Handle->type;
    memcpy(&(instruction->result), &(stack->stack[stack->top]), sizeof(ir_item_t));
    instruction->byte_offset = bytes_read;
    (stack->top)++;

    /* Print the stack				*/
#ifdef PRINTDEBUG
    print_stack(stack, method->getParametersNumber(method), method->getLocalsNumber(method));
#endif

    /* Return					*/
    return 0;
}

static inline JITINT16 make_ldelem (t_system *system, Method method, CILStack cilStack, JITUINT32 *current_label_ID, XanList *labels, JITUINT32 bytes_read, TypeDescriptor *classLocated, t_stack *stack) {
    JITUINT32               irType;
    JITUINT32               coerced_type;
    JITUINT32               arrayElementBytes;
    ir_instruction_t        *instruction;
    ir_instruction_t        *instruction2;
    ir_item_t		        tempItem;

    /* Assertions.
     */
    assert(method != NULL);
    PDEBUG("CILIR: make_ldelem: Start\n");

    /* Assertions.
     */
    assert(method != NULL);
    assert(stack != NULL);
    assert(cilStack != NULL);
    assert(system != NULL);
    assert(labels != NULL);
    assert(current_label_ID != NULL);
    assert((stack->stack[(stack->top) - 2]).internal_type == IROBJECT);

    /* Clean the top of the stack.
     */
    stack->cleanTop(stack);

    /* Retrieve the array object from the stack.
     */
    memcpy(&(stack->stack[(stack->top)]), &(stack->stack[(stack->top) - 2]), sizeof(ir_item_t));
    (stack->top)++;

    /* Test that the object is not NULL.
     */
    translate_Test_Null_Reference(&(system->cliManager), method, cilStack, bytes_read, stack);

    /* Clean the top of the stack.
     */
    stack->cleanTop(stack);

    /* Retrieve the index value from the stack.
     */
    memcpy(&(stack->stack[(stack->top)]), &(stack->stack[(stack->top) - 2]), sizeof(ir_item_t));
    (stack->top)++;

    /* Test that the index is not out of range.
     */
    translate_Test_Array_Index_Out_Of_Range(system, method, cilStack, current_label_ID, labels, bytes_read, stack);

    /* Pop the elements used only for testing from the stack.
     */
    (stack->top)--;
    (stack->top)--;

    /* Fetch the IR type of the element of the array.
     */
    if (classLocated != NULL) {
        irType = classLocated->getIRType(classLocated);
    } else {
        irType = IROBJECT;
        if (classLocated == NULL) {
            classLocated = (system->cliManager.metadataManager)->getTypeDescriptorFromIRType(system->cliManager.metadataManager, IROBJECT);
        }
    }

    /* Fetch the coerced type of the array element.
     */
    coerced_type = ((system->cliManager).metadataManager->generic_type_rules)->coerce_CLI_Type(irType);
    PDEBUG("COERCED TYPE == %d \n", coerced_type);

    /* Compute the number of bytes per element of the array.
     */
    if (irType == IRVALUETYPE){
        ILLayout                *layout_infos;

        /* Fetch the layout of the value type.
         */
        layout_infos        = (system->cliManager).layoutManager.layoutType(&((system->cliManager).layoutManager), classLocated);
        assert(layout_infos != NULL);

        /* Fetch the size of the element.
         */
        arrayElementBytes   = layout_infos->typeSize;

    } else {

        /* Fetch the size of the element.
         */
        arrayElementBytes   = IRDATA_getSizeOfIRType(irType);
    }

    /* At this point, the stack contains the element of the array to load at the top.
     * Moreover, below the element number, there is the address of the array.
     *
     * Compute the offset from the array address of the element to load.
     */
    (stack->top)--;
    instruction = method->newIRInstr(method);
    instruction->type = IRMUL;
    (instruction->param_2).value.v = arrayElementBytes;
    (instruction->param_2).type = IRINT32;
    (instruction->param_2).internal_type = IRINT32;
    memcpy(&(instruction->param_1), &(stack->stack[(stack->top)]), sizeof(ir_item_t));
    (instruction->result).value.v = method->incMaxVariablesReturnOld(method);
    (instruction->result).type = IROFFSET;
    (instruction->result).internal_type = IRINT32;
    assert((instruction->param_1).internal_type == (instruction->param_2).internal_type);
    memcpy(&tempItem, &(instruction->result), sizeof(ir_item_t));

    /* Convert the offset if necessary.
     */
    if (tempItem.internal_type != (stack->stack[(stack->top) - 1]).internal_type) {
        ir_instruction_t	*instructionConv;
        instructionConv = method->newIRInstr(method);
        instructionConv->type = IRCONV;
        memcpy(&(instructionConv->param_1), &tempItem, sizeof(ir_item_t));
        (instructionConv->param_2).value.v = stack->stack[stack->top - 1].internal_type;
        (instructionConv->param_2).type = IRTYPE;
        (instructionConv->param_2).internal_type = IRTYPE;
        (instructionConv->result).value.v = method->incMaxVariablesReturnOld(method);
        (instructionConv->result).type = IROFFSET;
        (instructionConv->result).internal_type = (instructionConv->param_2).value.v;
        memcpy(&tempItem, &(instructionConv->result), sizeof(ir_item_t));
        assert((instructionConv->param_1).internal_type != IRVALUETYPE);
    }

    /* Compute the address.
     */
    (stack->top)--;
    instruction2 = method->newIRInstr(method);
    instruction2->type = IRADD;
    memcpy(&(instruction2->param_1), &(stack->stack[(stack->top)]), sizeof(ir_item_t));
    memcpy(&(instruction2->param_2), &tempItem, sizeof(ir_item_t));
    (instruction2->result).value.v = method->incMaxVariablesReturnOld(method);
    (instruction2->result).type = IROFFSET;
    (instruction2->result).internal_type = (instruction2->param_2).internal_type;
    assert((instruction2->param_1).internal_type == (instruction2->param_2).internal_type);

    /* Load the element.
     */
    instruction = method->newIRInstr(method);
    instruction->type = IRLOADREL;
    memcpy(&(instruction->param_1), &(instruction2->result), sizeof(ir_item_t));
    (instruction->param_2).value.v = 0;
    (instruction->param_2).type = IRINT32;
    (instruction->param_2).internal_type = IRINT32;

    /* Set on top of the stack the value just loaded.
     */
    stack->cleanTop(stack);
    stack->stack[stack->top].value.v = method->incMaxVariablesReturnOld(method);
    stack->stack[stack->top].type = IROFFSET;
    stack->stack[stack->top].internal_type = irType;
    stack->stack[stack->top].type_infos	=  classLocated;
    (instruction->result).value.v = stack->stack[stack->top].value.v;
    (instruction->result).type = stack->stack[stack->top].type;
    (instruction->result).internal_type = stack->stack[stack->top].internal_type;
    make_ir_infos(&(instruction->result), &(stack->stack[stack->top]));
    instruction->byte_offset = bytes_read;
    (stack->top)++;

    /* Convert - if necessary - the value to a CLI stackable element type.
     */
    if (irType != coerced_type) {
        PDEBUG("type == %d ; however coerced_type == %d \n", irType, coerced_type);
        translate_cil_conv(&(system->cliManager), method, bytes_read, stack, coerced_type, 0, 0, 1);
    }

    PDEBUG("CILIR: make_ldelem: End\n");
    return 0;
}

static inline JITINT16 translate_cil_ldelem (t_system *system, CILStack cilStack, GenericContainer *container, t_binary_information *current_binary, Method method, JITUINT32 bytes_read, JITUINT32 token, t_stack *stack, JITUINT32 *current_label_ID, XanList *labels) {
    TypeDescriptor          *arrayType;

    /* Fetch the array type informations	*/
    arrayType = (system->cliManager).metadataManager->getTypeDescriptorFromToken(system->cliManager.metadataManager, container, current_binary, token);
    assert(arrayType != NULL);

    return make_ldelem(system, method, cilStack, current_label_ID, labels, bytes_read, arrayType, stack);
}

static inline JITINT16 make_stelem (t_system *system, Method method, CILStack cilStack, JITUINT32 *current_label_ID, XanList *labels, JITUINT32 bytes_read, TypeDescriptor *classLocated, t_stack *stack) {
    JITUINT32 irType;
    ir_instruction_t        *instruction;

    /* Assertions					*/
    assert(method != NULL);
    assert(system != NULL);
    assert(current_label_ID != NULL);
    assert(cilStack != NULL);
    assert(stack != NULL);
    PDEBUG("CILIR: make_stelem: Start\n");

    /* Fetch the array and push it on the stack     */
    memcpy(&(stack->stack[(stack->top)]), &(stack->stack[(stack->top) - 3]), sizeof(ir_item_t));
    assert((stack->stack[(stack->top) - 3]).type == IROFFSET);
    assert((stack->stack[(stack->top) - 3]).internal_type == IROBJECT);
    (stack->top)++;

    /* test if the array is a NULL reference */
    translate_Test_Null_Reference(&(system->cliManager), method, cilStack, bytes_read, stack);

    /* index must be a native integer or an int32 value */
    assert( ((stack->stack[(stack->top) - 3]).internal_type == IRNINT) || ((stack->stack[(stack->top) - 3]).internal_type == IRINT32) );

    /* Clean the top of the stack           */
    stack->cleanTop(stack);

    /* fetch the index and push it on the stack */
    memcpy(&(stack->stack[(stack->top)]), &(stack->stack[(stack->top) - 3]), sizeof(ir_item_t));
    (stack->top)++;

    /* test if the index is larger than the bound of the array */
    translate_Test_Array_Index_Out_Of_Range(system, method, cilStack, current_label_ID, labels, bytes_read, stack);

    /* pop the elements loaded on the stack only for testing purpouse */
    (stack->top)--;

    /* test if the classLocated is valid */
    if (classLocated != NULL) {
        irType = classLocated->getIRType(classLocated);

        /* test if the type mismatch the array type */
        //translate_Test_Array_Type_Mismatch(system, method, cilStack, current_label_ID,labels, bytes_read, stack, classLocated);
    } else {
        irType = IROBJECT;
        /* stelem.ref implicitly casts value to the element type of array before assigning the value
         * to the array element. This cast can fail, even for verified code. Thus the stelem.ref
         * instruction can throw the ArrayTypeMismatchException. */
        if (((stack->stack[(stack->top) - 2]).internal_type != IROBJECT)
                && ((stack->stack[(stack->top) - 2]).internal_type != IRMPOINTER)
                && ((stack->stack[(stack->top) - 2]).internal_type != IRNINT)
                && ((stack->stack[(stack->top) - 2]).internal_type != IRTPOINTER) ) {
            /* throw the ArrayTypeMismatchException */
            _translate_throw_CIL_Exception(&(system->cliManager), method, cilStack, current_label_ID, labels, bytes_read, stack, (system->exception_system)._System_ArrayTypeMismatchException_ID);
        }

    }
    /* pop the elements loaded on the stack only for testing purpouse */
    (stack->top)--;

    if (irType == IRVALUETYPE) {
        ir_instruction_t        *instruction2;
        ir_item_t		tempItem;
        ILLayout                *layout_infos;
        layout_infos = (system->cliManager).layoutManager.layoutType(&((system->cliManager).layoutManager), classLocated);

        /* Compute the address.
         */
        instruction = method->newIRInstr(method);
        instruction->type = IRMUL;
        (instruction->param_2).value.v = layout_infos->typeSize;
        (instruction->param_2).type = IRINT32;
        (instruction->param_2).internal_type = IRINT32;
        memcpy(&(instruction->param_1), &(stack->stack[(stack->top) - 2]), sizeof(ir_item_t));
        (instruction->result).value.v = method->incMaxVariablesReturnOld(method);
        (instruction->result).type = IROFFSET;
        (instruction->result).internal_type = IRINT32;
        assert((instruction->param_1).internal_type == (instruction->param_2).internal_type);

        memcpy(&tempItem, &(instruction->result), sizeof(ir_item_t));
        if (tempItem.internal_type != (stack->stack[(stack->top) - 3]).internal_type) {
            ir_instruction_t	*instructionConv;
            instructionConv = method->newIRInstr(method);
            instructionConv->type = IRCONV;
            memcpy(&(instructionConv->param_1), &tempItem, sizeof(ir_item_t));
            (instructionConv->param_2).value.v = stack->stack[stack->top - 3].internal_type;
            (instructionConv->param_2).type = IRTYPE;
            (instructionConv->param_2).internal_type = IRTYPE;
            (instructionConv->result).value.v = method->incMaxVariablesReturnOld(method);
            (instructionConv->result).type = IROFFSET;
            (instructionConv->result).internal_type = (instructionConv->param_2).value.v;
            memcpy(&tempItem, &(instructionConv->result), sizeof(ir_item_t));
            assert((instructionConv->param_1).internal_type != IRVALUETYPE);
        }
        instruction2 = method->newIRInstr(method);
        instruction2->type = IRADD;
        memcpy(&(instruction2->param_1), &(stack->stack[(stack->top) - 3]), sizeof(ir_item_t));
        memcpy(&(instruction2->param_2), &tempItem, sizeof(ir_item_t));
        (instruction2->result).value.v = method->incMaxVariablesReturnOld(method);
        (instruction2->result).type = IROFFSET;
        (instruction2->result).internal_type = (instruction2->param_2).internal_type;
        assert((instruction2->param_1).internal_type == (instruction2->param_2).internal_type);

        /* Push the address on  top of the stack	*/
        memcpy(&(stack->stack[stack->top]), &(instruction2->result), sizeof(ir_item_t));
        (stack->top)++;

        /* Create the IRGETADDRESS of the valuetype on the stack */
        instruction = method->newIRInstr(method);
        instruction->type = IRGETADDRESS;
        memcpy(&(instruction->param_1), &(stack->stack[(stack->top) - 2]), sizeof(ir_item_t));
        stack->stack[stack->top].value.v = method->incMaxVariablesReturnOld(method);
        stack->stack[stack->top].type = IROFFSET;
        stack->stack[stack->top].internal_type = IRMPOINTER;
        if ((instruction->param_1).type_infos != NULL) {
            stack->stack[stack->top].type_infos = (instruction->param_1).type_infos->makePointerDescriptor((instruction->param_1).type_infos);
        } else {
            stack->stack[stack->top].type_infos = NULL;
        }
        memcpy(&(instruction->result), &(stack->stack[stack->top]), sizeof(ir_item_t));
        (stack->top)++;
        instruction->byte_offset = bytes_read;
        assert((instruction->param_1).internal_type != NOPARAM);

        /* create an IRMEMCPY instruction */
        make_irmemcpy(method, cilStack, bytes_read, stack, current_label_ID, labels, layout_infos->typeSize, UNALIGNED_DONOTCHECK_ALIGNMENT, NULL);

        (stack->top)--;
        (stack->top)--;
        (stack->top)--;

    } else {
        ir_item_t		        tempItem;

        /* Check if we need to convert the value to store.
         */
        if (    (irType != (stack->stack[stack->top - 1].internal_type))        &&
                ((stack->stack[stack->top - 1].internal_type) != IRVALUETYPE)   ) {
            translate_cil_conv(&(system->cliManager), method, bytes_read, stack, irType, JITFALSE, JITFALSE, JITFALSE);
        }

        /* Compute the offset from the array address of the element to load.
         */
        instruction = method->newIRInstr(method);
        instruction->type = IRMUL;
        (instruction->param_2).value.v = IRDATA_getSizeOfIRType(irType);
        (instruction->param_2).type = IRINT32;
        (instruction->param_2).internal_type = IRINT32;
        memcpy(&(instruction->param_1), &(stack->stack[(stack->top - 2)]), sizeof(ir_item_t));
        (instruction->result).value.v = method->incMaxVariablesReturnOld(method);
        (instruction->result).type = IROFFSET;
        (instruction->result).internal_type = IRINT32;
        assert((instruction->param_1).internal_type == (instruction->param_2).internal_type);
        memcpy(&tempItem, &(instruction->result), sizeof(ir_item_t));

        /* Store the value in memory.
         */
        (stack->top)--;
        instruction = method->newIRInstr(method);
        assert(instruction != NULL);
        instruction->type = IRSTOREREL;
        memcpy(&(instruction->param_3), &(stack->stack[stack->top]), sizeof(ir_item_t));
#ifdef DEBUG
        if ((instruction->param_3).internal_type == IRVALUETYPE) {
            assert((instruction->param_3).type_infos != NULL);
        }
#endif
        (stack->top)--;
        memcpy(&(instruction->param_2), &tempItem, sizeof(ir_item_t));
        (stack->top)--;
        memcpy(&(instruction->param_1), &(stack->stack[stack->top]), sizeof(ir_item_t));
        instruction->byte_offset = bytes_read;
    }

    PDEBUG("CILIR: make_stelem: Exit\n");
    return 0;
}

static inline JITINT16 translate_cil_stelem (t_system *system, CILStack cilStack, GenericContainer *container, t_binary_information *current_binary, Method method, JITUINT32 bytes_read, JITUINT32 token, t_stack *stack, JITUINT32 *current_label_ID, XanList *labels) {
    TypeDescriptor          *arrayType;

    /* Fetch the array type informations	*/
    arrayType = (system->cliManager).metadataManager->getTypeDescriptorFromToken(system->cliManager.metadataManager, container, current_binary, token);
    assert(arrayType != NULL);

    return make_stelem(system, method, cilStack, current_label_ID, labels, bytes_read, arrayType, stack);
}


static inline JITINT16 translate_instruction_with_binary_logical_operator (t_system *system, Method method, JITUINT32 bytes_read, t_stack *stack, JITUINT32 logical_operator) {
    JITUINT32 return_type;

    /* Assertions		                */
    assert(method != NULL);
    assert(stack != NULL);
    assert(system != NULL);
    assert( (logical_operator == IROR)
            || (logical_operator == IRAND)
            || (logical_operator == IRXOR) );
    assert(stack->top >= (method->getParametersNumber(method) + method->getLocalsNumber(method) + 2));
    PDEBUG("CILIR: translate_instruction_with_binary_logical_operator: Start\n");


    /* Initialize the local variables       */
    return_type = NOPARAM;

    /* Convert Enums in IRINT32		*/
    convertEnumIntoInt32(&(stack->stack[(stack->top) - 1]));
    convertEnumIntoInt32(&(stack->stack[(stack->top) - 2]));

    /* coerce value to valid stackable CLI values */
    coerce_operands_for_binary_operation(&(system->cliManager), method, bytes_read, stack);

    /* retrieve the return_type value */
    return_type = ((system->cliManager).metadataManager->generic_type_rules)->get_return_IRType_For_Integer_operation((stack->stack[stack->top - 2]).internal_type, (stack->stack[stack->top - 1]).internal_type);
    assert(return_type != NOPARAM);

    /* Translate the binary operation       */
    _translate_binary_operation(method, bytes_read, stack, logical_operator, return_type);

    /* Return				*/
    PDEBUG("CILIR: translate_instruction_with_binary_logical_operator: End\n");
    return 0;
}

static inline JITINT16 translate_cil_compare (CLIManager_t *cliManager, Method method, CILStack cilStack, JITUINT32 bytes_read, JITUINT32 type, t_stack *stack, JITUINT32 *current_label_ID, XanList *labels, JITBOOLEAN unordered_check, JITBOOLEAN fix_all_labels, JITBOOLEAN force_stackable_type) {
    JITUINT32 return_internal_type;
    JITBOOLEAN verified_code;
#ifdef PRINTDEBUG
    JITUINT32 initial_stack_top;
#endif

    /* assertions */
    assert(method != NULL);
    assert(current_label_ID != NULL);
    assert(labels != NULL);
    assert(stack != NULL);
    assert(cilStack != NULL);
    assert(stack->top >= ((method->getParametersNumber(method) + method->getLocalsNumber(method) + 2)));
    assert(type == IREQ || type == IRLT || type == IRGT);
    PDEBUG("CILIR: translate_cil_compare: Start\n");

    /* initialize the locals */
    return_internal_type = NOPARAM;
    verified_code = 1;
#ifdef PRINTDEBUG
    initial_stack_top = stack->top;
#endif
    PDEBUG("CILIR: translate_cil_compare:   Param 1 TYPE == %d [before coercion]\n", (stack->stack[(stack->top) - 1]).internal_type);
    PDEBUG("CILIR: translate_cil_compare:   Param 2 TYPE == %d [before coercion]\n", (stack->stack[(stack->top) - 2]).internal_type);

    /* Check if the operands are an		*
     * Enum					*/
    convertEnumIntoInt32(&(stack->stack[(stack->top) - 1]));
    convertEnumIntoInt32(&(stack->stack[(stack->top) - 2]));

    /* Test if we have to check integers as unsigned values */
    if (unordered_check) {
        PDEBUG("CILIR: translate_cil_compare: unordered check...\n");
        coerce_operands_for_binary_operation_to_unsigned_types(cliManager, method, bytes_read, stack, JITFALSE);
    }

    /* Check if we need to force operand types as stackable	*/
    if (force_stackable_type) {

        /* coerce values to match valid CLI stackable types */
        PDEBUG("CILIR: translate_cil_compare: coerce values to match valid CLI stackable types\n");
        coerce_operands_for_binary_operation(cliManager, method, bytes_read, stack);
        PDEBUG("CILIR: translate_cil_compare:   Param 1 TYPE == %d [after coercion]\n", (stack->stack[(stack->top) - 1]).internal_type);
        PDEBUG("CILIR: translate_cil_compare:   Param 2 TYPE == %d [after coercion]\n", (stack->stack[(stack->top) - 2]).internal_type);

        /* Print the stack			*/
#ifdef PRINTDEBUG
        assert(stack->top == initial_stack_top);
        PDEBUG("CILIR: translate_cil_compare:   Stack before the compare instruction\n");
        print_stack(stack, method->getParametersNumber(method), method->getLocalsNumber(method));
#endif
    }

    /* evaluate the return internal_type: this is the type the result value stored on top of the stack will have
       to be converted to after the comparison */
    PDEBUG("CILIR: translate_cil_compare:   Param 1 TYPE == %d\n", (stack->stack[(stack->top) - 2]).internal_type);
    PDEBUG("CILIR: translate_cil_compare:   Param 2 TYPE == %d\n", (stack->stack[(stack->top) - 1]).internal_type);

    if (!force_stackable_type) {
        return_internal_type = IRINT32;
    }

    if (return_internal_type == NOPARAM) {
        return_internal_type = (cliManager->metadataManager->generic_type_rules)->get_return_IRType_For_Binary_Comparison(     (stack->stack[(stack->top) - 2]).internal_type, (stack->stack[(stack->top) - 1]).internal_type, type, &verified_code );
    }

    PDEBUG("CILIR: translate_cil_compare:   Return TYPE == %d\n", return_internal_type);
    assert(return_internal_type != NOPARAM);
    assert(return_internal_type == IRINT32);

    /* Check if we have to translate floating point	*
     * values to our internal rappresentation 	*
     * (IRFLOAT64)					*/
    if (	((stack->stack[stack->top - 1]).internal_type == IRFLOAT32)	||
            ((stack->stack[stack->top - 1]).internal_type == IRNFLOAT)	) {
        translate_cil_conv(cliManager, method, bytes_read, stack, IRFLOAT64, JITFALSE, JITFALSE, JITFALSE);
        assert((stack->stack[stack->top - 1]).internal_type == IRFLOAT64);
    }
    if (	((stack->stack[stack->top - 2]).internal_type == IRFLOAT32)	||
            ((stack->stack[stack->top - 2]).internal_type == IRNFLOAT)	) {
        ir_item_t	tempItem;

        /* Pop the first operand			*/
        memcpy(&tempItem, &(stack->stack[stack->top - 1]), sizeof(ir_item_t));
        (stack->top)--;

        /* Convert the second operand			*/
        translate_cil_conv(cliManager, method, bytes_read, stack, IRFLOAT64, JITFALSE, JITFALSE, JITFALSE);

        /* Push back to the stack the first operand	*/
        memcpy(&(stack->stack[stack->top]), &tempItem, sizeof(ir_item_t));
        (stack->top)++;
        assert((stack->stack[stack->top - 2]).internal_type == IRFLOAT64);
    }

    /* Create the "Compare" instruction		*/
    if (type == IREQ) {
        translate_cil_eq(cliManager, method, bytes_read, stack);
    } else if (type == IRLT) {
        translate_cil_lt(cliManager, method, bytes_read, stack);
    } else {
        assert(type == IRGT);
        translate_cil_gt(cliManager, method, bytes_read, stack);
    }

    /* Print the stack			*/
#ifdef PRINTDEBUG
    PDEBUG("CILIR: translate_cil_compare:   Stack after the compare instruction\n");
    assert(stack->top == (initial_stack_top - 1));
    print_stack(stack, method->getParametersNumber(method), method->getLocalsNumber(method));
#endif

    /* convert the return value into a IRINT32 */
    translate_cil_conv(cliManager, method, bytes_read, stack, IRINT32, 0, 0, 0);
    assert((stack->stack[(stack->top) - 1]).type == IROFFSET);
    assert((stack->stack[(stack->top) - 1]).internal_type == IRINT32);

    /* Print the stack			*/
#ifdef PRINTDEBUG
    PDEBUG("CILIR: translate_cil_compare:   Stack after the conversion instruction\n");
    assert(stack->top == (initial_stack_top - 1));
    print_stack(stack, method->getParametersNumber(method), method->getLocalsNumber(method));
#endif

    /* Return		*/
    PDEBUG("CILIR: translate_cil_compare: Exit\n");
    return 0;
}

static inline JITINT16 translate_cil_ldsflda (t_system *system, Method method, CILStack cilStack, JITUINT32 *current_label_ID, XanList *labels, JITUINT32 bytes_read, GenericContainer *container, t_binary_information *binary, JITUINT32 token, t_stack *stack) {
    ir_instruction_t        *instruction;
    FieldDescriptor         *fieldLocated;
    TypeDescriptor          *classID;
    ir_symbol_t             *staticObject;
    JITINT32 field_offset;

    /* Assertions			*/
    assert(method != NULL);
    assert(stack != NULL);
    assert(cilStack != NULL);
    assert(binary != NULL);
    assert(system != NULL);
    assert(current_label_ID != NULL);
    assert(labels != NULL);

    /* initialize the locals */
    instruction = NULL;
    fieldLocated = NULL;

    PDEBUG("CILIR : translate_cil_ldsflda : test if the field token is not valid \n");
    /* test if the field token is not valid */
    if (translate_Test_Missing_Field(system, method, cilStack, current_label_ID, labels, bytes_read, stack, binary, token) == 1) {
        PDEBUG("CILIR: cil_ldsflda: EXITING missing field-token found \n");
        return 1;
    }

    /* retrieve the fieldID and the binary associated with the field */
    fieldLocated = (system->cliManager).metadataManager->getFieldDescriptorFromToken(system->cliManager.metadataManager, container, binary, token);

    PDEBUG("CILIR : translate_cil_ldsflda : FIELD_NAME = \"%s\"\n", fieldLocated->getCompleteName(fieldLocated));

    PDEBUG("CILIR : translate_cil_ldsflda : translate_Test_Null_Reference_With_Static_Field_Check \n");
    /* test if object is null and field is static */
    translate_Test_Null_Reference_With_Static_Field_Check(&(system->cliManager), method, cilStack, current_label_ID, labels,  bytes_read, stack, fieldLocated);

    PDEBUG("CILIR : translate_cil_ldsflda : translate_Test_Field_Access \n");
    /* test if the current method has the correct rights to access the field */
    if (translate_Test_Field_Access(system, method, cilStack, current_label_ID, labels, bytes_read, stack, fieldLocated) == 1) {
        PDEBUG("CILIR : translate_cil_ldsflda : EXITING invalid field access detected \n");
        return 1;
    }
    PDEBUG("CILIR : translate_cil_ldsfld : END TESTING. \n");


    /* Fetch the class of the field	*/
    PDEBUG("CILIR: translate_cil_ldsflda : Fetch the class ID of the field\n");
    classID = fieldLocated->owner;
    assert(classID != NULL);

    /* Check if the field is static                         */
    assert(fieldLocated->attributes->is_static);

    /* Fetch the static Object				*/
    staticObject = STATICMEMORY_fetchStaticObjectSymbol(&(system->staticMemoryManager), method, classID);

    /* Fetch the field offset				*/
    field_offset = (system->garbage_collectors).gc.fetchStaticFieldOffset(fieldLocated);

    /* Sum the values to obtain the static field address	*/
    instruction = method->newIRInstr(method);
    instruction->type = IRADD;
    (instruction->result).value.v = method->incMaxVariablesReturnOld(method);
    (instruction->result).type = IROFFSET;
    (instruction->result).internal_type = IRMPOINTER;
    (instruction->result).type_infos = fieldLocated->getType(fieldLocated);
    (instruction->param_1).value.v = (JITNUINT) staticObject;
    (instruction->param_1).type = IRSYMBOL;
    (instruction->param_1).internal_type = (instruction->result).internal_type;
    (instruction->param_1).type_infos = classID;
    (instruction->param_2).value.v = (IR_ITEM_VALUE) field_offset;
    (instruction->param_2).type = (instruction->result).internal_type;
    (instruction->param_2).internal_type = (instruction->param_2).type;
    instruction->byte_offset = bytes_read;
    assert((instruction->param_1).internal_type == (instruction->param_2).internal_type);

    /* Clean the top of the stack           */
    stack->cleanTop(stack);

    /* Push the pointer			*/
    memcpy(&(stack->stack[stack->top]), &(instruction->result), sizeof(ir_item_t));
    (stack->top)++;

    /* Return                               */
    return 0;
}

static inline JITINT16 translate_cil_ldflda (t_system *system, Method method, CILStack cilStack, JITUINT32 *current_label_ID, XanList *labels, JITUINT32 bytes_read, GenericContainer *container, t_binary_information *binary, JITUINT32 token, t_stack *stack) {
    ir_instruction_t        *instruction;
    FieldDescriptor         *fieldLocated;
    JITBOOLEAN 		unmanaged;
    TypeDescriptor          *classID;
    ir_symbol_t             *staticObject;
    JITINT32 		field_offset;
    JITUINT32		typeToUse;
    ir_item_t		tempItem;

    /* Assertions			*/
    assert(method != NULL);
    assert(stack != NULL);
    assert(binary != NULL);
    assert(system != NULL);
    assert(cilStack != NULL);
    assert(current_label_ID != NULL);
    assert(labels != NULL);

    /* initialize the locals */
    instruction = NULL;
    fieldLocated = NULL;
    unmanaged = 0;

    PDEBUG("CILIR : translate_cil_ldflda : test if the field token is not valid \n");
    /* test if the field token is not valid */
    if (translate_Test_Missing_Field(system, method, cilStack, current_label_ID, labels, bytes_read, stack, binary, token) == 1) {
        PDEBUG("CILIR: cil_ldflda: EXITING missing field-token found \n");
        return 1;
    }

    /* retrieve the fieldID and the binary associated with the field */
    fieldLocated = (system->cliManager).metadataManager->getFieldDescriptorFromToken(system->cliManager.metadataManager, container, binary, token);

    PDEBUG("CILIR : translate_cil_ldflda : FIELD_NAME = \"%s\"\n", fieldLocated->getCompleteName(fieldLocated));

    PDEBUG("CILIR : translate_cil_ldflda : translate_Test_Null_Reference_With_Static_Field_Check \n");
    /* test if object is null and field is static */
    translate_Test_Null_Reference_With_Static_Field_Check(&(system->cliManager), method, cilStack, current_label_ID, labels, bytes_read, stack, fieldLocated);

    PDEBUG("CILIR : translate_cil_ldflda : translate_Test_Field_Access \n");
    /* test if the current method has the correct rights to access the field */
    if (translate_Test_Field_Access(system, method, cilStack, current_label_ID, labels, bytes_read, stack, fieldLocated) == 1) {
        PDEBUG("CILIR : translate_cil_ldflda : EXITING invalid field access detected \n");
        return 1;
    }

    /* Create the instruction				*/
    instruction = method->newIRInstr(method);
    assert(instruction != NULL);

    /* Check whether we are handling unmanaged types	*/
    (stack->top)--;
    assert((stack->stack[stack->top]).type == IROFFSET);
    if ((stack->stack[stack->top]).type == IRNINT) {
        unmanaged = 1;
    }
    if (unmanaged) {
        typeToUse	= IRNINT;
    } else {
        typeToUse	= IRMPOINTER;
    }

    /* Check if the field is static */
    if (fieldLocated->attributes->is_static) {

        /* Fetch the class of the field	*/
        PDEBUG("CILIR: translate_cil_ldflda : Fetch the class ID of the field\n");
        classID = fieldLocated->owner;
        assert(classID != NULL);

        /* Fetch the static Object	*/
        staticObject = STATICMEMORY_fetchStaticObjectSymbol(&(system->staticMemoryManager), method, classID);

        /* Fetch the field offset	*/
        field_offset = (system->garbage_collectors).gc.fetchStaticFieldOffset(fieldLocated);

        /* Clean the top of the stack           */
        stack->cleanTop(stack);

        /* Put the offset on the stack	*/
        stack->stack[stack->top].value.v = field_offset;
        stack->stack[stack->top].type = typeToUse;
        stack->stack[stack->top].internal_type = stack->stack[stack->top].type;
        (stack->top)++;

        /* Clean the top of the stack           */
        stack->cleanTop(stack);

        /* Put the base address of the static object on the stack	*/
        stack->stack[stack->top].value.v = (JITNUINT) staticObject;
        stack->stack[stack->top].type = IRSYMBOL;
        stack->stack[stack->top].internal_type = typeToUse;
        (stack->top)++;

        /* Sum the values to obtain the static field address	*/
        instruction = method->newIRInstr(method);
        assert(instruction != NULL);

        /* the field is static */
        instruction->type = IRADD;
        memcpy(&(instruction->param_1), &(stack->stack[stack->top]), sizeof(ir_item_t));
        memcpy(&(instruction->param_2), &(stack->stack[(stack->top) - 1]), sizeof(ir_item_t));
        stack->top--;
        stack->stack[stack->top].value.v = method->incMaxVariablesReturnOld(method);
        stack->stack[stack->top].type = IROFFSET;
        stack->stack[stack->top].internal_type = typeToUse;
        stack->stack[stack->top].type_infos = NULL;
        (instruction->result).value.v = (stack->stack[stack->top]).value.v;
        (instruction->result).type = (stack->stack[stack->top]).type;
        (instruction->result).internal_type = (stack->stack[stack->top]).internal_type;
        assert((instruction->result).type == IROFFSET);
        assert((instruction->param_1).internal_type == (instruction->param_2).internal_type);
        instruction->byte_offset = bytes_read;

        return 0;
    }

    /* The field is not static	*/
    instruction->type = IRADD;
    instruction->byte_offset = bytes_read;

    /* Object		*/
    if (stack->stack[stack->top].internal_type != typeToUse) {
        ir_instruction_t	*instructionConv;
        instructionConv = method->newIRInstrBefore(method, instruction);
        instructionConv->type = IRCONV;
        memcpy(&(instructionConv->param_1), &(stack->stack[stack->top]), sizeof(ir_item_t));
        (instructionConv->param_2).value.v = typeToUse;
        (instructionConv->param_2).type = IRTYPE;
        (instructionConv->param_2).internal_type = IRTYPE;
        (instructionConv->result).value.v = method->incMaxVariablesReturnOld(method);
        (instructionConv->result).type = IROFFSET;
        (instructionConv->result).internal_type = (instructionConv->param_2).value.v;
        memcpy(&(stack->stack[stack->top]), &(instructionConv->result), sizeof(ir_item_t));
        assert((instructionConv->param_1).internal_type != IRVALUETYPE);
    }
    memcpy(&(instruction->param_1), &(stack->stack[(stack->top)]), sizeof(ir_item_t));

    /* Field offset			*/
    (instruction->param_2).value.v = (system->garbage_collectors).gc.fetchFieldOffset(fieldLocated);
    (instruction->param_2).type = typeToUse;
    (instruction->param_2).internal_type = typeToUse;
    assert((instruction->param_1).internal_type == (instruction->param_2).internal_type);

    /* Result			*/
    memset(&tempItem, 0, sizeof(ir_item_t));
    tempItem.value.v	= method->incMaxVariablesReturnOld(method);
    tempItem.type 		= IROFFSET;
    tempItem.type_infos	= fieldLocated->getType(fieldLocated);
    tempItem.internal_type 	= (instruction->param_1).internal_type;
    memcpy(&(instruction->result), &tempItem, sizeof(ir_item_t));

    /* Push the result to the stack	*/
    stack->cleanTop(stack);
    memcpy(&(stack->stack[stack->top]), &(instruction->result), sizeof(ir_item_t));
    (stack->top)++;

    /* Return			*/
    return 0;
}

static inline JITINT16 translate_cil_ldfld (t_system *system, Method method, CILStack cilStack, JITUINT32 *current_label_ID, XanList *labels, JITUINT32 bytes_read, GenericContainer *container, t_binary_information *binary, JITUINT32 token, t_stack *stack) {
    ir_instruction_t        *instruction;
    FieldDescriptor         *fieldLocated;
    JITUINT32 type_of_the_located_field;
    JITUINT32 ir_field_type;
    JITINT32 field_offset;

    /* Assertions			*/
    assert(method != NULL);
    assert(stack != NULL);
    assert(binary != NULL);
    assert(system != NULL);
    assert(cilStack != NULL);
    assert(current_label_ID != NULL);
    assert(labels != NULL);

    /* Initialize the locals */
    instruction = NULL;
    fieldLocated = NULL;

    /* Test if the field token is not valid */
    if (translate_Test_Missing_Field(system, method, cilStack, current_label_ID, labels, bytes_read, stack, binary, token) == 1) {
        PDEBUG("CILIR: cil_ldfld:       EXITING missing field-token found \n");
        return 1;
    }

    /* retrieve the fieldID and the binary associated with the field */
    fieldLocated = (system->cliManager).metadataManager->getFieldDescriptorFromToken(system->cliManager.metadataManager, container, binary, token);
    assert(fieldLocated != NULL);
    PDEBUG("CILIR: translate_cil_ldfld:	FIELD_NAME = \"%s\"\n", fieldLocated->getCompleteName(fieldLocated));

    /* Test if object is null and field is static */
    PDEBUG("CILIR: translate_cil_ldfld:     Translate_Test_Null_Reference_With_Static_Field_Check\n");
    translate_Test_Null_Reference_With_Static_Field_Check(&(system->cliManager), method, cilStack, current_label_ID, labels, bytes_read, stack, fieldLocated);

    /* Test if the current method has the correct rights to access the field */
    if (translate_Test_Field_Access(system, method, cilStack, current_label_ID, labels, bytes_read, stack, fieldLocated) == 1) {
        PDEBUG("CILIR: translate_cil_ldfld:     EXITING invalid field access detected \n");
        return 1;
    }

    /* retrieve the type of the field */
    TypeDescriptor *resolved_type = fieldLocated->getType(fieldLocated);
    type_of_the_located_field = resolved_type->getIRType(resolved_type);
    assert(type_of_the_located_field != NOPARAM);

    /* Store information on the new	*
     * IR instruction		*/
    PDEBUG("CILIR: translate_cil_ldfld:     Translate the load relative instruction\n");
    (stack->top)--;
    assert((stack->stack[stack->top]).type == IROFFSET);
    instruction = method->newIRInstr(method);
    instruction->type = IRLOADREL;

    /* Object		*/
    memcpy(&(instruction->param_1), &(stack->stack[(stack->top)]), sizeof(ir_item_t));

    /* Field type		*/
    field_offset = (system->garbage_collectors).gc.fetchFieldOffset(fieldLocated);
    ir_field_type = type_of_the_located_field;

    /* Field offset		*/
    (instruction->param_2).value.v = field_offset;
    (instruction->param_2).type = IRINT32;
    (instruction->param_2).internal_type = IRINT32;

    /* Push the value loaded*
     * onto the top of the	*
     * stack		*/
    stack->stack[stack->top].value.v = method->incMaxVariablesReturnOld(method);
    stack->stack[stack->top].type = IROFFSET;
    stack->stack[stack->top].internal_type = ir_field_type;
    stack->stack[stack->top].type_infos = resolved_type;
    memcpy(&(instruction->result), &(stack->stack[(stack->top)]), sizeof(ir_item_t));
    instruction->byte_offset = bytes_read;
    (stack->top)++;

    /* The value that is on top of the stack will remain there after this opcode, so it has to be stackable */
    JITINT32 toType;
    toType = ((system->cliManager).metadataManager->generic_type_rules)->coerce_CLI_Type(stack->stack[stack->top - 1].internal_type);
    if (stack->stack[stack->top - 1].internal_type != toType) {
        _perform_conversion(&(system->cliManager), method, bytes_read, stack, toType, JITFALSE, NULL);
    }

    /* Return		*/
    PDEBUG("CILIR: translate_cil_ldfld: Exit\n");
    return 0;
}

static inline JITINT16 translate_cil_stfld (Method method, CILStack cilStack, JITUINT32 bytes_read, GenericContainer *container, t_binary_information *binary, JITUINT32 token, t_stack *stack, JITUINT32 *current_label_ID, XanList *labels, t_system *system) {
    ir_instruction_t        *instruction;
    FieldDescriptor         *fieldLocated;
    JITUINT32 irFieldType;

    /* Assertions			*/
    assert(method != NULL);
    assert(stack != NULL);
    assert(cilStack != NULL);
    assert(binary != NULL);
    assert(current_label_ID != NULL);
    assert(labels != NULL);
    assert(system != NULL);
    PDEBUG("CILIR: translate_cil_stfld: Start\n");

    /* Initialize the locals                */
    instruction = NULL;
    fieldLocated = NULL;

    /* test if the field token is not valid */
    PDEBUG("CILIR: translate_cil_stfld:     Test if the field token is not valid \n");
    if (translate_Test_Missing_Field(system, method, cilStack, current_label_ID, labels, bytes_read, stack, binary, token) == 1) {
        PDEBUG("CILIR: translate_cil_stfld: EXITING missing field-token found \n");
        return 1;
    }

    /* retrieve the fieldID and the binary associated with the field */
    fieldLocated = (system->cliManager).metadataManager->getFieldDescriptorFromToken(system->cliManager.metadataManager, container, binary, token);
    assert(fieldLocated != NULL);
    PDEBUG("CILIR: translate_cil_ldfld:	FIELD_NAME = \"%s\"\n", fieldLocated->getCompleteName(fieldLocated));

    /* Duplicate the object on the top of the stack         */
    memcpy(&(stack->stack[stack->top]), &(stack->stack[(stack->top) - 2]), sizeof(ir_item_t));
    assert( ((stack->stack[(stack->top) - 2]).internal_type == IROBJECT)
            || ((stack->stack[(stack->top) - 2]).internal_type == IRMPOINTER)
            || ((stack->stack[(stack->top) - 2]).internal_type == IRNINT)
            || ((stack->stack[(stack->top) - 2]).internal_type == IRINT32)
            || ((stack->stack[(stack->top) - 2]).internal_type == IRINT64)
            || ((stack->stack[(stack->top) - 2]).internal_type == IRNINT) );
    (stack->top)++;

    /* Make the conversion                                  */
    switch ((stack->stack[(stack->top) - 1]).internal_type) {
        case IRINT32:
        case IRINT64:
        case IRUINT32:
        case IRUINT64:
            _perform_conversion(&(system->cliManager), method, bytes_read, stack, IRNINT, JITFALSE, NULL);
            break;
    }

    /* Test NULL reference                                  */
    translate_Test_Null_Reference_With_Static_Field_Check(&(system->cliManager), method, cilStack, current_label_ID, labels, bytes_read, stack, fieldLocated);

    /* Pop the object reference from the top of the stack   */
    (stack->top)--;

    /* Test if the current method has the correct rights to access the field */
    PDEBUG("CILIR: translate_cil_stfld:     Translate_Test_Field_Access\n");
    if (translate_Test_Field_Access(system, method, cilStack, current_label_ID, labels, bytes_read, stack, fieldLocated) == 1) {
        PDEBUG("CILIR : translate_cil_stfld : EXITING invalid field access detected \n");
        return 1;
    }

    /* Fetch the IR field type	*/
    TypeDescriptor *resolved_type = fieldLocated->getType(fieldLocated);
    irFieldType = resolved_type->getIRType(resolved_type);

    /* Make the conversion		*/
    if (irFieldType != ((stack->stack[stack->top - 1]).internal_type)) {
        _perform_conversion(&(system->cliManager), method, bytes_read, stack, irFieldType, JITFALSE, NULL);
    }

    /* Create the instruction	*/
    instruction = method->newIRInstr(method);

    /* Store the instruction	*
     * informations			*/
    PDEBUG("CILIR: translate_cil_stfld:     Translate the store instruction\n");
    instruction->type = IRSTOREREL;
    instruction->byte_offset = bytes_read;

    /* Value to store			*/
    (stack->top)--;
    memcpy(&(instruction->param_3), &(stack->stack[(stack->top)]), sizeof(ir_item_t));

    /* Memory base address			*/
    (stack->top)--;
    memcpy(&(instruction->param_1), &(stack->stack[(stack->top)]), sizeof(ir_item_t));

    /* Memory offset			*/
    (instruction->param_2).value.v = (system->garbage_collectors).gc.fetchFieldOffset(fieldLocated);
    (instruction->param_2).type = IRINT32;
    (instruction->param_2).internal_type = IRINT32;

    /* Return				*/
    return 0;
}

static inline JITINT16 translate_cil_vcall (t_system *system, CILStack cilStack, GenericContainer *container, t_binary_information *binary, Method method, JITUINT32 token, JITUINT32 bytes_read, t_stack *stack, JITBOOLEAN isTail, TypeDescriptor *constrained, JITUINT32 *current_label_ID, XanList *labels) {
    ir_instruction_t        *instruction;
    ir_instruction_t        *instruction2;
    Method newMethod;
    ActualMethodDescriptor tempMethod;
    JITUINT32 obj_position;
    JITINT32 imtOffset;
    JITINT32 vtableOffset;
    VCallPath vtableIndex;
    JITUINT32 objVarID;
    JITUINT32 vtablePointerVarID;
    JITUINT32 imtPointerVarID;
    JITUINT32 keyID;
    JITUINT32 keyEqID;
    JITUINT32 vtableVarID;
    JITUINT32 methodVarID;
    JITUINT32 bucketVarID;
    TypeDescriptor *objVarType;
    JITUINT32 label_l1;
    JITUINT32 label_l2;

    PDEBUG("CILIR: translate_cil_vcall: Start\n");

    /* Setup the function call */
    if (translate_cil_call_setup(system, cilStack, container, binary, method, token, bytes_read, stack, isTail, current_label_ID, labels, &tempMethod) == 1) {
        return 1;
    }

    /* Initialize the variables		*/
    vtableOffset = 0;


    /* test if object is a NULL reference */
    /* At first we have to retrieve the object position on the stack */
    XanList *params = (tempMethod.method)->getParams(tempMethod.method);
    obj_position = (xanList_length(params) + 1);
    assert(obj_position > 0);


    if (constrained != NULL) {
        if (constrained->isValueType(constrained)) {
            MethodDescriptor *body = constrained->implementsMethod(constrained, tempMethod.method);
            TypeDescriptor *owner = body->owner;
            if (owner != constrained) {
                /* box ValueType */
                assert((system->cliManager).layoutManager.layoutType != NULL);

                ILLayout *layoutInfos = (system->cliManager).layoutManager.layoutType(&((system->cliManager).layoutManager), constrained);
                assert(layoutInfos != NULL);

                make_irnewobj(&(system->cliManager), method, cilStack, bytes_read, constrained, 0, stack, current_label_ID, labels, NULL);

                memcpy(&(stack->stack[(stack->top)]), &(stack->stack[(stack->top) - obj_position - 1]), sizeof(ir_item_t));
                memcpy(&(stack->stack[(stack->top) - obj_position - 1]), &(stack->stack[(stack->top) - 1]), sizeof(ir_item_t));
                (stack->top)++;

                make_irmemcpy(method, cilStack, bytes_read, stack, current_label_ID, labels, layoutInfos->typeSize, UNALIGNED_DONOTCHECK_ALIGNMENT, NULL);

            } else {
                /* devirtualize call */
                JITINT16 result = translate_cil_call_by_methodID(&(system->cliManager), method, body, bytes_read, stack, 0, tempMethod.actualSignature, cilStack, current_label_ID, labels);
                PDEBUG("CILIR: translate_cil_vcall: Exit\n");
                return result;
            }
        } else {
            /* dereference pointer*/
            instruction = method->newIRInstr(method);
            instruction->type = IRLOADREL;
            memcpy(&(instruction->param_1), &(stack->stack[(stack->top) - obj_position]), sizeof(ir_item_t));
            (instruction->param_2).value.v = 0;
            (instruction->param_2).type = IRINT32;
            (instruction->param_2).internal_type = IRINT32;
            (instruction->result).value.v = method->incMaxVariablesReturnOld(method);
            (instruction->result).type = IROFFSET;
            (instruction->result).internal_type = IROBJECT;
            (instruction->result).type_infos = NULL;
            memcpy(&(stack->stack[(stack->top) - obj_position]), &(instruction->result), sizeof(ir_item_t));
        }
    }

    /* duplicate the object on the top of the stack */
    memcpy(&(stack->stack[(stack->top)]), &(stack->stack[(stack->top) - obj_position]), sizeof(ir_item_t));
    assert((stack->stack[(stack->top)]).type == IROFFSET);
    objVarID = (stack->stack[stack->top]).value.v;
    objVarType = (stack->stack[stack->top]).type_infos;
    (stack->top)++;

    /* now we can do the test */
    translate_Test_Null_Reference(&(system->cliManager), method, cilStack, bytes_read, stack);

    /* pop the object from the stack */
    (stack->top)--;

    if (!(tempMethod.method)->attributes->is_virtual) {
        /* devirtualize call */
        JITINT16 result = translate_cil_call_by_methodID(&(system->cliManager), method, tempMethod.method, bytes_read, stack, 0, tempMethod.actualSignature, cilStack, current_label_ID, labels);
        PDEBUG("CILIR: translate_cil_vcall: Exit\n");
        return result;
    }

    /* Search the jump method	*/
    PDEBUG("CILIR: translate_cil_vcall:     Search the method\n");
    t_methods *methods = &((system->cliManager).methods);
    newMethod = methods->fetchOrCreateMethod(methods, tempMethod.method, JITFALSE);
    assert(newMethod != NULL);

    /* Assertions			*/
    assert(system != NULL);
    assert(newMethod->getID(newMethod) != NULL);

    /* retrieve the `methodVarID` value */
    methodVarID = method->incMaxVariablesReturnOld(method);

    /* retrieve the vtable index associated with the current method */
    vtableIndex = (system->garbage_collectors).gc.getIndexOfVTable(tempMethod.method);

    if (vtableIndex.useIMT) {

        imtOffset = (system->garbage_collectors).gc.fetchIMTOffset();

        /* Make the IR instruction to	*
         * load the pointer of the      *
         * virtual table		*/
        imtPointerVarID = method->incMaxVariablesReturnOld(method);
        instruction = method->newIRInstr(method);
        instruction->type = IRLOADREL;
        (instruction->param_1).value.v = objVarID;
        (instruction->param_1).type = IROFFSET;
        (instruction->param_1).internal_type = IROBJECT;
        (instruction->param_1).type_infos = objVarType;
        (instruction->param_2).value.v = (JITINT32) imtOffset;
        (instruction->param_2).type = IRINT32;
        (instruction->param_2).internal_type = IRINT32;
        (instruction->result).value.v = imtPointerVarID;
        (instruction->result).type = IROFFSET;
        (instruction->result).internal_type = IRNINT;
        instruction->byte_offset = bytes_read;

        /* Make the IR instruction to	*
         * load the bucket of the virtual *
         * table                        */
        bucketVarID = method->incMaxVariablesReturnOld(method);
        instruction = method->newIRInstr(method);
        instruction->type = IRADD;
        (instruction->param_1).value.v = imtPointerVarID;
        (instruction->param_1).type = IROFFSET;
        (instruction->param_1).internal_type = IRNINT;
        (instruction->param_2).value.v = (JITNUINT) (system->cliManager).translator.getIndexOfMethodDescriptorSymbol(&(system->cliManager), tempMethod.method);
        (instruction->param_2).type = IRSYMBOL;
        (instruction->param_2).internal_type = IRNINT;
        (instruction->result).value.v = bucketVarID;
        (instruction->result).type = IROFFSET;
        (instruction->result).internal_type = IRNINT;
        instruction->byte_offset = bytes_read;
        assert((instruction->param_1).internal_type == (instruction->param_2).internal_type);

        label_l1 = (*current_label_ID)++;
        label_l2 = (*current_label_ID)++;
        (*current_label_ID)++;

        instruction = method->newIRInstr(method);
        instruction->type = IRLABEL;
        instruction->byte_offset = bytes_read;
        (instruction->param_1).value.v = label_l1;
        (instruction->param_1).type = IRLABELITEM;
        (instruction->param_1).internal_type = IRLABELITEM;

        keyID = method->incMaxVariablesReturnOld(method);
        instruction = method->newIRInstr(method);
        instruction->type = IRLOADREL;
        (instruction->param_1).value.v = bucketVarID;
        (instruction->param_1).type = IROFFSET;
        (instruction->param_1).internal_type = IRNINT;
        (instruction->param_2).value.v = 0;
        (instruction->param_2).type = IRINT32;
        (instruction->param_2).internal_type = IRINT32;
        (instruction->result).value.v = keyID;
        (instruction->result).type = IROFFSET;
        (instruction->result).internal_type = IRNINT;
        instruction->byte_offset = bytes_read;

        /* test if method is equal */
        keyEqID = method->incMaxVariablesReturnOld(method);
        instruction = method->newIRInstr(method);
        instruction->type = IREQ;
        (instruction->param_1).value.v = keyID;
        (instruction->param_1).type = IROFFSET;
        (instruction->param_1).internal_type = IRNINT;
        (instruction->param_2).value.v = (JITNINT) (system->cliManager).translator.getMethodDescriptorSymbol(&(system->cliManager), tempMethod.method);
        (instruction->param_2).type = IRSYMBOL;
        (instruction->param_2).internal_type = IRNINT;
        (instruction->result).value.v = keyEqID;
        (instruction->result).type = IROFFSET;
        (instruction->result).internal_type = IRINT32;
        instruction->byte_offset = bytes_read;

        instruction = method->newIRInstr(method);
        instruction->type = IRBRANCHIF;
        instruction->byte_offset = bytes_read;
        (instruction->param_1).value.v = keyEqID;
        (instruction->param_1).type = IROFFSET;
        (instruction->param_1).internal_type = IRINT32;
        (instruction->param_2).value.v = label_l2;
        (instruction->param_2).type = IRLABELITEM;
        (instruction->param_2).internal_type = IRLABELITEM;

        /* load pointer of next imt slot in collision list */
        instruction = method->newIRInstr(method);
        instruction->type = IRLOADREL;
        (instruction->param_1).value.v = bucketVarID;
        (instruction->param_1).type = IROFFSET;
        (instruction->param_1).internal_type = IRNINT;
        (instruction->param_2).value.v = offsetof(IMTElem, next);
        (instruction->param_2).type = IRINT32;
        (instruction->param_2).internal_type = IRINT32;
        (instruction->result).value.v = bucketVarID;
        (instruction->result).type = IROFFSET;
        (instruction->result).internal_type = IRNINT;
        instruction->byte_offset = bytes_read;

        instruction = method->newIRInstr(method);
        instruction->type = IRBRANCH;
        instruction->byte_offset = bytes_read;
        (instruction->param_1).value.v = label_l1;
        (instruction->param_1).type = IRLABELITEM;
        (instruction->param_1).internal_type = IRLABELITEM;

        /* found method in imt load pointer */
        instruction = method->newIRInstr(method);
        instruction->type = IRLABEL;
        instruction->byte_offset = bytes_read;
        (instruction->param_1).value.v = label_l2;
        (instruction->param_1).type = IRLABELITEM;
        (instruction->param_1).internal_type = IRLABELITEM;

        instruction = method->newIRInstr(method);
        instruction->type = IRLOADREL;
        (instruction->param_1).value.v = bucketVarID;
        (instruction->param_1).type = IROFFSET;
        (instruction->param_1).internal_type = IRNINT;
        (instruction->param_2).value.v = offsetof(IMTElem, pointer);
        (instruction->param_2).type = IRINT32;
        (instruction->param_2).internal_type = IRINT32;
        (instruction->result).value.v = methodVarID;
        (instruction->result).type = IROFFSET;
        (instruction->result).internal_type = IRFPOINTER;
        instruction->byte_offset = bytes_read;

    } else {

        /* Clean the top of the stack           */
        stack->cleanTop(stack);
        vtableOffset = (system->garbage_collectors).gc.fetchVTableOffset();

        /* Compute the offset of the	*
         * virtual table, which is      *
         * constant for each object,    *
         * and for each type		*/

        /* Make the IR instruction to	*
         * load the pointer of the      *
         * virtual table		*/
        vtablePointerVarID = method->incMaxVariablesReturnOld(method);
        instruction2 = method->newIRInstr(method);
        instruction2->type = IRLOADREL;
        (instruction2->param_1).value.v = objVarID;
        (instruction2->param_1).type = IROFFSET;
        (instruction2->param_1).internal_type = IROBJECT;
        (instruction2->param_1).type_infos = objVarType;
        (instruction2->param_2).value.v = (JITINT32) vtableOffset;
        (instruction2->param_2).type = IRINT32;
        (instruction2->param_2).internal_type = IRINT32;
        (instruction2->result).value.v = vtablePointerVarID;
        (instruction2->result).type = IROFFSET;
        (instruction2->result).internal_type = IRNINT;
        instruction2->byte_offset = bytes_read;
        vtableVarID = vtablePointerVarID;

        /* Make the IR instruction to	*
         * load the slot of the virtual *
         * table                        */
        instruction2 = method->newIRInstr(method);
        instruction2->type = IRLOADREL;
        (instruction2->param_1).value.v = vtableVarID;
        (instruction2->param_1).type = IROFFSET;
        (instruction2->param_1).internal_type = IRNINT;
        (instruction2->param_2).value.v = vtableIndex.vtable_index * getIRSize(IRMPOINTER);
        (instruction2->param_2).type = IRINT32;
        (instruction2->param_2).internal_type = IRINT32;
        (instruction2->result).value.v = methodVarID;
        (instruction2->result).type = IROFFSET;
        (instruction2->result).internal_type = IRFPOINTER;
        instruction2->byte_offset = bytes_read;
    }

    /* Create the IR instruction		*/
    instruction = method->newIRInstr(method);

    /* Make a IR instruction to make an	*
     * indirect call			*/
    instruction->type = IRVCALL;
    (instruction->param_1).value.v = (IR_ITEM_VALUE) (JITNUINT) newMethod;
    (instruction->param_1).type = IRMETHODID;
    (instruction->param_1).internal_type = IRMETHODID;
    (instruction->param_2).value.v = isTail;
    (instruction->param_2).type = IRINT32;
    (instruction->param_2).internal_type = IRINT32;
    (instruction->param_3).value.v = methodVarID;
    (instruction->param_3).type = IROFFSET;
    (instruction->param_3).internal_type = IRFPOINTER;
    instruction->byte_offset = bytes_read;

    /* Create the IO of the IR call instruction	*/
    PDEBUG("CILIR: translate_cil_vcall:		Create the I/O of the IR call instruction\n");
    if (create_call_IO(&(system->cliManager), instruction, newMethod, stack, 0, method, NULL, bytes_read, cilStack, current_label_ID, labels) != 0) {
        print_err("CILIR: ERROR during creating the IO of the method. ", 0);
    }

    /* Return					*/
    PDEBUG("CILIR: translate_cil_vcall: Exit\n");
    return 0;
}

static inline JITINT16 translate_cil_castclass (t_system *system, Method method, CILStack cilStack, JITUINT32 token, GenericContainer *container, t_binary_information *binary, XanList *labels, JITUINT32 *current_label_ID, JITUINT32 bytes_read, t_stack *stack) {

    /* Assertions                           */
    assert(system != NULL);
    assert(method != NULL);
    assert(binary != NULL);
    assert(labels != NULL);
    assert(current_label_ID != NULL);
    assert(stack != NULL);

    /* Initialize the local variables       */
#ifdef PRINTDEBUG
    PDEBUG("CILIR: translate_cil_castclass: ENTERING CAST_CLASS ...\n");
    print_stack(stack, method->getParametersNumber(method), method->getLocalsNumber(method));
#endif

    if (translate_Test_Cast_Class(system, method, cilStack, current_label_ID, labels, bytes_read, stack, container, binary, token, 1) == 1) {
#ifdef PRINTDEBUG
        PDEBUG("CILIR: translate_cil_castclass: EXITING CAST_CLASS WITH ERRORS! \n");
        print_stack(stack, method->getParametersNumber(method), method->getLocalsNumber(method));
#endif
        return 1;
    }
#ifdef PRINTDEBUG
    PDEBUG("CILIR: translate_cil_castclass: EXITING CAST_CLASS ...\n");
    print_stack(stack, method->getParametersNumber(method), method->getLocalsNumber(method));
#endif

    return 0;
}

static inline JITINT16 translate_cil_gt (CLIManager_t *cliManager, Method method, JITUINT32 bytes_read, t_stack *stack) {
    ir_instruction_t        *instruction;
    JITUINT16 irType;

    /* Assertions			*/
    assert(method != NULL);
    assert(stack != NULL);

    /* Make the IREQ instruction	*/
    instruction = method->newIRInstr(method);
    instruction->type = IRGT;
    (stack->top)--;
    memcpy(&(instruction->param_2), &(stack->stack[stack->top]), sizeof(ir_item_t));
    (stack->top)--;
    memcpy(&(instruction->param_1), &(stack->stack[stack->top]), sizeof(ir_item_t));

    /* Clean the top of the stack           */
    stack->cleanTop(stack);

    /* Store the result			*/
    irType = IRINT32;
    stack->stack[(stack->top)].value.v = method->incMaxVariablesReturnOld(method);
    stack->stack[(stack->top)].type = IROFFSET;
    stack->stack[(stack->top)].internal_type = IRINT32;
    stack->stack[(stack->top)].type_infos = (cliManager->metadataManager)->getTypeDescriptorFromIRType(cliManager->metadataManager, irType);
    memcpy(&(instruction->result), &(stack->stack[stack->top]), sizeof(ir_item_t));
    (stack->top)++;
    instruction->byte_offset = bytes_read;

    /* Return				*/
    return 0;
}

static inline JITINT16 translate_cil_lt (CLIManager_t *cliManager, Method method, JITUINT32 bytes_read, t_stack *stack) {
    ir_instruction_t        *instruction;
    JITUINT16 irType;

    /* Assertions			*/
    assert(method != NULL);
    assert(stack != NULL);
    assert(stack->top >= (method->getParametersNumber(method) + method->getLocalsNumber(method)));

    /* Make the IREQ instruction	*/
    instruction = method->newIRInstr(method);
    instruction->type = IRLT;
    (stack->top)--;
    assert(stack->top >= (method->getParametersNumber(method) + method->getLocalsNumber(method)));
    memcpy(&(instruction->param_2), &(stack->stack[stack->top]), sizeof(ir_item_t));
    assert((instruction->param_2).internal_type != NOPARAM);
    (stack->top)--;
    assert(stack->top >= (method->getParametersNumber(method) + method->getLocalsNumber(method)));
    memcpy(&(instruction->param_1), &(stack->stack[stack->top]), sizeof(ir_item_t));
    assert((instruction->param_1).internal_type != NOPARAM);

    /* Clean the top of the stack           */
    stack->cleanTop(stack);

    /* Store the result			*/
    irType = IRINT32;
    stack->stack[(stack->top)].value.v = method->incMaxVariablesReturnOld(method);
    stack->stack[(stack->top)].type = IROFFSET;
    stack->stack[(stack->top)].internal_type = IRINT32;
    stack->stack[(stack->top)].type_infos = (cliManager->metadataManager)->getTypeDescriptorFromIRType(cliManager->metadataManager, irType);
    memcpy(&(instruction->result), &(stack->stack[stack->top]), sizeof(ir_item_t));
    (stack->top)++;
    assert(stack->top >= (method->getParametersNumber(method) + method->getLocalsNumber(method)));
    instruction->byte_offset = bytes_read;
    assert(stack->top >= (method->getParametersNumber(method) + method->getLocalsNumber(method)));

    /* Return				*/
    return 0;
}

static inline JITINT16 translate_cil_eq (CLIManager_t *cliManager, Method method, JITUINT32 bytes_read, t_stack *stack) {
    ir_instruction_t        *instruction;
    JITUINT16 irType;

    /* Assertions				*/
    assert(method != NULL);
    assert(stack != NULL);
    assert(stack->top >= (method->getParametersNumber(method) + method->getLocalsNumber(method)));

    /* Perform necessary conversions	*/
    if ((stack->stack[stack->top - 1]).internal_type != (stack->stack[stack->top - 2].internal_type)) {
        translate_cil_conv(cliManager, method, bytes_read, stack, (stack->stack[stack->top - 2]).internal_type, JITFALSE, JITFALSE, JITFALSE);
    }

    /* Make the IREQ instruction		*/
    instruction = method->newIRInstr(method);
    instruction->type = IREQ;
    (stack->top)--;
    assert(stack->top >= (method->getParametersNumber(method) + method->getLocalsNumber(method)));
    memcpy(&(instruction->param_1), &(stack->stack[stack->top]), sizeof(ir_item_t));
    assert((instruction->param_1).internal_type != NOPARAM);
    (stack->top)--;
    assert(stack->top >= (method->getParametersNumber(method) + method->getLocalsNumber(method)));
    memcpy(&(instruction->param_2), &(stack->stack[stack->top]), sizeof(ir_item_t));
    assert((instruction->param_2).internal_type != NOPARAM);

    /* Clean the top of the stack           */
    stack->cleanTop(stack);

    /* Store the result			*/
    irType = IRINT32;
    stack->stack[(stack->top)].value.v = method->incMaxVariablesReturnOld(method);
    stack->stack[(stack->top)].type = IROFFSET;
    stack->stack[(stack->top)].internal_type = irType;
    stack->stack[(stack->top)].type_infos = (cliManager->metadataManager)->getTypeDescriptorFromIRType(cliManager->metadataManager, irType);
    memcpy(&(instruction->result), &(stack->stack[stack->top]), sizeof(ir_item_t));
    (stack->top)++;
    assert(stack->top >= (method->getParametersNumber(method) + method->getLocalsNumber(method)));
    instruction->byte_offset = bytes_read;
    assert(stack->top >= (method->getParametersNumber(method) + method->getLocalsNumber(method)));

    /* Return				*/
    return 0;
}

static inline JITINT16 make_catcher_initialization_instructions (Method method, CILStack cilStack, JITUINT32 *current_label_ID, XanList *labels, JITUINT32 bytes_read, t_stack *stack, IR_ITEM_VALUE *exceptionVarID) {
    ir_instruction_t        *instruction;
    t_label                 *label_l1;
    XanList                 *icall_params;
    ir_item_t               *current_stack_item;

    /* assertions */
    assert(method != NULL);
    assert(cilStack != NULL);
    assert(current_label_ID != NULL);
    assert(labels != NULL);

    /* Initialize the local variables 	*/
    instruction = NULL;
    label_l1 = NULL;
    icall_params = NULL;
    current_stack_item = NULL;

    /* Allocate a new list			*/
    icall_params = xanList_new(sharedAllocFunction, freeFunction, NULL);
    assert(icall_params != NULL);

    /* Push the thrown object on the stack	*/
    if ((*exceptionVarID) == -1) {
        (*exceptionVarID)	= method->incMaxVariablesReturnOld(method);
    }
    assert((*exceptionVarID) != -1);
    stack->stack[stack->top].value.v = (*exceptionVarID);
    stack->stack[stack->top].type = IROFFSET;
    stack->stack[stack->top].internal_type = IROBJECT;
    stack->stack[(stack->top)].type_infos = NULL;

    /* In order, before starting to retrieve each try block in the method's "list of protected blocks"
     * we have to insert the `special IR instruction` called IRSTARTCATCHER. */
    PDEBUG("CILIR: MAKE_CATCHER_INIT : The catcher is started\n");
    instruction = method->newIRInstr(method);
    instruction->type = IRSTARTCATCHER;
    instruction->byte_offset = bytes_read;
    memcpy(&(instruction->result), &(stack->stack[stack->top]), sizeof(ir_item_t));
    (stack->top)++;

    /* Clean the top of the stack           */
    stack->cleanTop(stack);

    /* DUPLICATE THE TOP OF THE STACK */
    (stack->stack[(stack->top)]).value.v = (stack->stack[(stack->top) - 1]).value.v;
    (stack->stack[(stack->top)]).value.f = (stack->stack[(stack->top) - 1]).value.f;
    (stack->stack[(stack->top)]).type = (stack->stack[(stack->top) - 1]).type;
    (stack->stack[(stack->top)]).internal_type = (stack->stack[(stack->top) - 1]).internal_type;
    make_ir_infos(&(stack->stack[(stack->top)]), &(stack->stack[(stack->top) - 1]));

    PDEBUG("CILIR: MAKE_CATCHER_INIT : set-up the exception object \n");
    assert(current_stack_item == NULL);
    current_stack_item = make_new_IR_param();
    (current_stack_item->value).v = (stack->stack[(stack->top)]).value.v;
    (current_stack_item->value).f = (stack->stack[(stack->top)]).value.f;
    current_stack_item->type = (stack->stack[(stack->top)]).type;
    current_stack_item->internal_type = (stack->stack[(stack->top)]).internal_type;
    make_ir_infos(current_stack_item, &(stack->stack[(stack->top)]));
    assert(current_stack_item->internal_type == IROBJECT);
    xanList_append(icall_params, current_stack_item);
    assert(icall_params != NULL);
    translate_ncall(method, (JITINT8 *) "setCurrentThreadException", IRVOID, icall_params, bytes_read, 0, NULL);
    icall_params = NULL;

    /* Clean the top of the stack           */
    stack->cleanTop(stack);

    PDEBUG("CILIR: MAKE_CATCHER_INIT : isConstructed ?\n");
    assert(icall_params == NULL);
    stack->stack[stack->top].value.v = method->incMaxVariablesReturnOld(method);
    stack->stack[stack->top].type = IROFFSET;
    stack->stack[stack->top].internal_type = IRINT32;
    stack->stack[(stack->top)].type_infos = NULL;
    translate_ncall(method, (JITINT8 *) "isConstructed", IRINT32, NULL, bytes_read, stack->stack[stack->top].value.v, NULL);
    (stack->top)++;

    PDEBUG("CILIR: MAKE_CATCHER_INIT : test if object is constructed or not \n");
    /* test if the exception object is costructed or not */
    instruction = method->newIRInstr(method);
    instruction->type = IRBRANCHIF;
    instruction->byte_offset = bytes_read;
    (stack->top)--;
    (instruction->param_1).value.v = (stack->stack[stack->top]).value.v;
    (instruction->param_1).type = (stack->stack[stack->top]).type;
    (instruction->param_1).internal_type = (stack->stack[stack->top]).internal_type;
    assert((instruction->param_1).internal_type == IRINT32);
    label_l1 = (t_label *) allocMemory(sizeof(t_label));
    label_l1->ID = (*current_label_ID);
    label_l1->counter = 0;
    label_l1->ir_position = method->getInstructionsNumber(method) - 1;
    label_l1->byte_offset = bytes_read;
    label_l1->type = DEFAULT_LABEL_TYPE;
    cil_insert_label(cilStack, labels, label_l1, stack, -1);
    assert(IRMETHOD_getLabelInstruction(method->getIRMethod(method), label_l1->ID) == NULL);
    (*current_label_ID)++;
    (instruction->param_2).value.v = label_l1->ID;
    (instruction->param_2).type = IRLABELITEM;
    (instruction->param_2).internal_type = IRLABELITEM;

    PDEBUG("CILIR: MAKE_CATCHER_INIT : setConstructed \n");
    /* Load the integer constant '1' on the top of the stack */
    (stack->stack[stack->top]).value.v = 1;
    (stack->stack[stack->top]).type = IRINT32;
    (stack->stack[stack->top]).internal_type = IRINT32;
    stack->stack[(stack->top)].type_infos = NULL;

    icall_params = xanList_new(sharedAllocFunction, freeFunction, NULL);
    assert(icall_params != NULL);
    current_stack_item = make_new_IR_param();
    (current_stack_item->value).v = (stack->stack[(stack->top)]).value.v;
    (current_stack_item->value).f = (stack->stack[(stack->top)]).value.f;
    current_stack_item->type = (stack->stack[(stack->top)]).type;
    current_stack_item->internal_type = (stack->stack[(stack->top)]).internal_type;
    make_ir_infos(current_stack_item, &(stack->stack[(stack->top)]));
    xanList_append(icall_params, current_stack_item);
    translate_ncall(method, (JITINT8 *) "setConstructed", IRVOID, icall_params, bytes_read, 0, NULL);
    icall_params = NULL;

    /* Clean the top of the stack           */
    stack->cleanTop(stack);

    /* DUPLICATE THE TOP OF THE STACK. At this point, the last object present on the top of the stack is
     * the current exception object */
    (stack->stack[(stack->top)]).value.v = (stack->stack[(stack->top) - 1]).value.v;
    (stack->stack[(stack->top)]).value.f = (stack->stack[(stack->top) - 1]).value.f;
    (stack->stack[(stack->top)]).type = (stack->stack[(stack->top) - 1]).type;
    (stack->stack[(stack->top)]).internal_type = (stack->stack[(stack->top) - 1]).internal_type;
    make_ir_infos(&(stack->stack[(stack->top)]), &(stack->stack[(stack->top) - 1]));
    (stack->top)++;

    /* DUPLICATE THE TOP OF THE STACK. At this point, the last object present on the top of the stack is
     * the current exception object */
    (stack->stack[(stack->top)]).value.v = (stack->stack[(stack->top) - 1]).value.v;
    (stack->stack[(stack->top)]).value.f = (stack->stack[(stack->top) - 1]).value.f;
    (stack->stack[(stack->top)]).type = (stack->stack[(stack->top) - 1]).type;
    make_ir_infos(&(stack->stack[(stack->top)]), &(stack->stack[(stack->top) - 1]));
    (stack->stack[(stack->top)]).internal_type = (stack->stack[(stack->top) - 1]).internal_type;

    PDEBUG("CILIR: MAKE_CATCHER_INIT : retrieve the indirect function that represent the .ctor \n");
    /* retrieve the "indirect" function that represent the .ctor for the exception object */
    icall_params = xanList_new(sharedAllocFunction, freeFunction, NULL);
    assert(icall_params != NULL);
    current_stack_item = make_new_IR_param();
    memcpy(current_stack_item, &(stack->stack[(stack->top)]), sizeof(ir_item_t));
    xanList_append(icall_params, current_stack_item);
    stack->cleanTop(stack);
    (stack->stack[stack->top]).value.v = method->incMaxVariablesReturnOld(method);
    (stack->stack[stack->top]).type = IROFFSET;
    (stack->stack[stack->top]).internal_type = IRNINT;
    (stack->stack[stack->top]).type_infos = NULL;
    translate_ncall(method, (JITINT8 *) "fetchTheRightConstructor", IRNINT, icall_params, bytes_read, (stack->stack[stack->top]).value.v, NULL);
    (stack->top)++;

    PDEBUG("CILIR: MAKE_CATCHER_INIT : call the constructor on the exception-object \n");
    /* call the constructor on the exception-object */
    instruction = method->newIRInstr(method);
    instruction->type = IRICALL;
    (stack->top)--;
    memcpy(&(instruction->param_2), &(stack->stack[stack->top]), sizeof(ir_item_t));
    (instruction->param_1).value.v		= IRVOID;
    (instruction->param_1).internal_type 	= IRTYPE;
    (instruction->param_1).type		= (instruction->param_1).internal_type;
    (stack->top)--;
    instruction->callParameters		= xanList_new(allocFunction, freeFunction, NULL);
    instruction->byte_offset 		= bytes_read;

    /* now the object is initialized. The next step is to define the label `label` and set-up correctly
     * the classID parameter */
    instruction = method->newIRInstr(method);
    instruction->type = IRLABEL;
    (instruction->param_1).value.v = label_l1->ID;
    (instruction->param_1).type = IRLABELITEM;
    (instruction->param_1).internal_type = IRLABELITEM;
    instruction->byte_offset = bytes_read;

    /* finally the initial part of the catcher is completed. At this point the stack is empty except for the
     * exception_object that is located on the top of the stack */

    return 0;
}

static inline JITINT16 make_catcher_instructions (t_system *system, Method method, CILStack cilStack, JITUINT32 *current_label_ID, XanList *labels, JITUINT32 bytes_read, t_stack *stack, IR_ITEM_VALUE *exceptionVarID) {
    ir_instruction_t        *instruction;
    JITUINT32 rethrow_unhandled_label_ID;

    /* assertions */
    assert(system != NULL);
    assert(method != NULL);
    assert(cilStack != NULL);
    assert(current_label_ID != NULL);
    assert(labels != NULL);

    /* Check if runtime checks are enabled		*/
    if (    (!(system->cliManager.CLR).runtimeChecks)                                        &&
            (!IRPROGRAM_doesMethodBelongToALibrary(method->getIRMethod(method)))             ) {

        /* The runtime checks have been disabled		*/
        return 0;
    }

    if (method->hasCatcher(method)) {
        ir_method_t		*irMethod;
        ir_instruction_t	*catchInst;

        /* Fetch the IR method			*/
        irMethod	= method->getIRMethod(method);
        assert(irMethod != NULL);

        /* We have to make the "initialization instructions" of the catcher block. These instructions are used
         * to manage the construction process of a built-in exception that can be raised during
         * the program execution                                                                                */
        make_catcher_initialization_instructions(method, cilStack, current_label_ID, labels, bytes_read, stack, exceptionVarID);

        /* We define the label-ID of the rethrow_unhandled_label */
        rethrow_unhandled_label_ID = (*current_label_ID);
        (*current_label_ID)++;

        /* Call the recursive function that define the correct IR code of each block information */
        PDEBUG("Starting the analysis of the blocks... \n");
        make_blocks_instructions(system, method, method->getTryBlocks(method), current_label_ID, rethrow_unhandled_label_ID, bytes_read, stack);
        PDEBUG("Ended the analysis of the blocks \n");

        /* We must define the label `rethrow_unhandled_label` */
        PDEBUG("Defining the label of the \"rethrow_unhandled\" instruction... \n");
        instruction = method->newIRInstr(method);
        instruction->type = IRLABEL;
        (instruction->param_1).value.v = rethrow_unhandled_label_ID;
        (instruction->param_1).type = IRLABELITEM;
        (instruction->param_1).internal_type = IRLABELITEM;
        instruction->byte_offset = bytes_read;

        /* finally we end the method body defining the rethrow_unandled IR instruction */
        PDEBUG("Defining the \"rethrow_unhandled\" instruction... \n");
        catchInst			= IRMETHOD_getCatcherInstruction(irMethod);
        assert(catchInst != NULL);
        instruction 			= method->newIRInstr(method);
        instruction->type 		= IRTHROW;
        instruction->byte_offset 	= bytes_read;
        memcpy(&(instruction->param_1), &(catchInst->result), sizeof(ir_item_t));

        /* Before exiting the function, we must assure that the stack is empty. At this point there's only
         * one element on the stack. This element is the exception object */
        (stack->top)--;
    }

    return 0;
}

static inline void make_blocks_instructions (t_system *system, Method method, XanList *blocks, JITUINT32 *current_label_ID, JITUINT32 exit_label, JITUINT32 bytes_read, t_stack *stack) {
    ir_instruction_t        *instruction;
    XanListItem             *current_block;
    t_try_block             *current_try_block;
    JITUINT32 label_target_ID;

    /* assertions	*/
    assert(method != NULL);
    assert(current_label_ID != NULL);
    assert(system != NULL);

    if (blocks == NULL) {
        return;
    }

    /* initialize the local variables */
    label_target_ID = -1;

    /* retrieve the first element in the list */
    current_block =xanList_first(blocks);
    assert(current_block != NULL);

    while (current_block != NULL) {
        JITUINT32 label_from_ID;
        JITUINT32 label_to_ID;
        JITUINT32 label_start_handlers_ID;
        JITUINT32 label_check_handlers_ID;
        JITUINT32 label_check_finally_handlers_ID;
        JITUINT32 label_start_finally_handlers_ID;
        JITUINT32 label_target_handler_ID;
        JITUINT32 label_next_filter_or_catch_ID;
        JITBOOLEAN has_catch_handlers;
        JITBOOLEAN has_finally_handlers;
        XanListItem     *current_handler;
        t_try_handler   *current_try_handler;

        /* initialize the local variables */
        label_start_handlers_ID = -1;
        label_check_handlers_ID = -1;
        label_check_finally_handlers_ID = -1;
        label_start_finally_handlers_ID = -1;
        label_target_handler_ID = -1;
        label_next_filter_or_catch_ID = -1;
        PDEBUG("Found a protected block... \n");

        /* Check if I have to insert a label here */
        if (current_block->prev != NULL) {

            /* create a label instruction */
            instruction = method->newIRInstr(method);
            instruction->type = IRLABEL;
            (instruction->param_1).value.v = label_target_ID;
            (instruction->param_1).type = IRLABELITEM;
            (instruction->param_1).internal_type = IRLABELITEM;
            instruction->byte_offset = bytes_read;
            PDEBUG("Added the label \"L%d\"for the current block... \n", label_target_ID);
        }

        /* retrieve the current block */
        current_try_block = (t_try_block *) current_block->data;
        assert(current_try_block != NULL);

        /* fetch the label informations */
        label_from_ID = current_try_block->try_start_label_ID;
        label_to_ID = current_try_block->try_end_label_ID;

        /* fetch the label of the next sibling block or parent block */
        if (current_block->next == NULL) {
            label_target_ID = exit_label;
        } else {
            label_target_ID = (*current_label_ID);
            (*current_label_ID)++;
        }

        /* test if the protected block defines an handler or a finally */
        if (current_try_block->catch_handlers != NULL) {
            has_catch_handlers = 1;
        } else {
            has_catch_handlers = 0;
        }

        if (current_try_block->finally_handlers != NULL) {
            has_finally_handlers = 1;
        } else {
            has_finally_handlers = 0;
        }

        assert(has_catch_handlers || has_finally_handlers);

        if (has_catch_handlers) {
            /* create the ID for the label "start-handler-blocks" */
            label_start_handlers_ID = (*current_label_ID);
            (*current_label_ID)++;

            label_check_handlers_ID = (*current_label_ID);
            (*current_label_ID)++;
        }

        /* check if are available finally or fault handlers */
        if (has_finally_handlers) {
            /* create the ID for the label "start-finally-handler-blocks" */
            label_start_finally_handlers_ID = (*current_label_ID);
            (*current_label_ID)++;
            /* create the ID for the label "check-finally-handler-blocks" */
            label_check_finally_handlers_ID = (*current_label_ID);
            (*current_label_ID)++;
        }

        /* create the IRBRANCHIFPCNOTINRANGE instruction */
        instruction = method->newIRInstr(method);
        instruction->type = IRBRANCHIFPCNOTINRANGE;
        (instruction->param_1).value.v = label_from_ID;
        (instruction->param_1).type = IRLABELITEM;
        (instruction->param_1).internal_type = IRLABELITEM;
        (instruction->param_2).value.v = label_to_ID;
        (instruction->param_2).type = IRLABELITEM;
        (instruction->param_2).internal_type = IRLABELITEM;
        if (has_catch_handlers) {
            (instruction->param_3).value.v = label_check_handlers_ID;
        } else {
            (instruction->param_3).value.v = label_check_finally_handlers_ID;
        }
        (instruction->param_3).type = IRLABELITEM;
        (instruction->param_3).internal_type = IRLABELITEM;
        instruction->byte_offset = bytes_read;
        PDEBUG("Added a IRBRANCHIFPCNOTINRANGE instruction... \n");

        /* generate the catcher-instructions for each inner try_blocks of the current try_block */
        if (current_try_block->inner_try_blocks != NULL) {
            PDEBUG("Found inner protected blocks for the current block... \n");
            /* recursive call... */
            if (has_catch_handlers) {
                make_blocks_instructions(       system, method,
                                                current_try_block->inner_try_blocks,
                                                current_label_ID,
                                                label_start_handlers_ID,
                                                bytes_read, stack);
            } else {
                make_blocks_instructions(       system, method,
                                                current_try_block->inner_try_blocks,
                                                current_label_ID,
                                                label_start_finally_handlers_ID,
                                                bytes_read, stack);
            }
        }

        /* create a label instruction to mark the start of the handlers' section*/
        if (has_catch_handlers) {
            PDEBUG("This block defines catch handlers... \n");
            if (current_try_block->inner_try_blocks != NULL) {
                instruction = method->newIRInstr(method);
                instruction->type = IRLABEL;
                (instruction->param_1).value.v = label_start_handlers_ID;
                (instruction->param_1).type = IRLABELITEM;
                (instruction->param_1).internal_type = IRLABELITEM;
                instruction->byte_offset = bytes_read;
            }

            /* manage the catch and filter handlers associated with this protected block */
            assert(current_try_block->catch_handlers != NULL);
            current_handler = xanList_first(current_try_block->catch_handlers);
            assert(current_handler != NULL);
            while (current_handler != NULL) {
                current_try_handler = (t_try_handler *) current_handler->data;
                assert(current_try_handler != NULL);

                if (current_handler->prev != NULL) {

                    /* create a label instruction */
                    PDEBUG("A new catch handler... \n");
                    instruction = method->newIRInstr(method);
                    instruction->type = IRLABEL;
                    (instruction->param_1).value.v = label_next_filter_or_catch_ID;
                    (instruction->param_1).type = IRLABELITEM;
                    (instruction->param_1).internal_type = IRLABELITEM;
                    instruction->byte_offset = bytes_read;
                }

                /* verify if there is another catch or filter block */
                if (current_handler->next == NULL) {
                    if (has_finally_handlers) {
                        label_next_filter_or_catch_ID = label_start_finally_handlers_ID;
                    } else {
                        label_next_filter_or_catch_ID = exit_label;
                    }
                } else {
                    label_next_filter_or_catch_ID = (*current_label_ID);
                    (*current_label_ID)++;
                }

                if (current_try_handler->type == EXCEPTION_TYPE) {

                    /* assertions */
                    assert(current_try_handler->class != NULL);

                    PDEBUG("THE CURRENT HANDLER IS A CATCH HANDLER... %p\n", current_try_handler->class);

                    make_isSubType(system, method, current_label_ID, bytes_read, stack, (system->exception_system)._System_ExecutionEngineException_ID);

                    /* Insert the IRBRANCHIFNOT instruction	*/
                    instruction = method->newIRInstr(method);
                    instruction->type = IRBRANCHIFNOT;
                    instruction->byte_offset = bytes_read;
                    (stack->top)--;
                    (instruction->param_1).value.v = (stack->stack[stack->top]).value.v;
                    (instruction->param_1).type = (stack->stack[stack->top]).type;
                    (instruction->param_1).internal_type =
                        (stack->stack[stack->top]).internal_type;
                    assert((instruction->param_1).type == IROFFSET);
                    assert( (instruction->param_1).internal_type == IRINT32 );
                    (instruction->param_2).value.v = (*current_label_ID);
                    (*current_label_ID)++;
                    (instruction->param_2).type = IRLABELITEM;
                    (instruction->param_2).internal_type = IRLABELITEM;

                    /* jump to the next filter or catch handler */
                    instruction = method->newIRInstr(method);
                    instruction->type = IRBRANCH;
                    (instruction->param_1).value.v = label_next_filter_or_catch_ID;
                    (instruction->param_1).type = IRLABELITEM;
                    (instruction->param_1).internal_type = IRLABELITEM;
                    instruction->byte_offset = bytes_read;
                    assert((instruction->param_1).type == IRLABELITEM);

                    /* create a label instruction */
                    instruction = method->newIRInstr(method);
                    instruction->type = IRLABEL;
                    (instruction->param_1).value.v = (*current_label_ID) - 1;
                    (instruction->param_1).type = IRLABELITEM;
                    (instruction->param_1).internal_type = IRLABELITEM;
                    instruction->byte_offset = bytes_read;

                    /* test if the exception object is a catchable exception or not */
                    make_isSubType(system, method, current_label_ID, bytes_read, stack, current_try_handler->class);
                } else {
                    assert(current_try_handler->type == FILTER_TYPE);
                    PDEBUG("THE CURRENT HANDLER IS A FILTER HANDLER... \n");

                    /* Call the filter procedure */
                    instruction = method->newIRInstr(method);
                    instruction->type = IRCALLFILTER;
                    (instruction->param_1).value.v = current_try_handler->filter_start_label_ID;
                    (instruction->param_1).type = IRLABELITEM;
                    (instruction->param_1).internal_type = IRLABELITEM;

                    /* Clean the top of the stack           */
                    stack->cleanTop(stack);

                    stack->stack[stack->top].value.v = method->incMaxVariablesReturnOld(method);
                    stack->stack[stack->top].type = IROFFSET;
                    stack->stack[stack->top].internal_type = IRINT32;
                    (stack->stack[stack->top]).type_infos = NULL;
                    memcpy(&(instruction->result), &(stack->stack[stack->top]), sizeof(ir_item_t));
                    (stack->top)++;

                    /* we must insert a label for data-flow analysis purpose only*/
                    /* create a label instruction */
                    instruction = method->newIRInstr(method);
                    instruction->type = IRLABEL;
                    (instruction->param_1).value.v = (*current_label_ID);
                    (*current_label_ID)++;
                    (instruction->param_1).type = IRLABELITEM;
                    (instruction->param_1).internal_type = IRLABELITEM;
                    instruction->byte_offset = bytes_read;

                    /* update the endFinally or endFilter instruction inserting the correct
                     * data-flow informations */
                    assert(current_try_handler->end_filter_or_finally_inst != NULL);
                    assert(current_try_handler->end_filter_or_finally_inst->type == IRENDFILTER);
                    assert((current_try_handler->end_filter_or_finally_inst->param_2).type == NOPARAM);
                    assert((current_try_handler->end_filter_or_finally_inst->param_1).type == IRINT32);
                    (current_try_handler->end_filter_or_finally_inst->param_2).value.v = (*current_label_ID) - 1;
                    (current_try_handler->end_filter_or_finally_inst->param_2).type = IRLABELITEM;
                    (current_try_handler->end_filter_or_finally_inst->param_2).internal_type = IRLABELITEM;
                }

                /* Insert the IRBRANCHIFNOT instruction	*/
                instruction = method->newIRInstr(method);
                instruction->type = IRBRANCHIFNOT;
                instruction->byte_offset = bytes_read;
                (stack->top)--;
                (instruction->param_1).value.v = (stack->stack[stack->top]).value.v;
                (instruction->param_1).type = (stack->stack[stack->top]).type;
                (instruction->param_1).internal_type = (stack->stack[stack->top]).internal_type;
                assert((instruction->param_1).type == IROFFSET);
                assert( (instruction->param_1).internal_type == IRINT32 );
                (instruction->param_2).value.v = label_next_filter_or_catch_ID;
                (instruction->param_2).type = IRLABELITEM;
                (instruction->param_2).internal_type = IRLABELITEM;

                /* call the handler associated with this catch or filter */
                instruction = method->newIRInstr(method);
                instruction->type = IRBRANCH;
                (instruction->param_1).value.v = current_try_handler->handler_start_label_ID;
                (instruction->param_1).type = IRLABELITEM;
                (instruction->param_1).internal_type = IRLABELITEM;
                instruction->byte_offset = bytes_read;
                assert((instruction->param_1).type == IRLABELITEM);

                /* termination condition */
                current_handler = current_handler->next;
            }

            PDEBUG("STARTING THE CHECK_HANDLERS_BLOCK ... \n");
            instruction = method->newIRInstr(method);
            instruction->type = IRLABEL;
            (instruction->param_1).value.v = label_check_handlers_ID;
            (instruction->param_1).type = IRLABELITEM;
            (instruction->param_1).internal_type = IRLABELITEM;
            instruction->byte_offset = bytes_read;

            /* test if the exception has occurred into a catch or filter handler associated
             * with this protected block */
            assert(current_try_block->catch_handlers != NULL);
            current_handler = xanList_first(current_try_block->catch_handlers);
            assert(current_handler != NULL);
            while (current_handler != NULL) {
                current_try_handler = (t_try_handler *) current_handler->data;
                assert(current_try_handler != NULL);

                /* Check if I have to insert a label here */
                if (current_handler->prev != NULL) {

                    /* create a label instruction */
                    instruction = method->newIRInstr(method);
                    instruction->type = IRLABEL;
                    (instruction->param_1).value.v = label_target_handler_ID;
                    (instruction->param_1).type = IRLABELITEM;
                    (instruction->param_1).internal_type = IRLABELITEM;
                    instruction->byte_offset = bytes_read;
                }

                if (current_handler->next == NULL) {
                    if (has_finally_handlers) {
                        label_target_handler_ID = label_check_finally_handlers_ID;
                    } else {
                        label_target_handler_ID = label_target_ID;
                    }
                } else {
                    label_target_handler_ID = (*current_label_ID);
                    (*current_label_ID)++;
                }

                /* manage the current_handler */
                label_from_ID = current_try_handler->handler_start_label_ID;
                label_to_ID = current_try_handler->handler_end_label_ID;

                /* create the IRBRANCHIFPCNOTINRANGE instruction to the start-handler section */
                instruction = method->newIRInstr(method);
                instruction->type = IRBRANCHIFPCNOTINRANGE;
                (instruction->param_1).value.v = label_from_ID;
                (instruction->param_1).type = IRLABELITEM;
                (instruction->param_1).internal_type = IRLABELITEM;
                (instruction->param_2).value.v = label_to_ID;
                (instruction->param_2).type = IRLABELITEM;
                (instruction->param_2).internal_type = IRLABELITEM;
                (instruction->param_3).value.v = label_target_handler_ID;
                (instruction->param_3).type = IRLABELITEM;
                (instruction->param_3).internal_type = IRLABELITEM;
                instruction->byte_offset = bytes_read;

                /* The instruction that must be executed in this case */
                instruction = method->newIRInstr(method);
                instruction->type = IRBRANCH;
                if (has_finally_handlers) {
                    (instruction->param_1).value.v = label_start_finally_handlers_ID;
                } else {
                    (instruction->param_1).value.v = exit_label;
                }
                (instruction->param_1).type = IRLABELITEM;
                (instruction->param_1).internal_type = IRLABELITEM;
                instruction->byte_offset = bytes_read;
                assert((instruction->param_1).type == IRLABELITEM);

                /* termination condition */
                current_handler = current_handler->next;
            }
        }

        if ( (has_catch_handlers == 0) && (current_try_block->inner_try_blocks == NULL) ) {
            /* The instruction that must be executed in this case */
            instruction = method->newIRInstr(method);
            instruction->type = IRBRANCH;
            (instruction->param_1).value.v = label_start_finally_handlers_ID;
            (instruction->param_1).type = IRLABELITEM;
            (instruction->param_1).internal_type = IRLABELITEM;
            instruction->byte_offset = bytes_read;
            assert((instruction->param_1).type == IRLABELITEM);
        }

        if (has_finally_handlers) {

            /* create a label instruction to mark the start of the check-handlers section*/
            instruction = method->newIRInstr(method);
            instruction->type = IRLABEL;
            (instruction->param_1).value.v = label_check_finally_handlers_ID;
            (instruction->param_1).type = IRLABELITEM;
            (instruction->param_1).internal_type = IRLABELITEM;
            instruction->byte_offset = bytes_read;

            /* manage the fault and finally handlers associated with this protected block */
            assert(current_try_block->finally_handlers != NULL);
            current_handler = xanList_first(current_try_block->finally_handlers);
            assert(current_handler != NULL);
            while (current_handler != NULL) {
                current_try_handler = (t_try_handler *) current_handler->data;
                assert(current_try_handler != NULL);

                /* Check if I have to insert a label here */
                if (current_handler->prev != NULL) {

                    /* create a label instruction */
                    instruction = method->newIRInstr(method);
                    instruction->type = IRLABEL;
                    (instruction->param_1).value.v = label_target_handler_ID;
                    (instruction->param_1).type = IRLABELITEM;
                    (instruction->param_1).internal_type = IRLABELITEM;
                    instruction->byte_offset = bytes_read;
                }

                if (current_handler->next == NULL) {
                    label_target_handler_ID = label_target_ID;
                } else {
                    label_target_handler_ID = (*current_label_ID);
                    (*current_label_ID)++;
                }

                /* manage the current finally handler */
                label_from_ID = current_try_handler->handler_start_label_ID;
                label_to_ID = current_try_handler->handler_end_label_ID;

                /* create the IRBRANCHIFPCNOTINRANGE instruction */
                instruction = method->newIRInstr(method);
                instruction->type = IRBRANCHIFPCNOTINRANGE;
                (instruction->param_1).value.v = label_from_ID;
                (instruction->param_1).type = IRLABELITEM;
                (instruction->param_1).internal_type = IRLABELITEM;
                (instruction->param_2).value.v = label_to_ID;
                (instruction->param_2).type = IRLABELITEM;
                (instruction->param_2).internal_type = IRLABELITEM;
                (instruction->param_3).value.v = label_target_handler_ID;
                (instruction->param_3).type = IRLABELITEM;
                (instruction->param_3).internal_type = IRLABELITEM;
                instruction->byte_offset = bytes_read;

                /* The instruction that must be executed in this case */
                instruction = method->newIRInstr(method);
                instruction->type = IRBRANCH;
                (instruction->param_1).value.v = exit_label;
                (instruction->param_1).type = IRLABELITEM;
                (instruction->param_1).internal_type = IRLABELITEM;
                instruction->byte_offset = bytes_read;
                assert((instruction->param_1).type == IRLABELITEM);

                /* termination condition */
                current_handler = current_handler->next;
            }

            /* manage the fault and finally handlers associated with this protected block */
            assert(current_try_block->finally_handlers != NULL);
            current_handler = xanList_first(current_try_block->finally_handlers);
            assert(current_handler != NULL);

            /* create a label instruction for the finally handlers */
            instruction = method->newIRInstr(method);
            instruction->type = IRLABEL;
            (instruction->param_1).value.v = label_start_finally_handlers_ID;
            (instruction->param_1).type = IRLABELITEM;
            (instruction->param_1).internal_type = IRLABELITEM;
            instruction->byte_offset = bytes_read;

            while (current_handler != NULL) {
                current_try_handler = (t_try_handler *) current_handler->data;
                assert(current_try_handler != NULL);
                assert(current_try_handler->end_filter_or_finally_inst != NULL);
                assert(current_try_handler->end_filter_or_finally_inst->type == IRENDFINALLY);
                assert((current_try_handler->end_filter_or_finally_inst->param_2).type == NOPARAM);
//				assert((current_try_handler->end_filter_or_finally_inst->param_1).type == NOPARAM);

                /* manage the current finally handler */
                instruction = method->newIRInstr(method);
                instruction->type = IRCALLFINALLY;
                (instruction->param_1).value.v = current_try_handler->handler_start_label_ID;
                (instruction->param_1).type = IRLABELITEM;
                (instruction->param_1).internal_type = IRLABELITEM;
                instruction->byte_offset = bytes_read;

                /* termination condition */
                current_handler = current_handler->next;

                /* we must insert a label for data-flow analysis purpose only*/
                /* create a label instruction */
                instruction = method->newIRInstr(method);
                instruction->type = IRLABEL;
                (instruction->param_1).value.v = (*current_label_ID);
                (*current_label_ID)++;
                (instruction->param_1).type = IRLABELITEM;
                (instruction->param_1).internal_type = IRLABELITEM;
                instruction->byte_offset = bytes_read;
            }

            /* jump to the parent protected-block */
            instruction = method->newIRInstr(method);
            instruction->type = IRBRANCH;
            (instruction->param_1).value.v = exit_label;
            (instruction->param_1).type = IRLABELITEM;
            (instruction->param_1).internal_type = IRLABELITEM;
            instruction->byte_offset = bytes_read;
            assert((instruction->param_1).type == IRLABELITEM);
        }

        /* termination condition */
        current_block = current_block->next;
    }
}

static inline JITINT16 translate_ncall (Method method, JITINT8 *function_name, JITUINT32 result, XanList *params, JITUINT32 bytes_read, JITUINT32 result_variable_ID, ir_instruction_t *before) {
    ir_instruction_t         *instruction;

    /* Assertions           */
    assert(method != NULL);

    /* Now we must create the call instruction */
    /* Insert the new instruction before the specified one (if any) */
    instruction = method->newIRInstrBefore(method, before);
    instruction->type = IRNATIVECALL;
    (instruction->param_1).value.v = result;
    (instruction->param_1).type = IRTYPE;
    (instruction->param_1).internal_type = IRTYPE;
    if (function_name != NULL) {
        (instruction->param_2).value.v = (JITNUINT) function_name;
        (instruction->param_2).type = IRSTRING;
        (instruction->param_2).internal_type = IRSTRING;
    }

    if ((instruction->param_1).value.v != IRVOID ) {
        (instruction->result).value.v = result_variable_ID;
        (instruction->result).type = IROFFSET;
        (instruction->result).internal_type = (instruction->param_1).value.v;
    } else {
        (instruction->result).type = NOPARAM;
    }
    instruction->byte_offset = bytes_read;
    instruction->callParameters = params;
    (instruction->param_3).value.v = (IR_ITEM_VALUE) (JITNUINT) COMPILERCALL_getSymbol(function_name);
    (instruction->param_3).type = IRSYMBOL;
    (instruction->param_3).internal_type = IRFPOINTER;

    /* Return					*/
    return 0;
}

static inline JITUINT32 translate_Test_Null_Reference_With_Static_Field_Check (CLIManager_t *cliManager, Method method, CILStack cilStack, JITUINT32 *current_label_ID, XanList *labels, JITUINT32 bytes_read, t_stack *stack, FieldDescriptor *fieldID) {

    /* Assertions					*/
    assert(cliManager != NULL);
    assert(method != NULL);
    assert(stack != NULL);
    assert(labels != NULL);
    assert(cilStack != NULL);
    assert(fieldID != NULL);

    /* field is static ? */
    if (!fieldID->attributes->is_static) {
        translate_Test_Null_Reference(cliManager, method, cilStack, bytes_read, stack);
    }

    return 0;
}

static inline JITUINT32 translate_Test_Field_Access (t_system *system, Method method, CILStack cilStack, JITUINT32 *current_label_ID, XanList *labels, JITUINT32 bytes_read, t_stack *stack, FieldDescriptor *fieldLocated) {

    /* Assertions					*/
    assert(method != NULL);
    assert(stack != NULL);
    assert(cilStack != NULL);
    assert(labels != NULL);
    assert(system != NULL);
    assert(fieldLocated != NULL);

    /* test if the field is accessible */
    MethodDescriptor *cil_method = method->getID(method);
    if (!fieldLocated->isAccessible(fieldLocated, cil_method)) {
        _translate_throw_CIL_Exception(&(system->cliManager), method, cilStack, current_label_ID, labels, bytes_read, stack, (system->exception_system)._System_FieldAccessException_ID);
        return 1;
    }

    return 0;
}

static inline JITUINT32 translate_Test_Null_Reference (CLIManager_t *cliManager, Method method, CILStack cilStack, JITUINT32 bytes_read, t_stack *stack) {
#ifdef DEBUG
    JITUINT32 stackTop;
#endif

    /* Assertions					*/
    assert(cliManager != NULL);
    assert(method != NULL);
    assert(stack != NULL);
    assert(cilStack != NULL);

    /* Check if runtime checks are enabled		*/
    if (!(cliManager->CLR).runtimeChecks) {

        /* The runtime checks have been disabled		*/
        return 0;
    }

    /* Fetch the stack top						*/
#ifdef DEBUG
    stackTop = stack->top;
#endif

    /* Translate the runtime check			*/
    translate_cil_checknull(method, bytes_read, stack);

    /* Check the stack top				*/
#ifdef DEBUG
    assert(stackTop == stack->top);
#endif

    /* Return					*/
    return 0;
}

static inline JITUINT32 translate_Test_Array_Type_Mismatch (t_system *system, Method method, CILStack cilStack, JITUINT32 *current_label_ID, XanList *labels, JITUINT32 bytes_read, t_stack *stack, TypeDescriptor *classLocated) {
    TypeDescriptor *arrayType;
    JITUINT32 typePointerVarID;
    JITUINT32 keyEqID;
    JITUINT32 branch_label_ID;
    JITINT32 typeDescriptorOffset;
    ir_instruction_t        *instruction;
    ir_instruction_t	*loadInstruction;

#ifdef DEBUG
    JITUINT32 stackTop;
#endif

    /* assertions */
    assert(system != NULL);
    assert(method != NULL);
    assert(current_label_ID != NULL);
    assert(labels != NULL);
    assert(stack != NULL);
    assert(classLocated != NULL);
    assert(cilStack != NULL);
    assert(classLocated != NULL);

    /* Check if runtime checks are enabled		*/
    if (!(system->cliManager.CLR).runtimeChecks) {

        /* The runtime checks have been disabled		*/
        return 0;
    }

    /* Fetch the stack top						*/
#ifdef DEBUG
    stackTop = stack->top;
#endif

    PDEBUG("TYPE OF THE CLASSLOCATED: %s \n", classLocated->getCompleteName(classLocated));
    arrayType = classLocated->makeArrayDescriptor(classLocated, 1);
    typeDescriptorOffset = (system->garbage_collectors).gc.fetchTypeDescriptorOffset();

    /* Make the IR instruction to	*
     * load the pointer of the      *
     * virtual table		*/
    typePointerVarID = method->incMaxVariablesReturnOld(method);
    loadInstruction = method->newIRInstr(method);
    loadInstruction->type = IRLOADREL;
    memcpy(&(loadInstruction->param_1), &(stack->stack[(stack->top) - 1]), sizeof(ir_item_t));
    (loadInstruction->param_2).value.v = (JITINT32) typeDescriptorOffset;
    (loadInstruction->param_2).type = IRINT32;
    (loadInstruction->param_2).internal_type = IRINT32;
    (loadInstruction->result).value.v = typePointerVarID;
    (loadInstruction->result).type = IROFFSET;
    (loadInstruction->result).internal_type = IRNUINT;
    loadInstruction->byte_offset = bytes_read;

    /* test if method is equal */
    keyEqID = method->incMaxVariablesReturnOld(method);
    instruction = method->newIRInstr(method);
    instruction->type = IREQ;
    memcpy(&(instruction->param_1), &(loadInstruction->result), sizeof(ir_item_t));
    (instruction->param_2).value.v = (JITNUINT) arrayType;
    (instruction->param_2).type = IRNUINT;
    (instruction->param_2).internal_type = (instruction->param_2).type;
    (instruction->result).value.v = keyEqID;
    (instruction->result).type = IROFFSET;
    (instruction->result).internal_type = IRINT32;
    instruction->byte_offset = bytes_read;
    assert((instruction->param_1).internal_type == (instruction->param_2).internal_type);

    /* Insert the IRBRANCHIF instruction	*/
    branch_label_ID = (*current_label_ID)++;
    instruction = method->newIRInstr(method);
    instruction->type = IRBRANCHIF;
    (instruction->param_1).value.v = keyEqID;
    (instruction->param_1).type = IROFFSET;
    (instruction->param_1).internal_type = IRINT32;
    (instruction->param_2).value.v = branch_label_ID;
    (instruction->param_2).type = IRLABELITEM;
    (instruction->param_2).internal_type = IRLABELITEM;
    instruction->byte_offset = bytes_read;

    /* translate the THROW System.InvalidCastException */
    /* the unbox instruction must raise a System.InvalidCastException */
    _translate_throw_CIL_Exception(&(system->cliManager), method, cilStack, current_label_ID, labels, bytes_read, stack, (system->exception_system)._System_ArrayTypeMismatchException_ID);

    /* create a label instruction */
    instruction = method->newIRInstr(method);
    instruction->type = IRLABEL;
    (instruction->param_1).value.v = branch_label_ID;
    (instruction->param_1).type = IRLABELITEM;
    (instruction->param_1).internal_type = IRLABELITEM;
    instruction->byte_offset = bytes_read;

    /* Check the stack top				*/
#ifdef DEBUG
    assert(stackTop == stack->top);
#endif

    /* Return					*/
    return 0;
}

static inline JITUINT32 translate_Test_Array_Index_Out_Of_Range (t_system *system, Method method, CILStack cilStack, JITUINT32 *current_label_ID, XanList *labels, JITUINT32 bytes_read, t_stack *stack) {
    ir_instruction_t        *instruction;
    t_label label_l1;

#ifdef DEBUG
    JITUINT32 stackTop;
#endif

    /* Assertions					*/
    assert(method != NULL);
    assert(stack != NULL);
    assert(labels != NULL);
    assert(cilStack != NULL);
    assert(system != NULL);
    PDEBUG("CILIR: Test_Array_Index_Out_Of_Range: Start\n");

    /* Check if runtime checks are enabled		*/
    if (!(system->cliManager.CLR).runtimeChecks) {

        /* The runtime checks have been disabled		*/
        return 0;
    }

    /* Fetch the stack top				*/
#ifdef DEBUG
    stackTop = stack->top;
#endif

    /* Initialize the variables			*/
    instruction = NULL;

    /* Print the stack				*/
#ifdef PRINTDEBUG
    PDEBUG("CILIR: Test_Array_Index_Out_Of_Range:   Initial stack\n");
    print_stack(stack, method->getParametersNumber(method), method->getLocalsNumber(method));
#endif

    /* Clean the top of the stack                   */
    stack->cleanTop(stack);

    /* Fetch the array and push it on the stack     */
    memcpy(&(stack->stack[stack->top]), &(stack->stack[stack->top - 2]), sizeof(ir_item_t));
    (stack->top)++;

    /* Load the length of the array			*/
    internal_translate_ldlen(&(system->cliManager), &((system->garbage_collectors).gc), method, bytes_read, stack);

    /* fetch the index and push it on the stack */
    memcpy(&(stack->stack[stack->top]), &(stack->stack[stack->top - 2]), sizeof(ir_item_t));
    assert( ((stack->stack[(stack->top)]).internal_type == IRNINT) || ((stack->stack[(stack->top)]).internal_type == IRINT32) );
    (stack->top)++;

    /* Clean the top of the stack           */
    stack->cleanTop(stack);

    /* Convert the index to Unsigned Type */
    _perform_conversion(&(system->cliManager), method, bytes_read, stack, IRNUINT, JITFALSE, NULL);

    /* at this point the stack contains as first element (on the top) the size of the array.
     *  As second element the stack contains the requested index to check */
    /* Create the IRLT instruction		*/
    instruction = method->newIRInstr(method);
    instruction->type = IRLT;
    memcpy(&(instruction->param_2), &(stack->stack[stack->top - 2]), sizeof(ir_item_t));
    memcpy(&(instruction->param_1), &(stack->stack[stack->top - 1]), sizeof(ir_item_t));
    (stack->top) -= 2;

    /* Clean the top of the stack           */
    stack->cleanTop(stack);
    (stack->stack[(stack->top)]).value.v = method->incMaxVariablesReturnOld(method);
    (stack->stack[(stack->top)]).type = IROFFSET;
    (stack->stack[(stack->top)]).internal_type = IRINT32;
    (stack->stack[stack->top]).type_infos = NULL;
    (instruction->result).value.v = (stack->stack[stack->top]).value.v;
    (instruction->result).type = (stack->stack[stack->top]).type;
    (instruction->result).internal_type = (stack->stack[stack->top]).internal_type;
    (stack->top)++;
    instruction->byte_offset = bytes_read;

    /* test if array.size is greater than index */
    instruction = method->newIRInstr(method);
    instruction->type = IRBRANCHIF;
    instruction->byte_offset = bytes_read;
    (stack->top)--;
    (instruction->param_1).value.v = (stack->stack[stack->top]).value.v;
    (instruction->param_1).type = (stack->stack[stack->top]).type;
    (instruction->param_1).internal_type = (stack->stack[stack->top]).internal_type;
    assert((instruction->param_1).internal_type == IRINT32);
    label_l1.ID = (*current_label_ID);
    label_l1.counter = 0;
    label_l1.ir_position = method->getInstructionsNumber(method) - 1;
    label_l1.byte_offset = bytes_read;
    label_l1.type = DEFAULT_LABEL_TYPE;
    assert(IRMETHOD_getLabelInstruction(method->getIRMethod(method), label_l1.ID) == NULL);
    (*current_label_ID)++;
    (instruction->param_2).value.v = label_l1.ID;
    (instruction->param_2).type = IRLABELITEM;
    (instruction->param_2).internal_type = IRLABELITEM;

    /* We shall throw a System.IndexOutOfRangeException */
    assert(stackTop == stack->top);
    _translate_throw_CIL_Exception(&(system->cliManager), method, cilStack, current_label_ID, labels, bytes_read, stack, (system->exception_system)._System_IndexOutOfRangeException_ID);

    /* AT THIS POINT IN THE STACK THERE IS ONLY THE CONSTRUCTED EXCEPTION OBJECT */
    /* Fix up the label L1				*/
    instruction = method->newIRInstr(method);
    instruction->type = IRLABEL;
    instruction->byte_offset = bytes_read;
    (instruction->param_1).value.v = label_l1.ID;
    (instruction->param_1).type = IRLABELITEM;
    (instruction->param_1).internal_type = IRLABELITEM;

    /* Check the stack top				*/
#ifdef DEBUG
    assert(stackTop == stack->top);
#endif

    /* Return					*/
    PDEBUG("CILIR: Test_Array_Index_Out_Of_Range: Exit\n");
    return 0;
}

static inline JITUINT32 _translate_throw_CIL_Exception (CLIManager_t *cliManager, Method method, CILStack cilStack, JITUINT32 *current_label_ID, XanList *labels, JITUINT32 bytes_read, t_stack *stack, TypeDescriptor *type) {
    MethodDescriptor        *ctorMethodID;

    /* Assertions					*/
    assert(method != NULL);
    assert(stack != NULL);
    assert(labels != NULL);
    assert(cilStack != NULL);
    assert(current_label_ID != NULL);
    assert(cliManager != NULL);

    /* Initialize the local variables               */
    ctorMethodID = NULL;

    /* Create the exception object and place it on	*
     * top of the stack				*/
    make_irnewobj(cliManager, method, cilStack, bytes_read, type, 0, stack, current_label_ID, labels, NULL);

    /* ONCE CREATED THE Exception OBJECT, WE SHALL INVOKE THE RIGHT CONSTRUCTOR.
     * TO DO THIS, WE SHALL RETRIEVE THE 1 ARGS (we consider also the implicit object reference)
     * CONSTRUCTOR AND INVOKE IT ON THE OBJECT JUST CREATED. */
    /* STEP ONE : RETRIEVE THE MethodID of the constructor */
    ctorMethodID = retrieve_CILException_Ctor_MethodID_by_type(&(cliManager->methods), type);

    /* Call the constructor of the class of the     *
     * exception object				*/
    translate_cil_call_by_methodID(cliManager, method, ctorMethodID, bytes_read, stack, 1, NULL, cilStack, current_label_ID, labels);

    /* Throw the exception				*/
    return translate_cil_throw(cliManager, method, cilStack, current_label_ID, labels, bytes_read, stack);
}

static inline JITINT16 translate_cil_ldftn (t_system *system, Method method, CILStack cilStack, JITUINT32 bytes_read, t_stack *stack, GenericContainer *container, t_binary_information *binary, JITUINT32 token) {
    ActualMethodDescriptor tempMethod;
    Method newMethod;
    ir_symbol_t *entry_point;

    /* Assertions					*/
    assert(method != NULL);
    assert(stack != NULL);
    assert(cilStack != NULL);
    assert(system != NULL);
    assert(binary != NULL);


    /* Fetch the method information			*/
    tempMethod = (system->cliManager).metadataManager->getMethodDescriptorFromToken(system->cliManager.metadataManager, container, binary, token);

    /* Check if it already exist			*/
    t_methods *methods = &((system->cliManager).methods);
    newMethod = methods->fetchOrCreateMethod(methods, tempMethod.method, JITFALSE);
    assert(newMethod != NULL);

    entry_point = newMethod->getFunctionPointerSymbol(newMethod);
    assert(entry_point != NULL);

    /* Clean the top of the stack                   */
    stack->cleanTop(stack);

    /* Add the function pointer to the stack	*/
    (stack->stack[(stack->top)]).value.v = (JITNUINT) entry_point;
    (stack->stack[(stack->top)]).type = IRSYMBOL;
    (stack->stack[(stack->top)]).internal_type = IRFPOINTER;
    (stack->top)++;

    /* Add the variable that will contain the       *
     * function pointer to the stack		*/
    stack->stack[stack->top].value.v = method->incMaxVariablesReturnOld(method);
    stack->stack[stack->top].type = IROFFSET;
    stack->stack[stack->top].internal_type = IRFPOINTER;
    (stack->top)++;

    /* Store the function pointer			*/
    translate_cil_move(method, bytes_read, stack);

    /* Return					*/
    return 0;
}

static inline JITINT16 translate_cil_ldvirtftn (t_system *system, Method method, CILStack cilStack, JITUINT32 bytes_read, t_stack *stack, GenericContainer *container, t_binary_information *binary, JITUINT32 token, JITUINT32 *current_label_ID, XanList *labels) {
    ir_instruction_t        *instruction;
    ir_instruction_t        *instruction2;
    ActualMethodDescriptor tempMethod;
    JITINT32 imtOffset;
    JITINT32 vtableOffset;
    VCallPath vtableIndex;
    JITUINT32 objVarID;
    JITUINT32 vtablePointerVarID;
    JITUINT32 imtPointerVarID;
    JITUINT32 keyID;
    JITUINT32 keyEqID;
    JITUINT32 vtableVarID;
    JITUINT32 methodVarID;
    JITUINT32 bucketVarID;
    JITUINT32 label_l1;
    JITUINT32 label_l2;

    /* Fetch the method information			*/
    tempMethod = (system->cliManager).metadataManager->getMethodDescriptorFromToken(system->cliManager.metadataManager, container, binary, token);

    /* Initialize the variables		*/
    vtableOffset = 0;

    /* we have to check if the just located object on the stack is a valid IROBJECT
     * (by checking the internal_type of the stack_item */
    assert((stack->stack[(stack->top - 1)]).type == IROFFSET);
    assert((stack->stack[stack->top - 1]).internal_type == IROBJECT);

    objVarID = (stack->stack[stack->top - 1]).value.v;

    /* now we can do the test */
    translate_Test_Null_Reference(&(system->cliManager), method, cilStack, bytes_read, stack);

    /* Assertions			*/
    assert(system != NULL);

    /* retrieve the `methodVarID` value */
    methodVarID = method->incMaxVariablesReturnOld(method);

    /* retrieve the vtable index associated with the current method */
    vtableIndex = (system->garbage_collectors).gc.getIndexOfVTable(tempMethod.method);

    if (vtableIndex.useIMT) {

        imtOffset = (system->garbage_collectors).gc.fetchIMTOffset();

        /* Make the IR instruction to	*
         * load the pointer of the      *
         * virtual table		*/
        imtPointerVarID = method->incMaxVariablesReturnOld(method);
        instruction = method->newIRInstr(method);
        instruction->type = IRLOADREL;
        (instruction->param_1).value.v = objVarID;
        (instruction->param_1).type = IROFFSET;
        (instruction->param_1).internal_type = IROBJECT;
        (instruction->param_2).value.v = (JITINT32) imtOffset;
        (instruction->param_2).type = IRINT32;
        (instruction->param_2).internal_type = IRINT32;
        (instruction->result).value.v = imtPointerVarID;
        (instruction->result).type = IROFFSET;
        (instruction->result).internal_type = IRNINT;
        instruction->byte_offset = bytes_read;
        /* Make the IR instruction to	*
         * load the bucket of the virtual *
         * table                        */

        bucketVarID = method->incMaxVariablesReturnOld(method);
        instruction = method->newIRInstr(method);
        instruction->type = IRADD;
        (instruction->param_1).value.v = imtPointerVarID;
        (instruction->param_1).type = IROFFSET;
        (instruction->param_1).internal_type = IRNINT;
        (instruction->param_2).value.v = (JITNUINT) (system->cliManager).translator.getIndexOfMethodDescriptorSymbol(&(system->cliManager), tempMethod.method);
        (instruction->param_2).type = IRNINT;
        (instruction->param_2).internal_type = IRNINT;
        (instruction->result).value.v = bucketVarID;
        (instruction->result).type = IROFFSET;
        (instruction->result).internal_type = IRNINT;
        instruction->byte_offset = bytes_read;
        assert((instruction->param_1).internal_type == (instruction->param_2).internal_type);

        label_l1 = (*current_label_ID)++;
        label_l2 = (*current_label_ID)++;
        (*current_label_ID)++;

        instruction = method->newIRInstr(method);
        instruction->type = IRLABEL;
        instruction->byte_offset = bytes_read;
        (instruction->param_1).value.v = label_l1;
        (instruction->param_1).type = IRLABELITEM;
        (instruction->param_1).internal_type = IRLABELITEM;

        keyID = method->incMaxVariablesReturnOld(method);
        instruction = method->newIRInstr(method);
        instruction->type = IRLOADREL;
        (instruction->param_1).value.v = bucketVarID;
        (instruction->param_1).type = IROFFSET;
        (instruction->param_1).internal_type = IRNINT;
        (instruction->param_2).value.v = 0;
        (instruction->param_2).type = IRINT32;
        (instruction->param_2).internal_type = IRINT32;
        (instruction->result).value.v = keyID;
        (instruction->result).type = IROFFSET;
        (instruction->result).internal_type = IRNINT;
        instruction->byte_offset = bytes_read;

        /* test if method is equal */
        keyEqID = method->incMaxVariablesReturnOld(method);
        instruction = method->newIRInstr(method);
        instruction->type = IREQ;
        (instruction->param_1).value.v = keyID;
        (instruction->param_1).type = IROFFSET;
        (instruction->param_1).internal_type = IRNINT;
        (instruction->param_2).value.v = (JITNINT) (system->cliManager).translator.getMethodDescriptorSymbol(&(system->cliManager), tempMethod.method);
        (instruction->param_2).type = IRSYMBOL;
        (instruction->param_2).internal_type = IRNINT;
        (instruction->result).value.v = keyEqID;
        (instruction->result).type = IROFFSET;
        (instruction->result).internal_type = IRINT32;
        instruction->byte_offset = bytes_read;

        instruction = method->newIRInstr(method);
        instruction->type = IRBRANCHIF;
        instruction->byte_offset = bytes_read;
        (instruction->param_1).value.v = keyEqID;
        (instruction->param_1).type = IROFFSET;
        (instruction->param_1).internal_type = IRINT32;
        (instruction->param_2).value.v = label_l2;
        (instruction->param_2).type = IRLABELITEM;
        (instruction->param_2).internal_type = IRLABELITEM;

        /* load pointer of next imt slot in collision list */
        instruction = method->newIRInstr(method);
        instruction->type = IRLOADREL;
        (instruction->param_1).value.v = bucketVarID;
        (instruction->param_1).type = IROFFSET;
        (instruction->param_1).internal_type = IRNINT;
        (instruction->param_2).value.v = offsetof(IMTElem, next);
        (instruction->param_2).type = IRINT32;
        (instruction->param_2).internal_type = IRINT32;
        (instruction->result).value.v = bucketVarID;
        (instruction->result).type = IROFFSET;
        (instruction->result).internal_type = IRNINT;
        instruction->byte_offset = bytes_read;

        instruction = method->newIRInstr(method);
        instruction->type = IRBRANCH;
        instruction->byte_offset = bytes_read;
        (instruction->param_1).value.v = label_l1;
        (instruction->param_1).type = IRLABELITEM;
        (instruction->param_1).internal_type = IRLABELITEM;

        /* found method in imt load pointer */
        instruction = method->newIRInstr(method);
        instruction->type = IRLABEL;
        instruction->byte_offset = bytes_read;
        (instruction->param_1).value.v = label_l2;
        (instruction->param_1).type = IRLABELITEM;
        (instruction->param_1).internal_type = IRLABELITEM;

        instruction = method->newIRInstr(method);
        instruction->type = IRLOADREL;
        (instruction->param_1).value.v = bucketVarID;
        (instruction->param_1).type = IROFFSET;
        (instruction->param_1).internal_type = IRNINT;
        (instruction->param_2).value.v = offsetof(IMTElem, pointer);
        (instruction->param_2).type = IRINT32;
        (instruction->param_2).internal_type = IRINT32;
        (instruction->result).value.v = methodVarID;
        (instruction->result).type = IROFFSET;
        (instruction->result).internal_type = IRNINT;
        instruction->byte_offset = bytes_read;

    } else {

        /* Clean the top of the stack           */
        stack->cleanTop(stack);
        vtableOffset = (system->garbage_collectors).gc.fetchVTableOffset();

        /* Compute the offset of the	*
         * virtual table, which is      *
         * constant for each object,    *
         * and for each type		*/
        stack->stack[(stack->top)].value.v = objVarID;
        stack->stack[(stack->top)].type = IROFFSET;
        stack->stack[(stack->top)].internal_type = IROBJECT;
        stack->stack[(stack->top)].type_infos = NULL;

        /* Make the IR instruction to	*
         * load the pointer of the      *
         * virtual table		*/
        vtablePointerVarID = method->incMaxVariablesReturnOld(method);
        instruction2 = method->newIRInstr(method);
        instruction2->type = IRLOADREL;
        (instruction2->param_1).value.v = objVarID;
        (instruction2->param_1).type = IROFFSET;
        (instruction2->param_1).internal_type = IROBJECT;
        (instruction2->param_2).value.v = (JITINT32) vtableOffset;
        (instruction2->param_2).type = IRINT32;
        (instruction2->param_2).internal_type = IRINT32;
        (instruction2->result).value.v = vtablePointerVarID;
        (instruction2->result).type = IROFFSET;
        (instruction2->result).internal_type = IRNINT;
        instruction2->byte_offset = bytes_read;
        vtableVarID = vtablePointerVarID;

        /* Make the IR instruction to	*
         * load the slot of the virtual *
         * table                        */
        instruction2 = method->newIRInstr(method);
        instruction2->type = IRLOADREL;
        (instruction2->param_1).value.v = vtableVarID;
        (instruction2->param_1).type = IROFFSET;
        (instruction2->param_1).internal_type = IRNINT;
        (instruction2->param_2).value.v = vtableIndex.vtable_index * getIRSize(IRMPOINTER);
        (instruction2->param_2).type = IRINT32;
        (instruction2->param_2).internal_type = IRINT32;
        (instruction2->result).value.v = methodVarID;
        (instruction2->result).type = IROFFSET;
        (instruction2->result).internal_type = IRNINT;
        instruction2->byte_offset = bytes_read;

    }

    /* Clean the top of the stack                   */
    (stack->top)--;
    stack->cleanTop(stack);

    /* Add the function pointer to the stack	*/
    (stack->stack[(stack->top)]).value.v = methodVarID;
    (stack->stack[(stack->top)]).type = IROFFSET;
    (stack->stack[(stack->top)]).internal_type = IROBJECT;
    (stack->top)++;

    /* Return					*/
    return 0;
}

/**
 *
 * @param before if it is not NULL, the IR instructions will be inserted before the specified instruction.
 *               If it is NULL, they will be put at the end of the current method.
 */
static inline JITUINT32 translate_Test_OutOfMemory (CLIManager_t *cliManager, Method method, CILStack cilStack, JITUINT32 *current_label_ID, XanList *labels, JITUINT32 bytes_read, t_stack *stack, ir_instruction_t *before) {
    ir_instruction_t        *instruction;
    t_label                 *label_l1;

#ifdef DEBUG
    JITUINT32 stackTop;
#endif

    /* Assertions					*/
    assert(method != NULL);
    assert(stack != NULL);
    assert(cilStack != NULL);
    assert(labels != NULL);
    assert(current_label_ID != NULL);

    /* Check if runtime checks are enabled		*/
    if (!(cliManager->CLR).runtimeChecks) {

        /* The runtime checks have been disabled		*/
        return 0;
    }

    /* Fetch the stack top						*/
#ifdef DEBUG
    stackTop = stack->top;
#endif

    /* initialize the local variables */
    instruction = NULL;
    label_l1 = NULL;

    /* verify that the element on the top of the stack is an object */
    assert((stack->stack[(stack->top) - 1]).internal_type == IROBJECT);

    /* Before start the verification, we have to duplicate the element at the top of the stack */
    (stack->stack[(stack->top)]).value.v = (stack->stack[(stack->top) - 1]).value.v;
    (stack->stack[(stack->top)]).value.f = (stack->stack[(stack->top) - 1]).value.f;
    (stack->stack[(stack->top)]).type = (stack->stack[(stack->top) - 1]).type;
    (stack->stack[(stack->top)]).internal_type = (stack->stack[(stack->top) - 1]).internal_type;
    make_ir_infos(&(stack->stack[(stack->top)]), &(stack->stack[(stack->top) - 1]));
    (stack->top)++;

    /* Clean the top of the stack           */
    stack->cleanTop(stack);

    /* load NULL on the stack */
    (stack->stack[stack->top]).value.v = (JITNINT) NULL;
    (stack->stack[stack->top]).type = IRNINT;
    (stack->stack[stack->top]).internal_type = IRNINT;
    (stack->stack[stack->top]).type_infos = NULL;
    (stack->top)++;

    /* Insert the IREQ instruction		*/
    /* Insert the new instruction before the specified one (if any) */
    instruction = method->newIRInstrBefore(method, before);
    instruction->type = IREQ;
    memcpy(&(instruction->param_1), &(stack->stack[(stack->top) - 2]), sizeof(ir_item_t));
    (instruction->param_2).value.v = (stack->stack[(stack->top) - 1]).value.v;
    (instruction->param_2).value.f = (stack->stack[(stack->top) - 1]).value.f;
    (instruction->param_2).type = (stack->stack[(stack->top) - 1]).type;
    (instruction->param_2).internal_type = (stack->stack[(stack->top) - 1]).internal_type;
    make_ir_infos(&(instruction->param_2), &(stack->stack[(stack->top) - 1]));
    assert((instruction->param_1).internal_type == (instruction->param_2).internal_type);
    (stack->top) -= 2;

    /* Clean the top of the stack           */
    stack->cleanTop(stack);

    stack->stack[(stack->top)].value.v = method->incMaxVariablesReturnOld(method);
    stack->stack[(stack->top)].type = IROFFSET;
    stack->stack[(stack->top)].internal_type = IRINT32;
    (stack->stack[stack->top]).type_infos = NULL;
    (instruction->result).value.v = (stack->stack[stack->top]).value.v;
    (instruction->result).type = (stack->stack[stack->top]).type;
    (instruction->result).internal_type = (stack->stack[stack->top]).internal_type;
    (stack->top)++;
    instruction->byte_offset = bytes_read;

    /* test if the object is NULL */
    /* Insert the new instruction before the specified one (if any) */
    instruction = method->newIRInstrBefore(method, before);
    instruction->type = IRBRANCHIFNOT;
    instruction->byte_offset = bytes_read;
    (stack->top)--;
    (instruction->param_1).value.v = (stack->stack[stack->top]).value.v;
    (instruction->param_1).type = (stack->stack[stack->top]).type;
    (instruction->param_1).internal_type = (stack->stack[stack->top]).internal_type;
    assert((instruction->param_1).internal_type == IRINT32);
    label_l1 = (t_label *) allocMemory(sizeof(t_label));
    label_l1->ID = (*current_label_ID);
    label_l1->counter = 0;
    label_l1->ir_position = method->getInstructionsNumber(method) - 1;
    label_l1->byte_offset = bytes_read;
    label_l1->type = DEFAULT_LABEL_TYPE;
    cil_insert_label(cilStack, labels, label_l1, stack, -1);
    (*current_label_ID)++;
    assert(IRMETHOD_getLabelInstruction(method->getIRMethod(method), label_l1->ID) == NULL);
    (instruction->param_2).value.v = label_l1->ID;
    (instruction->param_2).type = IRLABELITEM;
    (instruction->param_2).internal_type = IRLABELITEM;

    /* An OutOfMemoryException occurred.. We must throw the exception */

    /* Clean the top of the stack           */
    stack->cleanTop(stack);

    /* PUSH the OutOfMemoryException object on the stack */
    stack->stack[stack->top].value.v = method->incMaxVariablesReturnOld(method);
    stack->stack[stack->top].type = IROFFSET;
    stack->stack[stack->top].internal_type = IROBJECT;
    (stack->stack[stack->top]).type_infos = NULL;
    (stack->top)++;

    translate_ncall(method, (JITINT8 *) "get_OutOfMemoryException", IROBJECT, NULL, bytes_read, stack->stack[stack->top].value.v, NULL);

    /* AT THIS POINT ON THE TOP OF THE STACK THERE IS ONLY THE CONSTRUCTED EXCEPTION OBJECT */
    PDEBUG("CILIR: test_OutOfMemory :       save the current stack trace \n");
    translate_ncall(method, (JITINT8 *) "updateTheStackTrace", IRVOID, NULL, bytes_read, 0, NULL);

    /* THROW THE EXCEPTION OBJECT */
    /* Insert the new instruction before the specified one (if any) */
    instruction = method->newIRInstrBefore(method, before);
    instruction->type = IRTHROW;
    (stack->top)--;
    memcpy(&(instruction->param_1), &(stack->stack[stack->top]), sizeof(ir_item_t));
    assert((instruction->param_1).internal_type == IROBJECT);
    assert((instruction->param_1).type == IROFFSET);
    instruction->byte_offset = bytes_read;

    /* Upon exiting the function, we must assure that all the labels are constructed properly.
     * In particular we must verify if the label l1 need to be created */
    /* fix-up the label l1 */
    /* Insert the new instruction before the specified one (if any) */
    instruction = method->newIRInstrBefore(method, before);
    instruction->type = IRLABEL;
    instruction->byte_offset = bytes_read;
    (instruction->param_1).value.v = label_l1->ID;
    (instruction->param_1).type = IRLABELITEM;
    (instruction->param_1).internal_type = IRLABELITEM;

    /* Check the stack top				*/
#ifdef DEBUG
    assert(stackTop == stack->top);
#endif

    /* Return					*/
    return 0;
}

static inline JITUINT32 translate_Test_Overflow (t_system *system, Method method, CILStack cilStack, JITUINT32 *current_label_ID, XanList *labels, JITUINT32 bytes_read, t_stack *stack) {
    ir_instruction_t        *instruction;
    t_label                 *label_l1;

#ifdef DEBUG
    JITUINT32 stackTop;
#endif

    /* Assertions					*/
    assert(method != NULL);
    assert(stack != NULL);
    assert(labels != NULL);
    assert(current_label_ID != NULL);
    assert(cilStack != NULL);
    assert(system != NULL);

    /* Check if runtime checks are enabled			*/
    if (!(system->cliManager.CLR).runtimeChecks) {

        /* The runtime checks have been disabled		*/
        return 0;
    }

    /* Fetch the stack top						*/
#ifdef DEBUG
    stackTop = stack->top;
#endif

    /* initialize the local variables */
    instruction = NULL;
    label_l1 = NULL;

    /* Clean the top of the stack           */
    stack->cleanTop(stack);

    /* Before start the verification, we have to duplicate the element `size` at the top of the stack */
    (stack->stack[(stack->top)]).value.v = (stack->stack[(stack->top) - 1]).value.v;
    (stack->stack[(stack->top)]).value.f = (stack->stack[(stack->top) - 1]).value.f;
    (stack->stack[(stack->top)]).type = (stack->stack[(stack->top) - 1]).type;
    (stack->stack[(stack->top)]).internal_type = (stack->stack[(stack->top) - 1]).internal_type;
    make_ir_infos(&(stack->stack[(stack->top)]), &(stack->stack[(stack->top) - 1]));
    (stack->top)++;

    /* Clean the top of the stack           */
    stack->cleanTop(stack);

    /* load 0 on the stack */
    (stack->stack[stack->top]).value.v = 0;
    (stack->stack[stack->top]).type = IRINT32;
    (stack->stack[stack->top]).internal_type = IRINT32;
    (stack->stack[stack->top]).type_infos = NULL;
    (stack->top)++;

    /* test if size is greater than 0 */
    instruction = method->newIRInstr(method);
    instruction->type = IRLT;
    (instruction->param_1).value.v = (stack->stack[(stack->top) - 2]).value.v;
    (instruction->param_1).value.f = (stack->stack[(stack->top) - 2]).value.f;
    (instruction->param_1).type = (stack->stack[(stack->top) - 2]).type;
    (instruction->param_1).internal_type = (stack->stack[(stack->top) - 2]).internal_type;
    make_ir_infos(&(instruction->param_1), &(stack->stack[(stack->top) - 2]));
    (instruction->param_2).value.v = (stack->stack[(stack->top) - 1]).value.v;
    (instruction->param_2).value.f = (stack->stack[(stack->top) - 1]).value.f;
    (instruction->param_2).type = (stack->stack[(stack->top) - 1]).type;
    (instruction->param_2).internal_type = (stack->stack[(stack->top) - 1]).internal_type;
    make_ir_infos(&(instruction->param_2), &(stack->stack[(stack->top) - 1]));
    (stack->top) -= 2;

    /* Clean the top of the stack           */
    stack->cleanTop(stack);

    stack->stack[(stack->top)].value.v = method->incMaxVariablesReturnOld(method);
    stack->stack[(stack->top)].type = IROFFSET;
    stack->stack[(stack->top)].internal_type = IRINT32;
    (stack->stack[stack->top]).type_infos = NULL;
    (instruction->result).value.v = (stack->stack[stack->top]).value.v;
    (instruction->result).type = (stack->stack[stack->top]).type;
    (instruction->result).internal_type = (stack->stack[stack->top]).internal_type;
    (stack->top)++;
    instruction->byte_offset = bytes_read;

    /* test if it's necessary to raise an exception */
    instruction = method->newIRInstr(method);
    instruction->type = IRBRANCHIFNOT;
    instruction->byte_offset = bytes_read;
    (stack->top)--;
    (instruction->param_1).value.v = (stack->stack[stack->top]).value.v;
    (instruction->param_1).type = (stack->stack[stack->top]).type;
    (instruction->param_1).internal_type = (stack->stack[stack->top]).internal_type;
    assert((instruction->param_1).internal_type == IRINT32);
    label_l1 = (t_label *) allocMemory(sizeof(t_label));
    label_l1->ID = (*current_label_ID);
    label_l1->counter = 0;
    label_l1->ir_position = method->getInstructionsNumber(method) - 1;
    label_l1->byte_offset = bytes_read;
    label_l1->type = DEFAULT_LABEL_TYPE;
    cil_insert_label(cilStack, labels, label_l1, stack, -1);
    (*current_label_ID)++;
    assert(IRMETHOD_getLabelInstruction(method->getIRMethod(method), label_l1->ID) == NULL);
    (instruction->param_2).value.v = label_l1->ID;
    (instruction->param_2).type = IRLABELITEM;
    (instruction->param_2).internal_type = IRLABELITEM;

    /* raise a System.OverflowException exception */
    _translate_throw_CIL_Exception(&(system->cliManager), method, cilStack, current_label_ID, labels, bytes_read, stack, (system->exception_system)._System_OverflowException_ID);

    /* fix-up the label l1 */
    instruction = method->newIRInstr(method);
    instruction->type = IRLABEL;
    instruction->byte_offset = bytes_read;
    (instruction->param_1).value.v = label_l1->ID;
    (instruction->param_1).type = IRLABELITEM;
    (instruction->param_1).internal_type = IRLABELITEM;

    /* Check the stack top				*/
#ifdef DEBUG
    assert(stackTop == stack->top);
#endif

    /* Return					*/
    return 0;
}

static inline JITUINT32 translate_Test_Type_Load_with_Type_Token (t_system *system, Method method, CILStack cilStack, JITUINT32 *current_label_ID, XanList *labels, JITUINT32 bytes_read, t_stack *stack, t_binary_information *binary, JITUINT32 token) {
#ifdef DEBUG
    JITUINT32 stackTop;
#endif

    /* Assertions                           */
    assert(current_label_ID != NULL);
    assert(labels != NULL);
    assert(stack != NULL);
    assert(binary != NULL);
    assert(system != NULL);
    assert(method != NULL);
    assert(cilStack != NULL);

    /* Check if runtime checks are enabled		*/
    if (!(system->cliManager.CLR).runtimeChecks) {

        /* The runtime checks have been disabled		*/
        return 0;
    }

    /* Fetch the stack top						*/
#ifdef DEBUG
    stackTop = stack->top;
#endif

    /* test if the classLocated is valid    */
    if (!isValidTypeToken(binary, token)) {
        _translate_throw_CIL_Exception(&(system->cliManager), method, cilStack, current_label_ID, labels, bytes_read, stack, (system->exception_system)._System_TypeLoadException_ID);

        /* Check the stack top			*/
#ifdef DEBUG
        assert(stackTop == stack->top);
#endif

        /* Return				*/
        return 1;
    }

    /* Check the stack top			*/
#ifdef DEBUG
    assert(stackTop == stack->top);
#endif

    /* Return				*/
    return 0;
}

static inline JITUINT32 translate_Test_Missing_Method (t_system *system, Method method, CILStack cilStack, JITUINT32 *current_label_ID, XanList *labels, JITUINT32 bytes_read, t_stack *stack, t_binary_information *binary, JITUINT32 token) {
#ifdef DEBUG
    JITUINT32 stackTop;
#endif

    /* Assertions                   */
    assert(current_label_ID != NULL);
    assert(labels != NULL);
    assert(stack != NULL);
    assert(binary != NULL);
    assert(cilStack != NULL);
    assert(system != NULL);
    assert(method != NULL);

    /* Check if runtime checks are enabled		*/
    if (!(system->cliManager.CLR).runtimeChecks) {

        /* The runtime checks have been disabled		*/
        return 0;
    }

    /* Fetch the stack top						*/
#ifdef DEBUG
    stackTop = stack->top;
#endif

    /* System.MissingMethodException can be thrown when there is an attempt to dynamically access a
     * method that does not exist */
    if (!isValidCallToken(binary, token)) {
        _translate_throw_CIL_Exception(&(system->cliManager), method, cilStack, current_label_ID, labels, bytes_read, stack, (system->exception_system)._System_MissingMethodException_ID);

        /* Check the stack top				*/
#ifdef DEBUG
        assert(stackTop == stack->top);
#endif

        return 1;
    }

    /* Check the stack top				*/
#ifdef DEBUG
    assert(stackTop == stack->top);
#endif

    /* Return					*/
    return 0;
}

static inline JITUINT32 translate_Test_Missing_Field (t_system *system, Method method, CILStack cilStack, JITUINT32 *current_label_ID, XanList *labels, JITUINT32 bytes_read, t_stack *stack, t_binary_information *binary, JITUINT32 token) {
#ifdef DEBUG
    JITUINT32 stackTop;
#endif

    /* Assertions                   */
    assert(current_label_ID != NULL);
    assert(labels != NULL);
    assert(stack != NULL);
    assert(binary != NULL);
    assert(system != NULL);
    assert(method != NULL);

    /* Check if runtime checks are enabled		*/
    if (!(system->cliManager.CLR).runtimeChecks) {

        /* The runtime checks have been disabled		*/
        return 0;
    }

    /* Fetch the stack top						*/
#ifdef DEBUG
    stackTop = stack->top;
#endif

    /* System.MissingFieldException can be thrown when there is an attempt to dynamically access a
     * field that does not exist */
    if (!isValidFieldToken(binary, token)) {
        _translate_throw_CIL_Exception(&(system->cliManager), method, cilStack, current_label_ID, labels, bytes_read, stack,   (system->exception_system)._System_MissingFieldException_ID);

        /* Check the stack top				*/
#ifdef DEBUG
        assert(stackTop == stack->top);
#endif

        return 1;
    }

    /* Check the stack top				*/
#ifdef DEBUG
    assert(stackTop == stack->top);
#endif

    return 0;
}

static inline JITUINT32 translate_Test_Method_Access (t_system *system, Method method, CILStack cilStack, JITUINT32 *current_label_ID, XanList *labels, JITUINT32 bytes_read, t_stack *stack, MethodDescriptor *callee) {
#ifdef DEBUG
    JITUINT32 stackTop;
#endif

    /* Assertions                   */
    assert(current_label_ID != NULL);
    assert(labels != NULL);
    assert(stack != NULL);
    assert(cilStack != NULL);
    assert(system != NULL);
    assert(method != NULL);
    assert(callee != NULL);

    /* Check if runtime checks are enabled		*/
    if (!(system->cliManager.CLR).runtimeChecks) {

        /* The runtime checks have been disabled		*/
        return 0;
    }

    /* Fetch the stack top						*/
#ifdef DEBUG
    stackTop = stack->top;
#endif

    /*	System.MethodAccessException can be thrown when there is an invalid attempt
     *	to access a private or protected method inside a class		*/

    if (!callee->isCallable(callee, method->getID(method))) {
        _translate_throw_CIL_Exception(&(system->cliManager), method, cilStack, current_label_ID, labels, bytes_read, stack, (system->exception_system)._System_MethodAccessException_ID);

        /* Check the stack top				*/
#ifdef DEBUG
        assert(stackTop == stack->top);
#endif

        return 1;
    }

    /* Check the stack top				*/
#ifdef DEBUG
    assert(stackTop == stack->top);
#endif

    return 0;
}

static inline JITUINT32 translate_Test_Invalid_Operation (t_system *system, Method method, CILStack cilStack, JITUINT32 *current_label_ID, XanList *labels, JITUINT32 bytes_read, t_stack *stack, MethodDescriptor *ctor) {
#ifdef DEBUG
    JITUINT32 stackTop;
#endif

    /* Assertions                   */
    assert(current_label_ID != NULL);
    assert(labels != NULL);
    assert(stack != NULL);
    assert(system != NULL);
    assert(method != NULL);
    assert(ctor != NULL);

    /* Check if runtime checks are enabled		*/
    if (!(system->cliManager.CLR).runtimeChecks) {

        /* The runtime checks have been disabled		*/
        return 0;
    }

    /* Fetch the stack top						*/
#ifdef DEBUG
    stackTop = stack->top;
#endif

    /* test if the ctor's class is ABSTRACT. If this is the case,
     * we shall throw a System.InvalidOperationException */
    if (ctor->attributes->is_abstract) {
        _translate_throw_CIL_Exception(&(system->cliManager), method, cilStack, current_label_ID, labels, bytes_read, stack, (system->exception_system)._System_InvalidOperationException_ID);

        /* Check the stack top				*/
#ifdef DEBUG
        assert(stackTop == stack->top);
#endif

        return 1;
    }

    /* Check the stack top				*/
#ifdef DEBUG
    assert(stackTop == stack->top);
#endif

    return 0;
}

/* NOTE: this implementation does not consider the case of a Nullable<T> type */
static inline JITUINT32 translate_Test_Cast_Class (t_system *system, Method method, CILStack cilStack, JITUINT32 *current_label_ID, XanList *labels, JITUINT32 bytes_read, t_stack *stack, GenericContainer *container, t_binary_information *binary, JITUINT32 token, JITBOOLEAN can_throw_exception) {
    TypeDescriptor                  *type_of_the_token;
    ir_instruction_t                *instruction;
    ir_instruction_t		*loadInstruction;
    ir_symbol_t                     *typeIndex;
    JITINT32 typeDescriptorOffset;
    JITUINT32 keyEqID;
    JITUINT32 keyID;
    JITUINT32 TypePointerVarID;
    JITUINT32 TypeCompPointerVarID;
    JITUINT32 bucketVarID;
    JITUINT32 valueVarID;
    JITUINT32 resultVarID;
    JITUINT32 hashLoop_label;
    JITUINT32 foundKey_label;
    JITUINT32 testValue_label;
    JITUINT32 endTest_label;
    JITUINT32 okCast_label;
    ir_item_t                       *items;
    XanList                         *icall_params;

#ifdef DEBUG
    JITUINT32 stackTop;
#endif

    /* assertions */
    assert(current_label_ID != NULL);
    assert(labels != NULL);
    assert(stack != NULL);
    assert(binary != NULL);
    assert(system != NULL);
    assert(cilStack != NULL);
    assert(method != NULL);

    /* Check if runtime checks are enabled		*/
    if (    (!(system->cliManager.CLR).runtimeChecks)                                       &&
            (!IRPROGRAM_doesMethodBelongToALibrary(method->getIRMethod(method)))             ) {

        /* The runtime checks have been disabled		*/
        return 0;
    }

    /* Fetch the stack top						*/
#ifdef DEBUG
    stackTop = stack->top;
#endif

    /* initialize the label elements */
    hashLoop_label = (*current_label_ID)++;
    foundKey_label = (*current_label_ID)++;
    testValue_label = (*current_label_ID)++;
    endTest_label = (*current_label_ID)++;
    okCast_label = (*current_label_ID)++;

    /* The castclass instruction attempts to cast obj (of type IROBJECT) to the class */
    assert((stack->stack[(stack->top) - 1]).internal_type == IROBJECT);

    /* System.TypeLoadException can be thrown when there is an attempt
     * to cast a class object into a type that doesn't exists */
    if (isValidTypeToken(binary, token) == 0) {
        _translate_throw_CIL_Exception(&(system->cliManager), method, cilStack, current_label_ID, labels, bytes_read, stack, (system->exception_system)._System_TypeLoadException_ID);

        /* Check the stack top				*/
#ifdef DEBUG
        assert(stackTop == stack->top);
#endif

        return 1;
    }

    /* fill the 'type_of_the_token' structure */

    type_of_the_token = (system->cliManager).metadataManager->getTypeDescriptorFromToken(system->cliManager.metadataManager, container, binary, token);
    assert(type_of_the_token != NULL);
    typeIndex = (system->cliManager).translator.getIndexOfTypeDescriptorSymbol(&(system->cliManager), type_of_the_token);
    typeDescriptorOffset = (system->garbage_collectors).gc.fetchTypeDescriptorOffset();
    valueVarID = method->incMaxVariablesReturnOld(method);

    keyEqID = method->incMaxVariablesReturnOld(method);
    instruction = method->newIRInstr(method);
    instruction->type = IREQ;
    memcpy(&(instruction->param_1), &(stack->stack[(stack->top) - 1]), sizeof(ir_item_t));
    (instruction->param_2).value.v = (JITNUINT) NULL;
    (instruction->param_2).type = (instruction->param_1).internal_type;
    (instruction->param_2).internal_type = (instruction->param_2).type;
    (instruction->result).value.v = keyEqID;
    (instruction->result).type = IROFFSET;
    (instruction->result).internal_type = IRINT32;
    instruction->byte_offset = bytes_read;
    assert((instruction->param_1).internal_type == (instruction->param_2).internal_type);

    instruction = method->newIRInstr(method);
    instruction->type = IRBRANCHIF;
    instruction->byte_offset = bytes_read;
    (instruction->param_1).value.v = keyEqID;
    (instruction->param_1).type = IROFFSET;
    (instruction->param_1).internal_type = IRINT32;
    (instruction->param_2).value.v = okCast_label;
    (instruction->param_2).type = IRLABELITEM;
    (instruction->param_2).internal_type = IRLABELITEM;
    instruction->byte_offset = bytes_read;

    /* Here navigate into hashtable */

    /* Make the IR instruction to	*
     * load the pointer of the      *
     * TypeDescriptor		*/
    TypePointerVarID = method->incMaxVariablesReturnOld(method);
    instruction = method->newIRInstr(method);
    instruction->type = IRLOADREL;
    memcpy(&(instruction->param_1), &(stack->stack[(stack->top) - 1]), sizeof(ir_item_t));
    (instruction->param_2).value.v = (JITINT32) typeDescriptorOffset;
    (instruction->param_2).type = IRINT32;
    (instruction->param_2).internal_type = IRINT32;
    (instruction->result).value.v = TypePointerVarID;
    (instruction->result).type = IROFFSET;
    (instruction->result).internal_type = IRNINT;
    instruction->byte_offset = bytes_read;

    TypeCompPointerVarID = method->incMaxVariablesReturnOld(method);
    instruction = method->newIRInstr(method);
    instruction->type = IRLOADREL;
    (instruction->param_1).value.v = TypePointerVarID;
    (instruction->param_1).type = IROFFSET;
    (instruction->param_1).internal_type = IRNINT;
    (instruction->param_2).value.v = 0;
    (instruction->param_2).type = IRINT32;
    (instruction->param_2).internal_type = IRINT32;
    (instruction->result).value.v = TypeCompPointerVarID;
    (instruction->result).type = IROFFSET;
    (instruction->result).internal_type = IRNINT;
    instruction->byte_offset = bytes_read;

    /* Make the IR instruction to	*
     * load the pointer of the      *
     * TypeDescriptor compatibility table bucket		*/
    bucketVarID = method->incMaxVariablesReturnOld(method);
    instruction = method->newIRInstr(method);
    instruction->type = IRADD;
    (instruction->param_1).value.v = TypeCompPointerVarID;
    (instruction->param_1).type = IROFFSET;
    (instruction->param_1).internal_type = IRNINT;
    (instruction->param_2).value.v = (JITNUINT) typeIndex;
    (instruction->param_2).type = IRSYMBOL;
    (instruction->param_2).internal_type = IRNINT;
    (instruction->result).value.v = bucketVarID;
    (instruction->result).type = IROFFSET;
    (instruction->result).internal_type = IRNINT;
    instruction->byte_offset = bytes_read;
    assert((instruction->param_1).internal_type == (instruction->param_2).internal_type);

    /* Test for correct Bucket */
    instruction = method->newIRInstr(method);
    instruction->type = IRLABEL;
    instruction->byte_offset = bytes_read;
    (instruction->param_1).value.v = hashLoop_label;
    (instruction->param_1).type = IRLABELITEM;
    (instruction->param_1).internal_type = IRLABELITEM;
    instruction->byte_offset = bytes_read;

    keyID = method->incMaxVariablesReturnOld(method);
    loadInstruction = method->newIRInstr(method);
    loadInstruction->type = IRLOADREL;
    (loadInstruction->param_1).value.v = bucketVarID;
    (loadInstruction->param_1).type = IROFFSET;
    (loadInstruction->param_1).internal_type = IRNINT;
    (loadInstruction->param_2).value.v = 0;
    (loadInstruction->param_2).type = IRINT32;
    (loadInstruction->param_2).internal_type = IRINT32;
    (loadInstruction->result).value.v = keyID;
    (loadInstruction->result).type = IROFFSET;
    (loadInstruction->result).internal_type = IRNINT;
    loadInstruction->byte_offset = bytes_read;

    keyEqID = method->incMaxVariablesReturnOld(method);
    instruction = method->newIRInstr(method);
    instruction->type = IREQ;
    memcpy(&(instruction->param_1), &(loadInstruction->result), sizeof(ir_item_t));
    (instruction->param_2).value.v = (JITNUINT) type_of_the_token;
    (instruction->param_2).type = (instruction->param_1).internal_type;
    (instruction->param_2).internal_type = (instruction->param_2).type;
    (instruction->result).value.v = keyEqID;
    (instruction->result).type = IROFFSET;
    (instruction->result).internal_type = IRINT32;
    instruction->byte_offset = bytes_read;
    assert((instruction->param_1).internal_type == (instruction->param_2).internal_type);

    instruction = method->newIRInstr(method);
    instruction->type = IRBRANCHIF;
    instruction->byte_offset = bytes_read;
    (instruction->param_1).value.v = keyEqID;
    (instruction->param_1).type = IROFFSET;
    (instruction->param_1).internal_type = IRINT32;
    (instruction->param_2).value.v = foundKey_label;
    (instruction->param_2).type = IRLABELITEM;
    (instruction->param_2).internal_type = IRLABELITEM;
    instruction->byte_offset = bytes_read;

    /* load pointer of next imt slot in collision list */
    instruction = method->newIRInstr(method);
    instruction->type = IRLOADREL;
    (instruction->param_1).value.v = bucketVarID;
    (instruction->param_1).type = IROFFSET;
    (instruction->param_1).internal_type = IRNINT;
    (instruction->param_2).value.v = offsetof(TypeElem, next);
    (instruction->param_2).type = IRINT32;
    (instruction->param_2).internal_type = IRINT32;
    (instruction->result).value.v = bucketVarID;
    (instruction->result).type = IROFFSET;
    (instruction->result).internal_type = IRNINT;
    instruction->byte_offset = bytes_read;

    keyEqID = method->incMaxVariablesReturnOld(method);
    instruction = method->newIRInstr(method);
    instruction->type = IREQ;
    (instruction->param_1).value.v = bucketVarID;
    (instruction->param_1).type = IROFFSET;
    (instruction->param_1).internal_type = IRNINT;
    (instruction->param_2).value.v = (JITNINT) NULL;
    (instruction->param_2).type = IRNINT;
    (instruction->param_2).internal_type = IRNINT;
    (instruction->result).value.v = keyEqID;
    (instruction->result).type = IROFFSET;
    (instruction->result).internal_type = IRINT32;
    instruction->byte_offset = bytes_read;
    assert((instruction->param_1).internal_type == (instruction->param_2).internal_type);

    instruction = method->newIRInstr(method);
    instruction->type = IRBRANCHIFNOT;
    instruction->byte_offset = bytes_read;
    (instruction->param_1).value.v = keyEqID;
    (instruction->param_1).type = IROFFSET;
    (instruction->param_1).internal_type = IRINT32;
    (instruction->param_2).value.v = hashLoop_label;
    (instruction->param_2).type = IRLABELITEM;
    (instruction->param_2).internal_type = IRLABELITEM;
    instruction->byte_offset = bytes_read;

    /* not found imt slot in collision list call trampoline to init */
    icall_params = xanList_new(sharedAllocFunction, freeFunction, NULL);
    assert(icall_params != NULL);
    items = (ir_item_t *) sharedAllocFunction(sizeof(ir_item_t));
    memcpy(items, &(stack->stack[(stack->top) - 1]), sizeof(ir_item_t));
    xanList_append(icall_params, items);
    items = (ir_item_t *) sharedAllocFunction(sizeof(ir_item_t));
    (items->value).v = (JITNUINT) type_of_the_token;
    items->type = IRCLASSID;
    items->internal_type = IRCLASSID;
    xanList_append(icall_params, items);
    translate_ncall(method, (JITINT8 *) "runtime_canCast", IRINT32, icall_params, bytes_read, valueVarID, NULL);
    icall_params = NULL;

    instruction = method->newIRInstr(method);
    instruction->type = IRBRANCH;
    instruction->byte_offset = bytes_read;
    (instruction->param_1).value.v = testValue_label;
    (instruction->param_1).type = IRLABELITEM;
    (instruction->param_1).internal_type = IRLABELITEM;
    instruction->byte_offset = bytes_read;

    /* found type in load descriptor pointer */
    instruction = method->newIRInstr(method);
    instruction->type = IRLABEL;
    instruction->byte_offset = bytes_read;
    (instruction->param_1).value.v = foundKey_label;
    (instruction->param_1).type = IRLABELITEM;
    (instruction->param_1).internal_type = IRLABELITEM;
    instruction->byte_offset = bytes_read;

    instruction = method->newIRInstr(method);
    instruction->type = IRLOADREL;
    (instruction->param_1).value.v = bucketVarID;
    (instruction->param_1).type = IROFFSET;
    (instruction->param_1).internal_type = IRNINT;
    (instruction->param_2).value.v = offsetof(TypeElem, value);
    (instruction->param_2).type = IRINT32;
    (instruction->param_2).internal_type = IRINT32;
    (instruction->result).value.v = valueVarID;
    (instruction->result).type = IROFFSET;
    (instruction->result).internal_type = IRINT32;
    instruction->byte_offset = bytes_read;

    /* pointer loaded in methodID var */
    instruction = method->newIRInstr(method);
    instruction->type = IRLABEL;
    instruction->byte_offset = bytes_read;
    (instruction->param_1).value.v = testValue_label;
    (instruction->param_1).type = IRLABELITEM;
    (instruction->param_1).internal_type = IRLABELITEM;
    instruction->byte_offset = bytes_read;

    keyEqID = method->incMaxVariablesReturnOld(method);
    instruction = method->newIRInstr(method);
    instruction->type = IREQ;
    (instruction->param_1).value.v = valueVarID;
    (instruction->param_1).type = IROFFSET;
    (instruction->param_1).internal_type = IRINT32;
    (instruction->param_2).value.v = JITTRUE;
    (instruction->param_2).type = IRINT32;
    (instruction->param_2).internal_type = IRINT32;
    (instruction->result).value.v = keyEqID;
    (instruction->result).type = IROFFSET;
    (instruction->result).internal_type = IRINT32;
    instruction->byte_offset = bytes_read;
    assert((instruction->param_1).internal_type == (instruction->param_2).internal_type);

    instruction = method->newIRInstr(method);
    instruction->type = IRBRANCHIF;
    instruction->byte_offset = bytes_read;
    (instruction->param_1).value.v = keyEqID;
    (instruction->param_1).type = IROFFSET;
    (instruction->param_1).internal_type = IRINT32;
    (instruction->param_2).value.v = okCast_label;
    (instruction->param_2).type = IRLABELITEM;
    (instruction->param_2).internal_type = IRLABELITEM;
    instruction->byte_offset = bytes_read;

    resultVarID = method->incMaxVariablesReturnOld(method);
    if (can_throw_exception == 1) {
        assert((system->exception_system)._System_InvalidCastException_ID != NULL);
        _translate_throw_CIL_Exception(&(system->cliManager), method, cilStack, current_label_ID, labels, bytes_read, stack, (system->exception_system)._System_InvalidCastException_ID);
    } else {
        /* store a null reference on the result variable */
        instruction = method->newIRInstr(method);
        instruction->type = IRMOVE;
        (instruction->result).value.v = resultVarID;
        (instruction->result).type = IROFFSET;
        (instruction->result).internal_type = IROBJECT;
        (instruction->result).type_infos = (stack->stack[stack->top - 1]).type_infos;
        (instruction->param_1).value.v = (JITNUINT) NULL;
        (instruction->param_1).type = IROBJECT;
        (instruction->param_1).internal_type = IROBJECT;
        (instruction->param_1).type_infos = (stack->stack[stack->top - 1]).type_infos;
        instruction->byte_offset = bytes_read;
        assert((instruction->result).internal_type == (instruction->param_1).internal_type);

        instruction = method->newIRInstr(method);
        instruction->type = IRBRANCH;
        (instruction->param_1).value.v = endTest_label;
        (instruction->param_1).type = IRLABELITEM;
        (instruction->param_1).internal_type = IRLABELITEM;
        instruction->byte_offset = bytes_read;
    }

    instruction = method->newIRInstr(method);
    instruction->type = IRLABEL;
    instruction->byte_offset = bytes_read;
    (instruction->param_1).value.v = okCast_label;
    (instruction->param_1).type = IRLABELITEM;
    (instruction->param_1).internal_type = IRLABELITEM;
    instruction->byte_offset = bytes_read;

    /*Handle standard Cast */
    instruction 		= method->newIRInstr(method);
    instruction->type 	= IRMOVE;
    (instruction->result).value.v = resultVarID;
    (instruction->result).type = IROFFSET;
    (instruction->result).internal_type = IROBJECT;
    (instruction->result).type_infos = (stack->stack[stack->top - 1]).type_infos;
    memcpy(&(instruction->param_1), &(stack->stack[stack->top - 1]), sizeof(ir_item_t));
    assert((stack->stack[stack->top - 1]).type == IROFFSET);
    assert((stack->stack[stack->top - 1]).internal_type == IROBJECT);
    instruction->byte_offset = bytes_read;
    assert((instruction->result).internal_type == (instruction->param_1).internal_type);

    instruction = method->newIRInstr(method);
    instruction->type = IRLABEL;
    instruction->byte_offset = bytes_read;
    (instruction->param_1).value.v = endTest_label;
    (instruction->param_1).type = IRLABELITEM;
    (instruction->param_1).internal_type = IRLABELITEM;
    instruction->byte_offset = bytes_read;

    /* load on the stack the new element */
    (stack->stack[stack->top - 1]).value.v = resultVarID;
    (stack->stack[stack->top - 1]).type = IROFFSET;
    (stack->stack[stack->top - 1]).internal_type = IROBJECT;

    /* Check the stack top				*/
#ifdef DEBUG
    assert(stackTop == stack->top);
#endif

    return 0;
}

static inline JITUINT32 translate_Test_Valid_Value_Address (t_system *system, Method method, CILStack cilStack, JITUINT32 *current_label_ID, XanList *labels, JITUINT32 bytes_read, t_stack *stack, JITUINT32 type) {
    JITUINT32 		byte_alignment;
    t_label                 *label_l1;
    ir_instruction_t        *instruction;
#ifdef DEBUG
    JITUINT32 		stackTop;
#endif

    /* Assertions                           */
    assert(current_label_ID != NULL);
    assert(labels != NULL);
    assert(cilStack != NULL);
    assert(stack != NULL);
    assert(system != NULL);
    assert(method != NULL);

    /* Check if runtime checks are enabled			*/
    if (!(system->cliManager.CLR).runtimeChecks) {

        /* The runtime checks have been disabled		*/
        return 0;
    }

    /* Fetch the stack top						*/
#ifdef DEBUG
    stackTop = stack->top;
#endif

    /* Clean the top of the stack           		*/
    stack->cleanTop(stack);

    /* Reload the address on the stack 			*/
    memcpy(&(stack->stack[(stack->top)]), &(stack->stack[(stack->top) - 1]), sizeof(ir_item_t));
    (stack->top)++;
    switch ((stack->stack[(stack->top) - 1]).internal_type) {
        case IRMPOINTER:
        case IRTPOINTER:
        case IROBJECT:
            PDEBUG("CILIR: COERCING THE address VALUE to a IRNINT \n");
            _perform_conversion(&(system->cliManager), method, bytes_read, stack, IRNINT, JITFALSE, NULL);
            break;
    }

    /* Print the stack					*/
#ifdef PRINTDEBUG
    PDEBUG("TTVVA: START - RELOADED THE ADDRESS AS A IRNINT \n");
    print_stack(stack, method->getParametersNumber(method), method->getLocalsNumber(method));
#endif

    /* Fetch the alignment					*/
    if (type == IROBJECT) {
        type = IRNINT;
    }
    byte_alignment	= getIRSize(type);

    /* Clean the top of the stack           		*/
    stack->cleanTop(stack);

    /* Load the byte_alignment value on the stack 		*/
    (stack->stack[stack->top]).value.v = byte_alignment;
    (stack->stack[stack->top]).type = IRINT32;
    (stack->stack[stack->top]).internal_type = IRINT32;
    (stack->stack[stack->top]).type_infos = NULL;
    (stack->top)++;

    /* Print the stack					*/
#ifdef PRINTDEBUG
    PDEBUG("TTVVA: START - RELOADED THE ADDRESS AS A IRNINT + IRINT32 ALIGNMENT \n");
    print_stack(stack, method->getParametersNumber(method), method->getLocalsNumber(method));
#endif

    /* Verify if the value is correctly aligned 		*/
    _translate_arithmetic_operation(&(system->cliManager), method, bytes_read, stack, IRREM);

    /* Print the stack					*/
#ifdef PRINTDEBUG
    PDEBUG("TTVVA: START - BOOLEAN (ISaLIGNED) \n");
    print_stack(stack, method->getParametersNumber(method), method->getLocalsNumber(method));
#endif

    /* Clean the top of the stack           		*/
    stack->cleanTop(stack);

    /* Test if the value on the stack is 0 */
    /* Load 0 on the stack */
    (stack->stack[stack->top]).value.v = 0;
    (stack->stack[stack->top]).type = (stack->stack[stack->top - 1]).internal_type;
    (stack->stack[stack->top]).internal_type = (stack->stack[stack->top]).type;
    (stack->stack[stack->top]).type_infos = NULL;
    (stack->top)++;

    /* Insert the IREQ instruction		*/
    instruction = method->newIRInstr(method);
    instruction->type = IREQ;
    memcpy(&(instruction->param_1), &(stack->stack[(stack->top) - 2]), sizeof(ir_item_t));
    memcpy(&(instruction->param_2), &(stack->stack[(stack->top) - 1]), sizeof(ir_item_t));
    assert((instruction->param_1).internal_type == (instruction->param_2).internal_type);
    (stack->top) -= 2;

    /* Clean the top of the stack           */
    stack->cleanTop(stack);

    stack->stack[(stack->top)].value.v = method->incMaxVariablesReturnOld(method);
    stack->stack[(stack->top)].type = IROFFSET;
    stack->stack[(stack->top)].internal_type = IRINT32;
    (stack->stack[stack->top]).type_infos = NULL;
    (instruction->result).value.v = (stack->stack[stack->top]).value.v;
    (instruction->result).type = (stack->stack[stack->top]).type;
    (instruction->result).internal_type = (stack->stack[stack->top]).internal_type;
    (stack->top)++;
    instruction->byte_offset = bytes_read;

    /* if value is not true, throw a NullReferenceException */
    /* translate the branch instruction */
    instruction = method->newIRInstr(method);
    instruction->type = IRBRANCHIF;
    instruction->byte_offset = bytes_read;
    (stack->top)--;
    (instruction->param_1).value.v = (stack->stack[stack->top]).value.v;
    (instruction->param_1).type = (stack->stack[stack->top]).type;
    (instruction->param_1).internal_type = (stack->stack[stack->top]).internal_type;
    assert((instruction->param_1).internal_type == IRINT32);
    label_l1 = (t_label *) allocMemory(sizeof(t_label));
    label_l1->ID = (*current_label_ID);
    label_l1->counter = 0;
    label_l1->ir_position = method->getInstructionsNumber(method) - 1;
    label_l1->byte_offset = bytes_read;
    label_l1->type = DEFAULT_LABEL_TYPE;
    cil_insert_label(cilStack, labels, label_l1, stack, -1);
    (*current_label_ID)++;
    assert(IRMETHOD_getLabelInstruction(method->getIRMethod(method), label_l1->ID) == NULL);
    (instruction->param_2).value.v = label_l1->ID;
    (instruction->param_2).type = IRLABELITEM;
    (instruction->param_2).internal_type = IRLABELITEM;

    PDEBUG("CILIR: test valid address:      invalid_Address detected! \n");
    _translate_throw_CIL_Exception(&(system->cliManager), method, cilStack, current_label_ID, labels, bytes_read, stack, (system->exception_system)._System_NullReferenceException_ID);

    /* Fix up the label L2				*/
    PDEBUG("CILIR: test valid address:      Fixup the label %d\n", label_l1->ID);
    instruction = method->newIRInstr(method);
    instruction->type = IRLABEL;
    instruction->byte_offset = bytes_read;
    (instruction->param_1).value.v = label_l1->ID;
    (instruction->param_1).type = IRLABELITEM;
    (instruction->param_1).internal_type = IRLABELITEM;

    /* Print the stack				*/
#ifdef PRINTDEBUG
    PDEBUG("TTVVA: EXIT \n");
    print_stack(stack, method->getParametersNumber(method), method->getLocalsNumber(method));
#endif

    /* Check the stack top				*/
#ifdef DEBUG
    assert(stackTop == stack->top);
#endif

    return 0;
}

static inline void invalidate_blocks_starting_at_label (XanList *labels, t_label *label) {
    t_try_block     *current_try_block;
    XanListItem             *current_block;

    assert(labels != NULL);
    assert(label != NULL);
    assert(xanList_length(labels) != 0);


    /* Label is invalid. We have to
     * delete all the informations regarding the blocks
     * that start at this offset. To do this we have to
     * delete also the unused labels associated with */

    PDEBUG("CILIR: INVALID LABEL FOUND into the invalidated basic block \n");
    assert(label->starting_handler == NULL);

    /* 1. remove the TRY block
     * 2. remove the labels associated with the try block */
    current_block = xanList_first(label->starting_blocks);
    assert(current_block != NULL);
    while (current_block != NULL) {
        current_try_block = (t_try_block *) current_block->data;
        assert(current_try_block != NULL);

        invalidate_A_block(labels, current_try_block);
        freeMemory(current_try_block);
        current_block = current_block->next;
    }

    /* remove the list of blocks */
    xanList_emptyOutList(label->starting_blocks);
}

static inline void invalidate_A_block (XanList *labels, t_try_block *block) {
    JITUINT32 try_start_label_ID;
    JITUINT32 try_end_label_ID;
    XanListItem             *current_label;
    t_label         *current_label_item;

    assert(labels != NULL);
    assert(block != NULL);

    /* initialize the local variables */
    try_start_label_ID = block->try_start_label_ID;
    try_end_label_ID = block->try_end_label_ID;

    current_label = xanList_first(labels);
    current_label_item = NULL;

    assert(current_label != NULL);
    while (current_label != NULL) {
        current_label_item = (t_label *) current_label->data;
        assert(current_label_item != NULL);
        if (current_label_item->ID == try_start_label_ID) {
            break;
        }
        current_label = current_label->next;
    }
    assert(current_label != NULL);
    assert(current_label_item != NULL);

    /* remove the current try block */
    xanList_removeElement(current_label_item->starting_blocks, block, JITTRUE);

    if (xanList_length(current_label_item->starting_blocks) == 0) {
        assert(current_label_item->ending_block == NULL);
        assert(current_label_item->ending_handler == NULL);
        xanList_removeElement(labels, current_label_item, JITTRUE);
    }

    current_label = xanList_first(labels);
    current_label_item = NULL;

    assert(current_label != NULL);
    while (current_label != NULL) {
        current_label_item = (t_label *) current_label->data;
        assert(current_label_item != NULL);
        if (current_label_item->ID == try_end_label_ID) {
            break;
        }
        current_label = current_label->next;
    }
    assert(current_label != NULL);
    assert(current_label_item != NULL);

    /* remove the current try block */
    current_label_item->ending_block = NULL;

    if (    (current_label_item->ending_block == NULL)
            &&      (xanList_length(current_label_item->starting_blocks) == 0)
            &&      (current_label_item->starting_handler == NULL)
            &&      (current_label_item->ending_handler == NULL)
       ) {
        xanList_removeElement(labels, current_label_item, JITTRUE);
    }

    /* now we have to remove all the labels regarding the handlers of this block */
    assert( (block->catch_handlers != NULL)
            || (block->finally_handlers != NULL));
    if (block->catch_handlers != NULL) {
        /* remove all the catch handlers associated with this protected try block */
        XanListItem             *current_block;
        t_try_handler           *current_handler;

        /* initialize the locals */
        current_block = xanList_first(block->catch_handlers);
        current_handler = NULL;
        assert(current_block != NULL);
        while (current_block != NULL) {
            current_handler = (t_try_handler *) current_block->data;
            assert(current_handler != NULL);

            invalidate_An_Handler(labels, current_handler);
            freeMemory(current_handler);

            current_block = current_block->next;
        }

        /* remove the catch_handlers list */
        xanList_destroyList(block->catch_handlers);
        block->catch_handlers = NULL;
    }

    if (block->finally_handlers != NULL) {
        /* remove all the catch handlers associated with this protected try block */
        XanListItem             *current_block;
        t_try_handler           *current_handler;

        /* initialize the locals */
        current_block = xanList_first(block->finally_handlers);
        current_handler = NULL;

        assert(current_block != NULL);
        while (current_block != NULL) {
            current_handler = (t_try_handler *) current_block->data;
            assert(current_handler != NULL);

            invalidate_An_Handler(labels, current_handler);
            freeMemory(current_handler);

            current_block = current_block->next;
        }

        /* remove the catch_handlers list */
        xanList_destroyList(block->finally_handlers);
        block->finally_handlers = NULL;
    }

    if (block->inner_try_blocks != NULL) {
        /* this block contains inner protected try blocks */
        XanListItem     *current_try_block;

        current_try_block = xanList_first(block->inner_try_blocks);
        assert(current_try_block != NULL);
        while (current_try_block != NULL) {
            t_try_block     *try_block;

            try_block = (t_try_block *) current_try_block->data;
            invalidate_A_block(labels, try_block);
            freeMemory(try_block);

            current_try_block = current_try_block->next;
        }

        /* remove the inner_try_blocks list */
        xanList_destroyList(block->inner_try_blocks);
        block->inner_try_blocks = NULL;
    }
}

static inline void invalidate_An_Handler (XanList *labels, t_try_handler *current_handler) {
    XanListItem             *current_label;
    t_label         *current_label_item;
    JITUINT32 handler_start_label_ID;
    JITUINT32 handler_end_label_ID;
    JITUINT32 filter_start_label_ID;

    /* assertions */
    assert(current_handler != NULL);
    assert(labels != NULL);

    /* initialize locals */
    current_label = xanList_first(labels);
    current_label_item = NULL;
    filter_start_label_ID = -1;

    /* initialize the local variables */
    handler_start_label_ID = current_handler->handler_start_label_ID;
    handler_end_label_ID = current_handler->handler_end_label_ID;
    if (current_handler->type == FILTER_TYPE) {
        filter_start_label_ID = current_handler->filter_start_label_ID;
    }

    assert(current_label != NULL);
    while (current_label != NULL) {
        current_label_item = (t_label *) current_label->data;
        assert(current_label_item != NULL);
        if (current_label_item->ID == handler_start_label_ID) {
            break;
        }
        current_label = current_label->next;
    }
    assert(current_label != NULL);
    assert(current_label_item != NULL);

    /* remove the current label */
    current_label_item->starting_handler = NULL;

    if (    (current_label_item->ending_block == NULL)
            &&      (xanList_length(current_label_item->starting_blocks) == 0)
            &&      (current_label_item->starting_handler == NULL)
            &&      (current_label_item->ending_handler == NULL)
       ) {
        xanList_removeElement(labels, current_label_item, JITTRUE);
    }

    current_label = xanList_first(labels);
    current_label_item = NULL;

    assert(current_label != NULL);
    while (current_label != NULL) {
        current_label_item = (t_label *) current_label->data;
        assert(current_label_item != NULL);
        if (current_label_item->ID == handler_end_label_ID) {
            break;
        }
        current_label = current_label->next;
    }
    assert(current_label != NULL);
    assert(current_label_item != NULL);

    /* remove the current label */
    current_label_item->ending_handler = NULL;

    if (    (current_label_item->ending_block == NULL)
            &&      (xanList_length(current_label_item->starting_blocks) == 0)
            &&      (current_label_item->starting_handler == NULL)
            &&      (current_label_item->ending_handler == NULL)
       ) {
        xanList_removeElement(labels, current_label_item, JITTRUE);
    }

    if (current_handler->type == FILTER_TYPE) {
        current_label = xanList_first(labels);
        current_label_item = NULL;

        assert(current_label != NULL);
        while (current_label != NULL) {
            current_label_item = (t_label *) current_label->data;
            assert(current_label_item != NULL);
            if (current_label_item->ID == filter_start_label_ID) {
                break;
            }
            current_label = current_label->next;
        }
        assert(current_label != NULL);
        assert(current_label_item != NULL);

        /* remove the current label */
        current_label_item->starting_handler = NULL;

        if (    (current_label_item->ending_block == NULL)
                &&      (xanList_length(current_label_item->starting_blocks) == 0)
                &&      (current_label_item->starting_handler == NULL)
                &&      (current_label_item->ending_handler == NULL)
           ) {
            xanList_removeElement(labels, current_label_item, JITTRUE);
        }
    }
}

static inline JITINT16 internal_translate_ldlen (CLIManager_t *cliManager, t_running_garbage_collector *gc, Method method, JITUINT32 bytes_read, t_stack *stack) {
    ir_instruction_t        *instruction;

    /* Assertions				*/
    assert(method != NULL);
    assert(stack != NULL);
    assert((stack->stack[(stack->top) - 1]).internal_type == IROBJECT);

    /* Load the length			*/
    (stack->top)--;
    instruction = method->newIRInstr(method);
    instruction->type = IRARRAYLENGTH;
    memcpy(&(instruction->param_1), &(stack->stack[stack->top]), sizeof(ir_item_t));

    /* Clean the top of the stack           */
    stack->cleanTop(stack);

    stack->stack[stack->top].value.v = method->incMaxVariablesReturnOld(method);
    stack->stack[stack->top].type = IROFFSET;
    stack->stack[stack->top].internal_type = IRINT32;
    (stack->stack[stack->top]).type_infos = (cliManager->metadataManager)->getTypeDescriptorFromIRType(cliManager->metadataManager, IRINT32);
    memcpy(&(instruction->result), &(stack->stack[stack->top]), sizeof(ir_item_t));
    instruction->byte_offset = bytes_read;
    (stack->top)++;

    /* Return				*/
    return 0;
}

static inline JITINT16 translate_cil_ldlen (CLIManager_t *cliManager, t_running_garbage_collector *gc, Method method, CILStack cilStack, JITUINT32 bytes_read, t_stack *stack, JITUINT32 *current_label_ID, XanList *labels) {

    /* Assertions			*/
    assert(method != NULL);
    assert(cilStack != NULL);
    assert(stack != NULL);
    assert(labels != NULL);
    assert((stack->stack[(stack->top) - 1]).internal_type == IROBJECT);

    /* Check that the array is not	*
     * a null reference		*/
    translate_Test_Null_Reference(cliManager, method, cilStack, bytes_read, stack);

    /* Load the length of the array	*/
    internal_translate_ldlen(cliManager, gc, method, bytes_read, stack);

    /* Return				*/
    return 0;
}

/**
 * @brief Perform the conversion of the value on top of the stack to another internal type
 *
 * @param toType the IR type we want the value to be converted to.
 * @param before if it is not NULL, the IR instructions to perform the conversion will be inserted before the specified instruction.
 *               If it is NULL, they will be put at the end of the current method.
 */
static inline JITBOOLEAN _perform_conversion (CLIManager_t *cliManager, Method method, JITUINT32 bytes_read, t_stack *stack, JITUINT32 toType, JITBOOLEAN with_overflow_check, ir_instruction_t *before) {
    ir_instruction_t        *instruction;

    /* Assertions                           */
    assert(stack != NULL);
    assert(method != NULL);
    assert(toType != NOPARAM);
    assert(toType != IRVALUETYPE);

    /* Initialize the local variable        */
    instruction = NULL;

    /* Make the IR instruction		*/
    instruction = method->newIRInstrBefore(method, before);
    (stack->top)--;
    memcpy(&(instruction->param_1), &(stack->stack[stack->top]), sizeof(ir_item_t));
    (instruction->param_2).value.v = toType;
    (instruction->param_2).type = IRTYPE;
    (instruction->param_2).internal_type = IRTYPE;
    if (with_overflow_check) {
        instruction->type = IRCONVOVF;
    } else {
        instruction->type = IRCONV;
    }

    /* Clean the top of the stack           */
    stack->cleanTop(stack);

    (stack->stack[stack->top]).value.v = method->incMaxVariablesReturnOld(method);
    (stack->stack[stack->top]).type = IROFFSET;
    (stack->stack[stack->top]).internal_type = toType;
    switch ((instruction->param_1).internal_type) {
        case IROBJECT:
            (stack->stack[stack->top]).type_infos = (instruction->param_1).type_infos;
            break;
        default:
            (stack->stack[stack->top]).type_infos = cliManager->metadataManager->getTypeDescriptorFromIRType(cliManager->metadataManager, toType);
    }
    memcpy(&(instruction->result), &(stack->stack[stack->top]), sizeof(ir_item_t));
    (stack->top)++;
    instruction->byte_offset = bytes_read;
    assert((instruction->result).type == IROFFSET);
    assert((instruction->param_1).internal_type != IRVALUETYPE);

    /* Return				*/
    return 0;
}

static inline JITBOOLEAN _translate_binary_operation (Method method, JITUINT32 bytes_read, t_stack *stack, JITUINT32 IRop, JITUINT32 return_internal_type) {
    ir_instruction_t        *instruction;

    /* assertions */
    assert(method != NULL);
    assert(stack != NULL);
    assert(return_internal_type != NOPARAM);
    PDEBUG("CILIR: _translate_binary_operation : PERFORMING A %d OPERATION \n", IRop);
    PDEBUG("CILIR: _translate_binary_operation : FIRST_PARAMETER is %d \n", (stack->stack[stack->top - 2]).internal_type);
    PDEBUG("CILIR: _translate_binary_operation : SECOND_PARAMETER is %d \n", (stack->stack[stack->top - 1]).internal_type);
    PDEBUG("CILIR: _translate_binary_operation : RETURN_TYPE is %d \n", return_internal_type);

    /* Initialize the local variables       */
    instruction 	= method->newIRInstr(method);
    assert(instruction != NULL);
    (stack->top)--;
    instruction->type = IRop;
    memcpy(&(instruction->param_2), &(stack->stack[stack->top]), sizeof(ir_item_t));
    (stack->top)--;
    memcpy(&(instruction->param_1), &(stack->stack[stack->top]), sizeof(ir_item_t));

    /* Clean the top of the stack           */
    stack->cleanTop(stack);

    stack->stack[stack->top].value.v = method->incMaxVariablesReturnOld(method);
    stack->stack[stack->top].type = IROFFSET;
    stack->stack[stack->top].internal_type = return_internal_type;
    (stack->stack[stack->top]).type_infos = (instruction->param_1).type_infos;
    if ((stack->stack[stack->top]).type_infos == NULL) {
        (stack->stack[stack->top]).type_infos = (instruction->param_2).type_infos;
    }
    memcpy(&(instruction->result), &(stack->stack[stack->top]), sizeof(ir_item_t));
    (stack->top)++;
    instruction->byte_offset = bytes_read;
    assert((instruction->result).internal_type != NOPARAM);

    /* Return				*/
    return 0;
}

MethodDescriptor * retrieve_CILException_Ctor_MethodID_by_type (t_methods *methods, TypeDescriptor *type) {
    MethodDescriptor *ctor = NULL;
    Method ctor_method;

    /* Assertions				*/
    assert(methods != NULL);
    assert(type != NULL);
    PDEBUG("CILIR: retrieve_CILException_Ctor_MethodID: Start\n");

    /* FETCH THE TOKEN FOR THE .ctor METHOD */
    XanList *methods_list = type->getConstructors(type);
    XanListItem *item = xanList_first(methods_list);
    while (item != NULL) {
        ctor = (MethodDescriptor *) item->data;
        ctor_method = methods->fetchOrCreateMethod(methods, ctor, JITFALSE);
#ifdef DEBUG
        ir_method_t *irMethod;
        if (ctor_method->getParametersNumber(ctor_method) > 0) {
            /* retrieve the irMethod */
            irMethod = ctor_method->getIRMethod(ctor_method);
            assert(irMethod != NULL);
            /* verify if the signature IR has defined a number of parameters
             * that is different from zero */
            assert((irMethod->signature).parameterTypes != NULL);
        }
#endif
        if (ctor_method->getParametersNumber(ctor_method) == 1) {
            /* we have found the correct constructor */
            PDEBUG("CILIR: retrieve_CILException_Ctor_MethodID:     Found the ctor with 1 parameter\n");
            break;
        }
        item = item->next;
    }
    assert(ctor != NULL);
    assert(ctor_method != NULL);
    assert(ctor_method->getParametersNumber(ctor_method) == 1);
    assert(ctor_method->getID(ctor_method) == ctor);

    /* Return					*/
    PDEBUG("CILIR: retrieve_CILException_Ctor_MethodID: Exit\n");
    return ctor;
}

static inline JITINT16 translate_cil_load_fconstant (CLIManager_t *cliManager, Method method, JITUINT32 bytes_read, IR_ITEM_FVALUE value, JITUINT32 type, t_stack *stack) {
    ir_instruction_t        *instruction;

    /* Assertions			*/
    assert(method != NULL);
    assert(stack != NULL);

    instruction = method->newIRInstr(method);
    instruction->type = IRMOVE;
    (instruction->result).value.v = method->incMaxVariablesReturnOld(method);
    (instruction->result).type = IROFFSET;
    (instruction->result).internal_type = type;
    (instruction->result).type_infos = (cliManager->metadataManager)->getTypeDescriptorFromIRType(cliManager->metadataManager, type);
    (instruction->param_1).value.f = value;
    (instruction->param_1).type = type;
    (instruction->param_1).internal_type = type;
    (instruction->param_1).type_infos = (cliManager->metadataManager)->getTypeDescriptorFromIRType(cliManager->metadataManager, type);
    instruction->byte_offset = bytes_read;
    assert((instruction->result).internal_type == (instruction->param_1).internal_type);

    /* Clean the top of the stack           */
    stack->cleanTop(stack);

    /* Load the new variable on the	*
     * stack			*/
    memcpy(&(stack->stack[stack->top]), &(instruction->result), sizeof(ir_item_t));
    (stack->top)++;

    /* Return			*/
    return 0;
}

static inline JITINT16 translate_cil_load_constant (CLIManager_t *cliManager, Method method, JITUINT32 bytes_read, IR_ITEM_VALUE value, JITUINT32 type, t_stack *stack) {
    ir_instruction_t        *instruction;

    /* Assertions			*/
    assert(method != NULL);
    assert(stack != NULL);

    instruction = method->newIRInstr(method);
    instruction->type = IRMOVE;
    (instruction->result).value.v = method->incMaxVariablesReturnOld(method);
    (instruction->result).type = IROFFSET;
    (instruction->result).internal_type = type;
    (instruction->result).type_infos = (cliManager->metadataManager)->getTypeDescriptorFromIRType(cliManager->metadataManager, type);
    (instruction->param_1).value.v = value;
    (instruction->param_1).type = type;
    (instruction->param_1).internal_type = type;
    instruction->byte_offset = bytes_read;
    assert((instruction->result).internal_type == (instruction->param_1).internal_type);

    /* Clean the top of the stack           */
    stack->cleanTop(stack);

    /* Load the new variable on the	*
     * stack			*/
    memcpy(&(stack->stack[stack->top]), &(instruction->result), sizeof(ir_item_t));
    (stack->top)++;

    /* Return			*/
    return 0;
}

static inline JITINT16 translate_cil_checknull (Method method, JITUINT32 bytes_read, t_stack *stack) {
    ir_instruction_t        *instruction;

    /* Assertions           */
    assert(method != NULL);
    assert(stack != NULL);
    assert((stack->stack[(stack->top) - 1]).type == IROFFSET);

    /* Generate the instruction */
    instruction = method->newIRInstr(method);
    instruction->type = IRCHECKNULL;
    memcpy(&(instruction->param_1), &(stack->stack[stack->top - 1]), sizeof(ir_item_t));
    instruction->byte_offset = bytes_read;

    /* Return		*/
    return 0;
}

static inline JITINT16 translate_cil_dup (Method method, JITUINT32 bytes_read, t_stack *stack) {

    /* assertions */
    assert(method != NULL);
    assert(stack != NULL);

    if ((stack->stack[stack->top - 1]).type != IROFFSET) {
        assert((stack->stack[stack->top - 1]).internal_type != IRVALUETYPE);

        /* Clean the top of the stack                                   */
        stack->cleanTop(stack);

        /* simply copy the content of the constant value on the stack   */
        memcpy(&(stack->stack[stack->top]), &(stack->stack[stack->top - 1]), sizeof(ir_item_t));
        (stack->top)++;

        /* Return							*/
        return 0;
    }
    assert((stack->stack[stack->top - 1]).type == IROFFSET);

    /* Dup the value to store			*/
    stack->dupTop(stack);
    stack->dupTop(stack);
    (stack->stack[stack->top - 1]).value.v = method->incMaxVariablesReturnOld(method);

    /* Store the value to a new variable		*/
    translate_cil_move( method, bytes_read, stack);

    /* Return					*/
    return 0;
}

static inline JITINT8 convertEnumIntoInt32 (ir_item_t *item) {
    JITINT8 result;

    /* Assertions				*/
    assert(item != NULL);

    /* Check if the operands are an		*
     * Enum					*/
    result = 0;
    if ((item->internal_type == IRVALUETYPE)) {
        TypeDescriptor *type = item->type_infos;
        if (type->isEnum(type)) {
            result = 1;
            item->internal_type = IRINT32;
        }
    }

    /* Return				*/
    return result;
}

static inline JITINT8 translate_cil_sizeof (CLIManager_t *cliManager, Method method, JITUINT32 bytes_read, t_stack *stack, GenericContainer *container, t_binary_information *binary, JITUINT32 token) {
    TypeDescriptor 		*resolved_type;
    JITUINT32 		irType;
    JITINT32 		toType;
    JITINT32 		fromType;
    ir_instruction_t	*instruction;

    /* Assertions				*/
    assert(stack != NULL);
    assert(binary != NULL);

    /* Fetch the metadata			*/
    resolved_type   			= cliManager->metadataManager->getTypeDescriptorFromToken(cliManager->metadataManager, container, binary, token);
    assert(resolved_type != NULL);

    /* Layout the type			*/
    (cliManager->layoutManager).layoutType(&(cliManager->layoutManager), resolved_type);

    /* Fetch the IR type			*/
    irType 					= resolved_type->getIRType(resolved_type);

    /* Produce the IR instruction		*/
    instruction 				= method->newIRInstr(method);
    instruction->type 			= IRSIZEOF;
    (instruction->param_1).value.v		= irType;
    (instruction->param_1).type_infos	= resolved_type;
    (instruction->param_1).type		= IRTYPE;
    (instruction->param_1).internal_type	= (instruction->param_1).type;
    (instruction->result).value.v		= method->incMaxVariablesReturnOld(method);
    (instruction->result).type		= IROFFSET;
    (instruction->result).internal_type	= IRUINT32;

    /* Clean the top of the stack           */
    stack->cleanTop(stack);

    /* Push the result to the stack		*/
    memcpy(&(stack->stack[stack->top]), &(instruction->result), sizeof(ir_item_t));
    (stack->top)++;

    /* The value that is on top of the stack*
     * will remain there after this opcode,	*
     * so it has to be stackable 		*/
    toType          = (cliManager->metadataManager->generic_type_rules)->coerce_CLI_Type(stack->stack[stack->top - 1].internal_type);
    fromType        = stack->stack[stack->top - 1].internal_type;

    /* Perform the conversion		*/
    if (fromType != toType) {
        _perform_conversion(cliManager, method, bytes_read, stack, toType, JITFALSE, NULL);
    }

    /* Return				*/
    return 0;
}

static inline JITINT16 translate_cil_cpblk (t_system *system, CILStack cilStack, Method method, JITUINT32 bytes_read, t_stack *stack, JITUINT32 *current_label_ID, XanList *labels, JITUINT8 alignment) {
    ir_instruction_t        *instruction;

    /* Assertions				*/
    assert(system != NULL);
    assert(method != NULL);
    assert(stack != NULL);
    assert(method->getIRMethod(method) != NULL);
    assert(current_label_ID != NULL);
    assert(labels != NULL);

    /* Clean the top of the stack           */
    stack->cleanTop(stack);

    /* Test if the src address has a correct alignment  */
    memcpy(&(stack->stack[(stack->top)]), &(stack->stack[(stack->top) - 2]), sizeof(ir_item_t));
    (stack->top)++;
    switch (alignment) {
        case 0:
            /* Test if is valid the target address in which we should store the value */
            translate_Test_Valid_Value_Address(system, method, cilStack, current_label_ID, labels, bytes_read, stack, IRNUINT);
            break;
        case 1:
            /* Test if is valid the target address in which we should store the value */
            translate_Test_Valid_Value_Address(system, method, cilStack, current_label_ID, labels, bytes_read, stack, IRINT8);
            break;
        case 2:
            /* Test if is valid the target address in which we should store the value */
            translate_Test_Valid_Value_Address(system, method, cilStack, current_label_ID, labels, bytes_read, stack, IRINT16);
            break;
        case 4:
            /* Test if is valid the target address in which we should store the value */
            translate_Test_Valid_Value_Address(system, method, cilStack, current_label_ID, labels, bytes_read, stack, IRINT32);
            break;
        default:
            print_err("CILIR: ERROR = invalid alignment for cpblk. ", 0);
            abort();
    }

    /* Pop the reloaded address from the stack */
    (stack->top)--;

    /* Clean the top of the stack           */
    stack->cleanTop(stack);

    /* Test if the dest address has a correct alignment  */
    memcpy(&(stack->stack[(stack->top)]), &(stack->stack[(stack->top) - 3]), sizeof(ir_item_t));
    (stack->top)++;
    /* Test if the target address in which we should store the value is correctly aligned */
    switch (alignment) {
        case UNALIGNED_DEFAULT_ALIGNMENT:
            translate_Test_Valid_Value_Address(system, method, cilStack, current_label_ID, labels, bytes_read, stack, IRNUINT);
            break;
        case UNALIGNED_BYTE_ALIGNMENT:
            translate_Test_Valid_Value_Address(system, method, cilStack, current_label_ID, labels, bytes_read, stack, IRINT8);
            break;
        case UNALIGNED_DOUBLEBYTE_ALIGNMENT:
            translate_Test_Valid_Value_Address(system, method, cilStack, current_label_ID, labels, bytes_read, stack, IRINT16);
            break;
        case UNALIGNED_QUADBYTE_ALIGNMENT:
            translate_Test_Valid_Value_Address(system, method, cilStack, current_label_ID, labels, bytes_read, stack, IRINT32);
            break;
        default:
            print_err("CILIR: ERROR = invalid alignment for cpblk. ", 0);
            abort();
    }
    /* Pop the reloaded address from the stack */
    (stack->top)--;

    /* Call the IRMEMCPY instruction in order to set the memory area of the object */
    instruction = method->newIRInstr(method);
    instruction->type = IRMEMCPY;
    (stack->top)--;
    /* Fetch the size of the memory region to copy */
    assert(((stack->stack[stack->top].internal_type == IRINT32) || (stack->stack[stack->top].internal_type == IRUINT32)));
    memcpy(&(instruction->param_3), &(stack->stack[stack->top]), sizeof(ir_item_t));
    (stack->top)--;
    /* Fetch the src address */
    (instruction->param_2).value.v = stack->stack[stack->top].value.v;
    (instruction->param_2).type = stack->stack[stack->top].type;
    (instruction->param_2).internal_type = stack->stack[stack->top].internal_type;
    make_ir_infos(&(instruction->param_2), &(stack->stack[stack->top]));
    (stack->top)--;
    /* Fetch the dest address */
    memcpy(&(instruction->param_1), &(stack->stack[stack->top]), sizeof(ir_item_t));
    instruction->byte_offset = bytes_read;
    PDEBUG("CILIR:			Insert a IRMEMCPY instruction\n");

    /* Check the result		*/
#ifdef DEBUG
    if (sizeof(JITNUINT) == 4) {
        assert( ((instruction->param_1).internal_type == IRMPOINTER)
                || ((instruction->param_1).internal_type == IRTPOINTER)
                || ((instruction->param_1).internal_type == IROBJECT)
                || ((instruction->param_1).internal_type == IRNINT)
                || ((instruction->param_1).internal_type == IRINT32)
                || ((instruction->param_1).internal_type == IRUINT32) );
    } else {
        assert( ((instruction->param_1).internal_type == IRMPOINTER)
                || ((instruction->param_1).internal_type == IRTPOINTER)
                || ((instruction->param_1).internal_type == IROBJECT)
                || ((instruction->param_1).internal_type == IRNINT)
                || ((instruction->param_1).internal_type == IRINT64)
                || ((instruction->param_1).internal_type == IRUINT64) );
    }
#endif

    /* Return			*/
    return 0;
}

static inline JITINT16 translate_cil_initblk (t_system *system, CILStack cilStack, Method method, JITUINT32 bytes_read, t_stack *stack, JITUINT32 *current_label_ID, XanList *labels, JITUINT8 alignment) {
    /* Translate to a IRMEM instruction */
    ir_instruction_t        *instruction;

    /* Assertions				*/
    assert(system != NULL);
    assert(method != NULL);
    assert(stack != NULL);
    assert(method->getIRMethod(method) != NULL);
    assert(current_label_ID != NULL);
    assert(labels != NULL);

    /* Clean the top of the stack           */
    stack->cleanTop(stack);

    /* Test if the address has a correct alignment  */
    memcpy(&(stack->stack[(stack->top)]), &(stack->stack[(stack->top) - 3]), sizeof(ir_item_t));
    (stack->top)++;

    /* Test if the target address in which we should store the value is correctly aligned */
    switch (alignment) {
        case UNALIGNED_DEFAULT_ALIGNMENT:
            translate_Test_Valid_Value_Address(system, method, cilStack, current_label_ID, labels, bytes_read, stack, IRNUINT);
            break;
        case UNALIGNED_BYTE_ALIGNMENT:
            translate_Test_Valid_Value_Address(system, method, cilStack, current_label_ID, labels, bytes_read, stack, IRINT8);
            break;
        case UNALIGNED_DOUBLEBYTE_ALIGNMENT:
            translate_Test_Valid_Value_Address(system, method, cilStack, current_label_ID, labels, bytes_read, stack, IRINT16);
            break;
        case UNALIGNED_QUADBYTE_ALIGNMENT:
            translate_Test_Valid_Value_Address(system, method, cilStack, current_label_ID, labels, bytes_read, stack, IRINT32);
            break;
        default:
            print_err("CILIR: ERROR = invalid alignment for initblk. ", 0);
            abort();
    }
    /* Pop the reloaded address from the stack */
    (stack->top)--;

    /* Call the IRMEMCPY instruction in order to set the memory area of the object */
    instruction = method->newIRInstr(method);
    instruction->type = IRINITMEMORY;
    (stack->top)--;
    /* Fetch the size of the memory region to copy */
    assert(((stack->stack[stack->top].internal_type == IRINT32) || (stack->stack[stack->top].internal_type == IRUINT32)));
    memcpy(&(instruction->param_3), &(stack->stack[stack->top]), sizeof(ir_item_t));
    (stack->top)--;
    /* Fetch the value for the initialization */
    memcpy(&(instruction->param_2), &(stack->stack[stack->top]), sizeof(ir_item_t));
    assert( ((instruction->param_2).internal_type == IRINT8)
            || ((instruction->param_2).internal_type == IRINT32)
            || ((instruction->param_2).internal_type == IRUINT8) );
    (stack->top)--;
    /* Fetch the dest address */
    memcpy(&(instruction->param_1), &(stack->stack[stack->top]), sizeof(ir_item_t));
    assert( ((instruction->param_1).internal_type == IRMPOINTER)
            || ((instruction->param_1).internal_type == IRTPOINTER)
            || ((instruction->param_1).internal_type == IROBJECT)
            || ((instruction->param_1).internal_type == IRNINT)
            || ((instruction->param_1).internal_type == IRNINT) );

    instruction->byte_offset = bytes_read;
    PDEBUG("CILIR:			Insert a IRINITMEMORY instruction\n");

    return 0;
}

static inline JITINT16 translate_cil_calli (CLIManager_t *cliManager, CILStack cilStack, GenericContainer *container, t_binary_information *binary, Method method, JITUINT32 token, JITUINT32 bytes_read, t_stack *stack, JITBOOLEAN isTail, JITUINT32 *current_label_ID, XanList *labels) {
    ir_instruction_t                *instruction;
    JITUINT8 counter;
    XanList                         *params;
    ir_item_t function_pointer;
    ir_item_t                       *current_stack_item;
    ir_signature_t          *ir_signature;

    /* Assertions						*/
    assert(cliManager != NULL);
    assert(cilStack != NULL);
    assert(internal_isStackCorrect(cliManager, method, stack));
    PDEBUG("CILIR:	translate_cil_calli:	Start\n");

    /* Init the method struct				*/;
    instruction = NULL;
    params = NULL;
    ir_signature = NULL;
    current_stack_item = NULL;

    /* Setup the function call                              */
    TypeDescriptor *resolved_type = cliManager->metadataManager->getTypeDescriptorFromToken(cliManager->metadataManager, container, binary, token);
    assert(resolved_type->element_type = ELEMENT_FNPTR);
    FunctionPointerDescriptor *pointer = (FunctionPointerDescriptor *) resolved_type->descriptor;
    JITUINT32 param_count = xanList_length(pointer->params);
    ir_signature = (ir_signature_t *) (sharedAllocFunction(sizeof(ir_signature_t)));
    (cliManager->translator).translateActualMethodSignatureToIR(cliManager, NULL, pointer, ir_signature);

    /* Printing the retrieved metadata			*/
    assert(ir_signature != NULL);
    PDEBUG("CILIR:	translate_cil_calli:	method signature %s\n", pointer->getSignatureInString(pointer));
    PDEBUG("CILIR:	translate_cil_calli:	param count %d\n", param_count );
    PDEBUG("CILIR:	translate_cil_calli:	calling convention %d\n", pointer->calling_convention);
    PDEBUG("CILIR:	translate_cil_calli:	*** return type properties\n");
    PDEBUG("CILIR:	translate_cil_calli:	byref : %d\n", pointer->result->isByRef);
    PDEBUG("CILIR:	translate_cil_calli:	*** return type properties\n");

    /*
            We know the number of parameters and the signature in general for the libjit invocation.
            We must create a list of parameters (elements of type ir_item_t); then we translate an indirect call instruction for indirect invocation
     */
#ifdef PRINTDEBUG
    PDEBUG("CILIR:	translate_cil_calli:	printing the stack before the translation\n");
    print_stack(stack, method->getParametersNumber(method), method->getLocalsNumber(method));
#endif

    /* Popping from the stack the method to be called       */
    PDEBUG("CILIR:	translate_cil_calli:	popping the function pointer\n");
    (stack->top)--;
    memcpy(&function_pointer, &(stack->stack[stack->top]), sizeof(ir_item_t));

    /* Creating the parameter list				*/
    assert(internal_isStackCorrect(cliManager, method, stack));
    if (param_count > 0) {
        PDEBUG("CILIR:	translate_cil_calli:	creating the parameter list\n");
        params = xanList_new(sharedAllocFunction, freeFunction, NULL);
        assert(params != NULL);

        /* We loop popping parameters from the stack	*/
        for (counter = 0; counter < param_count; counter++) {
            PDEBUG("CILIR:	translate_cil_calli:	popping the parameter %d from the stack\n", counter);
            /* Updating the stack count		*/
            (stack->top)--;

            /* Extracting the parameter */
            current_stack_item = make_new_IR_param();
            assert(current_stack_item != NULL);
            (current_stack_item->value).v = (stack->stack[(stack->top)]).value.v;
            (current_stack_item->value).f = (stack->stack[(stack->top)]).value.f;
            current_stack_item->type = (stack->stack[(stack->top)]).type;
            current_stack_item->internal_type = (stack->stack[(stack->top)]).internal_type;
            make_ir_infos(current_stack_item, &(stack->stack[(stack->top)]));

            /* Appending the param to the list	*/
            xanList_insert(params, current_stack_item);
            assert(params != NULL);
        }
    } else {
        PDEBUG("CILIR:	translate_cil_calli:	no parameters\n");
    }
    assert(internal_isStackCorrect(cliManager, method, stack));

    /* Print the stack after popping the parameters */
#ifdef PRINTDEBUG
    PDEBUG("CILIR:	translate_cil_calli:	printing the stack after popping the parameters\n");
    print_stack(stack, method->getParametersNumber(method), method->getLocalsNumber(method));
#endif

    /*
            Now we must create the indirect call instruction:
            - the first parameter is the return type
            - the second parameter is the function name
            - the third parameter was not used before the calli implementation, now it stores the function pointer (ir_item_t)

            The callParameters field stores the list of parameters as ir_item_t instances
            The nfunc field MUST be NULL for indirect invocation
     */
    PDEBUG("CILIR:	translate_cil_calli:	translating...\n");
    instruction = method->newIRInstr(method);
    instruction->type = IRICALL;
    (instruction->param_1).value.v = pointer->result->getIRType(pointer->result);
    (instruction->param_1).type = IRTYPE;
    (instruction->param_1).internal_type = IRTYPE;
    (instruction->param_1).type_infos = pointer->result;
    (instruction->param_3).value.v = (JITNUINT) ir_signature;
    (instruction->param_3).type = IRSIGNATURE;
    (instruction->param_3).internal_type = IRSIGNATURE;

    /* Push the result                      */
    assert(internal_isStackCorrect(cliManager, method, stack));
    if ((instruction->param_1).value.v != IRVOID ) {
        JITINT32 toType;
        JITINT32 fromType;
        PDEBUG("CILIR:	translate_cil_calli:	return type is not IRVOID, but %llu\n", (instruction->param_1).value.v);

        /* Push the result to the stack		*/
        assert(stack->top >= (method->getLocalsNumber(method) + method->getParametersNumber(method)));
        stack->cleanTop(stack);
        (stack->stack[stack->top]).value.v = method->incMaxVariablesReturnOld(method);
        (stack->stack[stack->top]).type = IROFFSET;
        (stack->stack[stack->top]).internal_type = (instruction->param_1).value.v;
        (stack->stack[stack->top]).type_infos	= (instruction->param_1).type_infos;
        memcpy(&(instruction->result), &(stack->stack[stack->top]), sizeof(ir_item_t));
        assert((instruction->result).type == IROFFSET);
        (stack->top)++;

        /* Convert the result to a stackable    *
         * type.                                *
         * Delete aliases			*/
        fromType = stack->stack[stack->top - 1].internal_type;
        switch (fromType) {
            case IRMPOINTER:
            case IRNINT:
                fromType = IRNINT;
                break;
        }

        /* Perform the conversion		*/
        toType = (cliManager->metadataManager->generic_type_rules)->coerce_CLI_Type(fromType);
        if (fromType != toType) {
            _perform_conversion(cliManager, method, bytes_read, stack, toType, JITFALSE, NULL);
        }
    }
    instruction->byte_offset = bytes_read;
    instruction->callParameters = params;
    memcpy(&(instruction->param_2), &function_pointer, sizeof(ir_item_t));
    assert(internal_isStackCorrect(cliManager, method, stack));

    /* Print the stack                      */
#ifdef PRINTDEBUG
    PDEBUG("CILIR:	translate_cil_calli:	printing the stack after the translation\n");
    print_stack(stack, method->getParametersNumber(method), method->getLocalsNumber(method));
#endif
    assert(internal_isStackCorrect(cliManager, method, stack));

    /* Return                               */
    PDEBUG("CILIR:	translate_cil_calli:	Exit\n");
    return 0;
}

static inline JITINT16 translate_cil_ldcalloc (CILStack cilStack, Method method, JITUINT32 bytes_read, t_stack *stack, JITUINT32 *current_label_ID, XanList *labels) {
    ir_instruction_t        *instruction;

    /* Pop the size                         */
    (stack->top)--;

    /* Make the IR instruction		*/
    instruction = method->newIRInstr(method);
    assert(instruction != NULL);
    instruction->type = IRALLOCA;
    memcpy(&(instruction->param_1), &(stack->stack[stack->top]), sizeof(ir_item_t));

    /* Clean the top of the stack           */
    stack->cleanTop(stack);

    stack->stack[stack->top].value.v = method->incMaxVariablesReturnOld(method);
    stack->stack[stack->top].type = IROFFSET;
    stack->stack[stack->top].internal_type = IRMPOINTER;
    stack->stack[stack->top].type_infos = NULL;
    (instruction->result).value.v = (stack->stack[stack->top]).value.v;
    (instruction->result).type = (stack->stack[stack->top]).type;
    (instruction->result).internal_type = stack->stack[stack->top].internal_type;
    assert((instruction->result).type == IROFFSET);
    instruction->byte_offset = bytes_read;
    (stack->top)++;

    /* Return                               */
    return 0;
}

static inline JITINT16 translate_cil_ldnull (CLIManager_t *cliManager, Method method, JITUINT32 bytes_read, t_stack *stack) {
    JITINT16 result;

    /* Assertions                   */
    assert(method != NULL);
    assert(stack != NULL);

    /* Translate the ldnull opcode	*/
    result = translate_cil_load_constant(cliManager, method, bytes_read, 0, IROBJECT, stack);

    /* Return				*/
    return result;
}

static inline JITINT16 translate_cil_switch (CLIManager_t *cliManager, CILStack cilStack, Method method, JITUINT32 *bytes_read, t_stack *stack, JITUINT32 *current_label_ID, XanList *labels, JITINT8 *body_stream) {
    /*
            Refer to page 424 of ECMA-335 4th edition, June 2006

            When we enter this clause, body_stream + bytes_read points to the number of targets the jump table contains;
            the element on the top of the stack is the value discriminating when we need to jump to the target.
            Once we retrieved the number of targers, we setup a for loop to execute the confront to each target within the jump table.
            if value is less than the current target iterator, the jump takes place.
     */
    /* The element on top of the stack representing the value to confront	*/
    ir_item_t switch_value;
    /* The number of targets within the jump table				*/
    JITUINT32 switch_num_targets;
    /* The current target being considered					*/
    JITINT32 switch_current_tgt;
    JITUINT32 switch_instruction_size;
    JITUINT32 from_offset;
    JITUINT32 count;
    JITBOOLEAN is_invalid_basic_block;

    /* Initialize the variables						*/
    is_invalid_basic_block = JITFALSE;

    /* Retrieving the number of targets within the jump table		*/
    read_from_buffer(body_stream, *bytes_read, 4, &switch_num_targets);
    (*bytes_read) += 4;
    PDEBUG("CILIR: The switch jump table contains %u targets\n", switch_num_targets);
    if (switch_num_targets == 0) {

        /* Return			*/
        PDEBUG("CILIR: done with the switch\n");
        return is_invalid_basic_block;
    }

    /* Retrieving the instruction size		                        */
    switch_instruction_size = (5 + (switch_num_targets * 4));
    from_offset = (*bytes_read) + (switch_num_targets * 4);

    /* Popping the value on top of the stack				*/
    PDEBUG("CILIR: %d elements on the stack so far\n", (stack->top) );
    memcpy(&switch_value, &(stack->stack[(stack->top) - 1]), sizeof(ir_item_t));
    (stack->top)--;
    PDEBUG("CILIR: one element popped from the stack, %d elements remaining\n", (stack->top) );

    /* Looping over the jump table                                          */
    for (count = 0; count < switch_num_targets; count++) {

        /* Retrieving the current target from the jump table */
        read_from_buffer(body_stream, *bytes_read, 4, &switch_current_tgt);
        (*bytes_read) += 4;
        PDEBUG("CILIR: Considering target %u, offset %d\n", count, switch_current_tgt);
        /*
                We have to push the two elements for the confront on the stack
                The check MUST be performed considering the parameters to be 32 bit UNSIGNED INTEGERs
         */

        /* Clean the top of the stack           */
        stack->cleanTop(stack);

        /* Pushing the value		*/
        memcpy(&(stack->stack[stack->top]), &switch_value, sizeof(ir_item_t));
        (stack->top)++;

        /* Clean the top of the stack           */
        stack->cleanTop(stack);

        /* Pushing the target iterator	*/
        (stack->stack[stack->top]).value.v = count;
        (stack->stack[stack->top]).type = switch_value.internal_type;
        (stack->stack[stack->top]).internal_type = (stack->stack[stack->top]).type;
        (stack->top)++;

        /* Translating the confront: we provide the target as the offset to jump to */
        is_invalid_basic_block |= translate_cil_beq(cliManager, cilStack,  method, current_label_ID, from_offset, switch_current_tgt, labels, switch_instruction_size, stack);
    }
    PDEBUG("CILIR: done with the switch\n");

    /* Return			*/
    return is_invalid_basic_block;
}

static inline JITINT16 translate_cil_arglist (CLIManager_t *cliManager, CILStack cilStack, Method method, JITUINT32 bytes_read, t_stack *stack, JITUINT32 *current_label_ID, XanList *labels) {
    JITINT16 res;
    ILLayout        *Runtime_Handle;
    ir_instruction_t *inst;

    /* Assertions                   */
    assert(method != NULL);
    assert(labels != NULL);
    assert(current_label_ID != NULL);
    assert(stack != NULL);

    /* Fetch the type               */
    Runtime_Handle = (cliManager->layoutManager).getRuntimeArgumentHandleLayout(&(cliManager->layoutManager));
    assert(Runtime_Handle != NULL);
    assert(Runtime_Handle->type != NULL);

    /* Create an empty RuntimeArgumentHandle ValueType and push it on the stack */
    res = make_irnewValueType(cliManager, method, bytes_read, Runtime_Handle->type, stack, NULL);

    /* Get the address of the RuntimeArgumentHandle ValueType	*/
    inst = method->newIRInstr(method);
    inst->type = IRGETADDRESS;
    /* PARAM 1: RuntimeArgumentHandle we want to take the address of: */
    memcpy(&(inst->param_1), &(stack->stack[stack->top - 1]), sizeof(ir_item_t));

    /* Clean the top of the stack           */
    stack->cleanTop(stack);

    /* Push the address			*/
    stack->stack[stack->top].value.v = method->incMaxVariablesReturnOld(method);
    stack->stack[stack->top].type = IROFFSET;
    stack->stack[stack->top].internal_type = IROBJECT;
    if ((inst->param_1).type_infos != NULL) {
        stack->stack[stack->top].type_infos = (inst->param_1).type_infos->makePointerDescriptor((inst->param_1).type_infos);
    }

    /* Return value: the address we obtain */
    memcpy(&(inst->result), &(stack->stack[stack->top]), sizeof(ir_item_t));
    (stack->top)++;

    /* Init the RuntimeArgumentHandle */
    inst = method->newIRInstr(method);
    inst->type = IRSTOREREL;
    /* Base address of the RuntimeArgumentHandle: */
    memcpy(&(inst->param_1), &(stack->stack[stack->top - 1]), sizeof(ir_item_t));
    /* Offset of the "value" field: */
    (inst->param_2).value.v = (JITINT32) ((cliManager->CLR).runtimeHandleManager.getRuntimeArgumentHandleValueOffset(&((cliManager->CLR).runtimeHandleManager)));
    (inst->param_2).type = IRINT32;
    (inst->param_2).internal_type = IRINT32;
    /* Value to store to init the RuntimeArgumentHandle (it is held in the last parameter): */
    (inst->param_3).value.v = method->getSignature(method)->parametersNumber - 1;
    (inst->param_3).type = IROFFSET;
    (inst->param_3).internal_type = IROBJECT;
//      make_ir_infos(&(inst->param_3), &(stack->stack[(stack->top) -1 - (param_size - vararg_param_index)]));

    (stack->top)--;

    /* Return                       */
    return res;
}

static inline JITINT32 internal_isStackCorrect (CLIManager_t *cliManager, Method method, t_stack *stack) {
    JITINT32 start;
    JITINT32 toType;
    JITINT32 count;

    /* Assertions			*/
    assert(method != NULL);

    /* Check the top		*/
    if (stack == NULL) {
        return JITFALSE;
    }
    start = method->getParametersNumber(method) + method->getLocalsNumber(method);
    if (stack->top < start) {
        print_err("CIL->IR: ERROR = Parameters and local variables have been deleted from the stack. ", 0);
        return JITFALSE;
    }

    /* Check the parameters and     *
     * local variables		*/
    for (count = 0; count < start; count++) {
        if ((stack->stack[count]).type != IROFFSET) {
            print_err("CIL->IR: ERROR = Parameters and local variables were not variables. ", 0);
            return JITFALSE;
        }
        if ((stack->stack[count]).value.v != count) {
            print_err("CIL->IR: ERROR = The ID of the parameters and local variables were wrong. ", 0);
            return JITFALSE;
        }
    }

    /* Check the stack		*/
    for (count = start; count < stack->top; count++) {
        toType = (cliManager->metadataManager->generic_type_rules)->coerce_CLI_Type(stack->stack[count].internal_type);
        if ((stack->stack[count]).internal_type != toType) {
            fprintf(stderr, "CIL->IR: ERROR = The type %u of a temporary is not stackable.\n", stack->stack[count].internal_type);
            return JITFALSE;
        }
    }

    /* Return			*/
    return JITTRUE;
}

static inline void initialize_local_variables (CLIManager_t *cliManager, Method method, t_stack *stack) {
    JITUINT32 count;

    /* Assertions				*/
    assert(cliManager != NULL);
    assert(method != NULL);
    assert(stack != NULL);

    for (count = method->getParametersNumber(method); count < (method->getLocalsNumber(method) + method->getParametersNumber(method)); count++) {
        ir_instruction_t        *instruction;
        if ((stack->stack[count]).internal_type != IRVALUETYPE) {
#ifdef DEBUG
            char buf[DIM_BUF];
#endif
            instruction = method->newIRInstr(method);
            instruction->type = IRMOVE;
            instruction->byte_offset = 0;
            memcpy(&(instruction->result), &(stack->stack[count]), sizeof(ir_item_t));
            memcpy(&(instruction->param_1), &(stack->stack[count]), sizeof(ir_item_t));
            (instruction->param_1).value.v = 0;
            (instruction->param_1).type = (stack->stack[count]).internal_type;
            (instruction->param_1).internal_type = (stack->stack[count]).internal_type;
            assert((instruction->result).internal_type == (instruction->param_1).internal_type);
#ifdef DEBUG
            switch ((stack->stack[count]).internal_type) {
                case IRNINT:
                case IRINT64:
                case IRINT32:
                case IRINT16:
                case IRINT8:
                case IRNUINT:
                case IRUINT64:
                case IRUINT32:
                case IRUINT16:
                case IRUINT8:
                case IRNFLOAT:
                case IRFLOAT64:
                case IRFLOAT32:
                case IRMPOINTER:
                case IRTPOINTER:
                case IROBJECT:
                    break;
                default:
                    snprintf(buf, sizeof(char) * DIM_BUF, "CIL IR: ERROR = Type %d of the variable %d of the method %s is not known. ", (stack->stack[count]).internal_type, count - method->getParametersNumber(method), method->getName(method));
                    print_err(buf, 0);
                    abort();
            }
#endif

        } else {
            ir_instruction_t        *getAddr;

            /* Get the address			*/
            getAddr = method->newIRInstr(method);
            getAddr->type = IRGETADDRESS;
            memcpy(&(getAddr->param_1), &(stack->stack[count]), sizeof(ir_item_t));
            (getAddr->result).value.v = method->incMaxVariablesReturnOld(method);
            (getAddr->result).type = IROFFSET;
            (getAddr->result).internal_type = IRMPOINTER;
            if ((getAddr->param_1).type_infos != NULL) {
                (getAddr->param_2).type_infos = (getAddr->param_1).type_infos->makePointerDescriptor((getAddr->param_1).type_infos);
            }

            /* Init the memory			*/
            instruction = method->newIRInstr(method);
            instruction->type = IRINITMEMORY;
            (instruction->param_3).value.v = (cliManager->layoutManager).layoutType(&(cliManager->layoutManager), (stack->stack[count]).type_infos)->typeSize;
            (instruction->param_3).type = IRUINT32;
            (instruction->param_3).internal_type = IRUINT32;
            (instruction->param_2).value.v = 0;
            (instruction->param_2).type = IRINT32;
            (instruction->param_2).internal_type = IRINT32;
            memcpy(&(instruction->param_1), &(getAddr->result), sizeof(ir_item_t));
        }
    }

    /* Return				*/
    return;
}

/* NOTE: this implementation does not consider the case of a Nullable<T> type */
static inline JITINT16 make_isSubType (t_system *system, Method method, JITUINT32 *current_label_ID, JITUINT32 bytes_read, t_stack *stack, TypeDescriptor *type_of_the_token) {
    ir_instruction_t        *instruction;
    ir_instruction_t	*loadInstruction;
    ir_symbol_t                     *typeIndex;
    JITINT32 typeDescriptorOffset;
    JITUINT32 keyEqID;
    JITUINT32 keyID;
    JITUINT32 TypePointerVarID;
    JITUINT32 TypeCompPointerVarID;
    JITUINT32 bucketVarID;
    JITUINT32 valueVarID;

    JITUINT32 hashLoop_label;
    JITUINT32 foundKey_label;
    JITUINT32 endTest_label;

    ir_item_t               *items;
    XanList                 *icall_params;

    /* assertions */
    assert(current_label_ID != NULL);
    assert(stack != NULL);
    assert(system != NULL);
    assert(method != NULL);

    /* initialize the label elements */
    hashLoop_label = (*current_label_ID)++;
    foundKey_label = (*current_label_ID)++;
    endTest_label = (*current_label_ID)++;

    /* The castclass instruction attempts to cast obj (of type IROBJECT) to the class */
    assert((stack->stack[(stack->top) - 1]).internal_type == IROBJECT);

    typeIndex = (system->cliManager).translator.getIndexOfTypeDescriptorSymbol(&(system->cliManager), type_of_the_token);
    typeDescriptorOffset = (system->garbage_collectors).gc.fetchTypeDescriptorOffset();
    valueVarID = method->incMaxVariablesReturnOld(method);

    /* Here navigate into hashtable */

    /* Make the IR instruction to	*
     * load the pointer of the      *
     * TypeDescriptor		*/
    TypePointerVarID = method->incMaxVariablesReturnOld(method);
    instruction = method->newIRInstr(method);
    instruction->type = IRLOADREL;
    memcpy(&(instruction->param_1), &(stack->stack[(stack->top) - 1]), sizeof(ir_item_t));
    (instruction->param_2).value.v = (JITINT32) typeDescriptorOffset;
    (instruction->param_2).type = IRINT32;
    (instruction->param_2).internal_type = IRINT32;
    (instruction->result).value.v = TypePointerVarID;
    (instruction->result).type = IROFFSET;
    (instruction->result).internal_type = IRNINT;
    instruction->byte_offset = bytes_read;

    TypeCompPointerVarID = method->incMaxVariablesReturnOld(method);
    instruction = method->newIRInstr(method);
    instruction->type = IRLOADREL;
    (instruction->param_1).value.v = TypePointerVarID;
    (instruction->param_1).type = IROFFSET;
    (instruction->param_1).internal_type = IRNINT;
    (instruction->param_2).value.v = 0;
    (instruction->param_2).type = IRINT32;
    (instruction->param_2).internal_type = IRINT32;
    (instruction->result).value.v = TypeCompPointerVarID;
    (instruction->result).type = IROFFSET;
    (instruction->result).internal_type = IRNINT;
    instruction->byte_offset = bytes_read;

    /* Make the IR instruction to	*
     * load the pointer of the      *
     * TypeDescriptor compatibility table bucket		*/
    bucketVarID = method->incMaxVariablesReturnOld(method);
    instruction = method->newIRInstr(method);
    instruction->type = IRADD;
    (instruction->param_1).value.v = TypeCompPointerVarID;
    (instruction->param_1).type = IROFFSET;
    (instruction->param_1).internal_type = IRNINT;
    (instruction->param_2).value.v = (JITNUINT) typeIndex;
    (instruction->param_2).type = IRSYMBOL;
    (instruction->param_2).internal_type = IRNINT;
    (instruction->result).value.v = bucketVarID;
    (instruction->result).type = IROFFSET;
    (instruction->result).internal_type = IRNINT;
    instruction->byte_offset = bytes_read;
    assert((instruction->param_1).internal_type == (instruction->param_2).internal_type);

    /* Test for correct Bucket */
    instruction = method->newIRInstr(method);
    instruction->type = IRLABEL;
    instruction->byte_offset = bytes_read;
    (instruction->param_1).value.v = hashLoop_label;
    (instruction->param_1).type = IRLABELITEM;
    (instruction->param_1).internal_type = IRLABELITEM;
    instruction->byte_offset = bytes_read;

    keyID = method->incMaxVariablesReturnOld(method);
    loadInstruction = method->newIRInstr(method);
    loadInstruction->type = IRLOADREL;
    (loadInstruction->param_1).value.v = bucketVarID;
    (loadInstruction->param_1).type = IROFFSET;
    (loadInstruction->param_1).internal_type = IRNINT;
    (loadInstruction->param_2).value.v = 0;
    (loadInstruction->param_2).type = IRINT32;
    (loadInstruction->param_2).internal_type = IRINT32;
    (loadInstruction->result).value.v = keyID;
    (loadInstruction->result).type = IROFFSET;
    (loadInstruction->result).internal_type = IRNINT;
    loadInstruction->byte_offset = bytes_read;

    keyEqID = method->incMaxVariablesReturnOld(method);
    instruction = method->newIRInstr(method);
    instruction->type = IREQ;
    memcpy(&(instruction->param_1), &(loadInstruction->result), sizeof(ir_item_t));
    (instruction->param_2).value.v = (JITNINT) type_of_the_token;
    (instruction->param_2).type = (instruction->param_1).internal_type;
    (instruction->param_2).internal_type = (instruction->param_2).type;
    (instruction->result).value.v = keyEqID;
    (instruction->result).type = IROFFSET;
    (instruction->result).internal_type = IRINT32;
    instruction->byte_offset = bytes_read;
    assert((instruction->param_1).internal_type == (instruction->param_2).internal_type);

    instruction = method->newIRInstr(method);
    instruction->type = IRBRANCHIF;
    instruction->byte_offset = bytes_read;
    (instruction->param_1).value.v = keyEqID;
    (instruction->param_1).type = IROFFSET;
    (instruction->param_1).internal_type = IRINT32;
    (instruction->param_2).value.v = foundKey_label;
    (instruction->param_2).type = IRLABELITEM;
    (instruction->param_2).internal_type = IRLABELITEM;
    instruction->byte_offset = bytes_read;

    /* load pointer of next imt slot in collision list */
    instruction = method->newIRInstr(method);
    instruction->type = IRLOADREL;
    (instruction->param_1).value.v = bucketVarID;
    (instruction->param_1).type = IROFFSET;
    (instruction->param_1).internal_type = IRNINT;
    (instruction->param_2).value.v = offsetof(TypeElem, next);
    (instruction->param_2).type = IRINT32;
    (instruction->param_2).internal_type = IRINT32;
    (instruction->result).value.v = bucketVarID;
    (instruction->result).type = IROFFSET;
    (instruction->result).internal_type = IRNINT;
    instruction->byte_offset = bytes_read;

    keyEqID = method->incMaxVariablesReturnOld(method);
    instruction = method->newIRInstr(method);
    instruction->type = IREQ;
    (instruction->param_1).value.v = bucketVarID;
    (instruction->param_1).type = IROFFSET;
    (instruction->param_1).internal_type = IRNINT;
    (instruction->param_2).value.v = (JITNINT) NULL;
    (instruction->param_2).type = IRNINT;
    (instruction->param_2).internal_type = IRNINT;
    (instruction->result).value.v = keyEqID;
    (instruction->result).type = IROFFSET;
    (instruction->result).internal_type = IRINT32;
    instruction->byte_offset = bytes_read;
    assert((instruction->param_1).internal_type == (instruction->param_2).internal_type);

    instruction = method->newIRInstr(method);
    instruction->type = IRBRANCHIFNOT;
    instruction->byte_offset = bytes_read;
    (instruction->param_1).value.v = keyEqID;
    (instruction->param_1).type = IROFFSET;
    (instruction->param_1).internal_type = IRINT32;
    (instruction->param_2).value.v = hashLoop_label;
    (instruction->param_2).type = IRLABELITEM;
    (instruction->param_2).internal_type = IRLABELITEM;
    instruction->byte_offset = bytes_read;

    /* not found imt slot in collision list call trampoline to init */

    icall_params = xanList_new(sharedAllocFunction, freeFunction, NULL);
    assert(icall_params != NULL);
    items = (ir_item_t *) sharedAllocFunction(sizeof(ir_item_t));
    memcpy(items, &(stack->stack[(stack->top) - 1]), sizeof(ir_item_t));
    xanList_append(icall_params, items);
    items = (ir_item_t *) sharedAllocFunction(sizeof(ir_item_t));
    (items->value).v = (JITNUINT) type_of_the_token;
    items->type = IRCLASSID;
    items->internal_type = IRCLASSID;
    xanList_append(icall_params, items);
    translate_ncall(method, (JITINT8 *) "runtime_canCast", IRINT32, icall_params, bytes_read, valueVarID, NULL);
    icall_params = NULL;

    instruction = method->newIRInstr(method);
    instruction->type = IRBRANCH;
    instruction->byte_offset = bytes_read;
    (instruction->param_1).value.v = endTest_label;
    (instruction->param_1).type = IRLABELITEM;
    (instruction->param_1).internal_type = IRLABELITEM;
    instruction->byte_offset = bytes_read;

    /* found type in load descriptor pointer */
    instruction = method->newIRInstr(method);
    instruction->type = IRLABEL;
    instruction->byte_offset = bytes_read;
    (instruction->param_1).value.v = foundKey_label;
    (instruction->param_1).type = IRLABELITEM;
    (instruction->param_1).internal_type = IRLABELITEM;
    instruction->byte_offset = bytes_read;

    instruction = method->newIRInstr(method);
    instruction->type = IRLOADREL;
    (instruction->param_1).value.v = bucketVarID;
    (instruction->param_1).type = IROFFSET;
    (instruction->param_1).internal_type = IRNINT;
    (instruction->param_2).value.v = offsetof(TypeElem, value);
    (instruction->param_2).type = IRINT32;
    (instruction->param_2).internal_type = IRINT32;
    (instruction->result).value.v = valueVarID;
    (instruction->result).type = IROFFSET;
    (instruction->result).internal_type = IRINT32;
    instruction->byte_offset = bytes_read;

    /* pointer loaded in methodID var */
    instruction = method->newIRInstr(method);
    instruction->type = IRLABEL;
    instruction->byte_offset = bytes_read;
    (instruction->param_1).value.v = endTest_label;
    (instruction->param_1).type = IRLABELITEM;
    (instruction->param_1).internal_type = IRLABELITEM;
    instruction->byte_offset = bytes_read;

    /* load on the stack the new element */
    stack->cleanTop(stack);
    (stack->stack[stack->top]).value.v = valueVarID;
    (stack->stack[stack->top]).type = IROFFSET;
    (stack->stack[stack->top]).internal_type = IRINT32;
    (stack->top)++;

    /* Return                       */
    return 0;
}

static inline JITINT16 translate_cil_endfinally (Method method, JITUINT32 bytes_read, XanStack *finallyLabels) {
    ir_instruction_t        *instruction;

    /* Assertions				*/
    assert(method != NULL);
    assert(finallyLabels != NULL);

    /* Append the endfinally instruction to	*
     * the method				*/
    instruction = method->newIRInstr(method);
    instruction->type = IRENDFINALLY;
    instruction->byte_offset = bytes_read;
    (instruction->param_1).value.v = ((IR_ITEM_VALUE) (JITNUINT) xanStack_top(finallyLabels)) - 1;
    (instruction->param_1).type = IRLABELITEM;
    (instruction->param_1).internal_type = IRLABELITEM;

    /* Return				*/
    return 0;
}

static inline t_stack * internal_insertLabels (Method method, XanList *labels, JITBOOLEAN *is_invalid_basic_block, JITUINT32 bytes_read, t_stack *stack, CILStack cilStack, XanStack *finallyLabels, IR_ITEM_VALUE *exceptionVarID) {
    t_label                 *label;
    ir_instruction_t        *instruction;
    ir_instruction_t        *labelInstruction;
    ir_method_t             *irMethod;
    XanListItem             *current_block;
    t_try_block             *current_try_block;

    /* Assertions				*/
    assert(method != NULL);
    assert(labels != NULL);
    assert(is_invalid_basic_block != NULL);
    assert(stack != NULL);
    assert(cilStack != NULL);
    assert(finallyLabels != NULL);

    /* Check if there are labels to insert	*/
    if (xanList_length(labels) == 0) {
        return stack;
    }

    /* Check if at the current position     *
     * there is a label to insert		*/
    label = fetch_label(labels, method->getInstructionsNumber(method));
    if (label == NULL) {
        PDEBUG("NO LABEL FOUND AT offset-CIL %d\n", bytes_read);
        return stack;
    }

    /* Check if we are inside an invalid	*
     * block				*/
    if (    (*is_invalid_basic_block)                               &&
            (label->type == EXCEPTION_HANDLING_LABEL_TYPE)          &&
            (label->ending_handler == NULL)                         &&
            (label->ending_block == NULL)                           ) {
        invalidate_blocks_starting_at_label(labels, label);
        return stack;
    }

    /* Fetch the IR method			*/
    irMethod = method->getIRMethod(method);
    assert(irMethod != NULL);

    /* Check the label type			*/
    if (label->type != EXCEPTION_HANDLING_LABEL_TYPE) {
        ir_instruction_t        *labelInstruction;

        /*	CASE : DEFAULT_LABEL_TYPE	*/
        /*	THIS LABEL IS NOT USED FOR THE EXCEPTION HANDLING MECHANISM.
         *	WE EXPECT THAT THE CORRECT VALUE OF stack IS STORED INTO
         *	THE label->stack FIELD. */
        PDEBUG("CILIR		CASE : DEFAULT_LABEL_TYPE \n");
        PDEBUG("CILIR:		Insert the label \"L%d\"\n", label->ID);
        instruction = method->newIRInstr(method);
        instruction->type = IRLABEL;
        (instruction->param_1).value.v = label->ID;
        (instruction->param_1).type = IRLABELITEM;
        (instruction->param_1).internal_type = IRLABELITEM;
        instruction->byte_offset = bytes_read;
        label->ir_position = method->getInstructionsNumber(method) - 1;
        labelInstruction = instruction;

        /* Print the stack			*/
#ifdef PRINTDEBUG
        PDEBUG("CILIR		New stack: \n");
        print_stack(label->stack, method->getParametersNumber(method), method->getLocalsNumber(method));
        assert(label->stack);
#endif

        /* Merge the stack			*/
        PDEBUG("CILIR		UPDATING THE STACK... \n");
        stack = cilStack->mergeStackes(cilStack, stack, label->stack, irMethod, labelInstruction);
        assert(stack != NULL);

        /* the block is no longer invalid ...   */
        (*is_invalid_basic_block) = 0;

        /* Return				*/
        return stack;
    }

    /* The label is for exception handling	*/
    PDEBUG("CILIR:		Insert the label \"L%d\"\n", label->ID);
    labelInstruction = NULL;
    instruction = method->newIRInstr(method);
    instruction->type = IRLABEL;
    (instruction->param_1).value.v = label->ID;
    (instruction->param_1).type = IRLABELITEM;
    (instruction->param_1).internal_type = IRLABELITEM;
    instruction->byte_offset = bytes_read;
    label->ir_position = method->getInstructionsNumber(method) - 1;
    labelInstruction = instruction;

    PDEBUG("CILIR:		We found the EXCEPTION_HANDLING label \"L%d\"\n", label->ID);
    assert(label->ending_block != NULL || label->ending_handler != NULL || label->starting_handler != NULL || xanList_length(label->starting_blocks) > 0);
    if (label->ending_handler != NULL) {
#ifdef DEBUG
        PDEBUG("CILIR: case_ending_handler \n");
        assert((label->ending_handler)->owner_block != NULL);
        if (label->starting_handler != NULL) {
            assert((label->starting_handler)->owner_block != NULL);
            assert((label->starting_handler)->owner_block == (label->starting_handler)->owner_block );
        }
        assert( ( (label->ending_handler)->owner_block)->stack_to_restore != NULL);
#endif
#ifdef PRINTDEBUG
        PDEBUG("CILIR		RESTORING THE STACK ...\n");
        PDEBUG("CILIR:		New stack [FROM THE BLOCK[\"L%d\",\"L%d\"]]\n", ((label->ending_handler)->owner_block) ->try_start_label_ID, ((label->ending_handler)->owner_block) ->try_end_label_ID);
        print_stack(((label->ending_handler)->owner_block)->stack_to_restore, method->getParametersNumber(method), method->getLocalsNumber(method));
#endif
        stack = ((label->ending_handler)->owner_block)->stack_to_restore;

        /* the block is no longer invalid ... */
        (*is_invalid_basic_block) = 0;
        PDEBUG("CILIR:		STACK RESTORED!\n");

        if ((label->ending_handler)->type != EXCEPTION_TYPE) {
            PDEBUG("CILIR:		ENCOUNTERED THE END OF A NOT EXCEPTION_TYPE HANDLER. IT'S NECESSARY TO UPDATE THE DATAFLOW INFORMATIONS...\n");
            JITUINT32 endFinally_Or_Filter_Posn;
            ir_instruction_t        *endFinally_Or_Filter_Inst;

            /* Check if we are closing a finally block	*/
            if ((label->ending_handler)->type == FINALLY_TYPE) {

                /* Pop the finally label from the finallyLabels stack	*/
                xanStack_pop(finallyLabels);
            }

            endFinally_Or_Filter_Posn = method->getInstructionsNumber(method) - 2;
            PDEBUG("endFinally_Or_Filter_Posn = %d \n", endFinally_Or_Filter_Posn);
            IRMETHOD_lock(irMethod);
            endFinally_Or_Filter_Inst = IRMETHOD_getInstructionAtPosition(irMethod, endFinally_Or_Filter_Posn);
            IRMETHOD_unlock(irMethod);

            PDEBUG("CILIR:	LAST INSTRUCTION OF THE HANDLER WAS A %d \n", (endFinally_Or_Filter_Inst->type));
            assert( endFinally_Or_Filter_Inst->type == IRENDFILTER ||       endFinally_Or_Filter_Inst->type == IRENDFINALLY);

            (label->ending_handler)->end_filter_or_finally_inst = endFinally_Or_Filter_Inst;
        }
    }
    /* WE HAVE TO VERIFY IF IT'S NECESSARY TO RESTORE AN OLD STACK
     * IMAGE. */
    if (label->ending_block != NULL) {
#ifdef DEBUG
        PDEBUG("CILIR: case_ending_block \n");
        if (label->starting_handler != NULL) {
            assert((label->starting_handler)->owner_block
                   != NULL );
            PDEBUG(" %p %p \n"
                   , (label->starting_handler)->owner_block
                   , label->ending_block);
            assert((label->starting_handler)->owner_block
                   == label->ending_block );
        }
        assert((label->ending_block)->stack_to_restore != NULL);
#endif
#ifdef PRINTDEBUG
        PDEBUG("CILIR		RESTORING THE STACK ...\n");
        PDEBUG("CILIR:		New stack [FROM THE" "BLOCK[\"L%d\",\"L%d\"]]\n" , (label->ending_block)->try_start_label_ID , (label->ending_block)->try_end_label_ID);
        print_stack((label->ending_block)->stack_to_restore , method->getParametersNumber(method) , method->getLocalsNumber(method));
#endif
        stack = (label->ending_block)->stack_to_restore;
        /* the block is no longer invalid ... */
        (*is_invalid_basic_block) = 0;
        PDEBUG("CILIR		STACK RESTORED!\n");
    }
    if (label->starting_handler != NULL) {
#ifdef DEBUG
        PDEBUG("CILIR: case_starting_handler \n");
        assert((label->starting_handler)->owner_block != NULL);
        if (label->ending_block != NULL) {
            PDEBUG("%p %p \n", label->ending_block
                   , (label->starting_handler)
                   ->owner_block);
            assert(label->ending_block
                   == (label->starting_handler)
                   ->owner_block );
        }
        PDEBUG("CILIR:		New stack [FROM THE"
               "BLOCK[\"L%d\",\"L%d\"]]\n"
               , ((label->starting_handler)
                  ->owner_block)->try_start_label_ID
               , ((label->starting_handler)
                  ->owner_block)->try_end_label_ID);
        PDEBUG("CILIR: owner_block == %p \n"
               , (label->starting_handler)
               ->owner_block);
        assert( ( (label->starting_handler)
                  ->owner_block)
                ->stack_to_restore != NULL);

        PDEBUG("CILIR		RESTORING THE STACK ...\n");
#endif

        /* FETCH THE CORRECT STACK FROM THE PARENT PROTECTED BLOCK */
        stack = ((label->starting_handler)->owner_block)->stack_to_restore;
        /* the block is no longer invalid ... */
        (*is_invalid_basic_block) = 0;

        if ((label->starting_handler)->type != FINALLY_TYPE) {
            ir_method_t		*irMethod;
            ir_instruction_t	*catchInst;

            /* Clean the top of the stack           */
            stack->cleanTop(stack);

            /* Fetch the IR method			*/
            irMethod	= method->getIRMethod(method);
            assert(irMethod != NULL);

            /* An exception object describing the
             * specific exceptional behaviour detected
             * is pushed onto the evaluation stack as the first
             * item upon entry of a filter or catch clause.
             * Fetch the catcher instruction		*/
            catchInst	= IRMETHOD_getCatcherInstruction(irMethod);

            /* Push the thrown object on the stack	*/
            if (catchInst == NULL) {
                if ((*exceptionVarID) == -1) {
                    (*exceptionVarID)			= method->incMaxVariablesReturnOld(method);
                }
                assert((*exceptionVarID) != -1);
                stack->stack[stack->top].value.v 	= (*exceptionVarID);
                stack->stack[stack->top].type 		= IROFFSET;
                stack->stack[stack->top].internal_type 	= IROBJECT;
                stack->stack[(stack->top)].type_infos 	= NULL;

            } else {

                memcpy(&(stack->stack[stack->top]), &(catchInst->result), sizeof(ir_item_t));
            }
            (stack->top)++;
        }

        /* Print the stack		*/
#ifdef PRINTDEBUG
        PDEBUG("CILIR:	New stack [FROM BLOCK[\"L%d\",\"L%d\"]]\n", ((label->starting_handler)->owner_block) ->try_start_label_ID, ((label->starting_handler)->owner_block) ->try_end_label_ID);
        print_stack(((label->starting_handler) ->owner_block)->stack_to_restore, method->getParametersNumber(method), method->getLocalsNumber(method));
        PDEBUG("CILIR		STACK RESTORED!\n");
#endif

        if ((label->starting_handler)->type == FILTER_TYPE) {

            /* Clean the top of the stack           */
            stack->cleanTop(stack);

            /* ADD A START-FILTER INSTRUCTION */
            PDEBUG("CILIR: ADD_LABEL_INS : START_FILTER_TYPE\n");
            IRMETHOD_eraseInstructionFields(irMethod, labelInstruction);
            assert(IRMETHOD_getLabelInstruction(irMethod, label->ID) == NULL);
            instruction = labelInstruction;
            instruction->type = IRSTARTFILTER;
            (instruction->param_1).value.v = label->ID;
            (instruction->param_1).type = IRLABELITEM;
            (instruction->param_1).internal_type = IRLABELITEM;
            stack->stack[stack->top].value.v = method->incMaxVariablesReturnOld(method);
            stack->stack[stack->top].type = IROFFSET;
            stack->stack[stack->top].internal_type = IROBJECT;
            memcpy(&(instruction->result), &(stack->stack[stack->top]), sizeof(ir_item_t));
            (stack->top)++;
            instruction->byte_offset = bytes_read;

        } else if (     ((label->starting_handler)->type == FINALLY_TYPE)       ||
                        ((label->starting_handler)->type == FAULT_TYPE)         ) {

            /* ADD A START-FINALLY INSTRUCTION */
            PDEBUG("CILIR: ADD_LABEL_INS : START_FINALLY_TYPE (L%d)\n", label->ID);
            IRMETHOD_eraseInstructionFields(irMethod, labelInstruction);
            assert(IRMETHOD_getLabelInstruction(irMethod, label->ID) == NULL);
            instruction = labelInstruction;
            instruction->type = IRSTARTFINALLY;
            (instruction->param_1).value.v = label->ID;
            (instruction->param_1).type = IRLABELITEM;
            (instruction->param_1).internal_type = IRLABELITEM;
            instruction->byte_offset = bytes_read;
            xanStack_push(finallyLabels, (void *) (JITNUINT) ((label->ID) + 1));
        }
    }

    /* ONCE RESTORED THE CORRECT IMAGE, WE MUST VERIFY IF THERE ARE
     * PROTECTED BLOCKS THAT START AT THIS CIL-ADDRESS */
    PDEBUG("CILIR: case_starting_block \n");

    /* initialize the current element */
    current_block = xanList_first(label->starting_blocks);
    while (current_block != NULL) {

        /* update the stack element for the current_block */
        current_try_block = (t_try_block *) current_block->data;
        assert(current_try_block->stack_to_restore == NULL);

        /* Print the stack				*/
#ifdef PRINTDEBUG
        PDEBUG("CILIR:	copying the current stack into the block structure\n");
        print_stack(stack, method->getParametersNumber(method), method->getLocalsNumber(method));
#endif

        /* Clone the stack				*/
        current_try_block->stack_to_restore = cilStack->cloneStack(cilStack, stack);

        /* Fetch the next element from the list		*/
        current_block = current_block->next;
    }

    /* Return				*/
    return stack;
}

static inline ir_item_t * make_new_IR_param () {
    ir_item_t       *param;

    param = (ir_item_t *) sharedAllocFunction(sizeof(ir_item_t));
    assert(param != NULL);
    memset(param, 0, sizeof(ir_item_t));

    return param;
}

static inline void make_ir_infos (ir_item_t * to, ir_item_t * from) {
    to->type_infos = from->type_infos;
}

static inline JITBOOLEAN internal_isValueType (TypeDescriptor *ilType) {
    if (!ilType->isValueType(ilType)) {
        return JITFALSE;
    }
    return JITTRUE;
}
