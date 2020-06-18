/*
 * Copyright (C) 2008 - 2011 Simone Campanoni
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
#include <base_symbol.h>

// My headers
#include <climanager_system.h>
#include <lib_pinvoke.h>
// End

typedef struct  _DynamicLibrary {
    JITINT8 *name;
    JITINT8 *fullName;
    void *handle;
    XanHashTable *symbols;
} DynamicLibrary;

typedef struct _DynamicLibrarySymbol {
    struct  _DynamicLibrary *owner;
    JITINT8 *name;
    void *pointer;
} DynamicLibrarySymbol;

JITUINT32 hashDynamicLibrary (DynamicLibrary *library);
JITINT32 equalsDynamicLibrary (DynamicLibrary *key1, DynamicLibrary *key2);
JITUINT32 hashDynamicLibrarySymbol (DynamicLibrarySymbol *symbol);
JITINT32 equalsDynamicLibrarySymbol (DynamicLibrarySymbol *key1, DynamicLibrarySymbol *key2);
static inline DynamicLibrary *newDynamicLibrary (PinvokeManager* manager, JITINT8 *name);
static inline void *newDynamicLibrarySymbol (DynamicLibrary *library, JITINT8 *name);
static inline ir_symbol_t *pInvokeManagerDeserialize (void *mem, JITUINT32 memBytes);
static inline void pInvokeManagerBuildMethod (PinvokeManager* manager, Method method);
static inline ir_instruction_t * internal_addPinvokeInstruction (PinvokeManager* manager, ir_method_t *caller, Method callee, ir_instruction_t *afterInst);
static void destroyPinvokeManager (PinvokeManager* manager);

extern CLIManager_t *cliManager;

JITUINT32 hashDynamicLibrary (DynamicLibrary *library) {
    if (library == NULL) {
        return 0;
    }
    JITUINT32 seed = hashString(library->name);
    return seed;
}

JITINT32 equalsDynamicLibrary (DynamicLibrary *key1, DynamicLibrary *key2) {
    JITINT32	result;

    if (key1 == NULL || key2 == NULL) {
        return 0;
    }
    result	= STRCMP(key1->name, key2->name);

    return !result;
}

JITUINT32 hashDynamicLibrarySymbol (DynamicLibrarySymbol *symbol) {
    if (symbol == NULL) {
        return 0;
    }
    JITUINT32 seed = hashString(symbol->name);;
    seed = combineHash(seed, symbol->owner);
    return seed;
}

JITINT32 equalsDynamicLibrarySymbol (DynamicLibrarySymbol *key1, DynamicLibrarySymbol *key2) {
    if (key1 == NULL || key2 == NULL) {
        return 0;
    }
    if (key1->owner != key2->owner) {
        return 0;
    }
    return !STRCMP(key1->name, key2->name);
}


DynamicLibrary *newDynamicLibrary (PinvokeManager* manager, JITINT8 *name) {
    DynamicLibrary library;
    char buf[1024];

    /* Assertions.
     */
    assert(manager != NULL);

    /* Check the name.
     */
    if (name == NULL) {
        return NULL;
    }

    /* Define the name of the library.
     */
    library.name = name;

    /* Check whether the library has been already loaded.
     */
    xanHashTable_lock(manager->libraries);
    DynamicLibrary *found = (DynamicLibrary *) xanHashTable_lookup(manager->libraries, (void *) &library);
    if (found == NULL) {
        JITINT8	prefixToUse[2048];
        size_t 	length;

        /* Find the path of the library to load.
         */
        setupPrefixFromPath((char *)manager->libPath, (char *)name, (char *) prefixToUse);

        /* Allocate the library.
         */
        found 		= allocFunction(sizeof(DynamicLibrary));
        length 		= STRLEN(prefixToUse) + STRLEN(name) + 2;
        found->fullName	= allocFunction(length);
        SNPRINTF(found->fullName, length, "%s/%s", prefixToUse, name);
        found->name	= (JITINT8 *)strdup((char *)name);
        found->symbols 	= xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, (JITUINT32 (*)(void *))hashDynamicLibrarySymbol, (JITINT32 (*)(void *, void *))equalsDynamicLibrarySymbol);

        /* Open the library.
         */
        found->handle 	= PLATFORM_dlopen((char *) found->fullName, RTLD_LAZY);
        if (found->handle == NULL) {
            snprintf(buf, sizeof(buf), "ILDJIT: PINVOKE: Library \"%s\" could not be loaded.\n%s", found->fullName, dlerror());
            print_err(buf, 0);
            abort();
        }

        /* Insert the just open library to the table.
         */
        xanHashTable_insert(manager->libraries, (void *) found, (void *) found);
        assert(xanHashTable_lookup(manager->libraries, (void *) &library) == found);
    }
    xanHashTable_unlock(manager->libraries);

    return found;
}

void *newDynamicLibrarySymbol (DynamicLibrary *library, JITINT8 *name) {
    DynamicLibrarySymbol 	symbol;

    /* assertions.
     */
    assert(library != NULL);

    /* Check the name.
     */
    if (name == NULL) {
        return NULL;
    }

    /* Define the symbol.
     */
    memset(&symbol, 0, sizeof(DynamicLibrarySymbol));
    symbol.name = name;
    symbol.owner = library;

    /* Find the symbol in the library.
     */
    xanHashTable_lock(library->symbols);
    DynamicLibrarySymbol *found = (DynamicLibrarySymbol *) xanHashTable_lookup(library->symbols, (void *) &symbol);
    if (found == NULL) {

        /* Allocate the structure to keep track of the library.
         */
        found = allocFunction(sizeof(DynamicLibrarySymbol));
        size_t length = sizeof(JITINT8) * (STRLEN(name) + 1);
        found->name = allocFunction(length);
        memcpy(found->name, name, length);
        found->owner = library;

        /* Link the symbol to the library.
         */
        found->pointer = PLATFORM_dlsym(library->handle, (char *) name);
        if (found->pointer == NULL) {
            char 	buf[1024];
            snprintf(buf, sizeof(buf), "ILDJIT: PINVOKE: Symbol \"%s\" could not be resolved in library \"%s\".", name, library->fullName);
            print_err(buf, 0);
            abort();
        }
        xanHashTable_insert(library->symbols, (void *) found, (void *) found);
    }
    xanHashTable_unlock(library->symbols);

    return found->pointer;
}


static inline ir_value_t pInvokeManagerResolve (ir_symbol_t *symbol) {
    ir_value_t value;

    MethodDescriptor *methodID = (MethodDescriptor *) symbol->data;
    assert(methodID != NULL);

    JITINT8 *symbolName = methodID->getImportName(methodID);
    JITINT8 *libraryName = methodID->getImportModule(methodID);

    /* Load Pointer to external Library */
    DynamicLibrary *library = newDynamicLibrary(&((cliManager->CLR).pinvokeManager), libraryName);
    void *pointer = newDynamicLibrarySymbol(library, symbolName);
    assert(pointer != NULL);

    value.v = (IR_ITEM_VALUE) (JITNUINT) pointer;
    return value;
}

static inline void pInvokeManagerSerialize (ir_symbol_t * symbol, void **mem, JITUINT32 *memBytesAllocated) {

    /* Allocate enough space to serialize the array.
     */
    (*memBytesAllocated)	= (ILDJIT_maxNumberOfDigits(IRUINT32) + 1);
    (*mem)			= allocFunction(*memBytesAllocated);

    MethodDescriptor *methodID = (MethodDescriptor *) symbol->data;
    assert(methodID != NULL);

    ir_symbol_t *methodSymbol = (cliManager->translator).getMethodDescriptorSymbol(cliManager, methodID);

    ILDJIT_writeIntegerValueInMemory(*mem, 0, *memBytesAllocated, methodSymbol->id, JITFALSE);

    return ;
}

static inline void pInvokeManagerDump (ir_symbol_t *symbol, FILE *fileToWrite) {
    MethodDescriptor 	*methodID;
    JITINT8 		*symbolName;
    JITINT8 		*libraryName;

    /* Assertions.
     */
    assert(symbol != NULL);
    assert(fileToWrite != NULL);

    /* Fetch the method ID.
     */
    methodID 	= (MethodDescriptor *) symbol->data;
    assert(methodID != NULL);

    /* Fetch the name of the symbol.
     */
    symbolName	= methodID->getImportName(methodID);
    libraryName 	= methodID->getImportModule(methodID);

    /* Dump.
     */
    fprintf(fileToWrite, "Machine code method \"%s\" in library \"%s\"", symbolName, libraryName);

    return ;
}

static inline ir_symbol_t *pInvokeManagerDeserialize (void *mem, JITUINT32 memBytes) {
    JITINT32 methodSymbolID;

    /* Assertions.
     */
    assert(mem != NULL);
    assert(memBytes > 0);

    /* Deserialize the method symbol ID.
     */
    ILDJIT_readIntegerValueFromMemory(mem, 0, &methodSymbolID);

    /* Load the symbol.
     */
    ir_symbol_t *symbol 	= IRSYMBOL_loadSymbol(methodSymbolID);

    /* Fetch the method descriptor.
     */
    MethodDescriptor *methodID = (MethodDescriptor *) (JITNUINT) (IRSYMBOL_resolveSymbol(symbol).v);

    return IRSYMBOL_createSymbol(PINVOKE_SYMBOL, (void *) methodID);
}

static inline void pInvokeManagerBuildMethod (PinvokeManager* manager, Method method) {
    ir_instruction_t        *inst;
    ir_instruction_t        *inst2;
    ir_method_t 		*ir_method;

    /* Fetch the IR method of the callee.
     */
    ir_method 	= method->getIRMethod(method);

    /* Add a call to the machine code callee.
     */
    inst		= internal_addPinvokeInstruction(manager, ir_method, method, NULL);

    /* Return instruction.
     */
    inst2 		= IRMETHOD_newInstructionOfType(ir_method, IRRET);
    memcpy(&(inst2->param_1), &(inst->result), sizeof(ir_item_t));

    method->lock(method);
    method->setState(method, IR_STATE);
    method->unlock(method);

    return ;
}

static inline ir_instruction_t * internal_addPinvokeInstruction (PinvokeManager* manager, ir_method_t *caller, Method callee, ir_instruction_t *afterInst) {
    ir_method_t		*calleeIRMethod;
    ir_instruction_t        *inst;
    MethodDescriptor 	*calleeID;
    ir_item_t               *icall_item;
    ir_symbol_t 		*symbol;
    XanList                 *icall_params;
    JITUINT32 		num_par;
    JITUINT32 		count;
    void			*fp;

    /* Assertions.
     */
    assert(manager != NULL);
    assert(caller != NULL);
    assert(callee != NULL);
#ifdef DEBUG
    if (afterInst != NULL) {
        assert(IRMETHOD_doesInstructionBelongToMethod(caller, afterInst));
    }
#endif

    /* Fetch the CIL ID of the callee.
     */
    calleeID 	= callee->getID(callee);
    assert(calleeID != NULL);

    /* Fetch the IR method of the callee.
     */
    calleeIRMethod 	= callee->getIRMethod(callee);
    assert(calleeIRMethod != NULL);

    /* Get the number of parameters of the method.
     */
    num_par 	= IRMETHOD_getMethodParametersNumber(calleeIRMethod);

    /* Fetch the symbol of the machine code of the callee.
     */
    symbol 		= IRSYMBOL_createSymbol(PINVOKE_SYMBOL, (void *) calleeID);
    assert(symbol != NULL);

    /* Check the invoked method.
     * Methods that quit the execution must be wrapped.
     */
    fp		= (void *)(JITNUINT)(pInvokeManagerResolve(symbol).v);
    if (fp == exit) {
        ir_item_t	*par1;
        inst 			= IRMETHOD_newInstructionOfTypeAfter(caller, afterInst, IREXIT);
        par1			= IRMETHOD_getInstructionParameter1(inst);
        (par1->value).v		= 0;
        par1->type		= IROFFSET;
        par1->internal_type	= (calleeIRMethod->signature).parameterTypes[0];
        par1->type_infos	= (calleeIRMethod->signature).ilParamsTypes[0];
        return inst;
    }

    /* Create the IR instruction for the method we want to call.
     */
    inst 		= IRMETHOD_newInstructionOfTypeAfter(caller, afterInst, IRNATIVECALL);

    /* Allocate the list of parameters of the real call of the method inside the machine code library.
     */
    icall_params 	= xanList_new(sharedAllocFunction, freeFunction, NULL);
    assert(icall_params != NULL);

    /* Set up method parameters list.
     */
    for (count = 0; count < num_par; count++) {
        icall_item 			= (ir_item_t *) sharedAllocFunction(sizeof(ir_item_t));
        (icall_item->value).v 		= count;
        icall_item->type 		= IROFFSET;
        icall_item->internal_type 	= (calleeIRMethod->signature).parameterTypes[count];

        /* we check if the parameter is a struct or an union  */
        if ((calleeIRMethod->signature).parameterTypes[count] == IRVALUETYPE) {
            icall_item->type_infos = (calleeIRMethod->signature).ilParamsTypes[count];
        }

        xanList_append(icall_params, icall_item);
    }
    inst->callParameters 	= icall_params;

    /* 1° parameter of IRNATIVECALL is the result type of our method.
     */
    (inst->param_1).value.v = (calleeIRMethod->signature).resultType;
    if ((calleeIRMethod->signature).resultType == IRVALUETYPE) {
        (inst->param_1).type_infos = (calleeIRMethod->signature).ilResultType;
    }
    (inst->param_1).type = IRTYPE;
    (inst->param_1).internal_type = IRTYPE;

    /* 2° parameter is the name of the method.
     * We do not set the name of the callee in case the IR code will be dumped making the pointer invalid for future loads.
     */
    (inst->param_2).value.v = 0;
    (inst->param_2).type = IRSTRING;
    (inst->param_2).internal_type = IRNINT;

    /* 3° parameter is the handle to the method inside the library.
     */
    (inst->param_3).value.v = (JITNUINT) symbol;
    (inst->param_3).type = IRSYMBOL;
    (inst->param_3).internal_type = IRFPOINTER;

    /* Result is the return value of the call.
     */
    switch ((calleeIRMethod->signature).resultType) {
        case NOPARAM:
        case IRVOID:
            break ;
        default:
            (inst->result).value.v 		= IRMETHOD_newVariableID(caller);
            inst->result.internal_type 	= (calleeIRMethod->signature).resultType;
            inst->result.type 		= IROFFSET;
            if ((calleeIRMethod->signature).resultType == IRVALUETYPE) {
                (inst->result).type_infos = (calleeIRMethod->signature).ilResultType;
            }
    }

    return inst;
}

void init_pinvokeManager (PinvokeManager *manager, JITINT8 *libPath) {

    /* Assertions.
     */
    assert(manager != NULL);
    assert(libPath != NULL);

    manager->libPath		= libPath;
    manager->buildMethod 		= pInvokeManagerBuildMethod;
    manager->addPinvokeInstruction	= internal_addPinvokeInstruction;
    manager->libraries 		= xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, (JITUINT32 (*)(void *))hashDynamicLibrary, (JITINT32 (*)(void *, void *))equalsDynamicLibrary);
    manager->destroy 		= destroyPinvokeManager;

    IRSYMBOL_registerSymbolManager(PINVOKE_SYMBOL, pInvokeManagerResolve, pInvokeManagerSerialize, pInvokeManagerDump, pInvokeManagerDeserialize);

    return ;
}

void destroyPinvokeManager (PinvokeManager* manager) {
    XanHashTableItem	*hashItem;

    /* Destroy the libraries loaded in memory.
     */
    hashItem	= xanHashTable_first(manager->libraries);
    while (hashItem != NULL) {
        DynamicLibrary		*loadedLib;
        XanHashTableItem	*hashItem2;

        /* Fetch the library loaded.
         */
        loadedLib	= hashItem->element;

        /* Destroy the symbols of the library.
         */
        hashItem2	= xanHashTable_first(loadedLib->symbols);
        while (hashItem2 != NULL) {
            DynamicLibrarySymbol	*sym;
            sym		= hashItem2->element;
            freeFunction(sym->name);
            freeFunction(sym);
            hashItem2	= xanHashTable_next(loadedLib->symbols, hashItem2);
        }

        /* Unload the library.
         */
        PLATFORM_dlclose(loadedLib->handle);

        /* Free the memory.
         */
        xanHashTable_destroyTable(loadedLib->symbols);
        freeFunction(loadedLib->name);
        freeFunction(loadedLib);

        /* Fetch the next element.
         */
        hashItem	= xanHashTable_next(manager->libraries, hashItem);
    }

    /* Free the memory.
     */
    xanHashTable_destroyTable(manager->libraries);

    return ;
}
