/*
 * Copyright (C) 2006 - 2012 Campanoni Simone
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
#include <jitsystem.h>
#include <compiler_memory_manager.h>

// My headers
#include <ir_method.h>
// End

#define MAX(x, y) ((x) > (y) ? (x) : (y))

typedef struct {
    void		*mem;
    JITUINT32	memBytes;
    JITUINT32	tag;
    JITUINT32	symbolID;
} ir_symbol_serialization;

static inline void internal_changeSymbolMapSize (JITUINT32 newSize);

extern ir_lib_t * ir_library;

JITUINT32 hashIRSymbol (ir_symbol_t *symbol) {
    JITUINT32 seed = (JITUINT32) (JITNUINT)symbol->data;

    seed = combineHash(seed, (JITUINT32) symbol->tag);
    return seed;
}

JITINT32 equalsIRSymbol (ir_symbol_t *symbol1, ir_symbol_t *symbol2) {
    return symbol1->data == symbol2->data  && symbol1->tag == symbol2->tag;
}

void shutdownSymbols (void) {
    XanHashTableItem	*item;
    ir_symbol_serialization	*c;

    item	= xanHashTable_first(ir_library->cachedSerialization);
    while (item != NULL) {
        c	= item->element;
        assert(c != NULL);
        freeFunction(c->mem);
        freeFunction(c);
        item	= xanHashTable_next(ir_library->cachedSerialization, item);
    }

    return ;
}

ir_symbol_t * IRSYMBOL_createSymbol (JITUINT32 tag, void *data) {
    ir_symbol_t symbol;

    symbol.tag = tag;
    symbol.data = data;
    xanHashTable_lock(ir_library->serialMap);
    ir_symbol_t *found = xanHashTable_lookup(ir_library->serialMap, &symbol);
    if (found == NULL) {
        found 		= sharedAllocFunction(sizeof(ir_symbol_t));
        found->value 	= MEMORY_obtainPrivateAddress();
        assert(*(found->value) == NULL);
        found->data 	= data;
        found->tag 	= tag;
        found->id 	= ir_library->maxSymbol++;
        xanHashTable_insert(ir_library->serialMap, (void *) found, (void *) found);
        ir_library->addToSymbolToSave(found);
    }
    xanHashTable_unlock(ir_library->serialMap);

    return found;
}

void * IRSYMBOL_unresolveSymbolFromIRItem (ir_item_t *item) {
    void		*v;

    /* Assertions.
     */
    assert(item != NULL);

    /* Initialie the variables.
     */
    v	= NULL;

    /* Check if the IR item includes an IR symbol.
     */
    if (IRDATA_isASymbol(item)) {
        ir_symbol_t	*sym;

        /* Fetch the symbol.
         */
        sym	= (ir_symbol_t *)(JITNUINT)(item->value).v;
        if (sym != NULL) {
            v	= IRSYMBOL_unresolveSymbol(sym);
        }
    }

    return v;
}

void * IRSYMBOL_unresolveSymbol (ir_symbol_t *symbol) {

    /* Assertions.
     */
    assert(symbol != NULL);

    return symbol->data;
}

ir_value_t IRSYMBOL_resolveSymbol (ir_symbol_t *symbol) {

    /* Assertions.
     */
    assert(symbol != NULL);

    /* symbol->value is using a private address, so *(symbol->value) == NULL means it has not been resolved */
    if ((*(symbol->value)) == NULL) {
        (*(symbol->value)) 	= allocFunction(sizeof(ir_value_t));
        (**(symbol->value)) 	= (ir_library->deserialMap[symbol->tag]).resolve(symbol);
    }
    assert((*(symbol->value)) != NULL);

    return **(symbol->value);
}

void IRSYMBOL_resolveSymbolFromIRItem (ir_item_t *item, ir_item_t *resolvedSymbol) {

    /* Assertions.
     */
    assert(item != NULL);
    assert(resolvedSymbol != NULL);
    assert(item != resolvedSymbol);

    /* Initialize the variables.
     */
    memset(resolvedSymbol, 0, sizeof(ir_item_t));

    /* Resolve the symbol.
     */
    resolvedSymbol->value = IRSYMBOL_resolveSymbol((ir_symbol_t *) (JITNUINT) (item->value).v);
    resolvedSymbol->type = item->internal_type;
    resolvedSymbol->internal_type = resolvedSymbol->type;
    resolvedSymbol->type_infos = item->type_infos;

    return ;
}

void IRSYMBOL_storeSymbolToInstructionParameter (ir_method_t *method, ir_instruction_t *inst, JITUINT8 toParam, ir_symbol_t *symbol, JITUINT16 internal_type, TypeDescriptor *typeInfos) {
    ir_item_t	*par;

    /* Assertions.
     */
    assert(method != NULL);
    assert(IRMETHOD_doesInstructionBelongToMethod(method, inst));

    /* Set the modified flag.
     */
    method->modified	= JITTRUE;

    /* Fetch the parameter.
     */
    par	= IRMETHOD_getInstructionParameter(inst, toParam);
    if (par == NULL) {
        fprintf(stderr, "ERROR: IRSYMBOL_storeSymbolToInstructionParameter: IR parameter %u does not exist.\n", toParam);
        abort();
    }

    /* Store the symbol.
     */
    IRSYMBOL_storeSymbolToIRItem(par, symbol, internal_type, typeInfos);

    return ;
}

void IRSYMBOL_storeSymbolToIRItem (ir_item_t *item, ir_symbol_t *symbol, JITUINT16 internal_type, TypeDescriptor *typeInfos) {

    /* Assertions.
     */
    assert(item != NULL);
    assert(symbol != NULL);

    /* Initialize the variables.
     */
    memset(item, 0, sizeof(ir_item_t));

    /* Store the symbol.
     */
    (item->value).v		= (IR_ITEM_VALUE)(JITNUINT) symbol;
    item->type		= IRSYMBOL;
    item->internal_type	= internal_type;
    item->type_infos	= typeInfos;

    return ;
}

JITBOOLEAN IRSYMBOL_isSerializationOfSymbolCached (JITUINT32 symbolID) {
    if (xanHashTable_lookup(ir_library->cachedSerialization, (void *)(JITNUINT) (symbolID + 1)) != NULL) {
        return JITTRUE;
    }

    return JITFALSE;
}

void IRSYMBOL_getSerializationOfSymbolCached (JITUINT32 symbolID, JITUINT32 *tag, void **mem, JITUINT32 *memBytes) {
    ir_symbol_serialization	*c;

    /* Fetch the serialization.
     */
    c	= xanHashTable_lookup(ir_library->cachedSerialization, (void *)(JITNUINT) (symbolID + 1));
    if (c == NULL) {
        return ;
    }

    /* Store the serialization.
     */
    (*tag)		= c->tag;
    (*memBytes)	= c->memBytes;
    (*mem)		= allocFunction(c->memBytes);
    memcpy(*mem, c->mem, c->memBytes);

    return ;
}

void IRSYMBOL_serializeSymbol (ir_symbol_t *symbol, void **mem, JITUINT32 *memBytesAllocated) {

    /* Assertions.
     */
    assert(symbol != NULL);
    assert(mem != NULL);
    assert(memBytesAllocated != NULL);

    /* The serialization of the symbol has not been cached.
     * Serialize the symbol.
     */
    (ir_library->deserialMap[symbol->tag]).serialize(symbol, mem, memBytesAllocated);

    return ;
}

void IRMETHOD_dumpSymbol (ir_symbol_t *symbol, FILE *fileToWrite) {
    (ir_library->deserialMap[symbol->tag]).dump(symbol, fileToWrite);
}

XanHashTable * IRSYMBOL_getLoadedSymbols (void) {
    return ir_library->loadedSymbols;
}

ir_symbol_t * IRSYMBOL_loadSymbol (JITUINT32 number) {
    ir_symbol_t	*s;

    s	= xanHashTable_lookup(ir_library->loadedSymbols, (void *)(JITNUINT)(number + 1));
    if (s == NULL) {
        s	= ir_library->loadSymbol(number);
        xanHashTable_insert(ir_library->loadedSymbols, (void *)(JITNUINT)(number+1), s);
    }
    assert(s != NULL);

    return s;
}

ir_symbol_t * IRSYMBOL_getSymbol (ir_item_t *item) {
    ir_symbol_t	*s;

    if (item->type != IRSYMBOL) {
        return NULL;
    }
    s	= (ir_symbol_t *)(JITNUINT)(item->value).v;

    return s;
}

JITBOOLEAN IRSYMBOL_hasSymbolIdentifier (ir_symbol_t *symbol) {
    return (symbol->symbolIdentifier > 0);
}

JITUINT32 IRSYMBOL_getSymbolIdentifier (ir_symbol_t *symbol) {
    if (symbol->symbolIdentifier == 0) {
        fprintf(stderr, "ILDJIT: ERROR = Symbol %u does not have a symbol identifier because it does not have a file associated with it.\n", symbol->id);
        abort();
    }

    return symbol->symbolIdentifier;
}

void IRSYMBOL_cacheSymbolSerialization (JITUINT32 symbolID, JITUINT32 tag, void *mem, JITUINT32 memBytes) {
    ir_symbol_serialization	*c;

    /* Allocate the cached serialization.
     */
    c		= allocFunction(sizeof(ir_symbol_serialization));
    c->symbolID	= symbolID;
    c->tag		= tag;
    c->memBytes	= memBytes;
    c->mem		= allocFunction(memBytes + 1);
    memcpy(c->mem, mem, memBytes);

    /* Keep track of the serialization.
     */
    xanHashTable_insert(ir_library->cachedSerialization, (void *)(JITNUINT) (symbolID + 1), c);

    return ;
}

ir_symbol_t * IRSYMBOL_deserializeSymbol (JITUINT32 tag, void *mem, JITUINT32 memBytes) {
    ir_symbol_t	*s;

    s	= (ir_library->deserialMap[tag]).deserialize(mem, memBytes);
    assert(s != NULL);

    return s;
}

JITBOOLEAN IRSYMBOL_isSymbolRegistered (JITUINT32 tag) {
    if (tag >= ir_library->deserialCount) {
        return JITFALSE;
    }
    if (	(ir_library->deserialMap[tag].deserialize != NULL)	||
            (ir_library->deserialMap[tag].serialize != NULL)	||
            (ir_library->deserialMap[tag].resolve != NULL)		||
            (ir_library->deserialMap[tag].dump != NULL)		) {
        return JITTRUE;
    }
    return JITFALSE;
}

JITBOOLEAN IRSYMBOL_registerSymbolManager (JITUINT32 tag, ir_value_t (*resolve)(ir_symbol_t *symbol), void (*serialize)(ir_symbol_t *sym, void **mem, JITUINT32 *memBytesAllocated), void (*dump)(ir_symbol_t *symbol, FILE *fileToWrite), ir_symbol_t * (*deserialize)(void *mem, JITUINT32 memBytes)) {

    /* Check if there is enough space to register the caller.
     */
    if (tag >= ir_library->deserialCount) {
        internal_changeSymbolMapSize(MAX(tag, 2 * ir_library->deserialCount));
    }

    /* Check if someone else already register for the same symbol type.
     */
    if (IRSYMBOL_isSymbolRegistered(tag)) {

        /* Check if the current registration differs with the old one.
         */
        if (	(ir_library->deserialMap[tag].deserialize != deserialize)	||
                (ir_library->deserialMap[tag].serialize != serialize)		||
                (ir_library->deserialMap[tag].resolve != resolve)		||
                (ir_library->deserialMap[tag].dump != dump)			) {
            abort();
            return JITFALSE;
        }
        return JITTRUE;
    }

    /* Register the caller.
     */
    ir_library->deserialMap[tag].deserialize 	= deserialize;
    ir_library->deserialMap[tag].serialize 		= serialize;
    ir_library->deserialMap[tag].resolve 		= resolve;
    ir_library->deserialMap[tag].dump 		= dump;

    return JITTRUE;
}

XanHashTable * IRSYMBOL_getSymbolTableForStaticMemories (void) {
    if (ir_library->staticMemorySymbolTable == NULL) {
        return NULL;
    }
    return *(ir_library->staticMemorySymbolTable);
}

JITUINT32 IRSYMBOL_getFreeSymbolTag (void) {
    JITUINT32	prevCount;

    /* Check a free tag.
     */
    prevCount	= ir_library->deserialCount;
    for (JITUINT32 count=0; count < prevCount; count++) {
        if (!IRSYMBOL_isSymbolRegistered(count)) {
            return count;
        }
    }

    /* Increase the size.
     */
    internal_changeSymbolMapSize(prevCount * 2);
    assert(!IRSYMBOL_isSymbolRegistered(prevCount));

    return prevCount;
}

static inline void internal_changeSymbolMapSize (JITUINT32 newSize) {
    JITUINT32	prevCount;

    if (newSize <= ir_library->deserialCount) {
        return ;
    }

    prevCount			= ir_library->deserialCount;
    ir_library->deserialCount 	= newSize;
    ir_library->deserialMap 	= dynamicReallocFunction(ir_library->deserialMap, ir_library->deserialCount * sizeof(IRSymbolType));
    for (JITUINT32 count = prevCount; count < ir_library->deserialCount; count++) {
        memset(&(ir_library->deserialMap[count]), 0, sizeof(IRSymbolType));
    }

    return ;
}
