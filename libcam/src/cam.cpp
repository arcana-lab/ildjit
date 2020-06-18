#include <assert.h>
#include <xanlib.h>
#include <iostream>
#include <string>
#include <fstream>
#include "cam.h"
#include "MemoryTracer.h"
#include "loop_trace.hh"

using namespace std;

static void segFaultHandler(int nSig)
{
  fprintf(stderr, "Caught segfault: %i: %s\n", errno, strerror(errno));
  exit(50);
}

static void setSegFaultHandler(){
  signal(SIGSEGV, segFaultHandler);
}

void CAM_init (cam_mode_t mode) {
  setSegFaultHandler();
  if (mode == CAM_MEMORY_PROFILE) {
    memory_trace_init();
  } else if (mode == CAM_LOOP_PROFILE) {
    loop_trace_init();
  }
}

void CAM_shutdown (cam_mode_t mode) {
  if (mode == CAM_MEMORY_PROFILE) {
    memory_trace_shutdown();
  } else if (mode == CAM_LOOP_PROFILE) {
    loop_trace_shutdown();
  }
}
