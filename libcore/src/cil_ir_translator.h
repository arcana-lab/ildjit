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
/**
 * @file cil_ir_translator.h
 * @brief CIL->IR translator
 *
 * This module implements the translation of the CIL methods into the IR equivalent one.
 */
#ifndef CIL_IR_TRANSLATOR_H
#define CIL_IR_TRANSLATOR_H

#include <xanlib.h>

// My headers
#include <jitsystem.h>
#include <decoder.h>
#include <system_manager.h>
#include <ilmethod.h>
// End

/**
 *
 *
 * @param method Method allocated by the caller, but fill up by the callee
 * @param stack CIL Stack allocated by the caller
 */
JITINT16 translate_method_from_cil_to_ir (t_system *system, Method method);
MethodDescriptor * retrieve_CILException_Ctor_MethodID_by_type (t_methods *methods, TypeDescriptor *classID);

#endif
