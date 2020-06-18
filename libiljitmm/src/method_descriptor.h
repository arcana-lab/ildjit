
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
#ifndef METHOD_DESCRIPTOR_H
#define METHOD_DESCRIPTOR_H

#include <xanlib.h>
#include <metadata_types.h>
#include <metadata_types_spec.h>

/* Actual Method Descriptor utility methods */
inline ActualMethodDescriptor *newActualMethodDescriptor ();

/* MethodDescriptor Accessor Method */
inline MethodDescriptor *newMethodDescriptor (metadataManager_t *manager, BasicMethod *row, TypeDescriptor *owner, GenericContainer *container);

inline JITBOOLEAN requireConservativeOwnerInitialization (MethodDescriptor *method);
inline JITINT8 *getNameFromMethodDescriptor (MethodDescriptor *method);
inline JITINT8 *getCompleteNameFromMethodDescriptor (MethodDescriptor *method);
inline JITINT8 *getMethodDescriptorSignatureInString (MethodDescriptor *method);
inline JITINT8 *getMethodDescriptorInternalName (MethodDescriptor *method);

inline JITBOOLEAN isCtor (MethodDescriptor *method);
inline JITBOOLEAN isCctor (MethodDescriptor *method);
inline JITBOOLEAN isFinalizer (MethodDescriptor *method);

inline JITINT8 *getImportName (MethodDescriptor *method);
inline JITINT8 *getImportModule (MethodDescriptor *method);

inline MethodDescriptor *getInstanceFromMethodDescriptor (MethodDescriptor *method, GenericContainer *container);
inline MethodDescriptor *getGenericDefinitionFromMethodDescriptor (MethodDescriptor *method);

inline FunctionPointerSpecDescriptor *getRawSignature (MethodDescriptor *method, GenericSpecContainer *container);

inline JITUINT32 getOverloadingSequenceID (MethodDescriptor *method);

inline MethodDescriptor *getBaseDefinition (MethodDescriptor *method);

inline PropertyDescriptor *getProperty (MethodDescriptor *method);
inline EventDescriptor *getEvent (MethodDescriptor *method);

inline FunctionPointerDescriptor *getFormalSignature (MethodDescriptor *method);

inline XanList *getParams (MethodDescriptor *method);
inline ParamDescriptor *getResult (MethodDescriptor *method);

inline JITBOOLEAN isCallebleMethod (MethodDescriptor *callee, MethodDescriptor *caller);

inline XanList *getGenericParametersFromMethodDescriptor (MethodDescriptor *method);
inline JITBOOLEAN isGenericMethodDefinition (MethodDescriptor *method);
inline JITBOOLEAN containOpenGenericParameterFromMethodDescriptor (MethodDescriptor *method);
inline JITBOOLEAN isGenericMethodDescriptor (MethodDescriptor *method);
inline GenericContainer *getResolvingContainerFromMethodDescriptor (MethodDescriptor *method);

inline void methodDescriptorLock (MethodDescriptor *method);
inline void methodDescriptorUnLock (MethodDescriptor *method);

inline DescriptorMetadataTicket *methodDescriptorCreateDescriptorMetadataTicket (MethodDescriptor *method, JITUINT32 type);
inline void methodDescriptorBroadcastDescriptorMetadataTicket (MethodDescriptor *method,  DescriptorMetadataTicket *ticket, void * data);

/* Param Descriptor Accessor Method */
inline ParamDescriptor *newParamDescriptor (metadataManager_t *manager, BasicParam *row, TypeDescriptor *type, MethodDescriptor *owner);
inline JITINT8* getNameFromParamDescriptor (ParamDescriptor *param);
inline JITINT8* getSignatureInStringFromParamDescriptor (ParamDescriptor *param);
inline JITUINT32 getIRTypeFromParamDescriptor (ParamDescriptor *param);

inline void paramDescriptorLock (ParamDescriptor *param);
inline void paramDescriptorUnLock (ParamDescriptor *param);

inline DescriptorMetadataTicket *paramDescriptorCreateDescriptorMetadataTicket (ParamDescriptor *param, JITUINT32 type);
inline void paramDescriptorBroadcastDescriptorMetadataTicket (ParamDescriptor *param,  DescriptorMetadataTicket *ticket, void * data);

/* Local Descriptor Accessor Method */
inline LocalDescriptor *newLocalDescriptor (JITBOOLEAN is_pinned, TypeDescriptor *type);

inline JITUINT32 getIRTypeFromLocalDescriptor (LocalDescriptor *local);

inline DescriptorMetadataTicket *localDescriptorCreateDescriptorMetadataTicket (LocalDescriptor *local, JITUINT32 type);
inline void localDescriptorBroadcastDescriptorMetadataTicket (LocalDescriptor *local,  DescriptorMetadataTicket *ticket, void * data);
#endif
