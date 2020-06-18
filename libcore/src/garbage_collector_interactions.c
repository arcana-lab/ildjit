/*
 * Copyright (C) 2006  Campanoni Simone, Di Biagio Andrea
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
#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ir_language.h>
#include <ir_method.h>
#include <jitsystem.h>
#include <compiler_memory_manager.h>
#include <gc_root_sets.h>
#include <iljit-utils.h>
#include <platform_API.h>
#include <layout_manager.h>

// My headers
#include <system_manager.h>
#include <iljit.h>
#include <lib_lock.h>
// End

/* Methods casts, used during functions link setup for convenience */
#define GCI_COLLECT							\
	(void (*)(void))				      \
	gci_collect
#define GCI_ISSUBTYPE						\
	(JITBOOLEAN (*)(void*, TypeDescriptor *))		    \
	gci_isSubtype
#define GCI_FETCHOVERSIZEOFFSET						\
	(JITINT32 (*)(void*))					  \
	gci_fetchOverSizeOffset
#define GCI_UNSETSTATICFLAG						\
	(void (*)(void*))				      \
	gci_unsetStaticFlag
#define GCI_GETTYPE							\
	(TypeDescriptor *       (*)(void*))					\
	gci_getType
#define GCI_GETTYPESIZE							\
	(JITINT32 (*)(void*))					  \
	gci_getTypeSize
#define GCI_ALLOCPERMANENTOBJECT					\
	(void*          (*)(TypeDescriptor *, JITUINT32))		\
	gci_allocPermanentObject
#define GCI_ALLOCOBJECT							\
	(void*          (*)(TypeDescriptor *, JITUINT32))		\
	gci_allocObject
#define GCI_FREEOBJECT							\
	(void (*)(void *))				       \
	gci_freeObject
#define GCI_ALLOCSTATICOBJECT						\
	(void*          (*)(TypeDescriptor *, JITUINT32))		\
	gci_allocStaticObject
#define GCI_CREATEARRAY							\
	(void*          (*)(ArrayDescriptor *, JITINT32))	\
	gci_createArray
#define GCI_ALLOCARRAY							\
	(void*          (*)(TypeDescriptor *, JITINT32, JITINT32))	\
	gci_allocArray
#define GCI_FETCHFIELDOFFSET						\
	(JITINT32 (*)(FieldDescriptor *))			  \
	gci_fetchFieldOffset
#define GCI_FETCHSTATICFIELDOFFSET					\
	(JITINT32 (*)(FieldDescriptor *))			  \
	gci_fetchStaticFieldOffset
#define GCI_FETCHVTABLEOFFSET						\
	(JITINT32 (*)(void))					  \
	gci_fetchVTableOffset
#define GCI_FETCHIMTOFFSET						\
	(JITINT32 (*)(void))					  \
	gci_fetchIMTOffset
#define GCI_FETCHTYPEDESCRIPTOROFFSET						\
	(JITINT32 (*)(void))					  \
	gci_fetchTypeDescriptorOffset
#define GCI_GETINDEXOFVTABLE						\
	(VCallPath (*)(MethodDescriptor *))				   \
	gci_getIndexOfVTable
#define GCI_GETARRAYLENGTH						\
	(JITUINT32 (*)(void*))					   \
	gci_getArrayLength
#define GCI_GETARRAYRANK						\
	(JITUINT16 (*)(void*))					   \
	gci_getArrayRank
#define GCI_GETARRAYLENGTHOFFSET					\
	(JITINT32 (*)(void))					  \
	gci_getArrayLengthOffset
#define GCI_GETARRAYSLOTIRTYPE						\
	(JITUINT32 (*)(void*))					   \
	gci_getArraySlotIRType
#define GCI_GETVTABLE							\
	(void**         (*)(void*))					\
	gci_getVTable
#define GCI_SETSTATICFLAG						\
	(void (*)(void*))				      \
	gci_setStaticFlag
#define GCI_SETPERMANENTFLAG						\
	(void (*)(void*))				      \
	gci_setPermanentFlag
#define GCI_SETFINALIZEDFLAG						\
	(void (*)(void*))				      \
	gci_setFinalizedFlag
#define GCI_UNSETPERMANENTFLAG						\
	(void (*)(void*))				      \
	gci_unsetPermanentFlag
#define GCI_ADDROOTSET							\
	(void (*)(void***, JITUINT32))			      \
	gci_addRootSet
#define GCI_POPLASTROOTSET						\
	(void (*)(void))				      \
	gci_popLastRootSet
#define GCI_THREADCREATE						\
	(JITINT32 (*)(pthread_t*,				  \
		      pthread_attr_t*,				  \
		      void* (*)(void*),				  \
		      void*))					  \
	gci_threadCreate
#define GCI_THREADEXIT							\
	(void (*)(pthread_t))						\
	gci_threadExit
#define GCI_LOCKOBJECT							\
	(JITBOOLEAN (*)(void*, JITINT32))			    \
	gci_lockObject
#define GCI_UNLOCKOBJECT						\
	(void (*)(void*))				      \
	gci_unlockObject
#define GCI_WAITFORPULSE						\
	(JITBOOLEAN (*)(void*, JITINT32))			    \
	gci_waitForPulse
#define GCI_SIGNALPULSE							\
	(void (*)(void*))				      \
	gci_signalPulse
#define GCI_SIGNALPULSEALL						\
	(void (*)(void*))				      \
	gci_signalPulseAll
#define SIZEOBJECT							\
	(JITNUINT (*)(void*))					  \
	sizeObject

/* Retrive object header. The argument is an object address in IR machine */
#define GET_OBJECT_HEADER(object) \
	((t_objectHeader*) ((JITNUINT) object - HEADER_FIXED_SIZE))

/* Retrive array header. The argument is an object address in IR machine */
#define GET_ARRAY_HEADER(array)	\
	((t_arrayHeader*) ((JITNUINT) array - HEADER_FIXED_SIZE))

/* Check if given object header stores given flag */
#define HAS_FLAG(header, flag) \
	((header->flags & flag) == flag)

/* Check if given object header stores IS_FINALIZED flag */
#define HAS_FINALIZED_FLAG(header) \
	HAS_FLAG(header, IS_FINALIZED)

/* Check if given object header stores IS_COLLECTABLE flag */
#define HAS_COLLECTABLE_FLAG(header) \
	HAS_FLAG(header, IS_COLLECTABLE)

/* Check if given object header stores IS_STATIC flag */
#define HAS_STATIC_FLAG(header)	\
	HAS_FLAG(header, IS_STATIC)

/* Check if given object header stores IS_ARRAY flag */
#define HAS_ARRAY_FLAG(header) \
	HAS_FLAG(header, IS_ARRAY)

/* Check if given header stores IS_VALUETYPE flag */
#define HAS_VALUETYPE_FLAG(header) \
	HAS_FLAG(header, IS_VALUETYPE)

/* Check if given garbage collector plugin needs root set support */
#define GC_NEEDS_ROOTSET(garbageCollector) \
	(garbageCollector->getSupport() & ILDJIT_GCSUPPORT_ROOTSET)

/*
 * Return the value of the field with given layout (fieldLayout) in object. You
 * can give an additonal field offset (additionalOffset) from object base
 * address. The type parameter is field IR machine type
 */
#define GET_FIELD_VALUE(type, object, fieldLayout, additonalOffset) \
	(*((type*) object + fieldLayout->offset + additionalOffset))

/* Some flags that you can store on objects/arrays headers */
#define IS_ARRAY                1
#define IS_COLLECTABLE          2
#define IS_VALUETYPE            4
#define IS_STATIC               8
#define IS_FINALIZED            16

/* Most programs today are single thread */
#define DEFAULT_THREAD_NUMBER   1

/* Object header */
typedef struct {
    void*                   virtualTable;           /**< Virtual table pointer              */
    IMTElem*                interfaceMethodTable;   /**< Interface Method Table             */
    TypeDescriptor          *type;                  /**< Real Object Type pointer           */
    JITUINT8 flags;                                 /**< Object flags (e.g. collectable)    */
    JITUINT32 objectSize;                           /**< Object overall size                */
    ILJITMonitor monitor;                           /**< Object monitor                     */
} t_objectHeader;

/* Array header */
typedef struct {
    void                    *virtualTable;          /**< Virtual table pointer              */
    IMTElem                 *interfaceMethodTable;  /**< Interface Method Table             */
    TypeDescriptor          *type;                  /**< Real Object Type pointer           */
    JITUINT8 flags;                                 /**< Array flags (e.g. collectable)     */
    JITUINT32 objectSize;                           /**< Array overall size                 */
    ILJITMonitor monitor;                           /**< Object monitor                     */

    /*
     * Maximum size of arrays lenght (maximum number of columns). Here is a
     * little schema for a bidimensional array:
     *
     * 1 2 3 # #
     * 4 5 # # #
     * 6 7 8 0 0
     *
     * The matrix rank is 3. The matrix dimension is max{3, 2, 5} = 5. Sharp (#)
     * means wasted space.
     */
    JITUINT32 dimSize;
} t_arrayHeader;

extern t_system *ildjitSystem;
static CLIManager_t *cliManager;
static IRVM_t *IRVM;
static t_running_garbage_collector *garbageCollector;

/* Each thread stored here its root set */
static XanHashTable*    rootSets;

/*
 * A root set view for the garbage collector. This variable is not NULL only
 * when the garbage collector is reading/writing the root set
 */
static XanList*         rootSetList;

/*
 * Add given root set to global one. Abort if current garbage collector don't
 * need root set support
 */
static void             gci_addRootSet (void*** rootSet, JITUINT32 size);

/* Remove last added root set from root set stack */
static void             gci_popLastRootSet (void);

/*
 * Create a new thread. Currently this method is only a wrapper around POSIX
 * phtread_create function. See pthread_create(3) for more infos
 */
static JITINT32         gci_threadCreate (pthread_t* thread,
        pthread_attr_t* attr,
        void* (*startRoutine)(void*),
        void* arg);

static void             gci_collect (void);

/* Check if object class is subtype of given type */
static JITBOOLEAN       gci_isSubtype (void* object, TypeDescriptor *type);

/* Get the offset of object oversize */
static JITINT32         gci_fetchOverSizeOffset (void* object);

/* Unset STATIC flag from given object */
static void             gci_unsetStaticFlag (void* object);

/* Get given object IL type */
static inline TypeDescriptor * gci_getType (void* object);

/* Get given object IL type size*/
static JITINT32         gci_getTypeSize (void * object);


/* Lock given object */
static JITBOOLEAN       gci_lockObject (void* object, JITINT32 timeout);

/* Unlock given object */
static void             gci_unlockObject (void* object);

/* Wait for a pulse signal on object */
static JITBOOLEAN       gci_waitForPulse (void* object, JITINT32 timeout);

/* Send a pulse signal to object */
static void             gci_signalPulse (void* object);

/* Send a pulse all signal to object */
static void             gci_signalPulseAll (void* object);

/*
 * Alloc a new permanent object of given class (type) with the specified
 * oversize (overSize). Abort if object can't be created (e.g. no avaible
 * memory)
 */
static void*            gci_allocPermanentObject (TypeDescriptor *type, JITUINT32 overSize);

/*
 * Alloc a new object of given class (type) with the specified oversize.
 * Abort if object can't be created
 */
static void*            gci_allocObject (TypeDescriptor *type, JITUINT32 overSize);

static void             gci_freeObject (void *obj);
static inline JITINT32 internal_are_equal_threads (pthread_t *k1, pthread_t *k2);

/*
 * Alloc a new static object of given class (type) with the specified
 * oversize (overSize). Abort if object can't be created
 */
static void*            gci_allocStaticObject (TypeDescriptor *type, JITUINT32 overSize);

/*
 * Alloc a new array. Each array element has class classID. The array
 * dimensions are rank and size. Abort if array can't be allocated
 */
static void*            gci_allocArray (TypeDescriptor *type, JITINT32 rank, JITINT32 size);

/*
 * Get the offset of the given field (fieldID) from the start address of its
 * owner
 */
static JITINT32         gci_fetchFieldOffset (FieldDescriptor *fieldID);

/*
 * Get the offset of the given static field (fieldID) from the start address of
 * its owner
 */
static JITINT32         gci_fetchStaticFieldOffset (FieldDescriptor *fieldID);

/* Get virtual table offset */
static JITINT32         gci_fetchVTableOffset (void);

/* Get interface method table offset */
static JITINT32         gci_fetchIMTOffset (void);

/* Get interface type descriptor offset */
static JITINT32         gci_fetchTypeDescriptorOffset (void);

/*
 * Get the index in the virtual table of given object (binaryInfo) of the given
 * method (methodID)
 */
static VCallPath        gci_getIndexOfVTable (MethodDescriptor *methodID);

/* Get given array length */
static JITUINT32        gci_getArrayLength (void* array);

/* Get given array rank */
static JITUINT16        gci_getArrayRank (void* array);

/* Get array header dimSize offset from a generic IR machine array address */
static JITINT32         gci_getArrayLengthOffset (void);

/* Get given array element IR type */
static JITUINT32        gci_getArraySlotIRType (void* array);

/* Get a pointer to given object virtual table */
static void**           gci_getVTable (void* object);

/* Check if given object is static */
static JITBOOLEAN       gci_isStatic (void* object);

/* Check if given object is permanent */
static JITBOOLEAN       gci_isPermanent (void* object);

/* Set IS_STATIC flags on given object */
static void             gci_setStaticFlag (void* object);

/*
 * Set permanent flag on given object. Currently this function behave like
 * setStaticFlag
 */
static void             gci_setPermanentFlag (void* object);

static void             gci_setFinalizedFlag (void* object);

/*
 * Unset permanent flag on given object. Currently this function behave like
 * unsetStaticFlag
 */
static void             gci_unsetPermanentFlag (void* object);

/* Private function prototypes */

/*
 * Build a new instance of given type, with specified oversize (overSize) and
 * flags (isCollectable, isStatic). If allocation fails an OutOfMemoryException
 * is raised and the program is aborted. On successfull allocations return a
 * pointer to new allocated object. The returned object pointer is an IR
 * machine address (without object header)
 */
static void*            gci_createInstance (TypeDescriptor* type, JITUINT32 overSize, JITBOOLEAN isCollectable, JITBOOLEAN isStatic);

/*
 * Build a new array that stores elements of given type. The array size depends
 * on given type size, rank and size parameters. Mark array collectable if
 * isCollectable is enabled. Return an IR machine array address on success. On
 * failure an OutOfMemoryException is raised and the program is aborted
 */
static void*            gci_createArray (ArrayDescriptor* type, JITUINT32 size);

/*
 * Fill referenceList with object references. Given object can be also an
 * array
 */
static void             gci_getReferences (void* object,
        XanList* referenceList,
        XanList* fieldsLayout,
        JITBOOLEAN isArray,
        JITBOOLEAN isValueType);

/* Run some cleanup tasks when a given thread die */
static void             gci_threadExit (pthread_t threadID);

size_t GC_getHeaderSize () {
    return sizeof(t_arrayHeader);
}

TypeDescriptor * GC_getObjectType (void* object) {
    return gci_getType(object);
}

void * GC_allocObject (TypeDescriptor *type, JITUINT32 overSize) {
    return gci_allocObject(type, overSize);
}

void * GC_allocArray (ArrayDescriptor *arrayType, JITINT32 size) {
    return gci_createArray(arrayType, size);
}

void * GC_allocUntracedObject (JITUINT32 overallSize, TypeDescriptor *ilType, void *vTable, IMTElem *IMT) {
    t_objectHeader	*header;
    void		*object;

    /* Allocate the space.
     */
    object 				= calloc(1, overallSize);
    if (object == NULL) {
        IRVM_throwBuiltinException(&(ildjitSystem->IRVM), OUT_OF_MEMORY_EXCEPTION);
        abort();
    }

    /* Set the header.
     */
    header 				= (t_objectHeader*) object;
    header->type 			= ilType;
    header->objectSize 		= overallSize;

    /* Set the virtual tables.
     */
    header->virtualTable		= vTable;
    header->interfaceMethodTable	= IMT;

    /* Initialize object lock and condition variable.
     */
    ILJITMonitorInit(&header->monitor);

    /* Normalize address to IR machine format (remove header).
     */
    object 			+= HEADER_FIXED_SIZE;

    return object;
}

void * GC_allocUntracedArray (JITUINT32 overallSize, TypeDescriptor *ilType, void *vTable, IMTElem *IMT, JITUINT32 length, JITINT32 isValueType) {
    t_arrayHeader	*header;
    void		*object;

    /* Assertions.
     */
    assert(ilType != NULL);

    /* Allocate the space.
     */
    object 				= calloc(1, overallSize);
    if (object == NULL) {
        IRVM_throwBuiltinException(&(ildjitSystem->IRVM), OUT_OF_MEMORY_EXCEPTION);
        abort();
    }

    /* Set the header.
     */
    header 				= (t_arrayHeader*) object;
    header->flags 			|= IS_ARRAY;
    if (isValueType) {
        header->flags 		|= IS_VALUETYPE;
    }
    header->type 			= ilType;
    header->objectSize 		= overallSize;
    header->dimSize 		= length;

    /* Set the virtual tables.
     */
    header->virtualTable		= vTable;
    header->interfaceMethodTable	= IMT;

    /* Initialize object lock and condition variable.
     */
    ILJITMonitorInit(&header->monitor);

    /* Normalize address to IR machine format (remove header).
     */
    object 			+= HEADER_FIXED_SIZE;

    return object;
}

void GC_freeObject (void *obj) {
    gci_freeObject(obj);

    return ;
}

/*
 * Fill referenceList with object references. Given object can be also an
 * array
 */
static void             gci_getReferences (void* object,
        XanList* referenceList,
        XanList* fieldsLayout,
        JITBOOLEAN isArray,
        JITBOOLEAN isValueType) {
    /* Current array element, if given object is an array */
    JITUINT32 count;

    /* Given object header, if the object is an array */
    t_arrayHeader*  arrayHeader;

    /* Object layout infos */
    ILLayout*       layoutInfos;

    /* A IR machine type */
    JITUINT32 irType;

    /* Given object field container */
    XanListItem*    currentField;

    /* Given object field layout */
    ILFieldLayout*  currentFieldLayout;

    /* Address of a field of given object */
    void*           fieldAddress;

    /* Assertions */
    assert(object != NULL);
    assert(referenceList != NULL);
    assert(fieldsLayout != NULL);
    PDEBUG("GC: getReferences: Start\n");

    /* Given object is an array */
    if (isArray) {
        /* Retrieve the array header */
        arrayHeader = GET_ARRAY_HEADER(object);

        /* Retrieve array layout info */
        layoutInfos = (cliManager->layoutManager).layoutType(&(cliManager->layoutManager), arrayHeader->type);
        assert(layoutInfos != NULL);

        /* Add all array elements to references objects */
        for (count = 0; count < arrayHeader->dimSize; count++) {
            /* Array stores basic values */
            if (isValueType) {
                PDEBUG("current_field_layout->IRType == %d ", layoutInfos->type->getIRType(layoutInfos->type));
                /* The stored values are references? */
                irType = layoutInfos->type->getIRType(layoutInfos->type);
                if (irType == IRVALUETYPE) {
                    fieldAddress = object + count * layoutInfos->typeSize;

                    gci_getReferences( fieldAddress, referenceList, layoutInfos->fieldsLayout, JITFALSE, JITTRUE);
                }
            }
            /* Array dont't stores basic values */
            else {
                /* The stored values are objects? */
                irType = layoutInfos->type->getIRType(layoutInfos->type);
                if (irType == IROBJECT) {
                    fieldAddress = object + count * getIRSize(IRMPOINTER);

                    gci_getReferences( fieldAddress, referenceList, layoutInfos->fieldsLayout, JITFALSE, JITFALSE);
                }
            }
        }
    }
    /* We are examing a real object */
    else {
        /* Visit each object field */
        currentField = xanList_first(fieldsLayout);
        while (currentField != NULL) {
            /* Retrieve the current field layout informations */
            currentFieldLayout = currentField->data;

            /* Current field stores a basic value */
            if (currentFieldLayout->IRType == IRVALUETYPE) {
                /* Retrieve the fields layout for the valuetype */
                layoutInfos = (ILLayout*) currentFieldLayout->layout;

                /* Get field address */
                fieldAddress = object + currentFieldLayout->offset;

                /* Find references on current field */
                gci_getReferences(fieldAddress, referenceList, layoutInfos->fieldsLayout, JITFALSE, JITTRUE);
            }
            /* Current field stores a reference */
            else if (currentFieldLayout->IRType == IROBJECT) {
                fieldAddress = object + currentFieldLayout->offset;

                PDEBUG("GC: getReferences:      Found a reference	= %p\n", fieldAddress);

                /* The found reference points to something */
                if (fieldAddress != NULL) {
                    xanList_insert(referenceList, fieldAddress);
                }
            }

            /* Retrieve the next field */
            currentField = currentField->next;
        }
    }
    PDEBUG("GC: getReferences: Exit\n");

    return;
}

/* Get objects referenced by given object */
XanList*                getReferencedObject (void* object) {
    /* Given object header */
    t_objectHeader* objectHeader;

    /* Enabled if given object is an array */
    JITBOOLEAN isArray;

    /* Enabled if given object is an array that stores basic type */
    JITBOOLEAN isValueType;

    /* A list of references */
    XanList*        referenceList;

    /* Object layout infos */
    ILLayout*       layout;

    /* Object layout infos, if static */
    ILLayoutStatic* layoutStatic;

    /* Given object fields layout */
    XanList*        fieldsLayout;

    /* Assertions */
    assert(object != NULL);
    PDEBUG("GC: getReferencedObject: Start\n");

    /* Initialize local variables */
    isValueType = JITFALSE;

    /* Create the list of instances */
    referenceList = xanList_new(allocFunction, freeFunction, NULL);
    assert(referenceList != NULL);
    PDEBUG("GC: getReferencedObject:        Fetch the header of the object\n");

    /* Retrieve the pointer to the header of the object */
    objectHeader = GET_OBJECT_HEADER(object);
    assert(objectHeader != NULL);
    PDEBUG("GC: getReferencedObject:                Header		= %p\n", objectHeader);

    /* Given object is an array */
    if (HAS_ARRAY_FLAG(objectHeader)) {
        PDEBUG("GC: getReferencedObject:        The instance is an array\n");

        /* Set the isArray flag */
        isArray = JITTRUE;

        /* Array stores basic types */
        if (HAS_VALUETYPE_FLAG(objectHeader)) {
            PDEBUG("GC: getReferencedObject:        The instance is a value type\n");

            /* Set the isValueType flag */
            isValueType = JITTRUE;
        }
    }
    /* Given object is a normal object */
    else {
        PDEBUG("GC: getReferencedObject:        "
               "The instance is a normal object\n");

        isArray = JITFALSE;
    }

    /* Given object is static */
    if (HAS_STATIC_FLAG(objectHeader)) {
        PDEBUG("GC: getReferencedObject:        "
               "The instance is static\n");

        /* Retrieve the fields layout informations */
        layoutStatic = (cliManager->layoutManager).layoutStaticType(&(cliManager->layoutManager), objectHeader->type);
        fieldsLayout = layoutStatic->fieldsLayout;
    }
    /* Given object is a normal object */
    else {
        PDEBUG("GC: getReferencedObject:        "
               "The instance is not static\n");

        /* Retrieve the fields layout informations */
        layout = (cliManager->layoutManager).layoutType(&(cliManager->layoutManager), objectHeader->type);
        fieldsLayout = layout->fieldsLayout;
    }

    PDEBUG("GC: getReferencedObject:        Check if exist some fields of this object\n");

    /* Given object has some field? */
    if (fieldsLayout == NULL) {
        PDEBUG("GC: getReferencedObject:        No fields\n");
    }
    /* Fields found */
    else {
        PDEBUG("GC: getReferencedObject:                "
               "Fields found\n");
        PDEBUG("GC: getReferencedObject:        "
               "Retrieve the references\n");

        /* Retrive the references */
        gci_getReferences(object, referenceList, fieldsLayout,
                          isArray, isValueType);
    }

    PDEBUG("GC: getReferencedObject: Exit\n");

    /* Return */
    return referenceList;
}

/* Check if given object is collectable */
JITBOOLEAN              isCollectable (void* object) {
    /* Return value */
    JITBOOLEAN collectable;

    /* Assertions */
    assert(object != NULL);

    PDEBUG("GC: isCollectable: START \n");

    /* Object uncollectable */
    if (gci_isStatic(object) || gci_isPermanent(object)) {
        collectable = JITFALSE;
        PDEBUG("GC: isCollectable: END - NOT COLLECTABLE\n");
    }
    /* Object collectable */
    else {
        collectable = JITTRUE;
        PDEBUG("GC: isCollectable: END - COLLECTABLE \n");
    }

    return collectable;
}

/* Returns current root set */
XanList*                getRootSet (void) {
    /* Root set slot index */
    JITUINT32 count;

    /* System reflection manager */
    t_reflectManager*               reflectionManager;

    /* A root set container */
    XanHashTableItem*                       threadRootSetItem;

    /* Root set of a thread */
    t_root_sets*                    threadRootSet;

    /* Error message buffer */
    char buffer[DIM_BUF];

    /* Object reference in the root set */
    void**                          objectReference;

    /* Assertions  */
    assert(rootSetList == NULL);

    PDEBUG("GC: getRootSet: Start\n");

    /* Lock the root sets */
    xanHashTable_lock(rootSets);

    /* Allocate the new list */
    rootSetList = xanList_new(allocFunction, freeFunction, NULL);

    /* Get all we need */
    reflectionManager = &((cliManager->CLR).reflectManager);

    /* Check if the current GC needs this kind of support */
    if (!GC_NEEDS_ROOTSET(garbageCollector->gc_plugin)) {
        snprintf(buffer, sizeof(buffer), "ILJIT: ERROR = The garbage collector %s sayd that it does not need the root set information, now it ask for this information.", garbageCollector->gc_plugin->getName());
        print_err(buffer, 0);
        abort();
    }

    PDEBUG("GC: getRootSet:         Add the slots from the methods stack "
           "to the root set\n");

    /* Cycle throught all threads root sets */
    threadRootSetItem = xanHashTable_first(rootSets);
    while (threadRootSetItem != NULL) {
        /* Retrive root set */
        threadRootSet = threadRootSetItem->element;

        /* Add the normal objects */
        count = 0;
        objectReference = threadRootSet->getRootSetSlot(threadRootSet, &count);
        count++;
        while (objectReference != NULL) {

            /* Current reference points to something */
            if ((*objectReference) != NULL) {
                assert(rootSetList != NULL);
                xanList_insert(rootSetList, objectReference);
            }

            /* Get next slot */
            objectReference = threadRootSet->getRootSetSlot(threadRootSet, &count);
            count++;
        }

        /* Fetch the next thread root set container */
        threadRootSetItem = xanHashTable_next(rootSets, threadRootSetItem);
    }

    /* Add the static objects */
    xanList_syncAppendList(rootSetList, xanHashTable_toSlotList((ildjitSystem->staticMemoryManager).staticObjects));

    /* Add the objects of the reflection library */
    if (reflectionManager->clrProperties != NULL) {
        xanList_appendList(rootSetList, xanHashTable_toSlotList(reflectionManager->clrProperties));
    }
    if (reflectionManager->clrMethods != NULL) {
        xanList_appendList(rootSetList, xanHashTable_toSlotList(reflectionManager->clrMethods));
    }
    if (reflectionManager->clrConstructors != NULL) {
        xanList_appendList(rootSetList, xanHashTable_toSlotList(reflectionManager->clrConstructors));
    }
    if (reflectionManager->clrFields != NULL) {
        xanList_appendList(rootSetList, xanHashTable_toSlotList(reflectionManager->clrFields));
    }
    if (reflectionManager->clrEvents != NULL) {
        xanList_appendList(rootSetList, xanHashTable_toSlotList(reflectionManager->clrEvents));
    }
    if (reflectionManager->clrParameter != NULL) {
        xanList_appendList(rootSetList, xanHashTable_toSlotList(reflectionManager->clrParameter));
    }


    if (reflectionManager->clrAssemblies != NULL) {
        xanList_appendList(rootSetList, xanHashTable_toSlotList( reflectionManager->clrAssemblies));
    }
    if (reflectionManager->clrModule != NULL) {
        xanList_appendList(rootSetList, xanHashTable_toSlotList(reflectionManager->clrModule));
    }
    if (reflectionManager->clrTypes != NULL) {
        xanList_appendList(rootSetList, xanHashTable_toSlotList(reflectionManager->clrTypes));
    }


    /* Add strings                                  */
    xanList_appendList(rootSetList, xanHashTable_toSlotList((cliManager->CLR).stringManager.usObjects));
    xanList_appendList(rootSetList, xanHashTable_toSlotList((cliManager->CLR).stringManager.internHashTable));

    /* Add the OutOfMemory exception */
    xanList_insert(rootSetList, &(ildjitSystem->exception_system._OutOfMemoryException));

    /* Unlock the root sets	*/
    xanHashTable_unlock(rootSets);

    /* Return the root set */
    return rootSetList;
}

/* Get given object size */
JITNUINT                sizeObject (void* object) {
    /* Given object header */
    t_objectHeader* objectHeader;

    /* Given object total size */
    JITUINT32 overallSize;

    /* Assertions */
    assert(object != NULL);

    PDEBUG("GC: sizeObject: Start\n");

    /* Retrieve the pointer to the header of the object */
    objectHeader = GET_OBJECT_HEADER(object);

    /* Retrieve the overall size of the instance */
    overallSize = objectHeader->objectSize;

    /* Print the overall size */
    PDEBUG("GC: sizeObject:         Size	= %u\n", overallSize);

    /* Return the value */
    return overallSize;
}

/* Unset permanent flag from given object */
static void             gci_unsetPermanentFlag (void* object) {
    t_objectHeader     *objectHeader;

    /* Assertions */
    assert(object != NULL);

    /* Retrieve the pointer to the header of the object */
    objectHeader = GET_OBJECT_HEADER(object);

    /* Unset the permanent flag */
    objectHeader->flags &= (IS_ARRAY | IS_VALUETYPE | IS_STATIC);
}

static void gci_setFinalizedFlag (void* object) {
    /* Given object header */
    t_objectHeader     *objectHeader;

    /* Check pointer */
    assert(object != NULL);

    /* Retrieve the pointer to the header of the object */
    objectHeader = GET_OBJECT_HEADER(object);

    /* Set the static flag */
    objectHeader->flags |= IS_FINALIZED;
}

/* Set permanent flag on given object */
static void     gci_setPermanentFlag (void* object) {
    /* Given object header */
    t_objectHeader     *objectHeader;

    /* Check pointer */
    assert(object != NULL);

    /* Retrieve the pointer to the header of the object */
    objectHeader = GET_OBJECT_HEADER(object);

    /* Set the static flag */
    objectHeader->flags |= IS_COLLECTABLE;
}

/* Set IS_STATIC flag on given object */
static void     gci_setStaticFlag (void* object) {
    /* Given object header */
    t_objectHeader* objectHeader;

    /* Check pointer */
    assert(object != NULL);

    /* Retrieve the pointer to the header of the object */
    objectHeader = GET_OBJECT_HEADER(object);

    /* Set the static flag */
    objectHeader->flags |= IS_STATIC;
}

/* Check if an object is permanent */
static JITBOOLEAN       gci_isPermanent (void* object) {
    /* Given object header */
    t_objectHeader* header;

    /* Check pointer */
    assert(object != NULL);

    /* Get object header */
    header = GET_OBJECT_HEADER(object);

    /* Retrieve the information */
    return HAS_COLLECTABLE_FLAG(header);
}

/* Check if given object is static */
static JITBOOLEAN       gci_isStatic (void* object) {
    /* Given object header */
    t_objectHeader* header;

    /* Check pointer */
    assert(object != NULL);

    /* Get object header */
    header = GET_OBJECT_HEADER(object);

    /* Check for IS_STATIC flag */
    return HAS_STATIC_FLAG(header);
}

/* Get a pointer to given object virtual table */
static void**           gci_getVTable (void* object) {
    /* Given object header */
    t_objectHeader* objectHeader;

    /* Assertions */
    assert(object != NULL);

    /* Get object header */
    objectHeader = GET_OBJECT_HEADER(object);

    /* Return the pointer that address the virtual table of the object */
    return objectHeader->virtualTable;
}

/* Get the IR type of array elements */
static JITUINT32        gci_getArraySlotIRType (void* array) {

    /* Given array header */
    t_arrayHeader*  header;

    /* Given array layout infos */
    ILLayout*       infos;

    /* Assertions */
    assert(array != NULL);

    /* Retrieve the array header */
    header = GET_ARRAY_HEADER(array);

    /* Retrieve the layout infos */
    infos = (cliManager->layoutManager).layoutType(&(cliManager->layoutManager), header->type);
    assert(infos != NULL);

    /* Return the array slot type */
    return infos->type->getIRType(infos->type);
}

/* Get array header dimSize offset from a generic IR machine array address */
static JITINT32         gci_getArrayLengthOffset (void) {
    /* dimSize field offset */
    JITINT32 offset;

    PDEBUG("GC: getArrayLengthOffset: Start\n");

    /* Compute the offset */
    offset = offsetof(t_arrayHeader, dimSize) - HEADER_FIXED_SIZE;

    PDEBUG("GC: getArrayLengthOffset:       Offset			= %d\n",
           offset);
    PDEBUG("GC: getArrayLengthOffset: Exit\n");

    /* Return computed offset */
    return offset;
}

/* Get given array rank */
static JITUINT16        gci_getArrayRank (void* array) {
    /* Given array header */
    t_arrayHeader*  header;
    ArrayDescriptor *arrayDesc;

    /* Assertions */
    assert(array != NULL);

    /* Retrieve the array header */
    header = GET_ARRAY_HEADER(array);
    arrayDesc = GET_ARRAY(header->type);
    assert(arrayDesc != NULL);

    /* Return the array rank */
    return arrayDesc->rank;
}

/* Get given array length */
static JITUINT32        gci_getArrayLength (void* array) {
    /* Given array header */
    t_arrayHeader*  header;

    /* Check pointer */
    assert(array != NULL);

    PDEBUG("GC: getArrayLength: Start\n");

    /* Retrieve the array header */
    header = GET_ARRAY_HEADER(array);

    PDEBUG("GC: getArrayLength:     Array length	= %d\n", header->dimSize);
    PDEBUG("GC: getArrayLength: Exit\n");

    /* Return the length of the array */
    return header->dimSize;
}

/* Get methodID index in virtual table of binaryInfo */
static VCallPath        gci_getIndexOfVTable (MethodDescriptor *methodID) {
    /* Virtual table entry */
    VCallPath internalVMethod;

    /* System layout manager */
    ILLayout_manager*       layoutManager;

    PDEBUG("GC: getIndexOfVTable: Start\n");

    /* Get layout manager */
    layoutManager = &(cliManager->layoutManager);

    /* Print the method name */
    PDEBUG("GC: getIndexOfVTable:   Method	= %s\n", methodID->getCompleteName(methodID));

    /* Fetch the method */
    internalVMethod = layoutManager->prepareVirtualMethodInvocation(layoutManager, methodID);

#ifdef PRINTDEBUG
    if (internalVMethod.useIMT) {
        //imt_index is not specified in VCallPath data structure.
        //PDEBUG("GC: getIndexOfVTable:   IMT Index	= %u\n", internalVMethod.imt_index);
        PDEBUG("GC: getIndexOfVTable:   IMT Index specified in data structure.");
    } else {
        PDEBUG("GC: getIndexOfVTable:   Vtable Index	= %u\n", internalVMethod.vtable_index);
    }
#endif
    PDEBUG("GC: getIndexOfVTable: Exit\n");

    /* Return method index in the virtual table */
    return internalVMethod;
}

/* Get virtual table offset */
JITINT32 gci_fetchVTableOffset (void) {
    return offsetof(t_objectHeader, virtualTable) - HEADER_FIXED_SIZE;
}

/* Get interface Method table offset */
JITINT32 gci_fetchIMTOffset (void) {
    return offsetof(t_objectHeader, interfaceMethodTable) - HEADER_FIXED_SIZE;
}

/* Get interface Method table offset */
JITINT32 gci_fetchTypeDescriptorOffset (void) {
    return offsetof(t_objectHeader, type) - HEADER_FIXED_SIZE;
}

/* Get given static field offset from its owner start address */
static JITINT32         gci_fetchStaticFieldOffset (FieldDescriptor *fieldID) {
    /* System layout manager */
    ILLayout_manager*       layoutManager;

    /* Field owner layout info */
    ILLayoutStatic*         layoutInfos;

    /* Field layout info */
    ILFieldLayout*          fieldInfos;

    /* Get layout manager*/
    layoutManager = &(cliManager->layoutManager);

    /* Retrieve the layout informations of ownerType */
    layoutInfos = layoutManager->layoutStaticType(layoutManager, fieldID->owner);

    /* Retrieve the field layout informations */
    fieldInfos = layoutInfos->fetchStaticFieldLayout(layoutInfos, fieldID);

    /* Return the offset of the field */
    return fieldInfos->offset;
}

/* Get given field offset from its owner start address */
static JITINT32         gci_fetchFieldOffset (FieldDescriptor *fieldID) {
    /* System layout manager */
    ILLayout_manager*       layoutManager;

    /* Field owner layout info */
    ILLayout*               layoutInfos;

    /* Field layout info */
    ILFieldLayout*          fieldInfos;

    /* Get layout manager*/
    layoutManager = &(cliManager->layoutManager);

    /* Retrieve the layout informations of ownerType */
    layoutInfos = layoutManager->layoutType(layoutManager, fieldID->owner);

    /* Retrieve the field layout informations */
    fieldInfos = layoutInfos->fetchFieldLayout(layoutInfos, fieldID);

    /* Return the offset of the field */
    return fieldInfos->offset;
}

static void gci_collect (void) {

    /* Run the collection		*/
    (garbageCollector->gc_plugin)->collect();

    /* Check if the garbage         *
    * collector has called the     *
    * getRootSet method.           */
    if (rootSetList != NULL) {
        xanList_destroyList(rootSetList);
        rootSetList = NULL;
    }
    assert(rootSetList == NULL);

    /* Return			*/
    return;
}

/* Alloc a new array */
static void*            gci_allocArray (TypeDescriptor *type, JITINT32 rank, JITINT32 size) {
    void *newArray;

    PDEBUG("GC: allocArray: Start\n");
    PDEBUG("GC: allocArray:         Rank	= %d \n", rank);
    PDEBUG("GC: allocArray:         Size    = %d \n", size);

    /* Create a new array */
    TypeDescriptor *resolvedType = type->makeArrayDescriptor(type, rank);
    ArrayDescriptor *arrayType = GET_ARRAY(resolvedType);
    newArray = gci_createArray(arrayType, size);
    PDEBUG("GC: allocArray:         New array       = %p\n", newArray);

    /* Return the just created array instance */
    PDEBUG("GC: allocArray: Exit\n");
    return newArray;
}

/* Alloc a new static object */
static void*            gci_allocStaticObject (TypeDescriptor *type, JITUINT32 overSize) {
    /* Create a permanent new instance */
    return gci_createInstance(type, overSize, JITFALSE, JITTRUE);
}

static void             gci_freeObject (void *obj) {
    void    *mem;

    mem = obj - HEADER_FIXED_SIZE;

    if (garbageCollector->gc_plugin->freeObject != NULL) {
        garbageCollector->gc_plugin->freeObject(mem);
    } else {
        freeFunction(mem);
    }

    return ;
}

/* Alloc a new normal object */
static void*            gci_allocObject (TypeDescriptor *type, JITUINT32 overSize) {
    void *newObject;

    PDEBUG("GC: allocObject: Start\n");

    /* Make a new object type */
    PDEBUG("GC: allocObject:        Make a new type information\n");

    /* Create a new instance */
    PDEBUG("GC: allocObject:        Create the new instance\n");
    newObject = gci_createInstance(type, overSize, JITTRUE, JITFALSE);
    PDEBUG("GC: allocObject:	New instance is %p\n", newObject);

    /* Return the just created instance */
    PDEBUG("GC: allocObject: Exit\n");
    return newObject;
}

/* Create a new permanent object */
static void*            gci_allocPermanentObject (TypeDescriptor *type, JITUINT32 overSize) {
    /* Try to alloc a new object */
    return gci_createInstance(type, overSize, JITFALSE, JITFALSE);
}

/* Unset given object STATIC flag */
static void             gci_unsetStaticFlag (void* object) {
    /* Given object header */
    t_objectHeader* objectHeader;

    /* Assertions */
    assert(object != NULL);

    /* Retrieve the pointer to the header of the object */
    objectHeader = GET_OBJECT_HEADER(object);

    /* Unset the static flag */
    objectHeader->flags &= (IS_ARRAY | IS_VALUETYPE | IS_COLLECTABLE);
}

/* Get the oversize offset of the given object */
static JITINT32         gci_fetchOverSizeOffset (void* object) {
    /* Object header */
    t_objectHeader* objectHeader;

    /* Array header */
    t_arrayHeader*  arrayHeader;

    /* Object/Array layout */
    ILLayout*       layoutInfos;

    /* Object layout (static version) */
    ILLayoutStatic* staticLayoutInfos;

    /* The oversize offset */
    JITUINT32 oversizeOffset;

    /* Array element size */
    JITUINT32 slotSize;

    /* Array element IR type */
    JITUINT32 irType;

    /* Assertions */
    assert(object != NULL);

    /* Fetch object header */
    objectHeader = GET_OBJECT_HEADER(object);

    /* Given object is an array */
    if (HAS_ARRAY_FLAG(objectHeader)) {
        /* Retrieve the array header */
        arrayHeader = (t_arrayHeader*) objectHeader;

        /* Retrieve the layout infos */
        layoutInfos = (cliManager->layoutManager).layoutType(&(cliManager->layoutManager), arrayHeader->type);

        /* Given array si a VALUE-TYPE? */
        if (HAS_VALUETYPE_FLAG(arrayHeader)) {
            /* Retrieve the oversize offset */
            oversizeOffset = layoutInfos->typeSize * gci_getArrayRank(object) * arrayHeader->dimSize;
            /* Given array isn't a VALUE-TYPE */
        } else {
            /* Retrieve the IR type associated with this array */
            irType = layoutInfos->type->getIRType(layoutInfos->type);

            /* retrieve the slot size */
            slotSize = getIRSize(irType);

            /* Evaluate the oversize offset */
            oversizeOffset = slotSize * gci_getArrayRank(object) * arrayHeader->dimSize;
        }
    }
    /* Given object is a real object */
    else {
        /* Given object is STATIC */
        if (HAS_STATIC_FLAG(objectHeader)) {
            /* Retrieve the oversize offset */
            staticLayoutInfos = (cliManager->layoutManager).layoutStaticType(&(cliManager->layoutManager), objectHeader->type);
            assert(staticLayoutInfos != NULL);
            oversizeOffset = staticLayoutInfos->typeSize;
        }
        /* Given object is a "normal" object */
        else {
            /* Retrive the oversize offset */
            layoutInfos = (cliManager->layoutManager).layoutType(&(cliManager->layoutManager), objectHeader->type);
            assert(layoutInfos != NULL);
            oversizeOffset = layoutInfos->typeSize;
        }
    }

    /* Return the offset */
    return oversizeOffset;
}

/* Get object IL type */
static inline TypeDescriptor * gci_getType (void* object) {

    /* Object header */
    t_objectHeader* objectHeader;

    /* Assertions */
    assert(object != NULL);

    /* Retrieve the pointer to the header of the object */
    objectHeader = GET_OBJECT_HEADER(object);

    /* Return the type of the current object */
    return objectHeader->type;
}

static JITINT32 gci_getTypeSize (void * object) { /* Given object type */
    TypeDescriptor          *type;
    JITUINT32 typeSize;
    ILLayout*                       layoutInfos;
    ILLayoutStatic*                 staticInfos;
    ILLayout_manager*               layoutManager;

    /* Get all we need */
    layoutManager = &(cliManager->layoutManager);

    /* Fetch the type */
    type = gci_getType(object);

    /* We are creating a normal instance */
    if (!gci_isStatic(object)) {
        layoutInfos = layoutManager->layoutType(layoutManager, type);
        typeSize = layoutInfos->typeSize;
    } else {
        staticInfos = layoutManager->layoutStaticType(layoutManager, type);
        typeSize = staticInfos->typeSize;
    }

    return typeSize;
}

void setup_garbage_collector_interaction (void) {

    /* Setup the CLI manager	*/
    cliManager = &(ildjitSystem->cliManager);

    /* Setup the IRVM		*/
    IRVM = &(ildjitSystem->IRVM);

    /* Get garbage collector */
    garbageCollector = &(ildjitSystem->garbage_collectors.gc);

    /* Update all the function pointers */
    garbageCollector->isSubtype = GCI_ISSUBTYPE;
    garbageCollector->fetchOverSizeOffset = GCI_FETCHOVERSIZEOFFSET;
    garbageCollector->unsetStaticFlag = GCI_UNSETSTATICFLAG;
    garbageCollector->collect = GCI_COLLECT;
    garbageCollector->getType = GCI_GETTYPE;
    garbageCollector->getTypeSize = GCI_GETTYPESIZE;
    garbageCollector->allocObject = GCI_ALLOCOBJECT;
    garbageCollector->freeObject = GCI_FREEOBJECT;
    garbageCollector->allocPermanentObject = GCI_ALLOCPERMANENTOBJECT;
    garbageCollector->allocStaticObject = GCI_ALLOCSTATICOBJECT;
    garbageCollector->allocArray = GCI_ALLOCARRAY;
    garbageCollector->createArray = GCI_CREATEARRAY;
    garbageCollector->fetchFieldOffset = GCI_FETCHFIELDOFFSET;
    garbageCollector->fetchStaticFieldOffset = GCI_FETCHSTATICFIELDOFFSET;
    garbageCollector->fetchVTableOffset = GCI_FETCHVTABLEOFFSET;
    garbageCollector->fetchIMTOffset = GCI_FETCHIMTOFFSET;
    garbageCollector->fetchTypeDescriptorOffset = GCI_FETCHTYPEDESCRIPTOROFFSET;
    garbageCollector->getIndexOfVTable = GCI_GETINDEXOFVTABLE;
    garbageCollector->getArrayLength = GCI_GETARRAYLENGTH;
    garbageCollector->getArrayRank = GCI_GETARRAYRANK;
    garbageCollector->getArrayLengthOffset = GCI_GETARRAYLENGTHOFFSET;
    garbageCollector->getArraySlotIRType = GCI_GETARRAYSLOTIRTYPE;
    garbageCollector->getVTable = GCI_GETVTABLE;
    garbageCollector->addRootSet = GCI_ADDROOTSET;
    garbageCollector->popLastRootSet = GCI_POPLASTROOTSET;
    garbageCollector->setStaticFlag = GCI_SETSTATICFLAG;
    garbageCollector->setPermanentFlag = GCI_SETPERMANENTFLAG;
    garbageCollector->setFinalizedFlag = GCI_SETFINALIZEDFLAG;
    garbageCollector->unsetPermanentFlag = GCI_UNSETPERMANENTFLAG;
    garbageCollector->getSize = SIZEOBJECT;
    garbageCollector->threadCreate = GCI_THREADCREATE;
    garbageCollector->threadExit = GCI_THREADEXIT;
    garbageCollector->lockObject = GCI_LOCKOBJECT;
    garbageCollector->unlockObject = GCI_UNLOCKOBJECT;
    garbageCollector->waitForPulse = GCI_WAITFORPULSE;
    garbageCollector->signalPulse = GCI_SIGNALPULSE;
    garbageCollector->signalPulseAll = GCI_SIGNALPULSEALL;

    return ;
}

static inline JITINT32 internal_are_equal_threads (pthread_t *k1, pthread_t *k2) {
    pthread_t* t1 = (pthread_t  *) k1;
    pthread_t* t2 = (pthread_t *) k2;

    if (t1 == t2) {
        return JITTRUE;
    }
    if (t1 == NULL || t2 == NULL) {
        return JITFALSE;
    }
    return PLATFORM_equalThread(*t1, *t2);
}

/* Create a new array that stores element of given type */
static void*            gci_createArray (ArrayDescriptor* type, JITUINT32 size) {
    /* System layout manager */
    ILLayout_manager*               layoutManager;

    /* Array global size without oversize */
    JITUINT32 arraySize;

    /* Array total size */
    JITUINT32 overallSize;

    /* Array layout info */
    TypeDescriptor*         arrayType;
    ILLayout*                       arrayLayoutInfos;
    ILLayout*    	layoutInfos;

    /* Pointer to new array instance */
    void*                           array;

    /* Array element size */
    JITUINT32 slotSize;

    /* New instance header */
    t_arrayHeader*                  header;

    /* Enabled if the new array contains basic types (int, long, ...) */
    JITBOOLEAN isValueType;

    /* Check pointer */
    assert(type != NULL);
    PDEBUG("GC: createArray: START\n");

    /* Get all we need */
    layoutManager = &(cliManager->layoutManager);

    /* Fetch the CIL type of System.Array	*/
    arrayType = type->getTypeDescriptor(type);
    assert(arrayType != NULL);

    /* Fetch the layout of the System.Array type	*/
    arrayLayoutInfos = layoutManager->layoutType(layoutManager, arrayType);
    assert(arrayLayoutInfos != NULL);

    /* Retrieve the IRType from the ILType.
     */
    layoutInfos = layoutManager->layoutType(layoutManager, type->type);

    /* Check if values stored inside the array are valuetypes.
     */
    isValueType = layoutInfos->type->isValueType(layoutInfos->type);

    /* Set the slot size for the array.
     */
    if (isValueType) {
        slotSize = layoutInfos->typeSize;
    } else {
        slotSize = getIRSize(IRMPOINTER);
    }

    /* Set the array size */
    arraySize = slotSize * type->rank * size;

    /* Retrieve the overall size.
     * An array can't have 0 fields: set at least an oversize of IRMPOINTER
     */
    overallSize = HEADER_FIXED_SIZE + arraySize + getIRSize(IRMPOINTER);
    assert(overallSize > HEADER_FIXED_SIZE);

    /* Call the garbage collector */
    PDEBUG("GC: createArray:	slotSize == %u\n", slotSize);
    PDEBUG("GC: createArray:        array_size == %u\n", arraySize);
    PDEBUG("GC: createArray:        rank == %u\n", type->rank);
    PDEBUG("GC: createArray:        size == %u\n", size);
    PDEBUG("GC: createArray:        overall_size == %u\n", overallSize);
    array = garbageCollector->gc_plugin->allocObject(overallSize);

    /* Check if the memory is finished */
    if (array == NULL) {
        IRVM_throwBuiltinException(&(ildjitSystem->IRVM), OUT_OF_MEMORY_EXCEPTION);
        print_err("createArray: ERROR = Libjit does not work as specified.", 0);
        abort();
    }

    /* Get array header */
    header = (t_arrayHeader*) array;
    assert(header != NULL);

    /* Set the IS_ARRAY flag */
    header->flags |= IS_ARRAY;

    /* Set the IS_COLLECTABLE flag */
    header->flags |= IS_COLLECTABLE;

    /* Set the IS_VALUETYPE flag */
    if (isValueType) {
        header->flags |= IS_VALUETYPE;
    }

    /* Update header infos */
    header->type = arrayType;
    header->objectSize = overallSize;
    header->dimSize = size;
    LAYOUT_getMethodTables(arrayLayoutInfos, &header->virtualTable, &header->interfaceMethodTable);

    /* Initialize object lock and condition variable */
    ILJITMonitorInit(&header->monitor);

    PDEBUG("GC: createArray: EXIT\n");

    /* Normalize addres to IR machine format (without header) */
    array += HEADER_FIXED_SIZE;
    assert(array != NULL);

    /* Return the instance just created */
    return array;
}

/* Create a new instance of given type */
static void*            gci_createInstance (TypeDescriptor* type, JITUINT32 overSize,
        JITBOOLEAN isCollectable,
        JITBOOLEAN isStatic) {
    /* The new instance */
    void*                           object;

    /* The total memory we need to alloc given object */
    JITUINT32 overallSize;

    /* Given type basic size, without header and oversize */
    JITUINT32 typeSize;

    /* Given type info, if we alloc in non static mode */
    ILLayout*                       layoutInfos;

    /* Given type info, if we alloc in static mode */
    ILLayoutStatic*                 staticInfos;

    /* System layout manager */
    ILLayout_manager*               layoutManager;

    /* Current running garbage collector */

    /* New instance header */
    t_objectHeader*                 header;

    /* Check pointer */
    assert(type != NULL);
    PDEBUG("GC: createInstance: Start\n");

    /* Initialize the variables	*/
    layoutInfos = NULL;
    staticInfos = NULL;
    object = NULL;

    /* Get all we need */
    layoutManager = &(cliManager->layoutManager);
    assert(layoutManager != NULL);

    /* Retrieve the layout infos for the specified type */
    PDEBUG("GC: createInstance:     Retrieve the layout informations\n");

    /* We are creating a normal instance */
    if (!isStatic) {
        /* Retrieve the typeSize of the instance */
        layoutInfos = layoutManager->layoutType(layoutManager, type);
        PDEBUG("GC: createInstance:     Type = %s\n", type->getCompleteName(type));
        typeSize = layoutInfos->typeSize;
    }
    /* We are creating a static instance */
    else {
        /* Retrieve the typeSize of the instance */
        staticInfos = layoutManager->layoutStaticType(layoutManager, type);
        typeSize = staticInfos->typeSize;
    }

    /* Call allocObject on GC */
    PDEBUG("GC: createInstance:     Call the GC to fetch the memory for the new instance\n");

    overallSize = HEADER_FIXED_SIZE + typeSize + overSize;
    object = garbageCollector->gc_plugin->allocObject(overallSize);
    PDEBUG("GC: createInstance:     Write the header of the new object\n");

    /* Ok, the garbage collector gave us the needed memory */
    if (object != NULL) {

        /* Erase the object memory */
        memset(object, 0, overallSize);

        /* Get object header */
        header = (t_objectHeader*) object;

        /* Set flags */
        if (isCollectable) {
            header->flags |= IS_COLLECTABLE;
        }
        if (isStatic) {
            header->flags |= IS_STATIC;
        }

        /* Save layout info, normal mode */
        if (layoutInfos != NULL) {
            LAYOUT_getMethodTables(layoutInfos, &header->virtualTable, &header->interfaceMethodTable);

            /* Using the default object as a matrix for the real instance */
//			assert(layoutInfos->initialized_obj != NULL);
//			memcpy(object+HEADER_FIXED_SIZE, layoutInfos->initialized_obj, typeSize);
        }
        /* Save static layout info into object header */
        else {

            /* Using the default object as a matrix for the real instance */
            if (typeSize > 0) {
                assert(staticInfos->initialized_obj != NULL);
                memcpy(object + HEADER_FIXED_SIZE, staticInfos->initialized_obj, typeSize);
            }
        }
        header->type = type;
        /* Save object size into its header */
        header->objectSize = overallSize;
        PDEBUG("GC: createInstance:	virtualTable = %p\n", header->virtualTable);
        PDEBUG("GC: createInstance:	interfaceMethodTable = %p\n", header->interfaceMethodTable);
        PDEBUG("GC: createInstace:	objectSize = %u\n", header->objectSize);

        /* Initialize object lock and condition variable */
        ILJITMonitorInit(&header->monitor);
    } else {

        /* Error: no avaible memory for allocation */
        IRVM_throwBuiltinException(&(ildjitSystem->IRVM), OUT_OF_MEMORY_EXCEPTION);
        print_err("createArray: ERROR = Libjit does not work as specified.", 0);
        abort();
    }

    /* Normalize address to IR machine format (remove header) */
    object += HEADER_FIXED_SIZE;

    /* Return the instance just created */
    PDEBUG("GC: createInstance: Exit\n");
    return object;
}

/* Check if given object class is subtype of given class identifier */
static JITBOOLEAN       gci_isSubtype (void* object, TypeDescriptor *type) {

    /* Given object type */
    TypeDescriptor          *objectType;

    /* Assertions */
    assert(type != NULL);
    assert(object != NULL);

    /* Fetch the type */
    objectType = gci_getType(object);

    /* Verify if objectType is subtype of ``type'' */
    return objectType->isSubtypeOf(objectType, type);
}

/* Get size of a given array slot */
JITUINT32               getArraySlotSize (void* array) {
    /* Array slot IR type */
    JITUINT32 irType;

    /* Given array slot size */
    JITUINT32 arraySlotSize;

    /* Given array header */
    t_arrayHeader*  header;

    /* Array layout info */
    ILLayout*       layoutInfos;

    /* Assertions */
    assert(array != NULL);

    /* Retrieve the IR type associated with the current slot type */
    irType = gci_getArraySlotIRType(array);

    /* Array don't stores references */
    if (irType != IRVALUETYPE) {
        arraySlotSize = getIRSize(irType);
    }
    /* Array stores references */
    else {
        /* Retrieve the array header */
        header = GET_ARRAY_HEADER(array);

        /* Get layout infos */
        layoutInfos = (cliManager->layoutManager).layoutType(&(cliManager->layoutManager), header->type);

        /* Get slot size */
        arraySlotSize = layoutInfos->typeSize;
    }

    return arraySlotSize;
}

/* Create a new thread */
static JITINT32         gci_threadCreate (pthread_t* thread,
        pthread_attr_t* attr,
        void* (*startRoutine)(void*),
        void* arg) {
    return PLATFORM_createThread(thread, attr, startRoutine, arg);
}

/* Add given root set to global one */
static void             gci_addRootSet (void*** rootSet, JITUINT32 size) {
    /* Loop counter */
    JITUINT32 count;

    /* Current thread identifier */
    pthread_t *threadId;

    /* Current thread root set */
    t_root_sets*                    threadRootSet;

    t_garbage_collector_plugin*     garbageCollector;

    /* Check pointer */
    assert(rootSet != NULL);

    PDEBUG("GC: addRootSet: Start\n");
    PDEBUG("GC: addRootSet:         Size of root set to add	= %u\n", size);

    /* Get garbage collector interface */
    garbageCollector = ildjitSystem->garbage_collectors.gc.gc_plugin;

    /* Check if the garbage collector has requested this kind of support */
    if (!GC_NEEDS_ROOTSET(garbageCollector)) {
        return;
    }

    /* Get current thread root set */
    threadId = allocFunction(sizeof(pthread_t));
    (*threadId) = PLATFORM_getSelfThreadID();
    threadRootSet = xanHashTable_syncLookup(rootSets, (void *) threadId);

    /* New thread? */
    if (threadRootSet == NULL) {
        threadRootSet = allocFunction(sizeof(t_root_sets));
        init_root_sets(threadRootSet);
        xanHashTable_syncInsert(rootSets, (void *) threadId, threadRootSet);
    }
    assert(threadRootSet != NULL);

    /* Lock root set */
    threadRootSet->lock(threadRootSet);

    /* Add a new slot for the new root set */
    threadRootSet->addNewRootSet(threadRootSet);

    /* Add the new root set */
    for (count = 0; count < size; count++) {
        PDEBUG("GC: addRootSet:         Add method %u-th element\n", count);
        assert(rootSet[count] != NULL);
        threadRootSet->addNewRootSetSlot(threadRootSet, rootSet[count]);

        PDEBUG("GC: addRootSet:		Root set top	= %u\n", threadRootSet->top);
    }
    assert(threadRootSet->top > 0);


    /* Unlock root set */
    threadRootSet->unlock(threadRootSet);

    PDEBUG("GC: addRootSet: Exit\n");

    /* Return from routine */
    return;
}

/* Remove last added root set from root set stack */
static void             gci_popLastRootSet (void) {
    /* Current thread root set */
    t_root_sets*                    threadRootSet;
    t_garbage_collector_plugin*     garbageCollector;
    pthread_t tempKey;

    PDEBUG("GC: popLastRootSet: Start\n");

    /* Fetch running garbage collector */
    garbageCollector = ildjitSystem->garbage_collectors.gc.gc_plugin;

    /* Check if the garbage collector has request this kind of support      */
    if (!GC_NEEDS_ROOTSET(garbageCollector)) {
        return;
    }

    /* Get current thread root set */
    tempKey = PLATFORM_getSelfThreadID();
    threadRootSet = xanHashTable_syncLookup(rootSets, (void *) &tempKey);
    assert(threadRootSet != NULL);

    /* Lock root set */
    threadRootSet->lock(threadRootSet);

    /* Pop the last root set */
    threadRootSet->popLastRootSet(threadRootSet);

    /* Unlock root set */
    threadRootSet->unlock(threadRootSet);

    PDEBUG("GC: popLastRootSet: Exit\n");

    /* Return */
    return;
}

/* Remove given thread root set from the active ones */
static void             gci_threadExit (pthread_t threadId) {

    /* The root set to delete */
    t_root_sets* threadRootSet;

    /* Check if there is a root set		*/
    if (rootSets == NULL) {
        return;
    }

    /* Lock by hand the table since we must do several operations */
    xanHashTable_lock(rootSets);

    /* Search and delete given thread root set */
    threadRootSet = xanHashTable_lookup(rootSets, (void *) &threadId);;
    if (threadRootSet != NULL) {
        freeFunction(threadRootSet);
        xanHashTable_removeElement(rootSets, (void *) &threadId);
    }

    /* All its done, exit from critical section */
    xanHashTable_unlock(rootSets);

}

/* Call Finalize method on given object */
void                    callFinalizer (void* object) {

    /* Given object header */
    t_objectHeader*         header;

    /* Given object layout infos */
    ILLayout*               layout;

    /* The Finalize method */
    Method finalize;

    /* Used to translate method */
    TranslationPipeline*    pipeliner;

    /* Finalize arguments */
    void*                   args[1];

    /* Finalizer aren't called on static objects */
    header = GET_OBJECT_HEADER(object);
    if (HAS_STATIC_FLAG(header)) {
        return;
    }
    if (HAS_FINALIZED_FLAG(header)) {
        return;
    }

    /* Retrive Finalize method */
    layout = (cliManager->layoutManager).layoutType(&(cliManager->layoutManager), header->type);
    finalize = layout->finalize;

    if (finalize != NULL) {

        /* Compile it */
        pipeliner = &(ildjitSystem->pipeliner);
        pipeliner->synchInsertMethod(pipeliner, finalize, MAX_METHOD_PRIORITY);

        /* Save Trace Profile */
        if (	(!(ildjitSystem->IRVM).behavior.aot) 		&&
                ((ildjitSystem->IRVM).behavior.pgc == 1)	) {
            MANFRED_appendMethodToProfile(&(ildjitSystem->manfred), finalize);
        }

        /* Check if the execution of code is allowed.
         */
        if (!(ildjitSystem->program).disableExecution) {

            /* And at last run */
            args[0] = &object;
            IRVM_run(&(ildjitSystem->IRVM), *(finalize->jit_function), args, NULL);
        }

        /* Free synchronization resources */
        ILJITMonitorDestroy(&header->monitor);
    }
}

/* Lock object */
static JITBOOLEAN gci_lockObject (void* object, JITINT32 timeout) {

    /* Object header */
    t_objectHeader* header;

    /* Whether lock has been acquired */
    JITBOOLEAN lockAcquired;

    /* Get object header */
    header = GET_OBJECT_HEADER(object);

    /* Lock object */
    lockAcquired = ILJITMonitorLock(&header->monitor, timeout);

    return lockAcquired;
}

/* Unlock object */
static void gci_unlockObject (void* object) {

    /* Object header */
    t_objectHeader* header;

    /* Get object header */
    header = GET_OBJECT_HEADER(object);

    /* Unlock the object */
    ILJITMonitorUnlock(&header->monitor);

}

/* Wait for pulse signal on object */
static JITBOOLEAN gci_waitForPulse (void* object, JITINT32 timeout) {

    /* Object header */
    t_objectHeader* header;

    /* Set to JITTRUE if pulse signal is detected before timeout */
    JITBOOLEAN pulseOccursBeforeTimeout;

    /* Get object header */
    header = GET_OBJECT_HEADER(object);

    /* Wait for signals */
    pulseOccursBeforeTimeout = ILJITMonitorWaitForPulse(&header->monitor,
                               timeout);

    return pulseOccursBeforeTimeout;

}

/* Signal pulse */
static void gci_signalPulse (void* object) {

    /* Given object header */
    t_objectHeader* header;

    /* Get the header */
    header = GET_OBJECT_HEADER(object);

    /* Signal the pulse */
    ILJITMonitorSignalPulse(&header->monitor);

}

/* Signal pulse all */
static void gci_signalPulseAll (void* object) {

    /* Given object header */
    t_objectHeader* header;

    /* Get the header */
    header = GET_OBJECT_HEADER(object);

    /* Signal the pulse all */
    ILJITMonitorSignalPulseAll(&header->monitor);
}
