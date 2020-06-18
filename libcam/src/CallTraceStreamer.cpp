#include "CallTraceStreamer.h"
#include "call_trace_cfc_parser.h"
#include <cassert>

CallTraceStreamer::CallTraceStreamer() 
  : ctcl(CallTraceCfcLex())
  , parser(BZ2ParserState(
        "call_trace.txt.bz2"
      , calltracecfclex
      , calltracecfc_scan_buffer
      , calltracecfc_switch_to_buffer
      , calltracecfc_delete_buffer
      , &ctcl
    ))
{ 
}

int CallTraceStreamer::getFullEntry(CallTraceLoopInvocationGroup& clig){
  assert(ctcl.state == NUM_INVOC || ctcl.state == INVOC_PATTERN);
  ctcl.callEntry = &clig;
  ctcl.rstatus = ctcl.ACTIVE;
  ctcl.firstPass = false;
  while(1){
    int dataRemaining = parser.parseBlock();
    if(!dataRemaining)
      return 0;
    if(!ctcl.rstatus == ctcl.ACTIVE)
      break;
  }
  return 1;
}

int CallTraceStreamer::getNextEntry(CallTraceLoopInvocationGroup& clig){
  if(!getFullEntry(clig))
    return 0;

  while(ctcl.rstatus == ctcl.INVOC_INCOMPLETE){
    CallTraceLoopInvocationGroup c;
    getFullEntry(c);
    if(!clig.lastAndFirstInvocationOverlap(c)) abort();
    if(!clig.mergeIntoAndReturnRemaining(c).empty()) abort();
  }

  return 1;
}

pair<set<uintptr_t>, set<uintptr_t>> CallTraceStreamer::parseForInstructionLists(){
  set<uintptr_t> callList;
  set<uintptr_t> subInstrList;
  CallTraceLoopInvocationGroup c;

  ctcl.callEntry = &c;
  ctcl.callList = &callList;
  ctcl.subInstrList = &subInstrList;
  ctcl.firstPass = true;

  while(parser.parseBlock()){}
  return pair<set<uintptr_t>, set<uintptr_t>>(callList, subInstrList);
}
