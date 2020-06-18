#!/bin/bash

declare CILPrograms;
declare success;
declare count;
declare testsNumber;
declare testsFailed;
declare testsSuccess;


success="Success"
CILPrograms=`find ./unit -maxdepth 1 -type f`
testsNumber=$((`find ./unit -maxdepth 1 -type f | wc -l` * 2))
let testsFailed=0
echo -e "Running $testsNumber unit tests"
export EF_DISABLE_BANNER=1
export EF_PROTECT_FREE=1
export EF_ALLOW_MALLOC_0=1
export EF_FREE_WIPES=1

let count=0
for i in $CILPrograms ; do
	b=`basename $i`
	echo -e "\r                                                                                                       \c "
	echo -e "\rWorking progress: $count/$testsNumber 	$b\c "
	let count=count+1
	export EF_PROTECT_BELOW=0
	$i
	if test $? -ne 0 ; then
		let testsFailed=$testsFailed+1
		success="Some test is failed"
		echo ""
		echo "	Test $i (with upper bound limit) is failed"
	fi
	let count=count+1
	export EF_PROTECT_BELOW=1
	$i
	if test $? -ne 0 ; then
		let testsFailed=$testsFailed+1
		success="Some test is failed"
		echo ""
		echo "	Test $i (with lower bound limit) is failed"
	fi
done
[ $testsFailed = 0 ] && echo -en "\r"
let testsSuccess=$testsNumber-$testsFailed
echo -e "Tests succesfully run: $testsSuccess/$testsNumber                                                                "
echo $success
