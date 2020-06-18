#include "MemoryTraceStreamer.h"
#include <cassert>

MemoryTraceStreamer::MemoryTraceStreamer(uintptr_t _instrID, string filename, bool _write)
  : instrID(_instrID)
  , mtl(MemoryTraceLex())
  , parser(BZ2ParserState(
        filename.c_str()
      , memorytracelex
      , memorytrace_scan_buffer
      , memorytrace_switch_to_buffer
      , memorytrace_delete_buffer
      , &mtl
    ))
  , write(_write)
  , status(1)
{
}

uint64_t MemoryTraceStreamer::getNumBufferedInstances(MemSet& memset){
  if(memset.size() > 0)
    return memset.back().getEnd() -  memset.front().getStart() + 1;
  else
    return 0;
}

MemSet MemoryTraceStreamer::getNextChunk(uint64_t numInstances, bool allowOvershoot){
  MemSet chunk;

  if(numInstances == 0)
    return chunk;

  /* Copy the remainder into chunk */
  copy(remainder.begin(), remainder.end(), back_inserter(chunk));
  remainder.clear();

  /* Set up the lexer state */
  mtl.memset = &chunk;
  mtl.numInstancesRequired = numInstances;

  while(getNumBufferedInstances(chunk) < numInstances && status){
    status = parser.parseBlock();
  }

  /* The stream has been exhausted */
  if(chunk.empty())
    return chunk;

  /* Find the entry which contains the final instance to be returned */
  uint64_t firstInstance = chunk.front().getStart();
  uint64_t lastInstance = firstInstance + numInstances - 1;
  auto compare = [] (const MemSetEntry& entry, uint64_t val) { return const_cast<MemSetEntry&>(entry).getEnd() < val; };
  MemSet::iterator straddleEntry = lower_bound(chunk.begin(), chunk.end(), lastInstance, compare);

  /* Split the straddleEntry if necessary and copy and entries in excess into remainder */
  if(straddleEntry != chunk.end()){
    if(straddleEntry->getEnd() > lastInstance)
      remainder.push_back(straddleEntry->splitOffEnd(straddleEntry->getEnd() - lastInstance));
    if(next(straddleEntry,1) != chunk.end()){
      copy(next(straddleEntry, 1), chunk.end(), back_inserter(remainder));
      chunk.erase(next(straddleEntry, 1), chunk.end());
    }
  }

#if 0
  /* Split the memset into chunk and remainder */
  MemSet remainder;
  for(auto it = memset.begin(); it != memset.end(); it++){
    if(it < straddleEntry){
      chunk.push_back(*it);
    }
    else if (it == straddleEntry){
      if(it->getEnd() > lastInstance){
        chunk.push_back(it->getSlice(it->getStart(), lastInstance));
        remainder.push_back(it->getSlice(lastInstance+1, it->getEnd()));
      }
      else{
        chunk.push_back(*it);
      }
    }
    else{
      remainder.push_back(*it);
    }
  }
#endif

  /* Set mtl.startInstance */
  mtl.startInstance = chunk.back().getEnd() + 1;

  assert(allowOvershoot || getNumBufferedInstances(chunk) == numInstances);

  return chunk;
}

uintptr_t MemoryTraceStreamer::getID(){ return instrID; }
bool MemoryTraceStreamer::isWrite(){ return write; }
