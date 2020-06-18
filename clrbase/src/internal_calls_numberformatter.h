/*
 * Copyright (C) 2009  Campanoni Simone
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

#ifndef INTERNAL_CALLS_NUMBERFORMATTER_H
#define INTERNAL_CALLS_NUMBERFORMATTER_H

#include <jitsystem.h>

/**
 * Get the formatter tables
 *
 * This is a Mono internal call
 *
 * @param mantissaBitsTable pointer to the field that will store a pointer to
 *                          the mantissa bits table
 * @param tensExponentTable pointer to the field that will store a pointer to
 *                          the tens exponent table
 * @param digitLowerTable pointer to the field that will store a pointer to the
 *                        digit lower table
 * @param digitUpperTable pointer to the field that will store a pointer to the
 *                        digit upper table
 * @param tenPowersList pointer to the field that will store a pointer to the
 *                      ten powers list table
 * @param decHexDigits pointer to the field that will store a pointer to the dec
 *                     hex digits table
 */
void System_NumberFormatter_GetFormatterTables (JITUINT64** mantissaBitsTable,
        JITINT32** tensExponentTable,
        JITINT16** digitLowerTable,
        JITINT16** digitUpperTable,
        JITINT64** tenPowersList,
        JITINT32** decHexDigits);

#endif /* INTERNAL_CALLS_NUMBERFORMATTER_H */
