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
#include <jitsystem.h>

// My headers
#include <internal_calls_environment.h>
#include <internal_calls_utilities.h>
#include <internal_calls_misc.h>
// End

extern t_system *ildjitSystem;

/* Get info about the system time zone */
JITBOOLEAN System_CurrentSystemTimeZone_GetTimeZoneData (JITINT32 year, void** timeInfo, void** timeNames) {
    t_arrayManager* arrayManager;
    t_stringManager* stringManager;
    TypeDescriptor *int64Type;
    TypeDescriptor *stringType;
    void* timezoneName;
    JITINT32 i;
    JITBOOLEAN success;

    arrayManager = &((ildjitSystem->cliManager).CLR.arrayManager);
    stringManager = &((ildjitSystem->cliManager).CLR.stringManager);

    METHOD_BEGIN(ildjitSystem, "System.CurrentSystemTimeZone.GetTimeZoneData");

    int64Type = (ildjitSystem->cliManager).metadataManager->getTypeDescriptorFromElementType((ildjitSystem->cliManager).metadataManager, ELEMENT_TYPE_I8);
    *timeInfo = arrayManager->newInstanceFromType(int64Type, 4);

    stringType = stringManager->fillILStringType();
    *timeNames = arrayManager->newInstanceFromType(stringType, 2);
    for (i = 0; i < 2; i++) {
        timezoneName = CLIMANAGER_StringManager_newInstanceFromUTF8((JITINT8*) "UTC",
                       strlen("UTC"));
        arrayManager->setValueArrayElement(*timeNames, i, timezoneName);
    }

    success = JITTRUE;

    METHOD_END(ildjitSystem, "System.CurrentSystemTimeZone.GetTimeZoneData");
    return success;
}
