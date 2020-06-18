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
#include <base_symbol.h>

// My headers
#include <ir_method.h>
#include <ir_data.h>
// End

extern ir_lib_t * ir_library;

JITINT32 IRDATA_getSizeOfObject (ir_item_t *item) {

    /* Assertions				*/
    assert(item != NULL);

    /* Compute the size of the type		*/
    if (item->type_infos == NULL) {
        print_err("IRDATA_getSizeOfObject: The type is not known. ", 0);
        abort();
    }
    return ir_library->getTypeSize(item->type_infos);
}

JITBOOLEAN IRDATA_isAMethodEntryPoint (ir_item_t *item) {
    switch (item->internal_type) {
        case IRMETHODENTRYPOINT:
        case IRFPOINTER:
            return JITTRUE;
    }
    return JITFALSE;
}

JITINT32 IRDATA_getSizeOfIRType (JITUINT16 irType) {

    /* Compute the size of the type		*/
    if (IRMETHOD_isAPointerType(irType)) {
        return sizeof(JITNUINT);
    }
    switch (irType) {
        case IRLABELITEM:
        case IRVOID:
            return 0;
        case IRINT8:
        case IRUINT8:
            return 1;
        case IRINT16:
        case IRUINT16:
            return 2;
        case IRINT32:
        case IRUINT32:
        case IRFLOAT32:
            return 4;
        case IRINT64:
        case IRUINT64:
        case IRFLOAT64:
            return 8;
        case IRNINT:
            return sizeof(JITNINT);
        case IRNUINT:
            return sizeof(JITNUINT);
        case IRNFLOAT:
            return sizeof(JITNFLOAT);
    }
    print_err("IRDATA_getSizeOfIRType The type is not known. ", 0);
    abort();
}

JITINT32 IRDATA_getSizeOfType (ir_item_t *item) {
    JITUINT32 type;

    /* Assertions				*/
    assert(item != NULL);

    /* Fetch the type			*/
    type = item->internal_type;
    if (type == IRTYPE) {
        type = (JITUINT32) (item->value).v;
    }

    /* Compute the size of the type		*/
    if (IRMETHOD_isAPointerType(type)) {
        return sizeof(JITNUINT);
    }
    switch (type) {
        case IRLABELITEM:
        case IRVOID:
            return 0;
        case IRINT8:
        case IRUINT8:
            return 1;
        case IRINT16:
        case IRUINT16:
            return 2;
        case IRINT32:
        case IRUINT32:
        case IRFLOAT32:
            return 4;
        case IRINT64:
        case IRUINT64:
        case IRFLOAT64:
            return 8;
        case IRNINT:
            return sizeof(JITNINT);
        case IRNUINT:
            return sizeof(JITNUINT);
        case IRNFLOAT:
            return sizeof(JITNFLOAT);
        case IRVALUETYPE:
            if (item->type_infos == NULL) {
                break;
            }
            assert(item->type_infos != NULL);
            return ir_library->getTypeSize(item->type_infos);
    }
    fprintf(stderr, "IRDATA_getSizeOfType The type %u is not know.\n", type);
    abort();
}

IR_ITEM_VALUE IRDATA_getMethodIDFromItem (ir_item_t *item) {
    ir_item_t	methodID;

    if (!IRDATA_isAMethodID(item)) {
        return 0;
    }

    /* Resolve the symbol.
     */
    if (item->type == IRSYMBOL) {
        IRSYMBOL_resolveSymbolFromIRItem(item, &methodID);
    } else {
        memcpy(&methodID, item, sizeof(ir_item_t));
    }
    assert(!IRDATA_isASymbol(&methodID));

    return methodID.value.v;
}

JITBOOLEAN IRDATA_isAMethodID (ir_item_t *item) {
    return internal_isATaggedSymbol(item, METHOD_SYMBOL);
}

IR_ITEM_VALUE IRDATA_getIntegerValueOfConstant (ir_item_t *item) {
    return (item->value).v;
}

IR_ITEM_FVALUE IRDATA_getFloatValueOfConstant (ir_item_t *item) {
    return (item->value).f;
}

JITBOOLEAN IRDATA_isAConstant (ir_item_t *item) {
    return (item->type != IROFFSET) && (item->type != IRSYMBOL);
}

JITBOOLEAN IRDATA_isAVariable (ir_item_t *item) {
    return (item->type == IROFFSET);
}

IR_ITEM_VALUE IRDATA_getVariableID (ir_item_t *item) {
    return (item->value).v;
}

JITBOOLEAN internal_isATaggedSymbol (ir_item_t *item, JITUINT32 tag) {
    ir_symbol_t	*s;

    if (item == NULL) {
        return JITFALSE;
    }
    if (item->type != IRSYMBOL) {
        return JITFALSE;
    }
    s	= (ir_symbol_t *)(JITNUINT)(item->value).v;
    if (s == NULL) {
        return JITFALSE;
    }
    if (s->tag != tag) {
        return JITFALSE;
    }
    return JITTRUE;
}

JITBOOLEAN IRDATA_isAssignable (TypeDescriptor * typeID, TypeDescriptor *desiredTypeID) {
    return typeID->assignableTo(typeID, desiredTypeID);
}

JITBOOLEAN IRDATA_isSubtype (TypeDescriptor *superTypeID, TypeDescriptor * subTypeID) {
    return subTypeID->isSubtypeOf(subTypeID, superTypeID);
}

ir_method_t * IRDATA_getInitializationMethodOfILTypeFromIRItem (ir_item_t *item){
    ir_item_t   resolvedSymbol;
    ir_symbol_t *s;

    if (item == NULL){
        return NULL;
    }
    if (item->type_infos != NULL){
        return IRDATA_getInitializationMethodOfILType(item->type_infos);
    }
    if (!IRDATA_isASymbol(item)){
        return NULL;
    }

    /* Fetch the symbol.
     */
    s   = IRSYMBOL_getSymbol(item);
    if (s == NULL){
        return NULL;
    }

    /* Check if the symbol is about static memory.
     */
    if (s->tag == STATIC_OBJECT_SYMBOL){
        TypeDescriptor  *ilType;
        ilType  = s->data;
        if (ilType == NULL){
            return NULL;
        }
        return IRDATA_getInitializationMethodOfILType(ilType);
    }
    IRSYMBOL_resolveSymbolFromIRItem(item, &resolvedSymbol);
    return IRDATA_getInitializationMethodOfILType(resolvedSymbol.type_infos);
}

ir_method_t * IRDATA_getInitializationMethodOfILType (TypeDescriptor *ilType){
    MethodDescriptor    *md;
    ir_method_t         *m;
    JITINT8             *cctorName;

    /* Check whether we have a type to check.
     */
    if (ilType == NULL){
        return NULL;
    }

    /* Fetch the method descriptor of the cctor.
     */
    md          = ilType->getCctor(ilType);
    if (md == NULL){
        return NULL;
    }

    /* Fetch the name of the cctor method.
     */
    cctorName   = md->getSignatureInString(md);
    if (cctorName == NULL){
        return NULL;
    }

    /* Fetch the IR method with the same name.
     */
    m           = IRPROGRAM_getMethodWithSignatureInString(cctorName);

    return m;
}

JITBOOLEAN IRDATA_isASymbol (ir_item_t *item) {
    return (item->type == IRSYMBOL);
}
