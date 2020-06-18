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

/**
   *@file loader.h
 */

#ifndef LOADER_H
#define LOADER_H

// My header
#include <system_manager.h>
#include <jitsystem.h>
// End

/**
 * @brief Loader
 * @param system System
 *
 * Start the loader, which begin to the input CIL program.
 * It fetch the program ID and search from the decoder plugins which can decode it, it start that decoder and wait the return value, after, the loader check if the CIL program is linked with other assemblies, if it is so, the loader, with all the decoder plugins, decode the metadata of them.
 * When the decode step is finish, it decode the entry method of the CIL program and for each CIL instruction call the CIL->IR module.
 */
JITBOOLEAN loader (t_system *system);

/**
 * @brief Shutdown the loader
 * @param system System
 *
 * Shutdown the loader
 */
JITBOOLEAN loader_shutdown (t_system *system);

/**
 * @brief Alloc a new empty binary
 *
 */
t_binary_information * newEmptyBinary ();

t_binary_information * loadAssembly (t_system *system, JITINT8 *assemblyName);

#endif
