#include <stdlib.h>

/** function needed by ir library interface */
void * tests_malloc (void * context, size_t size);
/** function needed by ir library interface */
void * tests_realloc (void * context, void * ptr, size_t size);
/** function needed by ir library interface */
void  tests_free (void * context, void * ptr);
