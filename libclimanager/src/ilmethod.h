/*
 * Copyright (C) 2006  Campanoni Simone
 *
 * iljit - This is a Just-in-time for the CIL language specified with the ECMA-335
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
#ifndef ILMETHOD_H
#define ILMETHOD_H

#include <xanlib.h>
#include <metadata/metadata_types.h>
#include <ir_method.h>
#include <ir_virtual_machine.h>

// My header
#include <cil_stack.h>
// End

#define CIL_STATE                               1
#define IR_STATE                                2
#define MACHINE_CODE_STATE                      3
#define EXECUTABLE_STATE                        4
#define NEEDS_RECOMPILE_STATE                   5

// Global states, state numbering sequence is relevant, if modified please modify also ilmethod.c accordingly
#define CIL_GSTATE				6
#define CILIR_ONGOING_GSTATE			7
#define DLA_PRE_GSTATE				8
#define DLA_ONGOING_GSTATE			9
#define IROPT_PRE_GSTATE			10
#define IROPT_ONGOING_GSTATE			11
#define IR_GSTATE				12

typedef struct t_try_block {
    JITUINT32 begin_offset;                 /**< THE OFFSET IN THE CIL CODE OF THE FUNCTION WHERE BEGINS THE
	                                         *  TRY-BLOCK BODY */

    JITUINT32 length;                       /**< THE LENGTH (IN BYTES) OF THE TRY-BLOCK */
    XanList         *inner_try_blocks;      /**< TO RETRIEVE THE INNER TRY-BLOCKS INSIDE THIS TRY-BLOCK.
	                                         *  THIS POINTER COULD BE NULL */

    XanList         *catch_handlers;        /**< THE CURRENT TRY-BLOCK CAN HAVE ONE OR MORE CATCH AND FILTER
	                                         *  HANDLERS ASSOCIATED WITH. THE XanList catches ALLOWS TO OBTAIN
	                                         *  ALL THE INFORMATIONS REGARDING EACH FAULT AND CATCH HANDLER.
	                                         *  THIS XanList IS MANTAINED ORDERED WITH RESPECT TO THE PRIORITY
	                                         * OF EACH HANDLER (I.E. THE FIRST ELEMENT OF THE LIST
	                                         * SHALL BE THE PRIOR TO BE CHECKED)  */

    XanList         *finally_handlers;      /**< THE CURRENT TRY-BLOCK CAN HAVE ONE OR MORE FALUT AND FINALLY HANDLERS
	                                         *  ASSOCIATED WITH. THE XanList finally_handlers ALLOWS TO OBTAIN ALL THE
	                                         *  INFORMATIONS REGARDING EACH FAULT AND FINALLY HANDLER.
	                                         *  THIS XanList IS MANTAINED ORDERED WITH RESPECT TO THE PRIORITY
	                                         *  OF EACH HANDLER (I.E. THE FIRST ELEMENT OF THE LIST
	                                         *  SHALL BE THE PRIOR TO BE CHECKED)  */

    t_stack         *stack_to_restore;      /**< THE IMAGE OF THE STACK THAT SHALL BE RESTORED ONCE EXITED FROM
	                                         * THE TRY-BLOCK WITH THE CIL Leave INSTRUCTION. INITIALLY
	                                         * stack_to_restore IS NULL. UPON ENTERING THE TRY-BODY THE VALUE
	                                         * OF stack_to_restore IS INITIALIZED WITH THE CURRENT Stack VALUE */
    struct t_try_block      *parent_block;  /**< THE BLOCK THAT OWNS THIS HANDLER
	                                        *  (NOT THE BLOCK ASSOCIATED WITH!)*/

    JITUINT32 try_start_label_ID;           /**< THE LABEL_ID ASSOCIATED WITH THE BEGINNING OF THE TRY-BLOCK */
    JITUINT32 try_end_label_ID;             /**< THE LABEL_ID ASSOCIATED WITH THE END OF THE TRY-BLOCK  */
} t_try_block;

typedef struct {
    JITUINT16 type;                                         /**< THE TYPE OF THE CATCH HANDLER. IT SHALL BE
	                                                         * ONE BETWEEN: FILTER_TYPE; EXCEPTION_TYPE;
	                                                         * FINALLY_TYPE; FAULT_TYPE.  */
    JITUINT32 handler_begin_offset;                         /**< THE OFFSET CIL AT WHICH THE HANDLER CODE BEGINS */
    JITUINT32 handler_length;                               /**< THE HANDLER LENGTH */
    JITUINT32 handler_start_label_ID;                       /**< THE LABEL ID ASSOCIATED WITH THE BEGINNING
	                                                         *   OF THE HANDLER BODY */
    JITUINT32 handler_end_label_ID;                         /**< THE LABEL ID ASSOCIATED WITH THE END
	                                                         *   OF THE HANDLER BODY */
    JITUINT32 filter_start_label_ID;                        /**< THE LABEL ID ASSOCIATED WITH THE BEGINNING OF THE FILTER BODY  */
    TypeDescriptor  *class;                                 /**< THE CLASS TOKEN (USED IN CATCH BLOCKS) */
    JITUINT32 filter_offset;                                /**< THE OFFSET AT WHICH BEGINS THE FILTER */
    t_try_block     *owner_block;                           /**< THE BLOCK THAT OWNS THIS HANDLER */
    JITUINT32 end_finally_successor_label_ID;               /**< THE LABEL THAT IDENTIFY A POSSIBLE SUCCESSIVE INSTRUCTION
	                                                         *   AFTER THE END FINALLY. USED FOR DATA-FLOW ANALYSIS */
    ir_instruction_t        *end_filter_or_finally_inst;    /**< link to the endFilter or endFinally instruction
	                                                         *   used by the catcher to correctly update
	                                                         *   the data-flow informations */
} t_try_handler;

typedef struct _Trampoline {
    struct _ILMethod *caller;
    ir_instruction_t *inst;
} Trampoline;

/**
 * Method
 *
 * This structure is used to store each information about a single method of the CIL application code.
 */
typedef struct _ILMethod {
    pthread_mutex_t		mutex;
    pthread_cond_t		notify;				/**< Used to make other process wait if this method is being CIL->IR by another one		*/
    ir_method_t		IRMethod;
    t_jit_function		**jit_function;			/**< JIT function bind to the method, make use of PMP		*/
    XanList			*cctorMethods;			/**< Set of cctors needed by the method	*/
    XanList                 *try_blocks;
    JITUINT32		state;
    JITUINT32		globalState;
    JITUINT32		externallyCallable;

    /* DLA dynamic trace information	*/
    JITFLOAT32 executionProbability;
    XanList                 *trampolinesSet;
    void (*addToTrampolinesSet)(struct _ILMethod *method, struct _ILMethod *caller, ir_instruction_t *inst);
    XanList * (*getTrampolinesSet)(struct _ILMethod *method);

    JITUINT32 (*getRootSetTop)(struct _ILMethod *method);
    JITUINT32 (*getRootSetSlot)(struct _ILMethod *method, JITUINT32 slotID);
    void (*addVariableToRootSet)(struct _ILMethod *method, JITUINT32 varID);
    XanList *                                               (*getCctorMethodsToCall)(struct _ILMethod *method);
    void (*addCctorToCall)(struct _ILMethod *method, struct _ILMethod *cctor);
    XanList *                       (*getTryBlocks)(struct _ILMethod *method);
    ir_method_t *                   (*getIRMethod)(struct _ILMethod *method);
    JITBOOLEAN (*isAnonymous)(struct _ILMethod *method);
    MethodDescriptor *                      (*getID)(struct _ILMethod *method);
    ir_signature_t *                (*getSignature)(struct _ILMethod *method);
    TypeDescriptor *                        (*getType)(struct _ILMethod *method);
    BasicAssembly *         (*getAssembly)(struct _ILMethod *method);
    GenericContainer *      (*getContainer)(struct _ILMethod *method);
    t_binary_information *(*getBinary)(struct _ILMethod *method);
    TypeDescriptor *                        (*getParameterILType)(struct _ILMethod *method, JITINT32 parameterNumber);
    TypeDescriptor *                        (*getReturnILType)(struct _ILMethod *method);
    JITUINT32 (*getParametersNumber)(struct _ILMethod *method);
    JITUINT32 (*getLocalsNumber)(struct _ILMethod *method);
    JITUINT32 (*getState)(struct _ILMethod *method);
    JITUINT32 (*getGlobalState)(struct _ILMethod *method);
    JITUINT32 (*getInstructionsNumber)(struct _ILMethod *method);
    void (*setResultType)(struct _ILMethod *method, JITUINT32 value);
    JITUINT32 (*getMaxVariables)(struct _ILMethod *method);
    t_jit_function * (*getJITFunction)(struct _ILMethod *method);
    void *                          (*getFunctionPointer)(struct _ILMethod *method);
    ir_symbol_t     *                                       (*getFunctionPointerSymbol)(struct _ILMethod *method);
    JITINT8 *                       (*getName)(struct _ILMethod *method);
    JITINT8 *                       (*getFullName)(struct _ILMethod *method);
    JITINT16 (*hasCatcher)(struct _ILMethod *method);
    void (*setTryBlocks)(struct _ILMethod *method, XanList *blocks);
    void (*setState)(struct _ILMethod *method, JITUINT32 state);
    void (*setGlobalState)(struct _ILMethod *method, JITUINT32 state);
    void			(*waitTillGlobalState)(struct _ILMethod *method, JITUINT32 state);
    void (*setID)(struct _ILMethod *method, MethodDescriptor *ID);
    XanList *                       (*getLocals)(struct _ILMethod *method);
    JITUINT32 (*incMaxVariablesReturnOld)(struct _ILMethod *method);
    void (*increaseMaxVariables)(struct _ILMethod *method);

    ir_local_t	*	(*insertLocal)			(struct _ILMethod *method, JITUINT32 internal_type, TypeDescriptor *type);	/**< Insert to the end of the list the new local	*/
    ir_instruction_t*	(*newIRInstr)			(struct _ILMethod *method);							/**< Insert to the end of the list the new IR instruction	*/
    ir_instruction_t*	(*newIRInstrBefore)		(struct _ILMethod *method, ir_instruction_t *beforeInst);			/**< Insert the new instruction before the beforeInst instruction	*/
    JITBOOLEAN		(*insertAnHandler)		(struct _ILMethod *method, t_try_block *block, t_try_handler *handler);		/**< Insert a new handler for the try-block identified by the block pointer	*/
    t_try_block	*	(*findTheOwnerTryBlock)		(struct _ILMethod *method, t_try_block *parent, JITUINT32 offset);		/**< find which protected try-block surrounds the address identified by `offset`. If the offset is a valid address for an handler, the method returns the t_try_block pointer that owns this handler	*/
    t_try_handler	*	(*findOwnerHandler)		(t_try_block * parent_block, JITUINT32 offset);
    JITBOOLEAN		(*isAnInnerTryBlock)		(t_try_block *parent, t_try_block *target);
    ir_instruction_t*	(*getIRInstructionFromPC)	(struct _ILMethod *method, JITNUINT pc);					/**< Return the IR instruction from the program counter	*/
    JITINT32		(*isCilImplemented)		(struct _ILMethod *method);							/**< Return 1 if the method has a body in CIL code, 0 otherwise	*/

    /**
     * Check if this method contains a body in IR language
     *
     * Return JITTRUE if this method body is implemented in IR language.
     * Currently such methods are CIL methods and auto generated delegates
     * methods. If called on abstract or native methods this method will
     * return JITFALSE.
     *
     * @param self like this pointer in OO languages
     *
     * @return JITTRUE if and only if this method body is implemented in IR
     *         language
     */
    JITBOOLEAN (*isIrImplemented)(struct _ILMethod *self);

    /**
     * Check if this method is provided by the runtime
     *
     * @param self like this pointer in OO languages
     *
     * @return JITTRUE if and only if this method is provided by the
     *         runtime, JITFALSE otherwise
     */
    JITBOOLEAN (*isRuntimeManaged)(struct _ILMethod *self);
    JITBOOLEAN (*isCctor)(struct _ILMethod *method);                                /**< Return 1 if the method is a cctor method, 0 otherwise		*/
    JITBOOLEAN (*isFinalizer)(struct _ILMethod *method);                            /**< Return 1 if the method is Finalizer method, 0 otherwise		*/
    JITBOOLEAN (*isGeneric)(struct _ILMethod *method);                              /**< Return 1 if the method is Generic method, 0 otherwise		*/
    /**
     * Check whether self method call must trigger a conservative .cctor
     * invocation
     *
     * @param self a method
     *
     * @return whether a conservative .cctor call must be triggered by self
     *         method invocation
     */
    JITBOOLEAN (*requireConservativeTypeInitialization)(struct _ILMethod* self);

    /**
     * Check whether self method is application entry point
     *
     * @param self a method
     *
     * @return JITTRUE if self is the application entry point, JITFALSE
     *                 otherwise
     */
    JITBOOLEAN (*isEntryPoint)(struct _ILMethod* self);

    /**
     * Check whether self is a .ctor
     *
     * @return JITTRUE if self is a .ctor, JITFALSE otherwise
     */
    JITBOOLEAN (*isCtor)(struct _ILMethod* self);

    /**
     * Check whether self is static
     *
     * @return JITTRUE if self is static, JITFALSE otherwise
     */
    JITBOOLEAN (*isStatic)(struct _ILMethod* self);
    void (*destroyMethod)(struct _ILMethod *self);
    void (*lock)(struct _ILMethod *method);                                                 /**< Lock the method		*/
    void (*unlock)(struct _ILMethod *method);                                               /**< Unlock the method		*/
} _ILMethod;

typedef struct _ILMethod *      Method;

void initializeMethod (Method self);

#endif
