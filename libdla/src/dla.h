/*
 * Copyright (C) 2009 Campanoni Simone
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
 * @file dla.h
 */
#ifndef DLA_H
#define DLA_H

#include <xanlib.h>
#include <ilmethod.h>
#include <ir_virtual_machine.h>

typedef struct {
    Method method;
    JITFLOAT32 callInstructionProbability;
    JITFLOAT32 intermediatePriority;
    JITFLOAT32 priority;
} dla_method_t;

/**
 * DLA Compiler
 */
typedef struct DLA_t {
    IRVM_t          *IRVM;
    JITFLOAT32 probabilityBoundary;

    XanList *       (*getMethodsToCompile)(struct DLA_t *_this, Method method, JITBOOLEAN *toBeCompiled);   /**< Returns a list of methods to compile. */
    void (*updateMethodsToCompile)(struct DLA_t *_this, Method method, XanList *methods, JITBOOLEAN *toBeCompiled);
    void (*shutdown)(struct DLA_t *_this);                                                                  /**< Shutdown the DLA compiler. */
} DLA_t;

void DLANew (DLA_t *_this, IRVM_t *IRVM);

char * libdlaVersion ();
void libdlaCompilationFlags (char *buffer, JITINT32 bufferLength);
void libdlaCompilationTime (char *buffer, JITINT32 bufferLength);

#endif
