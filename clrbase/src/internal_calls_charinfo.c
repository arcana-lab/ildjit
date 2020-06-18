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
#include <stdio.h>
#include <unicat.h>

// My headers
#include <internal_calls_charinfo.h>
#include <internal_calls_utilities.h>
// End

extern t_system *ildjitSystem;

JITINT32 Platform_SysCharInfo_GetUnicodeCategory (JITINT16 ch) {
    JITINT32 category;

    METHOD_BEGIN(ildjitSystem, "Platform.SysCharInfo.GetUnicodeCategody");

    /* The full Unicode category table is available */
    category = 0;
    if (ch < (unsigned) 0x10000) {
        category = (ILUnicodeCategory) ((charCategories[ch / 6] >> (5 * (ch % 6))) & 0x1F);
    } else {
        category = ILUnicode_OtherNotAssigned;
    }

    /* Return			*/
    METHOD_END(ildjitSystem, "Platform.SysCharInfo.GetUnicodeCategody");
    return category;
}

void * Platform_SysCharInfo_GetNewLine (void) {
    void            *object;

    METHOD_BEGIN(ildjitSystem, "Platform.SysCharInfo.GetNewLine");

    /* Make the String object	*/
    object = CLIMANAGER_StringManager_newInstanceFromUTF8((JITINT8 *) "\n", 1);
    assert(object != NULL);

    /* Return			*/
    METHOD_END(ildjitSystem, "Platform.SysCharInfo.GetNewLine");
    return object;
}
