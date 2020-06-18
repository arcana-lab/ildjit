#include <tests_utils.h>
#include <ir_language.h>
#include <ir_method.h>

int main (int argc, char * argv[]) {
    ir_lib_t ir_lib;
    ir_method_t method;

    IRLibNew(&ir_lib, NULL, tests_malloc, tests_realloc, tests_free, NULL);
    ir_method_init(&ir_lib, &method);

    assert(method.getInstructionsNumber(&method) == 0);
    assert(method.isVariableInRootSet(&method, 5) == 0);

    method.destroy(&method);

    return 0;
}
