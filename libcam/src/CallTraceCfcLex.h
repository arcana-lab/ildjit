#ifndef CALLTRACECFCLEX_H
#define CALLTRACECFCLEX_H

#include <cstdint>
#include <stack>
#include <set>
#include "CompressionPattern.h"
#include "CallTrace.h"

enum State { NUM_INVOC, INVOC_PATTERN, NUM_CALLS, CALLID, NUM_INSTANCES, INSTANCE_PATTERN, CFC, DUMPEND };

struct CallTraceCfcLex{
  enum ReturnStatus { ACTIVE, INVOC_COMPLETE, INVOC_INCOMPLETE };
  ReturnStatus rstatus;
  State state;
  int cfc_state;
  int reppatState;
  uint64_t invocNum;
  uint64_t remaining_callinstance_entries;
  uint64_t remaining_loopinvoc_entries;
  uint64_t remaining_call_entries;
  uint64_t start_invoc;
  uint64_t end_invoc;
  uintptr_t currCallID;
  bool firstPass;
  set<uintptr_t>* callList;
  set<uintptr_t>* subInstrList;
  CallTraceLoopInvocationGroup* callEntry;
  stack<CompressionPattern> patternStack;
  CallTraceCfcLex() : rstatus(ACTIVE), state(NUM_INVOC), cfc_state(0), reppatState(0), remaining_callinstance_entries(0), 
    remaining_loopinvoc_entries(0), remaining_call_entries(0), start_invoc(0), end_invoc(0), currCallID(0) {}
};

#endif
