/*
 * Copyright (C) 2008  Campanoni Simone
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
#ifndef INTERNAL_CALLS_CHARINFO_H
#define INTERNAL_CALLS_CHARINFO_H

#include <jitsystem.h>

/*
 * Unicode character categories.
 */
typedef enum {
    ILUnicode_UppercaseLetter = 0,
    ILUnicode_LowercaseLetter = 1,
    ILUnicode_TitlecaseLetter = 2,
    ILUnicode_ModifierLetter = 3,
    ILUnicode_OtherLetter = 4,
    ILUnicode_NonSpacingMark = 5,
    ILUnicode_SpaceCombiningMark = 6,
    ILUnicode_EnclosingMark = 7,
    ILUnicode_DecimalDigitNumber = 8,
    ILUnicode_LetterNumber = 9,
    ILUnicode_OtherNumber = 10,
    ILUnicode_SpaceSeparator = 11,
    ILUnicode_LineSeparator = 12,
    ILUnicode_ParagraphSeparator = 13,
    ILUnicode_Control = 14,
    ILUnicode_Format = 15,
    ILUnicode_Surrogate = 16,
    ILUnicode_PrivateUse = 17,
    ILUnicode_ConnectorPunctuation = 18,
    ILUnicode_DashPunctuation = 19,
    ILUnicode_OpenPunctuation = 20,
    ILUnicode_ClosePunctuation = 21,
    ILUnicode_InitialQuotePunctuation = 22,
    ILUnicode_FinalQuotePunctuation = 23,
    ILUnicode_OtherPunctuation = 24,
    ILUnicode_MathSymbol = 25,
    ILUnicode_CurrencySymbol = 26,
    ILUnicode_ModifierSymbol = 27,
    ILUnicode_OtherSymbol = 28,
    ILUnicode_OtherNotAssigned = 29,

} ILUnicodeCategory;

JITINT32        Platform_SysCharInfo_GetUnicodeCategory (JITINT16 ch);
void *          Platform_SysCharInfo_GetNewLine (void);

#endif
