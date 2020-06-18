#ifndef ILDJIT_CALLBACKS_H
#define ILDJIT_CALLBACKS_H

void ILDJIT_Shutdown (void);

/**
 *
 * @result 1 if pc is a loop header. 0 otherwise
 */
JITUINT32 ILDJIT_IsLoopHeader (JITUINT32 pc);

/**
 *
 * @result 0 if the pc does not belong to any loops. otherwise it returns the program counter of the outer loop header if it exist.
 */
JITUINT32 ILDJIT_LoopParentHeaderAddress (JITUINT32 pc);

/**
 *
 * @result 0 if the pc does not belong to any loops. otherwise it returns the program counter of the innermost loop header where pc belongs to
 */
JITUINT32 ILDJIT_LoopHeaderAddress (JITUINT32 pc);


/**
 *
 * @result The ID of the IR instruction where pc belongs to.
 */
JITUINT32 ILDJIT_IrId (JITUINT32 pc);

void ILDJIT_MappingCallback (JITNUINT pc, char *function_name, JITINT32 *ir_inst_number);

JITUINT32 ILDJIT_BranchPrediction (JITUINT32 pc);

void ILDJIT_CommitCallback (JITUINT32 pc);

void ILDJIT_IssueCallback (JITUINT32 pc);

JITUINT32 ILDJIT_RtnNameByAddress (JITUINT32 pc, char *buffer, JITUINT32 buffer_size);

JITUINT32 ILDJIT_IsTrampoline (JITUINT32 pc);

JITUINT32 ILDJIT_RtnAddressByAddress (JITUINT32 pc);

JITUINT32 ILDJIT_RtnAddressByName (char *methodName);

JITUINT32 ILDJIT_VoltageCallback (JITUINT32 pc, JITUINT32 recovery_pc);

void ILDJIT_NativeStart (void);

void ILDJIT_NativeStop (void);

#endif
