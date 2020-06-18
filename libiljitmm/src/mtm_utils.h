/*
 * Copyright (C) 2006 - 2010  Campanoni Simone
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


#ifndef MTM_UTILS_H
#define MTM_UTILS_H

#define MAX(m, n) m > n ? m : n

#include <metadata_table_manager.h>

void * private_get_table (t_metadata_tables *tables, JITUINT16 table_ID);
JITBOOLEAN private_is_type_def_or_ref_token_32 (void);
JITBOOLEAN private_is_method_def_or_ref_token_32 (void);
JITBOOLEAN private_is_type_or_method_def_token_32 (void);
JITUINT32 getBlobLength (JITINT8 *buf, JITUINT32 *bytesRead);
inline JITUINT32 uncompressValue (JITINT8 *signature, JITUINT32 *uncompressed);

#endif
