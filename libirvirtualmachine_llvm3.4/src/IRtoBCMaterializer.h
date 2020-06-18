#ifndef IRTOBCMATERIALIZER_H
#define IRTOBCMATERIALIZER_H

#include <stdlib.h>
#include <ir_virtual_machine.h>
#include <llvm/GVMaterializer.h>
#include <llvm/Support/system_error.h>

namespace llvm {
class IRtoBCMaterializer : public GVMaterializer {
	private:
		IRVM_t *irVM;

	public:

	IRtoBCMaterializer (IRVM_t *irVM) : GVMaterializer() {
		this->irVM	= irVM;
	}

	/// isMaterializable - True if GV can be materialized from whatever backing
	/// store this GVMaterializer uses and has not been materialized yet.
  	virtual bool isMaterializable(const GlobalValue *GV) const ;

	/// isDematerializable - True if GV has been materialized and can be
	/// dematerialized back to whatever backing store this GVMaterializer uses.
  	virtual bool isDematerializable(const GlobalValue *GV) const {
		return true;
	}

	/// Materialize - make sure the given GlobalValue is fully read.  On success,
	/// return the success code. Otherwise, return a code describing the error.
	///
	virtual error_code Materialize(GlobalValue *GV);

	/// Dematerialize - If the given GlobalValue is read in, and if the
	/// GVMaterializer supports it, release the memory for the GV, and set it up
	/// to be materialized lazily.  If the Materializer doesn't support this
	/// capability, this method is a noop.
	///
	virtual void Dematerialize(GlobalValue *){
		return ;
	}

	/// MaterializeModule - make sure the entire Module has been completely read.
	/// On success, return the success code; otherwise, a description of the error.
	///
	virtual error_code MaterializeModule(Module *M) {
		abort();
	}
};
}

#endif
