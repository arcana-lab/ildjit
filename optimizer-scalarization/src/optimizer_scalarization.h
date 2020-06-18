/*
 * Copyright (C) 2013-2014  Campanoni Simone
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
#ifndef OPTIMIZER_SCALARIZATION_H
#define OPTIMIZER_SCALARIZATION_H

/**
 * Mark a variable as possibly unused to gcc.
 **/
#define JITUNUSED __attribute__ ((unused))


/**
 * Debug printing.
 **/
#ifdef PRINTDEBUG
extern JITINT32 debugLevel;
#define PDEBUG(lvl, fmt, args...) do { if (lvl <= debugLevel) fprintf(stderr, "SCALAR: " fmt, ## args); } while (0)
#define PDEBUGC(lvl, fmt, args...) do { if (lvl <= debugLevel) fprintf(stderr, fmt, ## args); } while (0)
#define PDEBUGNL(lvl, fmt, args...) do { if (lvl <= debugLevel) fprintf(stderr, "SCALAR:\nSCALAR: " fmt, ##args); } while (0)
#define PDEBUGSTMT(lvl, stmt) do { if (lvl <= debugLevel) stmt; } while (0)
#define PDEBUGLEVEL(lvl) do { debugLevel = lvl; } while (0)
#define PDECL(decl) decl
#else
#define PDEBUG(lvl, fmt, args...)
#define PDEBUGC(lvl, fmt, args...)
#define PDEBUGNL(lvl, fmt, args...)
#define PDEBUGSTMT(lvl, stmt)
#define PDEBUGLEVEL(lvl)
#define PDECL(decl)
#endif

#endif
