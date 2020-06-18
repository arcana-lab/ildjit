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
#ifndef LIB_STRINGBUILDER_H
#define LIB_STRINGBUILDER_H

#include <jitsystem.h>
#include <metadata/metadata_types.h>

typedef struct {
    TypeDescriptor          *StringBuilderType;
    FieldDescriptor         *buildStringField;
    FieldDescriptor         *maxCapacityField;
    FieldDescriptor         *needsCopyField;
    JITINT32 buildStringOffset;
    JITINT32 maxCapacityOffset;
    JITINT32 needsCopyOffset;

    void *          (*getString)(void *stringBuilder);
    JITINT32 (*getMaxCapacity)(void *stringBuilder);
    void (*setString)(void *stringBuilder, void *string);
    void *          (*append)(void *stringBuilder, JITINT16 * literal, JITINT32 stringLength);
    JITINT32 (*getNewCapacity)(JITINT32 length, JITINT32 oldCapacity);
    JITINT8 (*getNeedsCopy)(void *stringBuilder);
    void (*setNeedsCopy)(void *stringBuilder, JITINT8 value);
    void *          (*insert)(void *stringBuilder, JITINT32 index, JITINT32 length);
    void (*printdbg)(void *stringBuilder, char* descrizione, JITINT8 mode);                          /*AGGIUNTA PER IL DEBUG*/
    void (*initialize)(void);
} t_stringBuilderManager;

void init_stringBuilderManager (t_stringBuilderManager *stringBuilderManager);

#endif
