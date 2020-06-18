#ifndef CALLTRACESTREAMER_H
#define CALLTRACESTREAMER_H

#include "BZ2ParserState.h"
#include "CallTraceCfcLex.h"

class CallTraceStreamer{
  CallTraceCfcLex ctcl;
  BZ2ParserState parser;

  int getFullEntry(CallTraceLoopInvocationGroup& clig);

public:
  CallTraceStreamer();

  /* Returns 1 if a valid CallTraceLoopInvocationGroup was filled, 0 otherwise */
  int getNextEntry(CallTraceLoopInvocationGroup& clig);

  /* Returns lists of call ids and sub-instruction ids */
  pair<set<uintptr_t>, set<uintptr_t>> parseForInstructionLists();
};

#endif
