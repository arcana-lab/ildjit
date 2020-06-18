#include <tests_utils.h>
#include <ir_method.h>

int main (int argc, char * argv[]) {
    ir_lib_t ir_lib;
    ir_method_t method;
    t_ir_instruction * firstInstr;
    t_ir_instruction * secondInstr;

    IRLibNew(&ir_lib, NULL, tests_malloc, tests_realloc, tests_free, NULL);
    ir_method_init(&ir_lib, &method);

    assert(method.getInstructionsNumber(&method) == 0);
    firstInstr = method.newInstruction(&method);
    assert(firstInstr != NULL);
    method.setInstructionType(&method, firstInstr, IRADD);
    method.setInstrPar1(firstInstr, 2, 0, IROFFSET, IRINT32, 0, NULL);
    method.setInstrPar2(firstInstr, 5, 0, IROFFSET, IRINT32, 0, NULL);
    method.updateRootSet(&method);
    assert(method.getMaxVariables(&method) == 6);

    secondInstr = method.newInstruction(&method);
    assert(secondInstr != NULL);
    method.setInstructionType(&method, secondInstr, IRADD);
    method.setInstrPar1(secondInstr, 20, 0, IROFFSET, IRINT32, 0, NULL);
    method.setInstrPar2(secondInstr, 40, 0, IROFFSET, IRINT32, 0, NULL);
    assert(method.getMaxVariables(&method) == 6);
    method.updateRootSet(&method);
    assert(method.getMaxVariables(&method) == 41);

    method.destroy(&method);
    assert(method.instructions == NULL);

    return 0;
}
