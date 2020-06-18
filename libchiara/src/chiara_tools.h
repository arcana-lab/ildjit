#ifndef CHIARA_TOOLS_H
#define CHIARA_TOOLS_H

#include <jitsystem.h>
#include <ir_method.h>

JITBOOLEAN TOOLS_isInstructionADominator (ir_method_t *method, JITUINT32 numInsts, ir_instruction_t *inst, ir_instruction_t *dominated, JITINT32 **doms, JITBOOLEAN(*dominance)(ir_method_t *, ir_instruction_t *, ir_instruction_t *));

#endif
