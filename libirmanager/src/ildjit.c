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
 * MERCHANTABIRITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// My headers
#include <ir_method.h>
#include <ildjit.h>
// End

extern ir_lib_t *ir_library;

compilation_scheme_t ILDJIT_compilationSchemeInUse (void) {
    return ir_library->compilation;
}

JITBOOLEAN ILDJIT_isAStaticCompilationScheme (compilation_scheme_t cs) {
    if (	(ir_library->compilation == staticScheme)	||
            (ir_library->compilation == aotScheme)		) {
        return JITTRUE;
    }

    return JITFALSE;
}

JITBOOLEAN ILDJIT_isADynamicCompilationScheme (compilation_scheme_t cs) {
    return !ILDJIT_isAStaticCompilationScheme(cs);
}
