#ifndef MEMORYTRACESTREAMER_H
#define MEMORYTRACESTREAMER_H

#include <map>
#include <vector>
#include "static_inst_rec.h"
#include "parser_wrappers.h"
#include "BZ2ParserState.h"
#include "memory_trace_parser.h"

class MemoryTraceStreamer{
  uintptr_t instrID;
  MemSet remainder;
  MemoryTraceLex mtl;
  BZ2ParserState parser;
  bool write;
  uint64_t lastReturnedInstance;
  int status;

  uint64_t getNumBufferedInstances(MemSet& memset);

public:
  MemoryTraceStreamer(uintptr_t _instrID, string filename, bool _write);

  /* Parse enough of the trace to get the specified number of instances */ 
  MemSet getNextChunk(uint64_t numInstances, bool allowOvershoot=false);

  uintptr_t getID();

  bool isWrite();

};
#endif
