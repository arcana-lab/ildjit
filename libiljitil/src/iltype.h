/*
 * Copyright (C) 2012  Simone Campanoni
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
 * @file iltype.h
 */
#ifndef ILTYPE_H
#define ILTYPE_H

/**
 * \defgroup ILTYPE Data type
 * \ingroup InputLanguage
 */

#include <jitsystem.h>

typedef struct void * il_type_t ;

/**
 * \ingroup ILTYPE
 * @brief Check whether a type correspond to an IR builtin data type
 *
 * A data type of the input language can be mapped to an IR builtin data type or not.
 *
 * This function can be used to figure this out.
 *
 * @param type Input language data type
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN ILTYPE_isPrimitive (il_type_t *type);

JITBOOLEAN ILTYPE_isSubtypeOf (il_type_t *type, il_type_t *superType);
JITBOOLEAN ILTYPE_assignableTo (il_type_t *type, il_type_t *typeToCastTo);
JITUINT32 ILTYPE_getIRType (il_type_t *type);

#endif
