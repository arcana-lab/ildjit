/*
 * Copyright (C) 2008 - 2011 Campanoni Simone
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
#ifndef INTERNAL_CALLS_STRING_H
#define INTERNAL_CALLS_STRING_H

#include <jitsystem.h>

/****************************************************************************************************************************
*                                               STRING                                                                      *
****************************************************************************************************************************/
JITINT32        System_String_GetHashCode (void *obj);
JITINT16        System_String_GetChar (void *_this, JITINT32 posn);
void            System_String_SetChar (void *_this, JITINT32 posn, JITINT16 value);
void *          System_String_Concat (void *str1, void *str2);
void *          System_String_ConcatString3 (void *str1, void *str2, void *str3);
void            System_String_ctor_ac (void **_this, void *valueArray);
void            System_String_ctor_pc (void **_this, JITINT16 *value);
void            System_String_ctor_ci (void **_this, JITINT16 c, JITINT32 count);
void            System_String_ctor_pcii (void **_this, JITINT16 *literal, JITINT32 startIndex, JITINT32 length);
void            System_String_ctor_acii (void **_this, void *valueArray, JITINT32 startIndex, JITINT32 length);
void            System_String_ctor_pbii (void **_this, JITINT8 *value, JITINT32 startIndex, JITINT32 length);
void            System_String_ctor_pb (void **_this, JITINT8 *value);
JITBOOLEAN      System_String_Equals (void *str1, void *str2);
JITINT32        System_String_FindInRange (void *_this, JITINT32 srcFirst, JITINT32 srcLast, JITINT32 srcStep, void *dest);
void            System_String_CopyToChecked (void *string, JITINT32 sourceIndex, void *array, JITINT32 destinationIndex, JITINT32 count);
JITINT32        System_String_IndexOf (void *string, JITINT16 value, JITINT32 startIndex, JITINT32 count);
JITINT32        System_String_IndexOfAny (void *string, void *array, JITINT32 startIndex, JITINT32 count);
void *          System_String_NewString (JITINT32 length);
void *          System_String_NewBuilder (void *string, JITINT32 length);
void            System_String_Copy_StringiString (void *stringDest, JITINT32 destPos, void *stringSrc);
void            System_String_Copy_StringiStringii (void *stringDest, JITINT32 destPos, void *stringSrc, JITINT32 srcPos, JITINT32 length);
void            System_String_InsertSpace (void *str, JITINT32 srcPos, JITINT32 destPos);
void            System_String_RemoveSpace (void *str, JITINT32 index, JITINT32 length);
void*           System_String_Intern (void *str);
void*           System_String_IsInterned (void* str);
JITINT32        System_String_LastIndexOfAny (void *_this, void *anyOfArray, JITINT32 startIndex, JITINT32 count);
JITINT32        System_String_LastIndexOf (void *_this, JITINT16 value, JITINT32 startIndex, JITINT32 count);
void            System_String_CharFill_siic (void *str, JITINT32 start, JITINT32 count, JITINT16 ch);
void            System_String_CharFill_siacii (void *str, JITINT32 start, void *chars, JITINT32 index, JITINT32 count);
void *          System_String_Replace (void *_this, JITINT16 oldChar, JITINT16 newChar);
void *          System_String_Replace_ccii (void *_this, JITINT16 oldChar, JITINT16 newChar, JITINT32 startIndex, JITINT32 count);
void *          System_String_ReplaceStringString (void *_this, void *str1, void *str2);
JITINT32        System_String_Compare (void *str1, void *str2);
void *          System_String_Trim (void *_this, void *trimCharsArray, JITINT32 trimFlags);
JITINT32        System_String_InternalOrdinal (void *strA, JITINT32 indexA, JITINT32 lengthA, void *strB, JITINT32 indexB, JITINT32 lengthB);
JITINT32        System_String_CompareInternal (void *strA, JITINT32 indexA, JITINT32 lengthA, void *strB, JITINT32 indexB, JITINT32 lengthB, JITBOOLEAN ignoreCase);

/**
 * Split self string, using separators as boundaries
 *
 * @param self like this in C++
 * @param separators where split the self string
 * @param count maximum number of substring to return
 * @param options if REMOVE_EMPTY_ENTRIES omit empty array elements from the
 *                array returned, if NONE include empty array elements in the
 *                array returned.
 *
 * @return the self string split around separators
 */
void* System_String_InternalSplit (void* self, void* separators, JITUINT32 count, JITUINT32 options);

/**
 * Allocate a new string with the given length
 *
 * @param length the length of the string to create
 *
 * @return a string whose length is length
 */
void* System_String_InternalAllocateStr (JITINT32 length);

/**
 * Checks if string is interned
 *
 * This is a Mono internal call.
 *
 * @param string the string to check
 *
 * @return a pointer to the interned string, or null if string is not interned
 */
void* System_String_InternalIsInterned (void* string);

/****************************************************************************************************************************
*                                               TEXT (14+10)								    *
****************************************************************************************************************************/
//System.Text.StringBuilder (14)
void *          System_Text_StringBuilder_Insert_ic (void *_this, JITINT32 index, JITINT16 value);
void *          System_Text_StringBuilder_Insert_iac1 (void *_this, JITINT32 index, void *valueArray); //per risolvere temp.un bug
void *          System_Text_StringBuilder_Insert_iacii (void *_this, JITINT32 index, void *valueArray, JITINT32 startIndex, JITINT32 length);
void *          System_Text_StringBuilder_Insert_iString (void *_this, JITINT32 index, void *stringValue);
void *          System_Text_StringBuilder_Insert_iStringi (void *_this, JITINT32 index, void *stringValue, JITINT32 count);
void *          System_Text_StringBuilder_Append_String (void *_this, void *string);
void *          System_Text_StringBuilder_Append_c (void *_this, JITINT16 value);
void *          System_Text_StringBuilder_Append_ci (void *_this, JITINT16 value, JITINT32 repeatCount);
void *          System_Text_StringBuilder_Append_Stringii (void *_this, void *stringValue, JITINT32 startIndex, JITINT32 length);
void *          System_Text_StringBuilder_Append_ac1 (void *_this, void *valueArray);                                           //per risolvere temp.un bug
void *          System_Text_StringBuilder_Append_acii1 (void *_this, void *valueArray, JITINT32 startIndex, JITINT32 length);   //per risolvere temp.un bug
JITINT32        System_Text_StringBuilder_EnsureCapacity (void *_this, JITINT32 capacity);
void *          System_Text_StringBuilder_Replace_cc (void *_this, JITINT16 oldChar, JITINT16 newChar);
void *          System_Text_StringBuilder_Replace_ccii (void *_this, JITINT16 orig, JITINT16 dest, JITINT32 start, JITINT32 end);


//System.Text.DefaultEncoding (10)
JITINT32        System_Text_DefaultEncoding_InternalCodePage (void);
JITINT32        System_Text_DefaultEncoding_InternalGetByteCount_acii (void* chars, JITINT32 index, JITINT32 count);
JITINT32        System_Text_DefaultEncoding_InternalGetByteCount_Stringii (void* s, JITINT32 index, JITINT32 count);
JITINT32        System_Text_DefaultEncoding_InternalGetBytes_aciiaBi1 (void* chars, JITINT32 charIndex, JITINT32 charCount, void* bytes, JITINT32 byteIndex); //per risolvere temp.un bug
JITINT32        System_Text_DefaultEncoding_InternalGetBytes_StringiiaBi (void* s, JITINT32 charIndex, JITINT32 charCount, void* bytes, JITINT32 byteIndex);
JITINT32        System_Text_DefaultEncoding_InternalGetCharCount (void* bytes, JITINT32 index, JITINT32 count);
JITINT32        System_Text_DefaultEncoding_InternalGetChars (void* bytes, JITINT32 byteIndex, JITINT32 byteCount,  void* chars, JITINT32 charIndex);
JITINT32        System_Text_DefaultEncoding_InternalGetMaxByteCount (JITINT32 charCount);
JITINT32        System_Text_DefaultEncoding_InternalGetMaxCharCount (JITINT32 byteCount);
void *          System_Text_DefaultEncoding_InternalGetString (void* bytes, JITINT32 index, JITINT32 count);

/**
 * Get the internal code page as a string.
 *
 * If the value of codePage is 1 and we can not compute a suitable code page
 * number and returns the internal code page as string.
 *
 * @param codePage number of the code page to load, 1 to get the internal
 *                 code page
 *
 * @return the string representation of codePage, or the string representation
 *         of the internal code page if codePage is 1
 */
void* System_Text_Encoding_InternalCodePage (JITUINT32* codePage);


/****************************************************************************************************************************
*                                               Platform.RegexpMethods (5)								    *
****************************************************************************************************************************/
JITNINT         Platform_RegexpMethods_CompileInternal (void* pattern, JITINT32 flags);
JITNINT         Platform_RegexpMethods_CompileWithSyntaxInternal (void* pattern, JITINT32 syntax);
JITINT32        Platform_RegexpMethods_ExecInternal (JITNINT compiled, void* input, JITINT32 flags);
void*           Platform_RegexpMethods_MatchInternal (JITNINT compiled, void* input, JITINT32 maxMatches, JITINT32 flags, void* elemType);
void            Platform_RegexpMethods_FreeInternal (JITNINT compiled);

/*
 * Get the LOS limit
 *
 * This is a Mono internal call.
 *
 *
 * @return the maximum values for INT
 */
JITINT32 System_String_GetLOSLimit (void);


#endif
