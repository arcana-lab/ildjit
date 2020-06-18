#include <tests_utils.h>
#include <ir_method.h>

int main (int argc, char * argv[]) {
    ir_lib_t ir_lib;
    ir_method_t method;
    JITUINT32 varsID[5][2];
    JITUINT32 varID, i, j;

    memset(varsID, 0, sizeof(varsID));

    IRLibNew(&ir_lib, NULL, tests_malloc, tests_realloc, tests_free, NULL);
    ir_method_init(&ir_lib, &method);

    /* basic size allowed, internal variable */
    method.root_set.alloc_base_size = 2;

    assert(method.allocRootSet(&method) != NULL);
    assert(method.getRootSetTop(&method) == 0);
    method.deleteRootSet(&method);
    assert(method.getRootSetTop(&method) == 0);

    assert(method.allocRootSet(&method) != NULL);
    assert(method.getRootSetTop(&method) == 0);
    method.addVariableToRootSet(&method, 666);
    assert(method.getRootSetSlot(&method, 0) == 666);
    assert(method.getRootSetTop(&method) == 1);

    for (i = 0; i < 10000; i++) {
        assert(method.allocRootSet(&method) != NULL);
    }

    assert(method.getRootSetTop(&method) == 1);
    method.deleteRootSet(&method);
    assert(method.getRootSetTop(&method) == 0);

    varsID[0][0] = 42;
    varsID[1][0] = 17;
    varsID[2][0] = 1;
    varsID[3][0] = 7;
    varsID[4][0] = 99;

    for (i = 0; i < 5; i++) {
        method.addVariableToRootSet(&method, varsID[i][0]);
        assert(method.getRootSetTop(&method) == i + 1);
    }

    for (i = 0; i < 5; i++) {
        varID = method.getRootSetSlot(&method, i);
        for (j = 0; j < 5; j++) {
            if (varsID[j][0] == varID) {
                break;
            }
        }
        assert(varsID[j][1] == 0);
        assert(varsID[j][0] == varID);
        varsID[j][1]++;
    }
    assert(method.getRootSetTop(&method) == 5);

    assert(method.delVariableToRootSet(&method, varsID[4][0]) > 0);
    assert(method.getRootSetTop(&method) == 4);
    assert(method.delVariableToRootSet(&method, varsID[0][0]) > 0);
    assert(method.getRootSetTop(&method) == 3);
    assert(method.delVariableToRootSet(&method, varsID[2][0]) > 0);
    assert(method.getRootSetTop(&method) == 2);
    assert(method.delVariableToRootSet(&method, varsID[3][0]) > 0);
    assert(method.getRootSetTop(&method) == 1);

    assert(method.getRootSetSlot(&method, 0) == varsID[1][0]);
    assert(method.delVariableToRootSet(&method, varsID[1][0]) > 0);
    assert(method.getRootSetTop(&method) == 0);

    method.destroy(&method);
    assert(method.getRootSetTop(&method) == 0);

    return 0;
}
