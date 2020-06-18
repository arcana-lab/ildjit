/*
 * Copyright (C) 2012  Campanoni Simone
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
#ifndef RUNTIME_H
#define RUNTIME_H

#include <zlib.h>
#include <jitsystem.h>

void RUNTIME_init (BZFILE *compressedOutputFile);
void dump_start_loop (JITINT8 *loopName);
void dump_end_loop (void);
void dump_start_method (char *methodName);
void dump_end_method (void);
void dump_start_loop_iteration (void);
void dump_memory_instruction (JITUINT32 instructionPosition, void *input1, JITUINT32 input1Size, void *input2, JITUINT32 input2Size, void *output, JITUINT32 outputSize);
void dump_os_instruction (void);
void dump_call_instruction (JITUINT32 instructionID);

#endif
