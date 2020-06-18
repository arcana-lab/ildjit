#!/bin/bash

declare i
declare tab
let tab=0
let count=0


while read thread
do
	rm -f $1-thread-$count
	while read line
	do
		echo "$line"|grep "Tracer: Exit" >/dev/null
		if [ $? -eq 0 ]; then
			let tab--
		fi
		echo -n "$tab"	>> $1-thread-$count
		for (( i = 0 ; i < tab ; i++ ))
		do
			echo -e -n "\t" >> $1-thread-$count
		done
		echo $line >> $1-thread-$count
		echo "$line"|grep "Tracer: Enter" >/dev/null
		if [ $? -eq 0 ]; then
			let tab++
		fi		
	done < <(grep "$thread" $1| sed 's/ on Thread 0x[a-f0-9]*//g')
	let count++
done < <(grep -o "0x[a-f0-9]*$" $1|sort -u)
