/*
 * Copyright (C) 2006 Campanoni Simone, Di Biagio Andrea
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
#ifndef GENERAL_TOOLS_H
#define GENERAL_TOOLS_H

// My headers
#include <ildjit_system.h>
// End

/**
 * Delegate builder
 *
 * This method is used by the runtime to build any kind of delegates. The most
 * important operation performed is the delegate signature checking. If the
 * signature of the method argument don't match the one of the delegate an
 * exception/error will be raised/occurs.
 */
/* TODO: implement signature dynamic checking, very boring */
void BuildDelegate (void* self, void* target, void* function_entry_point);

t_system * getSystem (t_system *system);

#endif
