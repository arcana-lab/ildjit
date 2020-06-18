/*
 * Copyright (C) 2008  Campanoni Simone
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <ir_optimization_interface.h>
#include <ir_language.h>
#include <iljit-utils.h>

// My headers
#include <optimizer_algebraic.h>
#include <config.h>
// End

#define INFORMATIONS    "This is the algebraic simplification plugin"
#define AUTHOR          "Campanoni Simone"

static inline JITUINT64 algebric_get_ID_job (void);
static inline char * algebric_get_version (void);
static inline void algebric_do_job (ir_method_t * method);
static inline char * algebric_get_informations (void);
static inline char * algebric_get_author (void);
static inline JITUINT64 algebric_get_dependences (void);
static inline JITUINT64 algebric_get_invalidations (void);
static inline void algebric_shutdown (JITFLOAT32 totalTime);
static inline void algebric_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix);
static inline void algebraic_getCompilationTime (char *buffer, JITINT32 bufferLength);
static inline void algebraic_getCompilationFlags (char *buffer, JITINT32 bufferLength);
void internal_translate_into_store_to_value (ir_method_t *method, ir_instruction_t *inst, ir_item_t *value);
void internal_translate_into_store (ir_method_t *method, ir_instruction_t *inst, ir_item_t *value);

ir_lib_t        *irLib = NULL;
ir_optimizer_t  *irOptimizer = NULL;
char            *prefix = NULL;

ir_optimization_interface_t plugin_interface = {
    algebric_get_ID_job,
    algebric_get_dependences,
    algebric_init,
    algebric_shutdown,
    algebric_do_job,
    algebric_get_version,
    algebric_get_informations,
    algebric_get_author,
    algebric_get_invalidations,
    NULL,
    algebraic_getCompilationFlags,
    algebraic_getCompilationTime
};

static inline void algebric_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix) {

    /* Assertions			*/
    assert(lib != NULL);
    assert(optimizer != NULL);
    assert(outputPrefix != NULL);

    irLib = lib;
    irOptimizer = optimizer;
    prefix = outputPrefix;
}

static inline void algebric_shutdown (JITFLOAT32 totalTime) {
    irLib = NULL;
    irOptimizer = NULL;
    prefix = NULL;
}

static inline JITUINT64 algebric_get_ID_job (void) {
    return ALGEBRAIC_SIMPLIFICATION;
}

static inline JITUINT64 algebric_get_dependences (void) {
    return 0;
}

static inline JITUINT64 algebric_get_invalidations (void) {
    return INVALIDATE_ALL;
}

static inline void algebric_do_job (ir_method_t * method) {
    JITINT32 count;
    ir_instruction_t        *inst;
    ir_item_t               *item;
    ir_item_t               *var;
    ir_item_t value;
    JITUINT32 instructionsNumber;

    /* Assertions				*/
    assert(method != NULL);

    /* Fetch the number of instructions	*/
    instructionsNumber = IRMETHOD_getInstructionsNumber(method);

    /* Apply the optimization               */
    for (count = 0; count < instructionsNumber; count++) {
        inst = IRMETHOD_getInstructionAtPosition(method, count);
        assert(inst != NULL);
        item = NULL;
        switch (inst->type) {
            case IRMUL:
            case IRMULOVF:
            case IRPOW:

                /* Fetch the constant		*/
                if (IRMETHOD_getInstructionParameter1Type(inst) != IROFFSET) {
                    item = IRMETHOD_getInstructionParameter1(inst);
                    var = IRMETHOD_getInstructionParameter2(inst);
                } else if (IRMETHOD_getInstructionParameter2Type(inst) != IROFFSET) {
                    item = IRMETHOD_getInstructionParameter2(inst);
                    var = IRMETHOD_getInstructionParameter1(inst);
                }
                if (item == NULL) {
                    break;
                }
                assert(item->type != IROFFSET);

                /* Apply the optimization	*/
                if (    ((item->value).v == 1)      ||
                        ((item->value).f == 1)     ) {
                    internal_translate_into_store(method, inst, var);

                } else if (     ((item->value).v == 0)      &&
                                ((item->value).f == 0)     ) {
                    memcpy(&value, item, sizeof(ir_item_t));
                    value.value.v = 0;
                    internal_translate_into_store_to_value(method, inst, &value);
                }
                break;
            case IRDIV:
                if (IRMETHOD_getInstructionParameter2Type(inst) == IROFFSET) {
                    break;
                }
                item = IRMETHOD_getInstructionParameter2(inst);
                assert(item != NULL);
                if (    ((item->value).v == 1)      ||
                        ((item->value).f == 1)     ) {
                    internal_translate_into_store(method, inst, IRMETHOD_getInstructionParameter1(inst));
                }
                break;
            case IRADD:
            case IRADDOVF:
                if (IRMETHOD_getInstructionParameter1Type(inst) != IROFFSET) {
                    item = IRMETHOD_getInstructionParameter1(inst);
                    var = IRMETHOD_getInstructionParameter2(inst);
                } else if (IRMETHOD_getInstructionParameter2Type(inst) != IROFFSET) {
                    var = IRMETHOD_getInstructionParameter1(inst);
                    item = IRMETHOD_getInstructionParameter2(inst);
                }
                if (item == NULL) {
                    break;
                }
                switch (item->type) {
                    case IRFLOAT32:
                    case IRFLOAT64:
                    case IRNFLOAT:
                        if ((item->value).f == 0) {
                            internal_translate_into_store(method, inst, var);
                        }
                        break;
                    default:
                        if ((item->value).v == 0) {
                            internal_translate_into_store(method, inst, var);
                        }
                }
                break;
            case IRREM:
                if (IRMETHOD_getInstructionParameter2Type(inst) == IROFFSET) {
                    break;
                }
                item = IRMETHOD_getInstructionParameter2(inst);
                switch (item->type) {
                    case IRFLOAT32:
                    case IRFLOAT64:
                    case IRNFLOAT:
                        if ((item->value).f == 0) {
                            memcpy(&value, item, sizeof(ir_item_t));
                            value.value.v = 0;
                            internal_translate_into_store_to_value(method, inst, &value);
                        }
                        break;
                    default:
                        if ((item->value).v == 0) {
                            memcpy(&value, item, sizeof(ir_item_t));
                            value.value.v = 0;
                            internal_translate_into_store_to_value(method, inst, &value);
                        }
                }
                break;
            case IRSHL:
            case IRSHR:
            case IRSUB:
            case IRSUBOVF:
                if (IRMETHOD_getInstructionParameter2Type(inst) == IROFFSET) {
                    break;
                }
                var = IRMETHOD_getInstructionParameter1(inst);
                item = IRMETHOD_getInstructionParameter2(inst);
                switch (item->type) {
                    case IRFLOAT32:
                    case IRFLOAT64:
                    case IRNFLOAT:
                        if ((item->value).f == 0) {
                            internal_translate_into_store(method, inst, var);
                        }
                        break;
                    default:
                        if ((item->value).v == 0) {
                            internal_translate_into_store(method, inst, var);
                        }
                }
                break;
        }
    }

    /* Return				*/
    return;
}

static inline char * algebric_get_version (void) {
    return VERSION;
}

static inline char * algebric_get_informations (void) {
    return INFORMATIONS;
}

static inline char * algebric_get_author (void) {
    return AUTHOR;
}

void internal_translate_into_store_to_value (ir_method_t *method, ir_instruction_t *inst, ir_item_t *value) {
    ir_item_t res;
    ir_item_t       *item;

    item = IRMETHOD_getInstructionResult(inst);
    assert(item != NULL);
    memcpy(&res, item, sizeof(ir_item_t));
    IRMETHOD_eraseInstructionFields(method, inst);
    IRMETHOD_setInstructionType(method, inst, IRMOVE);
    IRMETHOD_cpInstructionResult(inst, &res);
    IRMETHOD_cpInstructionParameter1(inst, value);
}

void internal_translate_into_store (ir_method_t *method, ir_instruction_t *inst, ir_item_t *value) {
    ir_item_t res;
    ir_item_t par;
    ir_item_t       *item;

    memcpy(&par, value, sizeof(ir_item_t));
    item = IRMETHOD_getInstructionResult(inst);
    assert(item != NULL);
    memcpy(&res, item, sizeof(ir_item_t));
    IRMETHOD_eraseInstructionFields(method, inst);
    IRMETHOD_setInstructionType(method, inst, IRMOVE);
    IRMETHOD_cpInstructionResult(inst, &res);
    IRMETHOD_cpInstructionParameter1(inst, &par);
}

static inline void algebraic_getCompilationFlags (char *buffer, JITINT32 bufferLength) {

    /* Assertions				*/
    assert(buffer != NULL);

    snprintf(buffer, sizeof(char) * bufferLength, " ");
#ifdef DEBUG
    strncat(buffer, "DEBUG ", sizeof(char) * bufferLength);
#endif
#ifdef PRINTDEBUG
    strncat(buffer, "PRINTDEBUG ", sizeof(char) * bufferLength);
#endif
#ifdef PROFILE
    strncat(buffer, "PROFILE ", sizeof(char) * bufferLength);
#endif
}

static inline void algebraic_getCompilationTime (char *buffer, JITINT32 bufferLength) {

    /* Assertions				*/
    assert(buffer != NULL);

    snprintf(buffer, sizeof(char) * bufferLength, "%s %s", __DATE__, __TIME__);

    /* Return				*/
    return;
}
