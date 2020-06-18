/*
 * Copyright (C) 2009  Campanoni Simone, Michele Tartara
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
#include <stdio.h>
#include <ildjit_locale.h>
#include <platform_API.h>
#include <ildjit.h>

// My headers
#include <internal_calls_utilities.h>
#include <internal_calls_reflection.h>
#include <internal_calls_array.h>
#include <clr_system.h>
// End

#define EqualRange(literal1, posn1, count, literal2, posn2) \
	(!memcmp(literal1 + (posn1), \
		 literal2 + (posn2), \
		 (count) * sizeof(JITINT16)))

typedef enum {
    Mono_StringSplitOption_None = 0,
    Mono_StringSplitOption_RemoveEmptyEntries = 1
} Mono_StringSplitOption;

/*
 * Match information that may be returned by "MatchInternal".
 */
typedef struct {
    JITINT32 start;
    JITINT32 end;

} RegexMatch;

extern t_system *ildjitSystem;

JITBOOLEAN System_String_Equals (void *str1, void *str2) {
    JITINT16        *literal1;
    JITINT16        *literal2;
    JITINT32 	result;
    JITINT32 	length1;
    JITINT32 	length2;
    JITBOOLEAN 	needMemCmp;

    METHOD_BEGIN(ildjitSystem, "System.String.Equals");

    /* Initialize the variables		*/
    literal1 = NULL;
    literal2 = NULL;
    result = 0;
    needMemCmp = 1;
    length1 = 0;
    length2 = 0;

    /* Fetch the lengths of the strings	*/
    if (str1 != NULL) {
        length1 = (ildjitSystem->cliManager).CLR.stringManager.getLength(str1);
    }
    if (str2 != NULL) {
        length2 = (ildjitSystem->cliManager).CLR.stringManager.getLength(str2);
    }

    if (str1 == NULL) {
        if (str2 == NULL) {
            result = 0;
            needMemCmp = 0;
        } else {
            result = 1;
            needMemCmp = 0;
        }
    } else if (str2 == NULL) {
        result = 1;
        needMemCmp = 0;
    } else if (str1 == str2) {
        result = 0;
        needMemCmp = 0;
    } else if (length1 != length2) {
        result = 1;
        needMemCmp = 0;
    }

#ifdef DEBUG
    JITINT8 buf[DIM_BUF];
    if (str1 != NULL) {
        literal1 = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(str1);
        assert(literal1 != NULL);
        (ildjitSystem->cliManager).CLR.stringManager.fromUTF16ToUTF8(literal1, buf, (ildjitSystem->cliManager).CLR.stringManager.getLength(str1));
        PDEBUG("Internal calls: System.String.Equals:   Literal 1	= %s	Length  = %d\n", buf, (ildjitSystem->cliManager).CLR.stringManager.getLength(str1));
    } else {
        PDEBUG("Internal calls: System.String.Equals:   Literal 1	= null\n");
    }
    if (str2 != NULL) {
        literal2 = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(str2);
        assert(literal2 != NULL);
        (ildjitSystem->cliManager).CLR.stringManager.fromUTF16ToUTF8(literal2, buf, (ildjitSystem->cliManager).CLR.stringManager.getLength(str2));
        PDEBUG("Internal calls: System.String.Equals:   Literal 2	= %s	Length	= %d\n", buf, (ildjitSystem->cliManager).CLR.stringManager.getLength(str2));
    } else {
        PDEBUG("Internal calls: System.String.Equals:   Literal 2	= null\n");
    }

#endif
    if (needMemCmp) {
        assert(length1 == length2);
        literal1 = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(str1);
        assert(literal1 != NULL);
        literal2 = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(str2);
        assert(literal2 != NULL);
        result = memcmp(literal1, literal2, length1 * sizeof(JITINT16));
    }

    METHOD_END(ildjitSystem, "System.String.Equals");
    return result == 0;
}

void System_String_ctor_pcii (void **_this, JITINT16 *literal, JITINT32 startIndex, JITINT32 length) {

    /* Assertions							*/
    METHOD_BEGIN(ildjitSystem, "System.String..ctor(char *value, int startIndex, int length)");

    /* Allocate the string */
    (*_this) = (ildjitSystem->garbage_collectors).gc.allocPermanentObject((ildjitSystem->cliManager).CLR.stringManager.StringType, sizeof(JITINT16) * length);
    if ((*_this) == NULL) {
        ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "OutOfMemoryException");
    }
    assert((*_this) != NULL);

    (ildjitSystem->cliManager).CLR.stringManager.ctor_pcii((*_this), literal, startIndex, length);
    assert(_this != NULL);

    /* Exit								*/
    METHOD_END(ildjitSystem, "System.String..ctor(char *value, int startIndex, int length)");
    return;
}


//blocco unsafe non supportato
void System_String_ctor_pc (void **_this, JITINT16 *value) {
    JITINT32 value_len;

    /* Assertions					*/
    assert(ildjitSystem != NULL);
    METHOD_BEGIN(ildjitSystem, "System.String..ctor(char *value)\n");

    value_len = (ildjitSystem->cliManager).CLR.arrayManager.getArrayLength(value, 0);
    assert(value_len >= 0);

    /* Allocate the string */
    (*_this) = (ildjitSystem->garbage_collectors).gc.allocPermanentObject((ildjitSystem->cliManager).CLR.stringManager.StringType, sizeof(JITINT16) * value_len);
    if ((*_this) == NULL) {
        ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "OutOfMemoryException");
    }
    assert((*_this) != NULL);

    (ildjitSystem->cliManager).CLR.stringManager.ctor_pcii((*_this), value, 0, value_len);
    assert(_this != NULL);

    /* Exit								*/
    METHOD_END(ildjitSystem, "System.String..ctor(char *value)\n");
    return;
}

//test ok
void System_String_ctor_ci (void **_this, JITINT16 c, JITINT32 count) {
    JITINT32 i;
    JITINT16        *buf;

    METHOD_BEGIN(ildjitSystem, "System.String..ctor(char c, int value)");

    buf = (JITINT16*) allocMemory(count * sizeof(JITINT16));
    assert(buf != NULL);

    for (i = 0; i < count; i++) {
        buf[i] = c;
    }

    /* Allocate the string */
    (*_this) = (ildjitSystem->garbage_collectors).gc.allocPermanentObject((ildjitSystem->cliManager).CLR.stringManager.StringType, sizeof(JITINT16) * count);
    if ((*_this) == NULL) {
        ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "OutOfMemoryException");
    }
    assert((*_this) != NULL);

    (ildjitSystem->cliManager).CLR.stringManager.ctor_pcii((*_this), buf, 0, count);
    assert(_this != NULL);

    freeMemory(buf);

    // Exit
    METHOD_END(ildjitSystem, "System.String..ctor(char c, int value)");
    return;
}

void System_String_ctor_ac (void **_this, void *valueArray) {
    JITINT16*       w;
    JITINT32 	w_len;
#ifdef PRINTDEBUG
    JITINT8 	buf[DIM_BUF];
    JITINT16*       literal;
#endif

    /* Assertions					*/
    assert(_this != NULL);
    METHOD_BEGIN(ildjitSystem, "System.String..ctor(char[] value)");

    if (valueArray != NULL) {
        w = (ildjitSystem->cliManager).CLR.arrayManager.getArrayElement(valueArray, 0);
        assert(w != NULL);
        w_len = (ildjitSystem->cliManager).CLR.arrayManager.getArrayLength(valueArray, 0);
        assert(w_len >= 0);

        /* Allocate the string */
        (*_this) = (ildjitSystem->garbage_collectors).gc.allocPermanentObject((ildjitSystem->cliManager).CLR.stringManager.StringType, sizeof(JITINT16) * w_len);
        if ((*_this) == NULL) {
            ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "OutOfMemoryException");
        }
        assert((*_this) != NULL);

        (ildjitSystem->cliManager).CLR.stringManager.ctor_pcii((*_this), w, 0, w_len);
    }

    /* Print the literal				*/
#ifdef PRINTDEBUG
    literal = (ildjitSystem->cliManager).CLR.stringManager.toLiteral((*_this));
    PDEBUG("RUNTIME: Internal call: \"System.String.ctor_ac\":      string = %p\n", _this);
    PDEBUG("RUNTIME: Internal call: \"System.String.ctor_ac\":      literal = %p\n", literal);
    (ildjitSystem->cliManager).CLR.stringManager.printLiteral((*_this), buf);
    PDEBUG("RUNTIME: Internal call: \"System.String.ctor_ac\":      String to copy = %s\n", buf);
#endif

    /* Exit						*/
    METHOD_END(ildjitSystem, "System.String..ctor(char[] value)");
    return;
}

void System_String_ctor_acii (void **_this, void *valueArray, JITINT32 startIndex, JITINT32 length) {
    JITINT16*       w;

    assert(valueArray != NULL);
    METHOD_BEGIN(ildjitSystem, "System.String..ctor(char[] value, int startIndex, int length)");

    w = (ildjitSystem->cliManager).CLR.arrayManager.getArrayElement(valueArray, 0);
    assert(w != NULL);

    /* Allocate the string */
    (*_this) = (ildjitSystem->garbage_collectors).gc.allocPermanentObject((ildjitSystem->cliManager).CLR.stringManager.StringType, sizeof(JITINT16) * length);
    if ((*_this) == NULL) {
        ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "OutOfMemoryException");
    }
    assert((*_this) != NULL);

    (ildjitSystem->cliManager).CLR.stringManager.ctor_pcii((*_this), w, startIndex, length);
    assert(_this != NULL);

    /* Exit								*/
    METHOD_END(ildjitSystem, "System.String..ctor(char[] value, int startIndex, int length)");
    return;
}

void System_String_ctor_pbii (void **_this, JITINT8 *value, JITINT32 startIndex, JITINT32 length) {
    JITINT16*       literal;

    /* Assertions		*/
    assert(_this != NULL);
    METHOD_BEGIN(ildjitSystem, "System.String..ctor(sbyte *value, int startIndex, int length)");

    literal = allocMemory((length + 1) * 2);
    (ildjitSystem->cliManager).CLR.stringManager.fromUTF8ToUTF16(value, literal, length);

    /* Allocate the string */
    (*_this) = (ildjitSystem->garbage_collectors).gc.allocPermanentObject((ildjitSystem->cliManager).CLR.stringManager.StringType, sizeof(JITINT16) * length);
    if ((*_this) == NULL) {
        ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "OutOfMemoryException");
    }
    assert((*_this) != NULL);

    (ildjitSystem->cliManager).CLR.stringManager.ctor_pcii((*_this), literal, startIndex, length);
    assert(_this != NULL);

    /* Free the memory		*/
    freeMemory(literal);

    /* Exit								*/
    METHOD_END(ildjitSystem, "System.String..ctor(sbyte *value, int startIndex, int length)");
    return;
}

void System_String_ctor_pb (void **_this, JITINT8 *value) {
    JITINT16*       literal;
    JITINT32 length = 0;

    assert(_this != NULL);
    METHOD_BEGIN(ildjitSystem, "System.String..ctor(sbyte *value)");

    length = strlen((char *) value);   //trova la lunghezza di value (terminato da /0)
    assert(length >= 0);

    literal = allocMemory((length + 1) * 2);
    (ildjitSystem->cliManager).CLR.stringManager.fromUTF8ToUTF16(value, literal, length);
    assert(literal != NULL);

    /* Allocate the string */
    (*_this) = (ildjitSystem->garbage_collectors).gc.allocPermanentObject((ildjitSystem->cliManager).CLR.stringManager.StringType, sizeof(JITINT16) * length);
    if ((*_this) == NULL) {
        ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "OutOfMemoryException");
    }
    assert((*_this) != NULL);

    (ildjitSystem->cliManager).CLR.stringManager.ctor_pcii((*_this), literal, 0, length);
    assert(_this != NULL);

    /* Free the memory		*/
    freeMemory(literal);

    /* Exit								*/
    METHOD_END(ildjitSystem, "System.String..ctor(sbyte *value)");
    return;
}

void * System_String_Trim (void *_this, void *trimCharsArray, JITINT32 trimFlags) {
    void            *obj;


    /* Assertions			*/
    assert(_this != NULL);
    assert(trimCharsArray != NULL);
    METHOD_BEGIN(ildjitSystem, "System.String.Trim");

    obj = (ildjitSystem->cliManager).CLR.stringManager.trim(_this, trimCharsArray, trimFlags);
    assert(obj != NULL);

    /* Return			*/
    METHOD_END(ildjitSystem, "System.String.Trim");
    return obj;
}

JITINT32 System_String_GetHashCode (void *_this) {
    JITINT32 hash;

    METHOD_BEGIN(ildjitSystem, "System.String.GetHashCode");

    hash = (ildjitSystem->cliManager).CLR.stringManager.hashFunction(_this);

    /* Return			*/
    METHOD_END(ildjitSystem, "System.String.GetHashCode");
    return hash;
}

JITINT16 System_String_GetChar (void *string, JITINT32 posn) {
    JITINT16 c;
    JITINT16        *literal;

    /* Assertions			*/
    assert(string != NULL);
    assert(posn >= 0);

    METHOD_BEGIN(ildjitSystem, "System.String.GetChar");

    /* Check the position		*/
    if (0 <= posn && posn < (ildjitSystem->cliManager).CLR.stringManager.getLength(string)) { /* Get the character		*/
        literal = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(string);
        assert(literal != NULL);
        c = literal[posn];

    } else {
        ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "IndexOutOfRangeException");
        abort();
    }

    /* Exit				*/
    METHOD_END(ildjitSystem, "System.String.GetChar");
    return c;
}

void System_String_SetChar (void *_this, JITINT32 posn, JITINT16 value) {       //chiamata da (internal class Latin1Encoding : Encoding).getString()
    JITINT16        *buf;
    JITINT32 buf_len;

    METHOD_BEGIN(ildjitSystem, "System.String.SetChar");

    buf_len = (ildjitSystem->cliManager).CLR.stringManager.getLength(_this);
    assert(buf_len >= 0);
    buf = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(_this);
    assert(buf != NULL);

    if (posn >= 0 && posn < buf_len) {
        buf[posn] = value;
    } else {
        ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "IndexOutOfRangeException");
    }

    /* Return			*/
    METHOD_END(ildjitSystem, "System.String.SetChar");
    return;
}

//chiamata da String.CompareOrdinal(); //TEST OK
JITINT32 System_String_InternalOrdinal (void *strA, JITINT32 indexA, JITINT32 lengthA, void *strB, JITINT32 indexB, JITINT32 lengthB) {
    JITINT32 result = 0;
    JITINT16        *bufA;
    JITINT16        *bufB;

    METHOD_BEGIN(ildjitSystem, "System.String.InternalOrdinal");

    /* Handle the easy cases first */
    if (!strA) {
        if (!strB) {
            result = 0;
        } else {
            result = -1;
        }
    } else if (!strB) {
        result = 1;
    } else {
        /* Compare the two strings */
        bufA = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(strA) + indexA;
        assert(bufA != NULL);
        bufB = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(strB) + indexB;
        assert(bufB != NULL);

        while (lengthA > 0 && lengthB > 0) {
            result = (*bufA) - (*bufB);

            if (result) {
                break;
            }

            ++bufA;
            ++bufB;
            --lengthA;
            --lengthB;
        }

        if (result == 0) { /* Determine the ordering based on the tail sections */
            if (lengthA > 0) {
                result = 1;
            } else if (lengthB > 0) {
                result = -1;
            } else {
                result = 0;
            }
        }
    }
    /* Return			*/
    METHOD_END(ildjitSystem, "System.String.InternalOrdinal");
    return result;
}

//chiamata da (public class CompareInfo : IDeserializationCallback).DefaultCompare) file pnetlib/runtime/System/Globalization/CompareInfo.cs
JITINT32 System_String_CompareInternal (void *strA, JITINT32 indexA, JITINT32 lengthA, void *strB, JITINT32 indexB, JITINT32 lengthB, JITBOOLEAN ignoreCase) {
    JITINT32 result;

    METHOD_BEGIN(ildjitSystem, "System.String.CompareInternal");

    result = (ildjitSystem->cliManager).CLR.stringManager.compareStrings(strA, indexA, lengthA, strB, indexB, lengthB, ignoreCase);

    /* Return			*/
    METHOD_END(ildjitSystem, "System.String.CompareInternal");
    return result;
}

JITINT32 System_String_Compare (void *str1, void *str2) { //TEST OK
    JITINT32 result;
    JITINT32 length1 = 0;
    JITINT32 length2 = 0;

    METHOD_BEGIN(ildjitSystem, "System.String.Compare");

    if (str1) {
        length1 = (ildjitSystem->cliManager).CLR.stringManager.getLength(str1);
    }
    assert(length1 >= 0);
    if (str2) {
        length2 = (ildjitSystem->cliManager).CLR.stringManager.getLength(str2);
    }
    assert(length2 >= 0);

    result = (ildjitSystem->cliManager).CLR.stringManager.compareStrings(str1, 0, length1, str2, 0, length2, 0);

    /* Return			*/
    METHOD_END(ildjitSystem, "System.String.Compare");
    return result;
}

void * System_String_ReplaceStringString (void *_this, void *oldValue, void *newValue) {
    JITINT32 posn;
    JITBOOLEAN foundMatch = 0;
    JITINT32 old_length;
    JITINT32 new_length;
    JITINT32 this_length;
    JITINT16*       this_literal;
    JITINT16*       old_literal;
    JITINT16*       new_literal;
    void            *result;
    JITINT16        *buf;
    JITINT32 finalLen;

    /* Assertions			*/
    assert(_this != NULL);
    METHOD_BEGIN(ildjitSystem, "System.String.Replace(String str1, String str2)");

    /* Initialize the variables	*/
    new_literal = NULL;

    this_literal = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(_this);
    assert(this_literal != NULL);
    this_length = (ildjitSystem->cliManager).CLR.stringManager.getLength(_this);
    assert(this_length >= 0);

    old_literal = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(oldValue);
    assert(old_literal != NULL);
    old_length = (ildjitSystem->cliManager).CLR.stringManager.getLength(oldValue);
    assert(old_length >= 0);


    /* Validate the parameters */
    if (oldValue == NULL) {
        ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "NullReferenceException");
        result = _this;
    } else {
        /* If "oldValue" is an empty string, then the
           string will not be changed */
        if (old_length == 0) {
            result = _this;
        } else {

            /* Get the length of the old and new values */
            if (newValue) {
                new_literal = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(newValue);
                assert(new_literal != NULL);
                new_length = (ildjitSystem->cliManager).CLR.stringManager.getLength(newValue);
                assert(new_length >= 0);
            } else {
                new_length = 0;
            }

            /* Determine the length of the final string */
            finalLen = 0;
            posn = 0;
            while ((posn + old_length) <= this_length) {
                if (EqualRange(this_literal, posn, old_length, old_literal, 0)) {
                    finalLen += new_length;
                    posn += old_length;
                    foundMatch = 1;
                } else {
                    ++finalLen;
                    ++posn;
                }
            }
            if (!foundMatch) { /* no match found */
                result = _this;
            } else {
                finalLen += this_length - posn;

                buf = (JITINT16*) allocMemory(finalLen * sizeof(JITINT16));
                assert(buf != NULL);

                /* Scan the input string again and perform the replacement */
                finalLen = 0;
                posn = 0;
                while ((posn) < this_length) {
                    if ((posn + old_length) <= this_length && EqualRange(this_literal, posn, old_length, old_literal, 0)) {
                        if (new_length > 0) {
                            memcpy(buf + finalLen, new_literal, new_length * sizeof(JITINT16));
                        }
                        finalLen += new_length;
                        posn += old_length;
                    } else {
                        buf[finalLen++] = this_literal[posn++];
                    }
                }

                result = (ildjitSystem->cliManager).CLR.stringManager.newInstance(buf, finalLen);
                assert(result != NULL);

                freeMemory(buf);
            }
        }
    }
    /* Return the final replaced string to the caller */
    METHOD_END(ildjitSystem, "System.String.Replace(String str1, String str2)");
    return result;
}

void * System_String_Replace_ccii (void *_this, JITINT16 oldChar, JITINT16 newChar, JITINT32 startIndex, JITINT32 count) {
    void            *result;

    JITINT16* result_literal;
    JITINT16* this_literal;
    JITINT32 this_length;
    JITINT32 i = 0;

    /* Assertions			*/
    assert(_this != NULL);
    METHOD_BEGIN(ildjitSystem, "System.String.Replace(char oldChar, char newChar, int startIndex, int count)");

    this_length = (ildjitSystem->cliManager).CLR.stringManager.getLength(_this);
    assert(this_length >= 0);

    if (startIndex < 0 || startIndex >= this_length || count < 0 || count + startIndex >= this_length) {
        ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "ArgumentOutOfRangeException");
        result = NULL;
    } else if (oldChar == newChar || this_length == 0) {
        result = _this;
    } else {
        this_literal = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(_this);
        assert(this_literal != NULL);

        result_literal = (JITINT16*) allocMemory(count * sizeof(JITINT16));

        for (i = startIndex; i < count; i++) {
            if (this_literal[i] == oldChar) {
                result_literal[i] = newChar;
            } else {
                result_literal[i] = this_literal[i];
            }
        }

        result = (ildjitSystem->cliManager).CLR.stringManager.newInstance(result_literal, count);
        assert(result != NULL);

        freeMemory(result_literal);
    }
    /* Return			*/
    METHOD_END(ildjitSystem, "System.String.Replace(char oldChar, char newChar, int startIndex, int count)");
    return result;
}

void * System_String_Replace (void *_this, JITINT16 oldChar, JITINT16 newChar) { //TEST OK
    void            *result;

    JITINT16* result_literal;
    JITINT16* this_literal;
    JITINT32 this_length;
    JITINT32 i = 0;

    /* Assertions			*/
    assert(_this != NULL);
    METHOD_BEGIN(ildjitSystem, "System.String.Replace(char oldChar, char newChar)");

    this_length = (ildjitSystem->cliManager).CLR.stringManager.getLength(_this);
    assert(this_length >= 0);

    /* If nothing will happen, then return the current string as-is */
    if (oldChar == newChar || this_length == 0) {
        result = _this;
    } else {
        this_literal = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(_this);
        assert(this_literal != NULL);

        result_literal = (JITINT16*) allocMemory(this_length * sizeof(JITINT16));

        for (i = 0; i < this_length; i++) {
            if (this_literal[i] == oldChar) {
                result_literal[i] = newChar;
            } else {
                result_literal[i] = this_literal[i];
            }
        }

        result = (ildjitSystem->cliManager).CLR.stringManager.newInstance(result_literal, this_length);
        assert(result != NULL);

        freeMemory(result_literal);
    }
    /* Return			*/
    METHOD_END(ildjitSystem, "System.String.Replace(char oldChar, char newChar)");
    return result;
}

JITINT32 System_String_IndexOf (void *string, JITINT16 value, JITINT32 startIndex, JITINT32 count) { //TEST OK
    JITINT32 result = -1;
    JITINT16        *literal;
    JITINT32 len;
    JITINT32 i;

    /* Assertions			*/
    assert(string != NULL);
    METHOD_BEGIN(ildjitSystem, "System.String.IndexOf");

    len = (ildjitSystem->cliManager).CLR.stringManager.getLength(string);
    assert(len >= 0);


    /* Check the arguments		*/
    if (    (startIndex < 0)        ||
            (count < 0)             ||
            (len - startIndex) < count) {
        //ILDJIT_throwExceptionWithName("System", "ArgumentOutOfRangeException");
    } else {
        /* Search the character		*/
        literal = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(string);
        assert(literal != NULL);

        for (i = startIndex; i < startIndex + count; i++) {
            if (literal[i] == value) {
                result = i;
                break;
            }
        }
    }
    /* Exit				*/
    METHOD_END(ildjitSystem, "System.String.IndexOf");
    return result;
}

JITINT32 System_String_IndexOfAny (void *string, void *array, JITINT32 startIndex, JITINT32 count) { //TEST OK
    JITINT32 arrayLength;
    JITINT32 j;
    JITINT32 i;
    JITINT16        *literal;
    JITINT16        *arrayBuf;
    JITINT32 len;

    /* Assertions			*/
    assert(string != NULL);
    assert(array != NULL);
    METHOD_BEGIN(ildjitSystem, "System.String.IndexOfAny");

    len = (ildjitSystem->cliManager).CLR.stringManager.getLength(string);
    assert(len >= 0);

    /* Check the arguments		*/
    if (    (startIndex < 0)        ||
            (count < 0)             ||
            (len - startIndex) < count) {
        ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "ArgumentOutOfRangeException");
    } else {
        arrayLength = (ildjitSystem->cliManager).CLR.arrayManager.getArrayLength(array, 0);
        assert(arrayLength >= 0);
        arrayBuf = (ildjitSystem->cliManager).CLR.arrayManager.getArrayElement(array, 0);
        assert(arrayBuf != NULL);

        if (arrayLength == 0) {
            METHOD_END(ildjitSystem, "System.String.IndexOfAny");
            return -1;
        }

        /* Get the character		*/
        literal = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(string);
        assert(literal != NULL);

        for (i = startIndex; i < startIndex + count; i++) {
            for (j = 0; j < arrayLength; j++) {
                if ( literal[i] == arrayBuf[j] ) {
                    METHOD_END(ildjitSystem, "System.String.IndexOfAny");
                    return i;
                }
            }
        }
    }
    /* Exit				*/
    METHOD_END(ildjitSystem, "System.String.IndexOfAny");
    return -1;
}

JITINT32 System_String_LastIndexOf (void *_this, JITINT16 value, JITINT32 startIndex, JITINT32 count) { //TEST OK
    JITINT16        *buf;
    JITINT32 len;

    /* Assertions			*/
    assert(_this != NULL);
    METHOD_BEGIN(ildjitSystem, "System.String.LastIndexOf");

    /* Validate the parameters */
    if (startIndex < 0 || count < 0 || (startIndex - count + 1 ) < 0) {
        ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "IndexOutOfRangeException");
        METHOD_END(ildjitSystem, "System.String.LastIndexOf");
        return -1;
    }

    len = (ildjitSystem->cliManager).CLR.stringManager.getLength(_this);
    assert(len >= 0);

    /* Adjust for overflow */
    if (startIndex >= len) {
        count -= startIndex - (len - 1);
        startIndex = len - 1;
    }

    /* Search for the value */
    buf = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(_this) + startIndex;
    assert(buf != NULL);

    while (count > 0) {
        if (*buf-- == value) {
            METHOD_END(ildjitSystem, "System.String.LastIndexOf");
            return startIndex;
        }
        --startIndex;
        --count;
    }

    METHOD_END(ildjitSystem, "System.String.LastIndexOf");
    return -1;
}

JITINT32 System_String_LastIndexOfAny (void *_this, void *anyOfArray, JITINT32 startIndex, JITINT32 count) { //TEST OK
    JITINT16        *buf;
    JITINT16        *anyBuf;
    JITINT32 anyLength;
    JITINT32 i;
    JITINT32 len;

    /* Assertions			*/
    assert(_this != NULL);
    METHOD_BEGIN(ildjitSystem, "System.String.LastIndexOfAny");

    /* Validate the parameters */
    if (anyOfArray == NULL) {
        ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "NullReferenceException");
        METHOD_END(ildjitSystem, "System.String.LastIndexOfAny");
        return -1;
    }
    if (startIndex < 0 || count < 0 || startIndex - count + 1 < 0) {
        ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "IndexOutOfRangeException");
        METHOD_END(ildjitSystem, "System.String.LastIndexOfAny");
        return -1;
    }

    /* Get the start and extent of the "anyOf" array */
    anyBuf = (ildjitSystem->cliManager).CLR.arrayManager.getArrayElement(anyOfArray, 0);
    assert(anyBuf != NULL);
    anyLength = (ildjitSystem->cliManager).CLR.arrayManager.getArrayLength(anyOfArray, 0);
    assert(anyLength >= 0);

    if (anyLength == 0) {     /* Bail out because there is nothing to find */
        return -1;
    }

    len = (ildjitSystem->cliManager).CLR.stringManager.getLength(_this);
    assert(len >= 0);

    /* Adjust for overflow */
    if (startIndex >= len) {
        count -= startIndex - (len - 1);
        startIndex = len - 1;
    }

    /* Search for the value */
    buf = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(_this) + startIndex;
    assert(buf != NULL);

    while (count > 0) {
        for (i = 0; i < anyLength; i++) {
            if (*buf == anyBuf[i]) {
                METHOD_END(ildjitSystem, "System.String.LastIndexOfAny");
                return startIndex;
            }
        }
        --buf;
        --startIndex;
        --count;
    }

    METHOD_END(ildjitSystem, "System.String.LastIndexOfAny");
    return -1;

}

//TEST OK chiamata da String.Copy() attraverso System_String_Copy_StringiString
void System_String_Copy_StringiStringii (void *stringDest, JITINT32 destPos, void *stringSrc, JITINT32 srcPos, JITINT32 length) {
    JITINT16        *literalDest;
    JITINT16        *literalSrc;

    /* Assertions			*/
    assert(stringDest != NULL);
    assert(stringSrc != NULL);
    METHOD_BEGIN(ildjitSystem, "System.String.Copy(String dest, int destPos, String src, int srcPos, int length");

    literalSrc = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(stringSrc);
    assert(literalSrc != NULL);
    literalDest = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(stringDest);
    assert(literalDest != NULL);

    memcpy(literalDest + destPos, literalSrc + srcPos, length * sizeof(JITINT16));

    /* Return			*/
    METHOD_END(ildjitSystem, "System.String.Copy(String dest, int destPos, String src, int srcPos, int length");
    return;
}

// TEST OK chiamata da String.Copy()
void System_String_Copy_StringiString (void *stringDest, JITINT32 destPos, void *stringSrc) {
    JITINT16        *literalDest;
    JITINT16        *literalSrc;
    JITINT32 lengthSrc;

    /* Assertions			*/
    assert(stringDest != NULL);
    assert(stringSrc != NULL);
    METHOD_BEGIN(ildjitSystem, "System.String.Copy(String dest, int destPos, String src)");

    literalSrc = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(stringSrc);
    assert(literalSrc != NULL);
    lengthSrc = (ildjitSystem->cliManager).CLR.stringManager.getLength(stringSrc);
    assert(lengthSrc >= 0);
    literalDest = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(stringDest);
    assert(literalDest != NULL);

    memcpy(literalDest + destPos, literalSrc, lengthSrc * sizeof(JITINT16));

    /* Return			*/
    METHOD_END(ildjitSystem, "System.String.Copy(String dest, int destPos, String src)");
    return;
}

// chiamato in più metodi di StringBuilder (anche dai suoi costruttori)
void * System_String_NewBuilder (void *string, JITINT32 length) {
    void            *newString;

    METHOD_BEGIN(ildjitSystem, "System.String.NewBuilder");

    newString = (ildjitSystem->cliManager).CLR.stringManager.newBuilder(string, length);
    assert(newString != NULL);

    /* Return			*/
    METHOD_END(ildjitSystem, "System.String.NewBuilder");
    return newString;
}

//chiamato in molti metodi di String e StringBuilder
void * System_String_NewString (JITINT32 length) {
    void            *string;

    METHOD_BEGIN(ildjitSystem, "System.String.NewString");

    string = (ildjitSystem->cliManager).CLR.stringManager.newInstanceEmpty(length);
    assert(string != NULL);

    /* Return			*/
    METHOD_END(ildjitSystem, "System.String.NewString");
    return string;
}

void System_String_CopyToChecked (void *string, JITINT32 sourceIndex, void *array, JITINT32 destinationIndex, JITINT32 count) {
    JITINT16        *literal;
    void            *destination;

    /* Assertions					*/
    assert(string != NULL);
    assert(array != NULL);
    METHOD_BEGIN(ildjitSystem, "System.String.CopyToChecked");
    assert((ildjitSystem->cliManager).CLR.stringManager.getLength(string) >= (sourceIndex + count));
    assert((ildjitSystem->cliManager).CLR.arrayManager.getArrayLength(array, 0) >= (destinationIndex + count));

    // Initialize the variables
    destination = NULL;

    // Print the String to copy
#ifdef DEBUG
    JITINT8 buf[DIM_BUF];
    buf[0] = '\0';
    PDEBUG("\nenter in copytochecked \nRUNTIME: Internal call: \"System.String.CopyToChecked\":     string = %p\n", string);
    (ildjitSystem->cliManager).CLR.stringManager.printLiteral(string, buf);
    PDEBUG("RUNTIME: Internal call: \"System.String.CopyToChecked\":        String to copy = %s\n", buf);
    PDEBUG("RUNTIME: Internal call: \"System.String.CopyToChecked\":        Destination index       = %d\n", destinationIndex);
#endif

    /* Fetch the destination address	*/
    destination = (ildjitSystem->cliManager).CLR.arrayManager.getArrayElement(array, destinationIndex);
    assert(destination != NULL);

    /* Fetch the literal of the string	*/
    PDEBUG("RUNTIME: Internal call: \"System.String.CopyToChecked\":        Fetch the literal string\n");
    literal = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(string);
    PDEBUG("RUNTIME: Internal call: \"System.String.CopyToChecked\":        literal = %p\n", literal);
    assert(literal != NULL);

    // Copy the elements inside the array
    memcpy(destination, &(literal[sourceIndex]), count * sizeof(JITUINT16));

    // Return
    METHOD_END(ildjitSystem, "System.String.CopyToChecked");
    return;
}

// TEST OK chiamato da LastIndexOf
JITINT32 System_String_FindInRange (void *this, JITINT32 srcFirst, JITINT32 srcLast, JITINT32 srcStep, void *dest) {
    JITINT16        *literal;
    JITINT16        *this_literal;
    JITINT32 length;

    /* Assertions				*/
    assert(this != NULL);
    assert(dest != NULL);
    METHOD_BEGIN(ildjitSystem, "System.String.FindInRange");

    /* Fetch the literal			*/
    literal = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(dest);
    assert(literal != NULL);
    this_literal = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(this);
    assert(this_literal != NULL);
    length = (ildjitSystem->cliManager).CLR.stringManager.getLength(dest);

    /* Drop out the first srcFirst character*
     * of the this string			*/
    this_literal = this_literal + srcFirst;
    assert(this_literal != NULL);

    /* Check the destination string		*/
    if (length == 0) {
        METHOD_END(ildjitSystem, "System.String.FindInRange");
        return srcFirst;
    }

    if (srcStep > 0) {

        /* Scan forward to the string	*/
        if (length == 1) {
            while (srcFirst <= srcLast) {
                if (this_literal[0] == literal[0]) {
                    METHOD_END(ildjitSystem, "System.String.FindInRange");
                    return srcFirst;
                }

                this_literal++;
                srcFirst++;
            }
        } else {
            while (srcFirst <= srcLast) {
                if (    (this_literal[0] == literal[0])                         &&
                        (!memcmp(this_literal, literal, sizeof(JITINT16) * length))     ) {
                    METHOD_END(ildjitSystem, "System.String.FindInRange");
                    return srcFirst;
                }

                this_literal++;
                srcFirst++;
            }
        }
        METHOD_END(ildjitSystem, "System.String.FindInRange");
        return -1;
    } else {

        /* Scan backwards for the string	*/
        if (length == 1) {
            while (srcFirst >= srcLast) {
                if (this_literal[0] == literal[0]) {
                    METHOD_END(ildjitSystem, "System.String.FindInRange");
                    return srcFirst;
                }

                this_literal--;
                srcFirst--;
            }
        } else {
            while (srcFirst >= srcLast) {
                if (    (this_literal[0] == literal[0])                         &&
                        (!memcmp(this_literal, literal, sizeof(JITINT16) * length))     ) {
                    METHOD_END(ildjitSystem, "System.String.FindInRange");
                    return srcFirst;
                }

                this_literal--;
                srcFirst--;
            }
        }
        METHOD_END(ildjitSystem, "System.String.FindInRange");
        return -1;
    }

    METHOD_END(ildjitSystem, "System.String.FindInRange");
    return -1;
}

//TEST OK chiamato da String.Concat(String, String, String)
void * System_String_ConcatString3 (void *str1, void *str2, void *str3) {
    void            *newObject;

    METHOD_BEGIN(ildjitSystem, "System.String.Concat(String str1, String str2, String str3)");

    /* Make a new System.String     *
     * object			*/
    newObject = (ildjitSystem->cliManager).CLR.stringManager.concatStrings(str1, str2);
    assert(newObject != NULL);
    newObject = (ildjitSystem->cliManager).CLR.stringManager.concatStrings(newObject, str3);
    assert(newObject != NULL);

    /* Exit					*/
    METHOD_END(ildjitSystem, "System.String.Concat(String str1, String str2, String str3)");
    return newObject;
}

void * System_String_Concat (void *str1, void *str2) {
    void                    *newObject;

    METHOD_BEGIN(ildjitSystem, "System.String.Concat(String str1, String str2)");

    /* Make a new System.String     *
     * object			*/
    newObject = (ildjitSystem->cliManager).CLR.stringManager.concatStrings(str1, str2);
    assert(newObject != NULL);

    /* Return the new string	*/
    METHOD_END(ildjitSystem, "System.String.Concat(String str1, String str2)");
    return newObject;
}

void System_String_InsertSpace (void *str, JITINT32 srcPos, JITINT32 destPos) {
    JITINT16        *str_literal;
    JITINT16        *new_literal;
    JITINT32 str_len;
    JITINT32 new_len;

    /* Assertions							*/
    METHOD_BEGIN(ildjitSystem, "System.String.InsertSpace");

    str_literal = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(str);
    assert(str_literal != NULL);

    str_len = (ildjitSystem->cliManager).CLR.stringManager.getLength(str);
    assert(str_len >= 0);

    assert(destPos >= 0);
    assert(srcPos >= 0);
    assert(srcPos < str_len);

    //costruire una nuova stringa...
    if (destPos - srcPos > 0) {
        new_len = str_len + destPos - srcPos;
    } else {
        new_len = str_len;
    }

    new_literal = (JITINT16*) allocMemory((new_len) * sizeof(JITINT16));

    memcpy(new_literal, str_literal, str_len);
    memmove(new_literal + destPos, new_literal + srcPos, (str_len - srcPos) * sizeof(JITINT16));

    (ildjitSystem->cliManager).CLR.stringManager.ctor_pcii(str, new_literal, 0, new_len);
    assert(str != NULL);

    freeMemory(new_literal);

    /* Exit								*/
    METHOD_END(ildjitSystem, "System.String.InsertSpace");
    return;
}

//chiamata da StringBuilder.Remove()
void  System_String_RemoveSpace (void *str, JITINT32 index, JITINT32 length) {
    JITINT16        *str_literal;
    JITINT16        *new_literal;
    JITINT32 str_len;
    JITINT32 new_len;

    /* Assertions							*/
    METHOD_BEGIN(ildjitSystem, "System.String.RemoveSpace");

    str_literal = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(str);
    str_len = (ildjitSystem->cliManager).CLR.stringManager.getLength(str);
    assert(str_literal != NULL);
    assert(str_len >= 0);
    new_len = str_len - length;
    if (new_len > 0) {
        new_literal = (JITINT16*) allocMemory((new_len) * sizeof(JITINT16));

        memmove(new_literal + index, str_literal + index + length, (str_len - (index + length)) * sizeof(JITINT16));

        (ildjitSystem->cliManager).CLR.stringManager.ctor_pcii(str, new_literal, 0, new_len);
        assert(str != NULL);

        freeMemory(new_literal);
    } else {
        assert(new_len == 0);
        (ildjitSystem->cliManager).CLR.stringManager.ctor_pcii(str, NULL, 0, 0);
    }

    /* Exit								*/
    METHOD_END(ildjitSystem, "System.String.RemoveSpace");
    return;
}

//TEST OK
void* System_String_Intern (void *str) {
    void            *ret;

    /* Assertions							*/
    METHOD_BEGIN(ildjitSystem, "System.String.Intern");

    ret = (ildjitSystem->cliManager).CLR.stringManager.intern(str);
    assert(ret != NULL);

    /* Exit								*/
    METHOD_END(ildjitSystem, "System.String.Intern");
    return ret;
}

//TEST OK
void* System_String_IsInterned (void* str) {
    void            *ret;

    /* Assertions							*/
    METHOD_BEGIN(ildjitSystem, "System.String.IsInterned");

    ret = (ildjitSystem->cliManager).CLR.stringManager.isInterned(str);

    /* Exit								*/
    METHOD_END(ildjitSystem, "System.String.IsInterned");
    return ret;
}

//TEST OK chiamata da String.PadLeft e String.PadRight
void System_String_CharFill_siic (void *str, JITINT32 start, JITINT32 count, JITINT16 ch) {
    JITINT16        *str_literal;

    /* Assertions							*/
    METHOD_BEGIN(ildjitSystem, "System.String.CharFill(String str, int start, int count, char ch)");

    str_literal = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(str);

    while (count > 0) {
        str_literal[start] = ch;
        --count;
        ++start;
    }

    /* Exit								*/
    METHOD_END(ildjitSystem, "System.String.CharFill(String str, int start, int count, char ch)");
    return;
}

//chiamata da chi????
void System_String_CharFill_siacii (void *str, JITINT32 start, void *chars, JITINT32 index, JITINT32 count) {
    JITINT16        *src;
    JITINT16        *dest;
    JITINT32 i;
    JITINT32 j;

    /* Assertions							*/
    METHOD_BEGIN(ildjitSystem, "System.String.CharFill(String str, int start, char[] chars, int index, int count)");

    src = (ildjitSystem->cliManager).CLR.arrayManager.getArrayElement(chars, 0) + index;
    dest = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(str) + start;
    i = index;
    j = start;
    while (count > 0) {
        dest[j] = src[i];
        j++;
        i++;
        --count;
    }

    /* Exit								*/
    METHOD_END(ildjitSystem, "System.String.CharFill(String str, int start, char[] chars, int index, int count)");
    return;
}

/* Split self around separators */
void* System_String_InternalSplit (void* self, void* separators,
                                   JITUINT32 count, JITUINT32 options) {
    t_stringManager* stringManager;
    t_arrayManager* arrayManager;
    TypeDescriptor *stringType;
    char* string;
    char* utf8Separators;
    JITUINT32 separatorCount;
    JITUINT32 i;
    char** bufferPointer;
    char* token;
    void* tokenAsString;
    XanList* tokenList;
    void* tokens;

    stringManager = &((ildjitSystem->cliManager).CLR.stringManager);
    arrayManager = &((ildjitSystem->cliManager).CLR.arrayManager);

    METHOD_BEGIN(ildjitSystem, "System.String.InternalSplit");

    string = (char*) stringManager->getUTF8copy(self);
    stringType = stringManager->fillILStringType();
    separatorCount = arrayManager->getArrayLength(separators, 0);
    utf8Separators = allocMemory(sizeof(JITINT8) * (separatorCount + 1));

    for (i = 0; i < separatorCount; i++) {
        utf8Separators[i] =
            *((JITINT8*) arrayManager->getArrayElement(separators, i));
    }
    utf8Separators[separatorCount] = '\0';

    bufferPointer = &string;
    i = 0;
    tokenList = xanList_new(allocFunction, freeFunction, NULL);
    do {
        token = PLATFORM_strsep(bufferPointer, utf8Separators);
        tokenAsString = CLIMANAGER_StringManager_newInstanceFromUTF8((JITINT8*) token,
                        strlen(token));
        if (options != Mono_StringSplitOption_RemoveEmptyEntries ||
                strcmp(token, "") != 0) {
            xanList_append(tokenList, tokenAsString);
        }
        i++;
    } while (*bufferPointer != NULL && i < count);
    tokens = arrayManager->newInstanceFromList(tokenList, stringType);

    freeMemory(string);
    freeMemory(utf8Separators);
    xanList_destroyList(tokenList);

    METHOD_END(ildjitSystem, "System.String.InternalSplit");

    return tokens;
}

/* Allocate a new string with the given length */
void* System_String_InternalAllocateStr (JITINT32 length) {
    void* string;

    METHOD_BEGIN(ildjitSystem, "System.String.InternalAllocateStr");

    string = System_String_NewString(length);

    METHOD_END(ildjitSystem, "System.String.InternalAllocateStr");

    return string;
}

/* Checks whether string is interned */
void* System_String_InternalIsInterned (void* string) {
    void* internedString;

    METHOD_BEGIN(ildjitSystem, "System.String.InternalIsInterned");

    internedString = System_String_IsInterned(string);

    METHOD_END(ildjitSystem, "System.String.InternalIsInterned");

    return internedString;
}

//************************* System.Text.StringBuilder *************************

//test ok
JITINT32 System_Text_StringBuilder_EnsureCapacity (void *_this, JITINT32 capacity) {

    void *str;
    void *newStr;
    JITINT32 curCapacity;
    JITINT32 maxCapacity;
    JITINT32 newCapacity;
    JITINT32 result;

    METHOD_BEGIN(ildjitSystem, "System.Text.StringBuilder.EnsureCapacity");

    str = (ildjitSystem->cliManager).CLR.stringBuilderManager.getString(_this);
    assert(str != NULL);

    curCapacity = (ildjitSystem->cliManager).CLR.stringManager.getCapacity(str);
    maxCapacity = (ildjitSystem->cliManager).CLR.stringBuilderManager.getMaxCapacity(_this);
    assert(maxCapacity > 0);
    assert(curCapacity >= 0);
    assert(curCapacity <= maxCapacity);


    if (capacity < 0 || capacity > maxCapacity) {
        ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "ArgumentOutOfRangeException");
        result = 0;
    } else {
        if (capacity < curCapacity) {
            result = curCapacity;
        } else {
            newCapacity = (ildjitSystem->cliManager).CLR.stringBuilderManager.getNewCapacity(capacity, curCapacity);
            if (newCapacity > maxCapacity) {
                newCapacity = maxCapacity;
            }

            newStr = System_String_NewBuilder(str, newCapacity);

            if (!newStr) {
                result = 0;
            } else {
                (ildjitSystem->cliManager).CLR.stringBuilderManager.setString(_this, newStr);
                (ildjitSystem->cliManager).CLR.stringBuilderManager.setNeedsCopy(_this, 0);
                result = newCapacity;
            }
        }
    }
    /* Return				*/
    METHOD_END(ildjitSystem, "System.Text.StringBuilder.EnsureCapacity");
    return result;
}

//test ok
void * System_Text_StringBuilder_Append_acii1 (void *_this, void *valueArray, JITINT32 startIndex, JITINT32 length) {
    JITINT16*       w;
    JITINT32 w_len;

    METHOD_BEGIN(ildjitSystem, "System.Text.StringBuilder.Append(char[] value, int startIndex, int length)");

    if (length > 0) {
        if (valueArray != NULL) {
            w = (ildjitSystem->cliManager).CLR.arrayManager.getArrayElement(valueArray, 0);
            assert(w != NULL);
            w_len = (ildjitSystem->cliManager).CLR.arrayManager.getArrayLength(valueArray, 0);
            assert(w_len >= 0);

            if (startIndex < 0 || startIndex >= w_len || length < 0 || length + startIndex >= w_len) {
                ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "ArgumentOutOfRangeException");
                METHOD_END(ildjitSystem, "System.Text.StringBuilder.Append(char[] value, int startIndex, int length)");
                return NULL;
            }
            _this = (ildjitSystem->cliManager).CLR.stringBuilderManager.append(_this, w + startIndex, length);
        } else {
            ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "NullReferenceException");
            METHOD_END(ildjitSystem, "System.Text.StringBuilder.Append(char[] value, int startIndex, int length)");
            return NULL;
        }
    }

    /* Return				*/
    METHOD_END(ildjitSystem, "System.Text.StringBuilder.Append(char[] value, int startIndex, int length)");
    return _this;
}

void * System_Text_StringBuilder_Append_ac1 (void *_this, void *valueArray) {
    JITINT16        *w;
    JITINT32 w_len;

    METHOD_BEGIN(ildjitSystem, "System.Text.StringBuilder.Append(char[] value)");

    if (valueArray != NULL) {
        w_len = (ildjitSystem->cliManager).CLR.arrayManager.getArrayLength(valueArray, 0);
        assert(w_len >= 0);
        if (w_len > 0) {
            w = (ildjitSystem->cliManager).CLR.arrayManager.getArrayElement(valueArray, 0);
            assert(w != NULL);
            _this = (ildjitSystem->cliManager).CLR.stringBuilderManager.append(_this, w, w_len);
        }
    } else {
        ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "NullReferenceException");
        METHOD_END(ildjitSystem, "System.Text.StringBuilder.Append(char[] value)");
        return NULL;
    }

    /* Return				*/
    METHOD_END(ildjitSystem, "System.Text.StringBuilder.Append(char[] value)");
    return _this;
}

void * System_Text_StringBuilder_Append_c (void *_this, JITINT16 value) {

    /* Assertions				*/
    assert(_this != NULL);

    METHOD_BEGIN(ildjitSystem, "System.Text.StringBuilder.Append(char value)");

    _this = (ildjitSystem->cliManager).CLR.stringBuilderManager.append(_this, &value, 1);
    assert(_this != NULL);

    /* Exit				*/
    METHOD_END(ildjitSystem, "System.Text.StringBuilder.Append(char value)");
    return _this;
}

void * System_Text_StringBuilder_Append_ci (void *_this, JITINT16 value, JITINT32 repeatCount) {
    JITINT32 i;

    /* Assertions				*/
    assert(_this != NULL);

    METHOD_BEGIN(ildjitSystem, "System.Text.StringBuilder.Append(char value, int repeatCount)");

    /* Validate the arguments		*/
    if (repeatCount < 0) {
        ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "ArgumentOutOfRangeException");
        METHOD_END(ildjitSystem, "System.Text.StringBuilder.Append(char value, int repeatCount)");
        return NULL;
    }

    /* append */
    for (i = 0; i < repeatCount; i++) {
        _this = (ildjitSystem->cliManager).CLR.stringBuilderManager.append(_this, &value, 1);
        assert(_this != NULL);
    }

    /* Exit				*/
    METHOD_END(ildjitSystem, "System.Text.StringBuilder.Append(char value, int repeatCount)");
    return _this;
}

void * System_Text_StringBuilder_Append_Stringii (void *_this, void *stringValue, JITINT32 startIndex, JITINT32 length) {
    JITUINT32 stringLength;
    JITINT16*       literal;

    /* Assertions			*/
    assert(_this != NULL);

    METHOD_BEGIN(ildjitSystem, "System.Text.StringBuilder.Append(String value, int startIndex, int length)");

    if (length > 0) {
        /* Append the string to the builder	*/
        if (stringValue) {
            stringLength = (ildjitSystem->cliManager).CLR.stringManager.getLength(stringValue);
            if (stringLength > 0) {
                if (     (startIndex < 0)                        ||
                         (startIndex >= stringLength)            ||
                         (length < 0)                            ||
                         ((stringLength - startIndex) < length)  ) {
                    ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "ArgumentOutOfRangeException");
                    METHOD_END(ildjitSystem, "System.Text.StringBuilder.Append(String value, int startIndex, int length)");
                    return NULL;
                }

                literal = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(stringValue);
                assert(literal != NULL);

                _this = (ildjitSystem->cliManager).CLR.stringBuilderManager.append(_this, literal + startIndex, length);
                assert(_this != NULL);
            }
        }
    }

    /* Return				*/
    METHOD_END(ildjitSystem, "System.Text.StringBuilder.Append(String value, int startIndex, int length)");
    return _this;
}

void * System_Text_StringBuilder_Append_String (void *_this, void *string) {
    JITINT32 stringLength;
    JITINT16        *literal;

    /* Assertions			*/
    assert(_this != NULL);

    METHOD_BEGIN(ildjitSystem, "System.Text.StringBuilder.Append(String value)");

    /* Append the string to the builder	*/
    if (string) {
        stringLength = (ildjitSystem->cliManager).CLR.stringManager.getLength(string);
        assert(stringLength >= 0);

        if (stringLength > 0) {
            literal = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(string);
            assert(literal != NULL);

            _this = (ildjitSystem->cliManager).CLR.stringBuilderManager.append(_this, literal, stringLength);
            assert(_this != NULL);
        }
    }

    /* Return				*/
    METHOD_END(ildjitSystem, "System.Text.StringBuilder.Append(String value)");
    return _this;
}

void * System_Text_StringBuilder_Insert_iString (void *_this, JITINT32 index, void *stringValue) {
    JITINT32 strLength;
    JITINT16        *strLiteral;
    void            *sbString;
    JITINT16        *sbStringLiteral;

    METHOD_BEGIN(ildjitSystem, "System.Text.StringBuilder.Insert(int index, String value)");

    if (stringValue) { /*lunghezza della stringa da inserire*/
        strLength = (ildjitSystem->cliManager).CLR.stringManager.getLength(stringValue);
        assert(strLength >= 0);

        if (strLength > 0) {
            /*crea lo spazio dove copiare i caratteri della nuova stringa*/
            _this = (ildjitSystem->cliManager).CLR.stringBuilderManager.insert(_this, index, strLength);

            if (_this) { /*estrae la stringa dallo stringbuilder _this */
                sbString = (ildjitSystem->cliManager).CLR.stringBuilderManager.getString(_this);
                assert(sbString != NULL);
                sbStringLiteral = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(sbString);
                assert(sbStringLiteral != NULL);

                /*caratteri della stringa da inserire*/
                strLiteral = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(stringValue);
                assert(strLiteral != NULL);

                /*copia dei caratteri della stringa da inserire nello spazio creato*/
                memcpy(sbStringLiteral + index, strLiteral, sizeof(JITINT16) * (strLength));

            } else {
                METHOD_END(ildjitSystem, "System.Text.StringBuilder.Insert(int index, String value)");
                return NULL;
            }
        }
    }


    /* Return				*/
    METHOD_END(ildjitSystem, "System.Text.StringBuilder.Insert(int index, String value)");
    return _this;
}

void * System_Text_StringBuilder_Insert_iStringi (void *_this, JITINT32 index, void *stringValue, JITINT32 count) {
    JITINT32 strLength;
    JITINT16        *strLiteral;
    void            *sbString;
    JITINT16        *sbStringLiteral;

    METHOD_BEGIN(ildjitSystem, "System.Text.StringBuilder.Insert(int index, String value, int count)");

    if (count < 0) {
        ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "ArgumentOutOfRangeException");
        METHOD_END(ildjitSystem, "System.Text.StringBuilder.Insert(int index, String value, int count)");
        return NULL;
    }

    if (stringValue) {
        /*lunghezza della stringa da inserire*/
        strLength = (ildjitSystem->cliManager).CLR.stringManager.getLength(stringValue);
        assert(strLength >= 0);

        if (strLength > 0 && count > 0) {
            /*crea lo spazio dove copiare i caratteri della nuova stringa*/
            _this = (ildjitSystem->cliManager).CLR.stringBuilderManager.insert(_this, index, strLength * count);

            if (_this) { /*estrae la stringa dallo stringbuilder _this */
                sbString = (ildjitSystem->cliManager).CLR.stringBuilderManager.getString(_this);
                assert(sbString != NULL);
                sbStringLiteral = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(sbString);
                assert(sbStringLiteral != NULL);

                /*caratteri della stringa da inserire*/
                strLiteral = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(stringValue);
                assert(strLiteral != NULL);

                /*ripete count volte*/
                while (count > 0) {
                    /*copia dei caratteri della stringa da inserire nello spazio creato*/
                    memcpy(sbStringLiteral + index, strLiteral, sizeof(JITINT16) * (strLength));

                    /*incrementa l'indice*/
                    index += strLength;
                    count--;
                }
            } else {
                METHOD_END(ildjitSystem, "System.Text.StringBuilder.Insert(int index, String value, int count)");
                return NULL;
            }
        }
    }

    /* Return				*/
    METHOD_END(ildjitSystem, "System.Text.StringBuilder.Insert(int index, String value, int count)");
    return _this;
}

void * System_Text_StringBuilder_Insert_iacii (void *_this, JITINT32 index, void *valueArray, JITINT32 startIndex, JITINT32 length) {
    JITINT32 arrayLength;
    JITINT16        *arrayLiteral;
    void            *sbString;
    JITINT16        *sbStringLiteral;

    /* Assertions				*/
    assert(ildjitSystem != NULL);
    METHOD_BEGIN(ildjitSystem, "System.Text.StringBuilder.Insert(int index, char[] value, int startIndex, int length)");

    if (valueArray) { /*estrae la lunghezza dell'array*/
        arrayLength = (ildjitSystem->cliManager).CLR.arrayManager.getArrayLength(valueArray, 0);
        assert(arrayLength >= 0);

        if (startIndex < 0 || startIndex > arrayLength) {
            ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "ArgumentOutOfRangeException");
            METHOD_END(ildjitSystem, "System.Text.StringBuilder.Insert(int index, char[] value, int startIndex, int length)");
            return NULL;
        }
        if ( startIndex > (arrayLength - length) ) {
            ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "ArgumentOutOfRangeException");
            METHOD_END(ildjitSystem, "System.Text.StringBuilder.Insert(int index, char[] value, int startIndex, int length)");
            return NULL;
        }
        if (length > 0) { /*crea lo spazio dove copiare i caratteri della nuova stringa*/
            _this = (ildjitSystem->cliManager).CLR.stringBuilderManager.insert(_this, index, length);

            if (_this) { /*estrae la stringa dallo stringbuilder _this */
                sbString = (ildjitSystem->cliManager).CLR.stringBuilderManager.getString(_this);
                assert(sbString != NULL);
                sbStringLiteral = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(sbString);
                assert(sbStringLiteral != NULL);

                /*estraggo i caratteri dall'array*/
                arrayLiteral = (ildjitSystem->cliManager).CLR.arrayManager.getArrayElement(valueArray, 0);
                assert(arrayLiteral != NULL);

                /*copia dei caratteri della stringa da inserire nello spazio creato*/
                memcpy(sbStringLiteral + index, arrayLiteral + startIndex, sizeof(JITINT16) * length);
            } else {
                METHOD_END(ildjitSystem, "System.Text.StringBuilder.Insert(int index, char[] value, int startIndex, int length)");
                return NULL;
            }
        }
    } else {
        if (startIndex != 0 || length != 0) {
            METHOD_END(ildjitSystem, "System.Text.StringBuilder.Insert(int index, char[] value, int startIndex, int length)");
            ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "NullReferenceException");
            return NULL;
        }
    }

    /* Return				*/
    METHOD_END(ildjitSystem, "System.Text.StringBuilder.Insert(int index, char[] value, int startIndex, int length)");
    return _this;
}

void * System_Text_StringBuilder_Insert_iac1 (void *_this, JITINT32 index, void *valueArray) {
    JITINT32 arrayLength;
    JITINT16        *arrayLiteral;
    void            *sbString;
    JITINT16        *sbStringLiteral;

    METHOD_BEGIN(ildjitSystem, "System.Text.StringBuilder.Insert(int index, char[] value)");

    if (valueArray) { /*estrae la lunghezza dell'array*/
        arrayLength = (ildjitSystem->cliManager).CLR.arrayManager.getArrayLength(valueArray, 0);
        assert(arrayLength >= 0);

        if (arrayLength > 0) { /*crea lo spazio dove copiare i caratteri della nuova stringa*/
            _this = (ildjitSystem->cliManager).CLR.stringBuilderManager.insert(_this, index, arrayLength);

            if (_this) { /*estrae la stringa dallo stringbuilder _this */
                sbString = (ildjitSystem->cliManager).CLR.stringBuilderManager.getString(_this);
                assert(sbString != NULL);
                sbStringLiteral = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(sbString);
                assert(sbStringLiteral != NULL);

                /*estraggo i caratteri dall'array*/
                arrayLiteral = (ildjitSystem->cliManager).CLR.arrayManager.getArrayElement(valueArray, 0);
                assert(arrayLiteral != NULL);

                /*copia dei caratteri della stringa da inserire nello spazio creato*/
                memcpy(sbStringLiteral + index, arrayLiteral, sizeof(JITINT16) * arrayLength);

            } else {
                METHOD_END(ildjitSystem, "System.Text.StringBuilder.Insert(int index, char[] value)");
                return NULL;
            }
        }
    }

    /* Return				*/
    METHOD_END(ildjitSystem, "System.Text.StringBuilder.Insert(int index, char[] value)");
    return _this;
}

void * System_Text_StringBuilder_Insert_ic (void *_this, JITINT32 index, JITINT16 value) {
    void            *sbString;
    JITINT16        *sbStringLiteral;

    METHOD_BEGIN(ildjitSystem, "System.Text.StringBuilder.Insert(int index, char value)");

    /*crea lo spazio dove copiare il carattere value*/
    _this = (ildjitSystem->cliManager).CLR.stringBuilderManager.insert(_this, index, 1);

    if (_this) { /*estrae la stringa dallo stringbuilder _this */
        sbString = (ildjitSystem->cliManager).CLR.stringBuilderManager.getString(_this);
        assert(sbString != NULL);
        sbStringLiteral = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(sbString);
        assert(sbStringLiteral != NULL);

        /*scrive il carattere nello spazio creato*/
        *(sbStringLiteral + index) = value;

    } else {
        METHOD_END(ildjitSystem, "System.Text.StringBuilder.Insert(int index, char value)");
        return NULL;
    }

    /* Return				*/
    METHOD_END(ildjitSystem, "System.Text.StringBuilder.Insert(int index, char value)");
    return _this;
}

void * System_Text_StringBuilder_Replace_cc (void *_this, JITINT16 oldChar, JITINT16 newChar) {
    JITINT32 needscopy;
    void            *string;
    JITINT16        *stringLiteral;
    JITINT32 stringCapacity;
    JITINT32 stringLength;
    JITINT32 i;

    /* Assertions			*/
    assert(_this != NULL);

    METHOD_BEGIN(ildjitSystem, "System.Text.StringBuilder.Replace(char oldChar, char newChar)");

    /* Fetch the string from the stringBuilder			*/
    string = (ildjitSystem->cliManager).CLR.stringBuilderManager.getString(_this);
    assert(string != NULL);

    if (oldChar != newChar) {
        needscopy = (ildjitSystem->cliManager).CLR.stringBuilderManager.getNeedsCopy(_this);

        if (needscopy) { /* Fetch the capacity of the string stored inside the stringbuilder    */
            stringCapacity = (ildjitSystem->cliManager).CLR.stringManager.getCapacity(string);
            assert(stringCapacity >= 0);

            string = System_String_NewBuilder(string, stringCapacity);
            assert(string != NULL);

            (ildjitSystem->cliManager).CLR.stringBuilderManager.setString(_this, string);
            (ildjitSystem->cliManager).CLR.stringBuilderManager.setNeedsCopy(_this, 0);
        }

        /*estrae lunghezza e caratteri dalla stringa dello stringbuilder _this */
        stringLength = (ildjitSystem->cliManager).CLR.stringManager.getLength(string);
        assert(stringLength >= 0);
        stringLiteral = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(string);
        assert(stringLiteral != NULL);

        /*sostituisce ogni occorrenza di oldChar con newChar*/
        for (i = 0; i < stringLength; i++) {
            if (stringLiteral[i] == oldChar) {
                stringLiteral[i] = newChar;
            }
        }
    }

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Text.StringBuilder.Replace(char oldChar, char newChar)");
    return _this;
}

void * System_Text_StringBuilder_Replace_ccii (void *_this, JITINT16 oldChar, JITINT16 newChar, JITINT32 startIndex, JITINT32 count) {
    JITINT32 needscopy;
    void            *string;
    JITINT16        *stringLiteral;
    JITINT32 stringCapacity;
    JITINT32 stringLength;

    /* Assertions			*/
    assert(_this != NULL);
    METHOD_BEGIN(ildjitSystem, "System.Text.StringBuilder.Replace(char oldChar, char newChar, int startIndex, int endIndex)");

    /* Fetch the string from the stringBuilder			*/
    string = (ildjitSystem->cliManager).CLR.stringBuilderManager.getString(_this);
    assert(string != NULL);
    /*estrae lunghezza dalla stringa dello stringbuilder _this */
    stringLength = (ildjitSystem->cliManager).CLR.stringManager.getLength(string);
    assert(stringLength >= 0);


    if (startIndex < 0 || startIndex > stringLength) {
        ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "ArgumentOutOfRangeException");
        METHOD_END(ildjitSystem, "System.Text.StringBuilder.Replace(char oldChar, char newChar, int startIndex, int endIndex)");
        return NULL;
    }
    if ((stringLength - startIndex) < count) {
        ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "ArgumentOutOfRangeException");
        METHOD_END(ildjitSystem, "System.Text.StringBuilder.Replace(char oldChar, char newChar, int startIndex, int endIndex)");
        return NULL;
    }
    if (oldChar != newChar) {
        needscopy = (ildjitSystem->cliManager).CLR.stringBuilderManager.getNeedsCopy(_this);

        if (needscopy) { /* Fetch the capacity of the string stored inside the stringbuilder    */
            stringCapacity = (ildjitSystem->cliManager).CLR.stringManager.getCapacity(string);
            assert(stringCapacity >= 0);

            string = System_String_NewBuilder(string, stringCapacity);
            assert(string != NULL);

            (ildjitSystem->cliManager).CLR.stringBuilderManager.setString(_this, string);
            (ildjitSystem->cliManager).CLR.stringBuilderManager.setNeedsCopy(_this, 0);
        }

        /*estrae i caratteri dalla stringa dello stringbuilder*/
        stringLiteral = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(string);
        assert(stringLiteral != NULL);

        stringLiteral = stringLiteral + startIndex;

        while (count > 0) {
            if (*stringLiteral == oldChar) {
                *stringLiteral = newChar;
            }
            stringLiteral++;
            count--;
        }
    }

    /* Return				*/
    METHOD_END(ildjitSystem, "System.Text.StringBuilder.Replace(char oldChar, char newChar, int startIndex, int endIndex)");
    return _this;
}

JITINT32 System_Text_DefaultEncoding_InternalCodePage (void) {
    JITINT32 ret;

    METHOD_BEGIN(ildjitSystem, "System.Text.DefaultEncoding.InternalCodePage");

    ret = getCurrentCodePage();

    /* Return				*/
    METHOD_END(ildjitSystem, "System.Text.DefaultEncoding.InternalCodePage");
    return ret;
}

JITINT32 System_Text_DefaultEncoding_InternalGetByteCount_acii (void* chars, JITINT32 index, JITINT32 count) {
    JITINT16        *literal;

    METHOD_BEGIN(ildjitSystem, "System.Text.DefaultEncoding.InternalGetByteCount(char[] chars, int index, int count)");

    /*estrae caratteri del vettore*/
    literal = (ildjitSystem->cliManager).CLR.arrayManager.getArrayElement(chars, 0);
    assert(literal != NULL);

    /* Return				*/
    METHOD_END(ildjitSystem, "System.Text.DefaultEncoding.InternalGetByteCount(char[] chars, int index, int count)");
    return count;
}

JITINT32 System_Text_DefaultEncoding_InternalGetByteCount_Stringii (void* s, JITINT32 index, JITINT32 count) {
    JITINT16        *strLiteral;
    JITINT32 strLen;
    JITINT32 ret = 0;

    METHOD_BEGIN(ildjitSystem, "System.Text.DefaultEncoding.InternalGetByteCount(String s, int index, int count)");

    /*estrae lunghezza della stringa*/
    strLen = (ildjitSystem->cliManager).CLR.stringManager.getLength(s);
    assert(strLen >= 0);

    if (strLen > 0) { /*estrae caratteri della stringa*/
        strLiteral = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(s);
        assert(strLiteral != NULL);

        ret = count;
    }

    /* Return				*/
    METHOD_END(ildjitSystem, "System.Text.DefaultEncoding.InternalGetByteCount(String s, int index, int count)");
    return ret;
}

JITINT32 System_Text_DefaultEncoding_InternalGetBytes_aciiaBi1 (void* chars, JITINT32 charIndex, JITINT32 charCount, void* bytes, JITINT32 byteIndex) {
    JITINT16        *charsLiteral;
    JITINT16        *bytesLiteral;
    JITINT32 charsLength;
    JITINT32 bytesLength;
    JITINT32 ret;

    /* Assertions			*/
    assert(chars != NULL);
    assert(bytes != NULL);
    METHOD_BEGIN(ildjitSystem, "System.Text.DefaultEncoding.InternalGetBytes(char[] chars, int charIndex, int charCount, byte[] bytes, int byteIndex)");

    /*estrae lunghezza e caratteri dal vettore di caratteri*/
    charsLength = (ildjitSystem->cliManager).CLR.arrayManager.getArrayLength(chars, 0);
    assert(charsLength >= 0);
    charsLiteral = (ildjitSystem->cliManager).CLR.arrayManager.getArrayElement(chars, 0);
    assert(charsLiteral != NULL);

    /*estrae lunghezza e byte dal vettore di byte*/
    bytesLength = (ildjitSystem->cliManager).CLR.arrayManager.getArrayLength(bytes, 0);
    assert(bytesLength >= 0);
    bytesLiteral = (ildjitSystem->cliManager).CLR.arrayManager.getArrayElement(bytes, 0);
    assert(bytesLiteral != NULL);

    assert(charsLength >= (charIndex + charCount));
    assert(bytesLength >= (byteIndex + charCount));
    ret = getBytes( ((JITINT16*) charsLiteral) + charIndex, charCount, ((JITINT8 *) bytesLiteral) + byteIndex, bytesLength - byteIndex);

    /* Return				*/
    METHOD_END(ildjitSystem, "System.Text.DefaultEncoding.InternalGetBytes(char[] chars, int charIndex, int charCount, byte[] bytes, int byteIndex)");
    return ret;
}

JITINT32 System_Text_DefaultEncoding_InternalGetBytes_StringiiaBi (void* s, JITINT32 charIndex, JITINT32 charCount, void* bytes, JITINT32 byteIndex) {
    JITINT16        *stringLiteral;
    JITINT16        *bytesLiteral;
    JITINT32 stringLength;
    JITINT32 bytesLength;
    JITINT32 ret;

    METHOD_BEGIN(ildjitSystem, "System.Text.DefaultEncoding.InternalGetBytes(String s, int charIndex, int charCount, byte[] bytes, int byteIndex)");

    /*estrae lunghezza e caratteri dalla stringa*/
    stringLength = (ildjitSystem->cliManager).CLR.stringManager.getLength(s);
    assert(stringLength >= 0);
    stringLiteral = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(s);
    assert(stringLiteral != NULL);

    /*estrae lunghezza e byte dal vettore di byte*/
    bytesLength = (ildjitSystem->cliManager).CLR.arrayManager.getArrayLength(bytes, 0);
    assert(bytesLength >= 0);
    bytesLiteral = (ildjitSystem->cliManager).CLR.arrayManager.getArrayElement(bytes, 0);
    assert(bytesLiteral != NULL);

    ret = getBytes( ((JITINT16*) stringLiteral) + charIndex, charCount, ((JITINT8 *) bytesLiteral) + byteIndex, bytesLength - byteIndex);

    /* Return				*/
    METHOD_END(ildjitSystem, "System.Text.DefaultEncoding.InternalGetBytes(String s, int charIndex, int charCount, byte[] bytes, int byteIndex)");
    return ret;
}

JITINT32 System_Text_DefaultEncoding_InternalGetCharCount (void* bytes, JITINT32 index, JITINT32 count) {
    JITINT16        *bytesLiteral;
    JITINT32 bytesLength;
    JITINT32 ret = 0;

    METHOD_BEGIN(ildjitSystem, "System.Text.DefaultEncoding.InternalGetCharCount");

    /*estrae lunghezza e byte dal vettore di byte*/
    bytesLength = (ildjitSystem->cliManager).CLR.arrayManager.getArrayLength(bytes, 0);
    assert(bytesLength >= 0);

    if (bytesLength > 0) {
        bytesLiteral = (ildjitSystem->cliManager).CLR.arrayManager.getArrayElement(bytes, 0);
        assert(bytesLiteral != NULL);

        ret = count;
    }

    /* Return				*/
    METHOD_END(ildjitSystem, "System.Text.DefaultEncoding.InternalGetCharCount");
    return ret;
}

JITINT32 System_Text_DefaultEncoding_InternalGetChars (void* bytes, JITINT32 byteIndex, JITINT32 byteCount,  void* chars, JITINT32 charIndex) {
    JITINT16        *charsLiteral;
    JITINT16        *bytesLiteral;
    JITINT32 charsLength;
    JITINT32 bytesLength;
    JITINT32 ret;

    METHOD_BEGIN(ildjitSystem, "System.Text.DefaultEncoding.InternalGetChars");

    /*estrae lunghezza e byte dal vettore di byte*/
    bytesLength = (ildjitSystem->cliManager).CLR.arrayManager.getArrayLength(bytes, 0);
    assert(bytesLength >= 0);
    bytesLiteral = (ildjitSystem->cliManager).CLR.arrayManager.getArrayElement(bytes, 0);
    assert(bytesLiteral != NULL);

    /*estrae lunghezza e caratteri dal vettore di caratteri*/
    charsLength = (ildjitSystem->cliManager).CLR.arrayManager.getArrayLength(chars, 0);
    assert(charsLength >= 0);
    charsLiteral = (ildjitSystem->cliManager).CLR.arrayManager.getArrayElement(chars, 0);
    assert(charsLiteral != NULL);

    ret = getChars( ((JITINT8 *) bytesLiteral) + byteIndex, byteCount, ((JITINT16 *) charsLiteral) + charIndex, charsLength - charIndex );

    /* Return				*/
    METHOD_END(ildjitSystem, "System.Text.DefaultEncoding.InternalGetChars");
    return ret;
}

JITINT32 System_Text_DefaultEncoding_InternalGetMaxByteCount (JITINT32 charCount) {
    JITINT32 ret;

    METHOD_BEGIN(ildjitSystem, "System.Text.DefaultEncoding.InternalGetMaxByteCount");

    ret = (JITUINT32) getMaxByteCount((JITUINT32) charCount);

    /* Return				*/
    METHOD_END(ildjitSystem, "System.Text.DefaultEncoding.InternalGetMaxByteCount");
    return ret;
}

JITINT32 System_Text_DefaultEncoding_InternalGetMaxCharCount (JITINT32 byteCount) {
    JITINT32 ret;

    METHOD_BEGIN(ildjitSystem, "System.Text.DefaultEncoding.InternalGetMaxCharCount");

    ret = getMaxCharCount(byteCount);

    /* Return				*/
    METHOD_END(ildjitSystem, "System.Text.DefaultEncoding.InternalGetMaxCharCount");
    return ret;
}

void * System_Text_DefaultEncoding_InternalGetString (void* bytes, JITINT32 index, JITINT32 count) {
    JITINT32 charCount;
    JITINT16        *bytesLiteral;
    JITINT32 bytesLength;
    JITINT16        *stringLiteral;
    JITINT32 stringLength;
    void            *string;

    METHOD_BEGIN(ildjitSystem, "System.Text.DefaultEncoding.InternalGetString");

    /*estrae lunghezza e byte dal vettore di byte*/
    bytesLength = (ildjitSystem->cliManager).CLR.arrayManager.getArrayLength(bytes, 0);
    assert(bytesLength >= 0);
    bytesLiteral = (ildjitSystem->cliManager).CLR.arrayManager.getArrayElement(bytes, 0);
    assert(bytesLiteral != NULL);

    charCount = count;

    if (charCount < 0) {
        METHOD_END(ildjitSystem, "System.Text.DefaultEncoding.InternalGetString");
        return NULL;
    }

    string = System_String_NewString(charCount);
    assert(string != NULL);

    /*estrae lunghezza e caratteri dalla stringa*/
    stringLength = (ildjitSystem->cliManager).CLR.stringManager.getLength(string);
    assert(stringLength >= 0);
    stringLiteral = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(string);
    assert(stringLiteral != NULL);

    getChars( ((JITINT8*) bytesLiteral) + index, count, stringLiteral, stringLength );

    /* Return				*/
    METHOD_END(ildjitSystem, "System.Text.DefaultEncoding.InternalGetString");
    return string;
}

/*
 * Get given code page, or the default code page if codePage is 1,
 * as a string.
 */
void* System_Text_Encoding_InternalCodePage (JITUINT32* codePage) {
    const JITINT8* codePageAsString;
    void* codePageAsCLIString;

    METHOD_BEGIN(ildjitSystem, "System.Text.Encoding.InternalCodePage");
    if (*codePage == 0 || *codePage == 1) {
        *codePage = getCurrentCodePage();
        codePageAsString = getCodePageAsString(*codePage);
    } else {
        codePageAsString = getCodePageAsString(*codePage);
    }

    codePageAsCLIString = CLIMANAGER_StringManager_newInstanceFromUTF8(
                              (JITINT8*) codePageAsString,
                              STRLEN(codePageAsString));
    METHOD_END(ildjitSystem, "System.Text.Encoding.InternalCodePage");

    return codePageAsCLIString;
}

JITNINT Platform_RegexpMethods_CompileInternal (void* pattern, JITINT32 flags) {
    JITINT8         *pat;
    JITINT32 error;
    regex_t         *result;

    METHOD_BEGIN(ildjitSystem, "Platform.RegexpMethods.CompileInternal");

    /*estrae dalla stringa una copia dei caratteri in formato a 8 bit*/
    pat = (ildjitSystem->cliManager).CLR.stringManager.getUTF8copy(pattern);
    if (!pat) {
        ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "OutOfMemoryException");
        METHOD_END(ildjitSystem, "Platform.RegexpMethods.CompileInternal");
        return 0;
    }

    /*alloca spazio per la variabile result*/
    result = (regex_t*) allocMemory(sizeof(regex_t));
    if (!result) {
        ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "OutOfMemoryException");
        METHOD_END(ildjitSystem, "Platform.RegexpMethods.CompileInternal");
        return 0;
    }

    /*chiama la funzione per compilare l'espressione*/
    error = PLATFORM_regcomp(result, (char *) pat, flags);
    if (error) {
        freeMemory(result);
        freeMemory(pat);
        METHOD_END(ildjitSystem, "Platform.RegexpMethods.CompileInternal");
        return 0;
    }

    /* Free the memory			*/
    freeMemory(pat);

    /* Return				*/
    METHOD_END(ildjitSystem, "Platform.RegexpMethods.CompileInternal");
    return (JITNINT) result;
}

JITNINT Platform_RegexpMethods_CompileWithSyntaxInternal (void* pattern, JITINT32 syntax) {
    JITINT8         *pat;
    JITINT32 error;
    struct re_pattern_buffer *result;

    /*estrae dalla stringa una copia dei caratteri in formato a 8 bit*/
    pat = (ildjitSystem->cliManager).CLR.stringManager.getUTF8copy(pattern);
    if (!pat) {
        ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "OutOfMemoryException");
        METHOD_END(ildjitSystem, "Platform.RegexpMethods.CompileWithSyntaxInternal");
        return 0;
    }

    /*alloca spazio per la variabile result*/
    result = (struct re_pattern_buffer *) allocMemory(sizeof(struct re_pattern_buffer));
    if (!result) {
        ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "OutOfMemoryException");
        METHOD_END(ildjitSystem, "Platform.RegexpMethods.CompileWithSyntaxInternal");
        return 0;
    }
    if (syntax == RE_SYNTAX_POSIX_BASIC) {
        error = PLATFORM_regcomp(result, (char *) pat, 0);

    } else if (syntax == RE_SYNTAX_POSIX_EXTENDED) {
        error = PLATFORM_regcomp(result, (char *) pat, REG_EXTENDED);

    } else {
        PLATFORM_re_set_syntax((reg_syntax_t) syntax);
        error = (PLATFORM_re_compile_pattern((char *) pat, strlen((char *) pat), result) != NULL);
    }
    if (error != 0) {
        freeMemory(result);
        METHOD_END(ildjitSystem, "Platform.RegexpMethods.CompileWithSyntaxInternal");
        return 0;
    }

    /* Free the memory			*/
    freeMemory(pat);

    /* Return				*/
    METHOD_END(ildjitSystem, "Platform.RegexpMethods.CompileWithSyntaxInternal");
    return (JITNINT) result;
}

JITINT32 Platform_RegexpMethods_ExecInternal (JITNINT compiled, void* input, JITINT32 flags) {
    JITINT32 ret;
    JITINT8         *pat;

    METHOD_BEGIN(ildjitSystem, "Platform.RegexpMethods.ExecInternal");

    pat = (ildjitSystem->cliManager).CLR.stringManager.getUTF8copy(input);
    if (!pat) {
        ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "OutOfMemoryException");
        METHOD_END(ildjitSystem, "Platform.RegexpMethods.ExecInternal");
        return -1;
    }

    ret = PLATFORM_regexec((regex_t*) compiled, (char *) pat, 0, 0, flags);

    /* Return				*/
    freeMemory(pat);
    METHOD_END(ildjitSystem, "Platform.RegexpMethods.ExecInternal");
    return ret;
}

void* Platform_RegexpMethods_MatchInternal (JITNINT compiled, void* input, JITINT32 maxMatches, JITINT32 flags, void* elemType) {
    void            *array;

    JITINT8         *pat;
    regmatch_t      *matches;
    void            *elemClass;
    JITINT32 numMatches;
    JITINT32 index;
    RegexMatch      *matchList;

    METHOD_BEGIN(ildjitSystem, "Platform.RegexpMethods.MatchInternal");

    pat = (ildjitSystem->cliManager).CLR.stringManager.getUTF8copy(input);
    if (!pat) {
        ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "OutOfMemoryException");
        METHOD_END(ildjitSystem, "Platform.RegexpMethods.MatchInternal");
        return 0;
    }
    if (maxMatches > 0) {
        matches = (regmatch_t *) allocMemory(maxMatches * sizeof(regmatch_t));
        if (!matches) {
            METHOD_END(ildjitSystem, "Platform.RegexpMethods.MatchInternal");
            return 0;
        }
    } else {
        matches = 0;
        maxMatches = 0;
    }


    if (PLATFORM_regexec((regex_t*) compiled, (char *) pat, (size_t) maxMatches, matches, flags) != 0) {
        if (matches != 0) {
            freeMemory(matches);
        }
        METHOD_END(ildjitSystem, "Platform.RegexpMethods.MatchInternal");
        return 0;
    }

    elemClass = System_Reflection_ClrType_GetClrBaseType(elemType);
    if (!elemClass) {
        if (matches != 0) {
            freeMemory(matches);
        }
        METHOD_END(ildjitSystem, "Platform.RegexpMethods.MatchInternal");
        return 0;
    }

    numMatches = ((regex_t*) compiled)->re_nsub + 1;

    numMatches = (maxMatches < numMatches) ? maxMatches : numMatches;

    array = System_Array_CreateArray( (JITNINT) elemClass, 1, numMatches, 0, 0);

    if (!array) {
        if (matches != 0) {
            freeMemory(matches);
        }
        METHOD_END(ildjitSystem, "Platform.RegexpMethods.MatchInternal");
        return 0;
    }

    matchList = (ildjitSystem->cliManager).CLR.arrayManager.getArrayElement(array, 0);

    index = 0;
    while (index < numMatches) {
        matchList[index].start = (JITINT32) (matches[index].rm_so);
        matchList[index].end = (JITINT32) (matches[index].rm_eo);
        ++index;
    }
    if (matches != 0) {
        freeMemory(matches);
    }


    /* Return				*/
    METHOD_END(ildjitSystem, "Platform.RegexpMethods.MatchInternal");
    return array;
}

void Platform_RegexpMethods_FreeInternal (JITNINT compiled) {

    METHOD_BEGIN(ildjitSystem, "Platform.RegexpMethods.FreeInternal");

    if (compiled) {
        PLATFORM_regfree((regex_t*) compiled);
        freeMemory((void*) compiled);
    }

    /* Return				*/
    METHOD_END(ildjitSystem, "Platform.RegexpMethods.FreeInternal");
    return;
}

JITINT32 System_String_GetLOSLimit (void) {
    JITINT32 maxInt;

    METHOD_BEGIN(ildjitSystem, "System.String.GetLOSLimit");

    maxInt = JITMAXINT32;

    METHOD_END(ildjitSystem, "System.String.GetLOSLimit");

    return maxInt;
}
