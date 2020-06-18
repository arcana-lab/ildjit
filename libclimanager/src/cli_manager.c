#include <stdio.h>
#include <assert.h>
#include <compiler_memory_manager.h>
#include <base_symbol.h>
#include <platform_API.h>

// My headers
#include <cli_manager.h>
#include <ilmethod.h>
#include <config.h>
#include <climanager_system.h>
// End

#define initTempGenericContainer(_container, _container_type, _parent, _arg_count) \
	_container.container_type = _container_type; \
	_container.parent = _parent; \
	_container.arg_count = _arg_count; \
	_container.paramType = alloca(sizeof(TypeDescriptor *) * _arg_count);

CLIManager_t *cliManager;

static inline void translateActualMethodSignatureToIR (CLIManager_t *self, MethodDescriptor *method, FunctionPointerDescriptor *signature_to_convert, ir_signature_t *ir_method);
static inline JITUINT32 internaltranslateSignatureToIR (CLIManager_t *self, MethodDescriptor *method, FunctionPointerDescriptor *signature, ir_signature_t *ir_signature, JITBOOLEAN isFormal);
static inline Method internal_allocNewMethod (CLIManager_t *cliManager, JITINT8 *name);
static inline Method internal_newMethod (CLIManager_t *cliManager, MethodDescriptor *methodID, JITBOOLEAN isExternallyCallable);
static inline Method internal_newAnonymousMethod (CLIManager_t *cliManager, JITINT8 *name);
static inline ir_symbol_t * getTypeDescriptorSymbol (CLIManager_t *cliManager, TypeDescriptor *type);
static inline ir_symbol_t * getMethodDescriptorSymbol (CLIManager_t *cliManager, MethodDescriptor *method);
static inline ir_symbol_t * getFieldDescriptorSymbol (CLIManager_t *cliManager, FieldDescriptor *field);
static inline ir_symbol_t * getIndexOfTypeDescriptorSymbol (CLIManager_t *cliManager, TypeDescriptor *type);
static inline ir_symbol_t * getIndexOfMethodDescriptorSymbol (CLIManager_t *cliManager, MethodDescriptor *type);
static inline ir_symbol_t * methodDescriptorDeserialize (void *mem, JITUINT32 memBytes);
static inline ir_symbol_t * fieldDescriptorDeserialize (void *mem, JITUINT32 memBytes);
static inline ir_symbol_t *typeDescriptorDeserialize (void *mem, JITUINT32 memBytes);
static inline ir_symbol_t * indexOfTypeDescriptorDeserialize (void *mem, JITUINT32 memBytes);
static inline ir_symbol_t * IndexOfMethodDescriptorDeserialize (void *mem, JITUINT32 memBytes);
static inline XanList * findCompatibleMethods (t_methods *methods, ir_signature_t *signature);
static inline Method fetchOrCreateAnonymousMethod (t_methods *methods, JITINT8 *name);
static inline JITBOOLEAN deleteMethod (t_methods *methods, Method method);
static inline ir_value_t typeDescriptorResolve (ir_symbol_t *symbol);
static inline JITUINT32 arrayDescriptorSerialize (ArrayDescriptor *array, void **mem, JITUINT32 *memBytesAllocated, JITUINT32 bytesWritten);
static inline JITUINT32 classDescriptorSerialize (ClassDescriptor *class, void **mem, JITUINT32 *memBytesAllocated, JITUINT32 bytesWritten);
static inline JITUINT32 pointerDescriptorSerialize (PointerDescriptor *ptr, void **mem, JITUINT32 *memBytesAllocated, JITUINT32 bytesWritten);
static inline JITUINT32 functionPointerDescriptorSerialize (FunctionPointerDescriptor *ptr, void **mem, JITUINT32 *memBytesAllocated, JITUINT32 bytesWritten);
static inline TypeDescriptor *arrayDescriptorDeserialize (void *mem, JITUINT32 bytesRead);
static inline TypeDescriptor *functionPointerDescriptorDeserialize (void *mem, JITUINT32 bytesRead);
static inline TypeDescriptor *pointerDescriptorDeserialize (void *mem, JITUINT32 bytesRead);
static inline TypeDescriptor *classDescriptorDeserialize (void *mem, JITUINT32 bytesRead);
static inline void typeDescriptorSerialize (ir_symbol_t * symbol, void **mem, JITUINT32 *memBytesAllocated);
static inline void typeDescriptorDump (ir_symbol_t *symbol, FILE *fileToWrite);
static inline ir_symbol_t * getTypeDescriptorSymbol (CLIManager_t *cliManager, TypeDescriptor *type);
static inline ir_value_t methodDescriptorResolve (ir_symbol_t *symbol);
static inline void methodDescriptorSerialize (ir_symbol_t * symbol, void **mem, JITUINT32 *memBytesAllocated);
static inline void methodDescriptorDump (ir_symbol_t *symbol, FILE *fileToWrite);
static inline ir_symbol_t * getMethodDescriptorSymbol (CLIManager_t *cliManager, MethodDescriptor *method);
static inline ir_value_t fieldDescriptorResolve (ir_symbol_t *symbol);
static inline void fieldDescriptorSerialize (ir_symbol_t * symbol, void **mem, JITUINT32 *memBytesAllocated);
static inline void fieldDescriptorDump (ir_symbol_t *symbol, FILE *fileToWrite);
static inline ir_symbol_t * getFieldDescriptorSymbol (CLIManager_t *cliManager, FieldDescriptor *field);
static inline ir_value_t indexOfTypeDescriptorResolve (ir_symbol_t *symbol);
static inline void indexOfTypeDescriptorSerialize (ir_symbol_t * symbol, void **mem, JITUINT32 *memBytesAllocated);
static inline void indexOfTypeDescriptorDump (ir_symbol_t *symbol, FILE *fileToWrite);
static inline ir_symbol_t * getIndexOfTypeDescriptorSymbol (CLIManager_t *cliManager, TypeDescriptor *type);
static inline ir_value_t IndexOfMethodDescriptorResolve (ir_symbol_t *symbol);
static inline void IndexOfMethodDescriptorSerialize (ir_symbol_t * symbol, void **mem, JITUINT32 *memBytesAllocated);
static inline void IndexOfMethodDescriptorDump (ir_symbol_t *symbol, FILE *fileToWrite);
static inline ir_symbol_t * getIndexOfMethodDescriptorSymbol (CLIManager_t *cliManager, MethodDescriptor *method);
ir_value_t functionPointerResolve (ir_symbol_t *symbol);
void functionPointerSerialize (ir_symbol_t * symbol, void **mem, JITUINT32 *memBytesAllocated);
void functionPointerDump (ir_symbol_t *symbol, FILE *fileToWrite);
ir_symbol_t * getFunctionPointerSymbol (Method method);
ir_symbol_t *functionPointerDeserialize (void *mem, JITUINT32 memBytes);

void init_CLIManager (CLIManager_t *lib, IRVM_t *irVM, t_running_garbage_collector *gc, Method (*fetchOrCreateMethod)(MethodDescriptor *method), void (*throwExceptionByName)(JITINT8 *nameSpace, JITINT8 *typeName), void (*ensureCorrectJITFunction)(Method method), JITBOOLEAN runtimeChecks, IR_ITEM_VALUE buildDelegateFunctionPointer, JITINT8 *machineCodePath) {

    /* Assertions                   		*/
    assert(lib != NULL);
    assert(irVM != NULL);
    assert(gc != NULL);

    /* Store the fields		*/
    lib->IRVM = irVM;
    lib->gc = gc;
    lib->fetchOrCreateMethod = fetchOrCreateMethod;
    lib->throwExceptionByName = throwExceptionByName;
    lib->ensureCorrectJITFunction = ensureCorrectJITFunction;

    /* Attach the functions				*/
    (lib->translator).newMethod				= internal_newMethod;
    (lib->translator).newAnonymousMethod			= internal_newAnonymousMethod;
    (lib->translator).translateActualMethodSignatureToIR	= translateActualMethodSignatureToIR;
    (lib->translator).getTypeDescriptorSymbol		= getTypeDescriptorSymbol;
    (lib->translator).getFieldDescriptorSymbol		= getFieldDescriptorSymbol;
    (lib->translator).getMethodDescriptorSymbol		= getMethodDescriptorSymbol;
    (lib->translator).getIndexOfTypeDescriptorSymbol	= getIndexOfTypeDescriptorSymbol;
    (lib->translator).getIndexOfMethodDescriptorSymbol	= getIndexOfMethodDescriptorSymbol;
    (lib->methods).findCompatibleMethods			= findCompatibleMethods;
    (lib->methods).fetchOrCreateAnonymousMethod		= fetchOrCreateAnonymousMethod;
    (lib->methods).deleteMethod				= deleteMethod;

    /* Make the list of binaries loaded by the	*
     * runtime					*/
    lib->binaries = xanList_new(sharedAllocFunction, freeFunction, NULL);
    assert(lib->binaries != NULL);

    /* Initialize the hash table pairing    *
    * functions and entry points           */
    (lib->methods).functionsEntryPoint = xanHashTable_new(11, 0, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);

    cliManager = lib;

    /* setup ValueType Manager*/
    LAYOUT_initManager(&(lib->layoutManager));

    /* Setup the Metadata library	*/
    lib->metadataManager		= sharedAllocFunction(sizeof(metadataManager_t));
    init_metadataManager(lib->metadataManager, lib->binaries);

    /* Make the System.Runtime*Handle manager */
    init_runtimeHandleManager(&(lib->CLR.runtimeHandleManager));

    /* Setup the stringBuilder library	*/
    init_stringBuilderManager(&(lib->CLR.stringBuilderManager));

    /* Setup the string library		*/
    init_stringManager(&(lib->CLR.stringManager));

    /* Setup the reflection library		*/
    init_reflectManager(&(lib->CLR.reflectManager));

    /* Setup the decimal library		*/
    init_decimalManager(&(lib->CLR.decimalManager));

    /* Setup the array library		*/
    init_arrayManager(&(lib->CLR.arrayManager));

    /* Setup the typedReference library	*/
    init_typedReferenceManager(&(lib->CLR.typedReferenceManager));

    /* Setup the typedReference library	*/
    init_varargManager(&(lib->CLR.varargManager));

    /* Setup the culture library            */
    init_cultureManager(&(lib->CLR.cultureManager));

    /* Setup the network manager		*/
    init_netManager(&(lib->CLR.netManager));

    /* Make the delegates manager           */
    init_delegatesManager(&((lib->CLR).delegatesManager), buildDelegateFunctionPointer);

    /* Setup the thread library		*/
    init_threadsManager(&((lib->CLR).threadsManager));

    /* Setup the platform invokation manager	*/
    init_pinvokeManager(&(lib->CLR).pinvokeManager, machineCodePath);

    /* Set the flags			*/
    (lib->CLR).runtimeChecks = runtimeChecks;

    IRSYMBOL_registerSymbolManager(TYPE_DESCRIPTOR_SYMBOL, typeDescriptorResolve, typeDescriptorSerialize, typeDescriptorDump, typeDescriptorDeserialize);
    IRSYMBOL_registerSymbolManager(INDEX_OF_TYPE_DESCRIPTOR_SYMBOL, indexOfTypeDescriptorResolve, indexOfTypeDescriptorSerialize, indexOfTypeDescriptorDump, indexOfTypeDescriptorDeserialize);
    IRSYMBOL_registerSymbolManager(METHOD_DESCRIPTOR_SYMBOL,methodDescriptorResolve, methodDescriptorSerialize, methodDescriptorDump, methodDescriptorDeserialize);
    IRSYMBOL_registerSymbolManager(FIELD_DESCRIPTOR_SYMBOL,fieldDescriptorResolve, fieldDescriptorSerialize, fieldDescriptorDump, fieldDescriptorDeserialize);
    IRSYMBOL_registerSymbolManager(FUNCTION_POINTER_SYMBOL, functionPointerResolve, functionPointerSerialize, functionPointerDump, functionPointerDeserialize);
    IRSYMBOL_registerSymbolManager(INDEX_OF_METHOD_DESCRIPTOR_SYMBOL, IndexOfMethodDescriptorResolve, IndexOfMethodDescriptorSerialize, IndexOfMethodDescriptorDump, IndexOfMethodDescriptorDeserialize);

    return;
}

void CLIMANAGER_initializeCLR (void) {

    cliManager->metadataManager->initialize();
    (cliManager->layoutManager).initialize();
    (cliManager->CLR).arrayManager.initialize();
    (cliManager->CLR).decimalManager.initialize();
    (cliManager->CLR).reflectManager.initialize();
    (cliManager->CLR).runtimeHandleManager.initialize();
    (cliManager->CLR).stringBuilderManager.initialize();
    (cliManager->CLR).typedReferenceManager.initialize();
    (cliManager->CLR).varargManager.initialize();
    (cliManager->CLR).stringManager.initialize();
    (cliManager->CLR).netManager.initialize();
    (cliManager->CLR).cultureManager.initialize();
    (cliManager->CLR).threadsManager.initialize();
    (cliManager->CLR).delegatesManager.initialize();

    return ;
}

void CLIMANAGER_shutdown (CLIManager_t *lib) {

    /* Assertions.
     */
    assert(lib != NULL);

    /* Destroy the managers.
     */
    (lib->CLR).stringManager.destroy(&((lib->CLR).stringManager));
    (lib->CLR).reflectManager.destroy(&((lib->CLR).reflectManager));
    (lib->CLR).delegatesManager.destroy(&((lib->CLR).delegatesManager));
    (lib->CLR).threadsManager.destroy(&((lib->CLR).threadsManager));
    (lib->CLR).pinvokeManager.destroy(&((lib->CLR).pinvokeManager));

    /* Destroy the generic manager.
     */
    lib->metadataManager->shutdown(lib->metadataManager);

    /* Destroy the tables.
     */
    xanHashTable_destroyTable((lib->methods).functionsEntryPoint);

    /* Destroy the layout manager.
     */
    LAYOUT_destroyManager(&(lib->layoutManager));

    return ;
}

static inline Method internal_allocNewMethod (CLIManager_t *cliManager, JITINT8 *name) {
    Method newMethod;
    pthread_mutexattr_t mutex_attr;
    pthread_condattr_t cond_attr;

    /* Alloc memory for the new method		*/
    newMethod                                               = (Method) sharedAllocFunction(sizeof(_ILMethod));

    /* Initialize the method			*/
    initializeMethod(newMethod);

    /* Initialize the variables			*/
    newMethod->try_blocks = NULL;
    newMethod->executionProbability = -1;
    newMethod->cctorMethods = xanList_new(sharedAllocFunction, freeFunction, NULL);
    newMethod->trampolinesSet = xanList_new(sharedAllocFunction, freeFunction, NULL);

    /* Make the mutex and the condition variable	*/
    PLATFORM_initMutexAttr(&mutex_attr);
    PLATFORM_initCondVarAttr(&cond_attr);
    PLATFORM_setMutexAttr_type(&mutex_attr, PTHREAD_MUTEX_ADAPTIVE_NP);
    MEMORY_shareMutex(&mutex_attr);
    MEMORY_shareConditionVariable(&cond_attr);
    PLATFORM_initMutex(&(newMethod->mutex), &mutex_attr);
    PLATFORM_initCondVar(&(newMethod->notify), &cond_attr);

    /* Initialize IR Method */
    IRMETHOD_init(&(newMethod->IRMethod), name);

    /* Save reference into Method Metadata */
    IRMETHOD_setMethodMetadata(&(newMethod->IRMethod), METHOD_METADATA, (void *) newMethod);
    assert(IRMETHOD_getMethodMetadata(&(newMethod->IRMethod), METHOD_METADATA) == newMethod);

    /* Set the method's flags		*/
    newMethod->externallyCallable = JITTRUE;

    return newMethod;
}

static inline Method internal_newAnonymousMethod (CLIManager_t *cliManager, JITINT8 *name) {
    Method newMethod;

    newMethod = internal_allocNewMethod(cliManager, name);
    assert(newMethod != NULL);
    newMethod->state = IR_STATE;

    return newMethod;
}

static inline Method internal_newMethod (CLIManager_t *cliManager, MethodDescriptor *methodID, JITBOOLEAN isExternallyCallable) {
    Method newMethod;
    FunctionPointerDescriptor *signature;
    JITINT8 *methodName;

    /* Alloc memory for the new method		*/
    methodName = methodID->getName(methodID);
    newMethod = internal_allocNewMethod(cliManager, (JITINT8 *)strdup((char *)methodName));

    /* Initialize the variables			*/
    newMethod->state = CIL_STATE;
    newMethod->globalState = CIL_GSTATE;
    newMethod->externallyCallable = isExternallyCallable;
    newMethod->externallyCallable = JITTRUE;//FIXME

    /* Initialize the IR method			*/
    signature = methodID->getFormalSignature(methodID);
    IRMETHOD_setMethodDescriptor(&(newMethod->IRMethod), methodID);
    internaltranslateSignatureToIR(cliManager, methodID, signature, &((newMethod->IRMethod).signature), JITTRUE);

    /* Create the backend-dependent signature	*/
    if (!methodID->attributes->is_internal_call) {

        /* Create the backend-dependent signature	*/
        IRVM_translateIRMethodSignatureToBackendSignature(cliManager->IRVM, &((newMethod->IRMethod).signature), *(newMethod->jit_function));

        /* Create the backend-dependent method		*/
        IRVM_newBackendMethod(cliManager->IRVM, newMethod, *(newMethod->jit_function), !((cliManager->IRVM->behavior).staticCompilation), newMethod->externallyCallable);

    } else {
        ir_signature_t adaptedSignature;
        ir_signature_t *originalSignature;
        JITUINT32 count;

        /* Initialize the signature			*/
        memset(&adaptedSignature, 0, sizeof(ir_signature_t));
        originalSignature = &((newMethod->IRMethod).signature);

        /* Adapt the signature parameters               */
        adaptedSignature.parametersNumber = originalSignature->parametersNumber;
        adaptedSignature.parameterTypes = allocFunction(sizeof(JITUINT32) * (adaptedSignature.parametersNumber + 1));
        for (count = 0; count < (adaptedSignature.parametersNumber); count++) {
            switch (originalSignature->parameterTypes[count]) {
                case IRVALUETYPE:
                    adaptedSignature.parameterTypes[count] = IRMPOINTER;
                    break;
                default:
                    adaptedSignature.parameterTypes[count] = originalSignature->parameterTypes[count];
            }
        }
        adaptedSignature.ilParamsTypes = allocFunction(sizeof(TypeDescriptor *) * (adaptedSignature.parametersNumber + 1));
        memcpy(adaptedSignature.ilParamsTypes, originalSignature->ilParamsTypes, sizeof(TypeDescriptor *) * (adaptedSignature.parametersNumber));
        adaptedSignature.ilParamsDescriptor = allocFunction(sizeof(ParamDescriptor *) * (adaptedSignature.parametersNumber + 1));
        memcpy(adaptedSignature.ilParamsDescriptor, originalSignature->ilParamsDescriptor, sizeof(ParamDescriptor *) * (adaptedSignature.parametersNumber));

        /* Adapt the signature result			*/
        switch (originalSignature->resultType) {
            case IRVALUETYPE:
                adaptedSignature.parameterTypes[adaptedSignature.parametersNumber] = IRMPOINTER;
                (adaptedSignature.parametersNumber)++;
                adaptedSignature.resultType = IRVOID;
                break;
            default:
                adaptedSignature.resultType = originalSignature->resultType;
        }
        adaptedSignature.ilResultType = originalSignature->ilResultType;

        /* Create the backend-dependent signature	*/
        IRVM_translateIRMethodSignatureToBackendSignature(cliManager->IRVM, &adaptedSignature, *(newMethod->jit_function));

        /* Free the memory				*/
        freeFunction(adaptedSignature.parameterTypes);
        freeFunction(adaptedSignature.ilParamsTypes);
        freeFunction(adaptedSignature.ilParamsDescriptor);
    }

    /* Return					*/
    return newMethod;
}

inline void translateActualMethodSignatureToIR (CLIManager_t *self, MethodDescriptor *method, FunctionPointerDescriptor *signature, ir_signature_t *ir_signature) {
    internaltranslateSignatureToIR(self, method, signature, ir_signature, JITFALSE);
}

static inline JITUINT32 internaltranslateSignatureToIR (CLIManager_t *self, MethodDescriptor *method, FunctionPointerDescriptor *signature, ir_signature_t *ir_signature, JITBOOLEAN isFormal) {
    JITUINT32 param_count;
    JITUINT32 count;
    JITUINT32 index;
    JITBOOLEAN hasInstance;
    JITBOOLEAN isVarargCall;
    TypeDescriptor          *result;
    XanList                 *params;
    XanList			*paramsDesc;
    XanListItem             *item;
    XanListItem             *item2;

    /* Assertions                           */
    assert(self != NULL);
    assert(signature != NULL);

    /* Initialize the variables		*/
    paramsDesc	= NULL;
    hasInstance 	= JITFALSE;
    index 		= 0;
    isVarargCall 	= JITFALSE;

    /* Fetch the parameters			*/
    params = signature->params;
    param_count = xanList_length(params);
    item = xanList_first(params);
    assert(param_count >= 0);
    if (method != NULL) {
        paramsDesc = method->params;
    }

    if (signature->hasThis || (method != NULL && method->isCctor(method))) {

        /* If is not a static method, there is an implicit	*
         * parameter that is the object of the class where	*
         * the method is located				*/
        param_count++;
        hasInstance = JITTRUE;
    }
    if (signature->calling_convention == VARARG && isFormal) {

        /*
         * We are performing a vararg call. We are translating the formal ir_signature
         * We have to modify the ir_signature adding an array for the vararg parameters
         * The actual ir_signature has not to be modified, since it already contains
         * all the actual parameters.
         */
        param_count++;
        isVarargCall = JITTRUE;
    }

    /* Set the result type of the function and the "internal types" for all the parameters */
    result = signature->result;

    /* Set number of parameter in signature */
    IRMETHOD_initSignature(ir_signature, param_count);

    /* Set the IR return type */
    ir_signature->resultType = result->getIRType(result);

    /* Set the CIL type description of the result type	*/
    ir_signature->ilResultType = result;

    /* Set the types of all the parameters */
    item2	= NULL;
    if (paramsDesc != NULL) {
        item2	= xanList_first(paramsDesc);
    }
    for (count = 0; count < param_count; count++) {
        if (hasInstance && (count == 0)) {

            /* Set the first parameter to OBJECT */
            ir_signature->ilParamsTypes[count] = method->owner;
            ir_signature->parameterTypes[count] = IROBJECT;

        } else if (isVarargCall && (count == param_count - 1)) {
            TypeDescriptor          *arrayType;

            /* Set an array as the last parameter of the ir_signature */
            arrayType = (self->CLR).typedReferenceManager.fillILTypedRefArray();
            ir_signature->ilParamsTypes[count] = arrayType;
            ir_signature->parameterTypes[count] = IROBJECT;

        } else {
            TypeDescriptor          *current_param;

            /* Retrieve the current parameter */
            current_param = (TypeDescriptor *) item->data;
            assert(current_param != NULL);

            /* Retrieve the IR type for the current parameter */
            ir_signature->parameterTypes[count] = current_param->getIRType(current_param);

            /* Set the CIL description		*/
            ir_signature->ilParamsTypes[count] = current_param;
            if (item2 != NULL) {
                ir_signature->ilParamsDescriptor[count] = (ParamDescriptor *)item2->data;
                item2 = item2->next;
            }

            /* update the index value as postcondition */
            item = item->next;
            index++;
        }
    }

    /* Return the number of parameters	*/
    return param_count;
}

void CLIMANAGER_setEntryPoint (CLIManager_t *cliManager, t_binary_information *entry_point) {
    cliManager->entryPoint.binary		= entry_point;

    return ;
}

t_binary_information* CLIMANAGER_fetchBinaryByNameAndPrefix(CLIManager_t *cliManager, JITINT8 *assemblyName, JITINT8 *assemblyPrefix) {
    XanListItem             *item;
    t_binary_information    *binary;

    /* Assertions			*/
    assert(cliManager!= NULL);
    assert(cliManager->binaries != NULL);
    assert(assemblyName != NULL);
    assert(assemblyPrefix != NULL);

    item    = xanList_first(cliManager->binaries);
    while (item != NULL) {
        binary  = item->data;

        if ( !STRCMP(binary->name, assemblyName) && !STRCMP(binary->prefix, assemblyPrefix) ) {
            return binary;
        }

        item    = item->next;
    }
    return NULL;
}

JITBOOLEAN CLIMANAGER_binaryIsDecoded(CLIManager_t *cliManager, t_binary_information *binary_info) {
    /* Assertions			*/
    assert(cliManager!= NULL);
    assert(cliManager->binaries != NULL);
    assert(binary_info != NULL);

    if ( xanList_syncFind(cliManager->binaries, binary_info ) != NULL ) {
        return JITTRUE;
    }

    return JITFALSE;
}

void CLIMANAGER_insertDecodedBinary (t_binary_information *binary_info) {

    /* Assertions.
     */
    assert(cliManager!= NULL);
    assert(cliManager->binaries != NULL);
    assert(binary_info != NULL);

    xanList_insert(cliManager->binaries, binary_info);

    return ;
}

static inline ir_value_t typeDescriptorResolve (ir_symbol_t *symbol) {
    ir_value_t value;

    value.v = (JITNUINT) symbol->data;
    return value;
}

static inline JITUINT32 arrayDescriptorSerialize (ArrayDescriptor *array, void **mem, JITUINT32 *memBytesAllocated, JITUINT32 bytesWritten) {
    JITUINT32 count;

    ir_symbol_t *typeSymbol = (cliManager->translator).getTypeDescriptorSymbol(cliManager, array->type);

    /* Allocate enough space to serialize the array.
     */
    (*memBytesAllocated)	+= (3 + array->numSizes + 1 + array->numBounds) * (ILDJIT_maxNumberOfDigits(IRUINT32) + 1);
    (*mem)			= dynamicReallocFunction(*mem, *memBytesAllocated);

    /* Serialize the array.
     */
    bytesWritten    += ILDJIT_writeIntegerValueInMemory(*mem, bytesWritten, *memBytesAllocated, typeSymbol->id, JITTRUE);
    bytesWritten    += ILDJIT_writeIntegerValueInMemory(*mem, bytesWritten, *memBytesAllocated, array->rank, JITTRUE);
    bytesWritten    += ILDJIT_writeIntegerValueInMemory(*mem, bytesWritten, *memBytesAllocated, array->numSizes, JITTRUE);
    if (array->numSizes > 0) {
        for (count = 0; count < array->numSizes; count++) {
            bytesWritten    += ILDJIT_writeIntegerValueInMemory(*mem, bytesWritten, *memBytesAllocated, array->sizes[count], JITTRUE);
        }
    }
    bytesWritten    += ILDJIT_writeIntegerValueInMemory(*mem, bytesWritten, *memBytesAllocated, array->numBounds, JITTRUE);
    if (array->numBounds >0) {
        for (count = 0; count < array->numBounds; count++) {
            bytesWritten    += ILDJIT_writeIntegerValueInMemory(*mem, bytesWritten, *memBytesAllocated, array->bounds[count], JITTRUE);
        }
    }

    return bytesWritten;
}

static inline TypeDescriptor *arrayDescriptorDeserialize (void *mem, JITUINT32 bytesRead) {
    JITINT32 typeDescriptorSymbolID;
    JITINT32 rank;
    JITINT32 numSizes;
    JITUINT32 *sizes = NULL;
    JITINT32 numBounds;
    JITUINT32 *bounds = NULL;
    JITUINT32 count;

    bytesRead	+= ILDJIT_readIntegerValueFromMemory(mem, bytesRead, &typeDescriptorSymbolID);
    ir_symbol_t *typeDescriptorSymbol = IRSYMBOL_loadSymbol(typeDescriptorSymbolID);
    TypeDescriptor *arrayType = (TypeDescriptor *) (JITNUINT) (IRSYMBOL_resolveSymbol(typeDescriptorSymbol).v);

    bytesRead	+= ILDJIT_readIntegerValueFromMemory(mem, bytesRead, &rank);
    bytesRead	+= ILDJIT_readIntegerValueFromMemory(mem, bytesRead, &numSizes);
    if (numSizes > 0) {
        sizes = alloca(sizeof(JITUINT32) * numSizes);
        for (count = 0; count < numSizes; count++) {
            bytesRead	+= ILDJIT_readIntegerValueFromMemory(mem, bytesRead, (JITINT32 *)&sizes[count]);
        }
    }
    bytesRead	+= ILDJIT_readIntegerValueFromMemory(mem, bytesRead, &numBounds);
    if (numBounds > 0) {
        sizes = alloca(sizeof(JITUINT32) * numBounds);
        for (count = 0; count < numBounds; count++) {
            bytesRead	+= ILDJIT_readIntegerValueFromMemory(mem, bytesRead, (JITINT32 *)&bounds[count]);
        }
    }

    return cliManager->metadataManager->makeArrayDescriptor(cliManager->metadataManager, arrayType, rank, numSizes, sizes, numBounds, bounds);
}

static inline JITUINT32 functionPointerDescriptorSerialize (FunctionPointerDescriptor *ptr, void **mem, JITUINT32 *memBytesAllocated, JITUINT32 bytesWritten) {

    /* Allocate enough space to serialize the array.
     */
    (*memBytesAllocated)	+= (8 + xanList_length(ptr->params)) * (ILDJIT_maxNumberOfDigits(IRUINT32) + 1);
    (*mem)			= dynamicReallocFunction(*mem, *memBytesAllocated);

    /* Serialize signature of the function pointer descriptor.
     */
    bytesWritten    += ILDJIT_writeIntegerValueInMemory(*mem, bytesWritten, *memBytesAllocated, ptr->hasThis, JITTRUE);
    bytesWritten    += ILDJIT_writeIntegerValueInMemory(*mem, bytesWritten, *memBytesAllocated, ptr->explicitThis, JITTRUE);
    bytesWritten    += ILDJIT_writeIntegerValueInMemory(*mem, bytesWritten, *memBytesAllocated, ptr->hasSentinel, JITTRUE);
    bytesWritten    += ILDJIT_writeIntegerValueInMemory(*mem, bytesWritten, *memBytesAllocated, ptr->sentinel_index, JITTRUE);
    bytesWritten    += ILDJIT_writeIntegerValueInMemory(*mem, bytesWritten, *memBytesAllocated, ptr->calling_convention, JITTRUE);
    bytesWritten    += ILDJIT_writeIntegerValueInMemory(*mem, bytesWritten, *memBytesAllocated, ptr->generic_param_count, JITTRUE);

    TypeDescriptor *resultType = ptr->result;
    ir_symbol_t *typeResultSymbol = (cliManager->translator).getTypeDescriptorSymbol(cliManager, resultType);
    bytesWritten    += ILDJIT_writeIntegerValueInMemory(*mem, bytesWritten, *memBytesAllocated, typeResultSymbol->id, JITTRUE);

    JITUINT32 paramCount = xanList_length(ptr->params);
    bytesWritten    += ILDJIT_writeIntegerValueInMemory(*mem, bytesWritten, *memBytesAllocated, paramCount, JITTRUE);
    XanListItem *item = xanList_first(ptr->params);
    while (item != NULL) {
        TypeDescriptor *paramType = (TypeDescriptor *) item->data;
        ir_symbol_t *typeParamSymbol = (cliManager->translator).getTypeDescriptorSymbol(cliManager, paramType);
        bytesWritten    += ILDJIT_writeIntegerValueInMemory(*mem, bytesWritten, *memBytesAllocated, typeParamSymbol->id, JITTRUE);
        item = item->next;
    }

    return bytesWritten ;
}

static inline TypeDescriptor *functionPointerDescriptorDeserialize (void *mem, JITUINT32 bytesRead) {
    JITINT32 hasThis;
    JITINT32 explicitThis;
    JITINT32 hasSentinel;
    JITINT32 sentinel_index;
    JITINT32 calling_convention;
    JITINT32 generic_param_count;
    JITINT32 resultTypeSymbolID;
    JITINT32 paramCount;
    JITUINT32 count;

    bytesRead	+= ILDJIT_readIntegerValueFromMemory(mem, bytesRead, &hasThis);
    bytesRead	+= ILDJIT_readIntegerValueFromMemory(mem, bytesRead, &explicitThis);
    bytesRead	+= ILDJIT_readIntegerValueFromMemory(mem, bytesRead, &hasSentinel);
    bytesRead	+= ILDJIT_readIntegerValueFromMemory(mem, bytesRead, &sentinel_index);
    bytesRead	+= ILDJIT_readIntegerValueFromMemory(mem, bytesRead, &calling_convention);
    bytesRead	+= ILDJIT_readIntegerValueFromMemory(mem, bytesRead, &generic_param_count);
    bytesRead	+= ILDJIT_readIntegerValueFromMemory(mem, bytesRead, &resultTypeSymbolID);

    ir_symbol_t *typeDescriptorSymbol = IRSYMBOL_loadSymbol(resultTypeSymbolID);
    TypeDescriptor *resultType = (TypeDescriptor *) (JITNUINT) (IRSYMBOL_resolveSymbol(typeDescriptorSymbol).v);
    bytesRead	+= ILDJIT_readIntegerValueFromMemory(mem, bytesRead, &paramCount);
    XanList *params = xanList_new(allocFunction, freeFunction, NULL);
    for (count = 0; count < paramCount; count++) {
        JITINT32 paramTypeSymbolID;
        bytesRead	+= ILDJIT_readIntegerValueFromMemory(mem, bytesRead, &paramTypeSymbolID);
        ir_symbol_t *paramTypeSymbol = IRSYMBOL_loadSymbol(paramTypeSymbolID);
        TypeDescriptor *paramType = (TypeDescriptor *) (JITNUINT) (IRSYMBOL_resolveSymbol(paramTypeSymbol).v);
        xanList_append(params, (void *) paramType);
    }
    TypeDescriptor *result = cliManager->metadataManager->makeFunctionPointerDescriptor(cliManager->metadataManager, hasThis, explicitThis, hasSentinel, sentinel_index, calling_convention, generic_param_count, resultType, params);
    xanList_destroyList(params);
    return result;
}

static inline JITUINT32 pointerDescriptorSerialize (PointerDescriptor *ptr, void **mem, JITUINT32 *memBytesAllocated, JITUINT32 bytesWritten) {

    /* Allocate enough space to serialize the array.
     */
    (*memBytesAllocated)	+= (ILDJIT_maxNumberOfDigits(IRUINT32) + 1);
    (*mem)			= dynamicReallocFunction(*mem, *memBytesAllocated);

    /* Serialize the pointer descriptor.
     */
    ir_symbol_t *typeSymbol = (cliManager->translator).getTypeDescriptorSymbol(cliManager, ptr->type);
    bytesWritten    += ILDJIT_writeIntegerValueInMemory(*mem, bytesWritten, *memBytesAllocated, typeSymbol->id, JITTRUE);

    return bytesWritten;
}

static inline TypeDescriptor *pointerDescriptorDeserialize (void *mem, JITUINT32 bytesRead) {
    JITINT32 typeDescriptorSymbolID;

    bytesRead	+= ILDJIT_readIntegerValueFromMemory(mem, bytesRead, &typeDescriptorSymbolID);
    ir_symbol_t *typeDescriptorSymbol = IRSYMBOL_loadSymbol(typeDescriptorSymbolID);
    TypeDescriptor *pointedType = (TypeDescriptor *) (JITNUINT) (IRSYMBOL_resolveSymbol(typeDescriptorSymbol).v);
    return pointedType->makePointerDescriptor(pointedType);
}

static inline JITUINT32 classDescriptorSerialize (ClassDescriptor *class, void **mem, JITUINT32 *memBytesAllocated, JITUINT32 bytesWritten) {
    JITINT8 *name;

    /* Serialize the type.
     */
    if (class->attributes->isNested) {

        /* Allocate enough space to serialize.
         */
        (*memBytesAllocated)	+= 2 * (ILDJIT_maxNumberOfDigits(IRUINT32) + 1);
        (*mem)			= dynamicReallocFunction(*mem, *memBytesAllocated);

        /* Serialize the type.
         */
        bytesWritten    += ILDJIT_writeIntegerValueInMemory(*mem, bytesWritten, *memBytesAllocated, 1, JITTRUE);
        TypeDescriptor *enclosingType = class->getEnclosing(class);
        ir_symbol_t *typeSymbol = (cliManager->translator).getTypeDescriptorSymbol(cliManager, enclosingType);
        bytesWritten    += ILDJIT_writeIntegerValueInMemory(*mem, bytesWritten, *memBytesAllocated, typeSymbol->id, JITTRUE);
    } else {

        /* Fetch the binary where the type is stored.
         */
        t_binary_information *binary = (t_binary_information *) class->row->binary;

        /* Allocate enough space to serialize.
         */
        (*memBytesAllocated)	+= (ILDJIT_maxNumberOfDigits(IRUINT32) + 1);
        (*memBytesAllocated)	+= STRLEN(binary->name) + 2 + 1;
        (*mem)			= dynamicReallocFunction(*mem, *memBytesAllocated);

        /* Serialize the fact that it is not nested the type.
         */
        bytesWritten    += ILDJIT_writeIntegerValueInMemory(*mem, bytesWritten, *memBytesAllocated, 0, JITTRUE);

        /* Serialize the name of the binary where the type is stored.
         */
        bytesWritten    += ILDJIT_writeStringInMemory(*mem, bytesWritten, *memBytesAllocated, binary->name, JITTRUE);

        JITINT8 *nameSpace = class->getTypeNameSpace(class);

        JITUINT32 nameSpaceLen = STRLEN(nameSpace);
        if (nameSpaceLen == 0) {

            /* Allocate enough space to serialize.
             */
            (*memBytesAllocated)	+= 1;
            (*mem)			= dynamicReallocFunction(*mem, *memBytesAllocated);

            /* Serialize.
             */
            bytesWritten    += ILDJIT_writeStringInMemory(*mem, bytesWritten, *memBytesAllocated, (JITINT8 *) "-", JITTRUE);

        } else {

            /* Allocate enough space to serialize.
             */
            (*memBytesAllocated)	+= STRLEN(nameSpace) + 1;
            (*mem)			= dynamicReallocFunction(*mem, *memBytesAllocated);

            /* Serialize the name space.
             */
            bytesWritten    += ILDJIT_writeStringInMemory(*mem, bytesWritten, *memBytesAllocated, nameSpace, JITTRUE);
        }
    }

    /* Fetch the name of the class.
     */
    if (class->isGeneric(class)) {
        TypeDescriptor *genericType = class->getGenericDefinition(class);
        name = genericType->getName(genericType);
    } else {
        name = class->getName(class);
    }

    /* Allocate enough space to serialize.
     */
    (*memBytesAllocated)	+= STRLEN(name) + 2;
    (*mem)			= dynamicReallocFunction(*mem, *memBytesAllocated);

    /* Serialize the name of the class.
     */
    bytesWritten    += ILDJIT_writeStringInMemory(*mem, bytesWritten, *memBytesAllocated, name, JITTRUE);

    /* Serialize the container of the class.
     */
    if (class->container == NULL) {

        /* Allocate enough space to serialize.
         */
        (*memBytesAllocated)	+= 2;
        (*mem)			= dynamicReallocFunction(*mem, *memBytesAllocated);

        /* Serialize the fact that there is no container.
         */
        bytesWritten    += ILDJIT_writeIntegerValueInMemory(*mem, bytesWritten, *memBytesAllocated, 0, JITTRUE);

    } else {
        JITUINT32 count;

        /* Allocate enough space to serialize.
         */
        (*memBytesAllocated)	+= ((1 + class->container->arg_count) * (ILDJIT_maxNumberOfDigits(IRUINT32) + 1));
        (*mem)			= dynamicReallocFunction(*mem, *memBytesAllocated);

        /* Serialize the number of arguments.
         */
        bytesWritten    += ILDJIT_writeIntegerValueInMemory(*mem, bytesWritten, *memBytesAllocated, class->container->arg_count, JITTRUE);

        /* Serialize the arguments.
         */
        for (count = 0; count < class->container->arg_count; count++) {
            TypeDescriptor *paramType = class->container->paramType[count];
            ir_symbol_t *paramTypeSymbol = (cliManager->translator).getTypeDescriptorSymbol(cliManager, paramType);
            bytesWritten    += ILDJIT_writeIntegerValueInMemory(*mem, bytesWritten, *memBytesAllocated, paramTypeSymbol->id, JITTRUE);
        }
    }

    return bytesWritten;
}

static inline TypeDescriptor *classDescriptorDeserialize (void *mem, JITUINT32 bytesRead) {
    JITINT32 	genericParamCount;
    JITINT32 	isNested;
    TypeDescriptor 	*result;
    TypeDescriptor 	*class;

    bytesRead	+= ILDJIT_readIntegerValueFromMemory(mem, bytesRead, &isNested);
    if (isNested) {
        JITINT32 	typeDescriptorSymbolID;
        JITINT8 	*typeName;
        bytesRead	+= ILDJIT_readIntegerValueFromMemory(mem, bytesRead, &typeDescriptorSymbolID);
        ir_symbol_t *typeDescriptorSymbol = IRSYMBOL_loadSymbol(typeDescriptorSymbolID);
        TypeDescriptor *enclosingType = (TypeDescriptor *) (JITNUINT) (IRSYMBOL_resolveSymbol(typeDescriptorSymbol).v);

        bytesRead	+= ILDJIT_readStringFromMemory(mem, bytesRead, &typeName);
        class 		= enclosingType->getNestedTypeFromName(enclosingType, typeName);

        /* Free the memory.
         */
        freeFunction(typeName);

    } else {
        JITINT8 		*binaryName;
        JITINT8 		*typeName;
        JITINT8		 	*typeNameSpace;
        t_binary_information 	*binary;

        /* Fetch the binary.
         */
        bytesRead	+= ILDJIT_readStringFromMemory(mem, bytesRead, &binaryName);
        binary 		= NULL;
        XanListItem *item = xanList_first(cliManager->binaries);
        while (item != NULL) {
            binary = (t_binary_information *) item->data;
            if (STRCMP(binaryName, binary->name)==0) {
                break;
            }
            item = item->next;
        }
        assert(binary != NULL);

        /* Fetch the type.
         */
        bytesRead	+= ILDJIT_readStringFromMemory(mem, bytesRead, &typeNameSpace);
        bytesRead	+= ILDJIT_readStringFromMemory(mem, bytesRead, &typeName);
        if (STRCMP(typeNameSpace, "-")==0) {
            class = cliManager->metadataManager->getTypeDescriptorFromNameAndAssembly(cliManager->metadataManager, binary ,NULL, typeName);
        } else {
            class = cliManager->metadataManager->getTypeDescriptorFromNameAndAssembly(cliManager->metadataManager, binary, typeNameSpace, typeName);
        }

        /* Free the memory.
         */
        freeFunction(binaryName);
        freeFunction(typeName);
        freeFunction(typeNameSpace);
    }
    bytesRead	+= ILDJIT_readIntegerValueFromMemory(mem, bytesRead, &genericParamCount);
    if (genericParamCount == 0) {
        result = class;
    } else {
        GenericContainer class_container;
        initTempGenericContainer(class_container, CLASS_LEVEL, NULL, genericParamCount);
        JITUINT32 count;
        for (count = 0; count < genericParamCount; count++) {
            JITINT32 genericParamSymbolID;
            bytesRead	+= ILDJIT_readIntegerValueFromMemory(mem, bytesRead, &genericParamSymbolID);
            ir_symbol_t *genericParamSymbol = IRSYMBOL_loadSymbol(genericParamSymbolID);
            class_container.paramType[count] = (TypeDescriptor *) (JITNUINT) (IRSYMBOL_resolveSymbol(genericParamSymbol).v);
        }
        result = class->getInstance(class, &class_container);
    }

    return result;
}

static inline void typeDescriptorSerialize (ir_symbol_t * symbol, void **mem, JITUINT32 *memBytesAllocated) {
    TypeDescriptor 	*type;
    JITUINT32	bytesWritten;

    /* Assertions.
     */
    assert(symbol != NULL);
    assert(mem != NULL);
    assert(memBytesAllocated != NULL);

    /* Initialize the variables.
     */
    type 			= (TypeDescriptor *) symbol->data;
    assert(type != NULL);

    /* Allocate the memory.
     */
    (*memBytesAllocated)	= 2 * (ILDJIT_maxNumberOfDigits(IRUINT32) + 1);
    (*mem)			= allocFunction(*memBytesAllocated);
    bytesWritten		= 0;

    /* Write whether the type is by reference or not.
     */
    bytesWritten    += ILDJIT_writeIntegerValueInMemory(*mem, bytesWritten, *memBytesAllocated, type->isByRef, JITTRUE);

    /* Write the type of the element.
     */
    bytesWritten    += ILDJIT_writeIntegerValueInMemory(*mem, bytesWritten, *memBytesAllocated, type->element_type, JITTRUE);

    /* Serialize the type.
     */
    switch (type->element_type) {
        case ELEMENT_ARRAY:
            bytesWritten	= arrayDescriptorSerialize((ArrayDescriptor *) type->descriptor, mem, memBytesAllocated, bytesWritten);
            break;
        case ELEMENT_FNPTR:
            bytesWritten	= functionPointerDescriptorSerialize((FunctionPointerDescriptor *) type->descriptor, mem, memBytesAllocated, bytesWritten);
            break;
        case ELEMENT_PTR:
            bytesWritten	= pointerDescriptorSerialize((PointerDescriptor *) type->descriptor, mem, memBytesAllocated, bytesWritten);
            break;
        case ELEMENT_CLASS:
            bytesWritten	= classDescriptorSerialize((ClassDescriptor *) type->descriptor, mem, memBytesAllocated, bytesWritten);
            break;
        case ELEMENT_VAR:
        case ELEMENT_MVAR:
        default:
            PDEBUG("typeDescriptorSerialize: ERROR! INVALID TYPE! %d\n", type->element_type);
            abort();
    }

    /* Truncate till the last byte written.
     */
    (*memBytesAllocated)	= bytesWritten;

    return ;
}

static inline void typeDescriptorDump (ir_symbol_t *symbol, FILE *fileToWrite) {
    TypeDescriptor *type;

    /* Initialize the variables				*/
    type = (TypeDescriptor *) symbol->data;
    assert(type != NULL);

    fprintf(fileToWrite, "Type Descriptor of %s", type->getCompleteName(type));
}

static inline ir_symbol_t * getTypeDescriptorSymbol (CLIManager_t *cliManager, TypeDescriptor *type) {
    return IRSYMBOL_createSymbol(TYPE_DESCRIPTOR_SYMBOL, (void *) type);
}

static inline ir_symbol_t *typeDescriptorDeserialize (void *mem, JITUINT32 memBytes) {
    TypeDescriptor 	*type;
    JITINT32 	isByRef;
    JITINT32 	element_type;
    JITUINT32	bytesRead;
    ir_symbol_t	*irSym;

    /* Fetch the attributes of the type.
     */
    bytesRead		= ILDJIT_readIntegerValueFromMemory(mem, 0, &isByRef);
    bytesRead		+= ILDJIT_readIntegerValueFromMemory(mem, bytesRead, &element_type);

    /* Fetch the type.
     */
    switch (element_type) {
        case ELEMENT_ARRAY:
            type = arrayDescriptorDeserialize(mem, bytesRead);
            break;
        case ELEMENT_FNPTR:
            type = functionPointerDescriptorDeserialize(mem, bytesRead);
            break;
        case ELEMENT_PTR:
            type = pointerDescriptorDeserialize(mem, bytesRead);
            break;
        case ELEMENT_CLASS:
            type = classDescriptorDeserialize(mem, bytesRead);
            break;
        case ELEMENT_VAR:
        case ELEMENT_MVAR:
        default:
            PDEBUG("typeDescriptorDeserialize: ERROR! INVALID TYPE! %d\n", element_type);
            abort();
    }
    assert(type != NULL);
    if (isByRef) {
        type = type->makeByRef(type);
    }
    assert(type != NULL);

    /* Fetch the symbol.
     */
    irSym	= getTypeDescriptorSymbol(cliManager, type);

    return irSym;
}

static inline ir_value_t methodDescriptorResolve (ir_symbol_t *symbol) {
    ir_value_t value;

    value.v = (JITNUINT) symbol->data;
    return value;
}

static inline void methodDescriptorSerialize (ir_symbol_t * symbol, void **mem, JITUINT32 *memBytesAllocated) {
    JITINT8 	*name;
    JITUINT32	bytesWritten;

    MethodDescriptor *method = (MethodDescriptor *) symbol->data;
    JITUINT32 overloadingSequenceID = method->getOverloadingSequenceID(method);
    TypeDescriptor *type = method->owner;
    ir_symbol_t *typeSymbol = (cliManager->translator).getTypeDescriptorSymbol(cliManager, type);

    /* Allocate the memory.
     */
    (*memBytesAllocated)	= (3 * (ILDJIT_maxNumberOfDigits(IRUINT32) + 1));
    (*mem)			= allocFunction(*memBytesAllocated);
    bytesWritten		= 0;

    /* Serialize the symbol ID.
     */
    bytesWritten    += ILDJIT_writeIntegerValueInMemory(*mem, bytesWritten, *memBytesAllocated, typeSymbol->id, JITTRUE);

    /* Fetch the name of the method.
     */
    if (method->isGeneric(method)) {
        MethodDescriptor *genericMethod = method->getGenericDefinition(method);
        name = genericMethod->getName(genericMethod);
    } else {
        name = method->getName(method);
    }

    /* Allocate enough space to serialize the array.
     */
    (*memBytesAllocated)	+= STRLEN(name) + 2;
    (*mem)			= dynamicReallocFunction(*mem, *memBytesAllocated);

    /* Serialize the name.
     */
    bytesWritten    += ILDJIT_writeStringInMemory(*mem, bytesWritten, *memBytesAllocated, name, JITTRUE);
    bytesWritten    += ILDJIT_writeIntegerValueInMemory(*mem, bytesWritten, *memBytesAllocated, overloadingSequenceID, JITTRUE);
    if (method->container == NULL) {
        bytesWritten    += ILDJIT_writeIntegerValueInMemory(*mem, bytesWritten, *memBytesAllocated, 0, JITTRUE);

    } else {
        JITUINT32 count;
        bytesWritten    += ILDJIT_writeIntegerValueInMemory(*mem, bytesWritten, *memBytesAllocated, method->container->arg_count, JITTRUE);

        /* Allocate enough space to serialize the arguments.
         */
        (*memBytesAllocated)	+= (method->container->arg_count * (ILDJIT_maxNumberOfDigits(IRUINT32) + 1));
        (*mem)			= dynamicReallocFunction(*mem, *memBytesAllocated);

        for (count = 0; count < method->container->arg_count; count++) {
            TypeDescriptor *paramType = method->container->paramType[count];
            ir_symbol_t *paramTypeSymbol = (cliManager->translator).getTypeDescriptorSymbol(cliManager, paramType);
            bytesWritten    += ILDJIT_writeIntegerValueInMemory(*mem, bytesWritten, *memBytesAllocated, paramTypeSymbol->id, JITTRUE);
        }
    }
    (*memBytesAllocated)	= bytesWritten;

    return ;
}

static inline void methodDescriptorDump (ir_symbol_t *symbol, FILE *fileToWrite) {
    MethodDescriptor *method;

    /* Initialize the variables				*/
    method = (MethodDescriptor *) symbol->data;
    assert(method != NULL);

    fprintf(fileToWrite, "Method Descriptor of %s", method->getCompleteName(method));
}

static inline ir_symbol_t * getMethodDescriptorSymbol (CLIManager_t *cliManager, MethodDescriptor *method) {
    return IRSYMBOL_createSymbol(METHOD_DESCRIPTOR_SYMBOL, (void *) method);
}

static inline ir_symbol_t * methodDescriptorDeserialize (void *mem, JITUINT32 memBytes) {
    JITINT32 	typeSymbolID;
    JITINT8 	*methodName;
    JITINT32 	overloadingSequenceID;
    JITINT32 	genericParamCount;
    JITUINT32	bytesRead;
    ir_symbol_t 	*symbol;

    /* Assertions.
     */
    assert(mem != NULL);
    assert(memBytes > 0);

    /* Initialize the variables.
     */
    methodName	= NULL;

    /* Deserialize the symbol ID.
     */
    bytesRead		= ILDJIT_readIntegerValueFromMemory(mem, 0, &typeSymbolID);

    /* Fetch the type symbol.
     */
    ir_symbol_t *typeSymbol = IRSYMBOL_loadSymbol(typeSymbolID);
    TypeDescriptor *type = (TypeDescriptor *) (JITNUINT) (IRSYMBOL_resolveSymbol(typeSymbol).v);
    assert(type != NULL);

    /* Deserialize the name of the method.
     */
    bytesRead		+= ILDJIT_readStringFromMemory(mem, bytesRead, &methodName);

    /* Fetch the method.
     */
    XanList *methodList = type->getMethodFromName(type, methodName);
    if (methodList == NULL) {
        fprintf(stderr, "ILDJIT: An error occured while loading the cached IR. The method \"%s\" has not been found in the IR code cache.\n", (char *)methodName);
        abort();
    }
    assert(methodList != NULL);
    assert(xanList_length(methodList) > 0);
    freeFunction(methodName);

    /* Deserialize the overloading sequence ID.
     */
    bytesRead	+= ILDJIT_readIntegerValueFromMemory(mem, bytesRead, &overloadingSequenceID);

    /* Fetch the sequence ID.
     */
    XanListItem *item = xanList_getElementFromPositionNumber(methodList, overloadingSequenceID);
    MethodDescriptor *genericMethod = item->data;

    /* Deserialize the number of generic parameters.
     */
    bytesRead	+= ILDJIT_readIntegerValueFromMemory(mem, bytesRead, &genericParamCount);

    /* Decode the parameters.
     */
    if (genericParamCount == 0) {
        symbol = getMethodDescriptorSymbol(cliManager, genericMethod);

    } else {
        GenericContainer method_container;
        initTempGenericContainer(method_container, METHOD_LEVEL, NULL, genericParamCount);
        JITUINT32 count;
        for (count = 0; count < genericParamCount; count++) {
            JITINT32 genericParamSymbolID;

            /* Deserialize the symbol ID of the current parameter.
             */
            bytesRead	+= ILDJIT_readIntegerValueFromMemory(mem, bytesRead, &genericParamSymbolID);

            /* Fetch the type of the current parameter.
             */
            ir_symbol_t *genericParamSymbol = IRSYMBOL_loadSymbol(genericParamSymbolID);
            method_container.paramType[count] = (TypeDescriptor *) (JITNUINT) (IRSYMBOL_resolveSymbol(genericParamSymbol).v);
        }
        MethodDescriptor *methodInstance = genericMethod->getInstance(genericMethod, &method_container);
        symbol = getMethodDescriptorSymbol(cliManager, methodInstance);
    }

    return symbol;
}

static inline ir_value_t fieldDescriptorResolve (ir_symbol_t *symbol) {
    ir_value_t value;

    value.v = (JITNUINT) symbol->data;
    return value;
}

static inline void fieldDescriptorSerialize (ir_symbol_t * symbol, void **mem, JITUINT32 *memBytesAllocated) {
    JITUINT32	bytesWritten;

    FieldDescriptor *field = (FieldDescriptor *) symbol->data;
    JITINT8 *name = field->getName(field);
    TypeDescriptor *type = field->owner;
    ir_symbol_t *typeSymbol = (cliManager->translator).getTypeDescriptorSymbol(cliManager, type);

    /* Allocate the memory.
     */
    (*memBytesAllocated)	= (ILDJIT_maxNumberOfDigits(IRUINT32) + 1);
    (*memBytesAllocated)	+= STRLEN(name) + 1;
    (*mem)			= allocFunction(*memBytesAllocated);
    bytesWritten		= 0;

    bytesWritten    += ILDJIT_writeIntegerValueInMemory(*mem, bytesWritten, *memBytesAllocated, typeSymbol->id, JITTRUE);
    bytesWritten    += ILDJIT_writeStringInMemory(*mem, bytesWritten, *memBytesAllocated, name, JITFALSE);

    (*memBytesAllocated)	= bytesWritten;

    return ;
}

static inline void fieldDescriptorDump (ir_symbol_t *symbol, FILE *fileToWrite) {
    FieldDescriptor *field;

    /* Initialize the variables				*/
    field = (FieldDescriptor *) symbol->data;
    assert(field != NULL);

    fprintf(fileToWrite, "Field Descriptor of %s", field->getCompleteName(field));
}

static inline ir_symbol_t * fieldDescriptorDeserialize (void *mem, JITUINT32 memBytes) {
    JITINT32 	typeSymbolID;
    JITUINT32	bytesRead;
    JITINT8 	*fieldName;

    /* Assertions.
     */
    assert(mem != NULL);
    assert(memBytes > 0);

    /* Deserialize the symbol ID.
     */
    bytesRead		= ILDJIT_readIntegerValueFromMemory(mem, 0, &typeSymbolID);

    /* Fetch the symbol.
     */
    ir_symbol_t *typeSymbol = IRSYMBOL_loadSymbol(typeSymbolID);
    TypeDescriptor *type = (TypeDescriptor *) (JITNUINT) (IRSYMBOL_resolveSymbol(typeSymbol).v);
    if (type == NULL) {
        fprintf(stderr, "ILDJIT: ERROR: A symbol cannot be resolved because the correspondent CIL type has not been found.\n");
        abort();
    }

    /* Deserialize the name of the field.
     */
    ILDJIT_readStringFromMemory(mem, bytesRead, &fieldName);

    /* Fetch the field symbol.
     */
    FieldDescriptor *field = type->getFieldFromName(type, fieldName);
    freeFunction(fieldName);
    return getFieldDescriptorSymbol(cliManager, field);
}

static inline ir_symbol_t * getFieldDescriptorSymbol (CLIManager_t *cliManager, FieldDescriptor *field) {
    return IRSYMBOL_createSymbol(FIELD_DESCRIPTOR_SYMBOL, (void *) field);
}

static inline ir_value_t indexOfTypeDescriptorResolve (ir_symbol_t *symbol) {
    ir_value_t value;

    value.v = (cliManager->metadataManager)->getIndexOfTypeDescriptor((TypeDescriptor *)symbol->data) *sizeof(TypeElem);
    return value;
}

static inline void indexOfTypeDescriptorSerialize (ir_symbol_t * symbol, void **mem, JITUINT32 *memBytesAllocated) {
    TypeDescriptor 	*type;
    ir_symbol_t 	*typeSymbol;
    JITUINT32	bytesWritten;

    /* Allocate the memory.
     */
    (*memBytesAllocated)	= (ILDJIT_maxNumberOfDigits(IRUINT32) + 1);
    (*mem)			= allocFunction(*memBytesAllocated);

    /* Fetch the type.
     */
    type 			= (TypeDescriptor *) symbol->data;
    assert(type != NULL);
    typeSymbol 		= (cliManager->translator).getTypeDescriptorSymbol(cliManager, type);
    assert(typeSymbol != NULL);

    /* Serialize the type.
     */
    bytesWritten		= ILDJIT_writeIntegerValueInMemory(*mem, 0, *memBytesAllocated, typeSymbol->id, JITFALSE);
    (*memBytesAllocated)	= bytesWritten;

    return ;
}

static inline void indexOfTypeDescriptorDump (ir_symbol_t *symbol, FILE *fileToWrite) {
    TypeDescriptor *type;

    /* Initialize the variables				*/
    type = (TypeDescriptor *) symbol->data;
    assert(type != NULL);

    fprintf(fileToWrite, "Index of %s in Class Compatibility Table", type->getCompleteName(type));
}

static inline ir_symbol_t * indexOfTypeDescriptorDeserialize (void *mem, JITUINT32 memBytes) {
    JITINT32 typeSymbolID;

    /* Assertions.
     */
    assert(mem != NULL);
    assert(memBytes > 0);

    /* Deserialize the symbol ID.
     */
    ILDJIT_readIntegerValueFromMemory(mem, 0, &typeSymbolID);

    /* Fetch the symbol.
     */
    ir_symbol_t *typeSymbol = IRSYMBOL_loadSymbol(typeSymbolID);
    TypeDescriptor *type = (TypeDescriptor *) (JITNUINT) (IRSYMBOL_resolveSymbol(typeSymbol).v);
    return getIndexOfTypeDescriptorSymbol(cliManager, type);
}

static inline ir_symbol_t * getIndexOfTypeDescriptorSymbol (CLIManager_t *cliManager, TypeDescriptor *type) {
    return IRSYMBOL_createSymbol(INDEX_OF_TYPE_DESCRIPTOR_SYMBOL, (void *) type);
}

static inline ir_value_t IndexOfMethodDescriptorResolve (ir_symbol_t *symbol) {
    ir_value_t value;

    value.v = (cliManager->layoutManager).getIndexOfMethodDescriptor((MethodDescriptor *) symbol->data) *sizeof(IMTElem);
    return value;
}

static inline void IndexOfMethodDescriptorSerialize (ir_symbol_t * symbol, void **mem, JITUINT32 *memBytesAllocated) {
    MethodDescriptor 	*method;
    JITUINT32		bytesWritten;

    /* Allocate the memory.
     */
    (*memBytesAllocated)	= (ILDJIT_maxNumberOfDigits(IRUINT32) + 1);
    (*mem)			= allocFunction(*memBytesAllocated);

    /* Initialize the variables				*/
    method = (MethodDescriptor *) symbol->data;
    assert(method != NULL);

    ir_symbol_t *methodSymbol = (cliManager->translator).getMethodDescriptorSymbol(cliManager, method);
    bytesWritten		= ILDJIT_writeIntegerValueInMemory(*mem, 0, *memBytesAllocated, methodSymbol->id, JITTRUE);
    (*memBytesAllocated)	= bytesWritten;

    return ;
}

static inline void IndexOfMethodDescriptorDump (ir_symbol_t *symbol, FILE *fileToWrite) {
    MethodDescriptor *method;

    /* Initialize the variables				*/
    method = (MethodDescriptor *) symbol->data;
    assert(method != NULL);

    fprintf(fileToWrite, "Index of %s in Interface Method Table", method->getCompleteName(method));
}

static inline ir_symbol_t * IndexOfMethodDescriptorDeserialize (void *mem, JITUINT32 memBytes) {
    JITINT32 methodSymbolID;

    /* Assertions.
     */
    assert(mem != NULL);
    assert(memBytes > 0);

    /* Deserialize the symbol ID.
     */
    ILDJIT_readIntegerValueFromMemory(mem, 0, &methodSymbolID);

    /* Fetch the symbol.
     */
    ir_symbol_t *methodSymbol = IRSYMBOL_loadSymbol(methodSymbolID);
    MethodDescriptor *method = (MethodDescriptor *) (JITNUINT) (IRSYMBOL_resolveSymbol(methodSymbol).v);
    return getIndexOfMethodDescriptorSymbol(cliManager, method);
}

static inline ir_symbol_t * getIndexOfMethodDescriptorSymbol (CLIManager_t *cliManager, MethodDescriptor *method) {
    return IRSYMBOL_createSymbol(INDEX_OF_METHOD_DESCRIPTOR_SYMBOL, (void *) method);
}

void libclimanagerCompilationFlags (char *buffer, int bufferLength) {

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

void libclimanagerCompilationTime (char *buffer, int bufferLength) {

    /* Assertions				*/
    assert(buffer != NULL);

    snprintf(buffer, sizeof(char) * bufferLength, "%s %s", __DATE__, __TIME__);
}

char * libclimanagerVersion () {
    return VERSION;
}

static inline XanList * findCompatibleMethods (t_methods *methods, ir_signature_t *signature) {
    XanList *compatibleMethods;

    /* Assertions.
     */
    assert(methods != NULL);
    assert(methods->methods != NULL);

    /* Check if the signature is empty.
     */
    if (signature == NULL) {
        return NULL;
    }

    /* Search the signature.
     */
    compatibleMethods 	= (XanList *) xanHashTable_syncLookup(methods->methods, (void *) signature);

    return compatibleMethods;
}

static inline JITBOOLEAN deleteMethod (t_methods *methods, Method method) {
    XanList *compatibleMethods;
    JITINT8 *name;

    /* Assertions		*/
    assert(methods != NULL);
    assert(method != NULL);

    /* Remove the method    *
     * from the anonymous	*
     * set			*/
    if (methods->anonymousContainer != NULL) {
        xanHashTable_lock(methods->anonymousContainer);
        name = (method->IRMethod).name;
        if (    (name != NULL)                                                                          &&
                (xanHashTable_lookup(methods->anonymousContainer, name) == method)      ) {

            /* Delete the method		*/
            xanHashTable_removeElement(methods->anonymousContainer, name);

            /* Return			*/
            xanHashTable_unlock(methods->anonymousContainer);
            return JITTRUE;
        }
        xanHashTable_unlock(methods->anonymousContainer);
    }

    /* Remove from the CLI	*
     * methods		*/
    if (    ((method->IRMethod).signature.ilParamsTypes != NULL)       &&
            (methods->methods != NULL)                              ) {
        xanHashTable_lock(methods->methods);
        compatibleMethods = xanHashTable_lookup(methods->methods, &((method->IRMethod).signature));
        if (    (compatibleMethods != NULL)                      	&&
                (xanList_find(compatibleMethods, method) != NULL)    	) {

            /* Delete the method		*/
            xanList_removeElement(compatibleMethods, method, JITTRUE);
            assert(xanList_find(methods->container, method) != NULL);
            xanList_removeElement(methods->container, method, JITTRUE);

            /* Return			*/
            xanHashTable_unlock(methods->methods);
            return JITTRUE;
        }
        assert(xanList_find(methods->container, method) == NULL);
        xanHashTable_unlock(methods->methods);
    }
    if (xanList_find(methods->container, method) != NULL) {

        /* Delete the method		*/
        xanList_removeElement(methods->container, method, JITTRUE);

        /* Return			*/
        return JITTRUE;
    }
    assert(xanList_find((cliManager->methods).container, method) == NULL);

    /* Return		*/
    return JITFALSE;
}

static inline Method fetchOrCreateAnonymousMethod (t_methods *methods, JITINT8 *name) {
    Method method;

    xanHashTable_lock(methods->anonymousContainer);
    method = xanHashTable_lookup(methods->anonymousContainer, name);
    if (method == NULL) {

        /* Create an Anonymous Method */
        method = internal_newAnonymousMethod(cliManager, name);
        assert(method != NULL);

        xanHashTable_insert(methods->anonymousContainer, name, method);
    }
    xanHashTable_unlock(methods->anonymousContainer);

    return method;
}
