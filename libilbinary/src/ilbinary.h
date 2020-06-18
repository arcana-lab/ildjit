/*
 * Copyright (C) 2012 - 2013  Campanoni Simone
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
#ifndef ILBINARY_H
#define ILBINARY_H

#include <stdio.h>
#include <decoder.h>

t_binary_information * ILBINARY_new (void);

void ILBINARY_setBinaryFile (t_binary_information *ilBinary, FILE *ilBinaryFile);

JITUINT32 ILBINARY_decodeFileType (t_binary_information *binary_info, JITINT8 binary_ID[2]);

void ILBINARY_setBinaryName (t_binary_information *ilBinary, JITINT8 *name);

#endif
