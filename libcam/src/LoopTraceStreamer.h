#ifndef LOOPTRACESTREAMER_H
#define LOOPTRACESTREAMER_H

#include "BZ2ParserState.h"
//#include "parser_wrappers.h"
#include "LoopTraceCfcLex.h"

class LoopTraceStreamer{
  LoopTraceCfcLex ltcl;
  BZ2ParserState parser;

  int getFullEntry(LoopTraceEntry& lte);

public:
  LoopTraceStreamer();

  /* Returns 1 if a valid LoopTraceEntry was filled, 0 otherwise */
  int getNextEntry(LoopTraceEntry& lte);

  /* Returns a list of instructions and the number of invocations */
  pair<set<uintptr_t>, uint64_t> parseForInstructionList();
};

#endif
