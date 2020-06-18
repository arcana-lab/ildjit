/*
 * Copyright (C) 2009  Simone Campanoni, Luca Rocchini
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

#include <compiler_memory_manager.h>
#include <metadata_types_manager.h>
#include <basic_concept_manager.h>
#include <type_spec_descriptor.h>
#include <type_spec_decoding_tool.h>
#include <iljitmm-system.h>
#include <stdlib.h>
#include <assert.h>
#include <platform_API.h>

#define VTABLE_TICKET 0
#define NAME_TICKET 1
#define COMPLETE_NAME_TICKET 2

#define TYPE_TICKET_COUNT 3

static inline GenericSpecContainer *internalGetSuperTypeGenericSpecContainer (TypeDescriptor *type);
static inline XanList *getInterfacesInformationFromTypeDescriptor (TypeDescriptor *type, GenericSpecContainer *container);

static inline VTable *internalPrepareVTable (TypeDescriptor *type);
static inline XanList *internalGetExplictOverrideInformations (TypeDescriptor *type);
static inline XanHashTable *internalPrepareGenericMap (XanHashTable *mapper, TypeDescriptor *type);
static inline VTableSlot *internalCloneVTableSlot (VTableSlot *slot, XanHashTable *source, XanHashTable *dest);
static inline XanHashTable *internalPrepareSlots (XanHashTable *table);
static inline void internalPrepareInterfacesSlots (TypeDescriptor *interfaceDescription, GenericSpecContainer *specContainer, TypeDescriptor *owner, VTable *table);
static inline MethodInformation *internalFindMethodInOverride (MethodInformation *method, VTable *table, JITUINT32 flags, JITBOOLEAN isInterface);
static inline void internalLayoutMethods (TypeDescriptor *type, VTable *table);
static inline void internalLayoutInterface (TypeDescriptor *type, VTable *table);
static inline void internalApplyExplicitOverride (TypeDescriptor *type, VTable *table);
static inline void internalApplyTypeCompatibility (TypeDescriptor *type);

static inline void internalInsertIntoTypeCompatibilty (TypeElem *bucket, TypeDescriptor *type, JITBOOLEAN value);
static inline JITBOOLEAN internalGetFromTypeCompatibilty (TypeElem *bucket, TypeDescriptor *type, JITBOOLEAN *value);


/* VTable computation helper */

static inline VTable *internalPrepareVTable (TypeDescriptor *type) {
    if (!WAIT_TICKET(type, VTABLE_TICKET)) {
        TypeDescriptor *superType = getSuperTypeFromTypeDescriptor(type);
        VTable *result = sharedAllocFunction(sizeof(VTable));
        /* following field is used to compute vtable*/
        if (superType == NULL) {
            result->interfacesSet = xanList_new(sharedAllocFunction, freeFunction, NULL);
            result->interfacesMethods = xanList_new(sharedAllocFunction, freeFunction, NULL);
            result->methodsList = xanList_new(sharedAllocFunction, freeFunction, NULL);
            result->genericParameterMap = xanHashTable_new(11, 0, sharedAllocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
            result->slots = xanHashTable_new(11, 0, sharedAllocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
            result->length = 0;
        } else {
            VTable *elem = internalPrepareVTable(superType);
            result->genericParameterMap = internalPrepareGenericMap(elem->genericParameterMap, type);
            result->slots = internalPrepareSlots(elem->slots);
            result->interfacesSet = xanList_cloneList(elem->interfacesSet);
            result->interfacesMethods = xanList_cloneList(elem->interfacesMethods);
            result->methodsList = xanList_cloneList(elem->methodsList);
            result->length = elem->length;
        }
        /* update interfacesMethods & interfaceset with information from direct declared interface */
        internalPrepareInterfacesSlots(type, NULL, type, result);
        /* update Layout with new method */
        internalLayoutMethods(type, result);
        /* update layout of interfaces */
        internalLayoutInterface(type, result);
        /* Apply explicit override rules */
        internalApplyExplicitOverride(type, result);
        /* update TypeCompatibilty to speedup cast check */
        internalApplyTypeCompatibility(type);

        type->vTable = (void *) result;
        BROADCAST_TICKET(type, VTABLE_TICKET);
    }
    return (VTable *) type->vTable;
}

static inline void internalApplyTypeCompatibility (TypeDescriptor *type) {
    TypeDescriptor *parent = type;

    while (parent != NULL) {
        internalInsertIntoTypeCompatibilty(type->compatibility, parent, JITTRUE);
        parent = getSuperTypeFromTypeDescriptor(parent);
    }
}

static inline XanList *internalGetExplictOverrideInformations (TypeDescriptor *type) {
    XanList *result;

    switch (type->element_type) {
        case ELEMENT_ARRAY:
        case ELEMENT_FNPTR:
        case ELEMENT_PTR:
        case ELEMENT_VAR:
        case ELEMENT_MVAR:
            result = xanList_new(sharedAllocFunction, freeFunction, NULL);
            break;
        case ELEMENT_CLASS:
            result = getExplictOverrideInformationsFromClassDescriptor((ClassDescriptor *) type->descriptor);
            break;
        default:
            PDEBUG("METADATA MANAGER: internalGetExplictOverrideInformations: ERROR! INVALID TYPE! %d\n", type->element_type);
            abort();
    }
    return result;
}

static inline GenericSpecContainer *internalGetSuperTypeGenericSpecContainer (TypeDescriptor *type) {
    GenericSpecContainer *result = NULL;

    switch (type->element_type) {
        case ELEMENT_VAR:
        case ELEMENT_MVAR:
        case ELEMENT_FNPTR:
        case ELEMENT_PTR:
        case ELEMENT_ARRAY:
            result = NULL;
            break;
        case ELEMENT_CLASS: {
            ClassDescriptor *class = (ClassDescriptor *) type->descriptor;
            result = getSuperClassGenericSpecContainer(class);
        }
        break;
        default:
            PDEBUG("METADATA MANAGER: internalGetSuperTypeGenericSpecContainer: ERROR! INVALID TYPE! %d\n", type->element_type);
            abort();
            break;
    }
    return result;

}

static inline XanHashTable *internalPrepareGenericMap (XanHashTable *mapper, TypeDescriptor *type) {
    GenericSpecContainer *specContainer = internalGetSuperTypeGenericSpecContainer(type);

    XanHashTable *result = xanHashTable_new(11, 0, sharedAllocFunction, dynamicReallocFunction, freeFunction, hashGenericParameterRule, equalsGenericParameterRule);
    XanHashTableItem *item = xanHashTable_first(mapper);

    while (item != NULL) {
        GenericParameterRule *pattern = (GenericParameterRule *) item->element;
        GenericParameterRule *new_pattern = sharedAllocFunction(sizeof(GenericParameterRule));
        new_pattern->owner = pattern->owner;
        new_pattern->var = pattern->var;
        new_pattern->replace = replaceTypeSpecDescriptor(pattern->replace, specContainer);
        xanHashTable_insert(result, (void *) new_pattern, (void *) new_pattern);
        item = xanHashTable_next(mapper, item);
    }

    destroyGenericSpecContainer(specContainer);

    /* update Generic Map with Generic Parameter defined by class itself */
    JITUINT32 count;
    for (count = 0; count < type->attributes->param_count; count++) {
        GenericParameterRule *new_pattern = sharedAllocFunction(sizeof(GenericParameterRule));
        GenericVarSpecDescriptor *varDescriptor = newGenericVarSpecDescriptor(count);
        new_pattern->replace = newTypeSpecDescriptor(ELEMENT_VAR, varDescriptor, JITFALSE);
        new_pattern->owner = type;
        new_pattern->var = count;
        xanHashTable_insert(result, (void *) new_pattern, (void *) new_pattern);
    }

    return result;
}

static inline VTableSlot *internalCloneVTableSlot (VTableSlot *slot, XanHashTable *source, XanHashTable *dest) {
    VTableSlot *new_slot = xanHashTable_lookup(dest, slot->declaration);

    if (new_slot == NULL) {
        new_slot = sharedAllocFunction(sizeof(VTableSlot));
        new_slot->declaration = slot->declaration;
        new_slot->body = slot->body;
        new_slot->explicitSlot = slot->explicitSlot;
        new_slot->index = slot->index;
        if (slot->overriden != NULL) {
            new_slot->overriden = internalCloneVTableSlot(slot->overriden, source, dest);
        } else {
            new_slot->overriden = NULL;
        }
        xanHashTable_insert(dest, new_slot->declaration, new_slot);
    }
    return new_slot;
}

static inline XanHashTable *internalPrepareSlots (XanHashTable *table) {
    XanHashTable *result = xanHashTable_new(11, 0, sharedAllocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    XanHashTableItem *item = xanHashTable_first(table);

    while (item != NULL) {
        VTableSlot *current_slot = (VTableSlot *) item->element;
        internalCloneVTableSlot(current_slot, table, result);
        item = xanHashTable_next(table, item);
    }
    return result;
}

static inline XanList *getInterfacesInformationFromTypeDescriptor (TypeDescriptor *type, GenericSpecContainer *container) {
    XanList *result;

    switch (type->element_type) {
        case ELEMENT_ARRAY:
        case ELEMENT_FNPTR:
        case ELEMENT_PTR:
        case ELEMENT_VAR:
        case ELEMENT_MVAR:
            result = xanList_new(allocFunction, freeFunction, NULL);
            break;
        case ELEMENT_CLASS:
            result = getInterfacesInformationFromClassDescriptor((ClassDescriptor *) type->descriptor, container);
            break;
        default:
            PDEBUG("METADATA MANAGER: getInterfacesInformationFromTypeDescriptor: ERROR! INVALID TYPE! %d\n", type->element_type);
            abort();
    }
    return result;
}

static inline void internalPrepareInterfacesSlots (TypeDescriptor *type, GenericSpecContainer *container, TypeDescriptor *owner, VTable *table) {
    XanList *list;

    /* Fetch the interfaces.
     */
    list 	= getInterfacesInformationFromTypeDescriptor(type, container);
    assert(list != NULL);

    /* Prepare the interface slots.
     */
    XanListItem *item = xanList_first(list);
    while (item != NULL) {
        InterfaceInformation 	*information;

        information 	= (InterfaceInformation *) item->data;
        assert(information != NULL);

        if (xanList_find(table->interfacesSet, information->interfaceDescription) == NULL) {
            XanList *methods = getDirectDeclaredMethodsTypeDescriptor(information->interfaceDescription);
            XanListItem *method_item = xanList_first(methods);
            while (method_item != NULL) {
                MethodDescriptor *current_method = (MethodDescriptor *) method_item->data;
                /* Prepare Structure to speed Up signature Comparison */
                MethodInformation *map = sharedAllocFunction(sizeof(MethodInformation));
                map->declaration = current_method;
                map->owner = owner;
                map->signature = getRawSignature(current_method, information->container);
                xanList_append(table->interfacesMethods, (void *) map);
                /*Prepare VTable Slot */
                VTableSlot *slot = sharedAllocFunction(sizeof(VTableSlot));
                slot->declaration = current_method;
                slot->body = NULL;
                slot->index = 0;
                slot->explicitSlot = JITFALSE;
                xanHashTable_insert(table->slots, current_method, (void *) slot);
                /* iterate on next method */
                method_item = method_item->next;
            }
            xanList_append(table->interfacesSet, information->interfaceDescription);
            internalPrepareInterfacesSlots(information->interfaceDescription, information->container, owner, table);
        }
        item = item->next;
    }

    /* Free the memory.
     */
    xanList_destroyListAndData(list);

    return ;
}

static inline MethodInformation *internalFindMethodInOverride (MethodInformation *method, VTable *table, JITUINT32 flags, JITBOOLEAN isInterface) {
    MethodInformation *result = NULL;
    JITINT8* name = getNameFromMethodDescriptor(method->declaration);
    XanListItem *item = xanList_last(table->methodsList);

    while (item != NULL) {
        MethodInformation *parent_method = (MethodInformation *) item->data;
        if (flags == 0) {
            if (method->declaration->owner == parent_method->declaration->owner) {
                /* Check only upper class in Heiarchy */
                item = item->prev;
                continue;
            }
        }
        if (flags == 1) {
            if ( !parent_method->declaration->attributes->is_virtual || !parent_method->declaration->attributes->need_new_slot ) {
                /* See ECMA-335 section 12.2 Partition II*/
                item = item->prev;
                continue;
            }
        }
        if (flags == 2) {
            if (!parent_method->declaration->attributes->is_virtual) {
                /* See ECMA-335 section 12.2 Partition II*/
                item = item->prev;
                continue;
            }
        }
        JITINT8* back_name = getNameFromMethodDescriptor(parent_method->declaration);
        if (STRCMP(name, back_name) == 0) {
            PDEBUG("METADATA MANAGER: internalFindMethodInOverride: %s equals to %s\n", getCompleteNameFromMethodDescriptor(method->declaration), getCompleteNameFromMethodDescriptor(parent_method->declaration));
            if (isInterface) {
                if (equalsWithMapperFunctionPointerSpecDescriptor(method->signature, method->owner, table->genericParameterMap, parent_method->signature)) {
                    result = parent_method;
                    break;
                }
            } else {
                if (equalsWithMapperFunctionPointerSpecDescriptor(parent_method->signature, parent_method->owner, table->genericParameterMap, method->signature)) {
                    result = parent_method;
                    break;
                }
            }
        }
        item = item->prev;
    }
    return result;
}

static inline void internalLayoutMethods (TypeDescriptor *type, VTable *table) {
    XanList *methods = getDirectDeclaredMethodsTypeDescriptor(type);
    XanListItem *item = xanList_first(methods);

    while (item != NULL) {
        MethodDescriptor *current_method = (MethodDescriptor *) item->data;
        if (current_method->attributes->is_virtual) {
            PDEBUG("METADATA MANAGER: internalLayoutMethods: Method = %s\n", current_method->getCompleteName(current_method));
            /* Init VTable Slot Structure */
            VTableSlot *slot = sharedAllocFunction(sizeof(VTableSlot));
            slot->declaration = current_method;
            slot->body = current_method;
            slot->explicitSlot = JITFALSE;
            /* Prepare Structure to speed Up signature Comparison */
            MethodInformation *map = sharedAllocFunction(sizeof(MethodInformation));
            map->declaration = current_method;
            map->owner = type;
            map->signature = getRawSignature(current_method, NULL);
            if (current_method->attributes->need_new_slot) {
                /* method always need a new slot */
                PDEBUG("METADATA MANAGER: internalLayoutMethods: METHOD NEED NEW SLOT! \n");
                if (current_method->attributes->param_count == 0) {
                    slot->index = (table->length)++;
                } else {
                    slot->index = 0;
                }
                slot->overriden = NULL;
            } else {
                /* check for overriden method */
                MethodInformation *found = internalFindMethodInOverride(map, table, 0, JITFALSE);
                if (found == NULL) {
                    PDEBUG("METADATA MANAGER: internalLayoutMethods: NOT EXIST EQUAL USE NEW SLOT! \n");
                    /* no method overriden assign a new slot */
                    if (current_method->attributes->param_count == 0) {
                        slot->index = (table->length)++;
                    } else {
                        slot->index = 0;
                    }
                    slot->overriden = NULL;
                } else {
                    /* update slot */
                    PDEBUG("METADATA MANAGER: internalLayoutMethods: METHOD IS EQUAL! REUSE SLOT\n");
                    VTableSlot *overriden_slot = xanHashTable_lookup(table->slots, found->declaration);
                    slot->index = overriden_slot->index;
                    slot->overriden = overriden_slot;
                    while (overriden_slot != NULL) {
                        overriden_slot->body = current_method;
                        overriden_slot = overriden_slot->overriden;
                    }
                }
            }
            xanHashTable_insert(table->slots, (void *) current_method, (void *) slot);
            xanList_append(table->methodsList, (void *) map);
        }
        item = item->next;
    }
}


static inline void internalLayoutInterface (TypeDescriptor *type, VTable *table) {
    XanListItem *item = xanList_first(table->interfacesMethods);

    while (item != NULL) {
        MethodInformation *current_method_information = (MethodInformation *) item->data;
        if (!current_method_information->declaration->attributes->is_static) {
            PDEBUG("METADATA MANAGER: internalLayoutInterface: [Interface]Method = %s\n", getCompleteNameFromMethodDescriptor(current_method_information->declaration));
            VTableSlot *slot = xanHashTable_lookup(table->slots, (void *) current_method_information->declaration);
            if (!slot->explicitSlot) {
                MethodInformation *equal_method = internalFindMethodInOverride(current_method_information, table, 1, JITTRUE);
                if (equal_method == NULL) {
                    equal_method = internalFindMethodInOverride(current_method_information, table, 2, JITTRUE);
                }
                if (equal_method == NULL) {
                    equal_method = internalFindMethodInOverride(current_method_information, table, 3, JITTRUE);
                }
                if (equal_method != NULL) {
                    PDEBUG("METADATA MANAGER: internalLayoutInterface: [Interface]Method found = %s\n", getCompleteNameFromMethodDescriptor(equal_method->declaration));
                    VTableSlot *equal_method_slot = xanHashTable_lookup(table->slots, equal_method->declaration);
                    slot->index = equal_method_slot->index;
                    slot->body = equal_method_slot->body;
                } else {
                    PDEBUG("METADATA MANAGER: internalLayoutInterface: No [Interface]Method found\n");
                }
            } else {
                PDEBUG("METADATA MANAGER: internalLayoutInterface: No [Interface]Method found\n");
            }
        }
        item = item->next;
    }
}


static inline void internalApplyExplicitOverride (TypeDescriptor *type, VTable *table) {
    XanList *explicitOverriding = internalGetExplictOverrideInformations(type);
    XanListItem *item = xanList_first(explicitOverriding);

    while (item != NULL) {
        ExplicitOverrideInformation *explicit = (ExplicitOverrideInformation *) item->data;
        PDEBUG("METADATA MANAGER: internalApplyExplicitOverride: Lookup for method %s\n", getCompleteNameFromMethodDescriptor(explicit->declaration));
        VTableSlot *found = (VTableSlot *) xanHashTable_lookup(table->slots, explicit->declaration);
        PDEBUG("METADATA MANAGER: internalApplyExplicitOverride: update method with %s\n", getCompleteNameFromMethodDescriptor(explicit->body));
        VTableSlot *overriding_slot = (VTableSlot *) xanHashTable_lookup(table->slots, explicit->body);
        while (found != NULL) {
            found->body = overriding_slot->body;
            found->index = overriding_slot->index;
            found->explicitSlot = JITTRUE;
            found = found->overriden;
        }
        item = item->next;
    }
    xanList_destroyListAndData(explicitOverriding);
}

inline XanHashTable *getVTableFromTypeDescriptor (TypeDescriptor *type, JITUINT32 *vTableLength) {
    VTable *table = internalPrepareVTable(type);

    if (vTableLength != NULL) {
        (*vTableLength) = table->length;
    }
    return table->slots;
}

/* TypeDescriptor Accessor Methods */
JITUINT32 hashTypeDescriptor (TypeDescriptor *type) {
    if (type == NULL) {
        return 0;
    }
    JITUINT32 seed = (JITUINT32) (JITNUINT)type->descriptor;
    seed = combineHash(seed, type->isByRef);
    return combineHash(seed, type->element_type);
}

JITINT32 equalsTypeDescriptor (TypeDescriptor *key1, TypeDescriptor *key2) {
    if (key1 == key2) {
        return 1;
    }
    if (key1 == NULL || key2 == NULL) {
        return 0;
    }
    return key1->element_type != key2->element_type && key1->descriptor == key2->descriptor && key1->isByRef == key2->isByRef;
}

inline TypeDescriptor *newTypeDescriptor (metadataManager_t *manager, ILElemType element_type, void * descriptor, JITBOOLEAN isByRef) {
    if (descriptor == NULL) {
        return NULL;
    }

    initializeMetadata();

    TypeDescriptor *result = sharedAllocFunction(sizeof(TypeDescriptor));

    pthread_mutexattr_t mutex_attr;
    PLATFORM_initMutexAttr(&mutex_attr);
    PLATFORM_setMutexAttr_type(&mutex_attr, PTHREAD_MUTEX_ADAPTIVE_NP);
    MEMORY_shareMutex(&mutex_attr);
    PLATFORM_initMutex(&(result->mutex), &mutex_attr);
    result->tickets = allocTicketArray(TYPE_TICKET_COUNT);

    result->manager = manager;
    result->name = NULL;
    result->completeName = NULL;
    result->isByRef = isByRef;
    result->element_type = element_type;
    result->descriptor = descriptor;
    result->compatibility = sharedAllocFunction(sizeof(TypeElem) * TYPE_COMP_SIZE);
    memset(result->compatibility, (JITUINT32) NULL, sizeof(TypeElem) * TYPE_COMP_SIZE);

    switch (element_type) {
        case ELEMENT_FNPTR:
        case ELEMENT_PTR:
            result->attributes = manager->System_IntPtr->attributes;
        case ELEMENT_ARRAY:
            result->attributes = &(manager->arrayAttributes);
            break;
        case ELEMENT_CLASS:
            result->attributes = ((ClassDescriptor*) descriptor)->attributes;
            break;
        case ELEMENT_VAR:
        case ELEMENT_MVAR:
            break;
        default:
            PDEBUG("METADATA MANAGER: getUnderliningClassDescriptorFromTypeDescriptor: ERROR! INVALID TYPE! %d\n", element_type);
            abort();
    }

    result->lock = typeDescriptorLock;
    result->unlock = typeDescriptorUnLock;

    result->createDescriptorMetadataTicket = typeDescriptorCreateDescriptorMetadataTicket;
    result->broadcastDescriptorMetadataTicket = typeDescriptorBroadcastDescriptorMetadataTicket;

    result->getResolvingContainer = getResolvingContainerFromTypeDescriptor;

    result->isValueType = isValueTypeTypeDescriptor;
    result->isEnum = isEnumTypeDescriptor;
    result->isDelegate = isDelegateTypeDescriptor;
    result->isNullable = isNullableTypeDescriptor;
    result->isPrimitive = isPrimitiveTypeDescriptor;

    result->getSuperType = getSuperTypeFromTypeDescriptor;
    result->isSubtypeOf = typeDescriptorIsSubtypeOf;
    result->assignableTo = typeDescriptorAssignableTo;

    result->getVTable = getVTableFromTypeDescriptor;

    result->getImplementedInterfaces = getImplementedInterfacesFromTypeDescriptor;

    result->getEnclosing = getEnclosingType;
    result->getNestedType = getNestedType;
    result->getNestedTypeFromName = getNestedTypeFromName;

    result->getCctor = getCctorTypeDescriptor;
    result->getFinalizer = getFinalizerTypeDescriptor;
    result->getConstructors = getConstructorsTypeDescriptor;
    result->getDirectDeclaredMethods = getDirectDeclaredMethodsTypeDescriptor;
    result->getMethodFromName = getMethodDescriptorFromNameTypeDescriptor;
    result->implementsMethod = typeDescriptorImplementsMethod;

    result->getDirectImplementedFields = getDirectImplementedFieldsTypeDescriptor;
    result->getFieldFromName = getFieldDescriptorFromNameTypeDescriptor;

    result->getEvents = getEventsTypeDescriptor;
    result->getEventFromName = getEventDescriptorFromNameTypeDescriptor;

    result->getProperties = getPropertiesTypeDescriptor;
    result->getPropertyFromName = getPropertyDescriptorFromNameTypeDescriptor;

    result->getIRType = getIRTypeFromTypeDescriptor;

    result->getName = getNameFromTypeDescriptor;
    result->getCompleteName = getCompleteNameFromTypeDescriptor;
    result->getTypeNameSpace = getTypeNameSpaceFromTypeDescriptor;

    result->getAssembly = getAssemblyFromTypeDescriptor;

    result->makeArrayDescriptor = makeArrayDescriptor;
    result->makePointerDescriptor = makePointerDescriptor;
    result->makeByRef = makeByRef;

    result->isGenericTypeDefinition = isGenericTypeDefinition;
    result->containOpenGenericParameter = containOpenGenericParameterFromTypeDescriptor;
    result->isGeneric = isGenericTypeDescriptor;
    result->getGenericDefinition = getGenericDefinitionFromTypeDescriptor;
    result->getInstance = getInstanceFromTypeDescriptor;

    result->getGenericParameters = getGenericParametersFromTypeDescriptor;

    result->getUnderliningClass = getUnderliningClassDescriptorFromTypeDescriptor;

    return result;
}

inline GenericContainer *getResolvingContainerFromTypeDescriptor (TypeDescriptor *type) {
    GenericContainer *result;

    switch (type->element_type) {
        case ELEMENT_CLASS:
            result = getResolvingContainerFromClassDescriptor((ClassDescriptor *) type->descriptor);
            break;
        default:
            result = JITFALSE;
    }
    return result;
}

inline JITINT8 *getNameFromTypeDescriptor (TypeDescriptor *type) {
    if (!WAIT_TICKET(type, NAME_TICKET)) {
        JITINT8 *result = NULL;
        switch (type->element_type) {
            case ELEMENT_ARRAY:
                result = getNameFromArrayDescriptor((ArrayDescriptor *) type->descriptor);
                break;
            case ELEMENT_FNPTR:
                result = getSignatureInStringFromFunctionPointerDescriptor((FunctionPointerDescriptor *) type->descriptor);
                break;
            case ELEMENT_PTR:
                result = getNameFromPointerDescriptor((PointerDescriptor *) type->descriptor);
                break;
            case ELEMENT_VAR:
                result = getNameFromGenericVarDescriptor((GenericVarDescriptor *) type->descriptor);
            case ELEMENT_MVAR:
                result = getNameFromGenericMethodVarDescriptor((GenericMethodVarDescriptor *) type->descriptor);
                break;
            case ELEMENT_CLASS:
                result = getNameFromClassDescriptor((ClassDescriptor *) type->descriptor);
                break;
            default:
                PDEBUG("METADATA MANAGER: getNameFromTypeDescriptor: ERROR! INVALID TYPE! %d\n", type->element_type);
                abort();
        }
        if (type->isByRef) {
            JITUINT32 lenght = STRLEN(result) + 1;
            size_t size = sizeof(JITINT8) * lenght;
            type->name = sharedAllocFunction(size);
            SNPRINTF(type->name, size, "%s&", result);
        } else {
            type->name = result;
        }
        BROADCAST_TICKET(type, NAME_TICKET);
    }
    return type->name;
}

inline JITINT8 *getCompleteNameFromTypeDescriptor (TypeDescriptor *type) {
    if (!WAIT_TICKET(type, COMPLETE_NAME_TICKET)) {
        JITINT8 *result = NULL;
        switch (type->element_type) {
            case ELEMENT_ARRAY:
                result = getCompleteNameFromArrayDescriptor((ArrayDescriptor *) type->descriptor);
                break;
            case ELEMENT_FNPTR:
                result = getSignatureInStringFromFunctionPointerDescriptor((FunctionPointerDescriptor *) type->descriptor);
                break;
            case ELEMENT_PTR:
                result = getCompleteNameFromPointerDescriptor((PointerDescriptor *) type->descriptor);
                break;
            case ELEMENT_VAR:
                result = getNameFromGenericVarDescriptor((GenericVarDescriptor *) type->descriptor);
                break;
            case ELEMENT_MVAR:
                result = getNameFromGenericMethodVarDescriptor((GenericMethodVarDescriptor *) type->descriptor);
                break;
            case ELEMENT_CLASS:
                result = getCompleteNameFromClassDescriptor((ClassDescriptor *) type->descriptor);
                break;
            default:
                PDEBUG("METADATA MANAGER: getCompleteNameFromTypeDescriptor: ERROR! INVALID TYPE! %d\n", type->element_type);
                abort();
        }
        if (type->isByRef) {
            JITUINT32 lenght = STRLEN(result) + 1 + 1;
            size_t size = sizeof(JITINT8) * lenght;
            type->completeName = sharedAllocFunction(size);
            SNPRINTF(type->completeName, size, "%s&", result);
        } else {
            type->completeName = result;
        }
        BROADCAST_TICKET(type, COMPLETE_NAME_TICKET);
    }
    return type->completeName;
}

inline JITINT8 *getTypeNameSpaceFromTypeDescriptor (TypeDescriptor *type) {
    JITINT8 *result = NULL;

    switch (type->element_type) {
        case ELEMENT_ARRAY:
            result = getTypeNameSpaceFromArrayDescriptor((ArrayDescriptor *) type->descriptor);
            break;
        case ELEMENT_CLASS:
            result = getTypeNameSpaceFromClassDescriptor((ClassDescriptor *) type->descriptor);
            break;
        case ELEMENT_FNPTR:
        case ELEMENT_PTR:
        case ELEMENT_VAR:
        case ELEMENT_MVAR: {
            ClassDescriptor *class = getUnderliningClassDescriptorFromTypeDescriptor(type);
            result = getTypeNameSpaceFromClassDescriptor(class);
        }
        break;
        default:
            PDEBUG("METADATA MANAGER: getTypeNameSpaceFromTypeDescriptor: ERROR! INVALID TYPE! %d\n", type->element_type);
            abort();
    }
    return result;
}

inline BasicAssembly *getAssemblyFromTypeDescriptor (TypeDescriptor *type) {
    BasicAssembly *result = NULL;

    switch (type->element_type) {
        case ELEMENT_ARRAY:
            result = getAssemblyFromArrayDescriptor((ArrayDescriptor *) type->descriptor);
            break;
        case ELEMENT_CLASS:
            result = getAssemblyFromClassDescriptor((ClassDescriptor *) type->descriptor);
            break;
        case ELEMENT_FNPTR:
        case ELEMENT_PTR:
        case ELEMENT_VAR:
        case ELEMENT_MVAR: {
            ClassDescriptor *class = getUnderliningClassDescriptorFromTypeDescriptor(type);
            result = getAssemblyFromClassDescriptor(class);
        }
        break;
        default:
            PDEBUG("METADATA MANAGER: getTypeNameSpaceFromTypeDescriptor: ERROR! INVALID TYPE! %d\n", type->element_type);
            abort();
    }
    return result;
}

inline ClassDescriptor *getUnderliningClassDescriptorFromTypeDescriptor (TypeDescriptor *type) {
    ClassDescriptor *refClass;

    initializeMetadata();

    switch (type->element_type) {
        case ELEMENT_FNPTR:
        case ELEMENT_PTR:
            refClass = type->manager->System_IntPtr;
        case ELEMENT_ARRAY:
            refClass = type->manager->System_Array;
            break;
        case ELEMENT_CLASS:
            refClass = (ClassDescriptor*) type->descriptor;
            break;
        case ELEMENT_VAR:
            refClass = getUnderliningClassDescriptorFromGenericVarDescriptor((GenericVarDescriptor *) type->descriptor);
            break;
        case ELEMENT_MVAR:
            refClass = getUnderliningClassDescriptorFromGenericMethodVarDescriptor((GenericMethodVarDescriptor *) type->descriptor);
            break;
        default:
            PDEBUG("METADATA MANAGER: getUnderliningClassDescriptorFromTypeDescriptor: ERROR! INVALID TYPE! %d\n", type->element_type);
            abort();
    }
    return refClass;
}

inline JITBOOLEAN isEnumTypeDescriptor (TypeDescriptor *type) {
    JITBOOLEAN result;

    switch (type->element_type) {
        case ELEMENT_FNPTR:
        case ELEMENT_PTR:
        case ELEMENT_VAR:
        case ELEMENT_MVAR:
        case ELEMENT_ARRAY:
            result = JITFALSE;
            break;
        case ELEMENT_CLASS:
            result = isEnumClassDescriptor((ClassDescriptor*) type->descriptor);
            break;
        default:
            PDEBUG("METADATA MANAGER: isEnumTypeDescriptor: ERROR! INVALID TYPE! %d\n", type->element_type);
            abort();
    }
    return result;
}

inline JITBOOLEAN isValueTypeTypeDescriptor (TypeDescriptor *type) {
    JITBOOLEAN result;

    switch (type->element_type) {
        case ELEMENT_FNPTR:
        case ELEMENT_PTR:
        case ELEMENT_VAR:
        case ELEMENT_MVAR:
        case ELEMENT_ARRAY:
            result = JITFALSE;
            break;
        case ELEMENT_CLASS:
            result = isValueTypeClassDescriptor((ClassDescriptor*) type->descriptor);
            break;
        default:
            PDEBUG("METADATA MANAGER: isValueTypeTypeDescriptor: ERROR! INVALID TYPE! %d\n", type->element_type);
            abort();
    }
    return result;
}

inline JITBOOLEAN isDelegateTypeDescriptor (TypeDescriptor *type) {
    JITBOOLEAN result;

    switch (type->element_type) {
        case ELEMENT_FNPTR:
        case ELEMENT_PTR:
        case ELEMENT_VAR:
        case ELEMENT_MVAR:
        case ELEMENT_ARRAY:
            result = JITFALSE;
            break;
        case ELEMENT_CLASS:
            result = isDelegateClassDescriptor((ClassDescriptor*) type->descriptor);
            break;
        default:
            PDEBUG("METADATA MANAGER: isDelegateClassDescriptor: ERROR! INVALID TYPE! %d\n", type->element_type);
            abort();
    }
    return result;
}

inline JITBOOLEAN isNullableTypeDescriptor (TypeDescriptor *type) {
    JITBOOLEAN result;

    switch (type->element_type) {
        case ELEMENT_FNPTR:
        case ELEMENT_PTR:
        case ELEMENT_VAR:
        case ELEMENT_MVAR:
        case ELEMENT_ARRAY:
            result = JITFALSE;
            break;
        case ELEMENT_CLASS:
            result = isNullableClassDescriptor((ClassDescriptor*) type->descriptor);
            break;
        default:
            PDEBUG("METADATA MANAGER: isNullableTypeDescriptor: ERROR! INVALID TYPE! %d\n", type->element_type);
            abort();
    }
    return result;
}

inline JITBOOLEAN isPrimitiveTypeDescriptor (TypeDescriptor *type) {
    JITBOOLEAN result;

    switch (type->element_type) {
        case ELEMENT_FNPTR:
        case ELEMENT_PTR:
        case ELEMENT_VAR:
        case ELEMENT_MVAR:
        case ELEMENT_ARRAY:
            result = JITFALSE;
            break;
        case ELEMENT_CLASS:
            result = isPrimitiveClassDescriptor((ClassDescriptor*) type->descriptor);
            break;
        default:
            PDEBUG("METADATA MANAGER: isPrimitiveClassDescriptor: ERROR! INVALID TYPE! %d\n", type->element_type);
            abort();
    }
    return result;
}

inline TypeDescriptor *getSuperTypeFromTypeDescriptor (TypeDescriptor *type) {
    TypeDescriptor *superType;

    initializeMetadata();

    switch (type->element_type) {
        case ELEMENT_FNPTR:
        case ELEMENT_PTR:
            superType = getTypeDescriptorFromClassDescriptor(type->manager->System_IntPtr);
        case ELEMENT_ARRAY:
            superType = getTypeDescriptorFromClassDescriptor(type->manager->System_Array);
            break;
        case ELEMENT_CLASS:
            superType = getSuperClass((ClassDescriptor*) type->descriptor);
            break;
        case ELEMENT_VAR:
        case ELEMENT_MVAR:
        default:
            PDEBUG("METADATA MANAGER: getSuperTypeFromTypeDescriptor: ERROR! INVALID TYPE! %d\n", type->element_type);
            abort();
    }
    return superType;
}

inline MethodDescriptor *typeDescriptorImplementsMethod (TypeDescriptor *type, MethodDescriptor *method) {
    JITUINT32 vtableLength;
    MethodDescriptor *methodDefinition;
    MethodDescriptor *bodyDefinition;
    MethodDescriptor *result;
    XanHashTable *vtable = getVTableFromTypeDescriptor(type, &vtableLength);

    if (isGenericMethodDescriptor(method) && !isGenericMethodDefinition(method)) {
        methodDefinition = getGenericDefinitionFromMethodDescriptor(method);
    } else {
        methodDefinition = method;
    }
    VTableSlot *slot = (VTableSlot *) xanHashTable_lookup(vtable, methodDefinition);

    if (slot == NULL) {
        return NULL;
    }
    bodyDefinition = slot->body;
    if (isGenericMethodDescriptor(method) && !isGenericMethodDefinition(method)) {
        result = getInstanceFromMethodDescriptor(bodyDefinition, method->container);
    } else {
        result = bodyDefinition;
    }

    return result;
}

inline JITUINT32 getIRTypeFromTypeDescriptor (TypeDescriptor *type) {
    JITUINT32 irType;

    if (type->isByRef) {
        irType = IRMPOINTER;
    } else {
        switch (type->element_type) {
            case ELEMENT_FNPTR:
            case ELEMENT_PTR:
                irType = IRNINT;
                break;
            case ELEMENT_ARRAY:
                irType = IROBJECT;
                break;
            case ELEMENT_CLASS:
                irType = getIRTypeFromClassDescriptor((ClassDescriptor *) type->descriptor);
                break;
            case ELEMENT_VAR:
                irType = getIRTypeFromGenericVarDescriptor((GenericVarDescriptor *) type->descriptor);
                break;
            case ELEMENT_MVAR:
                irType = getIRTypeFromGenericMethodVarDescriptor((GenericMethodVarDescriptor *) type->descriptor);
                break;
            default:
                PDEBUG("METADATA MANAGER: getIRTypeFromTypeDescriptor: ERROR! INVALID TYPE! %d\n", type->element_type);
                abort();
                break;
        }
    }
    return irType;
}

static inline void internalInsertIntoTypeCompatibilty (TypeElem *bucket, TypeDescriptor *type, JITBOOLEAN value) {
    JITUINT32 position = simpleHash(type) % TYPE_COMP_SIZE;

    bucket = &(bucket[position]);
    if (bucket->type == NULL) {
        bucket->type = type;
        bucket->value = value;
    } else {
        TypeElem *item = sharedAllocFunction(sizeof(TypeElem));
        item->next = NULL;
        item->type = type;
        item->value = value;
        while (bucket->next != NULL) {
            bucket = bucket->next;
        }
        bucket->next = item;
    }
}

static inline JITBOOLEAN internalGetFromTypeCompatibilty (TypeElem *bucket, TypeDescriptor *type, JITBOOLEAN *value) {
    JITBOOLEAN valid = JITFALSE;
    JITUINT32 position = simpleHash(type) % TYPE_COMP_SIZE;

    bucket = &(bucket[position]);
    if (bucket->type == type) {
        (*value) = bucket->value;
        valid = JITTRUE;
    } else {
        bucket = bucket->next;
        while (bucket != NULL) {
            if (bucket->type == type) {
                valid = JITTRUE;
                (*value) = bucket->value;
                break;
            }
            bucket = bucket->next;
        }
    }
    return valid;
}


inline JITBOOLEAN typeDescriptorAssignableTo (TypeDescriptor *type_to_cast, TypeDescriptor *desired_type) {
    JITBOOLEAN result = JITFALSE;
    JITBOOLEAN found = internalGetFromTypeCompatibilty(type_to_cast->compatibility, desired_type, &result);

    if (!found) {
        switch (desired_type->element_type) {
            case ELEMENT_VAR:
            case ELEMENT_MVAR:
            case ELEMENT_FNPTR:
            case ELEMENT_PTR:
                result = (type_to_cast == desired_type);
                break;
            case ELEMENT_ARRAY: {
                if (type_to_cast->element_type == ELEMENT_ARRAY) {
                    ArrayDescriptor *src = (ArrayDescriptor *) type_to_cast->descriptor;
                    ArrayDescriptor *dest = (ArrayDescriptor *) desired_type->descriptor;
                    result = ArrayDescriptorAssignableTo(src, dest);
                }
                break;
            }
            default: {
                ClassDescriptor *src = getUnderliningClassDescriptorFromTypeDescriptor(type_to_cast);
                ClassDescriptor *dest = getUnderliningClassDescriptorFromTypeDescriptor(desired_type);
                result = classDescriptorAssignableTo(src, dest);
            }
        }
        internalInsertIntoTypeCompatibilty(type_to_cast->compatibility, desired_type, result);
    }
    return result;
}

inline JITBOOLEAN typeDescriptorIsSubtypeOf (TypeDescriptor *type, TypeDescriptor *superType) {
    JITBOOLEAN result = JITFALSE;

    switch (superType->element_type) {
        case ELEMENT_VAR:
        case ELEMENT_MVAR:
        case ELEMENT_FNPTR:
        case ELEMENT_PTR:
        case ELEMENT_ARRAY:
            result = (type == superType);
            break;
        default: {
            ClassDescriptor *src = getUnderliningClassDescriptorFromTypeDescriptor(type);
            ClassDescriptor *dest = getUnderliningClassDescriptorFromTypeDescriptor(superType);
            return classDescriptorIsSubTypeOf(src, dest);
        }
    }
    return result;
}

inline TypeDescriptor *getEnclosingType (TypeDescriptor *type) {
    TypeDescriptor *result = NULL;

    switch (type->element_type) {
        case ELEMENT_VAR:
        case ELEMENT_MVAR:
        case ELEMENT_FNPTR:
        case ELEMENT_PTR:
        case ELEMENT_ARRAY:
            break;
        case ELEMENT_CLASS: {
            ClassDescriptor *class = (ClassDescriptor *) type->descriptor;
            result = getEnclosingClass(class);
        }
        break;
        default:
            PDEBUG("METADATA MANAGER: getEnclosingType: ERROR! INVALID TYPE! %d\n", type->element_type);
            abort();
            break;
    }
    return result;
}

inline XanList * getImplementedInterfacesFromTypeDescriptor (TypeDescriptor *type) {
    XanList *result = NULL;

    switch (type->element_type) {
        case ELEMENT_VAR:
        case ELEMENT_MVAR:
        case ELEMENT_FNPTR:
        case ELEMENT_PTR:
        case ELEMENT_ARRAY:
            result = type->manager->emptyList;
            break;
        case ELEMENT_CLASS: {
            ClassDescriptor *class = (ClassDescriptor *) type->descriptor;
            result = getImplementedInterfacesFromClassDescriptor(class);
        }
        break;
        default:
            PDEBUG("METADATA MANAGER: getEnclosingType: ERROR! INVALID TYPE! %d\n", type->element_type);
            abort();
            break;
    }
    return result;

}

inline XanList *getDirectImplementedFieldsTypeDescriptor (TypeDescriptor *type) {
    XanList *result = NULL;

    switch (type->element_type) {
        case ELEMENT_VAR:
        case ELEMENT_MVAR:
        case ELEMENT_FNPTR:
        case ELEMENT_PTR:
        case ELEMENT_ARRAY:
            result = type->manager->emptyList;
            break;
        case ELEMENT_CLASS: {
            ClassDescriptor *class = (ClassDescriptor *) type->descriptor;
            result = getDirectImplementedFieldsClassDescriptor(class);
        }
        break;
        default:
            PDEBUG("METADATA MANAGER: getDirectImplementedFieldsTypeDescriptor: ERROR! INVALID TYPE! %d\n", type->element_type);
            abort();
            break;
    }
    return result;
}

inline FieldDescriptor *getFieldDescriptorFromNameTypeDescriptor (TypeDescriptor *type, JITINT8 * name) {
    FieldDescriptor *result;
    ClassDescriptor *class;

    result = NULL;
    switch (type->element_type) {
        case ELEMENT_VAR:
        case ELEMENT_MVAR:
        case ELEMENT_FNPTR:
        case ELEMENT_PTR:
        case ELEMENT_ARRAY:
            result = NULL;
            break;
        case ELEMENT_CLASS:
            class = (ClassDescriptor *) type->descriptor;
            result = getFieldDescriptorFromNameClassDescriptor(class, name);
            break;
        default:
            PDEBUG("METADATA MANAGER: getFieldDescriptorFromNameTypeDescriptor: ERROR! INVALID TYPE! %d\n", type->element_type);
            abort();
            break;
    }
    return result;
}

inline XanList *getDirectDeclaredMethodsTypeDescriptor (TypeDescriptor *type) {
    XanList *result = NULL;

    switch (type->element_type) {
        case ELEMENT_VAR:
        case ELEMENT_MVAR:
        case ELEMENT_FNPTR:
        case ELEMENT_PTR:
        case ELEMENT_ARRAY:
            result = type->manager->emptyList;
            break;
        case ELEMENT_CLASS: {
            ClassDescriptor *class = (ClassDescriptor *) type->descriptor;
            result = getDirectDeclaredMethodsClassDescriptor(class);
        }
        break;
        default:
            PDEBUG("METADATA MANAGER: getDirectDeclaredMethodsTypeDescriptor: ERROR! INVALID TYPE! %d\n", type->element_type);
            abort();
            break;
    }
    return result;
}

inline XanList *getMethodDescriptorFromNameTypeDescriptor (TypeDescriptor *type, JITINT8 * name) {
    XanList 	*result;
    ClassDescriptor *class;

    result	= NULL;
    class	= NULL;
    switch (type->element_type) {
        case ELEMENT_VAR:
        case ELEMENT_MVAR:
        case ELEMENT_FNPTR:
        case ELEMENT_PTR:
        case ELEMENT_ARRAY:
            result = type->manager->emptyList;
            break;
        case ELEMENT_CLASS:
            class = (ClassDescriptor *) type->descriptor;
            result = getMethodDescriptorFromNameClassDescriptor(class, name);
            break;
        default:
            PDEBUG("METADATA MANAGER: getMethodDescriptorFromNameTypeDescriptor: ERROR! INVALID TYPE! %d\n", type->element_type);
            abort();
            break;
    }
    return result;
}

inline XanList *getConstructorsTypeDescriptor (TypeDescriptor *type) {
    XanList *result = NULL;

    switch (type->element_type) {
        case ELEMENT_VAR:
        case ELEMENT_MVAR:
        case ELEMENT_FNPTR:
        case ELEMENT_PTR:
        case ELEMENT_ARRAY:
            result = type->manager->emptyList;
            break;
        case ELEMENT_CLASS: {
            ClassDescriptor *class = (ClassDescriptor *) type->descriptor;
            result = getConstructorsClassDescriptor(class);
        }
        break;
        default:
            PDEBUG("METADATA MANAGER: getConstructorsTypeDescriptor: ERROR! INVALID TYPE! %d\n", type->element_type);
            abort();
            break;
    }
    return result;
}

inline MethodDescriptor *getCctorTypeDescriptor (TypeDescriptor *type) {
    MethodDescriptor *result = NULL;

    switch (type->element_type) {
        case ELEMENT_VAR:
        case ELEMENT_MVAR:
        case ELEMENT_FNPTR:
        case ELEMENT_PTR:
            result = NULL;
            break;
        case ELEMENT_CLASS: {
            ClassDescriptor *class = (ClassDescriptor *) type->descriptor;
            result = getCctorClassDescriptor(class);
        }
        break;
        default:
            PDEBUG("METADATA MANAGER: getCctorTypeDescriptor: ERROR! INVALID TYPE! %d\n", type->element_type);
            abort();
            break;
    }
    return result;
}

inline MethodDescriptor *getFinalizerTypeDescriptor (TypeDescriptor *type) {
    MethodDescriptor *result = NULL;

    switch (type->element_type) {
        case ELEMENT_VAR:
        case ELEMENT_MVAR:
        case ELEMENT_FNPTR:
        case ELEMENT_PTR:
        case ELEMENT_ARRAY:
            result = NULL;
            break;
        case ELEMENT_CLASS: {
            ClassDescriptor *class = (ClassDescriptor *) type->descriptor;
            result = getFinalizerClassDescriptor(class);
        }
        break;
        default:
            PDEBUG("METADATA MANAGER: getFinalizerTypeDescriptor: ERROR! INVALID TYPE! %d\n", type->element_type);
            abort();
            break;
    }
    return result;
}

inline XanList *getNestedType (TypeDescriptor *type) {
    XanList *result = NULL;

    switch (type->element_type) {
        case ELEMENT_VAR:
        case ELEMENT_MVAR:
        case ELEMENT_FNPTR:
        case ELEMENT_PTR:
        case ELEMENT_ARRAY:
            result = type->manager->emptyList;
            break;
        case ELEMENT_CLASS: {
            ClassDescriptor *class = (ClassDescriptor *) type->descriptor;
            result = getNestedClass(class);
        }
        break;
        default:
            PDEBUG("METADATA MANAGER: getNestedType: ERROR! INVALID TYPE! %d\n", type->element_type);
            abort();
            break;
    }
    return result;
}

inline TypeDescriptor *getNestedTypeFromName (TypeDescriptor *type, JITINT8 *name) {
    TypeDescriptor *result = NULL;

    switch (type->element_type) {
        case ELEMENT_VAR:
        case ELEMENT_MVAR:
        case ELEMENT_FNPTR:
        case ELEMENT_PTR:
        case ELEMENT_ARRAY:
            result = NULL;
            break;
        case ELEMENT_CLASS: {
            ClassDescriptor *class = (ClassDescriptor *) type->descriptor;
            result = getNestedClassFromName(class, name);
        }
        break;
        default:
            PDEBUG("METADATA MANAGER: getNestedTypeFromName: ERROR! INVALID TYPE! %d\n", type->element_type);
            abort();
            break;
    }
    return result;
}

inline XanList * getEventsTypeDescriptor (TypeDescriptor *type) {
    XanList *result = NULL;

    switch (type->element_type) {
        case ELEMENT_VAR:
        case ELEMENT_MVAR:
        case ELEMENT_FNPTR:
        case ELEMENT_PTR:
        case ELEMENT_ARRAY:
            result = type->manager->emptyList;
            break;
        case ELEMENT_CLASS: {
            ClassDescriptor *class = (ClassDescriptor *) type->descriptor;
            result = getEventsClassDescriptor(class);
        }
        break;
        default:
            PDEBUG("METADATA MANAGER: getEventsTypeDescriptor: ERROR! INVALID TYPE! %d\n", type->element_type);
            abort();
            break;
    }
    return result;
}

inline EventDescriptor *getEventDescriptorFromNameTypeDescriptor (TypeDescriptor *type, JITINT8 * name) {
    EventDescriptor *result = NULL;

    switch (type->element_type) {
        case ELEMENT_VAR:
        case ELEMENT_MVAR:
        case ELEMENT_FNPTR:
        case ELEMENT_PTR:
        case ELEMENT_ARRAY:
            result = NULL;
            break;
        case ELEMENT_CLASS: {
            ClassDescriptor *class = (ClassDescriptor *) type->descriptor;
            result = getEventDescriptorFromNameClassDescriptor(class, name);
        }
        break;
        default:
            PDEBUG("METADATA MANAGER: getEventDescriptorFromNameTypeDescriptor: ERROR! INVALID TYPE! %d\n", type->element_type);
            abort();
            break;
    }
    return result;
}

inline XanList *getPropertiesTypeDescriptor (TypeDescriptor *type) {
    XanList *result = NULL;

    switch (type->element_type) {
        case ELEMENT_VAR:
        case ELEMENT_MVAR:
        case ELEMENT_FNPTR:
        case ELEMENT_PTR:
        case ELEMENT_ARRAY:
            result = type->manager->emptyList;
            break;
        case ELEMENT_CLASS: {
            ClassDescriptor *class = (ClassDescriptor *) type->descriptor;
            result = getPropertiesClassDescriptor(class);
        }
        break;
        default:
            PDEBUG("METADATA MANAGER: getPropertyDescriptorFromNameTypeDescriptor: ERROR! INVALID TYPE! %d\n", type->element_type);
            abort();
            break;
    }
    return result;
}

inline PropertyDescriptor *getPropertyDescriptorFromNameTypeDescriptor (TypeDescriptor *type, JITINT8 * name) {
    PropertyDescriptor *result = NULL;

    switch (type->element_type) {
        case ELEMENT_VAR:
        case ELEMENT_MVAR:
        case ELEMENT_FNPTR:
        case ELEMENT_PTR:
        case ELEMENT_ARRAY:
            result = NULL;
            break;
        case ELEMENT_CLASS: {
            ClassDescriptor *class = (ClassDescriptor *) type->descriptor;
            result = getPropertyDescriptorFromNameClassDescriptor(class, name);
        }
        break;
        default:
            PDEBUG("METADATA MANAGER: getPropertyDescriptorFromNameTypeDescriptor: ERROR! INVALID TYPE! %d\n", type->element_type);
            abort();
            break;
    }
    return result;
}

inline TypeDescriptor *makeArrayDescriptor (TypeDescriptor *type, JITUINT32 rank) {
    ArrayDescriptor *array = newArrayDescriptor(type->manager, type, rank, 0, NULL, 0, NULL);

    return getTypeDescriptorFromArrayDescriptor(array);
}

inline TypeDescriptor *makePointerDescriptor (TypeDescriptor *type) {
    PointerDescriptor *pointer = newPointerDescriptor(type->manager, type);

    return getTypeDescriptorFromPointerDescriptor(pointer);
}

inline TypeDescriptor *makeByRef (TypeDescriptor *type) {
    TypeDescriptor *result;

    switch (type->element_type) {
        case ELEMENT_FNPTR:
            result = getByRefTypeDescriptorFromFunctionPointerDescriptor((FunctionPointerDescriptor *) type->descriptor);
            break;
        case ELEMENT_PTR:
            result = getByRefTypeDescriptorFromPointerDescriptor((PointerDescriptor *) type->descriptor);
            break;
        case ELEMENT_ARRAY:
            result = getByRefTypeDescriptorFromArrayDescriptor((ArrayDescriptor *) type->descriptor);
            break;
        case ELEMENT_CLASS:
            result = getByRefTypeDescriptorFromClassDescriptor((ClassDescriptor *) type->descriptor);
            break;
        case ELEMENT_VAR:
        case ELEMENT_MVAR:
        default:
            PDEBUG("METADATA MANAGER: makeByRef: ERROR! INVALID TYPE! %d\n", type->element_type);
            abort();
            break;
    }
    return result;
}

inline TypeDescriptor *getGenericDefinitionFromTypeDescriptor (TypeDescriptor *type) {
    TypeDescriptor *result;

    switch (type->element_type) {
        case ELEMENT_FNPTR:
            result = getGenericDefinitionFromFunctionPointerDescriptor((FunctionPointerDescriptor *) type->descriptor);
            break;
        case ELEMENT_PTR:
            result = getGenericDefinitionFromPointerDescriptor((PointerDescriptor *) type->descriptor);
            break;
        case ELEMENT_ARRAY:
            result = getGenericDefinitionFromArrayDescriptor((ArrayDescriptor *) type->descriptor);
            break;
        case ELEMENT_CLASS:
            result = getGenericDefinitionFromClassDescriptor((ClassDescriptor *) type->descriptor);
            break;
        case ELEMENT_VAR:
            result = type;
            break;
        case ELEMENT_MVAR:
            result = type;
            break;
        default:
            PDEBUG("METADATA MANAGER: getGenericDefinitionFromTypeDescriptor: ERROR! INVALID TYPE! %d\n", type->element_type);
            abort();
            break;
    }
    return result;
}

inline TypeDescriptor *getInstanceFromTypeDescriptor (TypeDescriptor *type, GenericContainer *container) {
    TypeDescriptor *result;

    switch (type->element_type) {
        case ELEMENT_FNPTR:
            result = getInstanceFromFunctionPointerDescriptor((FunctionPointerDescriptor *) type->descriptor, container);
            break;
        case ELEMENT_PTR:
            result = getInstanceFromPointerDescriptor((PointerDescriptor *) type->descriptor, container);
            break;
        case ELEMENT_ARRAY:
            result = getInstanceFromArrayDescriptor((ArrayDescriptor *) type->descriptor, container);
            break;
        case ELEMENT_CLASS:
            result = getInstanceFromClassDescriptor((ClassDescriptor *) type->descriptor, container);
            break;
        case ELEMENT_VAR:
            result = getInstanceFromGenericVarDescriptor((GenericVarDescriptor *) type->descriptor, container);
            break;
        case ELEMENT_MVAR:
            result = getInstanceFromGenericMethodVarDescriptor((GenericMethodVarDescriptor *) type->descriptor, container);
            break;
        default:
            PDEBUG("METADATA MANAGER: getInstanceFromTypeDescriptor: ERROR! INVALID TYPE! %d\n", type->element_type);
            abort();
            break;
    }
    return result;
}

inline XanList *getGenericParametersFromTypeDescriptor (TypeDescriptor *type) {
    XanList *result = NULL;

    switch (type->element_type) {
        case ELEMENT_CLASS:
            result = getGenericParametersFromClassDescriptor((ClassDescriptor *) type->descriptor);
            break;
        default:
            result = type->manager->emptyList;
    }
    return result;

}

inline JITBOOLEAN isGenericTypeDefinition (TypeDescriptor *type) {
    JITBOOLEAN result = JITFALSE;

    switch (type->element_type) {
        case ELEMENT_CLASS:
            result = isGenericClassDefinition((ClassDescriptor *) type->descriptor);
            break;
        default:
            result = JITFALSE;
    }
    return result;
}

inline JITBOOLEAN containOpenGenericParameterFromTypeDescriptor (TypeDescriptor *type) {
    JITBOOLEAN result = JITFALSE;

    switch (type->element_type) {
        case ELEMENT_CLASS:
            result = containOpenGenericParameterFromClassDescriptor((ClassDescriptor *) type->descriptor);
            break;
        case ELEMENT_ARRAY:
            result = containOpenGenericParameterFromArrayDescriptor((ArrayDescriptor *) type->descriptor);
            break;
        case ELEMENT_PTR:
            result = containOpenGenericParameterFromPointerDescriptor((PointerDescriptor *) type->descriptor);
            break;
        case ELEMENT_FNPTR:
            result = containOpenGenericParameterFromFunctionPointerDescriptor((FunctionPointerDescriptor *) type->descriptor);
            break;
        case ELEMENT_MVAR:
        case ELEMENT_VAR:
            result = JITTRUE;
            break;
        default:
            result = JITFALSE;
    }
    return result;
}

inline JITBOOLEAN isGenericTypeDescriptor (TypeDescriptor *type) {
    JITBOOLEAN result = JITFALSE;

    switch (type->element_type) {
        case ELEMENT_CLASS:
            result = isGenericClassDescriptor((ClassDescriptor *) type->descriptor);
            break;
        default:
            result = JITFALSE;
    }
    return result;
}

inline void typeDescriptorLock (TypeDescriptor *type) {
    PLATFORM_lockMutex(&(type->mutex));
    switch (type->element_type) {
        case ELEMENT_ARRAY:
            arrayDescriptorLock((ArrayDescriptor *) type->descriptor);
            break;
        case ELEMENT_FNPTR:
            functionPointerDescriptorLock((FunctionPointerDescriptor *) type->descriptor);
            break;
        case ELEMENT_PTR:
            pointerDescriptorLock((PointerDescriptor *) type->descriptor);
            break;
        case ELEMENT_VAR:
            genericVarDescriptorLock((GenericVarDescriptor *) type->descriptor);
            break;
        case ELEMENT_MVAR:
            genericMethodVarDescriptorLock((GenericMethodVarDescriptor *) type->descriptor);
            break;
        case ELEMENT_CLASS:
            classDescriptorLock((ClassDescriptor *) type->descriptor);
            break;
        default:
            PDEBUG("METADATA MANAGER: typeDescriptorLock: ERROR! INVALID TYPE! %d\n", type->element_type);
            abort();
    }
}

inline void typeDescriptorUnLock (TypeDescriptor *type) {
    switch (type->element_type) {
        case ELEMENT_ARRAY:
            arrayDescriptorUnLock((ArrayDescriptor *) type->descriptor);
            break;
        case ELEMENT_FNPTR:
            functionPointerDescriptorUnLock((FunctionPointerDescriptor *) type->descriptor);
            break;
        case ELEMENT_PTR:
            pointerDescriptorUnLock((PointerDescriptor *) type->descriptor);
            break;
        case ELEMENT_VAR:
            genericVarDescriptorUnLock((GenericVarDescriptor *) type->descriptor);
            break;
        case ELEMENT_MVAR:
            genericMethodVarDescriptorUnLock((GenericMethodVarDescriptor *) type->descriptor);
            break;
        case ELEMENT_CLASS:
            classDescriptorUnLock((ClassDescriptor *) type->descriptor);
            break;
        default:
            PDEBUG("METADATA MANAGER: typeDescriptorLock: ERROR! INVALID TYPE! %d\n", type->element_type);
            abort();
    }
    PLATFORM_unlockMutex(&(type->mutex));
}

inline DescriptorMetadataTicket *typeDescriptorCreateDescriptorMetadataTicket (TypeDescriptor *type_desc, JITUINT32 type) {
    DescriptorMetadataTicket *result = NULL;

    switch (type_desc->element_type) {
        case ELEMENT_ARRAY:
            result = arrayDescriptorCreateDescriptorMetadataTicket((ArrayDescriptor *) type_desc->descriptor, type);
            break;
        case ELEMENT_FNPTR:
            result = functionPointerDescriptorCreateDescriptorMetadataTicket((FunctionPointerDescriptor *) type_desc->descriptor, type);
            break;
        case ELEMENT_PTR:
            result = pointerDescriptorCreateDescriptorMetadataTicket((PointerDescriptor *) type_desc->descriptor, type);
            break;
        case ELEMENT_VAR:
            result = genericVarDescriptorCreateDescriptorMetadataTicket((GenericVarDescriptor *) type_desc->descriptor, type);
            break;
        case ELEMENT_MVAR:
            result = genericMethodVarDescriptorCreateDescriptorMetadataTicket((GenericMethodVarDescriptor *) type_desc->descriptor, type);
            break;
        case ELEMENT_CLASS:
            result = classDescriptorCreateDescriptorMetadataTicket((ClassDescriptor *) type_desc->descriptor, type);
            break;
        default:
            PDEBUG("METADATA MANAGER: typeDescriptorGetMetadata: ERROR! INVALID TYPE! %d\n", type_desc->element_type);
            abort();
    }
    return result;
}

inline void typeDescriptorBroadcastDescriptorMetadataTicket (TypeDescriptor *type_desc,  DescriptorMetadataTicket *ticket, void * data) {
    switch (type_desc->element_type) {
        case ELEMENT_ARRAY:
            arrayDescriptorBroadcastDescriptorMetadataTicket((ArrayDescriptor *) type_desc->descriptor, ticket, data);
            break;
        case ELEMENT_FNPTR:
            functionPointerDescriptorBroadcastDescriptorMetadataTicket((FunctionPointerDescriptor *) type_desc->descriptor, ticket, data);
            break;
        case ELEMENT_PTR:
            pointerDescriptorBroadcastDescriptorMetadataTicket((PointerDescriptor *) type_desc->descriptor, ticket, data);
            break;
        case ELEMENT_VAR:
            genericVarDescriptorBroadcastDescriptorMetadataTicket((GenericVarDescriptor *) type_desc->descriptor, ticket, data);
            break;
        case ELEMENT_MVAR:
            genericMethodVarDescriptorBroadcastDescriptorMetadataTicket((GenericMethodVarDescriptor *) type_desc->descriptor, ticket, data);
            break;
        case ELEMENT_CLASS:
            classDescriptorBroadcastDescriptorMetadataTicket((ClassDescriptor *) type_desc->descriptor, ticket, data);
            break;
        default:
            PDEBUG("METADATA MANAGER: typeDescriptorSetMetadata: ERROR! INVALID TYPE! %d\n", type_desc->element_type);
            abort();
    }
}
