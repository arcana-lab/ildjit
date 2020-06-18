
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
#ifndef BASIC_CONCEPT_MANAGER_H
#define BASIC_CONCEPT_MANAGER_H

#include <basic_types.h>
#include <metadata_types.h>

JITUINT32 hashExAttributesID (void *element);
JITINT32 equalsExAttributesID (void *key1, void *key2);

JITUINT32 hashBasicClassID (void *element);
JITINT32 equalsBasicClassID (void *key1, void *key2);

inline BasicTypeAttributes *getBasicTypeAttributes(metadataManager_t * manager, BasicClass * class );
inline BasicMethodAttributes *getBasicMethodAttributes (metadataManager_t *manager, BasicMethod *method);
inline BasicFieldAttributes *getBasicFieldAttributes (metadataManager_t *manager, BasicField *field);
inline BasicEventAttributes *getBasicEventAttributes (metadataManager_t *manager, BasicEvent *event);
inline BasicPropertyAttributes *getBasicPropertyAttributes (metadataManager_t *manager, BasicProperty *property);
inline BasicParamAttributes *getBasicParamAttributes (metadataManager_t *manager, BasicParam *param);
inline BasicGenericParamAttributes *getBasicGenericParamAttributes (metadataManager_t *manager, BasicGenericParam *param);


inline BasicClass *getBasicClassFromName (metadataManager_t *manager, JITINT8 *type_name_space, JITINT8 *type_name);
inline BasicClass *getBasicClassFromNameAndTypeNameSpace (metadataManager_t *manager, t_binary_information *binary, JITINT8 *type_name_space, JITINT8 *type_name);
inline BasicClass *getBasicClassFromTypeDefOrRef (metadataManager_t *manager, t_binary_information *binary, JITUINT32 type_token);
inline BasicClass *getBasicClassFromTableAndRow (metadataManager_t *manager, t_binary_information *binary, JITUINT8 table_id, JITUINT32 row_id);

inline JITUINT32 skipCustomMod (JITINT8 *signature);

#define isCmod(type) (((type)[0] == ELEMENT_TYPE_CMOD_REQD) || ((type)[0] == ELEMENT_TYPE_CMOD_OPT))
#define isType(type) (((type)[0] & 0xFF) <= 0x1e)
#define isPinned(type) ((type)[0] == ELEMENT_TYPE_PINNED)
#define isSentinel(type) ((type)[0] == ELEMENT_TYPE_SENTINEL)
#define isByref(type) ((type)[0] == ELEMENT_TYPE_BYREF)
#define isTypedRef(type) ((type)[0] == ELEMENT_TYPE_TYPEDBYREF)
#define isVoid(type) ((type)[0] == ELEMENT_TYPE_VOID)

#endif
