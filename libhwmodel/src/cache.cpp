#include <stdio.h>
#include <assert.h>
#include <cmath>

// My headers
#include <cache.h>
#include <cache_array.h>
#include <hwmodel_system.h>
// End

extern "C" {

static CacheArray* l1cache;
static CacheArray* l2cache;
static unsigned long long int * clockCycles;

void CACHE_initCache (unsigned long long int * programClockCycles){

	/* Assertions.
	 */
	assert(programClockCycles != NULL);
        clockCycles = programClockCycles;        
        
        l1cache = new CacheArray(8192, 8);
        l2cache = new CacheArray(8192*16, 8);

	return ;
}

extern "C" void CACHE_processLoad (void * addr, unsigned int inputSize) {
        /* Assertions.
         */
        assert(addr != NULL);
        
        int numRequests = ceil(inputSize / 4.0); // one word = 4 bytes
        for(int i = 0; i < numRequests; i++) {
          bool l1hit = l1cache->doMemoryRequest((unsigned int)addr + (i*4), 0);
          bool l2hit = l2cache->doMemoryRequest((unsigned int)addr + (i*4), 0);
          if(!l1hit) {
            (*clockCycles) += 9; // L1 miss penalty is 9 cycles
            if(!l2hit) {
              (*clockCycles) += 80; // L2 miss penalty is 80 cycles
            }
          }
        }

        return;
}

extern "C" void CACHE_processStore (void * addr, unsigned int inputSize) {
        /* Assertions.
         */
        assert(addr != NULL);
        
        // For now, stores are the same as loads
        CACHE_processLoad(addr, inputSize);
        
        return;
}

extern "C" void CACHE_shutdownCache() {
        delete l1cache;
        delete l2cache;

        return;
}

}
