/*
 * Copyright (C) 2009  Campanoni Simone
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
#ifndef SMALL_UNICODE_H
#define SMALL_UNICODE_H

#include <jitsystem.h>

JITINT32 ILIsWhitespaceUnicode (JITINT16 ch);
JITINT16 ILUnicodeCharToUpper (JITINT16 ch);
JITINT16 ILUnicodeCharToLower (JITINT16 ch);
void ILUnicodeStringToUpper (JITINT16 *dest, const JITINT16 *src, JITINT32 len);
void ILUnicodeStringToLower (JITINT16 *dest, const JITINT16 *src, JITINT32 len);
JITINT32 ILUnicodeStringCompareIgnoreCase (const JITINT16 *str1, const JITINT16 *str2, JITINT32 len);
JITINT32 ILUnicodeStringCompareNoIgnoreCase (const JITINT16 *str1, const JITINT16 *str2, JITINT32 len);


#endif
