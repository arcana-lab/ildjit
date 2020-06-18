/*
 * Copyright (C) 2009  Simone Campanoni, Luca Rocchini
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

#include <compiler_memory_manager.h>
#include <metadata_types_manager.h>
#include <basic_concept_manager.h>
#include <iljitmm-system.h>
#include <assert.h>

inline GenericContainer *newGenericContainer (ContainerType container_type, GenericContainer *parent, JITUINT32 arg_count) {
    GenericContainer *container = sharedAllocFunction(sizeof(GenericContainer));

    container->container_type = container_type;
    container->parent = parent;
    container->arg_count = arg_count;
    if (arg_count > 0) {
        container->paramType = sharedAllocFunction(sizeof(TypeDescriptor *) * arg_count);
    } else {
        container->paramType = NULL;
    }
    return container;
}

inline void destroyGenericContainer (GenericContainer *container) {
    freeFunction(container->paramType);
    freeFunction(container);
}

inline GenericContainer *cloneGenericContainer (GenericContainer *self) {
    if (self == NULL) {
        return NULL;
    }
    GenericContainer *container = sharedAllocFunction(sizeof(GenericContainer));
    memcpy(container, self, sizeof(GenericContainer));
    if (self->arg_count > 0) {
        container->paramType = sharedAllocFunction(sizeof(TypeDescriptor *) * (self->arg_count));
        memcpy(container->paramType, self->paramType, sizeof(TypeDescriptor *) * (self->arg_count));
    }
    assert(equalsGenericContainer(self, container) == 1);
    return container;
}

JITUINT32 hashGenericContainer (GenericContainer *container) {
    if (container == NULL) {
        return 0;
    }
    JITUINT32 seed = container->container_type;
    JITUINT32 count;
    for (count = 0; count < container->arg_count; count++) {
        TypeDescriptor *current_element = (container->paramType)[count];
        seed = combineHash(seed, (JITUINT32) current_element);
    }
    seed = combineHash(seed, container->parent);
    return seed;
}

JITINT32 equalsGenericContainer (GenericContainer *key1, GenericContainer *key2) {
    if (key1 == key2) {
        return 1;
    }
    if (key1 == NULL || key2 == NULL) {
        return 0;
    }
    if (key1->container_type != key2->container_type) {
        return 0;
    }
    if (key1->parent != key2->parent) {
        return 0;
    }
    if (key1->arg_count != key2->arg_count) {
        return 0;
    }
    if (key1->arg_count > 0) {
        if (memcmp(key1->paramType, key2->paramType, sizeof(TypeDescriptor *) * key1->arg_count) != 0) {
            return 0;
        }
    }
    return 1;
}

inline void insertTypeDescriptorIntoGenericContainer (GenericContainer *self, TypeDescriptor *type, JITUINT32 position) {
    (self->paramType)[position] = type;
}
