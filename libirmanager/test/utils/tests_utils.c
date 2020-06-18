#include <malloc.h>
#include <assert.h>

/** function needed by ir library interface */
void * tests_malloc (void * context, size_t size) {
    void * mem = malloc(size);

    assert(mem != NULL);
    return mem;
}
/** function needed by ir library interface */
void * tests_realloc (void * context, void * ptr, size_t size) {
    void * mem = realloc(ptr, size);

    assert(mem != NULL);
    return mem;
}
/** function needed by ir library interface */
void  tests_free (void * context, void * ptr) {
    return free(ptr);
}
