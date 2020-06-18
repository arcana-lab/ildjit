/*
 * Copyright (C) 2006
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
#include <ildjit_locale.h>
#include <iljit-utils.h>
#include <platform_API.h>

/* Code set */
typedef struct {
    const JITINT8 *name;
    unsigned page;
} code_set_t;

/* Known code sets */
static const code_set_t codesets[] = {
    { (JITINT8 *) "iso8859-1",	CP_8859_1																						 },
    { (JITINT8 *) "iso-8859-1",	CP_8859_1																						 },
    { (JITINT8 *) "iso8859-2",	CP_8859_2																						 },
    { (JITINT8 *) "iso-8859-2",	CP_8859_2																						 },
    { (JITINT8 *) "iso8859-3",	CP_8859_3																						 },
    { (JITINT8 *) "iso-8859-3",	CP_8859_3																						 },
    { (JITINT8 *) "iso8859-4",	CP_8859_4																						 },
    { (JITINT8 *) "iso-8859-4",	CP_8859_4																						 },
    { (JITINT8 *) "iso8859-5",	CP_8859_5																						 },
    { (JITINT8 *) "iso-8859-5",	CP_8859_5																						 },
    { (JITINT8 *) "iso8859-6",	CP_8859_6																						 },
    { (JITINT8 *) "iso-8859-6",	CP_8859_6																						 },
    { (JITINT8 *) "iso8859-7",	CP_8859_7																						 },
    { (JITINT8 *) "iso-8859-7",	CP_8859_7																						 },
    { (JITINT8 *) "iso8859-8",	CP_8859_8																						 },
    { (JITINT8 *) "iso-8859-8",	CP_8859_8																						 },
    { (JITINT8 *) "iso8859-9",	CP_8859_9																						 },
    { (JITINT8 *) "iso-8859-9",	CP_8859_9																						 },
    { (JITINT8 *) "iso8859-15",	CP_8859_15																						 },
    { (JITINT8 *) "iso-8859-15",	CP_8859_15																						 },
    { (JITINT8 *) "windows-1250",	1250																							 },
    { (JITINT8 *) "windows-1251",	1251																							 },
    { (JITINT8 *) "windows-1252",	1252																							 },
    { (JITINT8 *) "windows-1253",	1253																							 },
    { (JITINT8 *) "windows-1254",	1254																							 },
    { (JITINT8 *) "windows-1255",	1255																							 },
    { (JITINT8 *) "windows-1256",	1256																							 },
    { (JITINT8 *) "windows-1257",	1257																							 },
    { (JITINT8 *) "windows-1258",	1258																							 },
    { (JITINT8 *) "shift-jis",	CP_SHIFT_JIS																						 },
    { (JITINT8 *) "sjis",		CP_SHIFT_JIS																						 },
    { (JITINT8 *) "koi8-r",		CP_KOI8_R																						 },
    { (JITINT8 *) "koi8-u",		CP_KOI8_U																						 },
    { (JITINT8 *) "utf-7",		CP_UTF7																							 },
    { (JITINT8 *) "utf-8",		CP_UTF8																							 },
    { (JITINT8 *) "ucs-2",		CP_UCS2_DETECT																						 },
    { (JITINT8 *) "ucs-2le",	CP_UCS2_LE																						 },
    { (JITINT8 *) "ucs-2be",	CP_UCS2_BE																						 },
    { (JITINT8 *) "utf-16",		CP_UCS2_DETECT																						 },
    { (JITINT8 *) "utf-16le",	CP_UCS2_LE																						 },
    { (JITINT8 *) "utf-16be",	CP_UCS2_BE																						 },
    { (JITINT8 *) "us-ascii",	CP_8859_1																						 },
    { (JITINT8 *) "ansi-3.4-1968",	CP_8859_1																						 },
    { (JITINT8 *) "ansi-3.4-1986",	CP_8859_1																						 },
    { (JITINT8 *) "ansi-3.4",	CP_8859_1																						 },
    { (JITINT8 *) "ansi-x3.4-1968", CP_8859_1																						 }
};

static inline JITINT32 CompareCodesets (const JITINT8 *name1, const JITINT8 *name2);
static inline JITINT32 getCodeSetsSize (void);
static JITINT32 getCodePageFromName (const JITINT8* name);

/* Get the size of the set containing all (name, codepage) mappings */
static inline JITINT32 getCodeSetsSize (void) {
    return sizeof(codesets) / sizeof(code_set_t);
}

/*
 * Look in the code page table for the code page with the given name. Return 0
 * if not found or name is NULL.
 */
static JITINT32 getCodePageFromName (const JITINT8* name) {
    JITINT32 i;

    if (name == NULL) {
        return 0;
    }

    for (i = 0; i < getCodeSetsSize(); i++) {
        if (CompareCodesets(name, codesets[i].name)) {

            /* The found code set is nothing special */
            if (codesets[i].page != CP_UCS2_DETECT) {
                return codesets[i].page;

                /* Auto-detect the default UCS-2 encoding for this platform */
            } else {
                union {
                    short x;
                    unsigned char y[2];
                } convert;
                convert.x = 0x0102;
                if (convert.y[0] == 0x01) {
                    return CP_UCS2_BE;
                } else {
                    return CP_UCS2_LE;
                }
            }
        }
    }

    return 0;
}

/* Get the current code page */
JITINT32 getCurrentCodePage (void) {
    return getCodePageFromName(PLATFORM_getCurrentCodePageName());
}

/* Get the string representation of codePage */
const JITINT8* getCodePageAsString (JITINT32 codePage) {
    JITINT32 i;

    for (i = 0; i < getCodeSetsSize(); i++) {
        if (((JITINT32) codesets[i].page) == codePage) {
            return codesets[i].name;
        }
    }

    return NULL;
}

JITINT32 getBytes (JITINT16* chars, JITINT32 charCount, JITINT8* bytes, JITINT32 byteCount) {
    JITINT32 len;
    JITINT16 ch;

    /* Check for enough space in the output buffer */
    if (charCount > byteCount) {
        return -1;
    }

    /* Convert the characters */
    len = charCount;
    while (len > 0) {
        ch = *chars++;
        if (ch < (JITINT16) 0x0100) {
            *bytes++ = (JITINT8) ch;
        } else {
            *bytes++ = (JITINT8) '?';
        }
        --len;
    }
    return charCount;
}

JITINT32 getChars (JITINT8* bytes, JITINT32 byteCount, JITINT16 *chars, JITINT32 charCount) {
    JITINT32 len;

    /* Check for enough space in the output buffer */
    if (byteCount > charCount) {
        return -1;
    }

    /* Convert the bytes */
    len = byteCount;
    while (len > 0) {
        *chars++ = (JITINT16) (*bytes++);
        --len;
    }
    return byteCount;
}



JITUINT32 getMaxByteCount (JITUINT32 charCount) {
    return charCount * 16;
}

JITINT32 getMaxCharCount (JITINT32 byteCount) {
    return byteCount;
}

/*
 * Compare two codeset names.
 */
static inline JITINT32 CompareCodesets (const JITINT8 *name1, const JITINT8 *name2) {
    char n1;

    while (*name1 != '\0' && *name2 != '\0') {
        /* Normalize the incoming character */
        n1 = *name1;
        if (n1 >= 'A' && n1 <= 'Z') {
            n1 = n1 - 'A' + 'a';
        } else if (n1 == '_') {
            n1 = '-';
        }

        /* Do we have a match? */
        if (n1 != *name2) {
            return 0;
        }
        ++name1;
        ++name2;
    }
    if (*name2 == '\0') {
        /* We ignore extraneous data after a colon, because it is
           usually year information that we aren't interested in */
        return *name1 == '\0' || *name1 == ':';
    }
    return 0;
}
