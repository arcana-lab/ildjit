#ifndef CHIARA_PROFILE_H
#define CHIARA_PROFILE_H

/**
 * Profile the exits from the given loop.  If a loop successor has a non-loop
 * predecessor then the profile calls will be inserted in a new basic block
 * that is created along the edge between loop instructions and the successor.
 */
void
LOOPS_profileLoopExits(ir_method_t *method, loop_t *loop, char *exitFuncName, void *exitFunc);


#endif  /* ifndef CHIARA_PROFILE_H */
