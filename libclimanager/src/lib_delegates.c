/*
 * Copyright (C) 2008 - 2009 Simone Campanoni
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
#include <platform_API.h>
#include <error_codes.h>
#include <compiler_memory_manager.h>

// My headers
#include <lib_delegates.h>
#include <climanager_system.h>
// End

/* Constructor and destructor prototypes */
void destroyDelegatesManager (t_delegatesManager* self);

/* Public methods prototypes */
static inline void dm_buildMethod (t_delegatesManager* self, Method method);
static inline TypeDescriptor* dm_getMulticastDelegate (t_delegatesManager* self);
static TypeDescriptor* dm_getAsyncResult (t_delegatesManager* self);
static inline Method dm_getMDSCtor (t_delegatesManager* self);
static inline Method dm_getMDCtor (t_delegatesManager* self);
static Method dm_getARCtor (t_delegatesManager* self);
static inline Method dm_getDynamicInvoke (t_delegatesManager* self);
static Method dm_getEndInvoke (t_delegatesManager* self);
static MethodDescriptor* dm_getCalleeMethodID (t_delegatesManager* self, void* delegate);

/* Private methods prototypes */
static inline void dm_buildCtor (t_delegatesManager* self, Method method);
static inline void dm_buildInvoke (t_delegatesManager* self, Method method);
static inline void dm_buildBeginInvoke (t_delegatesManager* self, Method method);
static inline void dm_buildEndInvoke (t_delegatesManager* self, Method method);
static void dm_generateCopyArguments (t_delegatesManager* self, Method method);
static JITUINT32 dm_generateArgumentPack (t_delegatesManager* self, Method method, JITUINT32 count);
static void dm_generateCallResultUnbox (t_delegatesManager* self, Method method, ir_item_t* returnInfos, TypeDescriptor* returnILType);
static JITBOOLEAN dm_getCallReturnValue (t_delegatesManager* self, Method method, ir_item_t* returnInfos, TypeDescriptor** returnILType);
static void* dm_getMethodHandle (t_delegatesManager* self, void* delegate);
static inline void internal_delegates_check_metadata (void);
static inline void internal_lib_delegates_initialize (void);

pthread_once_t delegatesMetadataLock = PTHREAD_ONCE_INIT;

/* Constructor */
void init_delegatesManager (t_delegatesManager *self, IR_ITEM_VALUE buildDelegateFunctionPointer) {

    /* Setup methods links */
    self->buildMethod = dm_buildMethod;
    self->getMulticastDelegate = dm_getMulticastDelegate;
    self->getAsyncResult = dm_getAsyncResult;
    self->getMDSCtor = dm_getMDSCtor;
    self->getMDCtor = dm_getMDCtor;
    self->getARCtor = dm_getARCtor;
    self->getDynamicInvoke = dm_getDynamicInvoke;
    self->getEndInvoke = dm_getEndInvoke;
    self->getCalleeMethod = dm_getCalleeMethodID;
    self->initialize = internal_lib_delegates_initialize;
    self->destroy = destroyDelegatesManager;

    /* Setup the function pointer */
    self->buildDelegateFunctionPointer = buildDelegateFunctionPointer;

    /* Return			*/
    return;
}

extern CLIManager_t *cliManager;

/* Destructor */
void destroyDelegatesManager (t_delegatesManager* self) {

}

/* Build given method body */
static inline void dm_buildMethod (t_delegatesManager* self, Method method) {
    /* Method unqualified name */
    JITINT8* name;

    /* The self pointer is critical */
    assert(self != NULL);

    /* Invoke a custom function for build methods */
    name = method->getName(method);
    if (STRCMP((JITINT8 *) ".ctor", name) == 0) {
        dm_buildCtor(self, method);
    } else if (STRCMP((JITINT8 *) "Invoke", name) == 0) {
        dm_buildInvoke(self, method);
    } else if (STRCMP((JITINT8 *) "BeginInvoke", name) == 0) {
        dm_buildBeginInvoke(self, method);
    } else if (STRCMP((JITINT8 *) "EndInvoke", name) == 0) {
        dm_buildEndInvoke(self, method);
    } else {
        print_err("Given method isn't one of delegates framework", 0);
        abort();
    }

    method->lock(method);
    method->setState(method, IR_STATE);
    method->unlock(method);

    /* Dump method, if needed */
    //system->dumpIrMethod(system, method, NULL); FIXME
}

/* Get the System.DelegatesManager type */
static inline TypeDescriptor* dm_getMulticastDelegate (t_delegatesManager* self) {

    /* Load data if not yet done */
    PLATFORM_pthread_once(&delegatesMetadataLock, internal_delegates_check_metadata);

    return self->mdType;
}

/* Get the System.Runtime.Remoting.Messaging.AsyncResult type */
static TypeDescriptor* dm_getAsyncResult (t_delegatesManager* self) {

    /* Load data if not yet done */
    PLATFORM_pthread_once(&delegatesMetadataLock, internal_delegates_check_metadata);

    return self->arType;
}

/* Get System.MulticastDelegate constructor for static delegates */
static inline Method dm_getMDSCtor (t_delegatesManager* self) {

    /* Load data if not yet done */
    PLATFORM_pthread_once(&delegatesMetadataLock, internal_delegates_check_metadata);

    return self->mdSCtor;
}

/* Get System.MulticastDelegate constructor for instance delegates */
static inline Method dm_getMDCtor (t_delegatesManager* self) {

    /* Load data if not yet done */
    PLATFORM_pthread_once(&delegatesMetadataLock, internal_delegates_check_metadata);

    return self->mdCtor;
}

static Method dm_getARCtor (t_delegatesManager* self) {

    /* Load data if not yet done */
    PLATFORM_pthread_once(&delegatesMetadataLock, internal_delegates_check_metadata);

    return self->arCtor;
}

/* Get System.MulticastDelegate.DynamicInvoke method */
static inline Method dm_getDynamicInvoke (t_delegatesManager* self) {

    /* Load data if not yet done */
    PLATFORM_pthread_once(&delegatesMetadataLock, internal_delegates_check_metadata);

    /* Return				*/
    return self->dynamicInvoke;
}

/* Get System.Runtime.Remoting.Messaging.AsyncResult.EndInvoke method */
static Method dm_getEndInvoke (t_delegatesManager* self) {

    /* Load data if not yet done */
    PLATFORM_pthread_once(&delegatesMetadataLock, internal_delegates_check_metadata);

    return self->endInvoke;

}

/* Gets the description of the wrapped method */
static MethodDescriptor* dm_getCalleeMethodID (t_delegatesManager* self, void* delegate) {
    RuntimeHandleManager* rhm;
    void* methodHandle;
    CLRMethod* method;

    rhm = &((cliManager->CLR).runtimeHandleManager);

    methodHandle = dm_getMethodHandle(self, delegate);
    method = (CLRMethod*) rhm->getRuntimeMethodHandleValue(rhm, methodHandle);

    return method->ID;
}

/* Build the delegate constructor */
static inline void dm_buildCtor (t_delegatesManager* self, Method method) {

    /* Parameters type */
    TypeDescriptor *type;

    /* A generic IR instruction */
    ir_instruction_t* instruction;

    /* A IR instruction parameters */
    XanList* parameters;

    /* A method call parameter */
    ir_item_t* parameter;

    /* We expect a constructor with three arguments (The first is this) */
    if (method->getParametersNumber(method) != 3) {
        print_err("Constructor doesn't have the expected parameters number", 0);
        abort();
    }

    /* First IL argument must be an Object */
    type = method->getParameterILType(method, 1);
    TypeDescriptor *objectType = cliManager->metadataManager->getTypeDescriptorFromElementType(cliManager->metadataManager, ELEMENT_TYPE_OBJECT);
    if (type != objectType) {
        print_err("Malformed delegate constructor found. First parameter must be of type System.Object", 0);
        abort();
    }

    /* Second IL argument must be a System.IntPtr */
    type = method->getParameterILType(method, 2);
    TypeDescriptor *intPtrType = cliManager->metadataManager->getTypeDescriptorFromElementType(cliManager->metadataManager, ELEMENT_TYPE_I);
    if (type != intPtrType) {
        print_err("Malformed delegate constructor found. Second parameter must be of type System.IntPtr", 0);
        abort();
    }

    /*
     * Reserve space for parameters and local variables. We always have 3
     * parameters (just checked). We also copy each parameter in a local
     * variable, so the variables count is 6
     */
    ir_local_t *this = method->insertLocal(method, IROBJECT, method->getType(method));
    ir_local_t *target = method->insertLocal(method, IROBJECT, objectType);
    ir_local_t *ftnptr = method->insertLocal(method, IRNINT, intPtrType);

    /* Copy this on a new variable */
    instruction = method->newIRInstr(method);
    instruction->type = IRMOVE;
    (instruction->result).value.v = this->var_number;
    instruction->result.type = IROFFSET;
    instruction->result.internal_type = IROBJECT;
    (instruction->param_1).value.v = 0;
    instruction->param_1.type = IROFFSET;
    instruction->param_1.internal_type = IROBJECT;

    /* Copy target on a new variable */
    instruction = method->newIRInstr(method);
    instruction->type = IRMOVE;
    (instruction->result).value.v = target->var_number;
    instruction->result.type = IROFFSET;
    instruction->result.internal_type = IROBJECT;
    (instruction->param_1).value.v = 1;
    instruction->param_1.type = IROFFSET;
    instruction->param_1.internal_type = IROBJECT;

    /* Copy method on a new variable */
    instruction = method->newIRInstr(method);
    instruction->type = IRMOVE;
    (instruction->result).value.v = ftnptr->var_number;
    instruction->result.type = IROFFSET;
    instruction->result.internal_type = IRNINT;
    (instruction->param_1).value.v = 2;
    instruction->param_1.type = IROFFSET;
    instruction->param_1.internal_type = IRNINT;

    /* Call constructor implementation in native code */
    instruction = method->newIRInstr(method);
    instruction->type = IRNATIVECALL;
    (instruction->param_3).value.v = (cliManager->CLR).delegatesManager.buildDelegateFunctionPointer;
    (instruction->param_3).type = IRSYMBOL;
    (instruction->param_3).internal_type = IRFPOINTER;

    /* First parameter is the callee method return value */
    (instruction->param_1).value.v = IRVOID;
    instruction->param_1.type = IRTYPE;
    instruction->param_1.internal_type = IRTYPE;

    /* Second is callee method name */
    (instruction->param_2).value.v = (JITNUINT) "BuildDelegate";
    instruction->param_2.type = IRSTRING;
    instruction->param_2.internal_type = IRSTRING;

    /* Setup function call parameters, the first is this pointer */
    parameters = xanList_new(sharedAllocFunction, freeFunction, NULL);
    parameter = sharedAllocFunction(sizeof(ir_item_t));
    (parameter->value).v = this->var_number;
    parameter->type = IROFFSET;
    parameter->internal_type = IROBJECT;
    xanList_append(parameters, parameter);

    /* The second is target */
    parameter = sharedAllocFunction(sizeof(ir_item_t));
    (parameter->value).v = target->var_number;
    parameter->type = IROFFSET;
    parameter->internal_type = IROBJECT;
    xanList_append(parameters, parameter);

    /* The third is the delegate managed method */
    parameter = sharedAllocFunction(sizeof(ir_item_t));
    (parameter->value).v = ftnptr->var_number;
    parameter->type = IROFFSET;
    parameter->internal_type = IRNINT;
    xanList_append(parameters, parameter);

    /* Save parameters into call instruction */
    instruction->callParameters = parameters;

    /* Set result type */
    instruction->result.type = NOPARAM;

    /* At last place return instruction */
    instruction = method->newIRInstr(method);
    instruction->type = IRRET;
}

/* Build the Invoke method, it performs a synchronous call */
static inline void dm_buildInvoke (t_delegatesManager* self, Method method) {
    /* Number of input parameters */
    JITUINT32 paramCount;

    /* Index of the variable that reference the parameters array */
    JITUINT32 arrayVariableID;

    /* A generic IR instruction */
    ir_instruction_t* instruction;

    /* List of System.MulticastDelegate.DynamicInvoke parameters */
    XanList* parameters;

    /* A function call parameter */
    ir_item_t* parameter;

    /* Return value IL type */
    TypeDescriptor *returnILType;

    /* Return value infos */
    ir_item_t resultInfos;

    /* Setted to JITTRUE if called method return something */
    JITBOOLEAN somethingToReturn;

    /* We expect at least the this pointer */
    paramCount = method->getParametersNumber(method);
    if (paramCount < 1) {
        print_err("Malformed delegate Invoke method found. Invalid parameters number", 0);
        abort();
    }

    /* Prepare arguments to perform a call to mscorlib */
    dm_generateCopyArguments(self, method);
    arrayVariableID = dm_generateArgumentPack(self, method, paramCount - 1);

    /*
     * Generate an instruction to invoke
     * System.MulticastDelegate.DynamicInvoke to perform method call with
     * reflection library
     */
    instruction = method->newIRInstr(method);
    instruction->type = IRCALL;

    /* First parameter is the method to call */
    (instruction->param_1).value.v = (JITNUINT) self->getDynamicInvoke(self);
    instruction->param_1.type = IRMETHODID;
    instruction->param_1.internal_type = IRMETHODID;

    /* Second is flags */
    (instruction->param_2).value.v = 0;
    instruction->param_2.type = IRINT32;
    instruction->param_2.internal_type = IRINT32;

    /* Setup function call parameter, the first is this pointer */
    parameters = xanList_new(sharedAllocFunction, freeFunction, NULL);
    parameter = sharedAllocFunction(sizeof(ir_item_t));
    (parameter->value).v = paramCount;
    parameter->type = IROFFSET;
    parameter->internal_type = IROBJECT;
    xanList_append(parameters, parameter);

    /* The second is the parameters array */
    parameter = sharedAllocFunction(sizeof(ir_item_t));
    (parameter->value).v = arrayVariableID;
    parameter->type = IROFFSET;
    parameter->internal_type = IROBJECT;
    xanList_append(parameters, parameter);

    /* Save parameters into call instruction */
    instruction->callParameters = parameters;

    /* Set instruction return infos */
    somethingToReturn = dm_getCallReturnValue(self, method, &resultInfos, &returnILType);
    instruction->result = resultInfos;
    assert(((instruction->result).type == IROFFSET) || ((instruction->result).type == (instruction->result).internal_type));

    /* Check if we must unbox the return value */
    dm_generateCallResultUnbox(self, method, &resultInfos, returnILType);

    /* Place return instruction */
    instruction = method->newIRInstr(method);
    instruction->type = IRRET;

    /* Remember to add data to return, if any */
    if (somethingToReturn) {
        instruction->param_1 = resultInfos;
    }

}

/* Build the BeginInvoke method, it starts a asynchronous call */
static inline void dm_buildBeginInvoke (t_delegatesManager* self, Method method) {

    /* Number of parameters */
    JITUINT32 paramCount;


    /* Description of the AsyncResult class */
    TypeDescriptor* asyncResultType;

    /* Index of the variable that reference the parameters array */
    JITUINT32 arrayVariableID;

    /* A generic IR instruction */
    ir_instruction_t* instruction;

    /* A ctor call parameter */
    ir_item_t* parameter;

    /* Stores ctor parameters */
    XanList* parameters;

    /* Return value infos */
    ir_item_t resultInfos;

    /*
     * We expect at least the this pointer, a callback and a state
     * variable.
     */
    paramCount = method->getParametersNumber(method);
    if (paramCount < 3) {
        print_err("Malformed delegate BeginInvoke method found. Invalid parameters number", 0);
        abort();
    }


    /* Get the AsyncResult description */
    asyncResultType = self->getAsyncResult(self);

    /* Prepare arguments to perform a call to mscorlib */
    dm_generateCopyArguments(self, method);
    arrayVariableID = dm_generateArgumentPack(self, method, paramCount - 1 - 2);

    /* Create a new System.Runtime.Remoting.Messaging.AsyncResult */
    instruction = method->newIRInstr(method);
    instruction->type = IRNEWOBJ;
    (instruction->param_1).value.v = (JITNUINT) asyncResultType;
    instruction->param_1.type = IRCLASSID;
    instruction->param_1.internal_type = IRCLASSID;
    (instruction->param_2).value.v = 0;
    instruction->param_2.type = IRUINT32;
    instruction->param_2.internal_type = IRUINT32;
    (instruction->result).value.v = method->incMaxVariablesReturnOld(method);
    instruction->result.type = IROFFSET;
    instruction->result.internal_type = IROBJECT;

    /* Save the new return variable, so the return instruction can use it */
    resultInfos = instruction->result;

    /* Call the constructor */
    instruction = method->newIRInstr(method);
    instruction->type = IRCALL;

    /* First parameter is the method to call */
    (instruction->param_1).value.v = (JITNUINT) self->getARCtor(self);
    instruction->param_1.type = IRMETHODID;
    instruction->param_1.internal_type = IRMETHODID;

    /* Second is flag */
    (instruction->param_2).value.v = 0;
    instruction->param_2.type = IRINT32;
    instruction->param_2.internal_type = IRINT32;

    /* Setup ctor call parameter, the first is the delegate */
    parameters = xanList_new(sharedAllocFunction, freeFunction, NULL);
    parameter = sharedAllocFunction(sizeof(ir_item_t));
    *parameter = resultInfos;
    xanList_append(parameters, parameter);

    /* The second is the delegate */
    parameter = sharedAllocFunction(sizeof(ir_item_t));
    (parameter->value).v = paramCount;
    parameter->type = IROFFSET;
    parameter->internal_type = IROBJECT;
    xanList_append(parameters, parameter);

    /* The third is the parameters array */
    parameter = sharedAllocFunction(sizeof(ir_item_t));
    (parameter->value).v = arrayVariableID;
    parameter->type = IROFFSET;
    parameter->internal_type = IROBJECT;
    xanList_append(parameters, parameter);

    /* The fourth is the callback */
    parameter = sharedAllocFunction(sizeof(ir_item_t));
    (parameter->value).v = paramCount - 2;
    parameter->type = IROFFSET;
    parameter->internal_type = IROBJECT;
    xanList_append(parameters, parameter);

    /* The fifth is the call state */
    parameter = allocFunction(sizeof(ir_item_t));
    (parameter->value).v = paramCount - 1;
    parameter->type = IROFFSET;
    parameter->internal_type = IROBJECT;
    xanList_append(parameters, parameter);

    /* Save parameters into call instruction */
    instruction->callParameters = parameters;

    /* Ctors have no return value */
    (instruction->result).type = IRVOID;
    (instruction->result).internal_type	= (instruction->result).type;

    /* Place return instruction to return the future */
    instruction = method->newIRInstr(method);
    instruction->type = IRRET;
    instruction->param_1 = resultInfos;

    return ;
}

/* Build the EndInvoke method, it waits for an asynchronous call end */
static inline void dm_buildEndInvoke (t_delegatesManager* self, Method method) {

    /* Number of parameters */
    JITUINT32 paramCount;


    /* Index of the variable that reference the parameters array */
    JITUINT32 arrayVariableID;

    /* A generic IR instruction */
    ir_instruction_t* instruction;

    /* Stores EndInvoke parameters */
    XanList* parameters;

    /* An EndInvoke call parameter */
    ir_item_t* parameter;

    /* Setted to JITTRUE if called method return something */
    JITBOOLEAN somethingToReturn;

    /* Return value infos */
    ir_item_t resultInfos;

    /* Return value IL type */
    TypeDescriptor *returnILType;

    /*
     * We expect at least the this pointer and the future
     */
    paramCount = method->getParametersNumber(method);
    if (paramCount < 2) {
        print_err("Malformed delegate EndInvoke method found. Invalid parameters number", 0);
        abort();
    }



    /* Prepare arguments to perform a call to mscorlib */
    dm_generateCopyArguments(self, method);
    arrayVariableID = dm_generateArgumentPack(self, method, paramCount - 1 - 1);

    /*
     * Generate an instruction to invoke
     * System.Runtime.Remoting.Messaging.AsyncResult.EndInvoke.
     */
    instruction = method->newIRInstr(method);
    instruction->type = IRCALL;

    /* First parameter is the method to call */
    (instruction->param_1).value.v = (JITNUINT) self->getEndInvoke(self);
    instruction->param_1.type = IRMETHODID;
    instruction->param_1.internal_type = IRMETHODID;

    /* Second is flags */
    (instruction->param_2).value.v = 0;
    instruction->param_2.type = IRINT32;
    instruction->param_2.internal_type = IRINT32;

    /* Setup function call parameters, the first is the future pointer */
    parameters = xanList_new(sharedAllocFunction, freeFunction, NULL);
    parameter = sharedAllocFunction(sizeof(ir_item_t));
    (parameter->value).v = paramCount - 1;
    parameter->type = IROFFSET;
    parameter->internal_type = IROBJECT;
    xanList_append(parameters, parameter);

    /* The second is the output arguments array */
    parameter = sharedAllocFunction(sizeof(ir_item_t));
    (parameter->value).v = arrayVariableID;
    parameter->type = IROFFSET;
    parameter->internal_type = IROBJECT;
    xanList_append(parameters, parameter);

    /* Save parameters into call instruction */
    instruction->callParameters = parameters;

    /* Set instruction return infos */
    somethingToReturn = dm_getCallReturnValue(self, method, &resultInfos, &returnILType);
    instruction->result = resultInfos;

    /* Check if we must unbox the return value */
    dm_generateCallResultUnbox(self, method, &resultInfos, returnILType);

    /* Place return instruction */
    instruction = method->newIRInstr(method);
    instruction->type = IRRET;

    /* Remember to add data to return, if any */
    if (somethingToReturn) {
        instruction->param_1 = resultInfos;
    }

}

/* Copy input parameters into local variables */
static void dm_generateCopyArguments (t_delegatesManager* self, Method method) {
    /* Number of parameters */
    JITUINT32 paramCount;

    /* Local variable IL type */
    TypeDescriptor *localILType;

    /* Local variable IR type */
    JITUINT32 localIRType;

    /* A generic IR instruction */
    ir_instruction_t* instruction;
    ir_method_t *irMethod;

    /* Loop counter */
    JITNUINT i;

    /* Cache some pointers */
    irMethod = method->getIRMethod(method);

    /* Get the number of parameters */
    paramCount = method->getParametersNumber(method);

    /* Copy this pointer */
    localIRType = IRMETHOD_getParameterType(irMethod, 0);
    instruction = method->newIRInstr(method);
    instruction->type = IRMOVE;
    (instruction->result).value.v = method->incMaxVariablesReturnOld(method);
    instruction->result.type = IROFFSET;
    instruction->result.internal_type = localIRType;
    (instruction->param_1).value.v = 0;
    instruction->param_1.type = IROFFSET;
    instruction->param_1.internal_type = localIRType;

    /*
     * Declare a local variable for each parameter, then generate the IR
     * instruction to copy parameter into it. Instruction first parameter is
     * the destination variable, second parameter is the source variable
     */
    for (i = 1; i < paramCount; i++) {

        /* Make the local */
        localILType = method->getParameterILType(method, i);
        localIRType = IRMETHOD_getParameterType(irMethod, i);

        /* The parameter is a value type? */
        if (localILType->isValueType(localILType)) {

            JITUINT32 result_var = method->incMaxVariablesReturnOld(method);

            /* First build the box */
            instruction = method->newIRInstr(method);
            instruction->type = IRNEWOBJ;

            /* Object class */
            (instruction->param_1).value.v = (JITNUINT) localILType;
            instruction->param_1.type = IRCLASSID;
            instruction->param_1.internal_type = IRCLASSID;

            /* Object oversize */
            (instruction->param_2).value.v = 0;
            instruction->param_2.type = IRUINT32;
            instruction->param_2.internal_type = IRUINT32;

            /* Local variable that reference the object */
            (instruction->result).value.v = result_var;
            instruction->result.type = IROFFSET;
            instruction->result.internal_type = IROBJECT;

            /* Then copy the value type into the box */
            instruction = method->newIRInstr(method);
            instruction->type = IRSTOREREL;

            /* Where to copy the value type (base address) */
            (instruction->param_1).value.v = result_var;
            instruction->param_1.type = IROFFSET;
            instruction->param_1.internal_type = IROBJECT;

            /* Where to copy the value type (offset) */
            (instruction->param_2).value.v = 0;
            instruction->param_2.type = IRINT32;
            instruction->param_2.internal_type = IRINT32;

            /* The value type to copy */
            (instruction->param_3).value.v = i;
            instruction->param_3.type = IROFFSET;
            instruction->param_3.internal_type = localIRType;
            instruction->param_3.type_infos = localILType;


            /* Nothing to return */
            instruction->result.type = NOPARAM;

            /* Given parameter isn't a value type */
        } else {

            /* Copy the reference on a local variable */
            instruction = method->newIRInstr(method);
            instruction->type = IRMOVE;
            (instruction->result).value.v = method->incMaxVariablesReturnOld(method);
            instruction->result.type = IROFFSET;
            instruction->result.internal_type = localIRType;
            (instruction->param_1).value.v = i;
            instruction->param_1.type = IROFFSET;
            instruction->param_1.internal_type = localIRType;
        }
    }
}

/* Pack count arguments into an array */
static JITUINT32 dm_generateArgumentPack (t_delegatesManager* self, Method method, JITUINT32 count) {
    ir_method_t *irMethod;

    /* Index of the variable that reference the parameters array */
    JITUINT32 arrayVariableID;

    /* Local variable IR type */
    JITUINT32 localIRType;

    /* System.Object class description */
    TypeDescriptor* objectType;


    /* A generic IR instruction */
    ir_instruction_t* instruction;

    /* Loop counter */
    JITNUINT i;

    /* Number of parameters */
    JITUINT32 paramCount;

    /* Cache some pointers */
    irMethod = method->getIRMethod(method);

    /* Get the number of parameters */
    paramCount = method->getParametersNumber(method);

    /* Declare a local variable to store an array reference */
    localIRType = IROBJECT;
    arrayVariableID = method->incMaxVariablesReturnOld(method);

    /* Retrieve the System.Object class description */
    objectType = cliManager->metadataManager->getTypeDescriptorFromElementType(cliManager->metadataManager, ELEMENT_TYPE_OBJECT);

    /*
     * Build a new array of System.Object.
     */
    instruction = method->newIRInstr(method);
    instruction->type = IRNEWARR;
    (instruction->param_1).value.v = (JITNUINT) objectType;
    instruction->param_1.type = IRCLASSID;
    instruction->param_1.internal_type = IRCLASSID;
    (instruction->param_2).value.v = count;
    instruction->param_2.type = IRINT32;
    instruction->param_2.internal_type = IRINT32;
    (instruction->result).value.v = arrayVariableID;
    instruction->result.type = IROFFSET;
    instruction->result.internal_type = IROBJECT;

    /*
     * Generate instructions to copy each parameter into the array. We must
     * copy i-th local variable (a copy of the i-th argument, instruction
     * third parameter) into array (i-1)-th element (instruction second
     * parameter). Don't copy the this pointer (local variable 0)
     */
    for (i = 1; i <= count; i++) {
        localIRType = IRMETHOD_getParameterType(irMethod, i);
        instruction = method->newIRInstr(method);
        instruction->type = IRSTOREREL;
        (instruction->param_3).value.v = paramCount + i;
        instruction->param_3.type = IROFFSET;
        instruction->param_3.internal_type = localIRType;
        instruction->param_3.type_infos = IRMETHOD_getParameterILType(irMethod, i);
        (instruction->param_1).value.v = arrayVariableID;
        instruction->param_1.type = IROFFSET;
        instruction->param_1.internal_type = IROBJECT;
        (instruction->param_2).value.v = (i - 1) * IRDATA_getSizeOfType(&(instruction->param_3));
        instruction->param_2.type = IRINT32;
        instruction->param_2.internal_type = IRINT32;
        instruction->result.type = NOPARAM;
    }

    return arrayVariableID;
}

/* Eventually generates instruction to unbox a call result */
static void dm_generateCallResultUnbox (t_delegatesManager* self, Method method, ir_item_t* returnInfos, TypeDescriptor* returnILType) {
    /* Return value IR type */
    JITUINT32 returnIRType;

    /* This variable stores the return value */
    JITUINT32 returnVar;

    /* A generic IR instruction */
    ir_instruction_t* instruction;

    /* Gets the IR type of the call result */
    returnIRType = returnILType->getIRType(returnILType);

    /* Unbox, if needed */
    if (returnIRType != IRVOID && returnILType->isValueType(returnILType)) {

        /* Make a new variable to hold the value type */
        returnVar = method->incMaxVariablesReturnOld(method);

        /* Generate the unbox instruction */
        instruction = method->newIRInstr(method);
        instruction->type = IRLOADREL;

        /*
         * First parameter is the address of the value type to be
         * unboxed
         */
        instruction->param_1 = *returnInfos;

        /*
         * We want to read all the value type, so start reading at
         * offset 0
         */
        (instruction->param_2).value.v = 0;
        instruction->param_2.type = IRINT32;
        instruction->param_2.internal_type = IRINT32;

        /* Set return value to a local variable */
        (instruction->result).value.v = returnVar;
        instruction->result.type = IROFFSET;
        instruction->result.internal_type = returnIRType;
        instruction->result.type_infos = returnILType;

        /*
         * Save the new return variable, so the return instruction can
         * use it
         */
        *returnInfos = instruction->result;
    }
}

/* Get the return value for the given method to call */
static JITBOOLEAN dm_getCallReturnValue (t_delegatesManager* self, Method method, ir_item_t* returnInfos, TypeDescriptor** returnILType) {
    /* Return value IR type */
    JITUINT32 returnIRType;

    /* Setted to JITTRUE if called method return something */
    JITBOOLEAN somethingToReturn;

    /* This variable stores the return value */
    JITUINT32 returnVar;

    /* Get called method return type */
    TypeDescriptor *result_type = method->getReturnILType(method);

    returnIRType = result_type->getIRType(result_type);
    (*returnILType) = result_type;

    /* No return type, set it to void */
    memset(returnInfos, 0, sizeof(ir_item_t));
    if (returnIRType == IRVOID) {
        somethingToReturn = JITFALSE;
        returnInfos->type = IRVOID;
        returnInfos->internal_type	= returnInfos->type;

        /* Called method return something, get it */
    } else {
        somethingToReturn = JITTRUE;
        returnVar = method->incMaxVariablesReturnOld(method);
        (returnInfos->value).v = returnVar;
        returnInfos->type = IROFFSET;
        returnInfos->internal_type = IROBJECT;
    }

    return somethingToReturn;
}

/* Get the "method" field */
static void* dm_getMethodHandle (t_delegatesManager* self, void* delegate) {

    /* Load data if not yet done */
    PLATFORM_pthread_once(&delegatesMetadataLock, internal_delegates_check_metadata);

    return delegate + self->methodFieldOffset;
}

/* Load System.MulticastDelegate and cache some methods */
static inline void internal_delegates_check_metadata (void) {
    t_delegatesManager* self;

    /* Container of all resolved methods */
    t_methods* methods;

    /* The garbage collector */
    t_running_garbage_collector* garbageCollector;

    /* The System.Delegate type */
    TypeDescriptor* dType;

    /* The System.MulticastDelegate type */
    TypeDescriptor* mdType;

    /* The System.Runtime.Remoting.Messaging.AsyncResult type */
    TypeDescriptor* arType;

    /* The System.Type type */
    TypeDescriptor *typeType;

    /* System.MulticastDelegate constructor identifier */
    MethodDescriptor *mdCtorID;

    /* System.MulticastDelegate constructor */
    Method mdCtor;

    /* System.Runtime.Remoting.Messaging.AsyncResult ctor identifier */
    MethodDescriptor *arCtorID;

    /* System.Runtime.Remoting.Messaging.AsyncResult constructor */
    Method arCtor;

    /* The method field of System.Delegate */
    FieldDescriptor *methodField;

    /* Constructors counter */
    JITNUINT ctorCount;

    /* System.MulticastDelegate.DynamicInvokeImpl identifier */
    MethodDescriptor *dynamicInvokeImplID;

    /* System.MulticastDelegate.DynamicInvokeImpl method */
    Method dynamicInvokeImpl;

    /* System.Runtime.Remoting.Messaging.AsyncResult.EndInvoke identifier */
    MethodDescriptor *endInvokeID;

    /* System.Runtime.Remoting.Messaging.AsyncResult.EndInvoke */
    Method endInvoke;

    XanList *methods_list;
    XanListItem *item;

    /* Cache some data */
    self = &((cliManager->CLR).delegatesManager);
    assert(self != NULL);
    methods = &(cliManager->methods);
    assert(methods != NULL);
    garbageCollector = cliManager->gc;

    /* Locate System.Delegate class */
    dType = cliManager->metadataManager->getTypeDescriptorFromName(cliManager->metadataManager, (JITINT8 *) "System", (JITINT8 *) "Delegate");
    if (dType == NULL) {
        print_err("Unable to find System.Delegate type", 0);
        abort();
    }

    /* Locate System.MulticastDelegate class */
    mdType = cliManager->metadataManager->getTypeDescriptorFromName(cliManager->metadataManager, (JITINT8 *) "System", (JITINT8 *) "MulticastDelegate");
    if (mdType == NULL) {
        print_err("Unable to found System.MulticastDelegate type", 0);
        abort();
    }
    self->mdType = mdType;

    /* Locate System.Runtime.Remoting.Messaging.AsyncResult class */
    arType = cliManager->metadataManager->getTypeDescriptorFromName(cliManager->metadataManager, (JITINT8 *) "System.Runtime.Remoting.Messaging", (JITINT8 *) "AsyncResult");
    if (arType == NULL) {
        print_err("Unable to find the System.Runtime.Remoting.Messaging.AsyncResult type", 0);
        abort();
    }
    self->arType = arType;

    /* Locate System.Type class */
    typeType = cliManager->metadataManager->getTypeDescriptorFromName(cliManager->metadataManager, (JITINT8 *) "System", (JITINT8 *) "Type");
    if (typeType == NULL) {
        print_err("Unable to found System.Type type", 0);
        abort();
    }

    /* Get System.Delegate method field */
    methodField = dType->getFieldFromName(dType, (JITINT8 *) "method");
    self->methodFieldOffset = garbageCollector->fetchFieldOffset(methodField);

    /* Get System.MulticastDelegate constructors */
    mdCtorID = NULL;
    ctorCount = 0;
    methods_list = mdType->getConstructors(mdType);
    item = xanList_first(methods_list);
    TypeDescriptor *objectType = cliManager->metadataManager->getTypeDescriptorFromElementType(cliManager->metadataManager, ELEMENT_TYPE_OBJECT);
    while (item != NULL) {
        mdCtorID = (MethodDescriptor *) item->data;
        mdCtor = methods->fetchOrCreateMethod(methods, mdCtorID, JITFALSE);
        TypeDescriptor *paramType = mdCtor->getParameterILType(mdCtor, 1);
        if (paramType->isSubtypeOf(paramType, typeType) ) {
            self->mdSCtor = mdCtor;
            ctorCount++;
        } else if (paramType == objectType) {
            self->mdCtor = mdCtor;
            ctorCount++;
        }
        if (ctorCount == 2) {
            break;
        }
        item = item->next;
    }

    /* Get System.Runtime.Remoting.Messaging.AsyncResult */
    methods_list = arType->getConstructors(arType);
    item = xanList_first(methods_list);
    arCtorID = (MethodDescriptor *) item->data;
    arCtor = methods->fetchOrCreateMethod(methods, arCtorID, JITFALSE);
    self->arCtor = arCtor;

    /*
     * Get System.MulticastDelegate.DynamicInvokeImpl. It is the
     * implementation of System.MulticastDelegate.DynamicInvokeImpl;
     * don't show it to the caller
     */
    methods_list = mdType->getMethodFromName(mdType, (JITINT8 *) "DynamicInvokeImpl");
    item = xanList_first(methods_list);
    dynamicInvokeImplID = (MethodDescriptor *) item->data;
    dynamicInvokeImpl = methods->fetchOrCreateMethod(methods, dynamicInvokeImplID, JITTRUE);
    self->dynamicInvoke = dynamicInvokeImpl;

    /* Tag the method as callable externally.
     */
    IRMETHOD_setMethodAsCallableExternally(&(self->dynamicInvoke->IRMethod), JITTRUE);

    /* Get System.Runtime.Remoting.Messaging.AsyncResult.EndInvoke */
    methods_list = mdType->getMethodFromName(arType, (JITINT8 *) "EndInvoke");
    item = xanList_first(methods_list);
    endInvokeID = (MethodDescriptor *) item->data;
    endInvoke = methods->fetchOrCreateMethod(methods, endInvokeID, JITFALSE);
    self->endInvoke = endInvoke;

    /* Tag the method as callable externally.
     */
    IRMETHOD_setMethodAsCallableExternally(&(self->endInvoke->IRMethod), JITTRUE);

    return ;
}

static inline void internal_lib_delegates_initialize (void) {
    PLATFORM_pthread_once(&delegatesMetadataLock, internal_delegates_check_metadata);
}
