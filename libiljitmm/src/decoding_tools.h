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


// My headers
#ifndef DECODING_TOOLS_H
#define DECODING_TOOLS_H

#include <xanlib.h>
#include <metadata_types.h>
#include <metadata_types_spec.h>


inline JITUINT32 getTypeDescriptorFromTypeBlob (metadataManager_t *manager, GenericContainer *container, t_binary_information *binary, JITINT8 *signature, TypeDescriptor **infos);
inline JITUINT32 getFunctionPointerDescriptorFromBlob (metadataManager_t *manager, GenericContainer *container, t_binary_information *binary, JITINT8 *signature, FunctionPointerDescriptor **infos);

inline TypeDescriptor *getTypeDescriptorFromTypeDeforRefToken (metadataManager_t *manager, GenericContainer *container, t_binary_information *binary, JITUINT32 type_token);
inline TypeDescriptor *getTypeDescriptorFromTableAndRow (metadataManager_t *manager, GenericContainer *container, t_binary_information *binary, JITUINT8 table_id, JITUINT32 row_id);

inline FieldDescriptor *getFieldDescriptorFromTableAndRow (metadataManager_t *manager, GenericContainer *container, t_binary_information *binary, JITUINT8 table_id, JITUINT32 row_id);

inline FunctionPointerDescriptor *getFormalSignatureFromBlob (metadataManager_t *manager, GenericContainer *container, BasicMethod *row);
inline FunctionPointerSpecDescriptor *getRawSignatureFromBlob (metadataManager_t *manager, GenericSpecContainer *container, BasicMethod *row);

inline MethodDescriptor *getMethodDescriptorFromMethodDefOrRef (metadataManager_t *manager, GenericContainer *container, t_binary_information *binary, JITUINT32 type_token, FunctionPointerDescriptor **actualSignature);
inline MethodDescriptor *getMethodDescriptorFromTableAndRow (metadataManager_t *manager, GenericContainer *container, t_binary_information *binary, JITUINT8 table_id, JITUINT32 row_id, FunctionPointerDescriptor **actualSignature);

inline XanList *decodeMethodLocals (metadataManager_t *manager, t_binary_information *binary, JITUINT32 token, GenericContainer *container);

#endif
