#include <tests_utils.h>
#include <ir_method.h>

int main (int argc, char * argv[]) {
    ir_lib_t ir_lib;
    ir_method_t method;
    JITUINT32 i;

    IRLibNew(&ir_lib, NULL, tests_malloc, tests_realloc, tests_free, NULL);
    ir_method_init(&ir_lib, &method);

    assert(method.getLocalsNumber(&method) == 0);

    JITUINT32 size[] = { 42, 1, 17, 0 };
    for (i = 0; i < 4; i++) {
        method.setLocalsNumber(&method, size[i]);
        assert(method.getLocalsNumber(&method) == size[i]);
    }

    method.setLocalsNumber(&method, 3);
    assert(method.getLocalsNumber(&method) == 3);

    assert(method.getNextLocal(&method, NULL) == NULL);

    JITUINT32 first = 13;
    JITUINT32 second = 666;
    JITUINT32 third = 42;

    method.insertLocal(&method, &first);
    assert(method.getNextLocal(&method, NULL) == &first);

    method.insertLocal(&method, &second);
    assert(method.getNextLocal(&method, NULL) == &first);
    assert(method.getNextLocal(&method, &first) == &second);
    assert(method.getNextLocal(&method, &second) == NULL);

    method.insertLocal(&method, &third);
    assert(method.getNextLocal(&method, NULL) == &first);
    assert(method.getNextLocal(&method, &first) == &second);
    assert(method.getNextLocal(&method, &second) == &third);
    assert(method.getNextLocal(&method, &third) == NULL);


    method.destroy(&method);

    assert(method.local_internal_types == NULL);

    return 0;
}

