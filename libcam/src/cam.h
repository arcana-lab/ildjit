#ifndef CAM_H
#define CAM_H

#include <stdint.h>

#include <jitsystem.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uintptr_t inst_id_t;

/**
 * Global library functions.
 **/

typedef enum {CAM_MEMORY_PROFILE, CAM_LOOP_PROFILE} cam_mode_t;

// Init
void CAM_init (cam_mode_t mode);

// Shutdown
void CAM_shutdown (cam_mode_t mode);


/**
 * Memory address profiling.
 **/

// Register a memory reference
void CAM_mem(inst_id_t id, uintptr_t raddr1, uint64_t rlen1, uintptr_t raddr2, uint64_t rlen2, uintptr_t waddr, uint64_t wlen);

// Entry of the dumped file
// <inst id>
// N [ <loc>* ]			// Reads
// N [ <loc>* ] 		// Writes
// New line
//
// Each loc is: [a b length, startx, endx]


/**
 *
 * Loop and call profiling.
 *
 **/

// Register a loop invocation starting.
void CAM_profileLoopInvocationStart(JITNINT loopID);

// Register a loop invocation ending.
void CAM_profileLoopInvocationEnd(void);

// Register a loop iteration starting.
void CAM_profileLoopIterationStart(void);

// Register an instruction as being executed.
void CAM_profileLoopSeenInstruction(JITNINT instID);

// Register a call as being made.
void CAM_profileCallInvocationStart(JITNINT instID);

// Register a call as ending.
void CAM_profileCallInvocationEnd(void);

// Force the trace to write to disc and clear memory
void CAM_forceLoopTraceDump();
void CAM_forceMemTraceDump();

#ifdef __cplusplus
};
#endif

#endif
