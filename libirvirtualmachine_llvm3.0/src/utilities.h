/*
 * Copyright (C) 2011  Simone Campanoni, Glenn Holloway
 *
 * ir_virtual_machine.c - This is a translator from the IR language into the assembly language.

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
#ifndef UTILITIES_H
#define UTILITIES_H

// My headers
#include <ir_virtual_machine_backend.h>
// End

/// This is our simplistic type info
struct OurExceptionType_t {
    /// type info type
    int type;
};

struct OurBaseException_t {
    struct OurExceptionType_t type;

    // Note: This is properly aligned in unwind.h
    struct _Unwind_Exception unwindException;

    /* Pointer to the IR object			*/
    void	*thrownObject;
};

typedef struct OurBaseException_t OurException;
typedef struct _Unwind_Exception OurUnwindException;

Type * get_LLVM_type (IRVM_t *self, IRVM_internal_t *vm, JITUINT32 IRType, TypeDescriptor *ilType);
Type * get_LLVM_integer_type_to_use_as_pointer (IRVM_t *self, IRVM_internal_t *llvmRoots);
Value * get_LLVM_value (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_item_t *item, t_llvm_function_internal *llvmFunction);
Value * get_LLVM_value_for_using (IRVM_t *self, ir_method_t *method, ir_item_t *item, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
Value * get_LLVM_value_for_defining (IRVM_t *self, ir_method_t *method, ir_item_t *item, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
Value * get_coerced_byte_count (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_item_t *irValue, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
Value * cast_LLVM_value_between_pointers_and_integers (IRVM_t *self, IRVM_internal_t *llvmRoots, JITUINT32 targetIRType, JITUINT32 sourceIRType, Value *llvmValueToConvert, BasicBlock *currentBB);
Value * convert_LLVM_value (IRVM_t *self, IRVM_internal_t *llvmRoots, JITUINT16 currentIRType, JITUINT16 irTypeToConvert, TypeDescriptor *ilTypeToConvert, Value *llvmValueToConvert, BasicBlock *currentBB);
Function * get_LLVM_function_of_binary_method (IRVM_t *self, JITINT8 *functionName, JITUINT16 irReturnType, XanList *paramTypes);
void allocateBackendFunction (IRVM_t *self, t_jit_function *f, ir_method_t *irMethod);
JITINT8 * get_C_function_name (ir_method_t *m);
void free_memory_used_for_llvm_function (t_llvm_function_internal *llvmFunction);

extern "C" {
    OurUnwindException * createOurException (void *exceptionObject);
}

#endif
