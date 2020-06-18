/*
 * Copyright (C) 2008  Campanoni Simone
 *
 * This file is part of the MORPHEUS research project.
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
#ifndef PIN_DIDT_H
#define PIN_DIDT_H
#ifdef MORPHEUS

#include <jitsystem.h>
#include <stdio.h>

// My headers
#include <ilmethod.h>
// End

#define DIDT_BUF                      100

typedef struct {
    FILE                    *DIDT_report;
    JITUINT32 issued_pc[DIDT_BUF];
    JITUINT32 current_issued_pc_free_position;
    JITUINT32 committed_pc[DIDT_BUF];
    JITUINT32 current_committed_pc_free_position;
    JITUINT32 issued_ir[DIDT_BUF];
    JITUINT32 current_issued_ir_free_position;
    JITINT32 internalDIDT;
    JITINT32 libjitDIDT;
} didt_log_t;

void MORPHEUS_Initialize (didt_log_t *log);
void MORPHEUS_StartExecutionNotify ();
void internal_insert_native_start_function (Method method);
void internal_insert_native_stop_function (Method method);

#endif
#endif
