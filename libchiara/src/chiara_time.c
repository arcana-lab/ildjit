/*
 * Copyright (C) 2010 - 2011  Campanoni Simone
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <jitsystem.h>

// My headers
#include <chiara.h>
#include <chiara_system.h>
// End

typedef union {
    JITUINT64 int64;
    struct {
        JITUINT32 lo, hi;
    } int32;
} chiara_64_t;

JITUINT64 TIME_getClockTicks (void) {
    chiara_64_t temp;
    /*	asm volatile (
             	"\tpushl %ebx"
    		);
    	asm volatile (
    		"cpuid \n"
    		"rdtsc"
    		: "=a" ((temp).int32.lo), "=d"((temp).int32.hi) 	// outputs
    		:  	            					// inputs
    		: "%ecx"	 					// clobbers
            	);
    	asm volatile (
             	"\tpopl %ebx"
    		);*/
    return temp.int64;
}

JITUINT64 TIME_getClockTicksBaseline (void) {
    chiara_64_t temp;
    /*asm volatile (
         	"\tpushl %ebx"
    	);
    asm volatile (
    	"cpuid"
    	: "=a" ((temp).int32.lo), "=d"((temp).int32.hi) 	// outputs
    	:  	            					// inputs
    	: "%ecx"	 					// clobbers
        	);
    asm volatile (
         	"\tpopl %ebx"
    	);*/
    return temp.int64;
}

JITFLOAT64 TIME_toSeconds (JITUINT64 deltaTicks, JITUINT32 clockFrequencyToUse) {
    return ((JITFLOAT64) deltaTicks) / ((JITFLOAT64) (clockFrequencyToUse * 1000000));
}
