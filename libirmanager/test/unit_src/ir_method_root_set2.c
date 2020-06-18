#include <tests_utils.h>
#include <ir_language.h>
#include <ir_method.h>

int main (int argc, char * argv[]) {
    ir_lib_t ir_lib;
    ir_method_t method;
    JITUINT32 count;
    t_ir_instruction        *firstInstr;
    t_ir_instruction        *secondInstr;

    IRLibNew(&ir_lib, NULL, tests_malloc, tests_realloc, tests_free, NULL);
    ir_method_init(&ir_lib, &method);

    assert(method.getInstructionsNumber(&method) == 0);
    firstInstr = method.newInstruction(&method);
    assert(firstInstr != NULL);
    assert(method.getInstructionsNumber(&method) == 1);
    method.addVariableToRootSet(&method, 5);
    method.addVariableToRootSet(&method, 10);
    method.allocRootSet(&method);
    method.setInstrPar1(firstInstr, 5, 0, IROFFSET, IROBJECT, 0, NULL);
    method.setInstrPar2(firstInstr, 10, 0, IROFFSET, IROBJECT, 0, NULL);
    assert(method.getRootSetTop(&method) == 2);
    for (count = 0; count < method.getRootSetTop(&method); count++) {
        assert( (method.getRootSetSlot(&method, count) == 5) ||
                (method.getRootSetSlot(&method, count) == 10)   );
    }

    method.updateRootSet(&method);
    assert(method.getRootSetTop(&method) == 2);
    for (count = 0; count < method.getRootSetTop(&method); count++) {
        assert( (method.getRootSetSlot(&method, count) == 5) ||
                (method.getRootSetSlot(&method, count) == 10)   );
    }
    secondInstr = method.newInstruction(&method);
    assert(secondInstr != NULL);
    assert(method.getInstructionsNumber(&method) == 2);

    method.updateRootSet(&method);
    assert(method.getRootSetTop(&method) == 2);
    for (count = 0; count < method.getRootSetTop(&method); count++) {
        assert( (method.getRootSetSlot(&method, count) == 5) ||
                (method.getRootSetSlot(&method, count) == 10)   );
    }

    method.setInstrPar1(secondInstr, 7, 0, IROFFSET, IROBJECT, 0, NULL);
    method.addVariableToRootSet(&method, 7);
    method.allocRootSet(&method);
    assert(method.getRootSetTop(&method) == 3);
    for (count = 0; count < method.getRootSetTop(&method); count++) {
        assert( (method.getRootSetSlot(&method, count) == 5) ||
                (method.getRootSetSlot(&method, count) == 10) ||
                (method.getRootSetSlot(&method, count) == 7) );
    }

    assert(method.getRootSetTop(&method) == 3);
    assert(method.getInstructionsNumber(&method) == 2);
    firstInstr = method.getFirstInstruction(&method);
    assert(firstInstr != NULL);
    method.deleteInstruction(&method, firstInstr);
    assert(method.getInstructionsNumber(&method) == 1);
    assert(method.getFirstInstruction(&method) == secondInstr);
    method.allocRootSet(&method);
    method.updateRootSet(&method);
    assert(method.getRootSetTop(&method) <= 2);
    for (count = 0; count < method.getRootSetTop(&method); count++) {
        assert( (method.getRootSetSlot(&method, count) == 5)    ||
                (method.getRootSetSlot(&method, count) == 7)    );
    }

    method.destroy(&method);

    return 0;
}
