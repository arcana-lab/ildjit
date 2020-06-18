
#ifndef GC_COPY_UTILS_H
#define GC_COPY_UTILS_H

/* For time functions */
#include <time.h>

/* For basic type declaration, such as JITNUINT */
#include <jitsystem.h>

/**
 * Profile flag
 */
extern JITBOOLEAN profile;

/**
 * @def DEBUG_LOG(format, args...)
 *
 * Print a debug message
 *
 * This macro behave like printf function. It also pretty print the given
 * message.
 *
 * @param format    a format specifier, like first argument of printf
 * @param args...   a variable argument list
 *
 * @note avaible only if DEBUG is defined
 */

/**
 * @def DEBUG_ENTER(function_name)
 *
 * Declare that we are entering in a function
 *
 * @param function_name the name of current function
 */

/**
 * @def DEBUG_EXIT(function_name)
 *
 * Declare that current function has been leaved
 *
 * Call this macro to close debug stream for given function.
 *
 * @param function_name the name of the function to leave
 *
 * @note avaible only if DEBUG is defined
 */
#ifdef DEBUG

  /* Debug macro implementation */
  #define DEBUG_LOG(format, args...) \
    fprintf(stderr, format, ## args); \
    fprintf(stderr, "\n")
    
  #define DEBUG_ENTER(function_name) \
    DEBUG_LOG("\nEntering function: %s", function_name)

  #define DEBUG_EXIT(function_name) \
    DEBUG_LOG("Exiting function: %s", function_name);

#else

  /* Dummy definition of debug macros for production code */
  #define DEBUG_LOG(format, args...)
  #define DEBUG_ENTER(function_name)
  #define DEBUG_EXIT(function_name)

#endif /* DEBUG */

/**
 * Emit profiler code
 *
 * This macro put the code given as argument in a conditional structure to
 * ensures that it is executed only when garbage collector is running in
 * profile mode.
 *
 * @param statement the instruction to generate
 */
#define GC_PROFILE(statement) \
  if(profile) { \
    statement; \
  }

/**
 * Setup timer environment
 */
#define GC_PROFILE_TIME() \
  struct timespec start_time;

/**
 * Start a timer if profiling is enabled
 */
#define GC_PROFILE_TIME_START() \
  GC_PROFILE(start_time = start_timer())

/**
 * Stop a timer if profiling is enabled
 *
 * Call this macro to stop a previous started timer and to add start time minus
 * end time to total variable.
 * 
 * @param total a pointer to a time container
 */
#define GC_PROFILE_TIME_END(total) \
  GC_PROFILE(stop_timer(&start_time, total))

/**
 * Start a timer by getting actual time
 *
 * @return actual time
 */
struct timespec start_timer(void);

/**
 * Stop a timer and update a time counter
 *
 * @param start_time timer start time
 * @param total      a time counter
 */
void stop_timer(struct timespec* start_time, JITFLOAT32* total);

/**
 * Alloc a chunk of memory
 *
 * This function is a simple wrapper around malloc routine. If something goes
 * wrong this process will be killed.
 *
 * @param size the size of memory to alloc
 * 
 * @return a pointer to new allocated memory
 */
void* xalloc(JITNUINT size);

/**
 * Alloc an array in memory
 *
 * This function is a simple wrapper around calloc routine. If something goes
 * wrong this process will be killed.
 *
 * @param count the number of elements to alloc
 * @param size  the array element size
 *
 * @return a pointer to new allocated array
 */
void* xcalloc(JITNUINT count, JITNUINT size);

/**
 * Rellocation routine
 *
 * This function is a simple wrapper around realloc system routine. If
 * something goes wrong this process will be killed.
 *
 * @param address  the address to realloc
 * @param new_size the requested memory size
 *
 * @return a pointer to allocated memory
 */
void* xrealloc(void* address, JITNUINT new_size);

/**
 * Deallocate a memory segment
 *
 * This function is a simple wrapper around free routine. After deallocation
 * memory is set to NULL.
 *
 * @param memory the memory segment to deallocate
 */
void xfree(void* memory);

/**
 * Alloc memory like xalloc
 *
 * Use this function instead of xalloc to avoid compiler warning when using
 * libxan data structures.
 *
 * @param size number of bytes to alloc, it must be >= 0
 *
 * @return a pointer to allocated memory
 */
void* xan_alloc(int size);

/**
 * Realloc memory like xrealloc
 *
 * Use thid function insted of xrealloc to avoid compiler warnings when using
 * libxan data structures.
 *
 * @param address  the address to realloc
 * @param new_size the requested memory size
 *
 * @return a pointer to allocated memory
 */
void* xan_realloc(void* address, int new_size);

#endif /* GC_COPY_UTILS_H */
