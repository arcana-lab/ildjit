/*
 * Copyright (C) 2006 - 2012 Simone Campanoni  Luca Rocchini
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 */
#ifndef LIB_PINVOKE
#define LIB_PINVOKE

#include <xanlib.h>
#include <jitsystem.h>
#include <cli_manager.h>

typedef struct _PinvokeManager {
    XanHashTable 	*libraries;
    JITINT8		*libPath;

    void 			(*buildMethod)		(struct _PinvokeManager* self, Method method);
    ir_instruction_t * 	(*addPinvokeInstruction)(struct _PinvokeManager* self, ir_method_t *caller, Method callee, ir_instruction_t *afterInst);
    void 			(*destroy)		(struct _PinvokeManager* self);
} PinvokeManager;

void init_pinvokeManager (PinvokeManager *manager, JITINT8 *libPath);

#endif /* LIB_PINVOKE */
