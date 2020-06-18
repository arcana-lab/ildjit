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
#ifndef ILDJIT_LOCALE_H
#define ILDJIT_LOCALE_H

// My headers
#include <iljit-utils.h>
// End

/*
 * Some interesting Windows code page numbers.
 */
#define CP_8859_1                       28591
#define CP_8859_2                       28592
#define CP_8859_3                       28593
#define CP_8859_4                       28594
#define CP_8859_5                       28595
#define CP_8859_6                       28596
#define CP_8859_7                       28597
#define CP_8859_8                       28598
#define CP_8859_9                       28599
#define CP_8859_15                      28605
#define CP_SHIFT_JIS                    932
#define CP_KOI8_R                       20866
#define CP_KOI8_U                       21866
#define CP_UTF7                         65000
#define CP_UTF8                         65001
#define CP_UCS2_LE                      1200
#define CP_UCS2_BE                      1201
#define CP_UCS2_DETECT                  1202            /* Fake value for auto-detection */
#define CP_DEFAULT                      0

/**
 * Get the current code page
 *
 * This function is not thread-safe.
 *
 * @return the current code page or 0 on errors
 */
JITINT32 getCurrentCodePage (void);

/**
 * Get the given code page as a string
 *
 * @param codePage the wanted code page
 *
 * @return the string representation of codePage, or NULL if it is unknown
 */
const JITINT8* getCodePageAsString (JITINT32 codePage);

JITINT32 getBytes (JITINT16* chars, JITINT32 charCount, JITINT8* bytes, JITINT32 byteCount);

JITINT32 getChars (JITINT8* bytes, JITINT32 byteCount, JITINT16 *chars, JITINT32 charCount);

JITUINT32 getMaxByteCount (JITUINT32 charCount);
JITINT32 getMaxCharCount (JITINT32 byteCount);

#endif
