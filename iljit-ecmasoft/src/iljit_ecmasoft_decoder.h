/*
 * Copyright (C) 2006  Campanoni Simone
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
   *@file iljit_ecmasoft_decoder.h
 */
#ifndef ILJIT_DOTGNU_LOADER_H
#define ILJIT_DOTGNU_LOADER_H

/**
 * @mainpage ILDJIT decoder of the ECMA-335 standard.
 * \author Campanoni Simone
 * \version 0.1.0
 *
 * This is a decoder of the bytecode CIL as specified by the ECMA-335; this decoder read only the declarations inside the binary file that they are variable.
 *
 * Project web page: \n
 *      http://ildjit.sourceforge.net
 */

// My headers
#include <metadata_manager.h>
#include <iljit-utils.h>
#include <jitsystem.h>
#include <decoder.h>
// End

#define BINARY_ID                       "MZ"
#define DIM_BUF                         1024
#define MIN_DIRECTORIES                 15
#define MIN_UINT                        0
#define MAX_UINT                        (JITUINT32) 0xFFFFFFFFL
#define DOTTEXT_SECTION                 ".text$il"
#define CORMETA_SECTION                 ".cormeta"
#define DEBUG_SECTION                   ".ildebug"
#define SDATA_SECTION                   ".sdata\0\0"
#define DATA_SECTION                    ".data\0\0\0"
#define TLS_SECTION                     ".data\0\0\0\0"
#define RSRC_SECTION                    ".rsrc\0\0\0"
#define BSS_SECTION                     ".bss\0\0\0\0"

#ifdef PRINTDEBUG
#define PDEBUG(fmt, args ...) fprintf(stderr, fmt, ## args)
#else
#define PDEBUG(fmt, args ...)
#endif

#endif
