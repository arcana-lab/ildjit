/*
 * Copyright (C) 2013 - 2014  Campanoni Simone
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <assert.h>
#include <ir_optimization_interface.h>
#include <iljit-utils.h>
#include <jitsystem.h>
#include <ir_language.h>
#include <compiler_memory_manager.h>
#include <platform_API.h>
#include <chiara.h>
#include <ir_method.h>

// My headers
#include <optimizer_scalarization.h>
#include <config.h>
// End

#define INFORMATIONS 	"This is a scalarization plugin"
#define	AUTHOR		"Simone Campanoni and Timothy M. Jones"
#define JOB            	SCALARIZATION

ir_lib_t	*irLib		= NULL;
ir_optimizer_t	*irOptimizer	= NULL;
char 	        *prefix		= NULL;


/**
 * Macros for debug printing.
 **/
#if defined(PRINTDEBUG)
JITINT32 debugLevel = -1;
#endif


/**
 * A global variable.
 **/
typedef struct global_t
{
  JITUINT32 ID;
  ir_item_t base;
  ir_item_t offset;
} global_t;


/**
 * An edge between two instructions.
 **/
typedef struct edge_t
{
  ir_instruction_t *pred;
  ir_instruction_t *succ;
} edge_t;


/**
 * A usage region for a global.  This records the edges into and out of the
 * region.  Within the region the global can be considered to be within a
 * local variable.
 **/
typedef struct usage_region_t
{
  XanList *entryEdges;
  XanList *exitEdges;
} usage_region_t;


/**
 * Information about each global that is found.
 **/
typedef struct global_info_t
{
  ir_item_t *local;
  ir_item_t *guard;
  XanList *loads;
  XanList *stores;
  XanList *accesses;
  XanList *dependences;
} global_info_t;

/**
 * Information for the entire pass.
 **/
typedef struct pass_info_t
{
  JITUINT32 numInsts;
  JITUINT32 numGlobals;
  ir_method_t *method;
  XanHashTable *globals;
  XanHashTable *globalInfo;
  XanHashTable *idToGlobal;
  XanHashTable *instToGlobal;
  XanHashTable *origInstIDs;
  usage_region_t *usageRegions;
  XanList **predecessors;
  XanList **successors;
} pass_info_t;


#define itemToPtr(val) ((void *)(JITNINT)(val))
#define uintToPtr(val) ((void *)(JITNUINT)(val))
#define ptrToUint32(val) ((JITUINT32)(JITNUINT)(val))

#define origID(info, inst) ((ptrToUint32(xanHashTable_lookup(info->origInstIDs, inst))))


/**
 * Allocate a new list.
 **/
static inline XanList *
newList(void)
{
  return xanList_new(allocFunction, freeFunction, NULL);
}


/**
 * Allocate a new hash table.
 **/
static inline XanHashTable *
newHashTable(void)
{
  return xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
}


/**
 * Create an information structure for this pass.  Also add a nop as the first
 * instruction in the method, in case the actual first instruction accesses a
 * global that should be localised.
 **/
static pass_info_t *
initialisePassInfo(ir_method_t *method)
{
  JITUINT32 i;
  pass_info_t *info = allocFunction(sizeof(pass_info_t));
  if (IRMETHOD_getMethodParametersNumber(method) == 0) {
    IRMETHOD_newInstructionOfTypeBefore(method, IRMETHOD_getInstructionAtPosition(method, 0), IRNOP);
  }
  info->numInsts = IRMETHOD_getInstructionsNumber(method);
  info->numGlobals = 0;
  info->method = method;
  info->globals = newHashTable();
  info->globalInfo = newHashTable();
  info->idToGlobal = newHashTable();
  info->instToGlobal = newHashTable();
  info->origInstIDs = newHashTable();
  info->predecessors = IRMETHOD_getInstructionsPredecessors(method);
  info->successors = IRMETHOD_getInstructionsSuccessors(method);
  for (i = 0; i < info->numInsts; ++i) {
    xanHashTable_insert(info->origInstIDs, IRMETHOD_getInstructionAtPosition(method, i), uintToPtr(i));
  }
  IROPTIMIZER_callMethodOptimization(irOptimizer, method, LIVENESS_ANALYZER);
  IROPTIMIZER_callMethodOptimization(irOptimizer, method, REACHING_DEFINITIONS_ANALYZER);
  return info;
}


/**
 * Free a global information structure.
 **/
static void
freeGlobalInfo(global_info_t *ginfo)
{
  freeFunction(ginfo->local);
  freeFunction(ginfo->guard);
  xanList_destroyList(ginfo->loads);
  xanList_destroyList(ginfo->stores);
  xanList_destroyList(ginfo->accesses);
  xanList_destroyList(ginfo->dependences);
  freeFunction(ginfo);
}


/**
 * Free memory used by the pass information structure.  Also remove the first
 * nop that was added when the structure was initialised.
 **/
static void
freePassInfo(pass_info_t *info)
{
  XanHashTableItem *gItem;
  xanHashTable_destroyTableAndData(info->globals);
  xanHashTable_destroyTable(info->idToGlobal);
  xanHashTable_destroyTable(info->instToGlobal);
  xanHashTable_destroyTable(info->origInstIDs);
  gItem = xanHashTable_first(info->globalInfo);
  while (gItem) {
    freeGlobalInfo(gItem->element);
    gItem = xanHashTable_next(info->globalInfo, gItem);
  }
  xanHashTable_destroyTable(info->globalInfo);
  if (info->usageRegions) {
    JITUINT32 g;
    for (g = 0; g < info->numGlobals; ++g) {
      if (info->usageRegions[g].entryEdges != NULL) {
        xanList_destroyListAndData(info->usageRegions[g].entryEdges);
        xanList_destroyListAndData(info->usageRegions[g].exitEdges);
      }
    }
    freeFunction(info->usageRegions);
  }
  IRMETHOD_destroyInstructionsPredecessors(info->predecessors, info->numInsts);
  IRMETHOD_destroyInstructionsSuccessors(info->successors, info->numInsts);
  if (IRMETHOD_getMethodParametersNumber(info->method) == 0) {
    assert(IRMETHOD_getInstructionAtPosition(info->method, 0)->type == IRNOP);
    IRMETHOD_deleteInstruction(info->method, IRMETHOD_getInstructionAtPosition(info->method, 0));
  }
  freeFunction(info);
}


/**
 * Get the global variable loaded or stored by an instruction, if any.
 **/
static global_t *
getGlobalVariableAccessed(pass_info_t *info, ir_instruction_t *inst)
{
  global_t *global = xanHashTable_lookup(info->instToGlobal, inst);
  if (global == NULL) {
    ir_item_t *base = IRMETHOD_getInstructionParameter1(inst);
    if (IRDATA_isAGlobalVariable(base)) {
      if (inst->type == IRLOADREL || inst->type == IRSTOREREL) {
        ir_item_t *offset = IRMETHOD_getInstructionParameter2(inst);
        XanHashTable *gTable;

//        ir_method_t *m = IRDATA_getInitializationMethodOfILTypeFromIRItem(base);
//        fprintf(stderr, "XAN %s\n", m != NULL ? IRMETHOD_getSignatureInString(m) : "Nothing");

        /* See whether there is an existing global. */
        gTable = xanHashTable_lookup(info->globals, itemToPtr(base->value.v)); 
        if (!gTable) {
          gTable = newHashTable();
          xanHashTable_insert(info->globals, itemToPtr(base->value.v), gTable);
        } else {
          global = xanHashTable_lookup(gTable, itemToPtr(offset->value.v));
        }

        /* Create a new global if necessary. */
        if (!global) {
          global = allocFunction(sizeof(global_t));
          global->ID = info->numGlobals;
          memcpy(&global->base, base, sizeof(ir_item_t));
          memcpy(&global->offset, offset, sizeof(ir_item_t));
          xanHashTable_insert(gTable, itemToPtr(offset->value.v), global);
          xanHashTable_insert(info->idToGlobal, uintToPtr(global->ID), global);
          info->numGlobals += 1;
          PDEBUG(1, "New global (");
          PDEBUGSTMT(1, IRMETHOD_dumpSymbol((ir_symbol_t *)(JITNUINT)base->value.v, stderr));
          PDEBUGC(1, ", %llu), id %u\n", offset->value.v, global->ID);
        }

        /* Remember this mapping. */
        xanHashTable_insert(info->instToGlobal, inst, global);
      } else {
        abort();
      }
    }
  }
  return global;
}


/**
 * Create a new list, if it doesn't already exist, then add an instruction.
 **/
static void
addToInstListUnique(XanList *instList, ir_instruction_t *inst)
{
  if (!xanList_find(instList, inst)) {
    xanList_append(instList, inst);
  }
}


/**
 * Mark an instruction as accessing a global variable.  If the global hasn't
 * been seen before, it creates an information structure for it first.  Then
 * this structure is updated.
 **/
static void
markAccessToGlobal(pass_info_t *info, global_t *global, ir_instruction_t *inst)
{
  global_info_t *ginfo = xanHashTable_lookup(info->globalInfo, global);
  if (!ginfo) {
    ginfo = allocFunction(sizeof(global_info_t));
    ginfo->local = allocFunction(sizeof(ir_item_t));
    if (inst->type == IRLOADREL){
      memcpy(ginfo->local, IRMETHOD_getInstructionResult(inst), sizeof(ir_item_t));
    } else {
      memcpy(ginfo->local, IRMETHOD_getInstructionParameter3(inst), sizeof(ir_item_t));
      ginfo->local->type = IROFFSET;
    }
    ginfo->local->value.v = IRMETHOD_newVariableID(info->method);
    ginfo->guard = allocFunction(sizeof(ir_item_t));
    ginfo->guard->type = IROFFSET;
    ginfo->guard->internal_type = IRINT32;
    ginfo->guard->value.v = IRMETHOD_newVariableID(info->method);
    ginfo->guard->type_infos = NULL;
    ginfo->loads = newList();
    ginfo->stores = newList();
    ginfo->accesses = newList();
    ginfo->dependences = newList();
    xanHashTable_insert(info->globalInfo, global, ginfo);
  }
  xanList_append(ginfo->accesses, inst);
  if (inst->type == IRSTOREREL){
    xanList_append(ginfo->stores, inst);
  } else {
    xanList_append(ginfo->loads, inst);
  }
}


/**
 * Identify global variables that are loaded and stored in the method.
 **/
static void
identifyAccessesToGlobals(pass_info_t *info)
{
  JITINT32 i;
  for (i = 0; i < info->numInsts; ++i) {
    ir_instruction_t *inst = IRMETHOD_getInstructionAtPosition(info->method, i);
    global_t *global = NULL;
    switch (inst->type) {
    case IRLOADREL:
    case IRSTOREREL:
      global = getGlobalVariableAccessed(info, inst);
      if (global) {
        markAccessToGlobal(info, global, inst);
        PDEBUG(3, "%s inst %u accesses global %u\n", inst->type == IRLOADREL ? "Load" : "Store", inst->ID, global->ID);
      }
      break;
    }
  }
}


/**
 * Filter out globals that aren't interesting based on their accesses.  For the
 * moment this just means making sure there are at least 2 accesses to it.
 * Maybe this should be done elsewhere, since these checks could pass looking
 * at the whole method, but may not be true if you just look at one path in the
 * CFG.
 **/
static void
filterGlobalsWithFewAccesses(pass_info_t *info)
{
  XanList *toRemove = newList();
  XanListItem *removeItem;
  XanHashTableItem *tableItem;

  /* Search for globals with few accesses. */
  /* fprintf(stderr, "Warning: Filtering globals based on ID\n"); */
  tableItem = xanHashTable_first(info->globalInfo);
  while (tableItem) {
    global_t *global = tableItem->elementID;
    global_info_t *ginfo = tableItem->element;
    if (xanList_length(ginfo->accesses) < 2) {
      xanList_append(toRemove, global);
    /* } else if (global->ID > 12) { */
    /*   xanList_append(toRemove, global); */
    }
    tableItem = xanHashTable_next(info->globalInfo, tableItem);
  }

  /* Remove the globals we don't want to deal with. */
  removeItem = xanList_first(toRemove);
  while (removeItem) {
    global_t *global = removeItem->data;
    XanHashTable *gtable = xanHashTable_lookup(info->globals, itemToPtr(global->base.value.v));
    xanHashTable_removeElement(gtable, itemToPtr(global->offset.value.v));
    freeGlobalInfo(xanHashTable_lookup(info->globalInfo, global));
    xanHashTable_removeElement(info->globalInfo, global);
    removeItem = removeItem->next;
  }

  /* Clean up. */
  xanList_destroyList(toRemove);
}


/**
 * Identify points in the CFG where each global variable must stored back to
 * memory.  These are just before instructions that have a memory data
 * dependence with one of the instructions that accesses the global.
 **/
static void
identifyMemoryDependenceInsts(pass_info_t *info)
{
  XanHashTableItem *globalItem = xanHashTable_first(info->globalInfo);
  while (globalItem) {
    global_t *accGlobal = globalItem->elementID;
    global_info_t *ginfo = globalItem->element;
    XanListItem *instItem = xanList_first(ginfo->accesses);
    PDEBUG(2, "Global %u\n", accGlobal->ID);
    while (instItem) {
      ir_instruction_t *accInst = instItem->data;
      XanList *depList = IRMETHOD_getDataDependencesToInstruction(info->method, accInst);
      PDEBUG(3, "  Access inst %u\n", accInst->ID);
      if (depList) {
        XanListItem *depItem = xanList_first(depList);
        while (depItem) {
          data_dep_arc_list_t *dep = depItem->data;
          global_t *depGlobal = xanHashTable_lookup(info->instToGlobal, dep->inst);
          if (depGlobal != accGlobal && IRMETHOD_isAMemoryDataDependenceType(dep->depType)) {
            addToInstListUnique(ginfo->dependences, dep->inst);
            PDEBUG(4, "    Memory dependence with inst %u\n", dep->inst->ID);
          }
          depItem = depItem->next;
        }
        xanList_destroyList(depList);
      }
      instItem = instItem->next;
    }
    globalItem = xanHashTable_next(info->globalInfo, globalItem);
  }
}


/**
 * Identify the gen and kill sets for global usage.
 **/
static void
initUsageGenKillSets(pass_info_t *info, data_flow_t *usage)
{
  JITUINT32 i, j;
  JITUINT32 numParams;
  XanHashTableItem *globalItem;
  JITUINT32 blockBits = sizeof(JITNUINT) * 8;
  globalItem = xanHashTable_first(info->globalInfo);
  while (globalItem) {
    global_t *global = globalItem->elementID;
    global_info_t *ginfo = globalItem->element;
    XanListItem *instItem;

    /* Generate when accessed. */
    instItem = xanList_first(ginfo->accesses);
    while (instItem) {
      ir_instruction_t *inst = instItem->data;
      JITUINT32 block = global->ID / blockBits;
      assert(block <  usage->elements);
      usage->data[inst->ID].gen[block] |= 0x1 << (global->ID % blockBits);
      instItem = instItem->next;
    }

    /* Kill if there's a dependence. */
    instItem = xanList_first(ginfo->dependences);
    while (instItem) {
      ir_instruction_t *inst = instItem->data;
      JITUINT32 block = global->ID / blockBits;
      assert(block <  usage->elements);
      usage->data[inst->ID].kill[block] |= 0x1 << (global->ID % blockBits);
      instItem = instItem->next;
    }
    globalItem = xanHashTable_next(info->globalInfo, globalItem);
  }

  /* Kill if a return, parameter nop or first instruction. */
  numParams = IRMETHOD_getMethodParametersNumber(info->method);
  for (i = 0; i < info->numInsts; ++i) {
    ir_instruction_t *inst = IRMETHOD_getInstructionAtPosition(info->method, i);
    if (inst->type == IRRET || i == 0 || i < numParams) {
      for (j = 0; j < usage->elements; ++j) {
        usage->data[i].kill[j] = ~0;
      }
    }
  }
}


/**
 * Initialise the in and out sets for global usage.
 **/
static void
initUsageInOutSets(pass_info_t *info, data_flow_t *usage)
{
  JITUINT32 i, j;
  for (i = 0; i < info->numInsts; ++i) {
    for (j = 0; j < usage->elements; ++j) {
      usage->data[i].in[j] = 0;
      usage->data[i].out[j] = 0;
    }
  }
}


/**
 * Compute a data flow step to identify global usage regions.
 **/
static JITBOOLEAN
computeUsageDataFlowStep(pass_info_t *info, data_flow_t *usage)
{
  JITUINT32 i, j;
  JITBOOLEAN modified = JITFALSE;
  for (i = 0; i < usage->num; ++i) {
    for (j = 0; j < usage->elements; ++j) {
      JITNUINT old = usage->data[i].in[j];
      XanListItem *item;

      /* Get a new in set from predecessors. */
      item = xanList_first(info->predecessors[i]);
      while (item) {
        ir_instruction_t *pred = item->data;
        usage->data[i].in[j] |= usage->data[pred->ID].out[j];
        item = item->next;
      }

      /* Check for changes. */
      if (usage->data[i].in[j] != old) {
        modified = JITTRUE;
        /* PDEBUG(2, "  In set %u for inst %u was %u, now %u\n", j, i, old, usage->data[i].in[j]); */
      }

      /* Get a new out set from successors. */
      old = usage->data[i].out[j];
      item = xanList_first(info->successors[i]);
      while (item) {
        ir_instruction_t *succ = item->data;
        if (succ->type != IREXITNODE) {
          usage->data[i].out[j] |= usage->data[succ->ID].in[j];
        }
        item = item->next;
      }

      /* Check for changes. */
      if (usage->data[i].out[j] != old) {
        modified = JITTRUE;
        /* PDEBUG(2, "  Out set %u for inst %u was %u, now %u\n", j, i, old, usage->data[i].out[j]); */
      }

      /* Compute the new in set (only add things). */
      old = usage->data[i].in[j];
      usage->data[i].in[j] |= usage->data[i].gen[j] | (usage->data[i].out[j] & ~(usage->data[i].kill[j]));
      if (usage->data[i].in[j] != old) {
        modified = JITTRUE;
        /* PDEBUG(2, "  In set %u for inst %u was %u, now %u\n", j, i, old, usage->data[i].in[j]); */
      }

      /* Compute the new out set (only add things). */
      old = usage->data[i].out[j];
      usage->data[i].out[j] |= usage->data[i].gen[j] | (usage->data[i].in[j] & ~(usage->data[i].kill[j]));
      if (usage->data[i].out[j] != old) {
        modified = JITTRUE;
        /* PDEBUG(2, "  Out set %u for inst %u was %u, now %u\n", j, i, old, usage->data[i].out[j]); */
      }
    }
  }
  return modified;
}


/**
 * Create a new edge and add it to a list.
 **/
static void
createEdgeInList(ir_instruction_t *pred, ir_instruction_t *succ, XanList *list)
{
  edge_t *edge = allocFunction(sizeof(edge_t));
  edge->pred = pred;
  edge->succ = succ;
  xanList_append(list, edge);
}


/**
 * Create usage regions for each global.  Each usage region is a list of edges
 * into the region and another list out.
 **/
static void
createUsageRegionsFromDataFlow(pass_info_t *info, data_flow_t *usage)
{
  JITUINT32 i, j, k;
  info->usageRegions = allocFunction(info->numGlobals * sizeof(usage_region_t));
  for (i = 0; i < usage->num; ++i) {
    for (j = 0; j < usage->elements; ++j) {
      if (usage->data[i].kill[j] != 0 && (usage->data[i].in[j] != 0 || usage->data[i].out[j] != 0)) {
        JITUINT32 base = j * sizeof(JITNUINT) * 8;
        ir_instruction_t *inst = IRMETHOD_getInstructionAtPosition(info->method, i);
        for (k = 0; k < sizeof(JITNUINT) * 8 && base + k < info->numGlobals; ++k) {
          JITUINT32 bit = 0x1 << k;
          if ((usage->data[i].kill[j] & bit) == bit) {

            /* Create the region if necessary. */
            if (info->usageRegions[base + k].entryEdges == NULL) {
              info->usageRegions[base + k].entryEdges = newList();
              info->usageRegions[base + k].exitEdges = newList();
            }

            /* This instruction marks the end of a region. */
            if ((usage->data[i].in[j] & bit) == bit) {
              XanListItem *predItem = xanList_first(info->predecessors[i]);
              while (predItem) {
                ir_instruction_t *pred = predItem->data;
                createEdgeInList(pred, inst, info->usageRegions[base + k].exitEdges);
                PDEBUG(2, "  Edge %u - %u is region exit for global %u\n", pred->ID, inst->ID, base + k);
                predItem = predItem->next;
              }
            }

            /* This instruction marks the start of a region. */
            if ((usage->data[i].out[j] & bit) == bit) {
              XanListItem *succItem = xanList_first(info->successors[i]);
              while (succItem) {
                ir_instruction_t *succ = succItem->data;
                if (succ->type != IREXITNODE) {
                  createEdgeInList(inst, succ, info->usageRegions[base + k].entryEdges);
                  PDEBUG(2, "  Edge %u - %u is region entry for global %u\n", inst->ID, succ->ID, base + k);
                }
                succItem = succItem->next;
              }
            }
          }
        }
      }
    }
  }
}


/**
 * Identify regions within the method where globals are currently used.  All
 * loads and stores that access a global are contained in a single region,
 * although there can be multiple regions for each global.  Regions start with
 * the first instruction that accesses the global and end with the last
 * instruction to access it, or an instruction that has a memory data
 * dependence with one of the accessing instructions.  We use a two-way data
 * flow analysis to determine these regions.
 **/
static void
identifyUsageRegions(pass_info_t *info)
{
  JITBOOLEAN modified = JITTRUE;
  data_flow_t *usage;

  /* Allocate memory. */
  PDEBUG(1, "Identifying global usage regions\n");
  usage = DATAFLOW_allocateSets(info->numInsts, info->numGlobals);

  /* Initialise the gen and kill sets. */
  initUsageGenKillSets(info, usage);

  /* Initialise the in and out sets. */
  initUsageInOutSets(info, usage);

  /* Compute the usage regions. */
  while (modified) {
    modified = computeUsageDataFlowStep(info, usage);
  }

  /* Create the regions. */
  createUsageRegionsFromDataFlow(info, usage);

  /* Clean up. */
  DATAFLOW_destroySets(usage);
}


#if 0
/**
 * A call back to print a value, for debugging.
 **/
static void
printValue(ir_instruction_t *inst, JITUINT32 val)
{
  fprintf(stderr, "Inst %u, value %x\n", inst->ID, val);
  fflush(stderr);
}


/**
 * A call back to check two values, for debugging.
 **/
static void
checkValues(ir_instruction_t *inst, JITUINT32 v1, JITUINT32 v2)
{
  if (v1 != v2) {
    fprintf(stderr, "Inst %u mis-match %x vs %x\n", inst->ID, v1, v2);
    fflush(stderr);
    abort();
  } else {
    fprintf(stderr, "Inst %u match\n", inst->ID);
    fflush(stderr);
  }
}


/**
 * Insert a native call back before an instruction for debugging.
 **/
static void
insertDebuggingNativeCall(pass_info_t *info, ir_instruction_t *succ, ir_item_t *value, JITBOOLEAN before)
{
  ir_instruction_t *inst;
  ir_item_t param;
  if (before) {
    inst = IRMETHOD_newNativeCallInstructionBefore(info->method, succ, "printValue", printValue, NULL, NULL);
  } else {
    inst = IRMETHOD_newNativeCallInstructionAfter(info->method, succ, "printValue", printValue, NULL, NULL);
  }
  param.type = param.internal_type = IRINT32;
  param.value.v = (JITNUINT)inst;
  param.type_infos = NULL;
  IRMETHOD_addInstructionCallParameter(info->method, inst, &param);
  IRMETHOD_addInstructionCallParameter(info->method, inst, value);
}


/**
 * Insert a native call back to check two variables.
 **/
static ir_instruction_t *
insertDebuggingCheckVariablesCall(pass_info_t *info, ir_instruction_t *around, ir_item_t *v1, ir_item_t *v2, JITBOOLEAN before)
{
  ir_instruction_t *inst;
  ir_item_t param;
  if (before) {
    inst = IRMETHOD_newNativeCallInstructionBefore(info->method, around, "checkValues", checkValues, NULL, NULL);
  } else {
    inst = IRMETHOD_newNativeCallInstructionAfter(info->method, around, "checkValues", checkValues, NULL, NULL);
  }
  param.type = param.internal_type = IRINT32;
  param.value.v = (JITNUINT)inst;
  param.type_infos = NULL;
  IRMETHOD_addInstructionCallParameter(info->method, inst, &param);
  IRMETHOD_addInstructionCallParameter(info->method, inst, v1);
  IRMETHOD_addInstructionCallParameter(info->method, inst, v2);
  return inst;
}


/**
 * Insert a debugging store to write a value back to memory.
 **/
static ir_instruction_t *
insertDebuggingStoreValue(pass_info_t *info, ir_instruction_t *around, ir_item_t *base, ir_item_t *offset, ir_item_t *value, JITBOOLEAN before)
{
  ir_instruction_t *store;
  if (before) {
    store = IRMETHOD_newInstructionOfTypeBefore(info->method, around, IRSTOREREL);
  } else {
    store = IRMETHOD_newInstructionOfTypeAfter(info->method, around, IRSTOREREL);
  }
  IRMETHOD_cpInstructionParameter1(store, base);
  IRMETHOD_cpInstructionParameter2(store, offset);
  IRMETHOD_cpInstructionParameter3(store, value);
  return store;
}


/**
 * Insert a debugging load to record a location from memory.
 **/
static ir_instruction_t *
insertDebuggingLoadValue(pass_info_t *info, ir_instruction_t *around, ir_item_t *base, ir_item_t *offset, JITBOOLEAN before)
{
  ir_instruction_t *load;
  ir_item_t result;
  if (before) {
    load = IRMETHOD_newInstructionOfTypeBefore(info->method, around, IRLOADREL);
  } else {
    load = IRMETHOD_newInstructionOfTypeAfter(info->method, around, IRLOADREL);
  }
  IRMETHOD_newVariable(info->method, &result, IRINT32, NULL);
  IRMETHOD_cpInstructionParameter1(load, base);
  IRMETHOD_cpInstructionParameter2(load, offset);
  IRMETHOD_cpInstructionResult(load, &result);
  return load;
}


/**
 * Insert a load from memory and then check the value against a variable.
 **/
static ir_instruction_t *
insertDebuggingLoadCheckValue(pass_info_t *info, ir_instruction_t *around, ir_item_t *base, ir_item_t *offset, ir_item_t *check, JITBOOLEAN before)
{
  ir_instruction_t *load = insertDebuggingLoadValue(info, around, base, offset, before);
  return insertDebuggingCheckVariablesCall(info, load, IRMETHOD_getInstructionResult(load), check, JITFALSE);
}


/**
 * Insert debugging call backs round each non-label instruction to check a
 * memory location to see whether it has diverged from a variable.
 **/
static void
insertDebuggingCheckAll(pass_info_t *info, ir_item_t *base, ir_item_t *offset, ir_item_t *check)
{
  XanHashTable *seenInsts = newHashTable();
  JITUINT32 i, numInsts = IRMETHOD_getInstructionsNumber(info->method);
  for (i = 4; i < numInsts; ++i) {
    ir_instruction_t *inst = IRMETHOD_getInstructionAtPosition(info->method, i);
    if (!xanHashTable_lookup(seenInsts, inst)) {
      if (inst->type != IRLABEL) {
        ir_instruction_t *call = insertDebuggingLoadCheckValue(info, inst, base, offset, check, JITFALSE);
        ir_instruction_t *after = IRMETHOD_getPredecessorInstruction(info->method, call, NULL);
        xanHashTable_insert(seenInsts, inst, inst);
        xanHashTable_insert(seenInsts, call, call);
        xanHashTable_insert(seenInsts, after, after);
        numInsts = IRMETHOD_getInstructionsNumber(info->method);
      }
    }
  }
  xanHashTable_destroyTable(seenInsts);
}


/**
 * Find an instruction by its original ID.
 **/
static ir_instruction_t *
findInstByOrigID(pass_info_t *info, JITUINT32 origID)
{
  XanHashTableItem *item = xanHashTable_first(info->origInstIDs);
  while (item) {
    JITUINT32 id = ptrToUint32(item->element);
    if (id == origID) {
      return item->elementID;
    }
    item = xanHashTable_next(info->origInstIDs, item);
  }
  return NULL;
}


/**
 * Insert debugging stores to write a variable back to memory at certain points
 * in the CFG.  This is to find locations where a store should be placed but
 * for some reason hasn't been.
 **/
static void
insertDebuggingStores(pass_info_t *info, ir_item_t *base, ir_item_t *offset, ir_item_t *value)
{
  ir_instruction_t *around = findInstByOrigID(info, 90);
  insertDebuggingStoreValue(info, around, base, offset, value, JITFALSE);
}
#endif


/**
 * Insert a guard initialisation instruction along an edge in the CFG.
 * For now it is assumed that the edge will always be between successive
 * instructions, so inserting new basic blocks should be be an issue.
 **/
static ir_instruction_t *
insertGuardInitInEdge(pass_info_t *info, global_t *global, edge_t *edge)
{
  ir_item_t *guard = ((global_info_t *)xanHashTable_lookup(info->globalInfo, global))->guard;
  ir_instruction_t *move = IRMETHOD_newInstructionOfTypeBefore(info->method, edge->succ, IRMOVE);
  IRMETHOD_setInstructionParameter1(info->method, move, 0, 0.0, IRINT32, IRINT32, NULL);
  IRMETHOD_cpInstructionResult(move, guard);
  PDEBUG(2, "  Inserted new initialisation for guard var %llu between inst %u and inst %u\n", guard->value.v, origID(info, edge->pred), origID(info, edge->succ));
  return move;
}


/**
 * Insert a load from a global variable in memory along an edge in the CFG.
 * For now it is assumed that the edge will always be between successive
 * instructions, so inserting new basic blocks should be be an issue.
 **/
static ir_instruction_t *
insertLoadFromGlobalInEdge(pass_info_t *info, global_t *global, edge_t *edge)
{
  ir_item_t *local = ((global_info_t *)xanHashTable_lookup(info->globalInfo, global))->local;
  ir_instruction_t *load = IRMETHOD_newInstructionOfTypeBefore(info->method, edge->succ, IRLOADREL);
  assert(local);
  IRMETHOD_cpInstructionParameter1(load, &global->base);
  IRMETHOD_cpInstructionParameter2(load, &global->offset);
  IRMETHOD_cpInstructionResult(load, local);
  PDEBUG(2, "  Inserted new load to var %llu between inst %u and inst %u\n", local->value.v, origID(info, edge->pred), origID(info, edge->succ));
  /* insertDebuggingNativeCall(info, load, IRMETHOD_getInstructionParameter1(load), JITTRUE); */
  return load;
}


/**
 * Insert a store to a global variable in memory along an edge in the CFG.  For
 * now it is assumed that the edge will always be between successive
 * instructions, so inserting new basic blocks should be be an issue.
 **/
static ir_instruction_t *
insertStoreToGlobalInEdge(pass_info_t *info, global_t *global, edge_t *edge)
{
  ir_item_t *local = ((global_info_t *)xanHashTable_lookup(info->globalInfo, global))->local;
  ir_instruction_t *store = IRMETHOD_newInstructionOfTypeBefore(info->method, edge->succ, IRSTOREREL);
  assert(local);
  IRMETHOD_cpInstructionParameter1(store, &global->base);
  IRMETHOD_cpInstructionParameter2(store, &global->offset);
  IRMETHOD_cpInstructionParameter3(store, local);
  PDEBUG(2, "  Inserted new store from var %llu between inst %u and inst %u\n", local->value.v, origID(info, edge->pred), origID(info, edge->succ));
  return store;
}


/**
 * Insert a guarded store to a global variable in memory before a certain
 * instruction.  The guard means that the store will only actually get executed
 * if there should have been an update to that global variable.
 **/
static void
insertGuardedStoreToGlobalInEdge(pass_info_t *info, global_t *global, edge_t *edge)
{
  ir_item_t *guard = ((global_info_t *)xanHashTable_lookup(info->globalInfo, global))->guard;
  ir_instruction_t *store = insertStoreToGlobalInEdge(info, global, edge);
  ir_instruction_t *label = IRMETHOD_newLabelBefore(info->method, edge->succ);
  ir_instruction_t *branch = IRMETHOD_newInstructionOfTypeBefore(info->method, store, IRBRANCHIFNOT);
  IRMETHOD_cpInstructionParameter1(branch, guard);
  IRMETHOD_cpInstructionParameter2(branch, IRMETHOD_getInstructionParameter1(label));
  assert(IRMETHOD_getBranchDestination(info->method, branch) == label);
  assert(IRMETHOD_getBranchFallThrough(info->method, branch) == store);
  /* insertDebuggingNativeCall(info, store, IRMETHOD_getInstructionParameter3(store), JITTRUE); */
}


/**
 * Insert loads and stores to each global variable at the start and end of each
 * usage region of instructions for that global.
 **/
static void
moveGlobalsIntoVariables(pass_info_t *info)
{
  JITUINT32 g;
  PDEBUG(1, "Moving globals into variables\n");
  for (g = 0; g < info->numGlobals; ++g) {
    global_t *global = xanHashTable_lookup(info->idToGlobal, uintToPtr(g));
    global_info_t *ginfo = xanHashTable_lookup(info->globalInfo, global);
    if (global && ginfo) {
      if (info->usageRegions[global->ID].entryEdges != NULL) {

        /**
         * At this point we should try to be more intelligent about the loads
         * and stores that are added.  Loads only need to be added if we are
         * removing a load before any stores along at least one path in the CFG
         * from each point.  Ditto for stores.
         **/

        /* Work through new loads to insert. */
        /* fprintf(stderr, "Warning: Not adding new loads\n"); */
        if (xanList_length(ginfo->loads) > 0) {
          XanListItem *edgeItem = xanList_first(info->usageRegions[global->ID].entryEdges);
          while (edgeItem) {
            edge_t *edge = edgeItem->data;
            insertLoadFromGlobalInEdge(info, global, edge);
            /* ir_instruction_t *load = insertLoadFromGlobalInEdge(info, global, edge); */
            /* if (origID(info, edge->pred) == 3) { */
            /*   insertDebuggingCheckAll(info, &global->base, &global->offset, IRMETHOD_getInstructionResult(load)); */
            /* } */
            edgeItem = edgeItem->next;
          }
        }

        /* Work through new stores to insert. */
        /* fprintf(stderr, "Warning: Not adding new stores\n"); */
        if (xanList_length(ginfo->stores) > 0) {
          XanListItem *edgeItem = xanList_first(info->usageRegions[global->ID].exitEdges);
          while (edgeItem) {
            edge_t *edge = edgeItem->data;
            insertGuardedStoreToGlobalInEdge(info, global, edge);
            edgeItem = edgeItem->next;
          }
        }

        /* Initialise guards at the start of each region. */
        /* fprintf(stderr, "Warning: Not adding guard initialisation\n"); */
        if (xanList_length(ginfo->stores) > 0) {
          XanListItem *edgeItem = xanList_first(info->usageRegions[global->ID].entryEdges);
          while (edgeItem) {
            edge_t *edge = edgeItem->data;
            insertGuardInitInEdge(info, global, edge);
            edgeItem = edgeItem->next;
          }
        }
      }
    }
  }
}


/**
 * Convert stores to each global variable into moves to the new local variable
 * for that global.  Loads from the global can be simply removed.
 **/
static void
removeOriginalGlobalAccesses(pass_info_t *info)
{
  JITINT32 removedGlobals JITUNUSED = 0;
  XanHashTableItem *globalItem = xanHashTable_first(info->globalInfo);
  PDEBUG(1, "Removing original global accesses\n");
  while (globalItem) {
    global_info_t *ginfo = globalItem->element;
    XanListItem *accItem;

    /* Stores become moves into the global's variable. */
    accItem = xanList_first(ginfo->stores);
    while (accItem) {
      ir_instruction_t *accInst = accItem->data;

      /* Create the move into a local variable. */
      ir_instruction_t *move = IRMETHOD_newInstructionOfTypeBefore(info->method, accInst, IRMOVE);
      PDEBUG(2, "  Going to convert store %u to a move\n", origID(info, accInst));
      IRMETHOD_cpInstructionResult(move, ginfo->local);
      IRMETHOD_cpInstructionParameter1(move, IRMETHOD_getInstructionParameter3(accInst));

      /* Create the move into the guard. */
      move = IRMETHOD_newInstructionOfTypeBefore(info->method, accInst, IRMOVE);
      IRMETHOD_cpInstructionResult(move, ginfo->guard);
      IRMETHOD_setInstructionParameter1(info->method, move, 1, 0.0, IRINT32, IRINT32, NULL);
      accItem = accItem->next;
    }

    /* Loads become moves from the global's variable. */
    accItem = xanList_first(ginfo->loads);
    while (accItem) {
      ir_instruction_t *accInst = accItem->data;
      ir_instruction_t *move = IRMETHOD_newInstructionOfTypeBefore(info->method, accInst, IRMOVE);
      PDEBUG(2, "  Going to convert load %u to a move\n", origID(info, accInst));
      IRMETHOD_cpInstructionParameter1(move, ginfo->local);
      IRMETHOD_cpInstructionResult(move, IRMETHOD_getInstructionResult(accInst));
      accItem = accItem->next;
    }

    /* Remove all original instructions. */
    /* fprintf(stderr, "Warning: Not deleting original instructions\n"); */
    accItem = xanList_first(ginfo->accesses);
    while (accItem) {
      ir_instruction_t *accInst = accItem->data;
      PDEBUG(1, "  Going to delete inst %u\n", origID(info, accInst));
      IRMETHOD_deleteInstruction(info->method, accInst);
      /* if (origID(info, accInst) == 40) { */
      /*   insertDebuggingStores(info, IRMETHOD_getInstructionParameter1(accInst), IRMETHOD_getInstructionParameter2(accInst), IRMETHOD_getInstructionParameter3(accInst)); */
      /* } */
      accItem = accItem->next;
    }
    removedGlobals += 1;
    globalItem = xanHashTable_next(info->globalInfo, globalItem);
  }
  PDEBUG(0, "Removed %d globals from %s\n", removedGlobals, IRMETHOD_getCompleteMethodName(info->method));
}


/**
 * Run the main scalarisation algorithm for a single method.
 **/
static void
scalarisationSingleMethod(ir_method_t *method)
{
  pass_info_t *info;
  /* JITINT8 *name = IRMETHOD_getCompleteMethodName(method); */
  /* static JITBOOLEAN warned = JITFALSE; */
  /* if (!warned) { */
  /*   fprintf(stderr, "Warning: Filtering based on method name\n"); */
  /*   warned = JITTRUE; */
  /* } */
  /* if (name[4] != 'O') { */
  /*   return; */
  /* } */

  /* Debugging. */
  PDEBUG(1, "Scalarization for %s\n", IRMETHOD_getCompleteMethodName(method));
  PDEBUGSTMT(0, setenv("DOTPRINTER_FILENAME", "Scalarisation", 1));
  PDEBUGSTMT(0, IROPTIMIZER_callMethodOptimization(irOptimizer, method, METHOD_PRINTER));

  /* Initialise information for this pass. */
  info = initialisePassInfo(method);

  /* Identify the global accesses. */
  identifyAccessesToGlobals(info);
  filterGlobalsWithFewAccesses(info);

  /* Early exit. */
  if (xanHashTable_elementsInside(info->idToGlobal) == 0) {
    PDEBUG(1, "No globals\n");
    freePassInfo(info);
    return;
  }

  /* Identify memory instructions that are dependent on each global. */
  identifyMemoryDependenceInsts(info);

  /* Identify regions where the global is currently used. */
  identifyUsageRegions(info);

  /* Add new loads and stores. */
  moveGlobalsIntoVariables(info);

  /* Remove the original instructions. */
  removeOriginalGlobalAccesses(info);

  /* Clean up. */
  freePassInfo(info);
  PDEBUGSTMT(0, IROPTIMIZER_callMethodOptimization(irOptimizer, method, METHOD_PRINTER));
}


/**
 * Transform accesses to globals in memory so that they are loaded at the start
 * of the method (if read) and their updated value (if written to) is stored at
 * the end of the method.
 *
 * The algorithm works by identifying all loads and stores to each global
 * variable's memory location.  It then identifies all points in the CFG where
 * the value must be loaded from memory into a local variable (at the start of
 * the method and immediately after each instruction that a store to the global
 * has a dependence with) and stored back to memory (at the end of the method
 * and before each dependence instruction).  It then adds the new loads and
 * stores and removes the old ones, transforming the old stores to moves.
 **/
static void
scalarization_do_job(ir_method_t *method)
{
  char *env;
  XanHashTable *loops;
  XanHashTable *loopNames;
  XanHashTable *seenMethods;
  XanList *loopList;
  XanListItem *loopItem;

  /* Assertions. */
  assert(method != NULL);

  /* Debugging. */
  env = getenv("SCALARIZATION_DEBUG_LEVEL");
  if (env != NULL) {
    PDEBUGLEVEL(atoi(env));
  }
  PDEBUG(1, "do_job: Start\n");

  /* Analysis data dependences of the entire program. */
  PDEBUG(1, "do_job:   Run DDG\n");
  IROPTIMIZER_callMethodOptimization(irOptimizer, method, DDG_COMPUTER);

  /* Choose loops to consider. */
  PDEBUG(1, "do_job:   Identify loops\n");
  loops = IRLOOP_analyzeLoops(LOOPS_analyzeCircuits);
  loopNames = LOOPS_getLoopsNames(loops);
  loopList = xanHashTable_toList(loops);
  MISC_chooseLoopsToProfile(loopList, loopNames, NULL);

  /* Scalarise within each method containing a loop. */
  PDEBUG(1, "do_job:   Analyze methods that contain loops\n");
  seenMethods = newHashTable();
  loopItem = xanList_first(loopList);
  while (loopItem) {
    loop_t *loop = loopItem->data;
    if (!xanHashTable_lookup(seenMethods, loop->method)) {

      /* No DDG analysis within library methods. */
      if (!IRPROGRAM_doesMethodBelongToALibrary(loop->method)) {
        PDEBUG(1, "do_job:     Analyze %s\n", IRMETHOD_getSignatureInString(loop->method));
        scalarisationSingleMethod(loop->method);
      }
      xanHashTable_insert(seenMethods, loop->method, loop->method);
    }
    loopItem = loopItem->next;
  }

  /* Clean up. */
  xanHashTable_destroyTable(seenMethods);
  IRLOOP_destroyLoops(loops);
  LOOPS_destroyLoopsNames(loopNames);
  xanList_destroyList(loopList);

  return ;
}

static inline void scalarization_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix){

	/* Assertions.
	 */
	assert(lib != NULL);
	assert(optimizer != NULL);
	assert(outputPrefix != NULL);

	irLib		= lib;
	irOptimizer	= optimizer;
	prefix		= outputPrefix;
}

static inline void scalarization_shutdown (JITFLOAT32 totalTime){
        irLib		= NULL;
	irOptimizer	= NULL;
	prefix		= NULL;
}

static inline JITUINT64 scalarization_get_ID_job (void){
	return JOB;
}

static inline JITUINT64 scalarization_get_dependences (void){
	return 0;
}

static inline char * scalarization_get_version (void){
	return VERSION;
}

static inline char * scalarization_get_informations (void){
	return INFORMATIONS;
}

static inline char * scalarization_get_author (void){
	return AUTHOR;
}

static inline JITUINT64 scalarization_get_invalidations (void){
	return INVALIDATE_ALL;
}

static inline void scalarization_start_execution (void){
	return ;
}

static inline void scalarization_getCompilationFlags (char *buffer, JITINT32 bufferLength){

	/* Assertions				*/
	assert(buffer != NULL);

	snprintf(buffer, sizeof(char) * bufferLength, " ");
	#ifdef DEBUG
	strncat(buffer, "DEBUG ", sizeof(char) * bufferLength);
	#endif
	#ifdef PRINTDEBUG
	strncat(buffer, "PRINTDEBUG ", sizeof(char) * bufferLength);
	#endif
	#ifdef PROFILE
	strncat(buffer, "PROFILE ", sizeof(char) * bufferLength);
	#endif
}

static inline void scalarization_getCompilationTime (char *buffer, JITINT32 bufferLength){

	/* Assertions				*/
	assert(buffer != NULL);

	snprintf(buffer, sizeof(char) * bufferLength, "%s %s", __DATE__, __TIME__);

	/* Return				*/
	return;
}

ir_optimization_interface_t plugin_interface = {
	scalarization_get_ID_job	, 
	scalarization_get_dependences	,
	scalarization_init		,
	scalarization_shutdown		,
	scalarization_do_job		, 
	scalarization_get_version	,
	scalarization_get_informations	,
	scalarization_get_author	,
	scalarization_get_invalidations	,
	scalarization_start_execution	,
	scalarization_getCompilationFlags,
	scalarization_getCompilationTime
};
