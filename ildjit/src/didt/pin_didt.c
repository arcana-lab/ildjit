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
#ifdef MORPHEUS

#include <stdio.h>
#include <errno.h>
#include <jitsystem.h>
#include <ir_method.h>
#include <ir_optimization_interface.h>
#include <jit/jit.h>

// My headers
#include <ilmethod.h>
#include <system_manager.h>
#include <jit_metadata.h>
#include <iljit_dumper.h>
#include <pin_didt.h>
#include <general_tools.h>
// End

#define DIM_BUF 1024

void ILDJIT_NativeStart (void);
void ILDJIT_NativeStop (void);
JITINT32 internal_mapping (t_system *system, JITUINT32 pc, Method *method, t_ir_instruction **inst);
void internal_dump_pc_snapshot (t_system *system, JITUINT32 free_position, JITUINT32 *buffer);

void MORPHEUS_Initialize (didt_log_t *log) {
    t_system    *system;
    JITINT8 buf[DIM_BUF];
    didt_log_t  *didt_log;

    /* Fetch the system		*/
    system = getSystem(NULL);
    assert(system);
    didt_log = &(system->didt_log);

    /* Erase the counters		*/
    memset(log->issued_pc, -1, sizeof(JITNUINT) * DIDT_BUF);
    memset(log->committed_pc, -1, sizeof(JITNUINT) * DIDT_BUF);
    memset(log->issued_ir, -1, sizeof(JITNUINT) * DIDT_BUF);

    /* Open DIDT file               */
    snprintf(buf, sizeof(char) * DIM_BUF, "%s.Snapshot", (system->behavior).outputPrefix);
    didt_log->DIDT_report = fopen(buf, "w");
    if (didt_log->DIDT_report == NULL) {
        snprintf(buf, sizeof(char) * DIM_BUF, "ERROR = Cannot open file %s to wrte the assembly. ", buf);
        print_err(buf, errno);
        abort();
    }

    /* Return			*/
    return;
}

void MORPHEUS_Shutdown () {

}

void MORPHEUS_StartExecutionNotify () {
    t_system        *system;

    /* Fetch the system		*/
    system = getSystem(NULL);
    assert(system != NULL);

    /* Dump the assembly            */
    dump_methods_assembly(system->getMethods(system));

    /* Return			*/
    return;
}

void internal_insert_native_start_function (Method method) {
    jit_type_t sign;
    t_system    *system;

    system = getSystem(NULL);
    if (method == (system->program).entry_point) {
        sign = jit_type_create_signature(jit_abi_cdecl, jit_type_void, NULL, 0, 1);
        jit_insn_call_native(method->getJITFunction(method)->function, "ILDJIT_NativeStart", ILDJIT_NativeStart, sign, NULL, 0, JIT_CALL_NOTHROW);
    }
}

void internal_insert_native_stop_function (Method method) {
    jit_type_t sign;
    t_system    *system;

    system = getSystem(NULL);
    if (method == (system->program).entry_point) {
        sign = jit_type_create_signature(jit_abi_cdecl, jit_type_void, NULL, 0, 1);
        jit_insn_call_native(method->getJITFunction(method)->function, "ILDJIT_NativeStop", ILDJIT_NativeStop, sign, NULL, 0, JIT_CALL_NOTHROW);
    }
}

JITINT32 internal_mapping (t_system *system, JITUINT32 pc, Method *method, t_ir_instruction **inst) {
    jit_function_t func;

    (*method) = NULL;
    (*inst) = NULL;

    func = jit_function_from_pc((system->program).context, (void *) pc, NULL);
    if (func == NULL) {
        if (jit_function_is_a_trampoline((system->program).context, (void *) pc)) {
            return 1;
        }
        return 2;
    }
    (*method) = jit_function_get_meta(func, METHOD);
    if (*method == NULL) {
        print_err("ILDJIT: ERROR = Method is NULL. ", 0);
        abort();
    }
    (*inst) = (*method)->getIRInstructionFromPC(*method, pc);
    if (*inst == NULL) {
        ir_method_t *ir_method;
        ir_method = (*method)->getIRMethod(*method);
//      char            buf[DIM_BUF];
        //       snprintf(buf, DIM_BUF, "ILDJIT: WARNING = the mapping is not correct for the PC=0x%X for the method %s. ", pc, (*method)->getName(*method));
        //       print_err(buf, 0);
        (*inst) = IRMETHOD_getFirstInstruction(ir_method);
    }
    return 0;
}

void internal_dump_pc_snapshot (t_system *system, JITUINT32 free_position, JITUINT32 *buffer) {
    JITINT32 count;
    Method method;
    t_ir_instruction    *inst;
    didt_log_t          *didt_log;

    didt_log = &(system->didt_log);
    assert(didt_log != NULL);

    count = free_position;
    if (    (count < DIDT_BUF)    &&
            (buffer[count] != -1)   ) {
        while (count < DIDT_BUF) {
            if (internal_mapping(system, buffer[count], &method, &inst) == 0) {
                fprintf(didt_log->DIDT_report, "  PC=0x%X     Method=%s   IR=%d\n", buffer[count], method->getName(method), inst->ID);
            } else {
                fprintf(didt_log->DIDT_report, "  PC=0x%X\n", buffer[count]);
            }
            count++;
        }
    }
    count = 0;
    while (count < free_position) {
        if (internal_mapping(system, buffer[count], &method, &inst) == 0) {
            fprintf(didt_log->DIDT_report, "  PC=0x%X     Method=%s   IR=%d\n", buffer[count], method->getName(method), inst->ID);
        } else {
            fprintf(didt_log->DIDT_report, "  PC=0x%X\n", buffer[count]);
        }
        count++;
    }
}




/*###################################### Call backs from the ildjit PIN tool #############################################*/
JITUINT32 ILDJIT_IrId (JITUINT32 pc) {
    t_system            *system;
    t_ir_instruction    *inst;
    Method method;
    JITUINT32 result;

    system = getSystem(NULL);
    if (system == NULL) {
        print_err("ILDJIT: ERROR = System is NULL. ", 0);
        abort();
    }
    inst = NULL;
    method = NULL;
    result = internal_mapping(system, pc, &method, &inst);
    switch (result) {
        case 0:
            return inst->ID;
        case 1:
            return -2;
    }
    return -1;
}

void ILDJIT_MappingCallback (JITNUINT pc, char *function_name, JITINT32 *ir_inst_number) {
    t_system            *system;
    t_ir_instruction    *inst;
    Method method;

    system = getSystem(NULL);
    if (system == NULL) {
        print_err("ILDJIT: ERROR = System is NULL. ", 0);
        abort();
    }
    inst = NULL;
    method = NULL;
    if (internal_mapping(system, pc, &method, &inst) == 0) {
        (*ir_inst_number) = inst->ID;
        sprintf(function_name, "%s", method->getName(method));
    } else {
        (*ir_inst_number) = -1;
        sprintf(function_name, " ");
    }
}

void ILDJIT_BranchCallback (JITUINT32 pc) {

}

void ILDJIT_IssueCallback (JITUINT32 pc) {
    t_ir_instruction     *inst;
    Method method;
    t_system             *system;
    didt_log_t           *didt_log;

    system = getSystem(NULL);
    if (system == NULL) {
        print_err("ILDJIT: ERROR = System is NULL. ", 0);
        abort();
    }
    didt_log = &(system->didt_log);
    assert(didt_log != NULL);

    /* Store the pc                 */
    if (didt_log->current_issued_pc_free_position == DIDT_BUF) {
        didt_log->current_issued_pc_free_position = 0;
    }
    didt_log->issued_pc[didt_log->current_issued_pc_free_position] = pc;
    (didt_log->current_issued_pc_free_position)++;

    /* Store the IR                 */
    if (internal_mapping(system, pc, &method, &inst) != 0) {
        return;
    }
    if (didt_log->current_issued_ir_free_position == DIDT_BUF) {
        didt_log->current_issued_ir_free_position = 0;
    }
    if (didt_log->issued_ir[didt_log->current_issued_ir_free_position] != inst->ID) {
        didt_log->issued_ir[didt_log->current_issued_ir_free_position] = inst->ID;
        (didt_log->current_issued_ir_free_position)++;
    }
}

JITUINT32 ILDJIT_RtnNameByAddress (JITUINT32 pc, char *buffer, JITUINT32 buffer_size) {
    t_system            *system;
    t_ir_instruction    *inst;
    Method method;
    JITINT32 result;

    system = getSystem(NULL);
    if (system == NULL) {
        print_err("ILDJIT: ERROR = System is NULL. ", 0);
        abort();
    }

    /* Mapping                  */
    inst = NULL;
    method = NULL;
    result = internal_mapping(system, pc, &method, &inst);
    if (    (result == 0) || (method != NULL)   ) {
        snprintf(buffer, buffer_size, "%s", method->getName(method));
        return strlen(method->getName(method)) + 1;
    } else if (result == 1) {
        return -2;
    }
    return -1;
}

JITUINT32 ILDJIT_IsTrampoline (JITUINT32 pc) {
    Method method;
    t_ir_instruction    *inst;
    t_system                *system;
    JITUINT32 result;

    /* Get the system		*/
    system = getSystem(NULL);
    assert(system != NULL);

    /* Mapping              */
    result = internal_mapping(system, pc, &method, &inst);
    switch (result) {
        case 1:
            return 1;
    }
    return 0;
}

JITUINT32 ILDJIT_RtnAddressByAddress (JITUINT32 pc) {
    Method method;
    t_ir_instruction    *inst;
    t_system                *system;
    JITUINT32 result;

    /* Get the system		*/
    system = getSystem(NULL);
    assert(system != NULL);

    /* Mapping              */
    result = internal_mapping(system, pc, &method, &inst);
    switch (result) {
        case 0:
            return jit_function_get_first_pc((system->program).context, method->getJITFunction(method)->function);
        case 1:
            return -2;
        case 2:
            return -1;
        default:
            print_err("ILDJIT: internal_mapping error. ", 0);
            abort();
    }
    return -1;
}

JITUINT32 ILDJIT_RtnAddressByName (char *methodName) {
    XanListItem *item;
    XanList     *methods;
    Method currentMethod;
    t_system        *system;

    /* Get the system		*/
    system = getSystem(NULL);
    assert(system != NULL);

    methods = system->getMethods(system)->container;

    /* Dump the assembly of the	*
     * methods			*/
    item = methods->first(methods);
    assert(item != NULL);
    while (item != NULL) {
        currentMethod = (Method) item->data;
        assert(currentMethod != NULL);
        if (    (currentMethod->isIrImplemented(currentMethod))                 &&
                (currentMethod->getState(currentMethod) == EXECUTABLE_STATE)            ) {
            if (strcmp(currentMethod->getName(currentMethod), methodName) == 0) {
                return jit_function_get_first_pc((system->program).context, currentMethod->getJITFunction(currentMethod)->function);
            }
        }
        item = methods->next(methods, item);
    }
    return -1;
}

void ILDJIT_CommitCallback (JITUINT32 pc) {
    t_system            *system;
    didt_log_t          *didt_log;

    system = getSystem(NULL);
    if (system == NULL) {
        print_err("ILDJIT: ERROR = System is NULL. ", 0);
        abort();
    }
    didt_log = &(system->didt_log);

    /* Store the pc                 */
    if (didt_log->current_committed_pc_free_position == DIDT_BUF) {
        didt_log->current_committed_pc_free_position = 0;
    }
    didt_log->committed_pc[didt_log->current_committed_pc_free_position] = pc;
    (didt_log->current_committed_pc_free_position)++;
}

JITUINT32 ILDJIT_VoltageCallback (JITUINT32 pc) {
    t_system            *system;
    t_ir_instruction    *inst;
    Method method;
    JITINT32 result;
    didt_log_t          *didt_log;

    system = getSystem(NULL);
    if (system == NULL) {
        print_err("ILDJIT: ERROR = System is NULL. ", 0);
        abort();
    }
    didt_log = &(system->didt_log);

    //  fprintf(system->DIDT_report, "Last committed branch:    PC=0x%X     ", pc);

    /* Mapping                  */
    inst = NULL;
    method = NULL;
    result = internal_mapping(system, pc, &method, &inst);
    switch (result) {
        case 0:
//          fprintf(stderr, "ILDJIT: Mapping: Method = %s\n", method->getName(method));
//          fprintf(stderr, "ILDJIT: Mapping: Pc=0x%X IR=%u\n", pc, inst->ID);
            (inst->voltage_emergencies)++;
            XanListItem *item;
            didt_pc_t *didt;
            item = inst->DIDT_pc_list->first(inst->DIDT_pc_list);
            while (item != NULL) {
                didt = inst->DIDT_pc_list->data(inst->DIDT_pc_list, item);
                if (didt == NULL) {
                    print_err("ILDJIT: ERROR = DIDT list corrupted. ", 0);
                    abort();
                }
                if (didt->pc == pc) {
                    (didt->count)++;
                    break;
                }
                item = inst->DIDT_pc_list->next(inst->DIDT_pc_list, item);
            }
            didt = malloc(sizeof(didt_pc_t));
            didt->count = 1;
            didt->pc = pc;
            inst->DIDT_pc_list->insert(inst->DIDT_pc_list, didt);
//            fprintf(system->DIDT_report, "Method=%s     IR=%d\n", method->getName(method), inst->ID);
            result = 0;
            break;
        case 1:
            (didt_log->libjitDIDT)++;
//            fprintf(system->DIDT_report, "Trampoline\n");
            result = -1;
            break;
        case 2:
            (didt_log->internalDIDT)++;
//            fprintf(system->DIDT_report, "JIT\n");
            result = -1;
            break;
        default:
            print_err("ILDJIT: Mapping not correct. ", 0);
            abort();
    }

    /* Dump the snapshot            */
    /*    fprintf(system->DIDT_report, "ISSUE (from the old one to the new one)\n");
        internal_dump_pc_snapshot(system, system->current_issued_pc_free_position, system->issued_pc);
        fprintf(system->DIDT_report, "  PC=0x%X         EMERGENCY\n\n", pc);

        fprintf(system->DIDT_report, "COMMIT (from the old one to the new one)\n");
        internal_dump_pc_snapshot(system, system->current_committed_pc_free_position, system->committed_pc);
        fprintf(system->DIDT_report, "  PC=0x%X         EMERGENCY\n\n", pc);

        fprintf(system->DIDT_report, "IR ISSUE (from the old one to the new one)\n");
        count   = system->current_issued_ir_free_position;
        if (    (count < DIDT_BUF)            &&
                (system->issued_ir[count] != -1)){
                while (count < DIDT_BUF){
                    fprintf(system->DIDT_report, "  IR=%d\n", system->issued_ir[count]);
                    count++;
                }
        }
        count   = 0;
        while (count < system->current_issued_ir_free_position){
            fprintf(system->DIDT_report, "  IR=%d\n", system->issued_ir[count]);
            count++;
        }
        if (inst != NULL){
            fprintf(system->DIDT_report, "  IR=%d      EMERGENCY\n", inst->ID);
        }
        fprintf(system->DIDT_report, "**********************************************\n");*/

    return result;
}

void ILDJIT_NativeStart (void) {

}

void ILDJIT_NativeStop (void) {
    t_system    *system;
    XanList *methods;
    XanListItem *item;
    JITINT32 count;
    JITINT32 different_pc = 0;
    Method method;

    system = getSystem(NULL);
    if ((system->behavior).aot) {
        char buf[1000];
        snprintf(buf, 1000, "%s.DIDT", (system->behavior).outputPrefix);
        FILE *out = fopen(buf, "w");
        if (out == NULL) {
            print_err("ILDJIT: ERROR = Cannot open the file. ", errno);
            exit(0);
        }
        methods = system->getMethods(system)->container;
        count = 0;
        item = methods->first(methods);
        while (item != NULL) {
            method = (Method) item->data;
            if (    (method->getState(method) == EXECUTABLE_STATE)   &&
                    method->isCilImplemented(method)                ) {
                ((system->optimizations).optimizer).callMethodOptimization(&((system->optimizations).optimizer), method->getIRMethod(method), &(system->ir_system), METHOD_PRINTER);

                ir_method_t *ir;
                t_ir_instruction    *inst;
                JITINT32 i;
                ir = method->getIRMethod(method);
                for (i = 0; i < ir->getInstructionsNumber(ir); i++) {
                    inst = ir->getInstructionAt(ir, i);
                    different_pc += inst->DIDT_pc_list->length(inst->DIDT_pc_list);
                    count += inst->voltage_emergencies;
                }
            }
            item = methods->next(methods, item);
        }
        fprintf(out, "ILDJIT: Total number of voltages emergencies on the translated code = %d\n", count);
        fprintf(out, "ILDJIT: Total number of voltages emergencies on the jit code = %d\n", (system->didt_log).internalDIDT);
        fprintf(out, "ILDJIT: Total number of voltages emergencies on the trampolines = %d\n", (system->didt_log).libjitDIDT);
        fprintf(out, "ILDJIT: Total number of different program counter that have raised a voltage emergencies on the translated code = %d\n", different_pc);
    }
}

#endif
