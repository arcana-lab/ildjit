#include <tests_utils.h>
#include <ir_method.h>

int main (int argc, char * argv[]) {
    ir_lib_t ir_lib;
    ir_method_t method;
    t_ir_instruction * firstInstr;
    t_ir_instruction * secondInstr;

    IRLibNew(&ir_lib, NULL, tests_malloc, tests_realloc, tests_free, NULL);
    ir_method_init(&ir_lib, &method);

    assert(method.getNextInstruction(&method, NULL) == NULL);
    assert(method.getFirstInstruction(&method) == NULL);
    assert(method.getLastInstruction(&method) == NULL);

    assert(method.getInstructionsNumber(&method) == 0);
    firstInstr = method.newInstruction(&method);
    assert(firstInstr != NULL);
    assert(method.getInstructionsNumber(&method) == 1);

    assert(method.getPrevInstruction(&method, firstInstr) == NULL);
    assert(method.getInstructionAt(&method, 0) == firstInstr);
    assert(method.getNextInstruction(&method, firstInstr) == NULL);

    assert(method.getNextInstruction(&method, NULL) == firstInstr);
    assert(method.getFirstInstruction(&method) == firstInstr);
    assert(method.getLastInstruction(&method) == firstInstr);
    assert(method.getInstructionID(&method, firstInstr) == 0);

    assert(method.getInstructionsNumber(&method) == 1);
    secondInstr = method.newInstruction(&method);
    assert(secondInstr != NULL);
    assert(method.getInstructionsNumber(&method) == 2);

    assert(method.getNextInstruction(&method, NULL) == firstInstr);
    assert(method.getFirstInstruction(&method) == firstInstr);
    assert(method.getLastInstruction(&method) == secondInstr);

    assert(method.getPrevInstruction(&method, firstInstr) == NULL);
    assert(method.getInstructionAt(&method, 0) == firstInstr);
    assert(method.getNextInstruction(&method, firstInstr) == secondInstr);
    assert(method.getInstructionID(&method, firstInstr) == 0);

    assert(method.getPrevInstruction(&method, secondInstr) == firstInstr);
    assert(method.getInstructionAt(&method, 1) == secondInstr);
    assert(method.getNextInstruction(&method, secondInstr) == NULL);
    assert(method.getInstructionID(&method, firstInstr) == 0);
    assert(method.getInstructionID(&method, secondInstr) == 1);

    assert(method.getFirstInstruction(&method) == firstInstr);
    assert(method.getLastInstruction(&method) == secondInstr);

    assert(method.getInstructionsNumber(&method) == 2);
    method.deleteInstructions(&method);
    assert(method.getInstructionsNumber(&method) == 0);

    assert(method.getFirstInstruction(&method) == NULL);
    assert(method.getLastInstruction(&method) == NULL);

    firstInstr = method.newInstruction(&method);
    assert(firstInstr != NULL);
    secondInstr = method.newInstruction(&method);
    assert(secondInstr != NULL);
    assert(method.getInstructionsNumber(&method) == 2);

    assert(method.getFirstInstruction(&method) == firstInstr);
    assert(method.getLastInstruction(&method) == secondInstr);

    assert(method.getPrevInstruction(&method, firstInstr) == NULL);
    assert(method.getInstructionAt(&method, 0) == firstInstr);
    assert(method.getNextInstruction(&method, firstInstr) == secondInstr);
    assert(method.getInstructionID(&method, firstInstr) == 0);

    assert(method.getPrevInstruction(&method, secondInstr) == firstInstr);
    assert(method.getInstructionAt(&method, 1) == secondInstr);
    assert(method.getNextInstruction(&method, secondInstr) == NULL);
    assert(method.getInstructionID(&method, firstInstr) == 0);
    assert(method.getInstructionID(&method, secondInstr) == 1);

    assert(method.getInstructionsNumber(&method) == 2);
    method.deleteInstruction(&method, firstInstr);

    assert(method.getInstructionsNumber(&method) == 1);

    assert(method.getFirstInstruction(&method) == secondInstr);
    assert(method.getLastInstruction(&method) == secondInstr);

    assert(method.getPrevInstruction(&method, secondInstr) == NULL);
    assert(method.getInstructionAt(&method, 0) == secondInstr);
    assert(method.getNextInstruction(&method, secondInstr) == NULL);
    assert(method.getInstructionID(&method, secondInstr) == 0);

    assert(method.getInstructionsNumber(&method) == 1);
    method.deleteInstruction(&method, secondInstr);
    assert(method.getInstructionsNumber(&method) == 0);

    assert(method.getFirstInstruction(&method) == NULL);
    assert(method.getLastInstruction(&method) == NULL);

    firstInstr = method.newInstruction(&method);
    assert(firstInstr != NULL);
    secondInstr = method.newInstruction(&method);
    assert(secondInstr != NULL);
    assert(method.getInstructionsNumber(&method) == 2);

    assert(method.getFirstInstruction(&method) == firstInstr);
    assert(method.getLastInstruction(&method) == secondInstr);

    assert(method.getPrevInstruction(&method, firstInstr) == NULL);
    assert(method.getInstructionAt(&method, 0) == firstInstr);
    assert(method.getNextInstruction(&method, firstInstr) == secondInstr);
    assert(method.getInstructionID(&method, firstInstr) == 0);

    assert(method.getPrevInstruction(&method, secondInstr) == firstInstr);
    assert(method.getInstructionAt(&method, 1) == secondInstr);
    assert(method.getNextInstruction(&method, secondInstr) == NULL);
    assert(method.getInstructionID(&method, firstInstr) == 0);
    assert(method.getInstructionID(&method, secondInstr) == 1);

    assert(method.getInstructionsNumber(&method) == 2);
    method.deleteInstruction(&method, secondInstr);
    assert(method.getInstructionsNumber(&method) == 1);

    assert(method.getFirstInstruction(&method) == firstInstr);
    assert(method.getLastInstruction(&method) == firstInstr);

    assert(method.getPrevInstruction(&method, firstInstr) == NULL);
    assert(method.getInstructionAt(&method, 0) == firstInstr);
    assert(method.getNextInstruction(&method, firstInstr) == NULL);
    assert(method.getInstructionID(&method, firstInstr) == 0);

    assert(method.getInstructionsNumber(&method) == 1);
    method.deleteInstruction(&method, firstInstr);
    assert(method.getInstructionsNumber(&method) == 0);

    assert(method.getFirstInstruction(&method) == NULL);
    assert(method.getLastInstruction(&method) == NULL);

    assert(method.getInstructionsNumber(&method) == 0);
    firstInstr = method.newInstruction(&method);
    assert(firstInstr != NULL);
    assert(method.getInstructionsNumber(&method) == 1);

    method.destroy(&method);

    assert(method.instructions == NULL);

    return 0;
}
