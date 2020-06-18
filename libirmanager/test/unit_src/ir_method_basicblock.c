#include <tests_utils.h>
#include <ir_method.h>

int main (int argc, char * argv[]) {
    ir_lib_t ir_lib;
    ir_method_t method;
    BasicBlock * newBlock;

    IRLibNew(&ir_lib, NULL, tests_malloc, tests_realloc, tests_free, NULL);
    ir_method_init(&ir_lib, &method);

    assert(method.getBasicBlock(&method, -1) == NULL);
    assert(method.getBasicBlock(&method, 0) == NULL);
    assert(method.getBasicBlock(&method, 1) == NULL);
    assert(method.getBasicBlock(&method, 2) == NULL);

    assert(method.getBasicBlocksNumber(&method) == 0);
    newBlock = method.newBasicBlock(&method);
    assert(newBlock != NULL);
    newBlock->startInst = 42;
    assert(method.getBasicBlocksNumber(&method) == 1);

    assert(method.getBasicBlock(&method, -1) == NULL);
    assert(method.getBasicBlock(&method, 0) == newBlock);
    assert((method.getBasicBlock(&method, 0))->startInst == 42);
    assert(method.getBasicBlock(&method, 1) == NULL);
    assert(method.getBasicBlock(&method, 2) == NULL);


    assert(method.getBasicBlocksNumber(&method) == 1);
    newBlock = method.newBasicBlock(&method);
    assert(newBlock != NULL);
    newBlock->startInst = 1;
    assert(method.getBasicBlocksNumber(&method) == 2);

    assert(method.getBasicBlock(&method, -1) == NULL);
    assert(method.getBasicBlock(&method, 0) != NULL);
    assert((method.getBasicBlock(&method, 0))->startInst == 42);
    assert(method.getBasicBlock(&method, 1) == newBlock);
    assert((method.getBasicBlock(&method, 1))->startInst == 1);
    assert(method.getBasicBlock(&method, 2) == NULL);

    assert(method.getBasicBlocksNumber(&method) == 2);
    method.deleteBasicBlocks(&method);
    assert(method.getBasicBlocksNumber(&method) == 0);

    assert(method.getBasicBlock(&method, -1) == NULL);
    assert(method.getBasicBlock(&method, 0) == NULL);
    assert(method.getBasicBlock(&method, 1) == NULL);
    assert(method.getBasicBlock(&method, 2) == NULL);

    assert(method.getBasicBlocksNumber(&method) == 0);
    newBlock = method.newBasicBlock(&method);
    assert(newBlock != NULL);
    newBlock->startInst = 13;
    assert(method.getBasicBlocksNumber(&method) == 1);
    newBlock = method.newBasicBlock(&method);
    assert(newBlock != NULL);
    newBlock->startInst = 17;
    assert(method.getBasicBlocksNumber(&method) == 2);

    assert(method.getBasicBlock(&method, -1) == NULL);
    assert(method.getBasicBlock(&method, 0) != NULL);
    assert((method.getBasicBlock(&method, 0))->startInst == 13);
    assert(method.getBasicBlock(&method, 1) == newBlock);
    assert((method.getBasicBlock(&method, 1))->startInst == 17);
    assert(method.getBasicBlock(&method, 2) == NULL);

    method.destroy(&method);

    assert(method.getBasicBlock(&method, -1) == NULL);
    assert(method.getBasicBlock(&method, 0) == NULL);
    assert(method.getBasicBlock(&method, 1) == NULL);
    assert(method.getBasicBlock(&method, 2) == NULL);

    return 0;
}
