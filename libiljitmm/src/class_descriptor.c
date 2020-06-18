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
#include <decoding_tools.h>
#include <type_spec_decoding_tool.h>
#include <basic_concept_manager.h>
#include <iljitmm-system.h>
#include <stdlib.h>
#include <assert.h>
#include <platform_API.h>

#define TYPE_TICKET 0
#define BYREF_TYPE_TICKET 1
#define INTERFACE_TICKET 2
#define SUPERCLASS_TICKET 3
#define ENCLOSING_TICKET 4
#define NESTED_TICKET 5
#define NAME_TICKET 6
#define NAMESPACE_TICKET 7
#define COMPLETE_NAME_TICKET 8
#define FIELD_TICKET 9
#define METHOD_TICKET 10
#define EVENT_TICKET 11
#define PROPERTY_TICKET 12
#define ASSEMBLY_TICKET 13
#define GENERIC_PARAM_TICKET 14
#define RESOLVING_CONTAINER_TICKET 15

#define CLASS_TICKET_COUNT 16

static inline JITBOOLEAN checkClassDescriptorContainerConsistency (ClassDescriptor *class, GenericContainer *container);

static inline void internalGetDirectImplementedEvents (ClassDescriptor *class);
static inline void internalGetDirectImplementedProperties (ClassDescriptor *class);
static inline void internalGetDirectImplementedFields (ClassDescriptor *class);
static inline void internalGetDirectDeclaredMethods (ClassDescriptor *class);
static inline TypeDescriptor *internalNullableParameter (ClassDescriptor *class);


/* ClassDescriptor VTable Computation Helper Method */
inline XanList *getExplictOverrideInformationsFromClassDescriptor (ClassDescriptor *class) {
    t_binary_information *binary = (t_binary_information *) class->row->binary;

    XanList *result = xanList_new(sharedAllocFunction, freeFunction, NULL);
    XanList *list = (XanList *) getRowByKey(class->manager, binary, EXPLICIT_OVERRIDE, (void *) (JITNUINT)class->row->index);

    if (list != NULL) {
        GenericContainer *container = getResolvingContainerFromClassDescriptor(class);
        XanListItem *item = xanList_first(list);
        while (item != NULL) {
            t_row_method_impl_table *row = (t_row_method_impl_table *) item->data;
            ExplicitOverrideInformation *information = sharedAllocFunction(sizeof(ExplicitOverrideInformation));
            information->declaration = getMethodDescriptorFromMethodDefOrRef(class->manager, container, binary, row->method_declaration, NULL);
            information->body = getMethodDescriptorFromMethodDefOrRef(class->manager, container, binary, row->method_body, NULL);
            xanList_append(result, information);
            item = item->next;
        }
    }
    return result;
}

inline XanList *getInterfacesInformationFromClassDescriptor (ClassDescriptor *class, GenericSpecContainer *specContainer) {
    t_binary_information *binary = (t_binary_information *) class->row->binary;

    XanList *result = xanList_new(sharedAllocFunction, freeFunction, NULL);
    XanList *list = getRowByKey(class->manager, binary, INTERFACE, (void *) (JITNUINT)class->row->index);

    if (list != NULL) {
        GenericContainer *container = getResolvingContainerFromClassDescriptor(class);
        XanListItem *item = xanList_first(list);
        while (item != NULL) {
            t_row_interface_impl_table *row = (t_row_interface_impl_table *) item->data;
            InterfaceInformation *information = sharedAllocFunction(sizeof(InterfaceInformation));
            information->interfaceDescription = getTypeDescriptorFromTypeDeforRefToken(class->manager, container, binary, row->interfaceDescription);
            information->container = getGenericSpecContainerFromTypeDeforRefToken(class->manager, specContainer, binary, row->interfaceDescription);
            xanList_append(result, information);
            item = item->next;
        }
    }
    return result;
}

inline GenericSpecContainer *getSuperClassGenericSpecContainer (ClassDescriptor *class) {
    t_binary_information *binary = (t_binary_information *) class->row->binary;

    return getGenericSpecContainerFromTypeDeforRefToken(class->manager, NULL, binary, class->row->extendsIndex);
}


/* ClassDescriptor Accessor Method */
JITUINT32 hashClassDescriptor (ClassDescriptor *class) {
    if (class == NULL) {
        return 0;
    }
    JITUINT32 seed = combineHash((JITUINT32) class->row, hashGenericContainer(class->container));
    return seed;
}

JITINT32 equalsClassDescriptor (ClassDescriptor *key1, ClassDescriptor *key2) {
    if (key1 == key2) {
        return 1;
    }
    if (key1 == NULL || key2 == NULL) {
        return 0;
    }
    return key1->row == key2->row && equalsGenericContainer(key1->container, key2->container);
}

static inline JITBOOLEAN checkClassDescriptorContainerConsistency (ClassDescriptor *class, GenericContainer *container) {
    return JITTRUE;
}

inline ClassDescriptor *newClassDescriptor (metadataManager_t *manager, BasicClass *row, GenericContainer *container) {
    if (row == NULL) {
        return NULL;
    }
    ClassDescriptor class;
    class.manager = manager;
    class.container = container;
    class.row = row;
    xanHashTable_lock(manager->classDescriptors);
    ClassDescriptor *found = (ClassDescriptor *) xanHashTable_lookup(manager->classDescriptors, (void *) &class);
    if (found == NULL) {
        found = sharedAllocFunction(sizeof(ClassDescriptor));
        found->manager = manager;
        found->row = row;
        pthread_mutexattr_t mutex_attr;
        PLATFORM_initMutexAttr(&mutex_attr);
        PLATFORM_setMutexAttr_type(&mutex_attr, PTHREAD_MUTEX_ADAPTIVE_NP);
        MEMORY_shareMutex(&mutex_attr);
        found->tickets = allocTicketArray(CLASS_TICKET_COUNT);
        PLATFORM_initMutex(&(found->mutex), &mutex_attr);
        init_DescriptorMetadata(&(found->metadata));
        found->lock = classDescriptorLock;
        found->unlock = classDescriptorUnLock;
        found->createDescriptorMetadataTicket = classDescriptorCreateDescriptorMetadataTicket;
        found->broadcastDescriptorMetadataTicket = classDescriptorBroadcastDescriptorMetadataTicket;
        found->descriptor = NULL;
        found->byRefDescriptor = NULL;
        found->getTypeDescriptor = getTypeDescriptorFromClassDescriptor;
        found->getByRefTypeDescriptor = getByRefTypeDescriptorFromClassDescriptor;
        found->container = cloneGenericContainer(container);
        found->resolving_container = NULL;
        found->getResolvingContainer = getResolvingContainerFromClassDescriptor;
        /* Init Default value */
        found->superClass = NULL;
        found->interfaces = NULL;
        found->enclosing_type = NULL;
        found->nestedClass = NULL;
        found->nestedClassMap = NULL;
        found->directMethods = NULL;
        found->methodsMap = NULL;
        found->cctor = NULL;
        found->finalizer = NULL;
        found->constructors = NULL;
        found->fieldsMap = NULL;
        found->directFields = NULL;
        found->name = NULL;
        found->typeNameSpace = NULL;
        found->completeName = NULL;
        found->events = NULL;
        found->eventsMap = NULL;
        found->properties = NULL;
        found->propertiesMap = NULL;
        found->assembly = NULL;
        found->genericParameters = NULL;
        found->getResolvingContainer = getResolvingContainerFromClassDescriptor;
        found->getSuperClass = getSuperClass;
        found->getImplementedInterfaces = getImplementedInterfacesFromClassDescriptor;
        found->getEnclosing = getEnclosingClass;
        found->getNestedClass = getNestedClass;
        found->getNestedClassFromName = getNestedClassFromName;
        found->getCctor = getCctorClassDescriptor;
        found->getFinalizer = getFinalizerClassDescriptor;
        found->getConstructors = getConstructorsClassDescriptor;
        found->getDirectDeclaredMethods = getDirectDeclaredMethodsClassDescriptor;
        found->getMethodFromName = getMethodDescriptorFromNameClassDescriptor;
        found->getDirectImplementedFields = getDirectImplementedFieldsClassDescriptor;
        found->getFieldFromName = getFieldDescriptorFromNameClassDescriptor;
        found->isValueType = isValueTypeClassDescriptor;
        found->isEnum = isEnumClassDescriptor;
        found->isDelegate = isDelegateClassDescriptor;
        found->isPrimitive = isPrimitiveClassDescriptor;
        found->isNullable = isNullableClassDescriptor;
        found->getName = getNameFromClassDescriptor;
        found->getTypeNameSpace = getTypeNameSpaceFromClassDescriptor;
        found->getCompleteName = getCompleteNameFromClassDescriptor;
        found->implementsInterface = implementsInterface;
        found->extendsType = extendsType;
        found->isSubtypeOf = classDescriptorIsSubTypeOf;
        found->assignableTo = classDescriptorAssignableTo;
        found->getEvents = getEventsClassDescriptor;
        found->getEventFromName = getEventDescriptorFromNameClassDescriptor;
        found->getProperties = getPropertiesClassDescriptor;
        found->getPropertyFromName = getPropertyDescriptorFromNameClassDescriptor;
        found->isGenericClassDefinition = isGenericClassDefinition;
        found->containOpenGenericParameter = containOpenGenericParameterFromClassDescriptor;
        found->isGeneric = isGenericClassDescriptor;
        found->getInstance = getInstanceFromClassDescriptor;
        found->getGenericDefinition = getGenericDefinitionFromClassDescriptor;
        found->getAssembly = getAssemblyFromClassDescriptor;
        found->getIRType = getIRTypeFromClassDescriptor;
        found->getGenericParameters = getGenericParametersFromClassDescriptor;
        /* end init Default value */
        found->attributes = getBasicTypeAttributes(manager, row);
        xanHashTable_insert(manager->classDescriptors, (void *) found, (void *) found);
    }
    xanHashTable_unlock(manager->classDescriptors);
    return found;
}

inline TypeDescriptor *getTypeDescriptorFromClassDescriptor (ClassDescriptor *class) {

    /* Assertions.
     */
    assert(class != NULL);

    if (!WAIT_TICKET(class, TYPE_TICKET)) {
        /* not already cached */
        class->descriptor = newTypeDescriptor(class->manager, ELEMENT_CLASS, (void *) class, JITFALSE);
        BROADCAST_TICKET(class, TYPE_TICKET);
    }
    return class->descriptor;
}

inline TypeDescriptor *getByRefTypeDescriptorFromClassDescriptor (ClassDescriptor *class) {
    if (!WAIT_TICKET(class, BYREF_TYPE_TICKET)) {
        /* not already cached */
        class->byRefDescriptor = newTypeDescriptor(class->manager, ELEMENT_CLASS, (void *) class, JITTRUE);
        BROADCAST_TICKET(class, BYREF_TYPE_TICKET);
    }
    return class->byRefDescriptor;
}

inline JITUINT32 getIRTypeFromClassDescriptor (ClassDescriptor *class) {
    metadataManager_t *manager = class->manager;
    JITUINT32 irType;

    initializeMetadata();

    if (class->attributes->isInterface) {
        irType = IROBJECT;
    } else if (isEnumClassDescriptor(class)) {
        irType = IRINT32;
    } else if (isValueTypeClassDescriptor(class)) {
        if (class == manager->System_Void) {
            irType = IRVOID;
        } else if (class == manager->System_Boolean) {
            irType = IRINT32;
        } else if (class == manager->System_IntPtr) {
            irType = IRNINT;
        } else if (class == manager->System_SByte) {
            irType = IRINT8;
        } else if (class == manager->System_Int16) {
            irType = IRINT16;
        } else if (class == manager->System_Int32) {
            irType = IRINT32;
        } else if (class == manager->System_Int64) {
            irType = IRINT64;
        } else if (class == manager->System_Single) {
            irType = IRFLOAT32;
        } else if (class == manager->System_Double) {
            irType = IRFLOAT64;
        } else if (class == manager->System_UIntPtr) {
            irType = IRNUINT;
        } else if (class == manager->System_Byte) {
            irType = IRUINT8;
        } else if (class == manager->System_UInt16) {
            irType = IRUINT16;
        } else if (class == manager->System_UInt32) {
            irType = IRUINT32;
        } else if (class == manager->System_UInt64) {
            irType = IRUINT64;
        } else if (class == manager->System_Char) {
            irType = IRUINT16;
        } else {
            irType = IRVALUETYPE;
        }
    } else {
        irType = IROBJECT;
    }
    return irType;
}

static inline void internalGetDirectImplementedInterfaces (ClassDescriptor *class, XanList *set) {
    t_binary_information *binary = (t_binary_information *) class->row->binary;

    XanList *list = getRowByKey(class->manager, binary, INTERFACE, (void *) (JITNUINT)class->row->index);

    if (list != NULL) {
        GenericContainer *container = getResolvingContainerFromClassDescriptor(class);
        XanListItem *item = xanList_first(list);
        while (item != NULL) {
            t_row_interface_impl_table *row = (t_row_interface_impl_table *) item->data;
            TypeDescriptor *interfaceDescription = getTypeDescriptorFromTypeDeforRefToken(class->manager, container, binary, row->interfaceDescription);
            if (xanList_find(set, interfaceDescription) == NULL) {
                /* update interface test */
                xanList_append(set, (void *) interfaceDescription);
                /* traverse parent of current interface */
                internalGetDirectImplementedInterfaces(GET_CLASS(interfaceDescription), set);
            }
            item = item->next;
        }
    }
}

inline XanList * getImplementedInterfacesFromClassDescriptor (ClassDescriptor *class) {
    if (!WAIT_TICKET(class, INTERFACE_TICKET)) {
        /* not already cached */
        TypeDescriptor *superClass = getSuperClass(class);
        if (superClass == NULL) {
            class->interfaces = xanList_new(sharedAllocFunction, freeFunction, NULL);
        } else {
            XanList *parent_interfaces = getImplementedInterfacesFromTypeDescriptor(superClass);
            class->interfaces = xanList_cloneList(parent_interfaces);
        }
        internalGetDirectImplementedInterfaces(class, class->interfaces);
        BROADCAST_TICKET(class, INTERFACE_TICKET);
    }
    return class->interfaces;
}

inline TypeDescriptor *getSuperClass (ClassDescriptor *class) {
    if (class->row->extendsIndex == 0) {
        return NULL;                     //extends nothing
    }
    if (!WAIT_TICKET(class, SUPERCLASS_TICKET)) {
        t_binary_information *binary = (t_binary_information *) class->row->binary;
        GenericContainer *container = getResolvingContainerFromClassDescriptor(class);
        class->superClass = getTypeDescriptorFromTypeDeforRefToken(class->manager, container, binary, class->row->extendsIndex);
        BROADCAST_TICKET(class, SUPERCLASS_TICKET);
    }
    /*already cached */
    return class->superClass;
}

inline JITBOOLEAN implementsInterface (ClassDescriptor *type, ClassDescriptor *interfaceDescription) {
    JITBOOLEAN ok = JITFALSE;

    if (!interfaceDescription->attributes->isInterface) {
        PDEBUG("METADATA MANAGER: implements: ERROR!: value passed is not an interface type");
        abort();
    }
    if (type == interfaceDescription) {
        ok = JITTRUE;
    } else {
        TypeDescriptor *interfaceTypeDesc = getTypeDescriptorFromClassDescriptor(interfaceDescription);
        XanList *interfaces = getImplementedInterfacesFromClassDescriptor(type);
        XanListItem *currentInterface = xanList_first(interfaces);
        while (currentInterface != NULL) {
            TypeDescriptor *interface_type = (TypeDescriptor *) currentInterface->data;
            if (interface_type == interfaceTypeDesc) {
                ok = JITTRUE;
                break;
            }
            currentInterface = currentInterface->next;
        }
    }
    return ok;
}

inline JITBOOLEAN extendsType (ClassDescriptor *type, ClassDescriptor *supertype) {
    JITBOOLEAN ok = JITFALSE;

    initializeMetadata();

    if (type == supertype) {
        return JITTRUE;
    }
    if (type->manager->System_Object == type) {
        return JITFALSE;
    }
    TypeDescriptor *parent_type = getSuperClass(type);
    ok = extendsType(GET_CLASS(parent_type), supertype);
    return ok;
}

inline JITBOOLEAN classDescriptorIsSubTypeOf (ClassDescriptor *type, ClassDescriptor *superType) {
    if (superType->attributes->isInterface) {
        return implementsInterface(type, superType);
    }
    if (type->attributes->isInterface) {
        return JITFALSE;
    }
    return extendsType(type, superType);
}

inline JITBOOLEAN classDescriptorAssignableTo (ClassDescriptor *type_to_cast, ClassDescriptor *desired_type) {
    metadataManager_t *manager = type_to_cast->manager;

    initializeMetadata();

    if (desired_type == manager->System_Char && type_to_cast == manager->System_Int16) {
        return JITTRUE;
    }
    if (isEnumClassDescriptor(type_to_cast) && desired_type == manager->System_Int32) {
        return JITTRUE;
    }
    if (isNullableClassDescriptor(desired_type)) {
        TypeDescriptor *desired_type_nullable_parameter = internalNullableParameter(desired_type);
        if (isNullableClassDescriptor(type_to_cast)) {
            TypeDescriptor *type_to_cast_nullable_parameter = internalNullableParameter(type_to_cast);
            return typeDescriptorAssignableTo(type_to_cast_nullable_parameter, desired_type_nullable_parameter);
        } else {
            return typeDescriptorAssignableTo(getTypeDescriptorFromClassDescriptor(type_to_cast), desired_type_nullable_parameter);
        }
    }
    return classDescriptorIsSubTypeOf(type_to_cast, desired_type);
}

static inline TypeDescriptor *internalNullableParameter (ClassDescriptor *class) {
    assert(isNullableClassDescriptor(class));
    return class->container->paramType[0];
}

inline JITBOOLEAN isNullableClassDescriptor (ClassDescriptor *class) {

    initializeMetadata();

    if (class->manager->System_Nullable == NULL) {
        return JITFALSE;
    }
    return class->row == class->manager->System_Nullable->row;
}

inline JITBOOLEAN isValueTypeClassDescriptor (ClassDescriptor *class) {
    if (!class->attributes->isSealed) {
        return JITFALSE;
    }

    initializeMetadata();

    if (class->manager->System_ValueType == class) {
        return JITFALSE;
    }
    return extendsType(class, class->manager->System_ValueType);
}

inline JITBOOLEAN isEnumClassDescriptor (ClassDescriptor *class) {
    if (!class->attributes->isSealed) {
        return JITFALSE;
    }

    initializeMetadata();

    if (class->manager->System_Enum == class) {
        return JITFALSE;
    }
    return extendsType(class, class->manager->System_Enum);
}

inline JITBOOLEAN isDelegateClassDescriptor (ClassDescriptor *class) {
    if (!class->attributes->isSealed) {
        return JITFALSE;
    }

    initializeMetadata();

    if (class->manager->System_Delegate == class) {
        return JITFALSE;
    }
    return extendsType(class, class->manager->System_Delegate);
}

inline JITBOOLEAN isPrimitiveClassDescriptor (ClassDescriptor *class) {
    metadataManager_t *manager = class->manager;

    initializeMetadata();

    JITBOOLEAN result = (manager->System_SByte == class ||
                         manager->System_Byte == class ||
                         manager->System_Int16 == class ||
                         manager->System_UInt16 == class ||
                         manager->System_Int32 == class ||
                         manager->System_UInt32 == class ||
                         manager->System_Int64 == class ||
                         manager->System_UInt64 == class ||
                         manager->System_Char == class ||
                         manager->System_Single == class ||
                         manager->System_Double == class ||
                         manager->System_Boolean == class);
    return result;
}

inline TypeDescriptor *getEnclosingClass (ClassDescriptor *class) {

    if (!class->attributes->isNested) {
        return NULL;
    }

    if (!WAIT_TICKET(class, ENCLOSING_TICKET)) {
        t_binary_information *binary = (t_binary_information *) class->row->binary;

        t_row_nested_class_table *row = (t_row_nested_class_table *) getRowByKey(class->manager, binary, ENCLOSING_CLASS, (void *) (JITNUINT)class->row->index);
        if (row != NULL) {
            GenericContainer *container = getResolvingContainerFromClassDescriptor(class);
            class->enclosing_type = getTypeDescriptorFromTableAndRow(class->manager, container, binary, 0x0, row->enclosing_class);
        }
        BROADCAST_TICKET(class, ENCLOSING_TICKET);
    }
    return class->enclosing_type;
}

static inline void internalGetNestedClass (ClassDescriptor *class) {
    t_binary_information *binary = (t_binary_information *) class->row->binary;
    t_streams_metadata      *streams = &((binary->metadata).streams_metadata);
    t_metadata_tables *tables = &((streams->not_stream).tables);

    class->nestedClass = xanList_new(sharedAllocFunction, freeFunction, NULL);
    class->nestedClassMap = xanHashTable_new(11, 0, sharedAllocFunction, dynamicReallocFunction, freeFunction, hashString, equalsString);

    XanList *list = getRowByKey(class->manager, binary, NESTED_CLASS, (void *) class->row->index);
    if (list != NULL) {
        XanListItem *item = xanList_first(list);
        while (item != NULL) {
            t_row_nested_class_table *nested_class_row = (t_row_nested_class_table *) item->data;
            BasicClass *row = (BasicClass *) get_row(tables, TYPE_DEF_TABLE, nested_class_row->nested_class);
            ClassDescriptor *nestedClass = newClassDescriptor(class->manager, row, NULL);
            TypeDescriptor *nestedType = getTypeDescriptorFromClassDescriptor(nestedClass);
            xanList_append(class->nestedClass, (void *) nestedType);
            JITINT8 *class_name = getNameFromTypeDescriptor(nestedType);
            xanHashTable_insert(class->nestedClassMap, (void *) class_name, (void *) nestedType);
            item = item->next;
        }
    }

}

inline XanList *getNestedClass (ClassDescriptor *class) {
    if (!WAIT_TICKET(class, NESTED_TICKET)) {
        internalGetNestedClass(class);
        BROADCAST_TICKET(class, NESTED_TICKET);
    }
    /* already cached */
    return class->nestedClass;

}

inline TypeDescriptor *getNestedClassFromName (ClassDescriptor *class, JITINT8 *name) {
    if (!WAIT_TICKET(class, NESTED_TICKET)) {
        internalGetNestedClass(class);
        BROADCAST_TICKET(class, NESTED_TICKET);
    }
    /* already cached */
    return (TypeDescriptor *) xanHashTable_lookup(class->nestedClassMap, (void *) name);
}

inline JITINT8 *getNameFromClassDescriptor (ClassDescriptor *class) {
    if (!WAIT_TICKET(class, NAME_TICKET)) {
        /*not already cached */
        t_binary_information *binary = (t_binary_information *) class->row->binary;
        t_streams_metadata      *streams = &((binary->metadata).streams_metadata);
        t_string_stream *string_stream = &(streams->string_stream);

        JITINT8 *basicClassName = get_string(string_stream, class->row->type_name);
        if (class->container != NULL) {
            GenericContainer *container = getResolvingContainerFromClassDescriptor(class);
            JITUINT32 count;
            JITUINT32 param_count = container->arg_count;
            JITINT8 **paramName = alloca(sizeof(JITINT8 *) * param_count);
            for (count = 0; count < param_count; count++) {
                paramName[count] = getCompleteNameFromTypeDescriptor(container->paramType[count]);
            }
            JITUINT32 alloc_length = STRLEN(basicClassName) + 2;            // \0 plus generic open
            for (count = 0; count < param_count; count++) {
                alloc_length += STRLEN(paramName[count]) + 1;           // , or generic close
            }
            size_t length = sizeof(JITINT8) * alloc_length;
            class->name = sharedAllocFunction(length);
            SNPRINTF(class->name, length, "%s<", basicClassName);
            for (count = 0; count < param_count - 1; count++) {
                class->name = STRCAT(class->name, paramName[count]);
                class->name = STRCAT(class->name, ",");
            }
            class->name = STRCAT(class->name, paramName[count]);
            class->name = STRCAT(class->name, ">");
        } else {
            class->name = basicClassName;
        }
        BROADCAST_TICKET(class, NAME_TICKET);
    }
    return class->name;
}

inline JITINT8 *getTypeNameSpaceFromClassDescriptor (ClassDescriptor *class) {
    if (!WAIT_TICKET(class, NAMESPACE_TICKET)) {
        /*not already cached */
        TypeDescriptor *enclosing_type = getEnclosingClass(class);
        if (enclosing_type == NULL) {
            t_binary_information *binary = (t_binary_information *) class->row->binary;
            t_streams_metadata      *streams = &((binary->metadata).streams_metadata);
            t_string_stream *string_stream = &(streams->string_stream);
            class->typeNameSpace = get_string(string_stream, class->row->type_name_space);
        } else {
            class->typeNameSpace = getTypeNameSpaceFromTypeDescriptor(enclosing_type);
        }
        BROADCAST_TICKET(class, NAMESPACE_TICKET);
    }
    return class->typeNameSpace;
}

JITINT8 *getCompleteNameFromClassDescriptor (ClassDescriptor *class) {
    if (!WAIT_TICKET(class, COMPLETE_NAME_TICKET)) {
        /*not already cached */
        JITINT8 *name = getNameFromClassDescriptor(class);
        TypeDescriptor *enclosing_type = getEnclosingClass(class);
        JITINT8 *prefix = (JITINT8 *) "\0";
        if (enclosing_type == NULL) {
            prefix = getTypeNameSpaceFromClassDescriptor(class);
        } else {
            prefix = getCompleteNameFromTypeDescriptor(enclosing_type);
        }
        if (STRLEN(prefix) > 0) {
            size_t length = sizeof(JITINT8) * (STRLEN(name) + STRLEN(prefix) + 2);
            class->completeName = sharedAllocFunction(length);
            if (enclosing_type == NULL) {
                SNPRINTF(class->completeName, length, "%s.%s", prefix, name);
            } else {
                SNPRINTF(class->completeName, length, "%s+%s", prefix, name);
            }
        } else {
            class->completeName = class->name;
        }
        BROADCAST_TICKET(class, COMPLETE_NAME_TICKET);
    }
    return class->completeName;
}

static inline void internalGetDirectImplementedFields (ClassDescriptor *class) {
    class->directFields = xanList_new(sharedAllocFunction, freeFunction, NULL);
    class->fieldsMap = xanHashTable_new(11, 0, sharedAllocFunction, dynamicReallocFunction, freeFunction, hashString, equalsString);
    t_binary_information *binary = (t_binary_information *) class->row->binary;
    t_streams_metadata      *streams = &((binary->metadata).streams_metadata);
    t_metadata_tables *tables = &((streams->not_stream).tables);

    t_field_table *field_table = get_table(tables, FIELD_TABLE);
    if (field_table == NULL) {
        return;
    }
    BasicClass *next_type_row = (BasicClass *) get_row(tables, TYPE_DEF_TABLE, class->row->index + 1);
    JITUINT32 last_field;
    if (next_type_row != NULL) {
        last_field = next_type_row->field_list - 1;
    } else {
        last_field = field_table->cardinality;
    }
    JITUINT32 count;
    TypeDescriptor *owner = getTypeDescriptorFromClassDescriptor(class);
    for (count = class->row->field_list; count <= last_field; count++) {
        BasicField *field_row = (BasicField *) get_row(tables, FIELD_TABLE, count);
        FieldDescriptor *field = newFieldDescriptor(class->manager, field_row, owner);
        xanList_append(class->directFields, field);
        JITINT8 *field_name = getNameFromFieldDescriptor(field);
        xanHashTable_insert(class->fieldsMap, (void *) field_name, (void *) field);
    }
}

inline XanList *getDirectImplementedFieldsClassDescriptor (ClassDescriptor *class) {
    if (!WAIT_TICKET(class, FIELD_TICKET)) {
        internalGetDirectImplementedFields(class);
        BROADCAST_TICKET(class, FIELD_TICKET);
    }
    /* already cached */
    return class->directFields;
}


inline FieldDescriptor *getFieldDescriptorFromNameClassDescriptor (ClassDescriptor *class, JITINT8 * name) {
    if (!WAIT_TICKET(class, FIELD_TICKET)) {
        internalGetDirectImplementedFields(class);
        BROADCAST_TICKET(class, FIELD_TICKET);
    }
    /* already cached */
    return (FieldDescriptor *) xanHashTable_lookup(class->fieldsMap, (void *) name);
}

static inline void internalGetDirectDeclaredMethods (ClassDescriptor *class) {
    class->directMethods = xanList_new(sharedAllocFunction, freeFunction, NULL);
    class->constructors = xanList_new(sharedAllocFunction, freeFunction, NULL);
    class->methodsMap = xanHashTable_new(11, 0, sharedAllocFunction, dynamicReallocFunction, freeFunction, hashString, equalsString);
    t_binary_information *binary = (t_binary_information *) class->row->binary;
    t_streams_metadata      *streams = &((binary->metadata).streams_metadata);
    t_metadata_tables *tables = &((streams->not_stream).tables);

    t_method_def_table *method_table = get_table(tables, METHOD_DEF_TABLE);
    if (method_table == NULL) {
        return;
    }
    BasicClass *next_type_row = (BasicClass *) get_row(tables, TYPE_DEF_TABLE, class->row->index + 1);
    JITUINT32 last_method;
    if (next_type_row != NULL) {
        last_method = next_type_row->method_list - 1;
    } else {
        last_method = method_table->cardinality;
    }
    JITUINT32 count;
    TypeDescriptor *owner = getTypeDescriptorFromClassDescriptor(class);
    for (count = class->row->method_list; count <= last_method; count++) {
        BasicMethod *method_row = (BasicMethod *) get_row(tables, METHOD_DEF_TABLE, count);
        MethodDescriptor *method = newMethodDescriptor(class->manager, method_row, owner, NULL);
        xanList_append(class->directMethods, method);
        JITINT8 *method_name = getNameFromMethodDescriptor(method);
        XanList *method_by_name = xanHashTable_lookup(class->methodsMap, (void *) method_name);
        if (method_by_name == NULL) {
            method_by_name = xanList_new(sharedAllocFunction, freeFunction, NULL);
            xanHashTable_insert(class->methodsMap, (void *) method_name, method_by_name);
        }
        xanList_append(method_by_name, (void *) method);

        if (isFinalizer(method)) {
            class->finalizer = method;
        }
        if (isCctor(method)) {
            class->cctor = method;
        }
        if (isCtor(method)) {
            xanList_append(class->constructors, (void *) method);
        }
    }
}

inline XanList *getDirectDeclaredMethodsClassDescriptor (ClassDescriptor *class) {
    if (!WAIT_TICKET(class, METHOD_TICKET)) {
        internalGetDirectDeclaredMethods(class);
        BROADCAST_TICKET(class, METHOD_TICKET);
    }
    /*already cached */
    return class->directMethods;
}


inline XanList *getMethodDescriptorFromNameClassDescriptor (ClassDescriptor *class, JITINT8 * name) {
    if (!WAIT_TICKET(class, METHOD_TICKET)) {
        internalGetDirectDeclaredMethods(class);
        BROADCAST_TICKET(class, METHOD_TICKET);
    }
    /* already cached */
    return (XanList *) xanHashTable_lookup(class->methodsMap, (void *) name);
}

XanList *getConstructorsClassDescriptor (ClassDescriptor *class) {
    if (!WAIT_TICKET(class, METHOD_TICKET)) {
        internalGetDirectDeclaredMethods(class);
        BROADCAST_TICKET(class, METHOD_TICKET);
    }
    /*already cached */
    return class->constructors;
}

inline MethodDescriptor *getCctorClassDescriptor (ClassDescriptor *class) {
    if (!WAIT_TICKET(class, METHOD_TICKET)) {
        internalGetDirectDeclaredMethods(class);
        BROADCAST_TICKET(class, METHOD_TICKET);
    }
    /* already cached */
    return class->cctor;
}

inline MethodDescriptor *getFinalizerClassDescriptor (ClassDescriptor *class) {
    if (!WAIT_TICKET(class, METHOD_TICKET)) {
        internalGetDirectDeclaredMethods(class);
        BROADCAST_TICKET(class, METHOD_TICKET);
    }
    /* already cached */
    return class->finalizer;
}


static inline void internalGetDirectImplementedEvents (ClassDescriptor *class) {
    t_binary_information *binary = (t_binary_information *) class->row->binary;
    t_streams_metadata      *streams = &((binary->metadata).streams_metadata);
    t_metadata_tables *tables = &((streams->not_stream).tables);

    class->events = xanList_new(sharedAllocFunction, freeFunction, NULL);
    class->eventsMap = xanHashTable_new(11, 0, sharedAllocFunction, dynamicReallocFunction, freeFunction, hashString, equalsString);

    t_row_event_map_table *current_line = getRowByKey(class->manager, binary, EVENTS, (void *) class->row->index);
    if (current_line != NULL) {
        t_row_event_map_table *next_row = (t_row_event_map_table *) get_row(tables, EVENT_MAP_TABLE, current_line->index + 1);
        t_event_table *event_table = get_table(tables, EVENT_TABLE);
        JITUINT32 last;
        JITUINT32 count;
        if (next_row != NULL) {
            last = next_row->event_list - 1;
        } else {
            last = event_table->cardinality;
        }
        TypeDescriptor *owner = getTypeDescriptorFromClassDescriptor(class);
        for (count = current_line->event_list; count <= last; count++) {
            BasicEvent *row = (BasicEvent *) get_row(tables, EVENT_TABLE, count);
            EventDescriptor *event = newEventDescriptor(class->manager, row, owner);
            xanList_append(class->events, event);
            JITINT8 *event_name = getNameFromEventDescriptor(event);
            xanHashTable_insert(class->eventsMap, (void *) event_name, (void *) event);
        }
    }
}

inline XanList * getEventsClassDescriptor (ClassDescriptor *class) {
    if (!WAIT_TICKET(class, EVENT_TICKET)) {
        internalGetDirectImplementedEvents(class);
        BROADCAST_TICKET(class, EVENT_TICKET);
    }
    /* already cached */
    return class->events;
}

inline EventDescriptor *getEventDescriptorFromNameClassDescriptor (ClassDescriptor *class, JITINT8 * name) {
    if (!WAIT_TICKET(class, EVENT_TICKET)) {
        internalGetDirectImplementedEvents(class);
        BROADCAST_TICKET(class, EVENT_TICKET);
    }
    /* already cached */
    return (EventDescriptor *) xanHashTable_lookup(class->eventsMap, (void *) name);
}

static inline void internalGetDirectImplementedProperties (ClassDescriptor *class) {
    t_binary_information *binary = (t_binary_information *) class->row->binary;
    t_streams_metadata      *streams = &((binary->metadata).streams_metadata);
    t_metadata_tables *tables = &((streams->not_stream).tables);

    class->properties = xanList_new(sharedAllocFunction, freeFunction, NULL);
    class->propertiesMap = xanHashTable_new(11, 0, sharedAllocFunction, dynamicReallocFunction, freeFunction, hashString, equalsString);

    t_row_property_map_table *current_line = (t_row_property_map_table *) getRowByKey(class->manager, binary, PROPERTIES, (void *) class->row->index);
    if (current_line != NULL) {
        t_row_property_map_table *next_row = (t_row_property_map_table *) get_row(tables, PROPERTY_MAP_TABLE, current_line->index + 1);
        t_property_map_table *property_table = get_table(tables, PROPERTY_TABLE);
        JITUINT32 last;
        JITUINT32 count;
        if (next_row != NULL) {
            last = next_row->property_list - 1;
        } else {
            last = property_table->cardinality;
        }
        TypeDescriptor *owner = getTypeDescriptorFromClassDescriptor(class);
        for (count = current_line->property_list; count <= last; count++) {
            BasicProperty *row = (BasicProperty *) get_row(tables, PROPERTY_TABLE, count);
            PropertyDescriptor *property = newPropertyDescriptor(class->manager, row, owner);
            xanList_append(class->properties, property);
            JITINT8 *property_name = getNameFromPropertyDescriptor(property);
            xanHashTable_insert(class->propertiesMap, (void *) property_name, (void *) property);
        }
    }
}

inline XanList *getPropertiesClassDescriptor (ClassDescriptor *class) {
    if (!WAIT_TICKET(class, PROPERTY_TICKET)) {
        internalGetDirectImplementedProperties(class);
        BROADCAST_TICKET(class, PROPERTY_TICKET);
    }
    /* already cached */
    return class->properties;
}

inline PropertyDescriptor *getPropertyDescriptorFromNameClassDescriptor (ClassDescriptor *class, JITINT8 * name) {
    if (!WAIT_TICKET(class, PROPERTY_TICKET)) {
        internalGetDirectImplementedProperties(class);
        BROADCAST_TICKET(class, PROPERTY_TICKET);
    }
    /* already cached */
    return (PropertyDescriptor *) xanHashTable_lookup(class->propertiesMap, (void *) name);
}

inline BasicAssembly *getAssemblyFromClassDescriptor (ClassDescriptor *class) {
    if (!WAIT_TICKET(class, ASSEMBLY_TICKET)) {
        /* not already cached */
        t_binary_information *binary = (t_binary_information *) class->row->binary;
        t_streams_metadata      *streams = &((binary->metadata).streams_metadata);
        t_metadata_tables *tables = &((streams->not_stream).tables);
        class->assembly = (BasicAssembly *) get_row(tables, ASSEMBLY_TABLE, 1);
        BROADCAST_TICKET(class,  ASSEMBLY_TICKET);
    }
    return class->assembly;
}


inline XanList *getGenericParametersFromClassDescriptor (ClassDescriptor *type) {
    if (!WAIT_TICKET(type, GENERIC_PARAM_TICKET)) {
        /* not already cached */
        if (type->attributes->param_count == 0) {
            type->genericParameters = xanList_new(sharedAllocFunction, freeFunction, NULL);
        } else {
            if (type->container != NULL) {
                ClassDescriptor *genericClass = newClassDescriptor(type->manager, type->row, NULL);
                type->genericParameters = getGenericParametersFromClassDescriptor(genericClass);
            } else {
                type->genericParameters = xanList_new(sharedAllocFunction, freeFunction, NULL);
                t_binary_information *binary = (t_binary_information *) type->row->binary;
                TypeDescriptor *owner = getTypeDescriptorFromClassDescriptor(type);
                XanList *list = getRowByKey(type->manager, binary, CLASS_GENERIC_PARAM, (void *) type->row->index);
                if (list != NULL) {
                    XanListItem *item = xanList_first(list);
                    while (item != NULL) {
                        BasicGenericParam *row = (BasicGenericParam *) item->data;
                        GenericVarDescriptor *var = newGenericVarDescriptor(type->manager, row, owner);
                        xanList_append(type->genericParameters, (void *) var);
                        item = item->next;
                    }
                }
            }
        }
        BROADCAST_TICKET(type, GENERIC_PARAM_TICKET);
    }
    return type->genericParameters;
}

inline JITBOOLEAN isGenericClassDefinition (ClassDescriptor *class) {
    return class->attributes->param_count > 0 && class->container == NULL;
}

inline JITBOOLEAN containOpenGenericParameterFromClassDescriptor (ClassDescriptor *class) {
    if (isGenericClassDefinition(class)) {
        return JITTRUE;
    }
    if (class->container == NULL) {
        return JITFALSE;
    }
    JITUINT32 count;
    for (count = 0; count < class->container->arg_count; count++) {
        if (containOpenGenericParameterFromTypeDescriptor(class->container->paramType[count])) {
            return JITTRUE;
        }
    }
    return JITFALSE;
}

inline JITBOOLEAN isGenericClassDescriptor (ClassDescriptor *class) {
    return class->attributes->param_count > 0;
}

inline TypeDescriptor *getInstanceFromClassDescriptor (ClassDescriptor *class, GenericContainer *container) {
    ClassDescriptor *result = NULL;

    if (isGenericClassDefinition(class)) {
        if (checkClassDescriptorContainerConsistency(class, container)) {
            result = newClassDescriptor(class->manager, class->row, container);
        }
    }
    return getTypeDescriptorFromClassDescriptor(result);
}

inline TypeDescriptor *getGenericDefinitionFromClassDescriptor (ClassDescriptor *class) {
    ClassDescriptor *result = class;

    if (!isGenericClassDefinition(result)) {
        result = newClassDescriptor(class->manager, class->row, NULL);
    }
    return getTypeDescriptorFromClassDescriptor(class);
}

inline GenericContainer *getResolvingContainerFromClassDescriptor (ClassDescriptor *class) {
    if (class->attributes->param_count == 0) {
        return NULL;
    }
    if (!WAIT_TICKET(class, RESOLVING_CONTAINER_TICKET)) {
        /*not already cached*/
        if (class->container != NULL) {
            class->resolving_container = class->container;
        } else {
            XanList *genericParameters = getGenericParametersFromClassDescriptor(class);
            JITUINT32 paramCount = xanList_length(genericParameters);
            class->resolving_container = newGenericContainer(CLASS_LEVEL, NULL, paramCount);
            JITUINT32 count = 0;
            XanListItem *item = xanList_first(genericParameters);
            while (item != NULL) {
                GenericVarDescriptor *var = (GenericVarDescriptor *) item->data;
                TypeDescriptor *resolved_type = newTypeDescriptor(class->manager, ELEMENT_VAR, (void *) var, JITFALSE);
                insertTypeDescriptorIntoGenericContainer(class->resolving_container, resolved_type, count);
                item = item->next;
                count++;
            }
        }
        BROADCAST_TICKET(class, RESOLVING_CONTAINER_TICKET);
    }
    return class->resolving_container;
}

inline void classDescriptorLock (ClassDescriptor *class) {
    PLATFORM_lockMutex(&(class->mutex));
}

inline void classDescriptorUnLock (ClassDescriptor *class) {
    PLATFORM_unlockMutex(&(class->mutex));
}

inline DescriptorMetadataTicket *classDescriptorCreateDescriptorMetadataTicket (ClassDescriptor * class, JITUINT32 type) {
    return createDescriptorMetadataTicket(&(class->metadata), type);
}

inline void classDescriptorBroadcastDescriptorMetadataTicket (ClassDescriptor * class,  DescriptorMetadataTicket *ticket, void * data) {
    broadcastDescriptorMetadataTicket(ticket, data);
}
