#ifndef LOOPTRACECFCLEX_H
#define LOOPTRACECFCLEX_H

#include <cstdint>
#include <stack>
#include <set>
#include "CompressionPattern.h"
class LoopTraceEntry;

struct LoopTraceCfcLex{
  int state;
  enum ReturnStatus { ACTIVE, INVOC_COMPLETE, INVOC_INCOMPLETE };
  ReturnStatus rstatus;
  int cfc_state;
  int reppatState;
  uint64_t invocNum;
  uint64_t remaining_invoc_entries;
  uint64_t remaining_iter_entries;
  uint64_t start_invoc;
  uint64_t end_invoc;
  uintptr_t currInstruction;
  uint64_t start_iter;
  uint64_t end_iter;
  uint64_t num_dyn_inst;
  stack<CompressionPattern> patternStack;
  LoopTraceEntry *loopEntry;
  bool firstPass;
  set<uintptr_t> *instructionList;
  uint64_t *numInvoc;
  LoopTraceCfcLex() : state(0), rstatus(ACTIVE), cfc_state(0), reppatState(0), remaining_invoc_entries(0), 
    remaining_iter_entries(0), start_invoc(0), end_invoc(0) {}
};

#endif
