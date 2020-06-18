/*
 * Copyright (C) 2008-2010  Campanoni Simone
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
#include <stdio.h>
#include <platform_API.h>
#include <ildjit.h>
#include <static_memory.h>

// My headers
#include <clr_system.h>
#include <internal_calls_runtime.h>
#include <internal_calls_utilities.h>
// End

extern t_system *ildjitSystem;

void System_Runtime_CompilerServices_RuntimeHelpers_RunClassConstructor (void *classHandle) {
    RuntimeHandleManager	*rhManager;
    CLRType			*clrType;
    TypeDescriptor		*metadataType;
    StaticMemoryManager	*staticMemoryManager;

    /* Assertions.
     */
    assert(classHandle != NULL);
    METHOD_BEGIN(ildjitSystem, "System.Runtime.CompilerServices.RuntimeHelpers.RunClassConstructor");

    /* Cache some managers.
     */
    rhManager	 	= &((ildjitSystem->cliManager).CLR.runtimeHandleManager);
    staticMemoryManager	= &(ildjitSystem->staticMemoryManager);

    /* Fetch the class.
     */
    clrType		= rhManager->getRuntimeTypeHandleValue(rhManager, classHandle);
    assert(clrType != NULL);
    metadataType	= clrType->ID;
    assert(metadataType != NULL);

    /* Fetch the cctor.
     */
    if (metadataType->getCctor != NULL) {
        MethodDescriptor	*cctorID;
        Method			cctor;
        cctorID		= metadataType->getCctor(metadataType);
        assert(cctorID != NULL);
        cctor		= (ildjitSystem->cliManager).fetchOrCreateMethod(cctorID);
        assert(cctor != NULL);

        /* Execute the cctor.
         */
        STATICMEMORY_callCctorMethod(staticMemoryManager, cctor);
    }

    METHOD_END(ildjitSystem, "System.Runtime.CompilerServices.RuntimeHelpers.RunClassConstructor");
    return ;
}

/* Initialize the array array */
void System_Runtime_CompilerServices_RuntimeHelpers_InitializeArray ( void* array, void* runtimeHandler) {
    RuntimeHandleManager* rhManager;
    FieldDescriptor *fieldID;

    rhManager = &((ildjitSystem->cliManager).CLR.runtimeHandleManager);
    METHOD_BEGIN(ildjitSystem, "System.Runtime.CompilerServices.RuntimeHelpers.InitializeArray");

    fieldID = (FieldDescriptor *) rhManager->getRuntimeFieldHandleValue(rhManager, runtimeHandler);
    System_Runtime_CompilerServices_RuntimeHelpers_InitializeArray_ap(array, (JITNINT) fieldID);

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Runtime.CompilerServices.RuntimeHelpers.InitializeArray");
    return;
}

/* Initialize the given array */
void System_Runtime_CompilerServices_RuntimeHelpers_InitializeArray_ap (void* array, JITNINT fieldDescription) {
    t_binary_information    *binary;
    JITUINT32 fieldRVA;
    JITUINT32 arraySlotSize;
    JITUINT32 arrayLength;
    JITUINT32 entryPointAddress;
    JITUINT32 arrayIndex;
    FieldDescriptor         *fieldID;

    /* Assertions			*/
    assert(array != NULL);
    METHOD_BEGIN(ildjitSystem, "System.Runtime.CompilerServices.RuntimeHelpers.InitializeArray");

    /* Initialize the variables	*/
    fieldID = NULL;
    binary = NULL;
    fieldRVA = 0;
    entryPointAddress = 0;
    arrayLength = 0;
    arraySlotSize = 0;

    /* Initialize the field infos */
    fieldID = (FieldDescriptor *) fieldDescription;
    assert(fieldID != NULL);

    /* Fetch the binary		*/
    binary = (t_binary_information *) fieldID->row->binary;
    assert(binary != NULL);

    if (fieldID->attributes->has_rva) {

        /* The field has a RVA, so I	*
         * have to initialize the array	*
         * with the method fetchable by	*
         * this RVA			*/
        JITINT8         *buffer;
        JITUINT32 streamOffset;

        /* Initialize the variables     */
        streamOffset = 0;
        buffer = NULL;

        /* Fetch the RVA of the field	*/
        fieldRVA = fieldID->attributes->rva;
        assert(fieldRVA != 0);
        PDEBUG("Internal call: \"System.Runtime.CompilerServices.RuntimeHelpers.InitializeArray\":      Binary  = %s\n", ((t_binary_information *) fieldID->row->binary)->name);
        PDEBUG("Internal call: \"System.Runtime.CompilerServices.RuntimeHelpers.InitializeArray\":      RVA     = %d\n", fieldID->attributes->rva);

        /* Fetch the entry point in the	*
         * file				*/
        entryPointAddress = convert_virtualaddress_to_realaddress(binary->sections, fieldRVA);
        assert(entryPointAddress != 0);
        PDEBUG("Internal call: \"System.Runtime.CompilerServices.RuntimeHelpers.InitializeArray\":      Address = %d\n", entryPointAddress);

        /* Lock the binary		*/
        PLATFORM_lockMutex(&((binary->binary).mutex));

        /* Point the file in the entry	*
         * point address		*/
        if (entryPointAddress < (*(binary->binary.reader))->offset) {
            if (unroll_file(&(binary->binary)) != 0) {
                print_err("Internal call: \"System.Runtime.CompilerServices.RuntimeHelpers.InitializeArray\": ERROR= Cannot unroll the file. ", 0);
                abort();
            }
            assert((*(binary->binary.reader))->offset == 0);
        }
        assert(entryPointAddress >= (*(binary->binary.reader))->offset);

        /* Allocate the buffer          */
        buffer = allocMemory(entryPointAddress);

        /* Seek the file to the begin   *
        * of the data                  */
        assert(buffer != NULL);
        if (seek_within_file(&(binary->binary), buffer, (*(binary->binary.reader))->offset, entryPointAddress) != 0) {
            print_err("Internal call: \"System.Runtime.CompilerServices.RuntimeHelpers.InitializeArray\": ERROR= Cannot seek on the file. ", 0);
            abort();
        }

        /* Fetch the size of each singe	*
         * element of the array		*/
        arraySlotSize = (ildjitSystem->cliManager).CLR.arrayManager.getArrayElementSize(array);
        assert(arraySlotSize > 0);

        /* Fetch the length of the array*/
        arrayLength = (ildjitSystem->garbage_collectors).gc.getArrayLength(array);
        assert(arrayLength > 0);

        /* Reallocate the buffer          */
        buffer = reallocMemory(buffer, sizeof(char) * (get_metadata_start_point(&(binary->binary)) + (arraySlotSize * arrayLength)));

        /* Seek to the begin of the     *
         * metadata			*/
        assert(buffer != NULL);
        if (get_metadata_start_point(&(binary->binary)) > 512) {
            il_read(buffer, get_metadata_start_point(&(binary->binary)), &(binary->binary));
        }

        /* Read the values              */
        assert(buffer != NULL);
        il_read(buffer, arraySlotSize * arrayLength, &(binary->binary));

        /* Initialize the array		*/
        for (arrayIndex = 0; arrayIndex < arrayLength; arrayIndex++) {

            /* Store the value into the	*
             * array element		*/
            assert(array != NULL);
            (ildjitSystem->cliManager).CLR.arrayManager.setValueArrayElement(array, arrayIndex, buffer + streamOffset);
            streamOffset += arraySlotSize;
        }

        /* Unlock the binary			*/
        PLATFORM_unlockMutex(&((binary->binary).mutex));

        /* Free the memory                      */
        freeMemory(buffer);
    }

    /* Return		*/
    METHOD_END(ildjitSystem, "System.Runtime.CompilerServices.RuntimeHelpers.InitializeArray");
    return;
}

JITINT32 System_Runtime_InteropServices_Marshal_SizeOf (void *t) {
    CLRType *clrType;
    JITINT32 typeSize;
    IRVM_type *jit_type;
    ILLayout	*objectLayout;

    METHOD_BEGIN(ildjitSystem, "System.Runtime.InteropServices.Marshal.SizeOfInternal");

    /* The input object is an instance of the CIL class Type.
     * Fetch the internal representation of the equivalent CLR type.
     */
    clrType 	= ((ildjitSystem->cliManager).CLR.reflectManager).getPrivateDataFromType(t);
    assert(clrType != NULL);

    /* Fetch the layout of an object, which is an instance of this CIL type.
     */
    objectLayout	= (ildjitSystem->cliManager).layoutManager.layoutType(&(ildjitSystem->cliManager).layoutManager, clrType->ID);
    assert(objectLayout != NULL);
    assert(objectLayout->jit_type != NULL);

    /* Fetch the size of these instances.
     */
    jit_type 	= *(objectLayout->jit_type);
    if (jit_type != NULL) {
        typeSize 	= IRVM_getIRVMTypeSize(&(ildjitSystem->IRVM), jit_type);
    } else {
        typeSize	= IRDATA_getSizeOfIRType(clrType->ID->getIRType(clrType->ID));
    }

    METHOD_END(ildjitSystem, "System.Runtime.InteropServices.Marshal.SizeOfInternal");
    return typeSize;
}

void * System_Runtime_InteropServices_Marshal_PtrToStringAnsiInternal (JITNINT ptr, JITINT32 len) {
    void            *object;

    METHOD_BEGIN(ildjitSystem, "System.Runtime.InteropServices.Marshal.PtrToStringAnsiInternal");

    object = NULL;

    if ((void *) ptr != NULL) {
        if (len < 0) {
            len = STRLEN(ptr);
        }
        object = CLIMANAGER_StringManager_newInstanceFromUTF8((JITINT8 *) ptr, len);
    }

    /* Return                       */
    METHOD_END(ildjitSystem, "System.Runtime.InteropServices.Marshal.PtrToStringAnsiInternal");
    return object;
}

void System_Runtime_InteropServices_Marshal_FreeHGlobal (JITNINT hglobal) {

    /* Assertions			*/
    assert(hglobal != 0);
    METHOD_BEGIN(ildjitSystem, "System.Runtime.InteropServices.Marshal.FreeHGlobal");

    /* Free the memory		*/
    freeMemory((void *) hglobal);

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Runtime.InteropServices.Marshal.FreeHGlobal");
    return;
}

void * System_Runtime_InteropServices_Marshal_ReAllocHGlobal (void *pv, JITNINT cb) {

    METHOD_BEGIN(ildjitSystem, "System.Runtime.InteropServices.Marshal.ReAllocHGlobal");

    pv = reallocMemory(pv, cb);
    if (!pv) {
        ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "OutOfMemoryException");
        abort();
    }
    assert(pv != NULL);

    METHOD_END(ildjitSystem, "System.Runtime.InteropServices.Marshal.ReAllocHGlobal");
    return pv;
}

void * System_Runtime_InteropServices_Marshal_AllocHGlobal (JITNINT cb) {
    void            *ptr;

    /* Check corner cases		*/
    METHOD_BEGIN(ildjitSystem, "System.Runtime.InteropServices.Marshal.AllocHGlobal");
    if (cb == 0) {

        /* This returns a valid pointer for size 0 on MS.NET */
        cb = 4;
    }

    /* Allocate the memory		*/
    ptr = (void *) allocMemory((unsigned) cb);
    if (!ptr) {
        ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "OutOfMemoryException");
        abort();
    }
    assert(ptr != NULL);

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Runtime.InteropServices.Marshal.AllocHGlobal");
    return ptr;
}

void System_Runtime_InteropServices_Marshal_WriteIntPtr (JITNINT ptr, JITINT32 ofs, JITNINT val) {
    void	*destination;

    /* Assertions                   	*/
    assert((void *) ptr != NULL);
    METHOD_BEGIN(ildjitSystem, "System.Runtime.InteropServices.Marshal.WriteIntPtr");

    /* Compute the destination address	*/
    destination	= (void *)(ptr + ofs);

    /* Copy the value to the memory		*/
    memcpy(destination, &val, sizeof(JITNINT));

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Runtime.InteropServices.Marshal.WriteIntPtr");
    return;
}

void * System_Runtime_InteropServices_Marshal_StringToHGlobalAnsi (void *string) {
    JITINT16        *literal;
    JITINT8         *literalString;
    JITINT32 stringLength;

    METHOD_BEGIN(ildjitSystem, "System.Runtime.InteropServices.Marshal.StringToHGlobalAnsi");

    /* Initialize the variables	*/
    literalString = NULL;

    /* Fetch the literal            */
    literal = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(string);
    assert(literal != NULL);
    stringLength = (ildjitSystem->cliManager).CLR.stringManager.getLength(string);
    assert(stringLength >= 0);
    if (stringLength > 0) {
        literalString = (JITINT8 *) allocMemory(stringLength + 1);
        (ildjitSystem->cliManager).CLR.stringManager.fromUTF16ToUTF8(literal, literalString, stringLength);
    }

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Runtime.InteropServices.Marshal.StringToHGlobalAnsi");
    return literalString;
}

/*
 * Get the offset from the start of a String object to the start of the first
 * character
 */
JITINT32 System_Runtime_CompilerServices_RuntimeHelpers_get_OffsetToStringData (void) {
    t_stringManager* stringManager;
    JITINT32 offset;

    METHOD_BEGIN(ildjitSystem, "System.Runtime.CompilerServices.RuntimeHelpers." "get_OffsetToStringData");
    stringManager = &((ildjitSystem->cliManager).CLR.stringManager);

    offset = stringManager->getFirstCharOffset();

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Runtime.CompilerServices.RuntimeHelpers." "get_OffsetToStringData");
    return offset;
}
