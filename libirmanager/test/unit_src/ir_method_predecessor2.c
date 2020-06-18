#include <tests_utils.h>

#include <ir_language.h>
#include <ir_method.h>

int main (int argc, char * argv[]) {
    ir_lib_t ir_lib;
    ir_method_t method;
    t_ir_instruction * firstInstr;
    t_ir_instruction * secondInstr;
    t_ir_instruction * thirdInstr;
    t_ir_instruction * tmpInstr;

    IRLibNew(&ir_lib, NULL, tests_malloc, tests_realloc, tests_free, NULL);
    ir_method_init(&ir_lib, &method);

    /* First instruction			*/
    assert(method.getInstructionsNumber(&method) == 0);
    firstInstr = method.newInstruction(&method);
    assert(firstInstr != NULL);
    method.setInstructionType(&method, firstInstr, IRBRANCHIF);
    method.setInstrPar1(firstInstr, 0, 0, IRINT32, IRINT32, 0, NULL);
    method.setInstrPar2(firstInstr, 1, 0, IRLABELITEM, IRLABELITEM, 0, NULL);
    assert(method.getInstructionsNumber(&method) == 1);
    assert(method.getPrevInstruction(&method, firstInstr) == NULL);
    assert(method.getInstructionAt(&method, 0) == firstInstr);
    assert(method.getNextInstruction(&method, firstInstr) == NULL);

    /* Second instruction			*/
    secondInstr = method.newInstruction(&method);
    assert(secondInstr != NULL);
    assert(method.getInstructionsNumber(&method) == 2);
    method.setInstructionType(&method, secondInstr, IRRET);

    /* Third instruction			*/
    thirdInstr = method.newInstruction(&method);
    assert(thirdInstr != NULL);
    assert(method.getInstructionsNumber(&method) == 3);
    method.setInstructionType(&method, thirdInstr, IRLABEL);
    method.setInstrPar1(thirdInstr, 1, 0, IRLABELITEM, IRLABELITEM, 0, NULL);

    /* Check the previous instructions	*/
    assert(method.getPredecessorInstruction(&method, firstInstr, NULL) == NULL);
    tmpInstr = method.getPredecessorInstruction(&method, secondInstr, NULL);
    assert(tmpInstr != NULL);
    assert(tmpInstr == firstInstr);
    assert(method.getPredecessorInstruction(&method, secondInstr, tmpInstr) == NULL);
    tmpInstr = method.getPredecessorInstruction(&method, thirdInstr, NULL);
    assert(tmpInstr != NULL);
    assert(tmpInstr == firstInstr);
    assert(method.getPredecessorInstruction(&method, thirdInstr, tmpInstr) == NULL);

    /* Destroy the method			*/
    method.destroy(&method);

    assert(method.instructions == NULL);

    return 0;
}
