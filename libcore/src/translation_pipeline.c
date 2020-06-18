/*
 * Copyright (C) 2006 - 2012  Simone Campanoni
 *
 * Compilation pipeline system. In this module the compilation tasks are dispatched and performed in parallel.
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
 * @file translation_pipeline.c
 * @brief Pipeline system
 */
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <xanlib.h>
#include <iljit-utils.h>
#include <error_codes.h>
#include <ir_optimization_interface.h>
#include <compiler_memory_manager.h>
#include <dla.h>
#include <ir_virtual_machine.h>
#include <jit_metadata.h>
#include <platform_API.h>

// My headers
#include <translation_pipeline_sequential.h>
#include <translation_pipeline_parallel.h>
// End

#define SEQUENTIAL_PIPELINER

void init_pipeliner (TranslationPipeline *pipeline) {
#ifdef SEQUENTIAL_PIPELINER
    SEQUENTIALPIPELINER_init(pipeline);
#else
    PARALLELPIPELINER_init(pipeline);
#endif

    return;
}
