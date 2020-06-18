/*
 * Copyright (C) 2011 - 2012 Simone Campanoni, Glenn Holloway
 *
 * ir_virtual_machine.cpp - This is a translator from the IR language into the assembly language.

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
#include <ir_virtual_machine.h>
#include <compiler_memory_manager.h>
#include <iostream>
#include <fstream>
#include <ir_optimizer.h>
#include <ir_optimization_interface.h>
#include <xanlib.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/ADT/Triple.h>
#include <llvm/Target/TargetLibraryInfo.h>
#include <llvm/Target/TargetData.h>
#include <llvm/Transforms/IPO.h>

// My headers
#include <utilities.h>
#include <ir_virtual_machine_backend.h>
#include <ir_virtual_machine_system.h>
#include <config.h>
#include <MachineCodeGeneratorEventListener.h>
// End

void setupCodeOptimizations (IRVM_t *self){
	IRVM_internal_t 		*llvmRoots;
	PassManagerBuilder 		PMBuilder;

	/* Fetch LLVM's top-level handles	*/
	llvmRoots 		= (IRVM_internal_t *) self->data;
	assert(llvmRoots != NULL);

	/* Set the optimization level 		*/
	if ((self->optimizer).enableMachineDependentOptimizations) {
		PMBuilder.OptLevel = 3;
	} else {
		PMBuilder.OptLevel = 0;
	}

	/* Set the library information		*/
	//Triple TargetTriple(llvmRoots->llvmModule->getTargetTriple());
	//PMBuilder.LibraryInfo	= new TargetLibraryInfo(TargetTriple); TODO to understand what this does

	/* Set the method inlining 		*/
	if ((self->optimizer).enableMachineDependentOptimizations) {
		PMBuilder.Inliner = createFunctionInliningPass(275);
	}

	/* Set up the per-function pass manager	*/
	llvmRoots->FPM 		= new FunctionPassManager(llvmRoots->llvmModule);
	llvmRoots->FPM->add(new TargetData(llvmRoots->llvmModule));
	PMBuilder.populateFunctionPassManager(*llvmRoots->FPM);

	/* Set up the per-module pass manager	*/
	llvmRoots->MPM 		= new PassManager();
	llvmRoots->MPM->add(new TargetData(llvmRoots->llvmModule));
	PMBuilder.populateModulePassManager(*llvmRoots->MPM);

	return ;
}
