#ifndef CACHE_H
#define CACHE_H

#ifdef __cplusplus
extern "C" {
#endif

void CACHE_initCache (unsigned long long int * programClockCycles);
void CACHE_processLoad (void * addr, unsigned int inputSize);
void CACHE_processStore (void * addr, unsigned int inputSize);
void CACHE_shutdownCache();

#ifdef __cplusplus
};
#endif

#endif
