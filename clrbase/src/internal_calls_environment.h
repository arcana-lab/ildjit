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

#ifndef INTERNAL_CALLS_ENVIRONMENT_H
#define INTERNAL_CALLS_ENVIRONMENT_H

#include <jitsystem.h>

/**
 * Get the uptime in milliseconds
 *
 * This is a Mono internal call.
 *
 * @return the uptime in milliseconds
 */
JITINT32 System_Environment_get_TickCount (void);

/**
 * Get the newline character
 *
 * This is a Mono internal call.
 *
 * @return the newline character as a string
 */
void* System_Environment_get_NewLine (void);

/**
 * Get the running platform identifier
 *
 * This is a Mono internal call.
 *
 * @return the platform identifier
 */
JITINT32 System_Environment_get_Platform (void);

/**
 * Get the command line arguments
 *
 * This is a Mono internal call.
 *
 * @return an array filled with command line arguments
 */
void* System_Environment_GetCommandLineArgs (void);

/**
 * Get the version of the running OS
 *
 * This is a Mono internal call.
 *
 * @return the version of the running OS
 */
void* System_Environment_GetOSVersionString (void);

/**
 * Get the environment variable
 *
 * This is a Mono internal call.
 *
 *
 * @return the environment variable value
 */
void* System_Environment_internalGetEnvironmentVariable (void* variable);

#endif /* INTERNAL_CALLS_ENVIRONMENT_H */
