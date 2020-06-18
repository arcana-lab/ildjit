/*
 * Copyright (C) 2006  Campanoni Simone
 *
 * iljit - This is a Just-in-time for the CIL language specified with the ECMA-335
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

/**
 * @file error_codes.h
 * @brief Error codes
 *
 * In this file there are the error codes used by the function of the ILJIT module as return value
 */

#ifndef ERROR_CODES_H
#define ERROR_CODES_H

/* Loader					*/

/**
 * @def ALL_OK
 *
 * The function has run all needed computation without any error
 */
#define ALL_OK                                          0

/**
 * @def NO_DECODER_FOUND
 *
 * There is no decoder inside the prefix/lib/iljit/loaders
 */
#define NO_DECODER_FOUND                                -1

/**
 * @def NO_READ_POSSIBLE
 *
 * It couldn't read from the file
 */
#define NO_READ_POSSIBLE                                -2

/**
 * @def NO_SEEK_POSSIBLE
 *
 * It couldnt' seek inside the file
 */
#define NO_SEEK_POSSIBLE                                -3

/**
 * @def DECODER_RETURN_AN_ERROR
 *
 * The decoder plugin return an error during decoding the CIL bytecode
 */
#define DECODER_RETURN_AN_ERROR                         -4

/**
 * @def CANNOT_LOAD_CIL_IN_MEMORY
 *
 * An error occurs during loading the CIL in the memory
 */
#define CANNOT_LOAD_CIL_IN_MEMORY                       -5

/**
 * @def ENTRY_POINT_NOT_VALID
 *
 * The entry point specified inside the header of the bytecode is not valid
 */
#define ENTRY_POINT_NOT_VALID                           -6

/**
 * @def ENTRY_POINT_NOT_FOUND
 *
 * There isn't the entry point specified inside the header of the bytecode
 */
#define ENTRY_POINT_NOT_FOUND                           -7

/**
 * @def CANNOT_DECODE_ASSEMBLIES_REFERENCED
 *
 * It was not possible decode the assemblies referenced by the CIL program given as input
 */
#define CANNOT_DECODE_ASSEMBLIES_REFERENCED             -8

/**
 * @def CANNOT_ALLOCATE_MEMORY
 *
 * It was not possible allocate memory
 */
#define CANNOT_ALLOCATE_MEMORY                          -9

#define CANNOT_SEND_METHOD_TO_OPTIMIZER                 -10
/* JIT						*/

#define COMPILE_ERROR                                   -11

#define NATIVE_CALL                                     -12

#endif
