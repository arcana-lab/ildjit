/*
 * Copyright (C) 2010 - 2012 Campanoni Simone
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
#ifndef MANFRED_LOAD_H
#define MANFRED_LOAD_H
#include <metadata/metadata_types.h>

typedef struct _ir_type_definition_t {
    JITUINT32 internal_type;
    TypeDescriptor *type_infos;
} ir_type_definition_t;

JITINT32 manfred_yylex (void);
void manfred_yyerror (char *s);

#endif
