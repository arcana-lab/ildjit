#!/bin/bash

function print_header(){
	clear
	echo "###############################################################################################"
	echo "                            ILDJIT optimization tests                                            "
	echo "###############################################################################################"
	echo "Version: 0.0.3"
	echo "Author : Simone Campanoni (simo.xan@gmail.com)"
	echo ""
}

function print_footer(){
	echo "Exiting from testing"
	echo "###############################################################################################"
}

declare GC;
declare OPT;
declare FILE_OUTPUT;

GC="Allocator"
OPT="0"
FILE_OUTPUT="`pwd`/ILDJIT_Time_($GC)_O$OPT"
topDir="`pwd`"

print_header

echo "ILDJIT Profile" > $FILE_OUTPUT

echo "Garbage Collector = $GC" >> $FILE_OUTPUT
echo "Optimization level = $OPT" >> $FILE_OUTPUT
echo "CPUs used = 4" >> $FILE_OUTPUT

echo -e "Benchmark \tUser space time (in seconds) \tKernel space time (in seconds) \tCPU percentage \tPage faults \tAverage memory usage (in KB) \tMaximum memory usage (in KB)" >> $FILE_OUTPUT

for i in `find ./ -name '*.exe'` ; do
	d=`dirname $i`
	cd $topDir
	cd $d
	echo -e "\r                                                                \c "
	echo -e "\rWorking progress: $i"
	# Total time = (U + S) / 4
	/usr/bin/time --format="%U \t%S \t%P \t%R \t%K \t%M" taskset 0x0000000F iljit --gc=$GC -O$OPT `basename $i` &> ILDJIT_Time_tmp
	echo -e "`basename $i` \t`cat ILDJIT_Time_tmp`" >> $FILE_OUTPUT
	rm ILDJIT_Time_tmp
done

cd $topDir

rm -f `find ./ -name ILDJIT_Time_tmp`

print_footer
