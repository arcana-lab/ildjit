/*
 * Copyright (C) 2006 - 2012 Campanoni Simone, Di Biagio Andrea, Farina Roberto, Tartara Michele
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
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ecma_constant.h>
#include <metadata_manager.h>
#include <ir_language.h>
#include <compiler_memory_manager.h>
#include <jit_metadata.h>
#include <platform_API.h>

// My headers
#include <layout_manager.h>
#include <climanager_system.h>
// End

#define IMT_SIZE 50

pthread_once_t layoutManagerLock = PTHREAD_ONCE_INIT;

extern CLIManager_t *cliManager;

typedef struct {
    ILLayout    *layout;
    void        **vTable;
    IMTElem     **IMT;
} request_getMethodTables_t;

static inline void internal_layoutInitTables (ILLayout_manager *manager, ILLayout *layout);
static inline IRVM_type * layoutJITType (ILLayout_manager *manager, TypeDescriptor *type);
static inline ILLayout * layoutType (ILLayout_manager *manager, TypeDescriptor *type);
static inline ILLayoutStatic *layoutStaticType (ILLayout_manager *manager, TypeDescriptor *type);
static inline ILLayoutStatic * retrieve_static_type_layout (ILLayout_manager *manager, TypeDescriptor *type);
static inline ILLayout * retrieve_type_layout (ILLayout_manager *manager, TypeDescriptor *type);
static inline ILLayout * _alloc_ILLayout_infos (ILLayout_manager *manager);
static inline ILLayoutStatic * _alloc_ILLayoutStatic (ILLayout_manager *manager);
static inline void add_value_type_infos (ILLayout *layout);
static inline void internal_layoutFields (ILLayout_manager *manager, ILLayout *layout_infos);
static inline void debug_printFieldInfos (FieldDescriptor *field);
static inline ILFieldLayout * _alloc_ILFieldLayout_infos ();
static inline void updateFieldOffsets (ILLayout *layout_infos, JITUINT32 starting_offset);
static inline void _updateFields (ILLayout *layout_infos, ILLayout *supertype_layout);
static inline ILFieldLayout * _get_Field_Layout_Infos (FieldDescriptor *fieldID);
static inline JITBOOLEAN addFieldLayoutInfo (ILLayout *type_layout, ILFieldLayout *field_layout);
static inline JITUINT32 _perform_auto_layout (ILLayout *infos, JITUINT32 starting_offset);
static inline JITUINT32 _perform_sequential_layout (ILLayout *infos, JITUINT32 starting_offset);
static inline JITUINT32 _perform_explicit_layout (ILLayout *infos);
static inline JITUINT32 _perform_static_layout (ILLayoutStatic *infos);
static inline JITUINT32 internal_perform_fields_layout (XanList *fields, JITUINT32 startingOffset);
static inline VCallPath prepareVirtualMethodInvocation (ILLayout_manager *manager, MethodDescriptor *methodID);
static inline MethodImplementations *getMethodImplementations (ILLayout_manager *manager, MethodDescriptor *methodID);
static inline void * internal_getFunctionPointer (MethodDescriptor *cil_method);
static inline void * getGenericFunctionPointer (MethodDescriptor *cil_method, GenericContainer *container);
static inline JITINT32 getIndexOfMethodDescriptor (MethodDescriptor *method);
static inline void insertIntoIMT (IMTElem *bucket, MethodDescriptor *method, void *pointer);
static inline void *getFromIMT (IMTElem *bucket, MethodDescriptor *method);
static inline ILFieldLayout * fetchFieldLayout (ILLayout *layout, FieldDescriptor *field);
static inline ILFieldLayout * fetchStaticFieldLayout (ILLayoutStatic *layout, FieldDescriptor *field);
static inline ILLayout * getRuntimeFieldHandleLayout (ILLayout_manager *manager);
static inline ILLayout * getRuntimeMethodHandleLayout (ILLayout_manager *manager);
static inline ILLayout * getRuntimeTypeHandleLayout (ILLayout_manager *manager);
static inline ILLayout * getRuntimeArgumentHandleLayout (ILLayout_manager *manager);
static inline void internal_layout_manager_initialize (void);
static inline void internal_layout_manager_load_handle (void);
static inline void internal_dumpStaticLayout (ILLayoutStatic *layout);

void LAYOUT_initManager (ILLayout_manager *manager) {

    /* initialize the methodImplementationInfos hashtable */
    manager->methodImplementationInfos = xanHashTable_new(11, 0, sharedAllocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    assert(manager->methodImplementationInfos != NULL);

    manager->cachedVirtualMethodRequests	= xanList_new(allocFunction, freeFunction, NULL);
    manager->cachedVirtualTableSets         = xanList_new(allocFunction, freeFunction, NULL);

    manager->classesLayout = xanList_new(sharedAllocFunction, freeFunction, NULL);
    assert(manager->classesLayout != NULL);

    manager->staticTypeLayout = xanList_new(sharedAllocFunction, freeFunction, NULL);
    assert(manager->staticTypeLayout != NULL);

    /* initialize the runtime handles layout infos */
    manager->_Runtime_Field_Handle = NULL;
    manager->_Runtime_Method_Handle = NULL;
    manager->_Runtime_Type_Handle = NULL;
    manager->_Runtime_Argument_Handle = NULL;

    /* initialize the function pointers             */
    manager->layoutType = layoutType;
    manager->layoutJITType = layoutJITType;
    manager->layoutStaticType = layoutStaticType;
    manager->prepareVirtualMethodInvocation = prepareVirtualMethodInvocation;
    manager->getMethodImplementations = getMethodImplementations;
    manager->getRuntimeFieldHandleLayout = getRuntimeFieldHandleLayout;
    manager->getRuntimeMethodHandleLayout = getRuntimeMethodHandleLayout;
    manager->getRuntimeTypeHandleLayout = getRuntimeTypeHandleLayout;
    manager->getRuntimeArgumentHandleLayout = getRuntimeArgumentHandleLayout;
    manager->initialize = internal_layout_manager_initialize;
    manager->getIndexOfMethodDescriptor = getIndexOfMethodDescriptor;

    return ;
}

void LAYOUT_setCachingRequestsForVirtualMethodTables (ILLayout_manager *manager, JITBOOLEAN cache) {
    manager->cacheVirtualMethodRequests	= cache;

    return ;
}

void LAYOUT_setMethodTables (TypeDescriptor *ilType, void *vTable, IMTElem *IMT) {
    ILLayout 	*layout;
    JITBOOLEAN	cache;

    /* Assertions.
     */
    assert(ilType != NULL);

    /* Fetch the previous state.
     */
    cache		= (cliManager->layoutManager).cacheVirtualMethodRequests;

    /* Disable the creation of tables.
     */
    (cliManager->layoutManager).cacheVirtualMethodRequests	= JITTRUE;

    /* Fetch the layout.
     */
    layout		= layoutType(&(cliManager->layoutManager), ilType);
    assert(layout != NULL);
    assert(layout->virtualTable != NULL);
    assert(layout->virtualTableLength > 0);
    assert(layout->interfaceMethodTable != NULL);
    assert((*layout->virtualTable) == NULL);
    assert((*layout->interfaceMethodTable) == NULL);

    /* Restore the state.
     */
    (cliManager->layoutManager).cacheVirtualMethodRequests	= cache;

    /* Set the tables.
     */
    (*(layout->virtualTable))	= vTable;
    (*layout->interfaceMethodTable)	= IMT;

    return ;
}

void LAYOUT_getMethodTables (ILLayout *layout, void **vTable, IMTElem **IMT) {

    /* Assertions
     */
    assert(layout != NULL);
    assert(vTable != NULL);
    assert(IMT != NULL);
    assert(layout->type != NULL);
    assert(layout->type->attributes != NULL);
    assert(!layout->type->attributes->isAbstract);

    if ((cliManager->layoutManager).cacheVirtualMethodRequests) {
        xanList_lock((cliManager->layoutManager).cachedVirtualTableSets);
        request_getMethodTables_t   *request;
        request = allocFunction(sizeof(request_getMethodTables_t));
        request->layout = layout;
        request->vTable = vTable;
        request->IMT    = IMT;
        xanList_append((cliManager->layoutManager).cachedVirtualTableSets, request);
        xanList_unlock((cliManager->layoutManager).cachedVirtualTableSets);
    }

    PLATFORM_lockMutex(&(layout->mutex));
    if (*(layout->virtualTable) == NULL || *(layout->interfaceMethodTable) == NULL) {
        internal_layoutInitTables(layout->manager, layout);
    }
    (*vTable) = *(layout->virtualTable);
    (*IMT) = *(layout->interfaceMethodTable);
    PLATFORM_unlockMutex(&(layout->mutex));

    return ;
}

void LAYOUT_createCachedVirtualMethodTables (ILLayout_manager *manager) {
    XanListItem	*item;
    JITBOOLEAN	toCache;

    /* Assertions.
     */
    assert(manager != NULL);
    assert(manager->cachedVirtualMethodRequests != NULL);

    /* Disable caching for now.
     */
    toCache	= manager->cacheVirtualMethodRequests;
    manager->cacheVirtualMethodRequests	= JITFALSE;

    /* Create virtual method tables.
     */
    item	= xanList_first(manager->cachedVirtualMethodRequests);
    while (item != NULL) {
        ILLayout 	*layout;
        layout	= item->data;
        assert(layout != NULL);
        internal_layoutInitTables(manager, layout);
        item	= item->next;
    }

    /* Perform the requests to store virtual tables.
     */
    xanList_lock(manager->cachedVirtualTableSets);
    item	= xanList_first(manager->cachedVirtualTableSets);
    while (item != NULL) {
        request_getMethodTables_t   *request;
        request         = item->data;
        assert(request != NULL);
        LAYOUT_getMethodTables(request->layout, request->vTable, request->IMT);
        item	= item->next;
    }
    xanList_deleteAndFreeElements(manager->cachedVirtualTableSets);
    xanList_unlock(manager->cachedVirtualTableSets);

    /* Re-enable caching if it was enabled before.
     */
    manager->cacheVirtualMethodRequests	= toCache;

    /* Removed the requests.
     */
    xanList_emptyOutList(manager->cachedVirtualMethodRequests);

    return ;
}

void LAYOUT_destroyManager (ILLayout_manager *manager) {
    XanHashTableItem	*hashItem;

    /* Destroy the metadata about method implementations.
     */
    hashItem	= xanHashTable_first(manager->methodImplementationInfos);
    while (hashItem != NULL) {
        MethodImplementations	*m;
        m		= hashItem->element;
        assert(m != NULL);
        xanList_destroyList(m->implementor);
        xanList_destroyList(m->closure);
        freeFunction(m);
        hashItem	= xanHashTable_next(manager->methodImplementationInfos, hashItem);
    }

    /* Destroy the internal tables and lists.
     */
    xanHashTable_destroyTable(manager->methodImplementationInfos);
    xanList_destroyList(manager->classesLayout);
    xanList_destroyList(manager->staticTypeLayout);
    xanList_destroyList(manager->cachedVirtualMethodRequests);
    xanList_destroyListAndData(manager->cachedVirtualTableSets);

    return ;
}

static inline void internal_layout_manager_initialize (void) {

    /* Initialize the module	*/
    PLATFORM_pthread_once(&layoutManagerLock, internal_layout_manager_load_handle);

    /* Return			*/
    return;
}

static inline void internal_layout_manager_load_handle (void) {
    TypeDescriptor *typeFound;

    ILLayout_manager *manager = &(cliManager->layoutManager);

    typeFound = (cliManager->CLR).runtimeHandleManager.fillILRuntimeHandle(&((cliManager->CLR).runtimeHandleManager),  SYSTEM_RUNTIMEARGUMENTHANDLE);
    assert(typeFound != NULL);
    manager->_Runtime_Argument_Handle = layoutType(manager, typeFound);
    assert(manager->_Runtime_Argument_Handle != NULL);

    typeFound = (cliManager->CLR).runtimeHandleManager.fillILRuntimeHandle(&((cliManager->CLR).runtimeHandleManager), SYSTEM_RUNTIMETYPEHANDLE);
    assert(typeFound != NULL);
    manager->_Runtime_Type_Handle = manager->layoutType(manager, typeFound);
    assert(manager->_Runtime_Type_Handle != NULL);

    typeFound = (cliManager->CLR).runtimeHandleManager.fillILRuntimeHandle(&((cliManager->CLR).runtimeHandleManager), SYSTEM_RUNTIMEMETHODHANDLE);
    assert(typeFound != NULL);
    manager->_Runtime_Method_Handle = layoutType(manager, typeFound);
    assert(manager->_Runtime_Method_Handle != NULL);

    typeFound = (cliManager->CLR).runtimeHandleManager.fillILRuntimeHandle(&((cliManager->CLR).runtimeHandleManager), SYSTEM_RUNTIMEFIELDHANDLE);
    assert(typeFound != NULL);
    manager->_Runtime_Field_Handle = manager->layoutType(manager, typeFound);
    assert(manager->_Runtime_Field_Handle != NULL);
}

static inline ILLayout * getRuntimeArgumentHandleLayout (ILLayout_manager *manager) {

    /* assertions */
    assert(manager != NULL);

    PLATFORM_pthread_once(&layoutManagerLock, internal_layout_manager_load_handle);

    return manager->_Runtime_Argument_Handle;
}

static inline ILLayout * getRuntimeTypeHandleLayout (ILLayout_manager *manager) {

    /* assertions */
    assert(manager != NULL);

    PLATFORM_pthread_once(&layoutManagerLock, internal_layout_manager_load_handle);

    return manager->_Runtime_Type_Handle;
}

static inline ILLayout * getRuntimeMethodHandleLayout (ILLayout_manager *manager) {

    /* assertions */
    assert(manager != NULL);

    PLATFORM_pthread_once(&layoutManagerLock, internal_layout_manager_load_handle);

    return manager->_Runtime_Method_Handle;
}

static inline ILLayout * getRuntimeFieldHandleLayout (ILLayout_manager *manager) {

    /* assertions */
    assert(manager != NULL);

    PLATFORM_pthread_once(&layoutManagerLock, internal_layout_manager_load_handle);

    return manager->_Runtime_Field_Handle;
}

static inline ILLayoutStatic *layoutStaticType (ILLayout_manager *manager, TypeDescriptor *type) {

    /* Assertions                   */
    assert(manager != NULL);
    assert(type != NULL);
    PDEBUG("LAYOUT_MANAGER: layoutStaticType: Start\n");

    /* verify if the current type has already a ILLayout structure associated with */
    DescriptorMetadataTicket *ticket = type->createDescriptorMetadataTicket(type, STATIC_LAYOUT_METADATA);

    /* if not found, we have to evaluate the layout informations for the current type */
    if (ticket->data == NULL) {
        ILLayoutStatic  *layout;

        /* retrieve all the layout informations on the current type */
        layout = retrieve_static_type_layout(manager, type);
        assert(layout != NULL);
        //internal_dumpStaticLayout(layout);

        /* update the hashtable by inserting the new informations */
        type->broadcastDescriptorMetadataTicket(type, ticket, (void *) layout);
        xanList_syncAppend(manager->staticTypeLayout, layout);

    }

    /* Return the layout informations */
    PDEBUG("LAYOUT_MANAGER: layoutStaticType: Exit\n");
    return (ILLayoutStatic *) ticket->data;
}

static inline ILFieldLayout * fetchStaticFieldLayout (ILLayoutStatic *layout, FieldDescriptor *field) {
    ILFieldLayout   *currentFieldLayout;
    XanListItem     *currentItem;

    /* assertions */
    assert(layout != NULL);
    assert(field != NULL);
    assert(layout->fieldsLayout != NULL);

    /* initialize the local variables */
    currentFieldLayout = NULL;
    currentItem = NULL;

    /* retrieve the first field layout information */
    currentItem = xanList_first(layout->fieldsLayout);
    assert(currentItem != NULL);
    while (currentItem != NULL) {
        /* retrieve the layout infos for the current field */
        currentFieldLayout = (ILFieldLayout *) currentItem->data;
        assert(currentFieldLayout != NULL);
        assert(currentFieldLayout->field != NULL);

        /* verify if the field is found */
        if (currentFieldLayout->field == field) {
            return currentFieldLayout;
        }

        /* retrieve the next field informations */
        currentItem = currentItem->next;
    }

    return NULL;
}

static inline ILFieldLayout * fetchFieldLayout (ILLayout *layout, FieldDescriptor *field) {
    XanListItem     *currentItem;
    ILFieldLayout   *currentFieldLayout;

    /* assertions */
    assert(layout != NULL);
    assert(field != NULL);
    assert(layout->fieldsLayout != NULL);

    /* initialize the local variables */
    currentFieldLayout = NULL;
    currentItem = NULL;

    /* retrieve the first field layout information */
    currentItem = xanList_first(layout->fieldsLayout);
    assert(currentItem != NULL);
    while (currentItem != NULL) {
        /* retrieve the layout infos for the current field */
        currentFieldLayout = (ILFieldLayout *) xanList_getData(currentItem);
        assert(currentFieldLayout != NULL);
        assert(currentFieldLayout->field != NULL);

        /* verify if the field is found */
        if (currentFieldLayout->field == field) {
            return currentFieldLayout;
        }

        /* retrieve the next field informations */
        currentItem = currentItem->next;
    }

    return NULL;
}

static inline VCallPath prepareVirtualMethodInvocation (ILLayout_manager *manager, MethodDescriptor *methodID) {
    VCallPath result;

    /* Assertions			*/
    assert(manager != NULL);
    assert(methodID != NULL);
    PDEBUG("LAYOUT_MANAGER: prepareVirtualMethodInvocation: Start\n");

    if (methodID->attributes->param_count == 0) {

        xanHashTable_lock(manager->methodImplementationInfos);
        MethodImplementations *methodImplementors = xanHashTable_lookup(manager->methodImplementationInfos, methodID);
        if (methodImplementors == NULL) {
            methodImplementors = sharedAllocFunction(sizeof(MethodImplementations));
            methodImplementors->implementor = xanList_new(sharedAllocFunction, freeFunction, NULL);
            methodImplementors->closure = xanList_new(sharedAllocFunction, freeFunction, NULL);
            xanHashTable_insert(manager->methodImplementationInfos, methodID, methodImplementors);
        }
        xanHashTable_unlock(manager->methodImplementationInfos);

        if (methodID->owner->attributes->isInterface) {
            result.useIMT = JITTRUE;

            result.vtable_index = 0;
        } else {
            ILLayout *layout = layoutType(manager, methodID->owner);
            VTableSlot *vmethod = (VTableSlot *) xanHashTable_lookup(layout->methods, methodID);
            assert(vmethod != NULL);
            assert(vmethod->body != NULL);
            result.useIMT = JITFALSE;
            result.vtable_index = vmethod->index;
        }
    } else {
        MethodDescriptor *genericMethod = methodID->getGenericDefinition(methodID);

        xanHashTable_lock(manager->methodImplementationInfos);
        MethodImplementations *methodImplementors = xanHashTable_lookup(manager->methodImplementationInfos, genericMethod);
        if (methodImplementors == NULL) {
            methodImplementors = sharedAllocFunction(sizeof(MethodImplementations));
            methodImplementors->implementor = xanList_new(sharedAllocFunction, freeFunction, NULL);
            methodImplementors->closure = xanList_new(sharedAllocFunction, freeFunction, NULL);
            xanHashTable_insert(manager->methodImplementationInfos, genericMethod, methodImplementors);
        }
        xanHashTable_unlock(manager->methodImplementationInfos);
        assert(methodImplementors != NULL);
        xanList_syncAppend(methodImplementors->closure, methodID->container);

        xanList_lock(methodImplementors->implementor);
        XanListItem *item = xanList_first(methodImplementors->implementor);
        while (item != NULL) {
            ILLayout *layout = (ILLayout *) item->data;
            void *pointer = getFromIMT(*(layout->interfaceMethodTable), methodID);
            if (pointer == NULL) {
                VTableSlot *current_vmethod = (VTableSlot *) xanHashTable_lookup(layout->methods, genericMethod);
                pointer = getGenericFunctionPointer(current_vmethod->body, methodID->container);
                insertIntoIMT(*(layout->interfaceMethodTable), methodID, pointer);
            }
            item = item->next;
        }
        xanList_unlock(methodImplementors->implementor);

        result.useIMT = JITTRUE;
        result.vtable_index = 0;
    }

    /* Return the method		*/
    PDEBUG("LAYOUT_MANAGER: prepareVirtualMethodInvocation: Exit\n");
    return result;
}

static inline void add_value_type_infos (ILLayout *layout) {
    JITUINT32 packing_size = layout->packing_size;

    /* Allocate space */
    assert(*(layout->jit_type) == NULL);
    *(layout->jit_type) = allocFunction(sizeof(IRVM_type));

    /* update the layout infos */
    if (packing_size == 0) {
        packing_size = getIRSize(IRMPOINTER);
    }

    /* Initialize the ValueType in its jit representation */
    if (layout->type->isEnum(layout->type)) {

        /* An enum is an integer 32 bit			*/
        IRVM_translateIRTypeToIRVMType(cliManager->IRVM, IRINT32, *(layout->jit_type), NULL);

    } else {
        IRVM_setIRVMStructure(cliManager->IRVM, *(layout->jit_type), layout->typeSize, packing_size);
    }
}

void debug_print_layoutStatic_informations (ILLayoutStatic *infos) {

    /* assertions */
    assert(infos != NULL);
    assert(infos->type != NULL);
    assert(infos->typeSize >= 0);

    PDEBUG("----- START: LAYOUT-STATIC TYPE INFORMATIONS ----- \n");
    PDEBUG("TYPE_NAME: %s -----\n", infos->type->getCompleteName(infos->type));
    PDEBUG("TYPE_SIZE == %d \n", infos->typeSize);
    PDEBUG("INFORMATIONS ABOUT STATIC FIELDS ------ \n");
    if (infos->fieldsLayout == NULL) {
        PDEBUG("NO STATIC FIELDS ASSOCIATED WITH THIS TYPE \n");
    } else {
        ILFieldLayout   *currentFieldLayout;
        XanListItem     *currentItem;
        JITUINT32 numFields;

        /* initialize the local variable numFields */
        numFields = 0;

        /* retrieve the layout infos associated with the first field */
        currentItem = xanList_first(infos->fieldsLayout);
        assert(currentItem != NULL);
        currentFieldLayout = (ILFieldLayout *) xanList_getData(currentItem);
        assert(currentFieldLayout != NULL);

        while (currentFieldLayout != NULL) {
            numFields++;

            /* test the currentField layout informations consistency */
#ifdef DEBUG
            assert(currentFieldLayout->field != NULL);
            assert(currentFieldLayout->offset >= 0);
            assert(currentFieldLayout->length >= 0);
            if (currentFieldLayout->IRType == IRVALUETYPE) {
                assert(currentFieldLayout->layout != NULL);
            }
#endif

            /*retrieve the field informations */
            PDEBUG("------ FIELD #%d (static infos) ------ \n", numFields);
            PDEBUG("FIELD_NAME == \"%s\" \n", currentFieldLayout->field->getName(currentFieldLayout->field));
            if (currentFieldLayout->IRType == IRVALUETYPE) {
                debug_print_layout_informations
                ((ILLayout *) (currentFieldLayout->layout));
            }
            PDEBUG("FIELD_OFFSET == %d \n", currentFieldLayout->offset);
            PDEBUG("FIELD_LENGTH == %d \n", currentFieldLayout->length);
            PDEBUG("------ END FIELD #%d (static infos) ------ \n", numFields);

            /* loop-termination condition */
            currentItem = currentItem->next;
            if (currentItem == NULL) {
                break;
            }
            currentFieldLayout = (ILFieldLayout *) xanList_getData(currentItem);
            assert(currentFieldLayout != NULL);
        }

        /* verifying the post-conditions */
        assert(numFields > 0);

        PDEBUG("TOTAL_NUMBER_OF_FIELDS == %d \n", numFields);
    }

    PDEBUG("----- END: LAYOUT-STATIC TYPE INFORMATIONS ----- \n");
}

void debug_print_layout_informations (ILLayout *infos) {

    /* assertions */
    assert(infos != NULL);
    assert(infos->type != NULL);
    assert( (infos->type_of_layout == AUTOLAYOUT)
            || (infos->type_of_layout == SEQUENTIAL_LAYOUT)
            || (infos->type_of_layout == EXPLICIT_LAYOUT) );
    assert(infos->typeSize >= 0);
    if (infos->type_of_layout != SEQUENTIAL_LAYOUT) {
        assert(infos->packing_size == 0);
    }
    if (infos->virtualTableLength) {
        assert(infos->methods != NULL);
        assert(*(infos->virtualTable) != NULL);
    }

    PDEBUG("----- START: LAYOUT TYPE INFORMATIONS ----- \n");
    PDEBUG("TYPE_NAME: %s -----\n", infos->type->getCompleteName(infos->type));
    PDEBUG("TYPE_SIZE == %d \n", infos->typeSize);
    if (infos->type_of_layout == AUTOLAYOUT) {
        PDEBUG("TYPE_OF_LAYOUT == %s \n", "AUTO LAYOUT");
    } else if (infos->type_of_layout == SEQUENTIAL_LAYOUT) {
        PDEBUG("TYPE_OF_LAYOUT == %s \n", "SEQUENTIAL LAYOUT");
    } else {
        PDEBUG("TYPE_OF_LAYOUT == %s \n", "EXPLICIT LAYOUT");
    }
    PDEBUG("PACKING_SIZE == %d \n", infos->packing_size);
    PDEBUG("LENGTH OF THE VIRTUAL TABLE == %d \n", infos->virtualTableLength);
    PDEBUG("POINTER TO THE VIRTUAL TABLE == %p \n", *(infos->virtualTable));
    PDEBUG("INFORMATIONS ABOUT FIELDS ------ \n");
    if (infos->fieldsLayout == NULL) {
        PDEBUG("NO FIELDS ASSOCIATED WITH THIS TYPE \n");
    } else {
        ILFieldLayout   *currentFieldLayout;
        XanListItem     *currentItem;
        JITUINT32 numFields;

        /* initialize the local variable numFields */
        numFields = 0;

        /* retrieve the layout infos associated with the first field */
        currentItem = xanList_first(infos->fieldsLayout);
        assert(currentItem != NULL);
        currentFieldLayout = (ILFieldLayout *) xanList_getData(currentItem);
        assert(currentFieldLayout != NULL);

        while (currentFieldLayout != NULL) {
            numFields++;

            /* test the currentField layout informations consistency */
            assert(currentFieldLayout->field != NULL);
            assert(currentFieldLayout->offset >= 0);
            assert(currentFieldLayout->length >= 0);
            if (currentFieldLayout->IRType == IRVALUETYPE) {
                assert(currentFieldLayout->layout != NULL);
            }

            /*retrieve the field informations */
            PDEBUG("------ FIELD #%d ------ \n", numFields);
            PDEBUG("FIELD_NAME == \"%s\" \n", currentFieldLayout->field->getCompleteName(currentFieldLayout->field));
            if (currentFieldLayout->IRType == IRVALUETYPE) {
                debug_print_layout_informations((ILLayout *) (currentFieldLayout->layout));
            }
            PDEBUG("FIELD_OFFSET == %d \n", currentFieldLayout->offset);
            PDEBUG("FIELD_LENGTH == %d \n", currentFieldLayout->length);
            PDEBUG("------ END FIELD #%d ------ \n", numFields);

            /* loop-termination condition */
            currentItem = currentItem->next;
            if (currentItem == NULL) {
                break;
            }
            currentFieldLayout = (ILFieldLayout *) xanList_getData(currentItem);
            assert(currentFieldLayout != NULL);
        }

        /* verifying the post-conditions */
        assert(numFields > 0);

        PDEBUG("TOTAL_NUMBER_OF_FIELDS == %d \n", numFields);
    }

    PDEBUG("----- END: LAYOUT TYPE INFORMATIONS ----- \n");
}

static inline JITUINT32 _perform_explicit_layout (ILLayout *infos) {
    JITUINT32 	current_offset;
    JITUINT32 	current_size;
    XanListItem     *currentField;
    ILFieldLayout   *currentFieldLayout;

    /* Assertions 						*/
    assert(infos != NULL);
    assert(infos->type != NULL);
    assert(infos->type_of_layout == EXPLICIT_LAYOUT);
    assert(infos->fieldsLayout != NULL);

    /* Initialize the local variables 			*/
    currentField 		= NULL;
    currentFieldLayout 	= NULL;
    current_offset 		= 0;
    current_size 		= 0;

    /* Retrieve the offset of the last field in memory 	*/
    currentField = xanList_first(infos->fieldsLayout);
    assert(currentField != NULL);
    while (currentField != NULL) {

        /* Retrieve the field layout informations for the current field */
        currentFieldLayout = (ILFieldLayout *) currentField->data;
        assert(currentFieldLayout != NULL);
        assert(currentFieldLayout->length > 0);

        /* Set the offset						*/
        current_offset = (current_offset >= currentFieldLayout->offset) ? current_offset : currentFieldLayout->offset;
        current_size = currentFieldLayout->length;
        assert(current_size > 0);

        /* Fetch the next element					*/
        currentField = currentField->next;
    }
    assert(current_offset >= 0);
    assert(current_size > 0);

    /* Return the overall_size of an instance of the 	*
     * current type in memory 				*/
    return current_offset + current_size;
}

static inline JITUINT32 _perform_auto_layout (ILLayout *infos, JITUINT32 starting_offset) {
    return internal_perform_fields_layout(infos->fieldsLayout, starting_offset);
}

static inline JITUINT32 _perform_static_layout (ILLayoutStatic *infos) {
    return internal_perform_fields_layout(infos->fieldsLayout, 0);
}

static inline JITUINT32 _perform_sequential_layout (ILLayout *infos, JITUINT32 starting_offset) {
    XanListItem     *currentField;
    ILFieldLayout   *currentFieldLayout;
    JITUINT32 	currentOffset;
    JITUINT32 	_used_packing_size;
    JITUINT32 	padding;

    /* Assertions 				*/
    assert(infos != NULL);
    assert(infos->type != NULL);
    assert(infos->type_of_layout == SEQUENTIAL_LAYOUT);
    assert(infos->fieldsLayout != NULL);
    assert(infos->packing_size >= 0);

    /* Initialize the local variables 	*/
    currentFieldLayout 	= NULL;
    currentField 		= NULL;
    _used_packing_size 	= infos->packing_size;

    /* Set the inital offset		*/
    currentOffset 		= starting_offset;

    /* Adjust the offset based on the 	*
     * packing size				*/
    if (    (currentOffset > 0)		&&
            (_used_packing_size > 0) 	) {
        padding = currentOffset % _used_packing_size;
        assert(padding >= 0);
        currentOffset += padding;
    }

    /* retrieve the first field from the list */
    currentField = xanList_first(infos->fieldsLayout);
    assert(currentField != NULL);
    while (currentField != NULL) {

        /* Retrieve the field layout informations for the current field */
        currentFieldLayout = (ILFieldLayout *) currentField->data;
        assert(currentFieldLayout != NULL);
        assert(currentFieldLayout->length > 0);

        /* Assign the offset information 				*/
        currentFieldLayout->offset = currentOffset;

        /* Update the current offset information 			*/
        currentOffset += currentFieldLayout->length;

        /* Adjust the offset based on the packing size			*/
        if (_used_packing_size > 0) {
            padding = currentOffset % _used_packing_size;
            assert(padding >= 0);
            currentOffset += padding;
        }

        /* Retrieve the next field 					*/
        currentField = currentField->next;
    }

    /* Return the overall size of the type in memory */
    return currentOffset;
}

static inline void internal_layoutFields (ILLayout_manager *manager, ILLayout *layout_infos) {
    TypeDescriptor          *superType;
    ILLayout                *supertype_layout;

    /* Assertions                           */
    assert(manager != NULL);
    assert(layout_infos != NULL);
    assert(layout_infos->type != NULL);
    assert(layout_infos->fieldsLayout == NULL);

    /* initialize the local variables       */
    supertype_layout = NULL;


    superType = layout_infos->type->getSuperType(layout_infos->type);

    if (superType != NULL) {
        /* recursively call the layoutType procedure */
        supertype_layout = layoutType(manager, superType);
        assert(supertype_layout != NULL);
    }

    PDEBUG("LAYOUT_MANAGER: layoutFields: Class %s - START\n", layout_infos->type->getCompleteName(layout_infos->type));

    /* Layout the fields			*/
    XanList *fieldList = layout_infos->type->getDirectImplementedFields(layout_infos->type);
    XanListItem *item = xanList_first(fieldList);
    while (item != NULL) {
        FieldDescriptor *current_field;

        /* Fetch the field			*/
        current_field = (FieldDescriptor *) item->data;

        /* Print field informations 		*/
#ifdef PRINTDEBUG
        debug_printFieldInfos(current_field);
#endif

        /* Layout the field			*/
        if (!current_field->attributes->is_static) {
            ILFieldLayout   *field_layout;
            field_layout = _get_Field_Layout_Infos(current_field);
            if (field_layout->IRType == IRVALUETYPE) {
                ILLayout                *valueType_layout;
                TypeDescriptor  	*valueType;

                /* initialize the local variable `valueType` */
                valueType = NULL;

                /* retrieve the type of the valueType */
                valueType = field_layout->field->getType(field_layout->field);
                assert(valueType != NULL);

                /* verify if the ValueType has already an ILLayout structure associated with it */
                valueType_layout = layoutType(manager, valueType);

                /* update the layout informations for the value type */
                field_layout->layout = valueType_layout;

                /* update the `length` information for this field_layout */
                field_layout->length = valueType_layout->typeSize;

            } else {
                field_layout->length = getIRSize(field_layout->IRType);
                assert(field_layout->length > 0);
            }

            addFieldLayoutInfo(layout_infos, field_layout);
        }

        /* Fetch the next element		*/
        item = item->next;
    }
    PDEBUG("LAYOUT_MANAGER: layoutFields: Class %s - END\n", layout_infos->type->getCompleteName(layout_infos->type));

    if (supertype_layout != NULL) {
        /* update the field offsets */
        updateFieldOffsets(layout_infos, supertype_layout->typeSize);

        /* insert the other fields informations into the current layout */
        _updateFields(layout_infos, supertype_layout);
    } else {
        /* update the field offsets */
        updateFieldOffsets(layout_infos, 0);
    }
}

static inline void _updateFields (ILLayout *layout_infos, ILLayout *supertype_layout) {
    ILFieldLayout   *currentFieldLayout;
    XanListItem     *current_item;
    XanListItem     *previous_item;

    /* assertions */
    assert(layout_infos != NULL);
    assert(supertype_layout != NULL);

    /* preconditions */
    if (supertype_layout->fieldsLayout == NULL) {
        return;
    }

    if (layout_infos->fieldsLayout == NULL) {
        layout_infos->fieldsLayout = supertype_layout->fieldsLayout;
        layout_infos->typeSize = supertype_layout->typeSize;
        return;
    }

    /* retrieve the `previous_item` information */
    previous_item = xanList_first(layout_infos->fieldsLayout);
    assert(previous_item != NULL);

    /* retrieve the first field of the supertype */
    current_item = xanList_first(supertype_layout->fieldsLayout);
    assert(current_item != NULL);
    while (current_item != NULL) {
        /* retrieve the current field layout information */
        currentFieldLayout = xanList_getData(current_item);
        assert(currentFieldLayout != NULL);

        /* update the fieldsLayout informations */
        xanList_insertBefore(layout_infos->fieldsLayout, previous_item, currentFieldLayout);

        /* retrieve the next field */
        current_item = current_item->next;
    }
}

static inline void updateFieldOffsets (ILLayout *layout_infos, JITUINT32 starting_offset) {
    /* assertions */
    assert(layout_infos != NULL);
    if (layout_infos->fieldsLayout == NULL) {
        return;
    }

    PDEBUG("LAYOUT_MANAGER: updateFieldOffsets: START \n");

    /* if we have an explicit layout then we don't have to define the offsets for every
     * field because every field is associated with a particular offset-layout information */
    if (layout_infos->type_of_layout == EXPLICIT_LAYOUT) {
        layout_infos->typeSize = _perform_explicit_layout(layout_infos);
    } else if (layout_infos->type_of_layout == SEQUENTIAL_LAYOUT) {
        layout_infos->typeSize = _perform_sequential_layout(layout_infos, starting_offset);
    } else {
        assert(layout_infos->type_of_layout == AUTOLAYOUT);
        layout_infos->typeSize = _perform_auto_layout(layout_infos, starting_offset);
    }

    /* verify the postConditions */
    assert(layout_infos->typeSize > 0);

    PDEBUG("LAYOUT_MANAGER: updateFieldOffsets: EXIT \n");
}

static inline void debug_printFieldInfos (FieldDescriptor *field) {
    /* assertions */
    assert(field != NULL);

    PDEBUG("LAYOUT_MANAGER: debug_printFieldInfos: Field %s - START\n", field->getName(field));
    if (field->attributes->is_static == 1) {
        PDEBUG("LAYOUT_MANAGER: debug_printFieldInfos:			It is static\n");
    } else {
        PDEBUG("LAYOUT_MANAGER: debug_printFieldInfos:			It is not static\n");
    }
    if (field->attributes->is_special == 1) {
        PDEBUG("LAYOUT_MANAGER: debug_printFieldInfos:			It is special\n");
    } else {
        PDEBUG("LAYOUT_MANAGER: debug_printFieldInfos:			It is not special\n");
    }
    if (field->attributes->is_literal == 1) {
        PDEBUG("LAYOUT_MANAGER: debug_printFieldInfos:			It is a literal\n");
    } else {
        PDEBUG("LAYOUT_MANAGER: debug_printFieldInfos:			It is not a literal\n");
    }
    if (field->attributes->is_serializable == 1) {
        PDEBUG("LAYOUT_MANAGER: debug_printFieldInfos:			It is serializable\n");
    } else {
        PDEBUG("LAYOUT_MANAGER: debug_printFieldInfos:			It is not serializable\n");
    }
    if (field->attributes->is_init_only == 1) {
        PDEBUG("LAYOUT_MANAGER: debug_printFieldInfos:			It is only initializable\n");
    } else {
        PDEBUG("LAYOUT_MANAGER: debug_printFieldInfos:			It is not only initializable\n");
    }
    if (field->attributes->is_pinvoke == 1) {
        PDEBUG("LAYOUT_MANAGER: debug_printFieldInfos:			It is PInvoke\n");
    } else {
        PDEBUG("LAYOUT_MANAGER: debug_printFieldInfos:			It is not PInvoke\n");
    }
    if (field->attributes->is_rt_special_name == 1) {
        PDEBUG("LAYOUT_MANAGER: debug_printFieldInfos:			It is a special name\n");
    } else {
        PDEBUG("LAYOUT_MANAGER: debug_printFieldInfos:			It is not a special name\n");
    }
    if (field->attributes->has_marshal_information == 1) {
        PDEBUG("LAYOUT_MANAGER: debug_printFieldInfos:			It has a marshal information\n");
    } else {
        PDEBUG("LAYOUT_MANAGER: debug_printFieldInfos:			"
               "It has not a marshal information\n");
    }
    if (field->attributes->has_default == 1) {
        PDEBUG("LAYOUT_MANAGER: debug_printFieldInfos:			It has a default value\n");
    } else {
        PDEBUG("LAYOUT_MANAGER: debug_printFieldInfos:			It has not a default value\n");
    }
    if (field->attributes->has_rva == 1) {
        PDEBUG("LAYOUT_MANAGER: debug_printFieldInfos:			It has a RVA\n");
    }
    /*
                    print_err("LAYOUT_MANAGER: debug_printFieldInfos: TODO the RVA of fields. ", 0);
                    abort();
     */
    else {
        PDEBUG("LAYOUT_MANAGER: debug_printFieldInfos:			It has not a RVA\n");
    }

    PDEBUG("LAYOUT_MANAGER: debug_printFieldInfos: Field %s - EXIT\n", field->getCompleteName(field));
}

static inline JITBOOLEAN addFieldLayoutInfo (ILLayout *type_layout, ILFieldLayout *field_layout) {

    /* Assertions */
    assert(type_layout != NULL);

    if (field_layout == NULL) {
        PDEBUG("LAYOUT_MANAGER: addFieldLayoutInfo: WARNING: field_layout is NULL! \n");
        return 1;
    }

    /* if the fieldsLayout list is not initialized we have to create a new one
     * just before continue with the operation */
    if (type_layout->fieldsLayout == NULL) {
        type_layout->fieldsLayout = xanList_new(sharedAllocFunction, freeFunction, NULL);
    }

    /* add the field information as last element in the list of field layouts */
    xanList_append(type_layout->fieldsLayout, field_layout);

    return 0;
}

static inline ILLayout * layoutType (ILLayout_manager *manager, TypeDescriptor *type) {

    /* Assertions.
     */
    assert(manager != NULL);
    assert(type != NULL);
    PDEBUG("LAYOUT_MANAGER: layoutType: Start\n");
    PDEBUG("LAYOUT_MANAGER: layoutType:     Request for type \"%s\"\n", type->getCompleteName(type));

    /* Verify if the current type has already a ILLayout structure associated with */
    DescriptorMetadataTicket *ticket = type->createDescriptorMetadataTicket(type, INSTANCE_LAYOUT_METADATA);

    /* if not found, we have to evaluate the layout informations for the current type */
    if (ticket->data == NULL) {

        /* retrieve all the layout informations on the current type */
        PDEBUG("LAYOUT_MANAGER: layoutType:             The type is not already layout\n");
        ILLayout	*layout			= retrieve_type_layout(manager, type);
        void		**virtualTable		= *(layout->virtualTable);
        PDEBUG("LAYOUT_MANAGER: layoutType:                 Virtual table %p\n", virtualTable);
        IMTElem		*interfaceMethodTable	= *(layout->interfaceMethodTable);
        assert(layout != NULL);
        assert(layout->type != NULL);

        /* Check if it is an interface			*/
        if (!type->attributes->isInterface) {
            if (type->isValueType(type)) {
                /* the type is a ValueType -- update the value type informations */
                add_value_type_infos(layout);
            }

            /* update the hashtable by inserting the new informations */
            xanList_syncAppend(manager->classesLayout, layout);
        }

        type->broadcastDescriptorMetadataTicket(type, ticket, layout);

        PLATFORM_lockMutex(&(layout->mutex));
        if (!type->attributes->isAbstract && (virtualTable == NULL || interfaceMethodTable == NULL)) {
            PDEBUG("LAYOUT_MANAGER: layoutType:                 Layout the table\n");
            internal_layoutInitTables(manager, layout);
        }
        PLATFORM_unlockMutex(&(layout->mutex));

    } else if ((!type->attributes->isInterface) && (type->isValueType(type))) {
        ILLayout *layout = (ILLayout *) ticket->data;

        /* Handle the case of another instance made the layout */
        PLATFORM_lockMutex(&(layout->mutex));
        if ( *(layout->jit_type) == NULL ) {
            add_value_type_infos(layout);
        }
        PLATFORM_unlockMutex(&(layout->mutex));
    }

    /* Return the layout informations */
    PDEBUG("LAYOUT_MANAGER: layoutType: Exit\n");
    return (ILLayout *) ticket->data;
}

static inline IRVM_type * layoutJITType (ILLayout_manager *manager, TypeDescriptor *type) {

    /* Assertions			*/
    assert(manager != NULL);
    assert(type != NULL);
    PDEBUG("LAYOUT_MANAGER: layoutJITType: Start\n");
    PDEBUG("LAYOUT_MANAGER: layoutJITType:     Request for type \"%s\"\n", type->getCompleteName(type));

    /* verify if the current type has already a ILLayout structure associated with */
    DescriptorMetadataTicket *ticket = type->createDescriptorMetadataTicket(type, INSTANCE_LAYOUT_METADATA);

    /* if not found, we have to evaluate the layout informations for the current type */
    if (ticket->data == NULL) {

        /* retrieve all the layout informations on the current type */
        PDEBUG("LAYOUT_MANAGER: layoutJITType:             The type is not already layout\n");
        ILLayout *layout = retrieve_type_layout(manager, type);
        assert(layout != NULL);
        assert(layout->type != NULL);

        /* Check if it is an interface			*/
        if (!type->attributes->isInterface) {
            if (type->isValueType(type)) {
                /* the type is a ValueType -- update the value type informations */
                add_value_type_infos(layout);
            }

            /* update the hashtable by inserting the new informations */
            xanList_syncAppend(manager->classesLayout, layout);
        }

        type->broadcastDescriptorMetadataTicket(type, ticket, layout);

    } else if ((!type->attributes->isInterface) && (type->isValueType(type))) {
        ILLayout *layout = (ILLayout *) ticket->data;

        /* Handle the case of another instance made the layout */
        PLATFORM_lockMutex(&(layout->mutex));
        if ( *(layout->jit_type) == NULL ) {
            add_value_type_infos(layout);
        }
        PLATFORM_unlockMutex(&(layout->mutex));
    }

    /* Return the layout informations */
    PDEBUG("LAYOUT_MANAGER: layoutJITType: Exit\n");
    return *(((ILLayout *) ticket->data)->jit_type);
}

static inline ILLayoutStatic * retrieve_static_type_layout (ILLayout_manager *manager, TypeDescriptor *type) {
    FieldDescriptor         *current_field;
    ILLayoutStatic          *return_value;
    JITUINT32 rva_to_be_evaluated;
    void                    *default_obj;
    JITUINT32 entryPointAddress;
    JITUINT32 current_field_rva;
    JITINT8                     *buffer;
    JITINT32 bufferLength;

    /* assertions */
    assert(manager != NULL);
    assert(type != NULL);
    PDEBUG("LAYOUT_MANAGER: retrieve_static_type_layout: Start\n");

    /* initialize the local variables */
    rva_to_be_evaluated = JITFALSE;
    entryPointAddress = 0;
    current_field_rva = 0;
    default_obj = NULL;
    current_field = NULL;
    buffer = NULL;
    bufferLength = 0;

    return_value = _alloc_ILLayoutStatic(manager);
    assert(return_value != NULL);

    /* set the type information */
    return_value->type = type;
    return_value->initialized_obj = NULL;

    /* Field table present, layout fields */
    XanList *fields = type->getDirectImplementedFields(type);
    XanListItem *item = xanList_first(fields);
    while (item != NULL) {
        ILFieldLayout   *field_layout;

        current_field = (FieldDescriptor *) item->data;

        /* Check to see if the field is static	*/
        if (current_field->attributes->is_static) {

#ifdef PRINTDEBUG
            PDEBUG("LAYOUT_MANAGER: retrieve_static_type_layout: field %s has RVA: %d\n", current_field->getCompleteName(current_field), current_field->attributes->has_rva);
            debug_printFieldInfos(current_field);
#endif
            if (current_field->attributes->has_rva) {
                /* Set the flag to loop for RVAs later */
                rva_to_be_evaluated = 1;
            }

            /* retrieve the field layout informations for the current field */
            field_layout = _get_Field_Layout_Infos(current_field);
            assert(field_layout != NULL);

            if (field_layout->IRType == IRVALUETYPE) {
                ILLayout                *valueType_layout;
                TypeDescriptor          *valueType;

                PDEBUG("LAYOUT_MANAGER: retrieve_static_type_layout: IRType is IRVALUETYPE\n");

                /* initialize the local variable `valueType` */
                valueType = NULL;

                valueType = current_field->getType(current_field);
                assert(valueType != NULL);

                /* verify if the ValueType has already an ILLayout structure associated with it */
                valueType_layout = layoutType(manager, valueType);

                /* update the layout informations for the value type */
                field_layout->layout = valueType_layout;

                /* update the `length` information for this field_layout */
                field_layout->length = valueType_layout->typeSize;

                PDEBUG("LAYOUT_MANAGER: retrieve_static_type_layout : after evaluating the TypedReference of ValueType\n\t\tfield_layout->length: %d, field_layout->offset: %d\n", field_layout->length, field_layout->offset);
            } else {

                field_layout->length = getIRSize(field_layout->IRType);
            }

            /* test the post-conditions */
            assert(field_layout->length > 0);

            /* add to the field list the new instance of the field layout structure */
            xanList_append(return_value->fieldsLayout, field_layout);
        }

        item = item->next;
    }

    /* postconditions --> update the size value for the static type layout */
    PDEBUG("LAYOUT_MANAGER: retrieve_static_type_layout : Ready to perform static layout\n");
    return_value->typeSize = _perform_static_layout(return_value);
    PDEBUG("LAYOUT_MANAGER: retrieve_static_type_layout : Static layout performed\n");
    assert(return_value->typeSize >= 0);

    /* Nothing to initialize */
    if (return_value->typeSize == 0) {
        return_value->initialized_obj = NULL;
        return return_value;
    }

#ifdef PRINTDEBUG
    debug_print_layoutStatic_informations(return_value);
#endif

    PDEBUG("LAYOUT_MANAGER: the object size is %d bytes\n", return_value->typeSize);

    /* Allocate a memory buffer to be used as a default object to be initialized */
    default_obj = sharedAllocFunction(return_value->typeSize);
    assert(default_obj != NULL);
    /* Erase the object memory */
    memset(default_obj, 0, return_value->typeSize);

    if (rva_to_be_evaluated) {
        XanListItem     *currentField;
        ILFieldLayout   *currentFieldLayout;

        /* Allocate the buffer                  */
        bufferLength = 1024;
        buffer = allocMemory(bufferLength);
        assert(buffer != NULL);
        memset(buffer, 0, bufferLength);

        /* Loop over the fields, retrieve their RVA and assign it */
        currentField = xanList_first(return_value->fieldsLayout);
        assert(currentField != NULL);
        while (currentField != NULL) {
            /* Retrieve the field layout informations for the current field */
            currentFieldLayout = (ILFieldLayout *) xanList_getData(currentField);
            assert(currentFieldLayout != NULL);
            assert(currentFieldLayout->length > 0);
            /* Retrieve the current field metadata */
            current_field = currentFieldLayout->field;
            /* If the field has the RVA, retrieve the referenced value and initialize the field in the object being created */
            if (current_field->attributes->has_rva) {
                PDEBUG("LAYOUT_MANAGER: retrieve_static_type_layout: field %s has RVA: %d\n", current_field->getCompleteName(current_field), current_field->attributes->has_rva);
                /* If the field has the RVA, fetch it */
                current_field_rva = current_field->attributes->rva;
                PDEBUG("LAYOUT_MANAGER: the RVA for the field is %u\n", current_field_rva);
                /* Convert the RVA to a real address */
                t_binary_information *binary = (t_binary_information *) (type->getAssembly(type))->binary;
                entryPointAddress = convert_virtualaddress_to_realaddress(binary->sections, current_field_rva);
                PDEBUG("LAYOUT_MANAGER: the RVA has been converted to %u\n", entryPointAddress);
                /* Lock the binary					*/
                PLATFORM_lockMutex(&((binary->binary).mutex));
                /* Point the file in the entry point address		*/
                if (entryPointAddress < (*((binary->binary).reader))->offset) {
                    if (unroll_file(&(binary->binary)) != 0) {
                        print_err("LAYOUT_MANAGER: retrieve_static_type_layout: ERROR= Cannot unroll the file. ", 0);
                        abort();
                    }
                    //assert((((t_binary_information *) (type->getAssembly(type)->binary))->binary).offset == 0);
                    assert((*((((t_binary_information *) (type->getAssembly(type)->binary))->binary).reader))->offset == 0);
                }
                PDEBUG("LAYOUT_MANAGER: seeking within the file\n");
                if (seek_within_file(&(binary->binary), buffer, (*((binary->binary).reader))->offset, entryPointAddress) != 0) {
                    print_err("LAYOUT_MANAGER: retrieve_static_type_layout: ERROR= Cannot seek on the file. ", 0);
                    abort();
                }

                /* Seek to the begin of the metadata			*/
                PDEBUG("LAYOUT_MANAGER: seek to the begin of the metadata\n");
                if (bufferLength < get_metadata_start_point(&(binary->binary))) {
                    bufferLength = get_metadata_start_point(&(binary->binary)) + 1;
                    buffer = dynamicReallocFunction(buffer, bufferLength);
                    assert(buffer != NULL);
                    memset(buffer, 0, bufferLength);
                }
                if (get_metadata_start_point(&(binary->binary)) > 512) {
                    il_read(buffer, get_metadata_start_point(&(binary->binary)), &(binary->binary));
                }

                /* Using the offset and length infos for the field initialization into the default object */
                PDEBUG("LAYOUT_MANAGER: field length %d, offset %d\n", currentFieldLayout->length, currentFieldLayout->offset);

                /* Read from file		*/
                if (bufferLength < currentFieldLayout->length) {
                    bufferLength = currentFieldLayout->length;
                    buffer = dynamicReallocFunction(buffer, bufferLength);
                    assert(buffer != NULL);
                    memset(buffer, 0, bufferLength);
                }
                il_read(buffer, currentFieldLayout->length, &(binary->binary));
                PDEBUG("LAYOUT_MANAGER: read \"%s\" from file using the RVA\n", buffer);

                /* Unlock the binary			*/
                PLATFORM_unlockMutex(&((binary->binary).mutex));

                /* Initializing the object field with the value just read from the file using the RVA */
                assert(default_obj != NULL);
                assert(currentFieldLayout->offset < return_value->typeSize);
                assert((currentFieldLayout->offset + currentFieldLayout->length) <= (return_value->typeSize));
                memcpy((default_obj + (currentFieldLayout->offset)), buffer, currentFieldLayout->length);
                PDEBUG("LAYOUT_MANAGER: retrieve_static_type_layout: field %s initialized.\n", currentFieldLayout->field->getCompleteName(currentFieldLayout->field));
            } else {
                PDEBUG("LAYOUT_MANAGER: retrieve_static_type_layout: no RVA for the field is present\n");
            }

            /* Retrieve the next field */
            currentField = currentField->next;
        }

        /* Free the memory                      */
        freeMemory(buffer);
    }

    /* Insert the newly created default object into the ILLayout struct.
     */
    return_value->initialized_obj = default_obj;

    PDEBUG("LAYOUT_MANAGER: retrieve_static_type_layout: Exit\n");
    return return_value;
}

static inline ILLayout * retrieve_type_layout (ILLayout_manager *manager, TypeDescriptor *type) {
    ILLayout        *type_layout;

    /* Assertions.
     */
    assert(manager != NULL);
    assert(type != NULL);
    PDEBUG("LAYOUT_MANAGER: retrieve_type_layout: Start\n");

    /* Initialize the local variables.
     */
    type_layout = _alloc_ILLayout_infos(manager);
    assert(type_layout != NULL);

    /* Retrieve the type of the layout for this class.
     */
    type_layout->type_of_layout = type->attributes->layout;
    PDEBUG("LAYOUT_MANAGER: retrieve_type_layout:   TYPE_OF_LAYOUT == %d\n", type_layout->type_of_layout);
    type_layout->type = type;
    assert(type_layout->type != NULL);

    /* Retrieve the infos associated with the current type.
     */
    type_layout->packing_size = type->attributes->packing_size;

    /* Check if it is an interface.
     */
    if (type->attributes->isInterface) {
        return type_layout;
    }

    /* Layout fields.
     */
    internal_layoutFields(manager, type_layout);

    /* postconditions --> update the size value for the type layout */
    assert(type_layout->typeSize >= 0);
    type_layout->typeSize = (type_layout->typeSize >= type->attributes->class_size) ? type_layout->typeSize : type->attributes->class_size;

    /* Retrieve the supertype layout information.
     */
    type_layout->type->getSuperType(type_layout->type);

    /* Create the methods_table hash table.
     */
    type_layout->methods = type_layout->type->getVTable(type_layout->type, &(type_layout->virtualTableLength));
    assert(type_layout->methods != NULL);

    /* Print the layout information.
     */
#ifdef PRINTDEBUG
    debug_print_layout_informations(type_layout);
#endif

    PDEBUG("LAYOUT_MANAGER: retrieve_type_layout: Exit\n");
    return type_layout;
}

static inline ILLayoutStatic * _alloc_ILLayoutStatic (ILLayout_manager *manager) {
    ILLayoutStatic  *return_value;

    return_value = (ILLayoutStatic *) sharedAllocFunction(sizeof(ILLayoutStatic));
//#ifdef DEBUG
    if (return_value == NULL) {
        print_err("LAYOUT_MANAGER: _alloc_ILLayoutStatic : allocFunction error!", 0);
        abort();
    }
//#endif

    /* initialize the ILLayout structure */
    return_value->manager = manager;
    return_value->type = NULL;
    return_value->typeSize = 0;
    return_value->fieldsLayout = xanList_new(sharedAllocFunction, freeFunction, NULL);
    return_value->fetchStaticFieldLayout = fetchStaticFieldLayout;

    /* verify the postconditions */
    assert(return_value->fieldsLayout != NULL);
    assert(return_value->fetchStaticFieldLayout != NULL);

    /* return the initialized structure */
    return return_value;
}

static inline ILLayout * _alloc_ILLayout_infos (ILLayout_manager *manager) {
    ILLayout        *return_value;

    return_value = (ILLayout *) sharedAllocFunction(sizeof(ILLayout));

    /* initialize the ILLayout structure */
    pthread_mutexattr_t mutex_attr;
    PLATFORM_initMutexAttr(&mutex_attr);
    PLATFORM_setMutexAttr_type(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);
    MEMORY_shareMutex(&mutex_attr);
    PLATFORM_initMutex(&(return_value->mutex), &mutex_attr);
    return_value->manager = manager;
    return_value->jit_type = MEMORY_obtainPrivateAddress();
    assert(*(return_value->jit_type) == NULL);
    return_value->type = NULL;
    return_value->typeSize = 0;
    return_value->packing_size = 0;
    return_value->virtualTableLength = 0;
    return_value->virtualTable = MEMORY_obtainPrivateAddress();
    assert(*(return_value->virtualTable) == NULL);
    return_value->interfaceMethodTable = MEMORY_obtainPrivateAddress();
    assert(*(return_value->interfaceMethodTable) == NULL);
    return_value->methods = NULL;
    return_value->type_of_layout = -1;
    return_value->fieldsLayout = NULL;
    return_value->fetchFieldLayout = fetchFieldLayout;

    /* return the initialized structure */
    return return_value;
}

static inline ILFieldLayout * _alloc_ILFieldLayout_infos () {
    ILFieldLayout   *return_value;

    return_value = (ILFieldLayout *) sharedAllocFunction(sizeof(ILFieldLayout));

    /* initialize the ILLayout structure */
    return_value->field = NULL;
    return_value->layout = NULL;
    return_value->IRType = NOPARAM;
    return_value->offset = 0;
    return_value->length = 0;

    /* return the initialized structure */
    return return_value;
}

static inline ILFieldLayout * _get_Field_Layout_Infos (FieldDescriptor *field) {
    ILFieldLayout                   *field_layout;

    /* assertions */
    assert(field != NULL);

    PDEBUG("LAYOUT_MANAGER : _get_Field_Layout_Infos : START \n");

    /* initialize the ILFieldLayout structure */
    field_layout = _alloc_ILFieldLayout_infos();
    assert(field_layout != NULL);

    /* initialize the metadata informations associated with the field layout */
    field_layout->field = field;
    assert(field_layout->field != NULL);


    /* retireve the IR-type of the field */
    TypeDescriptor *field_type = field->getType(field);
    field_layout->IRType = field_type->getIRType(field_type);

    if (field->attributes->has_offset) {
        PDEBUG("LAYOUT_MANAGER : ANDY _get_Field_Layout_Infos : FIELDLAYOUT INFOS ARE ASSOCIATED WITH THIS FIELD!  \n");
        field_layout->offset = field->attributes->offset;
        PDEBUG("LAYOUT_MANAGER : offset: %d\n", field_layout->offset);
        assert(field_layout->offset >= 0);
    } else {
        PDEBUG("LAYOUT_MANAGER : _get_Field_Layout_Infos : NO FIELDLAYOUT INFOS ASSOCIATED WITH THIS FIELD \n");
        PDEBUG("LAYOUT_MANAGER : _get_Field_Layout_Infos : EXIT \n");
    }

    return field_layout;
}

static inline MethodImplementations *getMethodImplementations (ILLayout_manager *manager, MethodDescriptor *methodID) {
    MethodDescriptor *methodToFind;

    if (!methodID->isGeneric(methodID) || methodID->isGenericMethodDefinition(methodID)) {
        methodToFind = methodID;
    } else {
        methodToFind = methodID->getGenericDefinition(methodID);
    }
    MethodImplementations *infos = xanHashTable_syncLookup(manager->methodImplementationInfos, methodToFind);
    return infos;
}

static inline void internal_layoutInitTables (ILLayout_manager *manager, ILLayout *layout) {
    XanHashTableItem	*item;
    VTableSlot		*vmethod;
    Method			method;
    void			**virtualTable;
    Method			*virtualTableMethods;
    IMTElem			*interfaceMethodTable;

    /* Assertions.
     */
    assert(manager != NULL);
    assert(layout != NULL);
    PDEBUG("LAYOUT_MANAGER: internal_layoutInitTables: Start\n");
    PDEBUG("LAYOUT_MANAGER: internal_layoutInitTables:  Type %s\n", layout->type->getCompleteName(layout->type));

    /* Check whether we have already layout the tables.
     */
    if (	((*(layout->virtualTable)) != NULL)		&&
            ((*(layout->interfaceMethodTable)) != NULL)	) {

        /* Tables have been already created.
         */
        PDEBUG("LAYOUT_MANAGER: internal_layoutInitTables: Exit\n");
        return ;
    }

    /* Check if we need to post-pone the creation of the virtual table.
     */
    if (manager->cacheVirtualMethodRequests) {
        PDEBUG("LAYOUT_MANAGER: internal_layoutInitTables:  Cache virtual method requests\n");
        xanList_lock(manager->cachedVirtualMethodRequests);
        if (xanList_find(manager->cachedVirtualMethodRequests, layout) == NULL) {
            xanList_append(manager->cachedVirtualMethodRequests, layout);
        }
        xanList_unlock(manager->cachedVirtualMethodRequests);
        return ;
    }

    /* Allocate and initialize the virtual table.
     */
    *(layout->virtualTable) 	= allocFunction(getIRSize(IRMPOINTER) * layout->virtualTableLength);
    *(layout->interfaceMethodTable) = allocMemory(sizeof(IMTElem) * IMT_SIZE);
    layout->virtualTableMethods 	= allocFunction(sizeof(Method) * layout->virtualTableLength);

    /* Cache some pointers.
     */
    virtualTable 			= *(layout->virtualTable);
    interfaceMethodTable 		= *(layout->interfaceMethodTable);
    virtualTableMethods 		= (layout->virtualTableMethods);

    /* Fill up the virtual tables.
     */
    item 				= xanHashTable_first(layout->methods);
    while (item != NULL) {
        MethodImplementations	*methodImplementors;

        /* Fetch the virtual table slot.
         */
        vmethod 	= (VTableSlot *) item->element;
        assert(vmethod != NULL);
        assert(vmethod->body != NULL);
        assert(vmethod->index < layout->virtualTableLength);

        /* Fetch possible implementations reachable from the current table slot.
         */
        xanHashTable_lock(manager->methodImplementationInfos);
        methodImplementors	= (MethodImplementations *) xanHashTable_lookup(manager->methodImplementationInfos, vmethod->declaration);
        if (methodImplementors == NULL) {
            methodImplementors 		= sharedAllocFunction(sizeof(MethodImplementations));
            methodImplementors->implementor	= xanList_new(sharedAllocFunction, freeFunction, NULL);
            methodImplementors->closure 	= xanList_new(sharedAllocFunction, freeFunction, NULL);
            xanHashTable_insert(manager->methodImplementationInfos, vmethod->declaration, methodImplementors);
        }
        assert(xanHashTable_lookup(manager->methodImplementationInfos, vmethod->declaration) == methodImplementors);

        /* Register class as implementor of the current virtual method.
         */
        xanList_append(methodImplementors->implementor, layout);
        xanHashTable_unlock(manager->methodImplementationInfos);

        /* Fill up the current slot of both virtual tables.
         */
        if (vmethod->declaration->attributes->param_count == 0) {

            /* Fill up the virtual table slot.
             */
            if (virtualTable[vmethod->index] == NULL) {

                /* Fetch the method that will be executed when the current slot will be used.
                 */
                PDEBUG("LAYOUT_MANAGER: internal_layoutInitTables:      Method	= %s	Index	= %d\n", vmethod->body->getCompleteName(vmethod->body), vmethod->index);
                method					= cliManager->fetchOrCreateMethod(vmethod->body);
                assert(method != NULL);

                /* Fill up the virtual table.
                 */
                virtualTable[vmethod->index] 		= internal_getFunctionPointer(vmethod->body);
                assert(virtualTable[vmethod->index] != NULL);
                virtualTableMethods[vmethod->index] 	= method;

                /* Check if current method is Finalize and cache its address.
                 */
                if (method->isFinalizer(method)) {
                    layout->finalize = method;
                }
            }

            /* Fill up the IMT slot.
             */
            if (vmethod->declaration->owner->attributes->isInterface) {
                void	*pointer;

                /* Prepare IMT.
                 */
                pointer	= virtualTable[vmethod->index];
                insertIntoIMT(interfaceMethodTable, vmethod->declaration, pointer);
            }

        } else if (vmethod->declaration->attributes->param_count > 0) {
            xanList_lock(methodImplementors->closure);
            XanListItem *item = xanList_first(methodImplementors->closure);
            while (item != NULL) {
                GenericContainer *slot = (GenericContainer *) item->data;
                MethodDescriptor *methodID = vmethod->declaration->getInstance(vmethod->declaration, slot);

                /* Prepare IMT.
                 */
                void *pointer = getGenericFunctionPointer(vmethod->body, slot);
                insertIntoIMT(interfaceMethodTable, methodID, pointer);
                item = item->next;
            }
            xanList_unlock(methodImplementors->closure);
        }

        /* Fetch the next element.
         */
        item 	= xanHashTable_next(layout->methods, item);
    }

    PDEBUG("LAYOUT_MANAGER: internal_layoutInitTables: Exit\n");
    return;
}

static inline JITINT32 getIndexOfMethodDescriptor (MethodDescriptor *method) {
    return simpleHash((void *) method) % IMT_SIZE;
}

static inline void insertIntoIMT (IMTElem *bucket, MethodDescriptor *method, void *pointer) {
    JITUINT32 position = getIndexOfMethodDescriptor(method);

    bucket = &(bucket[position]);
    if (bucket->method == NULL) {
        bucket->method = method;
        bucket->pointer = pointer;
    } else {
        IMTElem *item = allocFunction(sizeof(IMTElem));
        item->next = NULL;
        item->method = method;
        item->pointer = pointer;
        while (bucket->next != NULL) {
            bucket = bucket->next;
        }
        bucket->next = item;
    }
}

static inline void *getFromIMT (IMTElem *bucket, MethodDescriptor *method) {
    void *result = NULL;
    JITUINT32 position = getIndexOfMethodDescriptor(method);

    bucket = &(bucket[position]);
    if (bucket->method == method) {
        result = bucket->pointer;
    } else {
        bucket = bucket->next;
        while (bucket != NULL) {
            if (bucket->method == method) {
                result = bucket->pointer;
                break;
            }
            bucket = bucket->next;
        }
    }
    return result;
}

static inline void * internal_getFunctionPointer (MethodDescriptor *cil_method) {
    void *result;

    Method method = cliManager->fetchOrCreateMethod(cil_method);
    assert(method != NULL);

    result = method->getFunctionPointer(method);
    return result;
}

static inline void * getGenericFunctionPointer (MethodDescriptor *cil_method, GenericContainer *container) {
    MethodDescriptor *methodInst = cil_method->getInstance(cil_method, container);

    Method method = cliManager->fetchOrCreateMethod(methodInst);

    return method->getFunctionPointer(method);
}

static inline JITUINT32 internal_perform_fields_layout (XanList *fields, JITUINT32 startingOffset) {
    JITUINT32 	upstair_fields_size;
    JITUINT32 	downstair_fields_size;
    JITUINT32 	current_uf_offset;
    JITUINT32 	current_df_offset;
    JITUINT32 	padding;
    JITUINT32	pointer_bytes;
    XanListItem     *currentField;
    ILFieldLayout   *currentFieldLayout;
#ifdef DEBUG
    JITUINT32 	basic_alignment;
#endif

    /* Assertions 						*/
    assert(fields != NULL);

    /* Initialize the variables 				*/
    upstair_fields_size 	= 0;
    downstair_fields_size 	= 0;
    current_uf_offset 	= 0;
    current_df_offset 	= 0;
    currentFieldLayout 	= NULL;
    currentField 		= NULL;

    /* Fetch the basic alignment (one word)			*/
#ifdef DEBUG
    basic_alignment 	= getIRSize(IRMPOINTER);
#endif

    /* Set the number of bytes per pointer			*/
    pointer_bytes		= 8;

    /* define the size of each portion of fields in memory */
    /* retrieve the first field from the list */
    currentField = xanList_first(fields);
    while (currentField != NULL) {

        /* Retrieve the field layout informations for the current field */
        currentFieldLayout = (ILFieldLayout *) currentField->data;
        assert(currentFieldLayout != NULL);
        assert(currentFieldLayout->offset == 0);

        /* Check if the field should be put above or below	*/
        if (    (currentFieldLayout->IRType == IRMPOINTER)	||
                (currentFieldLayout->IRType == IRTPOINTER) 	||
                (currentFieldLayout->IRType == IROBJECT)   	||
                (currentFieldLayout->IRType == IRNINT) 		) {

            /* Update the upstair fields size			*/
            upstair_fields_size 	+= currentFieldLayout->length;
            assert(currentFieldLayout->length == basic_alignment);

        } else {
            size_t	fieldSize;

            /* Compute the padding					*/
            if (currentFieldLayout->IRType == IRVALUETYPE) {

                /* Grant that every ValueType is alligned to memory addresses that	*
                 * are multiples of ``sizeof(JITNUINT)``.				*
                 * Fetch the size of pointers						*/
                fieldSize		= getIRSize(IRMPOINTER);

            } else {
                fieldSize		= getIRSize(currentFieldLayout->IRType);
            }
            padding 	= (downstair_fields_size % fieldSize) ? fieldSize - (downstair_fields_size % fieldSize ) : 0;
            assert((downstair_fields_size + padding) % fieldSize == 0);
            assert(padding >= 0);
            assert(padding < fieldSize);

            /* Update the downstair fields size			*/
            downstair_fields_size 	+= (currentFieldLayout->length + padding);
        }

        /* Fetch the next element				*/
        currentField = currentField->next;
    }

    /* Adjust the upstair field size			*/
    padding 		= (upstair_fields_size % pointer_bytes) ? (pointer_bytes - (upstair_fields_size % pointer_bytes)) : 0;
    upstair_fields_size 	+= padding;

    /* Update the current_field_offsets informations 	*/
    current_uf_offset = upstair_fields_size + startingOffset - padding;
    current_df_offset = upstair_fields_size + startingOffset;
    assert(upstair_fields_size >= 0);

    /* Define the offset of each field in memory 		*/
    currentField = xanList_first(fields);
    while (currentField != NULL) {

        /* Retrieve the field layout informations for the current field */
        currentFieldLayout 	= (ILFieldLayout *) currentField->data;
        assert(currentFieldLayout != NULL);

        if (    (currentFieldLayout->IRType == IRMPOINTER)	||
                (currentFieldLayout->IRType == IRTPOINTER)	||
                (currentFieldLayout->IRType == IROBJECT)        ||
                (currentFieldLayout->IRType == IRNINT) 		) {

            current_uf_offset 	-= currentFieldLayout->length;
            assert(current_uf_offset >= startingOffset);

            /* Set the offset of the field					*/
            currentFieldLayout->offset = current_uf_offset;

        } else {
            size_t	fieldSize;

            /* Compute the padding			*/
            if (currentFieldLayout->IRType == IRVALUETYPE) {

                /* Grant that every ValueType is alligned to memory addresses that	*
                 * are multiples of ``sizeof(JITNUINT)``.				*
                 * Fetch the size of pointers						*/
                fieldSize		= getIRSize(IRMPOINTER);

            } else {
                fieldSize		= getIRSize(currentFieldLayout->IRType);
            }
            padding 	= (current_df_offset % fieldSize) ? fieldSize - (current_df_offset % fieldSize ) : 0;
            //assert((downstair_fields_size + padding) % fieldSize == 0); FIXME TOCHECK
            assert(padding >= 0);
            assert(padding < fieldSize);

            /* Set the offset of the field					*/
            currentFieldLayout->offset 	= current_df_offset + padding;

            /* Update the downstair field size				*/
            current_df_offset 		+= (currentFieldLayout->length + padding);
            assert(current_df_offset <= (upstair_fields_size + downstair_fields_size + startingOffset));
        }
        //assert(currentFieldLayout->offset % currentFieldLayout->length == 0); FIXME

        /* Fetch the next element			*/
        currentField	= currentField->next;
    }
    assert(current_uf_offset == startingOffset);
    assert(current_df_offset == (upstair_fields_size + downstair_fields_size + startingOffset));

    /* Return the dimension in bytes of an instance of the current type in memory */
    return upstair_fields_size + downstair_fields_size + startingOffset;
}

static inline void internal_dumpStaticLayout (ILLayoutStatic *layout){
    XanListItem     *currentField;

    /* Consider all static fields.
     */
    fprintf(stderr, "Layout of \"%s\"\n", layout->type->getCompleteName(layout->type));
    currentField = xanList_first(layout->fieldsLayout);
    while (currentField != NULL) {
        ILFieldLayout   *currentFieldLayout;

        /* Retrieve the field layout information for the current field.
         */
        currentFieldLayout = (ILFieldLayout *) currentField->data;
        assert(currentFieldLayout != NULL);
        assert(currentFieldLayout->length > 0);

        fprintf(stderr, "   Field \"%s\": %u\n", currentFieldLayout->field->name, currentFieldLayout->offset);

        /* Retrieve the next field.
         */
        currentField = currentField->next;
    }

    return ;
}
