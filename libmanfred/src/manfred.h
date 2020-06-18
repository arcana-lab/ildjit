/*
 * Copyright (C) 2010 - 2012  Campanoni Simone
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
#ifndef MANFRED_H
#define MANFRED_H

#include <jitsystem.h>
#include <xanlib.h>
#include <decoder.h>
#include <ilmethod.h>
#include <jitsystem.h>
#include <cli_manager.h>

typedef struct manfred_t {
    XanList 	*methodsTrace;
    XanList 	*symbolsToProcess;
    XanList 	*methodsToInitialize;
    CLIManager_t 	*cliManager;
    JITBOOLEAN 	isLoading;
    JITINT8 	*programDirectory;
    JITINT32	isProgramPreCompiled;
    ir_symbol_t     *nullSymbol;
    ir_symbol_t    	**loadedSymbol;
    JITUINT32	loadedSymbolSize;
} manfred_t;

ir_symbol_t * MANFRED_loadSymbol (manfred_t *_this, JITUINT32 number);

void MANFRED_addToSymbolToSave (manfred_t *_this, ir_symbol_t *symbol);

JITBOOLEAN MANFRED_isProgramPreCompiled (manfred_t *self);

void MANFRED_saveProfile (manfred_t *_this, JITINT8 *directoryName);

void MANFRED_saveProgram (manfred_t *_this, JITINT8 *directoryName);

void MANFRED_loadProgram (manfred_t *_this, JITINT8 *directoryName);

XanList * MANFRED_loadProfile (manfred_t *_this, JITINT8 *directoryName);

void MANFRED_appendMethodToProfile (manfred_t *_this, Method method);

void MANFRED_init (manfred_t *self, CLIManager_t *cliManager);

void MANFRED_shutdown (manfred_t *self);

char * libmanfredVersion ();

void libmanfredCompilationFlags (char *buffer, JITINT32 bufferLength);

void libmanfredCompilationTime (char *buffer, JITINT32 bufferLength);

#endif
