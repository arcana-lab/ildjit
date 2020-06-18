
using System;

/*
 * Create an array of float. This test is a part of Java Grande Forum
 * Benchmark suite. It is taken from JGFCreateBench benchmark.
 */  
namespace CSGrande {

  class FloatArray : IBench {

    public const int INITSIZE = 10000;
    public const int MAXSIZE = 100000;
    public const int MAX = 128;
    public const int ARRAYS_COUNT = 16;

    public void RunBench() {

      float[] array;

      for(int n = 1; n<= MAX; n *= 2) {
	for(int size = INITSIZE; size < MAXSIZE; size *= 2)
	  for(int i = 0; i < size; i++)
	    for(int j = 0; j < ARRAYS_COUNT; j++)
	      array = new float[n];
        System.GC.Collect();
      }

    }

  }

}
