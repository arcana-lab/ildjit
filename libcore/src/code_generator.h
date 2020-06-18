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
#ifndef CODE_GENERATOR_H
#define CODE_GENERATOR_H

#include <ilmethod.h>

// My headers
#include <system_manager.h>
// End

void CODE_init (CodeGenerator *self);
void CODE_shutdown (CodeGenerator *self);
Method CODE_generateWrapperOfEntryPoint (CodeGenerator *self);
void CODE_generateMachineCode (CodeGenerator *self, Method method);
void CODE_linkMethodToProgram (CodeGenerator *self, Method method);
void CODE_cacheCodeGeneration (CodeGenerator *self);
void CODE_generateAndLinkCodeForCachedMethods (CodeGenerator *self);
JITUINT32 CODE_optimizeIR (CodeGenerator *self, Method method, JITUINT32 jobState, XanVar *checkPoint);

#endif
