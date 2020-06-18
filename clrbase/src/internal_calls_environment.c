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
#include <platform_API.h>

// My headers
#include <internal_calls_environment.h>
#include <internal_calls_utilities.h>
#include <internal_calls_charinfo.h>
#include <internal_calls_misc.h>
#include <internal_calls_IO.h>
// End

extern t_system *ildjitSystem;

/* Get the uptime in milliseconds */
JITINT32 System_Environment_get_TickCount (void) {
    JITINT32 uptime;

    METHOD_BEGIN(ildjitSystem, "System.Environment.get_TickCount");
    uptime = Platform_TimeMethods_GetUpTime();
    METHOD_END(ildjitSystem, "System.Environment.get_TickCount");

    return uptime;
}

/* Get the newline character */
void* System_Environment_get_NewLine (void) {
    void* newLine;

    METHOD_BEGIN(ildjitSystem, "System.Environment.get_NewLine");
    newLine = Platform_SysCharInfo_GetNewLine();
    METHOD_END(ildjitSystem, "System.Environment.get_NewLine");

    return newLine;
}

/* Get the running platform identifier */
JITINT32 System_Environment_get_Platform (void) {
    JITINT32 platform;

    METHOD_BEGIN(ildjitSystem, "System.Environment.get_Platform");
    platform = Platform_InfoMethods_GetPlatformID();
    METHOD_END(ildjitSystem, "System.Environment.get_Platform");

    return platform;
}

/* Get the command line arguments */
void* System_Environment_GetCommandLineArgs (void) {
    void* args;

    METHOD_BEGIN(ildjitSystem, "System.Environment.GetCommandLineArgs");

    args = Platform_TaskMethods_GetCommandLineArgs();

    METHOD_END(ildjitSystem, "System.Environment.GetCommandLineArgs");

    return args;
}

/* Get the version of the running OS */
void* System_Environment_GetOSVersionString (void) {
    struct utsname name;
    char* iterator;
    int dotIndex;
    char major[11], minor[11], build[11], version[64];
    void* versionAsString;

    METHOD_BEGIN(ildjitSystem, "System.Environment.GetOSVersionString");

    PLATFORM_getPlatformInfo(&name);
    iterator = name.release;

    dotIndex = strchr(iterator, '.') - iterator;
    strncpy(major, iterator, dotIndex);
    major[dotIndex] = '\0';
    iterator += dotIndex + 1;

    dotIndex = strchr(iterator, '.') - iterator;
    strncpy(minor, iterator, dotIndex);
    minor[dotIndex] = '\0';
    iterator += dotIndex + 1;

    dotIndex = strchr(iterator, '-') - iterator;
    strncpy(build, iterator, dotIndex);
    build[dotIndex] = '\0';

    sprintf(version, "%s.%s.%s", major, minor, build);
    versionAsString = CLIMANAGER_StringManager_newInstanceFromUTF8((JITINT8*) version,
                      strlen(version));

    METHOD_END(ildjitSystem, "System.Environment.GetOSVersionString");

    return versionAsString;
}

void* System_Environment_internalGetEnvironmentVariable (void *variable) {
    void* paramEnvironmentVariable;

    METHOD_BEGIN(ildjitSystem, "System.Environment.internalGetEnvironmentVariable");
    paramEnvironmentVariable = Platform_TaskMethods_GetEnvironmentVariable(variable);
    METHOD_END(ildjitSystem, "System.Environment.internalGetEnvironmentVariable");

    return paramEnvironmentVariable;
}
