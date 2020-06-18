/*
 * Copyright (C) 2007-2010  Campanoni Simone, Michele Melchiori
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
#ifndef OPTIMIZER_SSA_BACK_CONVERT_H
#define OPTIMIZER_SSA_BACK_CONVERT_H

#ifdef PRINTDEBUG
#define PDEBUG(fmt, args...) fprintf(stderr, fmt, ## args)
#else
#define PDEBUG(fmt, args...)
#endif

/**
 * @defgroup SSA_BACK_CONVERT_OPTIONS Options for the SSA back conversion plugin
 */

/**
 * @def OPTIMIZER_SSA_BACK_CONVERT_REMOVE_USELESS_CODE
 * @ingroup SSA_BACK_CONVERT_OPTIONS
 *
 * type: boolean
 * If equal to \c JITTRUE at the end of the optimization the plugin calls
 * continuosly these plugins until no modification at the method code are made:
 * <ul>
 * <li>\c CONSTANT_PROPAGATOR</li>
 * <li>\c COPY_PROPAGATOR</li>
 * <li>\c DEADCODE_ELIMINATOR</li>
 * <li>\c NULL_CHECK_REMOVER</li>
 * <li>\c ALGEBRAIC_SIMPLIFICATION</li>
 * </ul>
 * This is done in order to reduce the number of locals in the method, because
 * the register allocator may go crazy when many useless variables are used in
 * the method.
 */
#define OPTIMIZER_SSA_BACK_CONVERT_REMOVE_USELESS_CODE JITFALSE

#endif
