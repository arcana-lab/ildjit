
/* For assert */
#include <assert.h>

/* For null */
#include <stdlib.h>

#include <iljit_gc_copy.h>

/* For profiler functions */
#include <gc_copy_utils.h>

/* For collector_t and related routines */
#include <collector.h>

/* Managed memory */
gc_memory_t heap;

/* Total collect time */
JITFLOAT32 total_collect_time;

/* TRUE if profile support is enabled */
JITBOOLEAN profile;

/* Get object size */
JITNUINT (*object_size)(void* object);

/* Get object children */
XanList* (*object_children)(void* object);

/* Get root set */
XanList* (*get_root_set)(void);

/* Call object finalizer */
void (*call_finalizer)(void*);

/* This symbol is used by ildjit, right? */
t_garbage_collector_plugin plugin_interface = {
  .init = gc_init,
  .shutdown = gc_shutdown,
  .getSupport = gc_get_support,
  .allocObject = gc_alloc_object,
  .reallocObject = gc_realloc_object,
  .collect = gc_collect,
  .threadCreate = gc_threadCreate,
  .gcInformations = gc_informations,
  .getName = gc_get_name,
  .getVersion = gc_get_version,
  .getAuthor = gc_get_author
};

/* Initialize garbage collector environment */
JITINT16 gc_init(t_gc_behavior* behavior, JITNUINT memory_size) {

  /* Function return value */
  JITINT16 retval;

  /* Check pointers */
  assert(behavior != NULL);

  /* Verify that callbacks are ok */
  if(behavior->sizeObject == NULL || behavior->getReferencedObject == NULL)
    retval = GC_BAD_IFACE;
  else {
    /* Save callbacks */
    object_size = behavior->sizeObject;
    object_children = behavior->getReferencedObject;
    get_root_set = behavior->getRootSet;
    call_finalizer = behavior->callFinalizer;

    /* No memory_size specified: use default value */
    if(memory_size == 0)
      memory_size = GC_DEFAULT_HEAP_SIZE;

    /* Initialize heap */
    gcm_init(&heap, memory_size);

    /* Initialize statistics */
    total_collect_time = 0;
    profile = behavior->profile;

    retval = GC_NO_ERROR;
  }

  return retval;

}

/* Turn off garbage collector */
JITINT16 gc_shutdown(void) {

  /* Destroy root set stack and memory */
  gcm_destroy(&heap);

  return GC_NO_ERROR;

}

/* Get mandatory features key */
JITINT32 gc_get_support(void) {

  return ILDJIT_GCSUPPORT_ROOTSET;

}

/* Alloc a new object in the heap */
void* gc_alloc_object(JITNUINT size) {

  /* New address */
  void* address;

  /* Get memory from heap */
  address = gcm_alloc(&heap, size);

  /* Not enought memory: try to run collection algorithm */
  if(address == NULL) {
    gc_collect();
    address = gcm_alloc(&heap, size);
  }

  return address;

}

/* Realloc an object on the heap */
void* gc_realloc_object(void* object, JITUINT32 size) {

  abort();

}

/* Run collection algorithm */
void gc_collect(void) {

  /* Collector object */
  collector_t collector;

  /* Start collection timer */
  GC_PROFILE_TIME();
  GC_PROFILE_TIME_START();

  /* Print a debug message for external profiler */
  GC_START_COLLECTION(GC_NAME, profile);
  
  /* Build a new collector */
  co_init(&collector, &heap);

  /* Run collection algorithm */
  co_collect(&collector);

  /* Destroy collector */
  co_destroy(&collector);

  /* Update profiler info */
  GC_PROFILE_TIME_END(&total_collect_time);

  /* Print a debug message for external profiler */
  GC_END_COLLECTION(GC_NAME, profile);

}

/* Create a new thread for CIL code: unsupported */
void gc_threadCreate(pthread_t* thread,
		     pthread_attr_t* attr,
		     void* (*startRoutine)(void*),
		     void* arg) {
  abort();

}

/* Get garbage collector informations */
void gc_informations(t_gc_informations *info) {

  /* Get memory stats from gc_memory component */
  info->heapMemoryAllocated = gcm_size(&heap);
  info->actualHeapMemory = gcm_used(&heap);
  info->maxHeapMemory = gcm_max_used(&heap);
  
  /* Persistent data structures are in gc_memory component */
  info->overHead = gcm_over_head(&heap);

  /* Collect time is stored by a global variable */
  info->totalCollectTime = total_collect_time;

  /* Alloc time is stored in gc_memory component */
  info->totalAllocTime = gcm_total_alloc_time(&heap);

}

/* Return garbage collector name */
JITINT8* gc_get_name(void) {

  return GC_NAME;

}

/* Return garbage collector version */
JITINT8* gc_get_version(void) {

  return GC_VERSION;

}

/* Return garbage collector author */
JITINT8* gc_get_author(void) {

  return GC_AUTHOR;

}
