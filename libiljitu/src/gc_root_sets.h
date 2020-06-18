/*
 * Copyright (C) 2006  Campanoni Simone
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
#ifndef GC_ROOT_SETS_H
#define GC_ROOT_SETS_H

#include <jitsystem.h>
#include <platform_API.h>

typedef struct t_root_sets {
    pthread_mutex_t mutex;
    void            ***rootSets;
    JITUINT32 top;
    JITUINT32 allocated;
    JITNUINT _size;
    JITINT16 (*shutdown)(struct t_root_sets *rootSets);
    void (*addNewRootSetSlot)(struct t_root_sets *rootSets, void **varPointer);
    void (*addNewRootSet)(struct t_root_sets *rootSets);
    void **         (*getRootSetSlot)(struct t_root_sets *rootSets, JITUINT32 *slotID);
    void (*popLastRootSet)(struct t_root_sets *rootSets);
    JITUINT32 (*length)(struct t_root_sets *rootSets);
    JITBOOLEAN (*isInRootSets)(struct t_root_sets *rootSets, void *object);
    void (*lock)(struct t_root_sets *rootSets);
    void (*unlock)(struct t_root_sets *rootSets);
} t_root_sets;

void init_root_sets (t_root_sets *rootSets);

#endif
