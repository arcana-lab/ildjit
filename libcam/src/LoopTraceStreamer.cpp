#include "LoopTraceStreamer.h"
#include "loop_trace_cfc_parser.h"
#include <cassert>

LoopTraceStreamer::LoopTraceStreamer() 
  : ltcl(LoopTraceCfcLex())
  , parser(BZ2ParserState(
        "loop_trace.txt.bz2"
      , looptracecfclex
      , looptracecfc_scan_buffer
      , looptracecfc_switch_to_buffer
      , looptracecfc_delete_buffer
      , &ltcl
    ))
{ 
}

int LoopTraceStreamer::getFullEntry(LoopTraceEntry& lte){
  assert(ltcl.state == 0 || ltcl.state == 2);
  ltcl.loopEntry = &lte;
  ltcl.rstatus = ltcl.ACTIVE;
  ltcl.firstPass = false;
  while(1){
    int dataRemaining = parser.parseBlock();
    if(!dataRemaining)
      return 0;
    if(!ltcl.rstatus == ltcl.ACTIVE)
      break;
  }
  return 1;
}

int LoopTraceStreamer::getNextEntry(LoopTraceEntry& lte){
  if(!getFullEntry(lte))
    return 0;

  while(ltcl.rstatus == ltcl.INVOC_INCOMPLETE){
    LoopTraceEntry l;
    getFullEntry(l);
    if(!lte.lastAndFirstInvocationOverlap(l)) abort();
    if(!lte.mergeIntoAndReturnRemaining(l).empty()) abort();
  }

  return 1;
}

pair<set<uintptr_t>, uint64_t> LoopTraceStreamer::parseForInstructionList(){
  set<uintptr_t> instructionList;
  uint64_t numInvoc = 0;
  LoopTraceEntry lte;

  ltcl.loopEntry = &lte;
  ltcl.instructionList = &instructionList;
  ltcl.numInvoc = &numInvoc;
  ltcl.firstPass = true;

  while(parser.parseBlock()){}
  return pair<set<uintptr_t>, uint64_t>(instructionList, numInvoc+1);
}
