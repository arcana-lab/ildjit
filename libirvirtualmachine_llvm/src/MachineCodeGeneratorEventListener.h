#ifndef MACHINECODEGENERATOREVENTLISTENER_H
#define MACHINECODEGENERATOREVENTLISTENER_H

#include <stdlib.h>
#include <ir_virtual_machine.h>
#include <llvm/ExecutionEngine/JITEventListener.h>

namespace llvm {

struct MachineCodeGeneratorEventListener : public JITEventListener {
private:
    IRVM_t *irVM;
public:

    MachineCodeGeneratorEventListener (IRVM_t *irVM) : JITEventListener() {
        this->irVM	= irVM;
    }

    virtual void NotifyFunctionEmitted(const Function &F, void *Code, size_t Size, const EmittedFunctionDetails &Details);

    virtual void NotifyFreeingMachineCode(void *OldPtr);
};
}

#endif
