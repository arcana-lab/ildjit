/*
 * Copyright (C) 2006 - 2012 Campanoni Simone, Ciancone AndreA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABIRITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <stdlib.h>
#include <metadata/metadata_types.h>
#include <compiler_memory_manager.h>

// My headers
#include <ir_internal_functions.h>
#include <ir_symbol.h>
#include <ir_method.h>
// End

ir_lib_t * ir_library;

void IRLibDestroy (ir_lib_t *irLib) {
    XanHashTableItem	*item;

    /* Destroy serial map.
     */
    item	= xanHashTable_first(irLib->serialMap);
    while (item != NULL) {
        ir_symbol_t	*s;
        s	= item->element;
        if (s->value != NULL) {
            if ((*s->value) != NULL) {
                freeFunction(*(s->value));
            }
            freeFunction(s->value);
        }
        freeFunction(s);
        item	= xanHashTable_next(irLib->serialMap, item);
    }
    xanHashTable_destroyTable(irLib->serialMap);

    /* Destroy cached serializations.
     */
    shutdownSymbols();
    xanHashTable_destroyTable(irLib->cachedSerialization);

    /* Destroy loaded symbols.
     */
    xanHashTable_destroyTable(irLib->loadedSymbols);

    /* Destroy globals.
     */
    item	= xanHashTable_first(irLib->globals);
    while (item != NULL) {
        ir_global_t	*g;
        g	= item->element;
        assert(g != NULL);
        freeFunction(g->initialValue);
        freeFunction(g->name);
        freeFunction(g);
        item	= xanHashTable_next(irLib->globals, item);
    }
    xanHashTable_destroyTable(irLib->globals);

    /* Destroy the deserialize map.
     */
    freeFunction(irLib->deserialMap);

    /* Erase the memory for the library.
     */
    memset(irLib, 0, sizeof(ir_lib_t));

    return ;
}

void IRLibNew (
    ir_lib_t * ir_lib,
    compilation_scheme_t compilation,
    ir_method_t *   (newIRMethod) (JITINT8 *name),
    void(createTrampoline) (ir_method_t * method),
    ir_method_t *   (*getIRMethod)(ir_method_t * method, IR_ITEM_VALUE irMethodID, JITBOOLEAN ensureIRTranslation),
    JITINT8 *       (*getProgramName)(void),
    void (*translateToMachineCode)(ir_method_t * method),
    JITINT32 (*run)(ir_method_t * method, void **args, void *returnArea),
    JITUINT32 (*getTypeSize)(TypeDescriptor *typeDescriptor),
    ir_method_t *   (*getEntryPoint)(void),
    ir_method_t *   (*getIRMethodFromEntryPoint)(void *entryPointAddress),
    IR_ITEM_VALUE (*getIRMethodID)(ir_method_t * method),
    XanList *       (*getIRMethods)(void),
    XanList *       (*getImplementationsOfMethod)(IR_ITEM_VALUE irMethodID),
    XanList *       (*getCompatibleMethods)(ir_signature_t * signature),
    void *          (*getFunctionPointer)(ir_method_t * method),
    ir_symbol_t *   (*loadSymbol)(JITUINT32 number),
    void (*addToSymbolToSave)(ir_symbol_t *symbol),
    ir_method_t *   (*getCallerOfMethodInExecution)(ir_method_t *m),
    void (*saveProgram)(JITINT8 *name),
    JITBOOLEAN (*hasIRBody)(ir_method_t *method),
    JITBOOLEAN (*unregisterMethod)(ir_method_t *method),
    void (*tagMethodAsProfiled)(ir_method_t *method),
    JITINT8 * (*getNativeMethodName) (JITINT8 *internalName),
    XanHashTable **staticMemorySymbolTable,
    XanList **ilBinaries
) {

    ir_lib->compilation		= compilation;
    ir_lib->unregisterMethod 	= unregisterMethod;
    ir_lib->newIRMethod 		= newIRMethod;
    ir_lib->createTrampoline = createTrampoline;
    ir_lib->getIRMethod = getIRMethod;
    ir_lib->getProgramName = getProgramName;
    ir_lib->translateToMachineCode = translateToMachineCode;
    ir_lib->run = run;
    ir_lib->getTypeSize = getTypeSize;
    ir_lib->getEntryPoint = getEntryPoint;
    ir_lib->getIRMethodFromEntryPoint = getIRMethodFromEntryPoint;
    ir_lib->getIRMethodID = getIRMethodID;
    ir_lib->getIRMethods = getIRMethods;
    ir_lib->getImplementationsOfMethod = getImplementationsOfMethod;
    ir_lib->getCompatibleMethods = getCompatibleMethods;
    ir_lib->getFunctionPointer = getFunctionPointer;
    ir_lib->loadSymbol = loadSymbol;
    ir_lib->addToSymbolToSave = addToSymbolToSave;
    ir_lib->getCallerOfMethodInExecution = getCallerOfMethodInExecution;
    ir_lib->getNativeMethodName	= getNativeMethodName;
    ir_lib->hasIRBody 		= hasIRBody;
    ir_lib->saveProgram	 	= saveProgram;
    ir_lib->serialMap 		= xanHashTable_new(11, 0, sharedAllocFunction, dynamicReallocFunction, freeFunction, (JITUINT32 (*)(void *))hashIRSymbol, (JITINT32 (*)(void *, void *))equalsIRSymbol);
    ir_lib->cachedSerialization	= xanHashTable_new(11, 0, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    ir_lib->globals 		= xanHashTable_new(11, 0, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    ir_lib->loadedSymbols		= xanHashTable_new(11, 0, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    ir_lib->maxSymbol 		= 1;
    ir_lib->deserialCount 		= DESERIAL_SIZE;
    ir_lib->deserialMap 		= sharedAllocFunction(ir_lib->deserialCount * sizeof(IRSymbolType));
    ir_lib->tagMethodAsProfiled 	= tagMethodAsProfiled;
    ir_lib->staticMemorySymbolTable	= staticMemorySymbolTable;
    ir_lib->ilBinaries		= ilBinaries;

    ir_library = ir_lib;

    return;
}
