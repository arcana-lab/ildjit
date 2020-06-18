
namespace CSGrande {

  class CSGrande {

    public static void Main() {

      IBench[] benchs = {
	                  new BasicComplexObject(), new BasicEmptyObject(),
			  new BasicEmptyObjectWithConstructor(),
			  new BasicFourFloatObject(),
			  new BasicFourIntegerObject(),
			  new BasicFourLongObject(), new BasicInlineObject(),
			  new BasicObject(), new BasicOneIntegerObject(),
			  new BasicSubclassObject(),
			  new BasicTwoIntegerObject(), new FloatArray(),
			  new IntegerArray(), new LongArray(), new ObjectArray()
                        };
      
      foreach(IBench bench in benchs)
        bench.RunBench();

    }

  }

}
