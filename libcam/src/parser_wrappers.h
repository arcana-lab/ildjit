#ifndef PARSER_WRAPPERS_H
#define PARSER_WRAPPERS_H

#include "static_inst_rec.h"
#include "StaticLoopRec.h"
#include "CallTrace.h"
#include <set>
using namespace std;

#define DEP_RAW         0x1
#define DEP_WAR         0x2
#define DEP_WAW         0x4
#define DEP_MRAW        0x8
#define DEP_MWAR        0x10
#define DEP_MWAW        0x20

/* Loop trace */
void loopTraceInit();
pair<set<uintptr_t>, uint64_t> parseLoopTraceForInstrList();
int loopTraceGetNextEntry(LoopTraceEntry& loopEntry);

/* Call trace */
void callTraceInit();
StreamParseCallTrace parseCallTrace();
pair<set<uintptr_t>, set<uintptr_t>> parseCallTraceForInstrList();
int callTraceGetNextEntry(CallTraceLoopInvocationGroup& callEntry);

/* Memory trace */
MemoryTrace parse_memory_trace();
MemoryTrace parse_memory_trace_parallel();
pair<vector<uintptr_t>, vector<uintptr_t>> findMemoryTraces();

/* DDG */
pair<set<pair<uintptr_t, uintptr_t>>, set<pair<uintptr_t, uintptr_t>>> parse_dependence_pairs();
vector<tuple<uintptr_t, uintptr_t, unsigned int>> parse_static_ddg();

#endif
