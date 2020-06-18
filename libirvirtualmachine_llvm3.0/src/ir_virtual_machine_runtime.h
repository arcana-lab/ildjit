#ifndef IR_VIRTUAL_MACHINE_RUNTIME_H
#define IR_VIRTUAL_MACHINE_RUNTIME_H

#include <jitsystem.h>

extern "C" {
    JITINT32 ir_isnanf (JITFLOAT32 value);
    JITINT32 ir_isnan (JITFLOAT64 value);
    JITINT32 ir_isinff (JITFLOAT32 value);
    JITINT32 ir_isinf (JITFLOAT64 value);
    void ir_leaveExecution (JITINT32 exitCode);
    void * ir_malloc (size_t size);
    void * ir_calloc (size_t nmemb, size_t size);
    size_t ir_strlen (char *str);
    int ir_strcmp (char *s1, char *s2);
    char * ir_strchr (char *s, JITINT32 c);
    int ir_memcmp (char *s1, char *s2);
    void * ir_memcpy (void *s1, void *s2, JITINT32 n);
    void ir_memset (void *s, int c, int n);
};

#endif
