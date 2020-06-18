#!/bin/bash

baseDir=`pwd`

for j in `cat listOfModules`
do
	cd $j
	echo "$j"
	files=`find ./ -name "*.[ch]"`
	for i in $files ; do 
		echo "Dir `pwd`: $i" ;
		astyle --style=java --add-brackets --indent-switches $i ;
	done
	rm -f `find ./ -name \*.orig` ;
	cd ..
done
