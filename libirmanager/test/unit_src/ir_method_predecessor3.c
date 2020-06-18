#include <tests_utils.h>

#include <ir_language.h>
#include <ir_method.h>

int main (int argc, char * argv[]) {
    ir_lib_t ir_lib;
    ir_method_t method;
    t_ir_instruction * firstInstr;
    t_ir_instruction * secondInstr;

    IRLibNew(&ir_lib, NULL, tests_malloc, tests_realloc, tests_free, NULL);
    ir_method_init(&ir_lib, &method);

    /* First instruction			*/
    assert(method.getInstructionsNumber(&method) == 0);
    firstInstr = method.newInstruction(&method);
    assert(firstInstr != NULL);
    method.setInstructionType(&method, firstInstr, IRRET);
    method.setInstrPar1(firstInstr, 0, 0, IRINT32, IRINT32, 0, NULL);
    assert(method.getInstructionsNumber(&method) == 1);
    assert(method.getPrevInstruction(&method, firstInstr) == NULL);
    assert(method.getInstructionAt(&method, 0) == firstInstr);
    assert(method.getNextInstruction(&method, firstInstr) == NULL);

    /* Second instruction			*/
    secondInstr = method.newInstruction(&method);
    assert(secondInstr != NULL);
    assert(method.getInstructionsNumber(&method) == 2);
    method.setInstructionType(&method, secondInstr, IRADD);

    /* Check the previous instructions	*/
    assert(method.getPredecessorInstruction(&method, firstInstr, NULL) == NULL);
    assert(method.getPredecessorInstruction(&method, secondInstr, NULL) == NULL);

    /* Destroy the method			*/
    method.destroy(&method);

    assert(method.instructions == NULL);

    return 0;
}
