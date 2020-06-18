/*
 * Copyright (C) 2008  Campanoni Simone
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
#include <jit_metadata.h>
#include <framework_garbage_collector.h>
#include <ildjit.h>

// My header
#include <internal_calls_reflection.h>
#include <internal_calls_utilities.h>
#include <internal_calls_IO.h>
#include <clr_system.h>
// End

#define ASSEMBLY_FULL_NAME_TEMPLATE "%s, Version = %u.%u.%u.%u, Culture = %s, Hash algorithm = %s"

static inline void * internal_GetExecutingAssembly (void);
static inline BasicAssembly * getAssemblyFromName (JITINT8 *name);

extern t_system *ildjitSystem;


void System_ModuleHandle_ResolveMethodHandle_Int32 (void *clrModule, JITINT32 token, void *runtimeMethodHandle) {
    RuntimeHandleManager	*rhManager;
    t_reflectManager	*refManager;
    ILConcept 		decoded_concept;
    MethodDescriptor	*fetch_descriptor;
    JITINT32		valueOffset;
    CLRModule 		*internalClrModule;
    BasicModule		*basicModule;
    t_binary_information	*binaryToUse;
    void			**ptrToStore;

    /* Assertions.
     */
    assert(clrModule != NULL);
    assert(runtimeMethodHandle != NULL);
    METHOD_BEGIN(ildjitSystem, "RuntimeMethodHandle System.ModuleHandle.ResolveMethodHandle(ModuleHandle *m, int token)");

    /* Cache some pointers.
     */
    rhManager 	= &((ildjitSystem->cliManager).CLR.runtimeHandleManager);
    refManager	= &((ildjitSystem->cliManager).CLR.reflectManager);

    /* Fetch the CLR module.
     */
    internalClrModule	= refManager->getPrivateDataFromModule(clrModule);
    assert(internalClrModule != NULL);
    basicModule	= internalClrModule->ID;
    assert(basicModule != NULL);
    binaryToUse	= ((t_row_module_table *) basicModule)->binary;
    assert(binaryToUse != NULL);

    /* Decode the token.
     */
    fetch_descriptor	= NULL;
    decoded_concept		= (ildjitSystem->cliManager).metadataManager->fetchDescriptor(ildjitSystem->cliManager.metadataManager, NULL, binaryToUse, token, (void *)&fetch_descriptor);
    if (decoded_concept != ILMETHOD) {
        fprintf(stderr, "ILDJIT: ERROR = Library function System.ModuleHandle.ResolveMethodHandle did not find the method requested.\n");
        fprintf(stderr, "	Token of the method = %u\n", token);
        abort();
    }

    /* Initialize the runtime method handle.
     */
    valueOffset	= rhManager->getRuntimeMethodHandleValueOffset(rhManager);
    ptrToStore	= ((void **)(((JITNINT)runtimeMethodHandle) + valueOffset));
    assert(ptrToStore != NULL);
    (*ptrToStore)	= (void *)fetch_descriptor;
    assert((*ptrToStore != NULL));

    METHOD_END(ildjitSystem, "RuntimeMethodHandle System.ModuleHandle.ResolveMethodHandle(ModuleHandle *m, int token)");
    return ;
}

void * System_RuntimeMethodHandle_GetFunctionPointer (void *runtimeMethodHandle) {
    RuntimeHandleManager	*rhManager;
    MethodDescriptor	*mID;
    Method			method;
    ir_method_t		*irMethod;
    void			*fPtr;

    /* Assertions.
     */
    assert(runtimeMethodHandle != NULL);

    /* Initialize the variables.
     */
    fPtr		= NULL;

    /* Cache some pointers.
     */
    rhManager 	= &((ildjitSystem->cliManager).CLR.runtimeHandleManager);

    /* Fetch the method descriptor.
     */
    mID		= rhManager->getRuntimeMethodHandleValue(rhManager, runtimeMethodHandle);
    assert(mID != NULL);

    /* Fetch the method.
     */
    method		= (ildjitSystem->cliManager).fetchOrCreateMethod(mID);
    assert(method != NULL);
    irMethod	= method->getIRMethod(method);
    assert(irMethod != NULL);

    /* Fetch the function pointer.
     */
    fPtr		= IRMETHOD_getFunctionPointer(irMethod);
    assert(fPtr != NULL);

    return fPtr;
}

//***************************************************************************************************************/
//*							CLR_TYPE						*/
//***************************************************************************************************************/
void * System_Reflection_ClrType_GetClrModule (void *_this) {
    void                    * obj;
    CLRType                 * clrType;
    TypeDescriptor  *type;
    BasicAssembly   *assembly;
    t_streams_metadata      * streams;
    t_metadata_tables       * tables;
    BasicModule     * module_row;

    /*Assertion	*/
    assert(_this != NULL);

    METHOD_BEGIN(ildjitSystem, "System.Reflection.ClrType.GetClrModule");

    /*Initialize the variables	*/
    obj = NULL;
    clrType = NULL;
    streams = NULL;
    tables = NULL;
    module_row = NULL;

    /* Fetch the clrType	*/
    clrType = ((ildjitSystem->cliManager).CLR.reflectManager).getPrivateDataFromType(_this);
    assert(clrType != NULL);
    assert(clrType->ID != NULL);

    /* Fetch the tables	*/
    type = clrType->ID;
    assembly = type->getAssembly(type);
    streams = &(((t_binary_information *) assembly->binary)->metadata.streams_metadata);
    assert(streams != NULL);
    tables = &(streams->not_stream.tables);
    assert(tables != NULL);

    module_row = (BasicModule *) get_first_row(tables, MODULE_TABLE);
    assert(module_row != NULL);

    obj = ((ildjitSystem->cliManager).CLR.reflectManager).getModule(module_row);
    assert(obj != NULL);

    METHOD_END(ildjitSystem, "System.Reflection.ClrType.GetClrModule");
    return obj;
}

void * System_Reflection_ClrType_GetClrNestedDeclaringType (void *_this) {
    void                    *obj;
    CLRType                 *clrType;
    TypeDescriptor  *type;
    TypeDescriptor  *ilType;
    void                    *string;

    assert(_this != NULL);
    METHOD_BEGIN(ildjitSystem, "System.Reflection.ClrType.GetClrNestedDeclaringType");

    /* Initialize the variables	*/
    obj = NULL;
    clrType = NULL;
    type = NULL;
    ilType = NULL;
    string = NULL;

    /*Fetch the clrType	*/
    clrType = ((ildjitSystem->cliManager).CLR.reflectManager).getPrivateDataFromType(_this);
    assert(clrType != NULL);
    assert(clrType->ID != NULL);

    type = clrType->ID;
    assert(type != NULL);

    if (type->attributes->isNested) {
        ilType = type->getEnclosing(type);
        assert(ilType != NULL);

        obj = ((ildjitSystem->cliManager).CLR.reflectManager).getType(ilType);
        assert(obj != NULL);

        /* Build full name	*/
        string = GetTypeName(obj, 0);
    }

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Reflection.ClrType.GetClrNestedDeclaringType");
    return string;
}

void * System_Reflection_ClrType_GetClrGUID (void *_this) {
    void            *obj;
    CLRType         *clrType;
    TypeDescriptor          *ilType;

    assert(_this != NULL);
    METHOD_BEGIN(ildjitSystem, "System.Reflection.ClrType.GetClrGUID");

    /* Initialize the variables	*/
    obj = NULL;
    clrType = NULL;

    /* Fetch the clrType		*/
    clrType = ((ildjitSystem->cliManager).CLR.reflectManager).getPrivateDataFromType(_this);
    assert(clrType != NULL);
    assert(clrType->ID != NULL);

    ilType = (ildjitSystem->cliManager).metadataManager->getTypeDescriptorFromName((ildjitSystem->cliManager).metadataManager, (JITINT8 *) "System", (JITINT8 *) "Guid");
    assert(ilType != NULL);

    obj = (ildjitSystem->garbage_collectors).gc.allocObject(ilType, 0);
    assert(obj != NULL);

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Reflection.ClrType.GetClrGUID");
    return obj;
}

JITBOOLEAN System_Reflection_ClrType_IsClrNestedType (void *_this) {
    CLRType                 *clrType;
    TypeDescriptor  *type;

    assert(_this != NULL);
    METHOD_BEGIN(ildjitSystem, "System.Reflection.ClrType.IsClrNestedType");

    /* Initialize the variables	*/
    clrType = NULL;
    type = NULL;

    /* Fetch the CLRType		*/
    clrType = ((ildjitSystem->cliManager).CLR.reflectManager).getPrivateDataFromType(_this);
    assert(clrType != NULL);
    assert(clrType->ID != NULL);
    type = clrType->ID;

    METHOD_END(ildjitSystem, "System.Reflection.ClrType.IsClrNestedType");
    return type->attributes->isNested;
}

void * System_Reflection_ClrType_GetClrBaseType (void *_this) {
    void                    *obj;
    CLRType                 *clrType;
    TypeDescriptor  *type;
    TypeDescriptor  *parent_type;
    void                    *string;

    assert(_this != NULL);
    METHOD_BEGIN(ildjitSystem, "System.Reflection.ClrType.GetClrBaseType");

    /*Initialize the variables	*/
    obj = NULL;
    clrType = NULL;
    type = NULL;
    parent_type = NULL;
    string = NULL;

    /* Fetch the CLRType		*/
    clrType = ((ildjitSystem->cliManager).CLR.reflectManager).getPrivateDataFromType(_this);
    assert(clrType != NULL);
    assert(clrType->ID != NULL);

    type = clrType->ID;
    assert(type != NULL);

    parent_type = type->getSuperType(type);
    if (parent_type != NULL) {
        obj = ((ildjitSystem->cliManager).CLR.reflectManager).getType(parent_type);
    }

    /* Add the namespace to the type	*/
    string = GetTypeName(obj, 1);

    METHOD_END(ildjitSystem, "System.Reflection.ClrType.GetClrBaseType");
    return string;
}

void * System_Reflection_ClrType_GetClrAssembly (void *_this) {
    void                    *obj;
    CLRType                 *clrType;
    BasicAssembly   *assembly;

    assert(_this != NULL);
    METHOD_BEGIN(ildjitSystem, "System.Reflection.ClrType.GetClrAssembly");

    /*Initialize the variables	*/
    clrType = NULL;
    obj = NULL;

    /* Fetch the CLRType		*/
    clrType = ((ildjitSystem->cliManager).CLR.reflectManager).getPrivateDataFromType(_this);
    assert(clrType != NULL);
    assert(clrType->ID != NULL);

    assembly = clrType->ID->getAssembly(clrType->ID);
    assert(assembly != NULL);

    obj = ((ildjitSystem->cliManager).CLR.reflectManager).getAssembly(assembly);
    assert(obj != NULL);

    METHOD_END(ildjitSystem, "System.Reflection.ClrType.GetClrAssembly");
    return obj;
}

JITINT32 System_Reflection_ClrType_GetAttributeFlagsImpl (void *_this) {
    JITINT32 value;
    CLRType                 *clrType;

    assert(_this != NULL);
    METHOD_BEGIN(ildjitSystem, "System.Reflection.ClrType.GetAttributeFlagsImpl");

    /* Initialize the variables	*/
    clrType = NULL;
    value = -1;

    clrType = ((ildjitSystem->cliManager).CLR.reflectManager).getPrivateDataFromType(_this);
    assert(clrType != NULL);
    assert(clrType->ID != NULL);

    value = clrType->ID->attributes->flags;
    assert(value >= 0);

    METHOD_END(ildjitSystem, "System.Reflection.ClrType.GetAttributeFlagsImpl");
    return value;
}

JITINT32 System_Reflection_ClrType_GetClrArrayRank (void *_this) {
    JITINT32 rank;
    CLRType         *clrType;
    ArrayDescriptor         *array;

    assert(_this != NULL);
    METHOD_BEGIN(ildjitSystem, "System.Reflection.ClrType.GetClrArrayRank");

    /*Initialize the variables	*/
    clrType = NULL;
    rank = -1;
    array = NULL;

    /* Fetch the CLRType		*/
    clrType = ((ildjitSystem->cliManager).CLR.reflectManager).getPrivateDataFromType(_this);
    assert(clrType != NULL);
    assert(clrType->ID != NULL);

    array = GET_ARRAY(clrType->ID);
    assert(array != NULL);

    rank = array->rank;
    assert(rank >= 0);

    METHOD_END(ildjitSystem, "System.Reflection.ClrType.GetClrArrayRank");
    return rank;
}

void * System_Reflection_ClrType_GetInterfaces (void *object) {
    void                            *obj;
    CLRType                         *clrType;
    void                            *interface_type;
    TypeDescriptor                  *type;
    XanList                         *interfaces;
    XanListItem                     *item;
    JITINT32 i;

    assert(object != NULL);
    METHOD_BEGIN(ildjitSystem, "System.Reflection.ClrType.GetInterfaces");

    /*Initialize the variables	*/
    item = NULL;
    obj = NULL;
    interfaces = NULL;
    interface_type = NULL;

    /*Fetch the ClrType			*/
    clrType = ((ildjitSystem->cliManager).CLR.reflectManager).getPrivateDataFromType(object);
    assert(clrType != NULL);
    assert(clrType->ID != NULL);
    type = GC_getObjectType(object);
    assert(type != NULL);

    interfaces = type->getImplementedInterfaces(type);
    assert(interfaces != NULL);
    assert(interfaces->len >= 0);

    TypeDescriptor *objectType = (ildjitSystem->cliManager).metadataManager->getTypeDescriptorFromElementType((ildjitSystem->cliManager).metadataManager, ELEMENT_TYPE_OBJECT);
    obj = (ildjitSystem->garbage_collectors).gc.allocArray(objectType, 1, interfaces->len);
    assert(obj != NULL);

    if (interfaces->len > 0) {
        item = xanList_first(interfaces);
        assert(item != NULL);
        for (i = 0; i < (interfaces->len); i++) {

            interface_type = CLIMANAGER_getCLRType(item->data);
            assert(interface_type != NULL);

            (ildjitSystem->cliManager).CLR.arrayManager.setValueArrayElement(obj, i, &interface_type);

            item = item->next;
            assert(item != NULL);

        }
    }

    METHOD_END(ildjitSystem, "System.Reflection.ClrType.GetInterfaces");
    return obj;
}

void * System_Reflection_ClrType_GetMembersImpl (void *_this, JITINT32 memberTypes, JITINT32 bindingAttrs, void *passType, void *nameString) {
    JITINT16                *literalUTF16;
    JITINT8                 *literalUTF8;
    JITINT32 stringLength;
    CLRType                 *clrClass;
    TypeDescriptor          *nested_type;
    FieldDescriptor         *field;
    MethodDescriptor        *method;
    PropertyDescriptor      *property;
    EventDescriptor         *event;
    void                    *object;
    JITINT32 arraySize;
    JITINT32 position;
    void                    *array;
    CLRType                 *clrArray;
    TypeDescriptor  *arrayType;
    TypeDescriptor *class;
    JITINT8                 *nameMember;

    METHOD_BEGIN(ildjitSystem, "System.Reflection.ClrType.GetMembersImpl");

    /* Initialize the variables	*/
    object = NULL;
    literalUTF16 = NULL;
    event = NULL;
    property = NULL;
    literalUTF8 = NULL;
    clrClass = NULL;
    method = NULL;
    field = NULL;
    nameMember = NULL;
    stringLength = 0;
    arraySize = 0;
    position = 0;
    clrArray = NULL;
    nested_type = NULL;
    array = NULL;

    /* Assertions			*/
    assert(_this != NULL);

    /* Validate the parameters	*/
    if (passType == NULL) {
        ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "ArgumentNullException");
    }
    if (bindingAttrs == 0) {
        bindingAttrs = (JITINT32) (BF_Public | BF_Instance | BF_Static);
    }
    stringLength = (ildjitSystem->cliManager).CLR.stringManager.getLength(nameString);
    if (stringLength >= 128) {
        bindingAttrs &= ~(JITINT32) BF_IgnoreCase;
    }

    /* Fetch the class stored	*
     * inside the _this and arrayClass object	*/
    clrClass = ((ildjitSystem->cliManager).CLR.reflectManager).getPrivateDataFromType(_this);
    assert(clrClass != NULL);
    assert(clrClass->ID != NULL);
    assert(clrClass->object != NULL);
    class = clrClass->ID;

    clrArray = ((ildjitSystem->cliManager).CLR.reflectManager).getPrivateDataFromType(passType);
    assert(clrArray != NULL);
    assert(clrArray->ID != NULL);
    assert(clrArray->object != NULL);
    arrayType = GET_ARRAY(clrArray->ID)->type;

    /* Print the class		*/
    PDEBUG("Internal calls: System.Reflection.ClrType.GetMembersImpl:       Class	= %s\n", class->getCompleteName(class));


    //FIXME Manca il check sui binding attributes

    /* Check if we are looking for a Field	*/
    if (memberTypes & MEMBER_TYPES_FIELD) {

        if (nameString != NULL) {
            /* Fetch the literal            */
            literalUTF16 = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(nameString);
            assert(literalUTF16 != NULL);
            literalUTF8 = allocMemory(sizeof(JITINT8) * (stringLength + 1));
            assert(literalUTF8 != NULL);
            (ildjitSystem->cliManager).CLR.stringManager.fromUTF16ToUTF8(literalUTF16, literalUTF8, stringLength);
            PDEBUG("Internal calls: System.Reflection.ClrType.GetMembersImpl:       Member	= %s\n", literalUTF8);

            XanList *list = class->getDirectImplementedFields(class);
            XanListItem *item = xanList_first(list);
            /* Iterate through the table to count the arraySize	*/

            /* Search the member inside the	*
             * fields of the class		*/
            while (item != NULL) {
                field = (FieldDescriptor *) item->data;

                /* Fetch the current name		*/
                nameMember = field->getName(field);
                PDEBUG("Internal calls: System.Reflection.ClrType.GetMembersImpl:               Current Field	= %s\n", nameMember);

                /* Check				*/
                if (reflectionStringCmp(literalUTF8, (JITINT8 *) nameMember, bindingAttrs & (JITINT32) BF_IgnoreCase) == 0) {
                    PDEBUG("Internal calls: System.Reflection.ClrType.GetMembersImpl:                       Increment the array size\n");
                    arraySize++;
                }
                /* Fetch the next field			*/
                item = item->next;
            }
            /* Now allocate the array and insert the object */
            array = (ildjitSystem->garbage_collectors).gc.allocArray(arrayType, 1, arraySize);
            assert(array != NULL);

            /* Search the member inside the	*
             * fields of the class		*/
            item = xanList_first(list);
            while (item != NULL) {
                field = (FieldDescriptor *) item->data;

                /* Fetch the current name		*/
                nameMember = field->getName(field);
                assert(nameMember != NULL);
                PDEBUG("Internal calls: System.Reflection.ClrType.GetMembersImpl:               Current Field	= %s\n", nameMember);

                /* Check				*/
                if (reflectionStringCmp(literalUTF8, (JITINT8 *) nameMember, bindingAttrs & (JITINT32) BF_IgnoreCase) == 0) {
                    PDEBUG("Internal calls: System.Reflection.ClrType.GetMembersImpl:                       Found the member\n");
                    object = ((ildjitSystem->cliManager).CLR.reflectManager).getField(field);
                    assert(object != NULL);

                    /* Insert the object in the right position	*/
                    (ildjitSystem->cliManager).CLR.arrayManager.setValueArrayElement(array, position, &object);
                    position++;

                }

                /* Fetch the next field			*/
                item = item->next;
            }

        } else {
            /* Iterate through the table to count the arraySize	*/

            /* Search the member inside the	*
             * fields of the class		*/
            XanList *list = class->getDirectImplementedFields(class);
            arraySize = xanList_length(list);


            /* Now allocate the array and insert the object */
            assert(arraySize >= 0);
            array = (ildjitSystem->garbage_collectors).gc.allocArray(arrayType, 1, arraySize);
            assert(array != NULL);

            /* Search the member inside the	*
             * fields of the class		*/
            XanListItem *item = xanList_first(list);
            while (item != NULL) {
                field = (FieldDescriptor *) item->data;

                PDEBUG("Internal calls: System.Reflection.ClrType.GetMembersImpl:                       Found the member\n");
                object = ((ildjitSystem->cliManager).CLR.reflectManager).getField(field);
                assert(object != NULL);

                /* Insert the object in the right position	*/
                (ildjitSystem->cliManager).CLR.arrayManager.setValueArrayElement(array, position, &object);
                position++;

                /* Fetch the next field			*/
                item = item->next;
            }
        }
    }
    /* Check if we are looking for a Method or a Constructor	*/
    else if ((memberTypes & MEMBER_TYPES_METHOD) || (memberTypes & MEMBER_TYPES_CONSTRUCTOR)) {
        if (nameString != NULL) {
            /* Search the member inside the	*
             * methods of the class		*/
            XanList *list = class->getDirectDeclaredMethods(class);
            XanListItem *item = xanList_first(list);
            while (item != NULL) {
                method = (MethodDescriptor *) item->data;
                /* Fetch the current name		*/
                nameMember = method->getName(method);
                assert(nameMember != NULL);
                PDEBUG("Internal calls: System.Reflection.ClrType.GetMembersImpl:               Current Method	= %s\n", nameMember);

                /* Check				*/
                if (reflectionStringCmp(literalUTF8, (JITINT8 *) nameMember, bindingAttrs & (JITINT32) BF_IgnoreCase) == 0) {
                    PDEBUG("Internal calls: System.Reflection.ClrType.GetMembersImpl:                       Increment the array size\n");
                    arraySize++;
                }

                /* Fetch the next field			*/
                item = item->next;
            }

            /* Now allocate the array and insert the object */
            array = (ildjitSystem->garbage_collectors).gc.allocArray(arrayType, 1, arraySize);
            assert(array != NULL);

            /* Search the member inside the	*
             * methods of the class		*/
            item = xanList_first(list);
            while (item != NULL) {
                method = (MethodDescriptor *) item->data;

                /* Fetch the current name		*/
                nameMember = method->getName(method);
                assert(nameMember != NULL);
                PDEBUG("Internal calls: System.Reflection.ClrType.GetMembersImpl:               Current Method	= %s\n", nameMember);

                /* Check				*/
                if (reflectionStringCmp(literalUTF8, (JITINT8 *) nameMember, bindingAttrs & (JITINT32) BF_IgnoreCase) == 0) {
                    PDEBUG("Internal calls: System.Reflection.ClrType.GetMembersImpl:                       Found the member\n");

                    /* Check if the current method is a constructor	*/
                    if (method->isCtor(method)) {
                        object = ((ildjitSystem->cliManager).CLR.reflectManager).getConstructor(method);
                    } else {
                        object = ((ildjitSystem->cliManager).CLR.reflectManager).getMethod(method);
                    }
                    assert(object != NULL);

                    /* Insert the object in the right position	*/
                    (ildjitSystem->cliManager).CLR.arrayManager.setValueArrayElement(array, position, &object);
                    position++;

                }

                /* Fetch the next field			*/
                item = item->next;
            }

        } else {
            /* Search the member inside the	*
             * methods of the class		*/
            XanList *list = class->getDirectDeclaredMethods(class);
            arraySize = xanList_length(list);

            /* Now allocate the array and insert the object */
            array = (ildjitSystem->garbage_collectors).gc.allocArray(arrayType, 1, arraySize);
            assert(array != NULL);

            /* Search the member inside the	*
             * methods of the class		*/
            XanListItem *item = xanList_first(list);
            while (item != NULL) {
                method = (MethodDescriptor *) item->data;

                PDEBUG("Internal calls: System.Reflection.ClrType.GetMembersImpl:                       Found the member\n");

                /* Check if the current method is a constructor	*/
                if (method->isCtor(method)) {
                    object = ((ildjitSystem->cliManager).CLR.reflectManager).getConstructor(method);
                } else {
                    object = ((ildjitSystem->cliManager).CLR.reflectManager).getMethod(method);
                }
                assert(object != NULL);

                /* Insert the object in the right position	*/
                (ildjitSystem->cliManager).CLR.arrayManager.setValueArrayElement(array, position, &object);
                position++;

                /* Fetch the next field			*/
                item = item->next;
            }
        }
    }
    /* Check if we are looking for a Property	*/
    else if (memberTypes & MEMBER_TYPES_PROPERTY) {
        if (nameString != NULL) {
            /* Search the member inside the	*
             * propertys of the class		*/
            XanList *list = class->getProperties(class);
            XanListItem *item = xanList_first(list);
            while (item != NULL) {
                property = (PropertyDescriptor *) item->data;
                /* Fetch the current name		*/
                nameMember = property->getName(property);
                assert(nameMember != NULL);
                PDEBUG("Internal calls: System.Reflection.ClrType.GetMembersImpl:               Current Property	= %s\n", nameMember);

                /* Check				*/
                if (reflectionStringCmp(literalUTF8, (JITINT8 *) nameMember, bindingAttrs & (JITINT32) BF_IgnoreCase) == 0) {
                    PDEBUG("Internal calls: System.Reflection.ClrType.GetMembersImpl:                       Increment the array size\n");
                    arraySize++;
                }

                /* Fetch the next field			*/
                item = item->next;
            }

            /* Now allocate the array and insert the object */
            array = (ildjitSystem->garbage_collectors).gc.allocArray(arrayType, 1, arraySize);
            assert(array != NULL);

            /* Search the member inside the	*
             * propertys of the class		*/
            item = xanList_first(list);
            while (item != NULL) {
                property = (PropertyDescriptor *) item->data;

                /* Fetch the current name		*/
                nameMember = property->getName(property);
                assert(nameMember != NULL);
                PDEBUG("Internal calls: System.Reflection.ClrType.GetMembersImpl:               Current Property	= %s\n", nameMember);

                /* Check				*/
                if (reflectionStringCmp(literalUTF8, (JITINT8 *) nameMember, bindingAttrs & (JITINT32) BF_IgnoreCase) == 0) {
                    PDEBUG("Internal calls: System.Reflection.ClrType.GetMembersImpl:                       Found the member\n");

                    object = ((ildjitSystem->cliManager).CLR.reflectManager).getProperty(property);
                    assert(object != NULL);

                    /* Insert the object in the right position	*/
                    (ildjitSystem->cliManager).CLR.arrayManager.setValueArrayElement(array, position, &object);
                    position++;

                }

                /* Fetch the next field			*/
                item = item->next;
            }

        } else {
            /* Search the member inside the	*
             * propertys of the class		*/
            XanList *list = class->getProperties(class);
            arraySize = xanList_length(list);

            /* Now allocate the array and insert the object */
            array = (ildjitSystem->garbage_collectors).gc.allocArray(arrayType, 1, arraySize);
            assert(array != NULL);

            /* Search the member inside the	*
             * propertys of the class		*/
            XanListItem *item = xanList_first(list);
            while (item != NULL) {
                property = (PropertyDescriptor *) item->data;

                PDEBUG("Internal calls: System.Reflection.ClrType.GetMembersImpl:                       Found the member\n");
                object = ((ildjitSystem->cliManager).CLR.reflectManager).getProperty(property);
                assert(object != NULL);

                /* Insert the object in the right position	*/
                (ildjitSystem->cliManager).CLR.arrayManager.setValueArrayElement(array, position, &object);
                position++;

                /* Fetch the next field			*/
                item = item->next;
            }
        }
    }
    /* Check if we are looking for a Event	*/
    else if (memberTypes & MEMBER_TYPES_EVENT) {
        if (nameString != NULL) {
            /* Search the member inside the	*
             * events of the class		*/
            XanList *list = class->getEvents(class);
            XanListItem *item = xanList_first(list);
            while (item != NULL) {
                event = (EventDescriptor *) item->data;
                /* Fetch the current name		*/
                nameMember = event->getName(event);
                assert(nameMember != NULL);
                PDEBUG("Internal calls: System.Reflection.ClrType.GetMembersImpl:               Current Event	= %s\n", nameMember);

                /* Check				*/
                if (reflectionStringCmp(literalUTF8, (JITINT8 *) nameMember, bindingAttrs & (JITINT32) BF_IgnoreCase) == 0) {
                    PDEBUG("Internal calls: System.Reflection.ClrType.GetMembersImpl:                       Increment the array size\n");
                    arraySize++;
                }

                /* Fetch the next field			*/
                item = item->next;
            }

            /* Now allocate the array and insert the object */
            array = (ildjitSystem->garbage_collectors).gc.allocArray(arrayType, 1, arraySize);
            assert(array != NULL);

            /* Search the member inside the	*
             * events of the class		*/
            item = xanList_first(list);
            while (item != NULL) {
                event = (EventDescriptor *) item->data;

                /* Fetch the current name		*/
                nameMember = event->getName(event);
                assert(nameMember != NULL);
                PDEBUG("Internal calls: System.Reflection.ClrType.GetMembersImpl:               Current Event	= %s\n", nameMember);

                /* Check				*/
                if (reflectionStringCmp(literalUTF8, (JITINT8 *) nameMember, bindingAttrs & (JITINT32) BF_IgnoreCase) == 0) {
                    PDEBUG("Internal calls: System.Reflection.ClrType.GetMembersImpl:                       Found the member\n");

                    object = ((ildjitSystem->cliManager).CLR.reflectManager).getEvent(event);
                    assert(object != NULL);

                    /* Insert the object in the right position	*/
                    (ildjitSystem->cliManager).CLR.arrayManager.setValueArrayElement(array, position, &object);
                    position++;

                }

                /* Fetch the next field			*/
                item = item->next;
            }

        } else {
            /* Search the member inside the	*
             * events of the class		*/
            XanList *list = class->getEvents(class);
            arraySize = xanList_length(list);

            /* Now allocate the array and insert the object */
            array = (ildjitSystem->garbage_collectors).gc.allocArray(arrayType, 1, arraySize);
            assert(array != NULL);

            /* Search the member inside the	*
             * events of the class		*/
            XanListItem *item = xanList_first(list);
            while (item != NULL) {
                event = (EventDescriptor *) item->data;

                PDEBUG("Internal calls: System.Reflection.ClrType.GetMembersImpl:                       Found the member\n");
                object = ((ildjitSystem->cliManager).CLR.reflectManager).getEvent(event);
                assert(object != NULL);

                /* Insert the object in the right position	*/
                (ildjitSystem->cliManager).CLR.arrayManager.setValueArrayElement(array, position, &object);
                position++;

                /* Fetch the next field			*/
                item = item->next;
            }
        }
    } else if (memberTypes & MEMBER_TYPES_NESTEDTYPE) {
        if (nameString != NULL) {
            /* Search the member inside the	*
             * nested_types of the type		*/
            XanList *list = class->getNestedType(class);
            XanListItem *item = xanList_first(list);
            while (item != NULL) {
                nested_type = (TypeDescriptor *) item->data;
                /* Fetch the current name		*/
                nameMember = nested_type->getName(nested_type);
                assert(nameMember != NULL);
                PDEBUG("Internal calls: System.Reflection.ClrType.GetMembersImpl:               Current Class	= %s\n", nameMember);

                /* Check				*/
                if (reflectionStringCmp(literalUTF8, (JITINT8 *) nameMember, bindingAttrs & (JITINT32) BF_IgnoreCase) == 0) {
                    PDEBUG("Internal calls: System.Reflection.ClrType.GetMembersImpl:                       Increment the array size\n");
                    arraySize++;
                }

                /* Fetch the next field			*/
                item = item->next;
            }

            /* Now allocate the array and insert the object */
            array = (ildjitSystem->garbage_collectors).gc.allocArray(arrayType, 1, arraySize);
            assert(array != NULL);

            /* Search the member inside the	*
             * nested_types of the class		*/
            item = xanList_first(list);
            while (item != NULL) {
                nested_type = (TypeDescriptor *) item->data;

                /* Fetch the current name		*/
                nameMember = nested_type->getName(nested_type);
                assert(nameMember != NULL);
                PDEBUG("Internal calls: System.Reflection.ClrType.GetMembersImpl:               Current Class	= %s\n", nameMember);

                /* Check				*/
                if (reflectionStringCmp(literalUTF8, (JITINT8 *) nameMember, bindingAttrs & (JITINT32) BF_IgnoreCase) == 0) {
                    PDEBUG("Internal calls: System.Reflection.ClrType.GetMembersImpl:                       Found the member\n");

                    object = ((ildjitSystem->cliManager).CLR.reflectManager).getType(nested_type);
                    assert(object != NULL);

                    /* Insert the object in the right position	*/
                    (ildjitSystem->cliManager).CLR.arrayManager.setValueArrayElement(array, position, &object);
                    position++;

                }

                /* Fetch the next field			*/
                item = item->next;
            }

        } else {
            /* Search the member inside the	*
             * nested_types of the class		*/
            XanList *list = class->getNestedType(class);
            arraySize = xanList_length(list);

            /* Now allocate the array and insert the object */
            array = (ildjitSystem->garbage_collectors).gc.allocArray(arrayType, 1, arraySize);
            assert(array != NULL);

            /* Search the member inside the	*
             * nested_types of the class		*/
            XanListItem *item = xanList_first(list);
            while (item != NULL) {
                nested_type = (TypeDescriptor *) item->data;

                PDEBUG("Internal calls: System.Reflection.ClrType.GetMembersImpl:                       Found the member\n");
                object = ((ildjitSystem->cliManager).CLR.reflectManager).getType(nested_type);
                assert(object != NULL);

                /* Insert the object in the right position	*/
                (ildjitSystem->cliManager).CLR.arrayManager.setValueArrayElement(array, position, &object);
                position++;

                /* Fetch the next field			*/
                item = item->next;
            }
        }
    }

    /* Final assertion	*/
    assert(arraySize == position);

    /* Exit				*/
    METHOD_END(ildjitSystem, "System.Reflection.ClrType.GetMembersImpl");
    return array;
}

void * System_Reflection_ClrType_GetMemberImpl (void *_this, void *name, JITINT32 memberTypes, JITINT32 bindingAttrs, void *binder, JITINT32 callConv, void *typesArray, void *modifiersArray) {
    JITINT16                *literalUTF16;
    JITINT8                 *literalUTF8;
    JITINT32 stringLength;
    CLRType                 *clrClass;
    FieldDescriptor         *field;
    MethodDescriptor        *method;
    PropertyDescriptor      *property;
    EventDescriptor         *event;
    JITINT8                 *nameMember;
    void                    *object;
    TypeDescriptor *class;

    /* Assertions			*/
    assert(_this != NULL);

    METHOD_BEGIN(ildjitSystem, "System.Reflection.ClrType.GetMemberImpl");

    /* Initialize the variables	*/
    object = NULL;
    literalUTF16 = NULL;
    event = NULL;
    property = NULL;
    literalUTF8 = NULL;
    clrClass = NULL;
    method = NULL;
    field = NULL;
    nameMember = NULL;
    stringLength = 0;

    /* Validate the parameters	*/
    if (name == NULL) {
        ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "ArgumentNullException");
    }
    if (bindingAttrs == 0) {
        bindingAttrs = (JITINT32) (BF_Public | BF_Instance | BF_Static);
    }
    stringLength = (ildjitSystem->cliManager).CLR.stringManager.getLength(name);
    if (stringLength >= 128) {
        bindingAttrs &= ~(JITINT32) BF_IgnoreCase;
    }

    /* Fetch the class stored	*
     * inside the _this object	*/
    clrClass = ((ildjitSystem->cliManager).CLR.reflectManager).getPrivateDataFromType(_this);
    assert(clrClass != NULL);
    assert(clrClass->ID != NULL);
    assert(clrClass->object != NULL);
    class = clrClass->ID;

    /* Print the class		*/
#ifdef DEBUG
    PDEBUG("Internal calls: System.Reflection.ClrType.GetMemberImpl:        Class	= %s\n", class->getCompleteName(class));
#endif

    /* Fetch the literal            */
    literalUTF16 = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(name);
    assert(literalUTF16 != NULL);
    literalUTF8 = allocMemory(sizeof(JITINT8) * (stringLength + 1));
    assert(literalUTF8 != NULL);
    (ildjitSystem->cliManager).CLR.stringManager.fromUTF16ToUTF8(literalUTF16, literalUTF8, stringLength);
    PDEBUG("Internal calls: System.Reflection.ClrType.GetMemberImpl:        Member	= %s\n", literalUTF8);

    if (memberTypes & MEMBER_TYPES_FIELD) {
        /* Search the member inside the	*

         * fields of the class		*/
        XanList *list = class->getDirectImplementedFields(class);
        XanListItem *item = xanList_first(list);
        while (item != NULL) {
            field = (FieldDescriptor *) item->data;
            /* Fetch the current name		*/
            nameMember = field->getName(field);
            assert(nameMember != NULL);
            PDEBUG("Internal calls: System.Reflection.ClrType.GetMemberImpl:                Current Field	= %s\n", nameMember);

            /* Check				*/
            if (reflectionStringCmp(literalUTF8, (JITINT8 *) nameMember, bindingAttrs & (JITINT32) BF_IgnoreCase) == 0) {
                PDEBUG("Internal calls: System.Reflection.ClrType.GetMemberImpl:                        Found the member\n");
                object = ((ildjitSystem->cliManager).CLR.reflectManager).getField(field);
                assert(object != NULL);
                break;
            }

            /* Fetch the next field			*/
            item = item->next;
        }

    } else if ((memberTypes & MEMBER_TYPES_METHOD) || (memberTypes & MEMBER_TYPES_CONSTRUCTOR)) {
        /* Search the member inside the	*

         * methods of the class		*/
        XanList *list = class->getDirectDeclaredMethods(class);
        XanListItem *item = xanList_first(list);
        while (item != NULL) {
            method = (MethodDescriptor *) item->data;
            /* Fetch the current name		*/
            nameMember = method->getName(method);
            assert(nameMember != NULL);
            PDEBUG("Internal calls: System.Reflection.ClrType.GetMemberImpl:                Current Method	= %s\n", nameMember);

            /* Check				*/
            if (reflectionStringCmp(literalUTF8, (JITINT8 *) nameMember, bindingAttrs & (JITINT32) BF_IgnoreCase) == 0) {
                /* Check if the current method is a constructor	*/
                if (method->isCtor(method)) {
                    object = ((ildjitSystem->cliManager).CLR.reflectManager).getConstructor(method);
                    assert(object != NULL);
                } else {
                    object = ((ildjitSystem->cliManager).CLR.reflectManager).getMethod(method);
                    assert(object != NULL);
                }
                break;
            }

            /* Fetch the next method			*/
            item = item->next;
        }
    } else if (memberTypes & MEMBER_TYPES_PROPERTY) {

        /* Search the member inside the	*

         * propertys of the class		*/
        XanList *list = class->getProperties(class);
        XanListItem *item = xanList_first(list);
        while (item != NULL) {
            property = (PropertyDescriptor *) item->data;
            /* Fetch the current name		*/
            nameMember = property->getName(property);
            assert(nameMember != NULL);
            PDEBUG("Internal calls: System.Reflection.ClrType.GetMemberImpl:                Current Property	= %s\n", nameMember);

            /* Check				*/
            if (reflectionStringCmp(literalUTF8, (JITINT8 *) nameMember, bindingAttrs & (JITINT32) BF_IgnoreCase) == 0) {
                PDEBUG("Internal calls: System.Reflection.ClrType.GetMemberImpl:                        Found the member\n");
                object = ((ildjitSystem->cliManager).CLR.reflectManager).getProperty(property);
                assert(object != NULL);
                break;
            }

            /* Fetch the next property			*/
            item = item->next;
        }
    } else if (memberTypes & MEMBER_TYPES_EVENT) {

        /* Search the member inside the	*

         * events of the class		*/
        XanList *list = class->getEvents(class);
        XanListItem *item = xanList_first(list);
        while (item != NULL) {
            event = (EventDescriptor *) item->data;
            /* Fetch the current name		*/
            nameMember = event->getName(event);
            assert(nameMember != NULL);
            PDEBUG("Internal calls: System.Reflection.ClrType.GetMemberImpl:                Current Event	= %s\n", nameMember);

            /* Check				*/
            if (reflectionStringCmp(literalUTF8, (JITINT8 *) nameMember, bindingAttrs & (JITINT32) BF_IgnoreCase) == 0) {
                PDEBUG("Internal calls: System.Reflection.ClrType.GetMemberImpl:                        Found the member\n");
                object = ((ildjitSystem->cliManager).CLR.reflectManager).getEvent(event);
                assert(object != NULL);
                break;
            }

            /* Fetch the next event			*/
            item = item->next;
        }
    }
    /* Return			*/
    METHOD_END(ildjitSystem, "System.Reflection.ClrType.GetMemberImpl");
    return object;
}

void * System_Reflection_ClrType_GetClrFullName (void *object) { //già fatto
    void            *name;

    /* Assertions			*/
    assert(object != NULL);
    METHOD_BEGIN(ildjitSystem, "System.Reflection.ClrType.GetClrFullName");

    /* Fetch the string		*/
    name = GetTypeName(object, 1);
    assert(name != NULL);

    /* Exit				*/
    METHOD_END(ildjitSystem, "System.Reflection.ClrType.GetClrFullName");
    return name;
}

void * System_Reflection_ClrType_GetClrNamespace (void *object) { //già fatto
    void            *string;

    /* Assertions				*/
    assert(object != NULL);
    METHOD_BEGIN(ildjitSystem, "System.Reflection.ClrType.GetClrNamespace");

    /* Fetch the name			*/
    string = GetTypeNamespace(object);
    assert(string != NULL);

    /* Return				*/
    METHOD_END(ildjitSystem, "System.Reflection.ClrType.GetClrNamespace");
    return string;
}

void * System_Reflection_ClrType_GetClrName (void *object) {
    void            *string;

    /* Assertions				*/
    assert(object != NULL);
    METHOD_BEGIN(ildjitSystem, "System.Reflection.ClrType.GetClrName");

    /* Fetch the name			*/
    string = GetTypeName(object, 0);
    assert(string != NULL);

    /* Return				*/
    METHOD_END(ildjitSystem, "System.Reflection.ClrType.GetClrName");
    return string;
}

void * System_Reflection_ClrType_GetElementType (void *object) {
    CLRType         *class;
    void *obj;
    TypeDescriptor *elementType;

    /* Assertions			*/
    assert(object != NULL);
    METHOD_BEGIN(ildjitSystem, "System.Reflection.ClrType.GetElementType");

    /* Initialize the variables	*/
    class = NULL;
    elementType = NULL;
    obj = NULL;
    class = ((ildjitSystem->cliManager).CLR.reflectManager).getPrivateDataFromType(object);
    assert(class != NULL);
    assert(class->ID != NULL);
    assert(class->object != NULL);

    if (class->ID->isByRef) {
        elementType = (TypeDescriptor *) class->ID->descriptor;
    } else {
        switch (class->ID->element_type) {
            case ELEMENT_ARRAY:
                elementType = GET_ARRAY(class->ID)->type;
                break;
            case ELEMENT_PTR:
                elementType = GET_PTR(class->ID)->type;
                break;
            default:
                elementType = NULL;
        }
    }
    if (elementType != NULL) {
        obj = ((ildjitSystem->cliManager).CLR.reflectManager).getType(elementType);
    }
    /* Return the System.Type of the object	*/
    METHOD_END(ildjitSystem, "System.Reflection.ClrType.GetElementType");
    return obj;
}

JITBOOLEAN System_Reflection_ClrType_IsSubclassOf (void *object, void *otherObject) {
    TypeDescriptor                  *objectType;
    TypeDescriptor                  *otherObjectType;
    CLRType                 *objectCLRType;
    CLRType                 *otherObjectCLRType;
    JITBOOLEAN result;

    /* Assertions				*/
    assert(object != NULL);
    assert(otherObject != NULL);
    METHOD_BEGIN(ildjitSystem, "System.Reflection.ClrType.IsSubclassOf");


    /* Fetch the type of the object		*/
    objectCLRType = ((ildjitSystem->cliManager).CLR.reflectManager).getPrivateDataFromType(object);
    assert(objectCLRType != NULL);
    assert(objectCLRType->ID != NULL);
    objectType = objectCLRType->ID;

    /* Fetch the type of the other object	*/
    otherObjectCLRType = ((ildjitSystem->cliManager).CLR.reflectManager).getPrivateDataFromType(otherObject);
    assert(otherObjectCLRType != NULL);
    assert(otherObjectCLRType->ID != NULL);
    otherObjectType = otherObjectCLRType->ID;

    /* Print the object type		*/
#ifdef DEBUG
    PDEBUG("Internal calls: System.Reflection.ClrType.IsSubclassOf: Object type		= %s\n", objectType->getCompleteName(objectType));
    PDEBUG("Internal calls: System.Reflection.ClrType.IsSubclassOf: Other object type	= %s\n", otherObjectType->getCompleteName(otherObjectType));
#endif

    /* Check if it is a subtype             */
    result = JITFALSE;
    if (objectType != otherObjectType) {
        result = objectType->isSubtypeOf(objectType, otherObjectType);
    }

    /* Return				*/
    METHOD_END(ildjitSystem, "System.Reflection.ClrType.IsSubclassOf");
    return result;
}

//***************************************************************************************************************/
//*							CLR_HELPERS						*/
//***************************************************************************************************************/
void * System_Reflection_ClrHelpers_GetCustomAttributes (JITNINT item, JITNINT type, JITBOOLEAN inherit) {
    METHOD_BEGIN(ildjitSystem, "System.Reflection.ClrHelpers.GetCustomAttributes");

    METHOD_END(ildjitSystem, "System.Reflection.ClrHelpers.GetCustomAttributes");
    return NULL;
}

void * System_Reflection_ClrHelpers_GetDeclaringType (JITNINT item) {
    CLRMember               *clrMember;
    MethodDescriptor        *t_method;
    EventDescriptor 	*t_event;
    FieldDescriptor 	*t_field;
    PropertyDescriptor      *t_property;
    TypeDescriptor 		*owner;
    void 			*reflectionObject;

    METHOD_BEGIN(ildjitSystem, "System.Reflection.ClrHelpers.GetDeclaringType");

    /*Initialize the variables	*/
    clrMember = NULL;
    t_method = NULL;
    t_event = NULL;
    t_field = NULL;
    t_property = NULL;
    owner = NULL;

    /* Fetch the CLRMember          */
    clrMember = (CLRMember *) item;

    switch (clrMember->type) {
        case CLRCONSTRUCTOR:
        case CLRMETHOD:
            /*cast to right t_row	*/
            t_method = (MethodDescriptor *) clrMember->ID;
            assert(t_method != NULL);
            owner = t_method->owner;
            assert(owner != NULL);
            break;
        case CLREVENT:
            /*cast to right t_row	*/
            t_event = (EventDescriptor *) clrMember->ID;
            assert(t_event != NULL);
            owner = t_event->owner;
            assert(owner != NULL);
            break;
        case CLRFIELD:
            /*cast to right t_row	*/
            t_field = (FieldDescriptor *) clrMember->ID;
            assert(t_field != NULL);

            owner = t_field->owner;
            assert(owner != NULL);
            break;
        case CLRPROPERTY:
            /*cast to right t_row	*/
            t_property = (PropertyDescriptor *) clrMember->ID;
            assert(t_property != NULL);

            owner = t_property->owner;
            assert(owner != NULL);
            break;
        default:
            print_err("bad Clr types as input ", 0);
            abort();
    }

    /* Fetch the reflection object.
     */
    reflectionObject = ((ildjitSystem->cliManager).CLR.reflectManager).getType(owner);
    CLRType *clrType =((ildjitSystem->cliManager).CLR.reflectManager).getPrivateDataFromType(reflectionObject);

    METHOD_END(ildjitSystem, "System.Reflection.ClrHelpers.GetDeclaringType");
    return clrType;
}

void * System_Reflection_ClrHelpers_GetName (JITNINT item) {
    CLRMember               * clrMember;
    BasicAssembly   * t_assembly;
    MethodDescriptor        * t_method;
    EventDescriptor * t_event;
    FieldDescriptor * t_field;
    PropertyDescriptor      * t_property;
    TypeDescriptor  * t_type;
    BasicModule     * t_module;
    void                    * name;
    JITINT8                 * literal;
    t_binary_information *binary;
    t_string_stream         *stringStream;

    METHOD_BEGIN(ildjitSystem, "System.Reflection.ClrHelpers.GetName");

    /*Initialize the variables	*/
    clrMember = NULL;
    t_assembly = NULL;
    t_method = NULL;
    t_event = NULL;
    t_field = NULL;
    t_property = NULL;
    t_type = NULL;
    t_module = NULL;
    name = NULL;
    stringStream = NULL;
    literal = NULL;

    /* Fetch the CLRMember          */
    clrMember = (CLRMember *) item;



    switch (clrMember->type) {
        case CLRASSEMBLY:
            /*cast to right t_row	*/
        {
            t_assembly = (BasicAssembly *) clrMember->ID;
            assert(t_assembly != NULL);
            binary = (t_binary_information *) t_assembly->binary;
            stringStream = &((binary->metadata).streams_metadata.string_stream);
            assert(stringStream != NULL);
            /*fetch the literal	*/
            literal = get_string(stringStream, t_assembly->name);
            assert(literal != NULL);
            break;
        }
        case CLRCONSTRUCTOR:
        case CLRMETHOD:
            /*cast to right t_row	*/
            t_method = (MethodDescriptor *) clrMember->ID;
            assert(t_method != NULL);

            /*fetch the literal	*/
            literal = t_method->getName(t_method);
            assert(literal != NULL);
            break;
        case CLREVENT:
            /*cast to right t_row	*/
            t_event = (EventDescriptor *) clrMember->ID;
            assert(t_event != NULL);

            /*fetch the literal	*/
            literal = t_event->getName(t_event);
            assert(literal != NULL);
            break;
        case CLRFIELD:
            /*cast to right t_row	*/
            t_field = (FieldDescriptor *) clrMember->ID;
            assert(t_field != NULL);

            /*fetch the literal	*/
            literal = t_field->getName(t_field);
            assert(literal != NULL);

            break;
        case CLRPROPERTY:
            /*cast to right t_row	*/
            t_property = (PropertyDescriptor *) clrMember->ID;
            assert(t_property != NULL);

            /*fetch the literal	*/
            literal = t_property->getName(t_property);
            assert(literal != NULL);

            break;
        case CLRTYPE:
            /*cast to right t_row	*/
            t_type = (TypeDescriptor *) clrMember->ID;
            assert(t_type != NULL);

            /*fetch the literal	*/
            literal = t_type->getName(t_type);
            assert(literal != NULL);

            break;
        case CLRMODULE:
            /* Fetch the string stream	*/
            t_module = (BasicModule *) clrMember->ID;
            assert(t_module != NULL);
            binary = (t_binary_information *) t_module->binary;
            stringStream = &((binary->metadata).streams_metadata.string_stream);
            assert(stringStream != NULL);

            /*fetch the literal	*/
            literal = get_string(stringStream, t_module->name);
            assert(literal != NULL);

            break;
        default:
            print_err("bad Clr types as input ", 0);
            abort();
    }
    /*alloc return string   */
    name = CLIMANAGER_StringManager_newInstanceFromUTF8((JITINT8 *) literal, STRLEN(literal));
    assert(name != NULL);

    METHOD_END(ildjitSystem, "System.Reflection.ClrHelpers.GetName");
    return name;
}

void * System_Reflection_ClrHelpers_GetParameterType (JITNINT item, JITINT32 num) {
    CLRMember               * clrMember;
    MethodDescriptor        * method;
    void                    * obj;

    METHOD_BEGIN(ildjitSystem, "System.Reflection.ClrHelpers.GetParameterType");

    /*Initialize the variables	*/
    clrMember = NULL;
    method = NULL;
    obj = NULL;

    /* Fetch the CLRMember          */
    clrMember = (CLRMember *) item;
    assert(clrMember != NULL);

    method = (MethodDescriptor *) clrMember->ID;
    assert(method != NULL);

    ParamDescriptor *param;
    if (num == 0) {
        param = method->getResult(method);
    } else {
        XanList *params = method->getParams(method);
        param = xanList_getElementFromPositionNumber(params, num - 1)->data;
    }

    obj = ((ildjitSystem->cliManager).CLR.reflectManager).getType(param->type);
    assert(obj != NULL);

    METHOD_END(ildjitSystem, "System.Reflection.ClrHelpers.GetParameterType");
    return obj;
}

JITINT32 System_Reflection_ClrHelpers_GetNumParameters (JITNINT item) {
    CLRMember               * clrMember;
    JITINT32 numParam;
    MethodDescriptor        * t_method;

    METHOD_BEGIN(ildjitSystem, "System.Reflection.ClrHelpers.GetNumParameters");

    /* Initialize the variables     */
    clrMember = NULL;
    numParam = -1;

    /* Fetch the CLRMember          */
    clrMember = (CLRMember *) item;

    assert((clrMember->type == CLRMETHOD) || (clrMember->type == CLRCONSTRUCTOR));

    t_method = (MethodDescriptor *) clrMember->ID;
    assert(t_method != NULL);

    XanList* params = t_method->getParams(t_method);
    numParam = xanList_length(params);
    assert(numParam >= 0);

    METHOD_END(ildjitSystem, "System.Reflection.ClrHelpers.GetNumParameters");
    return numParam;
}

JITINT32 System_Reflection_ClrHelpers_GetMemberAttrs (JITNINT item) {
    CLRMember               * clrMember;
    MethodDescriptor        * t_method;
    EventDescriptor * t_event;
    FieldDescriptor * t_field;
    PropertyDescriptor      * t_property;
    JITINT32 attrs;

    /*Initialize the variables	*/
    clrMember = NULL;
    t_method = NULL;
    t_event = NULL;
    t_field = NULL;
    t_property = NULL;

    METHOD_BEGIN(ildjitSystem, "System.Reflection.ClrHelpers.GetMemberAttrs");

    /* Fetch the CLRMember          */
    clrMember = (CLRMember *) item;

    switch (clrMember->type) {
        case CLRCONSTRUCTOR:
        case CLRMETHOD:
            /*cast to right t_row	*/
            t_method = (MethodDescriptor *) clrMember->ID;
            assert(t_method != NULL);

            attrs = (JITINT32) t_method->row->flags;
            break;
        case CLREVENT:
            /*cast to right t_row	*/
            t_event = (EventDescriptor *) clrMember->ID;
            assert(t_event != NULL);

            attrs = (JITINT32) t_event->row->event_flags;
            break;
        case CLRFIELD:
            /*cast to right t_row	*/
            t_field = (FieldDescriptor *) clrMember->ID;
            assert(t_field != NULL);

            attrs = (JITINT32) t_field->row->flags;
            break;
        case CLRPROPERTY:
            /*cast to right t_row	*/
            t_property = (PropertyDescriptor *) clrMember->ID;
            assert(t_property != NULL);

            attrs = (JITINT32) t_property->row->flags;
            break;
        default:
            print_err("bad Clr types as input ", 0);
            abort();
    }

    METHOD_END(ildjitSystem, "System.Reflection.ClrHelpers.GetMemberAttrs");

    return attrs;
}

JITINT32 System_Reflection_ClrHelpers_GetCallConv (JITNINT item) {
    CLRMember               * clrMember;
    MethodDescriptor        * t_method;

    METHOD_BEGIN(ildjitSystem, "System.Reflection.ClrHelpers.GetCallConv");

    /* Initialize the variables     */
    clrMember = NULL;
    t_method = NULL;

    /* Fetch the CLRMember          */
    clrMember = (CLRMember *) item;

    switch (clrMember->type) {
        case CLRMETHOD:
        case CLRCONSTRUCTOR:
            t_method = (MethodDescriptor *) clrMember->ID;
            assert(t_method != NULL);
            break;
        default:
            print_err("bad Clr types as input ", 0);
            abort();
    }

    METHOD_END(ildjitSystem, "System.Reflection.ClrHelpers.GetCallConv");

    return t_method->attributes->calling_convention;
}

JITINT32 System_Reflection_ClrHelpers_GetImplAttrs (JITNINT item) { //OK?
    CLRMember               * clrMember;
    MethodDescriptor        * t_method;
    JITINT32 implAttrsNum;

    METHOD_BEGIN(ildjitSystem, "System.Reflection.ClrHelpers.GetImplAttrs");

    /* Initialize the variables     */
    clrMember = NULL;
    t_method = NULL;
    implAttrsNum = -1;

    /* Fetch the CLRMember          */
    clrMember = (CLRMember *) item;
    assert((clrMember->type == CLRMETHOD) || (clrMember->type == CLRCONSTRUCTOR));

    t_method = (MethodDescriptor *) clrMember->ID;
    assert(t_method != NULL);
    implAttrsNum = t_method->row->impl_flags;
    assert(implAttrsNum >= 0);

    METHOD_END(ildjitSystem, "System.Reflection.ClrHelpers.GetImplAttr");

    return implAttrsNum;
}

void * System_Reflection_ClrHelpers_GetSemantics (JITNINT itemPrivate, JITINT32 type, JITBOOLEAN nonPublic) { //già fatto
    void            *object;
    CLRMember       *clrMember;
    MethodDescriptor        *methodID;
#ifdef PRINTDEBUG
    JITINT8         	*name;
#endif

    METHOD_BEGIN(ildjitSystem, "System.Reflection.ClrHelpers.GetSemantics");

    /* Initialize the variables	*/
    object = NULL;
    clrMember = NULL;

    /* Fetch the CLRMember		*/
    clrMember = (CLRMember *) itemPrivate;
    if (clrMember != NULL) {

        /* Fetch the right method ID	*/
        methodID = ILMethodSemGetByType(clrMember, type);
        if (methodID != NULL) {

            /* Print the name	*/
#ifdef PRINTDEBUG
            name = methodID->getCompleteName(methodID);
            assert(name != NULL);
            PDEBUG("Internal calls: System.Reflection.ClrHelpers.GetSemantics:      Method	= %s\n", name);
#endif
            if (methodID->isCtor(methodID)) {
                object = ((ildjitSystem->cliManager).CLR.reflectManager).getConstructor(methodID);
                assert(object != NULL);
            } else {
                object = ((ildjitSystem->cliManager).CLR.reflectManager).getMethod(methodID);
                assert(object != NULL);
            }
            assert(object != NULL);
        }
    }

    /* Exit				*/
    METHOD_END(ildjitSystem, "System.Reflection.ClrHelpers.GetSemantics");
    return object;
}

JITBOOLEAN System_Reflection_ClrHelpers_HasSemantics (JITNINT item, JITINT32 type, JITBOOLEAN nonPublic) {
    CLRMember               *clrMember;
    MethodDescriptor *methodID;
    JITBOOLEAN boolean;

    METHOD_BEGIN(ildjitSystem, "System.Reflection.ClrHelpers.HasSemantics");

    /*initialize the variables	*/
    clrMember = NULL;
    methodID = NULL;
    boolean = JITFALSE;

    clrMember = (CLRMember *) item;
    assert(clrMember->type == CLRPROPERTY);

    methodID = ILMethodSemGetByType(clrMember, type);
    /*
       if (!nonPublic && (methodID == NULL) && clrMember->ID->attributes->accessMask==PUBLIC_METHOD) {
            methodID = NULL;
       }
     */
    if (methodID == NULL) { // && _ILClrCheckAccess FIXME
        boolean = JITTRUE;
    }

    METHOD_END(ildjitSystem, "System.Reflection.ClrHelpers.HasSemantics");
    return boolean;

}

JITBOOLEAN System_Reflection_ClrHelpers_CanAccess (JITNINT item) {
//FIXME TODO è un wrapper a _ILClrCheckAccess
    CLRMember               * clrMember;
    BasicAssembly   * t_assembly;
    MethodDescriptor        * t_method;
    EventDescriptor * t_event;
    FieldDescriptor * t_field;
    PropertyDescriptor      * t_property;
    TypeDescriptor  * t_type;

    METHOD_BEGIN(ildjitSystem, "System.Reflection.ClrHelpers.CanAccess");

    /*Initialize the variables	*/
    clrMember = NULL;
    t_assembly = NULL;
    t_method = NULL;
    t_event = NULL;
    t_field = NULL;
    t_property = NULL;
    t_type = NULL;

    /* Fetch the CLRMember          */
    clrMember = (CLRMember *) item;

    switch (clrMember->type) {
        case CLRASSEMBLY:
            t_assembly = (BasicAssembly *) clrMember->ID;
            assert(t_assembly != NULL);
            break;
        case CLRCONSTRUCTOR:
        case CLRMETHOD:
            t_method = (MethodDescriptor *) clrMember->ID;
            assert(t_method != NULL);
            break;
        case CLREVENT:
            t_event = (EventDescriptor *) clrMember->ID;
            assert(t_event != NULL);
            break;
        case CLRFIELD:
            t_field = (FieldDescriptor *) clrMember->ID;
            assert(t_field != NULL);
            break;
        case CLRPARAMETER:
            break;
        case CLRPROPERTY:
            t_property = (PropertyDescriptor *) clrMember->ID;
            assert(t_property != NULL);
            break;
        case CLRTYPE:
            t_type = (TypeDescriptor *) clrMember->ID;
            assert(t_type != NULL);
            break;
        default:
            print_err("bad Clr types as input ", 0);
            abort();
    }

    //FIXME
    METHOD_END(ildjitSystem, "System.Reflection.ClrHelpers.CanAccess");

    return JITTRUE;

}

//***************************************************************************************************************/
//*							CLR_FIELD						*/
//***************************************************************************************************************/
void * System_Reflection_ClrField_GetValueInternal (void *_this, void *object) {
    CLRField                *clrField;
    void                    *newObj;

    /* Assertions.
     */
    assert(_this != NULL);
    METHOD_BEGIN(ildjitSystem, "System.Reflection.ClrField.GetValueInternal");

    /* Initialize the variables.
     */
    clrField 	= NULL;
    newObj		= NULL;

    /* Fetch clrField.
     */
    clrField = ((ildjitSystem->cliManager).CLR.reflectManager).getPrivateDataFromField(_this);

    if (clrField == NULL) {
        ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "OutOfMemoryException");
        abort();
    }
    assert(clrField->ID != NULL);

    /* Check whether the field is inside the object.
     */
    if (object != NULL) {
        print_err("ERROR: The internal method System.Reflection.ClrField.GetValueInternal is not yet implemented. ", 0);
        abort();

    } else {
        TypeDescriptor		*typeOfField;
        TypeDescriptor		*ownerOfField;
        StaticMemoryManager	*staticMemoryManager;
        ILLayoutStatic		*classLayout;
        ILLayout_manager	*layoutManager;
        ILFieldLayout		*fieldLayout;
        void			*staticClassMemory;

        /* Cache some managers.
         */
        staticMemoryManager	= &(ildjitSystem->staticMemoryManager);
        layoutManager		= &((ildjitSystem->cliManager).layoutManager);

        /* Fetch the metadata of the class.
         */
        ownerOfField	= clrField->ID->owner;
        assert(ownerOfField != NULL);
        typeOfField	= clrField->ID->getType(clrField->ID);
        assert(typeOfField != NULL);

        /* Fetch the layout of the static memory.
         */
        classLayout		= layoutManager->layoutStaticType(layoutManager, ownerOfField);
        assert(classLayout != NULL);

        /* Fetch the offset of the field.
         */
        fieldLayout		= classLayout->fetchStaticFieldLayout(classLayout, clrField->ID);
        assert(fieldLayout != NULL);

        /* Fetch the memory allocated for the class.
         */
        staticClassMemory	= STATICMEMORY_fetchStaticObject(staticMemoryManager, NULL, ownerOfField);
        assert(staticClassMemory != NULL);

        /* Allocate a new object of the type of the field.
         */
        newObj 			= (ildjitSystem->garbage_collectors).gc.allocObject(typeOfField, 0);
        assert(newObj != NULL);

        /* Store the value of the field inside the just allocated object;
         */
        memcpy(newObj, (void *)(((JITNINT)staticClassMemory) + fieldLayout->offset), fieldLayout->length);
    }

    METHOD_END(ildjitSystem, "System.Reflection.ClrField.GetValueInternal");
    return newObj;
}

void * System_Reflection_ClrField_GetFieldType (JITNINT item) {
    CLRField                *clrField;
    TypeDescriptor  *ilType;
    void                    * obj;

    METHOD_BEGIN(ildjitSystem, "System.Reflection.ClrField.GetFieldType");

    /* Initialize the variables	*/
    clrField = NULL;
    obj = NULL;

    /* Fetch right Clrfield and field row	*/
    clrField = (CLRField *) item;
    assert(clrField != NULL);
    assert(clrField->ID != NULL);

    /* Fill information into the clrType	*/
    ilType = clrField->ID->getType(clrField->ID);
    assert(ilType != NULL);

    /*Create the return object	*/
    obj = ((ildjitSystem->cliManager).CLR.reflectManager).getType(ilType);
    assert(obj != NULL);

    METHOD_END(ildjitSystem, "System.Reflection.ClrField.GetFieldType");

    return obj;
}

//***************************************************************************************************************/
//*							CLR_CONSTRUCTOR						*/
//***************************************************************************************************************/
void * System_Reflection_ClrConstructor_Invoke (void *_this, JITINT32 invokeAttr, void *binder, void *parametersArray, void *culture) { //uguale a ClrMethod_Invoke
    void            *newObject;
    CLRConstructor  *clrConstructor;
    Method 		method;
#ifdef PRINTDEBUG
    JITINT8         *name;
#endif

    /* Assertions				*/
    assert(_this != NULL);
    METHOD_BEGIN(ildjitSystem, "System.Reflection.ClrConstructor.Invoke");

    /* Initialize the variables		*/
    clrConstructor = NULL;
    newObject = NULL;
    method = NULL;

    /* Fetch the method to call		*/
    clrConstructor = ((ildjitSystem->cliManager).CLR.reflectManager).getPrivateDataFromConstructor(_this);
    if (clrConstructor == NULL) {
        ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "MissingMethodException");
    }
    assert(clrConstructor != NULL);
    assert(clrConstructor->ID != NULL);

    /* Print the name of the method		*/
#ifdef PRINTDEBUG
    name = clrConstructor->ID->getCompleteName(clrConstructor->ID);
    assert(name != NULL);
    PDEBUG("Internal calls: System.Reflection.ClrConstructor.Invoke:        Constructor to call	= %s\n", name);
#endif

    /* Put the method inside the pipeline	*/
    PDEBUG("Internal calls: System.Reflection.ClrConstructor.Invoke:        Fetch the constructor\n");
    t_methods *methods = &((ildjitSystem->cliManager).methods);
    method = methods->fetchOrCreateMethod(methods, clrConstructor->ID, JITTRUE);
    assert(method != NULL);
    PDEBUG("Internal calls: System.Reflection.ClrConstructor.Invoke:        Put the constructor inside the pipeline\n");
    (ildjitSystem->pipeliner).synchInsertMethod(&(ildjitSystem->pipeliner), method, MAX_METHOD_PRIORITY);
    if (	(!(ildjitSystem->IRVM).behavior.aot) 		&&
            ((ildjitSystem->IRVM).behavior.pgc == 1)	) {
        MANFRED_appendMethodToProfile(&(ildjitSystem->manfred), method);
    }
    newObject = InvokeMethod(method, NULL, parametersArray);

    /* Exit				*/
    METHOD_END(ildjitSystem, "System.Reflection.ClrConstructor.Invoke");
    return newObject;

}

//***************************************************************************************************************/
//*							CLR_METHOD						*/
//***************************************************************************************************************/
void * System_Reflection_ClrMethod_GetBaseDefinition (void *_this) {
    CLRMethod               *clrMethod;
    MethodDescriptor        *method;
    MethodDescriptor        *base_method;
    void                    * obj;

    assert(_this != NULL);
    METHOD_BEGIN(ildjitSystem, "System.Reflection.ClrMethod.GetBaseDefinition");

    /*Initialize the variables	*/
    clrMethod = NULL;
    obj = NULL;

    /* Fetch the CLRMethod          */
    clrMethod = ((ildjitSystem->cliManager).CLR.reflectManager).getPrivateDataFromMethod(_this);

    if (clrMethod == NULL) {
        ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "OutOfMemoryException");
    }
    assert(clrMethod->ID != NULL);

    /* get right Method structure	*/
    method = clrMethod->ID;
    assert(method != NULL);

    base_method = method->getBaseDefinition(method);

    if (base_method == method) {
        obj = _this;
    } else {
        obj = ((ildjitSystem->cliManager).CLR.reflectManager).getMethod(base_method);
        assert(obj != NULL);
    }

    /* Return				*/
    METHOD_END(ildjitSystem, "System.Reflection.ClrMethod.GetBaseDefinition");

    return obj;
}

void * System_Reflection_ClrMethod_Invoke (void *_this, void *object, JITINT32 invokeAttr, void *binder, void *parametersArray, void *culture) {
    void            *newObject;
    CLRMethod       *clrMethod;
    JITINT8         *name;
    Method method;

    /* Assertions				*/
    assert(_this != NULL);
    METHOD_BEGIN(ildjitSystem, "System.Reflection.ClrMethod.Invoke");

    /* Initialize the variables		*/
    clrMethod = NULL;
    name = NULL;
    newObject = NULL;
    method = NULL;

    /* Fetch the method to call		*/
    clrMethod = ((ildjitSystem->cliManager).CLR.reflectManager).getPrivateDataFromMethod(_this);
    if (clrMethod == NULL) {
        ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "MissingMethodException");
        abort();
    }
    assert(clrMethod != NULL);
    assert(clrMethod->ID != NULL);

    /* Print the name of the method		*/
#ifdef DEBUG
    name = clrMethod->ID->getCompleteName(clrMethod->ID);
    assert(name != NULL);
    PDEBUG("Internal calls: System.Reflection.ClrMethod.Invoke:     Method to call	= %s\n", name);
#endif

    /* Put the method inside the pipeline	*/
    PDEBUG("Internal calls: System.Reflection.ClrMethod.Invoke:     Fetch the method\n");
    t_methods *methods = &((ildjitSystem->cliManager).methods);
    method = methods->fetchOrCreateMethod(methods, clrMethod->ID, JITTRUE);
    assert(method != NULL);
    newObject = InvokeMethod(method, object, parametersArray);

    /* Exit				*/
    METHOD_END(ildjitSystem, "System.Reflection.ClrMethod.Invoke");
    return newObject;
}

//***************************************************************************************************************/
//*							CLR_ASSEMBLY						*/
//***************************************************************************************************************/
void * System_Reflection_Assembly_GetEntryPoint (void *_this) { // da pnet sembra che ritorni l'oggetto descritto dal method associato
    // => ritorna l'oggetto associato alla prima riga della tabella t_method_def_table
    void                    *obj;
    CLRAssembly             *clrAssembly;
    t_binary_information *binary;
    MethodDescriptor        *method;

    METHOD_BEGIN(ildjitSystem, "System.Reflection.Assembly.GetEntryPoint");

    /* Initialize the variables	*/
    clrAssembly = NULL;
    obj = NULL;

    /* Fetch the CLRAssembly	*/
    clrAssembly = ((ildjitSystem->cliManager).CLR.reflectManager).getPrivateDataFromAssembly(_this);
    assert(clrAssembly != NULL);
    assert(clrAssembly->ID != NULL);

    /* Fetch the tables	*/
    binary = (t_binary_information *) clrAssembly->ID->binary;
    method = (ildjitSystem->cliManager).metadataManager->getMethodDescriptorFromTableAndRow((ildjitSystem->cliManager).metadataManager, NULL, binary, 0x0, 1, NULL); //METHOD_DEF_TABLE
    assert(method != NULL);


    obj = ((ildjitSystem->cliManager).CLR.reflectManager).getMethod(method);
    assert(obj != NULL);

    /* Return				*/
    METHOD_END(ildjitSystem, "System.Reflection.Assembly.GetEntryPoint");

    return obj;
}

void * System_Reflection_Assembly_GetSatellitePath (void *_this, void *fileNameString) {
    void            *string;
    CLRAssembly     *clrAssembly;
    JITINT8         *name;
    JITINT8         *prefixString;
    JITINT8         *fileNameStringUTF8;
    JITINT16        *fileNameStringUTF16;
    JITINT32 nameLen;
    JITINT32 prefixLen;
    JITINT32 fullLen;

    /* Assertions			*/
    assert(_this != NULL);
    METHOD_BEGIN(ildjitSystem, "System.String System.Reflection.Assembly.GetSatellitePath(System.String filename)");

    /* Initialize the variables	*/
    clrAssembly = NULL;
    string = NULL;
    name = NULL;
    fileNameStringUTF8 = NULL;
    fileNameStringUTF16 = NULL;
    prefixString = NULL;
    nameLen = -1;
    prefixLen = -1;
    fullLen = -1;
    string = NULL;

    /* Fetch the CLRAssembly			*/
    clrAssembly = ((ildjitSystem->cliManager).CLR.reflectManager).getPrivateDataFromAssembly(_this);
    if (clrAssembly == NULL) {

        /* Return					*/
        METHOD_END(ildjitSystem, "System.String System.Reflection.Assembly.GetSatellitePath(System.String filename)");
        return NULL;
    }
    assert(clrAssembly != NULL);
    assert(clrAssembly->ID != NULL);

    /* Fetch the name of the file			*/
    if (fileNameString == NULL) {

        /* Return					*/
        METHOD_END(ildjitSystem, "System.String System.Reflection.Assembly.GetSatellitePath(System.String filename)");
        return NULL;
    }
    fileNameStringUTF16 = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(fileNameString);
    assert(fileNameStringUTF16 != NULL);
    nameLen = (ildjitSystem->cliManager).CLR.stringManager.getLength(fileNameString);
    if (nameLen == 0) {

        /* Return					*/
        METHOD_END(ildjitSystem, "System.String System.Reflection.Assembly.GetSatellitePath(System.String filename)");
        return NULL;
    }
    assert(nameLen > 0);
    nameLen++;
    fileNameStringUTF8 = allocMemory(nameLen);
    (ildjitSystem->cliManager).CLR.stringManager.fromUTF16ToUTF8(fileNameStringUTF16, fileNameStringUTF8, nameLen - 1);

    /* Fetch the prefix of the assembly		*/
    t_binary_information *binary = (t_binary_information *) clrAssembly->ID->binary;
    prefixString = binary->prefix;
    assert(prefixString != NULL);
    prefixLen = STRLEN(prefixString);
    assert(prefixLen > 0);

    /* Add the separator character between prefix	*
     * and file name				*/
    prefixLen++;

    /* Compute the full string length		*/
    fullLen = nameLen + prefixLen;
    assert(fullLen > 0);

    /* Allocate the literal to store the full file	*
     * name						*/
    name = (JITINT8 *) allocMemory(fullLen);
    assert(name != NULL);
    SNPRINTF(name, fullLen, "%s/%s", prefixString, fileNameStringUTF8);

    /* Check the existance of the file		*/
    string = NULL;
    if (internal_fileExists(name)) {

        /* Allocate an instance of System.String	*/
        string = CLIMANAGER_StringManager_newInstanceFromUTF8(name, fullLen);
        assert(string != NULL);
    }

    /* Free the memory				*/
    freeMemory(name);

    /* Return				*/
    METHOD_END(ildjitSystem, "System.Reflection.Assembly.GetSatellitePath");
    return string;
}

void * System_Reflection_Assembly_LoadFromFile (void *name, JITINT32 *error, void *object) {
    void                    *newObject;
    void                    *objectAssembly;
    JITINT16                *literal;
    JITINT32 literalLength;
    BasicAssembly   *ilAssembly;
    JITINT8                 *buf;

    /* Assertions				        */
    assert(ildjitSystem != NULL);
    assert(name != NULL);
    assert(object != NULL);
    METHOD_BEGIN(ildjitSystem, "System.Reflection.Assembly.LoadFromFile");
    newObject = NULL;

    /* Fetch the literal			        */
    literal = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(name);
    assert(literal != NULL);

    /* Fetch the length				*/
    literalLength = (ildjitSystem->cliManager).CLR.stringManager.getLength(name);

    /* Check the filename			        */
    if (literal == NULL) {
        (*error) = LOAD_ERROR_FILE_NOT_FOUND;
        METHOD_END(ildjitSystem, "System.Reflection.Assembly.LoadFromFile");
        return NULL;
    }
    if (    (literal[0] == '\0')    ||
            (literalLength == 0)    ) {
        (*error) = LOAD_ERROR_INVALID_NAME;
        METHOD_END(ildjitSystem, "System.Reflection.Assembly.LoadFromFile");
        return NULL;
    }

    /* Fetch the name of the Assembly to load	*/
    buf = allocMemory(literalLength + 1);
    (ildjitSystem->cliManager).CLR.stringManager.fromUTF16ToUTF8(literal, buf, literalLength);

    /* Search the ILAssembly			*/
    ilAssembly = getAssemblyFromName(buf);
    if (ilAssembly == NULL) {

        /* Free the memory				*/
        freeMemory(buf);

        /* Return					*/
        (*error) = LOAD_ERROR_FILE_NOT_FOUND;
        METHOD_END(ildjitSystem, "System.Reflection.Assembly.LoadFromFile");
        return NULL;
    }

    /* Fetch the assembly   */
    objectAssembly = ((ildjitSystem->cliManager).CLR.reflectManager).getAssembly(ilAssembly);
    if (objectAssembly == NULL) {
        ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "OutOfMemoryException");
        abort();
    }
    assert(objectAssembly != NULL);
    assert(((ildjitSystem->cliManager).CLR.reflectManager).getPrivateDataFromAssembly(objectAssembly)->ID == ilAssembly);
    (*error) = LOAD_ERROR_OK;

    /* Free the memory				*/
    freeMemory(buf);

    /* Return				*/
    METHOD_END(ildjitSystem, "System.Reflection.Assembly.LoadFromFile");
    return newObject;
}

void * System_Reflection_Assembly_LoadFromName (void *name, JITINT32 * error, void *object) { //già fatto
    JITINT16                *literal;
    JITINT32 literalLength;
    BasicAssembly   *ilAssembly;
    JITINT8                 *buf;
    void                    *objectAssembly;

    /* Assertions				*/
    assert(ildjitSystem != NULL);
    assert(name != NULL);
    assert(object != NULL);
    METHOD_BEGIN(ildjitSystem, "System.Reflection.Assembly.LoadFromName");

    /* Initialize the variables		*/
    objectAssembly = NULL;
    literal = NULL;

    /* Fetch the literal			*/
    literal = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(name);
    assert(literal != NULL);

    /* Check the literal			*/
    if (literal != NULL) {

        /* Fetch the literal length		*/
        literalLength = (ildjitSystem->cliManager).CLR.stringManager.getLength(name);

        /* Print the name of the Assembly to load	*/
        buf = (JITINT8 *) allocMemory(literalLength + 1);
        (ildjitSystem->cliManager).CLR.stringManager.fromUTF16ToUTF8(literal, buf, literalLength);
        PDEBUG("Internal calls: System.Reflection.Assembly.LoadFromName:        Assembly to load	= %s\n", buf);

        /* Search the ILAssembly			*/
        ilAssembly = getAssemblyFromName(buf);
        if (ilAssembly == NULL) {
            PDEBUG("Internal calls: System.Reflection.Assembly.LoadFromName:        getAssemblyFromName did not return the assembly\n");
            (*error) = LOAD_ERROR_FILE_NOT_FOUND;
        } else {

            /* Fetch the assembly   */
            objectAssembly = ((ildjitSystem->cliManager).CLR.reflectManager).getAssembly(ilAssembly);
            if (objectAssembly == NULL) {
                ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "OutOfMemoryException");
            }
            assert(objectAssembly != NULL);
            assert(((ildjitSystem->cliManager).CLR.reflectManager).getPrivateDataFromAssembly(objectAssembly)->ID == ilAssembly);
            (*error) = LOAD_ERROR_OK;
        }

        /* Free the memory			*/
        freeMemory(buf);

    } else {
        (*error) = LOAD_ERROR_FILE_NOT_FOUND;
    }

    /* Return				*/
    METHOD_END(ildjitSystem, "System.Reflection.Assembly.LoadFromName");
    return objectAssembly;
}

void * System_Reflection_Assembly_GetCallingAssembly (void) { //già fatto
    void            *assembly;

    METHOD_BEGIN(ildjitSystem, "System.Reflection.Assembly.GetCallingAssembly");

    /* Fetch the executing assembly		*/
    assembly = internal_GetExecutingAssembly();
    assert(assembly != NULL);

    /* Return				*/
    METHOD_END(ildjitSystem, "System.Reflection.Assembly.GetCallingAssembly");
    return assembly;
}

void * System_Reflection_Assembly_GetTypes (void *_this) { //da pnet ritorna tutti gli oggeti descritti da type associati a questo assembly
    // => ritorna un array di oggetti "creati" a partire dalla tabella type_def
    void                    *array;
    CLRAssembly             *clrAssembly;
    t_binary_information *binary;
    t_streams_metadata      *streams;
    t_metadata_tables       *tables;
    JITINT32 i;
    t_type_def_table        *type_table;
    TypeDescriptor  *ilType;
    void                    *obj;

    METHOD_BEGIN(ildjitSystem, "System.Reflection.Assembly.GetTypes");

    /* Initialize the variables	*/
    clrAssembly = NULL;
    array = NULL;
    streams = NULL;
    tables = NULL;
    type_table = NULL;
    obj = NULL;

    /* Fetch the CLRAssembly	*/
    clrAssembly = ((ildjitSystem->cliManager).CLR.reflectManager).getPrivateDataFromAssembly(_this);
    assert(clrAssembly != NULL);
    assert(clrAssembly->ID != NULL);

    /* Fetch the tables	*/
    binary = (t_binary_information *) clrAssembly->ID->binary;
    streams = &(binary->metadata.streams_metadata);
    assert(streams != NULL);
    tables = &(streams->not_stream.tables);
    assert(tables != NULL);

    type_table = (t_type_def_table *) get_table(tables, TYPE_DEF_TABLE);
    assert(type_table != NULL);

    /* Fetch the first row of the type_def_table, is used in the allocArray method	*/

    TypeDescriptor *objectType = (ildjitSystem->cliManager).metadataManager->getTypeDescriptorFromElementType((ildjitSystem->cliManager).metadataManager, ELEMENT_TYPE_OBJECT);
    array = (ildjitSystem->garbage_collectors).gc.allocArray(objectType, 1, type_table->cardinality);
    assert(array != NULL);

    /* Fetch each row of the type_def table,
     * binary is always the same so is outside the for cycle	*/

    for (i = 1; i < type_table->cardinality; i++) {
        ilType = (ildjitSystem->cliManager).metadataManager->getTypeDescriptorFromTableAndRow((ildjitSystem->cliManager).metadataManager, NULL, binary, 0x0, i); //TYPE_DEF_TABLE
        assert(ilType != NULL);

        obj = ((ildjitSystem->cliManager).CLR.reflectManager).getType(ilType);
        assert(obj != NULL);

        (ildjitSystem->cliManager).CLR.arrayManager.setValueArrayElement(array, i - 1, &obj);

    }
    /* Return				*/
    METHOD_END(ildjitSystem, "System.Reflection.Assembly.GetTypes");
    return array;
}

void * System_Reflection_Assembly_GetModuleInternal (void *_this, void *nameString) {
    void                    *obj;
    CLRAssembly             *clrAssembly;
    t_binary_information *binary;
    t_streams_metadata      *streams;
    t_metadata_tables       *tables;
    t_module_table          *module_table;
    JITINT16                *literalUTF16;
    JITINT8                 *literalUTF8;
    JITINT8                 *module_name;
    JITINT8                 *file_name;
    JITINT32 i;
    JITINT32 stringLength = -1;
    t_row_module_table      *module_row;
    t_row_file_table        *file_row;
    t_file_table            *file_table;

    METHOD_BEGIN(ildjitSystem, "System.Reflection.Assembly.GetModuleInternal");

    /* Initialize the variables	*/
    clrAssembly = NULL;
    obj = NULL;
    streams = NULL;
    tables = NULL;
    module_table = NULL;
    literalUTF16 = NULL;
    literalUTF8 = NULL;
    module_name = NULL;
    module_row = NULL;
    file_row = NULL;
    file_table = NULL;
    file_name = NULL;

    /* Fetch the CLRAssembly	*/
    clrAssembly = ((ildjitSystem->cliManager).CLR.reflectManager).getPrivateDataFromAssembly(_this);
    assert(clrAssembly != NULL);
    assert(clrAssembly->ID != NULL);

    literalUTF16 = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(nameString);
    assert(literalUTF16 != NULL);

    /* Fetch the length of the string*/
    stringLength = (ildjitSystem->cliManager).CLR.stringManager.getLength(nameString);
    assert(stringLength > 0);

    /* Make the UTF8 literal	*/
    literalUTF8 = allocMemory((stringLength + 1) * sizeof(JITINT8));
    assert(literalUTF8 != NULL);
    (ildjitSystem->cliManager).CLR.stringManager.fromUTF16ToUTF8(literalUTF16, literalUTF8, stringLength);

    /* Fetch the tables	*/
    binary = (t_binary_information *) clrAssembly->ID->binary;
    streams = &(binary->metadata.streams_metadata);
    assert(streams != NULL);
    tables = &(streams->not_stream.tables);
    assert(tables != NULL);

    module_table = (t_module_table *) get_table(tables, MODULE_TABLE);
    assert(module_table != NULL);
    file_table = (t_file_table *) get_table(tables, FILE_TABLE);
    assert(file_table != NULL);

    for (i = 1; i < module_table->cardinality; i++) {
        module_row = (t_row_module_table *) get_row(tables, MODULE_TABLE, i);
        assert(module_row != NULL);

        module_name = get_string(&(streams->string_stream), module_row->name);
        assert(module_name != NULL);

        if (reflectionStringCmp(literalUTF8, module_name, 0)) {
            assert(module_row != NULL);
            break;
        }
    }

    obj = ((ildjitSystem->cliManager).CLR.reflectManager).getModule(module_row);
    assert(obj != NULL);

    if (obj == NULL) {
        for (i = 1; i < file_table->cardinality; i++) {
            file_row = (t_row_file_table *) get_row(tables, FILE_TABLE, i);
            assert(file_row != NULL);

            file_name = get_string(&(streams->string_stream), file_row->name);
            assert(file_name != NULL);

            if (reflectionStringCmp(literalUTF8, file_name, 0)) {
                //FIXME devo passare da una riga della tabella file ed una di module
                //clrModule.ID = file_row;
                assert(file_row != NULL);
                break;
            }
        }

        obj = ((ildjitSystem->cliManager).CLR.reflectManager).getModule(module_row);
        assert(obj != NULL);

    }

    /* Return				*/
    METHOD_END(ildjitSystem, "System.Reflection.Assembly.GetModuleInternal");
    return obj;
}

void * System_Reflection_Assembly_GetType (void *_this, void *stringName, JITBOOLEAN throwOnError, JITBOOLEAN ignoreCase) { //già fatto
    void                    *object;
    TypeDescriptor  *ilType;
    CLRAssembly             *clrAssembly;
    t_binary_information *binary;

    JITINT16                *literalUTF16;
    JITINT8                 *literalUTF8;
    JITINT8                 *tmpString;
    JITINT8                 *typeName;
    JITINT8                 *typeNameSpace;
    JITINT32 stringLength;

    /* Assertions			*/
    assert(_this != NULL);
    assert(stringName != NULL);

    /* Initialize the variables	*/
    object = NULL;
    literalUTF16 = NULL;
    literalUTF8 = NULL;

    METHOD_BEGIN(ildjitSystem, "System.Reflection.Assembly.GetType");

    /* Fetch the CLRAssembly	*/
    clrAssembly = ((ildjitSystem->cliManager).CLR.reflectManager).getPrivateDataFromAssembly(_this);
    assert(clrAssembly != NULL);
    assert(clrAssembly->ID != NULL);
    binary = (t_binary_information *) clrAssembly->ID->binary;

    /* Fetch the literal		*/
    literalUTF16 = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(stringName);
    assert(literalUTF16 != NULL);

    /* Fetch the length of the string*/
    stringLength = (ildjitSystem->cliManager).CLR.stringManager.getLength(stringName);
    assert(stringLength > 0);

    /* Make the UTF8 literal	*/
    literalUTF8 = allocMemory((stringLength + 1) * sizeof(JITINT8));
    assert(literalUTF8 != NULL);
    (ildjitSystem->cliManager).CLR.stringManager.fromUTF16ToUTF8(literalUTF16, literalUTF8, stringLength);
    PDEBUG("Internal calls: System.Reflection.Assembly.GetType:     Type    = %s\n", literalUTF8);
    tmpString = fetchLastName(literalUTF8);
    assert(tmpString != NULL);
    typeName = allocMemory(sizeof(JITINT8) * (STRLEN(tmpString) + 1));
    SNPRINTF(typeName, STRLEN(tmpString) + 1, "%s", tmpString);
    typeNameSpace = literalUTF8;
    (*(literalUTF8 + (stringLength - STRLEN(typeName)))) = '\0';


    PDEBUG("Internal calls: System.Reflection.Assembly.GetType:             TypeName        = %s\n", typeName);
    PDEBUG("Internal calls: System.Reflection.Assembly.GetType:             TypeNameSpace   = %s\n", typeNameSpace);

    /* Make the ILType		*/
    ilType = (ildjitSystem->cliManager).metadataManager->getTypeDescriptorFromNameAndAssembly((ildjitSystem->cliManager).metadataManager, binary, typeNameSpace, typeName);

    /* Fetch the type instance	*/
    if (ilType != NULL) {
        object = ((ildjitSystem->cliManager).CLR.reflectManager).getType(ilType);
        assert(object != NULL);
    }
    if ( (object == NULL)   && (throwOnError) ) {
        ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "TypeLoadException");
    }

    /* Free the memory		*/
    freeMemory(typeName);
    freeMemory(literalUTF8);

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Reflection.Assembly.GetType");
    return object;
}

void * System_Reflection_Assembly_GetExportedTypes (void *_this) {
    void                    *array;
    CLRAssembly             *clrAssembly;
    t_binary_information *binary;
    t_type_def_table        *type_table;
    t_streams_metadata      *streams;
    t_metadata_tables       *tables;
    JITINT32 i;
    TypeDescriptor  *ilType;
    void                    *obj;

    //FIXME pnet return all the types for this assembly, also for the pnet projrct is a todo to return only the exported ones

    METHOD_BEGIN(ildjitSystem, "System.Reflection.Assembly.GetExportedTypes");

    /* Initialize the variables	*/
    clrAssembly = NULL;
    array = NULL;
    type_table = NULL;
    streams = NULL;
    tables = NULL;
    ilType = NULL;
    obj = NULL;

    /* Fetch the CLRAssembly	*/
    clrAssembly = ((ildjitSystem->cliManager).CLR.reflectManager).getPrivateDataFromAssembly(_this);
    assert(clrAssembly != NULL);
    assert(clrAssembly->ID != NULL);


    /* Fetch the tables	*/
    binary = (t_binary_information *) clrAssembly->ID->binary;
    streams = &(binary->metadata.streams_metadata);
    assert(streams != NULL);
    tables = &(streams->not_stream.tables);
    assert(tables != NULL);
    type_table = (t_type_def_table *) get_table(tables, TYPE_DEF_TABLE);
    assert(type_table != NULL);

    TypeDescriptor *objectType = (ildjitSystem->cliManager).metadataManager->getTypeDescriptorFromElementType((ildjitSystem->cliManager).metadataManager, ELEMENT_TYPE_OBJECT);

    array = (ildjitSystem->garbage_collectors).gc.allocArray(objectType, 1, type_table->cardinality);
    assert(array != NULL);

    for (i = 1; i < type_table->cardinality; i++) {
        ilType = (ildjitSystem->cliManager).metadataManager->getTypeDescriptorFromTableAndRow((ildjitSystem->cliManager).metadataManager, NULL, binary, 0x0, i); //TYPE_DEF_TABLE
        assert(ilType != NULL);

        obj = ((ildjitSystem->cliManager).CLR.reflectManager).getType(ilType);
        assert(obj != NULL);

        (ildjitSystem->cliManager).CLR.arrayManager.setValueArrayElement(array, i - 1, &obj);
    }

    /* Return				*/
    METHOD_END(ildjitSystem, "System.Reflection.Assembly.GetExportedTypes");
    return array;
}

void * System_Reflection_Assembly_GetManifestResourceStream (void *_this, void *stringName) {
    METHOD_BEGIN(ildjitSystem, "System.Reflection.Assembly.GetManifestResourceStream");

    METHOD_END(ildjitSystem, "System.Reflection.Assembly.GetManifestResourceStreams");
    return NULL;
}

/* Return given Assembly full name */
void* System_Reflection_Assembly_GetFullName (void* object) {
    /* The reflection manager */
    t_reflectManager* reflectManager;

    /* The assembly that we are examining */
    CLRAssembly* assembly;

    /* Assembly identifier in metadata table */
    BasicAssembly* assemblyID;

    t_binary_information *binary;

    /* System metadata */
    t_metadata* metadata;

    /* The string stream inside metadata */
    t_string_stream* stringStream;

    /* Assembly name */
    JITINT8* name;

    /* Assembly culture */
    JITINT8* culture;

    /* Assembly hash algorithm */
    JITINT8* hashAlgorithm;

    /* A buffer used to build the assembly full name */
    JITINT8* buffer;

    /* Used to calculate buffer length */
    size_t length;

    /* The return value, aka the assembly full name, as String object */
    void* fullName;

    METHOD_BEGIN(ildjitSystem, "System.Reflection.Assembly.GetFullName");

    /* Cache some pointers */
    reflectManager = &((ildjitSystem->cliManager).CLR.reflectManager);

    /* Fetch the assembly */
    assembly = reflectManager->getPrivateDataFromAssembly(object);
    assemblyID = assembly->ID;

    /* Fetch the name of the assembly */
    binary = (t_binary_information *) assemblyID->binary;
    metadata = &(binary->metadata);
    stringStream = &(metadata->streams_metadata.string_stream);
    name = get_string(stringStream, assemblyID->name);

    /* Fetch the culture name */
    if (assemblyID->culture == 0) {
        culture = (JITINT8 *) "neutral";
    } else {
        culture = get_string(stringStream, assemblyID->culture);
    }

    /* Fetch hash algorithm */
    switch (assemblyID->hash_algorithm_ID) {
        case 0x8003:
            hashAlgorithm = (JITINT8 *) "MD5";
            break;
        case 0x8004:
            hashAlgorithm = (JITINT8 *) "SHA1";
            break;
        default:
            hashAlgorithm = (JITINT8 *) "None";
    }

    /*
     * Compute how many characters we need in the worst case: first add the
     * length of the template string
     */
    length = STRLEN(ASSEMBLY_FULL_NAME_TEMPLATE);

    /* Add the length of the name; sub the 2 (%s) just counted chars */
    length += STRLEN(name) - 2;

    /*
     * Add the chars for version info. In the worst case we have a version
     * number with 5 digits (2^16 -1 = 65535). Remember that we have 4
     * version number and that we have just counted 2 chars (%d) for each of
     * them
     */
    length += 4 * (5 - 2);

    /*
     * Add space for culture and hash algorithm, as for name; also add space
     * for trailing '\0'
     */
    length += STRLEN(culture) - 2 + STRLEN(hashAlgorithm) - 2 + 1;

    /* Make the full name */
    buffer = allocMemory(sizeof(char) * length);
    SNPRINTF(buffer, length, ASSEMBLY_FULL_NAME_TEMPLATE, name,
             assemblyID->major_version, assemblyID->minor_version,
             assemblyID->build_number, assemblyID->revision_number,
             culture, hashAlgorithm);

    /* At last build a new String object */
    fullName = CLIMANAGER_StringManager_newInstanceFromUTF8(buffer, STRLEN(buffer));

    /* Free resources */
    freeMemory(buffer);

    METHOD_END(ildjitSystem, "System.Reflection.Assembly.GetFullName");

    return fullName;
}

void * System_Reflection_Assembly_GetEntryAssembly (void) { //già fatto
    void                    *object;
    BasicAssembly   	*assembly;

    METHOD_BEGIN(ildjitSystem, "System.Reflection.Assembly.GetEntryAssembly");

    /* Initialize the variables		*/
    object = NULL;
    assembly = NULL;

    /* Search the assembly that contains the method metadata*/
    assembly = ((ildjitSystem->program).entry_point)->getAssembly((ildjitSystem->program).entry_point);
    assert(assembly != NULL);

    /* Fetch the instance of System.Reflection.Assembly of  *
     * the callerBinary					*/
    object = ((ildjitSystem->cliManager).CLR.reflectManager).getAssembly(assembly);
    assert(object != NULL);

    /* Return the System.Type of the object	*/
    METHOD_END(ildjitSystem, "System.Reflection.Assembly.GetEntryAssembly");
    return object;
}

void * System_Reflection_Assembly_GetExecutingAssembly (void) { //già fatto
    void            *assembly;

    METHOD_BEGIN(ildjitSystem, "System.Reflection.Assembly.GetExecutingAssembly");

    /* Fetch the executing assembly		*/
    assembly = internal_GetExecutingAssembly();
    assert(assembly != NULL);

    /* Return				*/
    METHOD_END(ildjitSystem, "System.Reflection.Assembly.GetExecutingAssembly");
    return assembly;
}

//***************************************************************************************************************/
//*							_MODULE							*/
//***************************************************************************************************************/
void * System_Reflection_Module_GetAssembly (void *_this) {
    CLRModule               *clrModule;
    t_binary_information *binary;
    t_streams_metadata      *streams;
    t_metadata_tables       *tables;
    BasicAssembly   *assembly_row;
    void                    *obj;

    /* Assertions                   */
    assert(_this != NULL);

    METHOD_BEGIN(ildjitSystem, "System.Reflection.Module.GetAssembly");

    /*Initialize the variables      */
    clrModule = NULL;
    streams = NULL;
    tables = NULL;
    assembly_row = NULL;
    obj = NULL;

    /*Fetch the clrModule	*/
    clrModule = ((ildjitSystem->cliManager).CLR.reflectManager).getPrivateDataFromModule(_this);
    assert(clrModule != NULL);
    assert(clrModule->ID != NULL);


    /* Fetch the tables	*/
    binary = (t_binary_information *) clrModule->ID->binary;

    /* Fetch the tables	*/
    streams = &(binary->metadata.streams_metadata);
    assert(streams != NULL);
    tables = &(streams->not_stream.tables);
    assert(tables != NULL);

    assembly_row = (BasicAssembly *) get_first_row(tables, ASSEMBLY_TABLE);
    assert(assembly_row != NULL);

    /* Build the clrAssembly	*/
    obj = ((ildjitSystem->cliManager).CLR.reflectManager).getAssembly(assembly_row);
    assert(obj != NULL);

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Reflection.Module.GetAssembly");
    return obj;
}

void * System_Reflection_Module_GetModuleType (void *_this) { // non è il solito Get*
    CLRModule               *clrModule;
    t_binary_information *binary;
    void                    *obj;
    TypeDescriptor  *ilType;

    /* Assertions                   */
    assert(_this != NULL);
    METHOD_BEGIN(ildjitSystem, "System.Reflection.Module.GetModuleType");

    /*Initialize the variables      */
    clrModule = NULL;
    obj = NULL;
    ilType = NULL;

    clrModule = ((ildjitSystem->cliManager).CLR.reflectManager).getPrivateDataFromModule(_this);
    assert(clrModule != NULL);
    assert(clrModule->ID != NULL);

    /* Fetch the tables	*/
    binary = (t_binary_information *) clrModule->ID->binary;

    ilType = (ildjitSystem->cliManager).metadataManager->getTypeDescriptorFromTableAndRow((ildjitSystem->cliManager).metadataManager, NULL, binary, 0x0, 1); //TYPE_DEF_TABLE
    assert(ilType != NULL);

    obj = ((ildjitSystem->cliManager).CLR.reflectManager).getType(ilType);
    assert(obj != NULL);

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Reflection.Module.GetModuleType");
    return obj;
}
///////////////////24-08//////////////////
void * System_Reflection_Module_GetTypes (void *_this) { //ritorna un array di oggetti descritti dalla tabella type associata a questo module
    CLRModule               *clrModule;
    t_binary_information *binary;
    t_streams_metadata      *streams;
    t_metadata_tables       *tables;
    t_type_def_table        *type_table;
    void                    *array;
    void                    *obj;
    TypeDescriptor  *ilType;
    JITINT32 i;

    /* Assertions                   */
    assert(_this != NULL);
    METHOD_BEGIN(ildjitSystem, "System.Reflection.Module.GetTypes");

    /*Initialize the variables      */
    clrModule = NULL;
    streams = NULL;
    tables = NULL;
    type_table = NULL;
    array = NULL;
    obj = NULL;
    ilType = NULL;

    /*Fetch the clrModule	*/
    clrModule = ((ildjitSystem->cliManager).CLR.reflectManager).getPrivateDataFromModule(_this);
    assert(clrModule != NULL);
    assert(clrModule->ID != NULL);

    /* Fetch the tables	*/
    binary = (t_binary_information *) clrModule->ID->binary;
    streams = &(binary->metadata.streams_metadata);
    assert(streams != NULL);
    tables = &(streams->not_stream.tables);
    assert(tables != NULL);
    type_table = (t_type_def_table *) get_table(tables, TYPE_DEF_TABLE);
    assert(type_table != NULL);


    TypeDescriptor *objectType = (ildjitSystem->cliManager).metadataManager->getTypeDescriptorFromElementType((ildjitSystem->cliManager).metadataManager, ELEMENT_TYPE_OBJECT);
    array = (ildjitSystem->garbage_collectors).gc.allocArray(objectType, 1, type_table->cardinality);
    assert(array != NULL);

    for (i = 1; i < type_table->cardinality; i++) {
        ilType = (ildjitSystem->cliManager).metadataManager->getTypeDescriptorFromTableAndRow((ildjitSystem->cliManager).metadataManager, NULL, binary, 0x0, i); //TYPE_DEF_TABLE
        assert(ilType != NULL);

        /* Fetch the object	*/
        obj = ((ildjitSystem->cliManager).CLR.reflectManager).getType(ilType);
        assert(obj != NULL);

        /* Insert the object into the array	*/
        (ildjitSystem->cliManager).CLR.arrayManager.setValueArrayElement(array, i - 1, &obj);
    }

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Reflection.Module.GetTypes");
    return array;
}

void * System_Reflection_Module_GetType (void *_this, void *name, JITBOOLEAN throwOnError, JITBOOLEAN ignoreCase) { //FIXME???
    CLRModule               *clrModule;
    t_binary_information *binary;
    TypeDescriptor  *ilType;
    void                    *obj;

    /* Assertions                   */
    assert(_this != NULL);
    assert(name != NULL);
    METHOD_BEGIN(ildjitSystem, "System.Reflection.Module.GetType");

    /*Initialize the variables      */
    clrModule = NULL;
    binary = NULL;
    ilType = NULL;
    obj = NULL;

    /*Fetch the clrModule	*/
    clrModule = ((ildjitSystem->cliManager).CLR.reflectManager).getPrivateDataFromModule(_this);
    assert(clrModule != NULL);
    assert(clrModule->ID != NULL);

    /* Fetch the tables	*/
    binary = (t_binary_information *) clrModule->ID->binary;

    ilType = (ildjitSystem->cliManager).metadataManager->getTypeDescriptorFromTableAndRow((ildjitSystem->cliManager).metadataManager, NULL, binary, 0x0, 1); //TYPE_DEF_TABLE
    assert(ilType != NULL);

    obj = ((ildjitSystem->cliManager).CLR.reflectManager).getType(ilType);
    assert(obj != NULL);

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Reflection.Module.GetType");

    return obj;
}

//***************************************************************************************************************/
//*							CLR_PARAMETER						*/
//***************************************************************************************************************/
void * System_Reflection_ClrParameter_GetParamAttrs (JITNINT item) {    //FIXME guardando pnetlib ritorna un oggetto di tipo ParameterAttributes che non è altro che i flag della
    // tabella più alcuni altri, quindi alla fine un intero. Devo crearmi l'enum da qualche parte o posso
    // cambiare il tipo di ritorno del metodo visto che cmq è un intero (esadecimale) ?
    CLRParameter            *clrParameter;
    void                    * obj;
    //ParamDescriptor 	*param;

    METHOD_BEGIN(ildjitSystem, "System.Reflection.ClrParameter.GetParamAttrs");

    /*Initialize the variables	*/
    clrParameter = NULL;
    obj = NULL;

    clrParameter = (CLRParameter *) item;
    assert(clrParameter != NULL);
    assert(clrParameter->ID != NULL);

    //param = (ParamDescriptor *) clrParameter->ID;

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Reflection.ClrParameter.GetParamAttrs");

    //FIXME param_row->flags
    return obj;
}

void * System_Reflection_ClrParameter_GetParamName (JITNINT item) { //copiato da GetTypeName
    CLRParameter            *clrParameter;
    JITINT8                 *name;
    JITINT32 length;
    JITINT8                 *stringName;
    void                    *string;

    METHOD_BEGIN(ildjitSystem, "System.Reflection.ClrParameter.GetParamName");

    /*Initialize the variables	*/
    clrParameter = NULL;
    name = NULL;
    length = 0;
    stringName = NULL;
    string = NULL;

    clrParameter = (CLRParameter *) item;
    assert(clrParameter != NULL);
    assert(clrParameter->ID != NULL);

    stringName = clrParameter->ID->getName(clrParameter->ID);
    assert(name != NULL);

    length = STRLEN(stringName);
    length++;

    name = (JITINT8 *) allocMemory(sizeof(JITINT8) * length);

    SNPRINTF(name, sizeof(JITINT8) * length, "%s", stringName);

    string = CLIMANAGER_StringManager_newInstanceFromUTF8(name, length + 1);
    assert(string != NULL);

    freeMemory(name);

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Reflection.ClrParameter.GetParamName");
    return string;
}

//***************************************************************************************************************/
//*							CLR_PROPERTY						*/
//***************************************************************************************************************/
void * System_Reflection_ClrProperty_GetPropertyType (JITNINT item) { //ritorna l'oggetto descritto dalla riga di type_def associata alla property
    CLRProperty                     *clrProperty;
    TypeDescriptor          *ilType;
    void                            *obj;

    METHOD_BEGIN(ildjitSystem, "System.Reflection.ClrProperty.GetPropertyType");

    /*Initialize the variables	*/
    clrProperty = NULL;
    obj = NULL;

    clrProperty = (CLRProperty *) item;
    assert(clrProperty != NULL);
    assert(clrProperty->ID != NULL);

    ilType = clrProperty->ID->getType(clrProperty->ID);
    obj = ((ildjitSystem->cliManager).CLR.reflectManager).getType(ilType);
    assert(obj != NULL);

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Reflection.ClrProperty.GetPropertyType");

    return obj;
}

//***************************************************************************************************************/
//*							CLR_RESOURCE_STREAM					*/
//***************************************************************************************************************/
JITINT32 System_Reflection_ClrResourceStream_ResourceRead (JITNINT handle, JITINT64 position, void * buffer, JITINT32 offset, JITINT32 count) {

    METHOD_BEGIN(ildjitSystem, "System.Reflection.ClrResourceStream.ResourceRead");

    //FIXME buffer deve puntare al primo elemento dell'array
    memcpy(buffer + offset, &position, count);

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Reflection.ClrResourceStream.ResourceRead");

    return count;
}

//***************************************************************************************************************/
//*							METHOD_BASE						*/
//***************************************************************************************************************/

/* Get a System.Reflection.MethodBase object to represent the given handle */
void* System_Reflection_MethodBase_GetMethodFromHandle (void* handle) {
    /* The System.Runtime*Handle manager */
    RuntimeHandleManager* rhManager;

    /* The reflection library manager */
    t_reflectManager* rManager;

    /* Compiler representation of a method */
    CLRMethod* clrMethod;

    /* Compiler representation of a constructor */
    CLRConstructor* clrConstructor;

    /* The object to return */
    void* object;

    METHOD_BEGIN(ildjitSystem,
                 "System.Reflection.MethodBase.GetMethodFromHandle");

    /* Get some managers */
    rhManager = &((ildjitSystem->cliManager).CLR.runtimeHandleManager);
    rManager = &((ildjitSystem->cliManager).CLR.reflectManager);

    /* Get the method description and check its type */
    clrMethod = (CLRMethod*) rhManager->getRuntimeMethodHandleValue( rhManager, handle);
    switch (clrMethod->type) {

            /* We must build a System.Reflection.ClrMethod */
        case CLRMETHOD:
            object = rManager->getMethod(clrMethod->ID);
            break;

            /* We must build a System.Reflection.ClrConstructor */
        case CLRCONSTRUCTOR:
            clrConstructor = (CLRConstructor*) clrMethod;
            object = rManager->getConstructor(clrConstructor->ID);
            break;

            /* Unknown description found */
        default:
            print_err("System.Reflection."
                      "MethodBase.GetMethodFromHandle: "
                      "given handle doesn't store a pointer "
                      "to a method or constructor description", 0);
            abort();
    }

    METHOD_END(ildjitSystem,
               "System.Reflection.MethodBase.GetMethodFromHandle");

    return object;

}

void * System_Reflection_MethodBase_GetCurrentMethod (void) {
    ir_method_t *irMethod;
    Method method;

    METHOD_BEGIN(ildjitSystem, "System.Reflection.MethodBase.GetCurrentMethod");

    irMethod = IRVM_getRunningMethod(&(ildjitSystem->IRVM));
    if (irMethod == NULL) {
        return NULL;
    }
    method = (Method) (JITNUINT) IRMETHOD_getMethodID(irMethod);
    assert(method != NULL);

    MethodDescriptor *methodID = method->getID(method);
    void *object = ((ildjitSystem->cliManager).CLR.reflectManager).getMethod(methodID);

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Reflection.MethodBase.GetCurrentMethod");

    return object;
}

//***************************************************************************************************************/
//*							_TYPE							*/
//***************************************************************************************************************/
void * System_Type_GetType (void *nameString, JITBOOLEAN throwOnError, JITBOOLEAN ignoreCase) { //già fatto
    JITINT16        *literal;
    JITINT8         *literalUTF8;
    JITINT8         *typeName;
    JITINT8         *typeNameSpace;
    JITINT32 length;
    TypeDescriptor  *ilType;
    void            *objType;

    /* Assertions			*/
    assert(nameString != NULL);
    METHOD_BEGIN(ildjitSystem, "System.Type.GetType");

    objType = NULL;
    length = (ildjitSystem->cliManager).CLR.stringManager.getLength(nameString);
    assert(length > 0);
    literal = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(nameString);
    assert(literal != NULL);
    literalUTF8 = allocMemory(length + 1);
    assert(literalUTF8 != NULL);
    (ildjitSystem->cliManager).CLR.stringManager.fromUTF16ToUTF8(literal, literalUTF8, length);
    typeName        = fetchLastName(literalUTF8);
    typeNameSpace   = literalUTF8;
    (*(literalUTF8 + (length - STRLEN(typeName) - 1)))      = '\0';
    ilType = (ildjitSystem->cliManager).metadataManager->getTypeDescriptorFromName((ildjitSystem->cliManager).metadataManager, typeNameSpace, typeName);
    if (ilType != NULL) {
        objType = ((ildjitSystem->cliManager).CLR.reflectManager).getType(ilType);
        assert(objType != NULL);
    }

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Type.GetType");
    return objType;
}

//**************************************************************************************************
void * System_Type_GetTypeFromHandle (void *handle) {
    TypeDescriptor  	*ilType;
    void   	         	*object;
    CLR_t  	         	*clr;
    RuntimeHandleManager	*rhManager;
    CLRType			*clrType;

    /* Assertions.
     */
    assert(handle != NULL);
    METHOD_BEGIN(ildjitSystem, "System.Type.GetTypeFromHandle");

    /* Initialize the variables.
     */
    object = NULL;

    /* Cache some pointers.
     */
    clr 		= &((ildjitSystem->cliManager).CLR);
    rhManager 	= &(clr->runtimeHandleManager);

    /* Fetch the internal representation of the CIL type (i.e., CLRType *).
     */
    clrType		= rhManager->getRuntimeTypeHandleValue(rhManager, handle);
    assert(clrType != NULL);
    ilType		= clrType->ID;
    assert(ilType != NULL);

    /* Fetch the instance of System.Type    *
     * of the class of the object given as  *
     * incoming argument			*/
    object = (clr->reflectManager).getType(ilType);
    assert(object != NULL);

    /* Return the System.Type of the object	*/
    METHOD_END(ildjitSystem, "System.Type.GetTypeFromHandle");
    return object;
}

static inline void * internal_GetExecutingAssembly (void) {
    void                    *object;
    t_binary_information    *caller_binary;
    Method method;
    BasicAssembly   *assembly;
    ir_method_t *irMethod;

    /* Assertions				*/
    assert(ildjitSystem != NULL);

    /* Fetch the running method		*/
    irMethod = IRVM_getRunningMethod(&(ildjitSystem->IRVM));
    if (irMethod == NULL) {
        return NULL;
    }
    method = (Method) (JITNUINT) IRMETHOD_getMethodID(irMethod);
    assert(method != NULL);
    PDEBUG("RUNTIME: Internal call: internalGetExecutingAssembly:   Caller method   = %s\n", method->getName(method));

    /* Fetch the binary of the method			*/
    caller_binary = method->getBinary(method);
    assert(caller_binary != NULL);
    PDEBUG("RUNTIME: Internal call: internalGetExecutingAssembly:   Caller binary   = %s\n", caller_binary->name);

    /* Search the assembly that contains the method metadata*/
    assembly = method->getAssembly(method);
    assert(assembly != NULL);

#ifdef PRINTDEBUG
    char* name = get_string(&((caller_binary->metadata).streams_metadata.string_stream), assembly->name);
    assert(name != NULL);
    PDEBUG("RUNTIME: Internal call: internalGetExecutingAssembly:   Caller assembly	= %s\n", name);
#endif


    /* Fetch the instance of System.Reflection.Assembly of  *
     * the callerBinary					*/
    object = ((ildjitSystem->cliManager).CLR.reflectManager).getAssembly(assembly);
    assert(object != NULL);

    /* Return the System.Type of the object	*/
    return object;
}

static inline BasicAssembly *getAssemblyFromName (JITINT8 *name) {
    BasicAssembly   *assembly;

    /* Assertions			*/
    assert(name != NULL);
    PDEBUG("GENERAL_TOOLS: getAssemblyFromName: Start\n");

    assembly = (ildjitSystem->cliManager).metadataManager->getAssemblyFromName((ildjitSystem->cliManager).metadataManager, name);
    /* Check if we have found the assembly	*/
    if (assembly == NULL) {
        PDEBUG("GENERAL_TOOLS: getAssemblyFromName:     The assembly %s is not already loaded\n", name);
        /* Load the assembly		*/
        PDEBUG("GENERAL_TOOLS: getAssemblyFromName:     Load the assembly %s\n", name);
        t_binary_information *current_binary = ILDJIT_loadAssembly(name);
        assert(STRCMP(current_binary->name, name) == 0);
        if (current_binary != NULL) {
            assembly = (ildjitSystem->cliManager).metadataManager->getAssemblyFromName((ildjitSystem->cliManager).metadataManager, name);
        } else {
            PDEBUG("GENERAL_TOOLS: getAssemblyFromName:             Cannot load the assembly %s\n", name);
        }
    }

    /* Return				*/
    PDEBUG("GENERAL_TOOLS: getAssemblyFromName: Exit\n");
    return assembly;
}
