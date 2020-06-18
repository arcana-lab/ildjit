/*
 * Copyright (C) 2006  Campanoni Simone, Di Biagio Andrea, Farina Roberto
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
#ifndef LAYOUT_MANAGER_H
#define LAYOUT_MANAGER_H

#include <iljit-utils.h>
#include <metadata/metadata_types.h>

// My headers
#include <ilmethod.h>
// End

typedef struct  _IMTElem {
    MethodDescriptor *method;
    void *pointer;
    struct _IMTElem *next;
} IMTElem;

typedef struct _MethodImplementations {
    XanList *implementor;
    XanList *closure;
} MethodImplementations;

typedef struct {
    FieldDescriptor         *field;         //metadata informations about the field
    struct ILLayout         *layout;        //differs from NULL only if the field is a `ValueType`
    JITINT32 IRType;                        //the IRType associated with the field
    JITINT32 offset;                        //the offset is calculated starting from the beginning of the INSTANCE
    JITINT32 length;                        //the length of the field in memory
} ILFieldLayout;

/**
 * Layout of a class
 */
typedef struct ILLayout {
    pthread_mutex_t 	mutex;
    struct ILLayout_manager *manager;
    TypeDescriptor		*type;          	/**< Metadata information about the type					*/
    IRVM_type		**jit_type;     	/**< IR virtual machine type, make use of PMP					*/
    XanList         	*fieldsLayout;  	/**< An ordered list of ILFieldLayout structures				*/
    JITUINT32 		type_of_layout; 	/**< A constant that define the type of layout					*/
    JITUINT32 		typeSize; 		/**< Size of an instance of this type in memory					*/
    JITUINT32 		packing_size;           /**< Packing size used for explicit layout of fields				*/
    JITUINT32 		virtualTableLength;   	/**< The length of the virtual table (may differs from methodsNumber)		*/
    void            	***virtualTable;        /**< A pointer to the virtual table of the current type, make use of PMP	*/
    IMTElem         	**interfaceMethodTable; /**< Cache the generic interface method pointers, make use of PMP		*/
    Method			*virtualTableMethods;	/**< Methods that compose the virtual table.					*/
    XanHashTable    	*methods;               /**< Resolved methods. Contains (t_virtual_method *) elements			*/
    ILFieldLayout   * (*fetchFieldLayout)(struct ILLayout *layout, FieldDescriptor *field);
    void          		*initialized_obj;       /**< Fresh copy of the object initialized with RVA				*/
    Method	 		finalize;		/**< The Finalize method							*/
} ILLayout;

typedef struct ILLayoutStatic {
    struct ILLayout_manager *manager;
    TypeDescriptor          *type;          //metadata informations about the type
    XanList         *fieldsLayout;          //an ordered list of ILFieldLayout structures
    JITUINT32 typeSize;                     //size of an instance of this type in memory
    ILFieldLayout * (*fetchStaticFieldLayout)(struct ILLayoutStatic *layout, FieldDescriptor *field);
    void            *initialized_obj;       //fresh copy of the object initialized with RVA
} ILLayoutStatic;

typedef struct ILLayout_manager {
    JITBOOLEAN		cacheVirtualMethodRequests;
    XanList			*cachedVirtualMethodRequests;
    XanList         *cachedVirtualTableSets;
    XanList                 *classesLayout;
    XanList                 *staticTypeLayout;
    XanHashTable            *methodImplementationInfos;
    ILLayout *              _Runtime_Field_Handle;
    ILLayout *              _Runtime_Method_Handle;
    ILLayout *              _Runtime_Type_Handle;
    ILLayout *              _Runtime_Argument_Handle;
    IRVM_type * (*layoutJITType)(struct ILLayout_manager *manager, TypeDescriptor *type);
    ILLayout *              (*layoutType)(struct ILLayout_manager *manager, TypeDescriptor *type);
    ILLayoutStatic *        (*layoutStaticType)(struct ILLayout_manager *manager, TypeDescriptor *type);
    ILLayout *              (*getRuntimeFieldHandleLayout)(struct ILLayout_manager *manager);
    ILLayout *              (*getRuntimeMethodHandleLayout)(struct ILLayout_manager *manager);
    ILLayout *              (*getRuntimeTypeHandleLayout)(struct ILLayout_manager *manager);
    ILLayout *              (*getRuntimeArgumentHandleLayout)(struct ILLayout_manager *manager);
    VCallPath (*prepareVirtualMethodInvocation)(struct ILLayout_manager *manager, MethodDescriptor *methodID);
    MethodImplementations *         (*getMethodImplementations)(struct ILLayout_manager *manager, MethodDescriptor *methodID);
    JITINT32 (*getIndexOfMethodDescriptor)(MethodDescriptor *method);
    void (*initialize)(void);
} ILLayout_manager;

void LAYOUT_initManager (ILLayout_manager *manager);
void LAYOUT_setCachingRequestsForVirtualMethodTables (ILLayout_manager *manager, JITBOOLEAN cache);
void LAYOUT_createCachedVirtualMethodTables (ILLayout_manager *manager);
void LAYOUT_setMethodTables (TypeDescriptor *ilType, void *vTable, IMTElem *IMT);
void LAYOUT_getMethodTables (ILLayout *layout, void **vTable, IMTElem **IMT);
void LAYOUT_destroyManager (ILLayout_manager *manager);
void debug_print_layout_informations (ILLayout *infos);
void debug_print_layoutStatic_informations (ILLayoutStatic *infos);

#endif
