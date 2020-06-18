#!/bin/bash

if test $# -lt 1 ; then
	echo "USAGE: `basename $0` GPERF_FILE" ;
	exit 1 ;
fi

awk '
	BEGIN{
		n=0;
	} {
		if (n <= 1){
			if ($1 == "%%") {
				print ;
				n++;
			} else {
				if (	(n == 1)	&&
					($2 != "")	&&
					($3 == "")	){
					printf("%s %s, \"%s\"\n", $1, $2, $2);
				} else {
					print ;
				}
			}
		} else {
			print ;
		}
	}' $1 ;
