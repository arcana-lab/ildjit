#ifndef INLINER_CONFIGURATION_H
#define INLINER_CONFIGURATION_H

#include <jitsystem.h>

extern JITUINT64 	threshold_insns_with_loops;
extern JITUINT64 	threshold_insns_without_loops;
extern JITUINT64	threshold_caller_insns;
extern JITUINT64	threshold_program_insns;
extern JITBOOLEAN 	disableLibraries;
extern JITBOOLEAN 	disableNotWorthFunctions;

#endif
