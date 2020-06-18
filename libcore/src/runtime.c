/*
 * Copyright (C) 2006 - 2009  Campanoni Simone
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
#include <stdlib.h>
#include <assert.h>
#include <string.h>

// My headers
#include <runtime.h>
#include <translation_pipeline.h>
#include <general_tools.h>
#include <iljit.h>
#include <cil_ir_translator.h>
#include <static_memory.h>
// End

extern t_system *ildjitSystem;

void RUNTIME_standaloneBinaryIntializeSystem (JITINT8 *programName) {

    /* Allocate the system data structure.
     */
    allocateAndInitializeSystem(JITFALSE, JITFALSE, JITFALSE, 0, NULL, JITFALSE, JITFALSE);
    assert(ildjitSystem != NULL);

    /* Set the name of the program.
     */
    ildjitSystem->programName	= strdup((char *)programName);
    assert(ildjitSystem->programName != NULL);

    /* Disable the generation of virtual tables.
     * This has been already done and dumped in the standalone binary.
     */
    LAYOUT_setCachingRequestsForVirtualMethodTables(&((ildjitSystem->cliManager).layoutManager), JITTRUE);

    return ;
}

void RUNTIME_standaloneBinaryShutdownSystem (void) {
    freeFunction(ildjitSystem);

    return ;
}

void * RUNTIME_setArguments (int argc, char *argv[]) {
    TypeDescriptor          *stringClassLocated;
    JITUINT32               cilArgc;
    JITUINT32               count;
    JITINT8                 **cilArgv;
    void                    *array;

    /* Assertions.
     */
    assert(argc > 0);
    assert(argv != NULL);

    /* Set the program arguments.
     */
    (ildjitSystem->arg).argc	= argc;
    (ildjitSystem->arg).argv	= argv;

    /* Set the CIL program arguments.
     */
    cilArgc                     = argc - 1;
    cilArgv                     = (JITINT8 **)&(argv[1]);

    /* Allocate the CIL array input.
     */
    stringClassLocated          = (ildjitSystem->cliManager).CLR.stringManager.fillILStringType();
    assert(stringClassLocated != NULL);
    array                       = (ildjitSystem->garbage_collectors).gc.allocArray(stringClassLocated, 1, cilArgc);
    assert(array != NULL);
    assert((ildjitSystem->garbage_collectors).gc.getArrayLength(array) == cilArgc);

    /* Create the CIL program arguments.
     */
    for (count = 0; count < cilArgc; count++) {
        void            *arg_string;
        arg_string = CLIMANAGER_StringManager_newInstanceFromUTF8(cilArgv[count], strlen((char *) cilArgv[count]));
        (ildjitSystem->cliManager).CLR.arrayManager.setValueArrayElement(array, count, &arg_string);
    }

    return array;
}

JITINT32 RUNTIME_fromSymbolIDToInteger (JITUINT32 sID) {
    return (JITINT32) (JITNUINT)RUNTIME_fromSymbolIDToPointer(sID);
}

JITINT32 RUNTIME_fromSerializedSymbolToInteger (JITUINT32 tag, void *mem, JITUINT32 memBytes) {
    return (JITINT32) (JITNUINT) RUNTIME_fromSerializedSymbolToPointer(tag, mem, memBytes);
}

void * RUNTIME_fromSerializedSymbolToPointer (JITUINT32 tag, void *mem, JITUINT32 memBytes) {
    ir_symbol_t 	*sym;
    ir_value_t	v;
    void		*p;

    /* Fetch the symbol.
     */
    sym	= IRSYMBOL_deserializeSymbol(tag, mem, memBytes);
    assert(sym != NULL);

    /* Resolve the symbol.
     */
    v	= IRSYMBOL_resolveSymbol(sym);

    /* Fetch the pointer.
     */
    p	= (void *)(JITNUINT)v.v;

    return p;
}

void * RUNTIME_fromSymbolIDToPointer (JITUINT32 sID) {
    ir_symbol_t	*s;
    ir_value_t	v;
    void		*p;

    s	= IRSYMBOL_loadSymbol(sID);
    assert(s != NULL);

    /* Resolve the symbol.
     */
    v	= IRSYMBOL_resolveSymbol(s);

    /* Fetch the pointer.
     */
    p	= (void *)(JITNUINT)v.v;

    return p;
}

/* Function that is used to find the correct constructor. For example, in case of
 * a DivideByZero exception there are 3 possible .ctor methods to invoke. We are interested
 * in choose the right constructor. According to the fact that this is the first exception
 * thrown in the system, we have to call a different constructor. */

/* while we are trying to create a CIL exception we assume that the constructor always exists
 * and that the .ctor Class is NOT static. REMIND THAT THIS COULD NOT BE TRUE IN OTHER CASES.
 * Once we have located the right constructor, we shall compile it (if necessary) and make it
 * visible to the code as an indirect pointer to v-table (see the libjit documentation).
 *
 * So, the void-pointer returned by this function is the pointer to the .ctor function to be invoked
 * via indirect call. The body of this function is very similar to the body of `virtual_jump`
 * in ir_virtual_machine.
 * NOTE: THIS FUNCTION IS INTENDED TO USE ONLY INTO A CATCHER BLOCK (SEE LIBJIT DOCUMENTATION).
 * AND IT'S PRIMARY GOAL CONSISTS IN INITIALIZE THE CIL EXCEPTION OBJECTS CREATED WITHIN A LIBJIT HANDLER)
 */
void * fetchTheRightConstructor (void * object_created) {
    TypeDescriptor          *type_of_the_object;
    Method ctor;
    MethodDescriptor        *ctorID;

    PDEBUG("RUNTIME: fetchTheRightConstructor: Start\n");

    /* ASSERTIONS : object_created & system must not be NULL pointers */
    assert(object_created != NULL);

    /* RETRIEVE THE binary INFO & THE classID INFO QUERYING THE GC */
    PDEBUG("RUNTIME: fetchTheRightConstructor:      Fetch the metadata information about the exception type\n");
    type_of_the_object = (ildjitSystem->garbage_collectors).gc.getType(object_created);
    assert(type_of_the_object != NULL);

    PDEBUG("RUNTIME: fetchTheRightConstructor:      Search the ctor method ID\n");
    ctorID = retrieve_CILException_Ctor_MethodID_by_type(&((ildjitSystem->cliManager).methods), type_of_the_object);
    assert(ctorID != NULL);

    /* Finally, we have to find the jit_function for the desired constructor. */
    PDEBUG("RUNTIME: fetchTheRightConstructor:      Fetch the ctor method address\n");
    ctor = (ildjitSystem->staticMemoryManager).fetchStaticConstructor(&(ildjitSystem->staticMemoryManager), ctorID);
    assert(ctor != NULL);

    /* Return							*/
    PDEBUG("RUNTIME: fetchTheRightConstructor: Exit\n");
    return ctor->getFunctionPointer(ctor);
}

JITBOOLEAN runtime_canCast (void *object, TypeDescriptor *desired_type) {
    TypeDescriptor          *ownerType;
    JITNUINT result;

    /* assertions */
    assert(object != NULL);
    assert(desired_type != NULL);

    /* initialize the local variable `ownerType` */
    ownerType = (ildjitSystem->garbage_collectors).gc.getType(object);
    assert(ownerType != NULL);

    /* check assignbility' */
    ownerType->lock(ownerType);
    result = ownerType->assignableTo(ownerType, desired_type);
    ownerType->unlock(ownerType);

    /* Return				*/
    return result;
}

void * runtimeBoxNullValueType (t_system *system, TypeDescriptor *valueTypeID) {
    ILLayout *vt_layout;
    void    *boxedVT;

    /* assertions */
    assert(system != NULL);
    assert(valueTypeID != NULL);

    /* retrieve the layout informations about the requested valuetype */
    vt_layout = (ildjitSystem->cliManager).layoutManager.layoutType(&((ildjitSystem->cliManager).layoutManager), valueTypeID);
    assert(vt_layout != NULL);

    /* allocate memory for the requested instance of value type */
    boxedVT = (ildjitSystem->garbage_collectors).gc.allocObject(valueTypeID, 0);

    /* set to zero the just allocated memory block */
    memset(boxedVT, 0, vt_layout->typeSize);

    /* return the boxed valueType */
    return boxedVT;
}

void * runtimeBoxNullPrimitiveType (t_system *system, JITUINT32 IRType) {
    TypeDescriptor  *primitiveType;

    /* assertions */
    assert(system != NULL);
    assert((ildjitSystem->cliManager).entryPoint.binary != NULL);

    /* retrieve the primitive type */
    primitiveType = (ildjitSystem->cliManager).metadataManager->getTypeDescriptorFromIRType((ildjitSystem->cliManager).metadataManager, IRType);

    /* test the postconditions */
    assert(primitiveType != NULL);

    /* call the runtimeBoxNullValueType */
    return runtimeBoxNullValueType(system, primitiveType);
}

void tracer_startMethod (ir_method_t *method) {
    JITINT8 *methodName;

    /* Assertions                           */
    assert(method != NULL);

    /* Get the name of the method		*/
    IRMETHOD_lock(method);
    methodName = IRMETHOD_getSignatureInString(method);
    if (methodName == NULL) {
        methodName	= IRMETHOD_getMethodName(method);
    }
    IRMETHOD_unlock(method);

    /* Print the message			*/
    fprintf(stderr, "Thread %ld: Enter to the method %s\n", gettid(), methodName);

    return ;
}

void tracer_exitMethod (ir_method_t *method) {
    JITINT8 *methodName;

    /* Assertions                           */
    assert(method != NULL);

    /* Get the name of the method		*/
    IRMETHOD_lock(method);
    methodName = IRMETHOD_getSignatureInString(method);
    if (methodName == NULL) {
        methodName	= IRMETHOD_getMethodName(method);
    }
    IRMETHOD_unlock(method);

    /* Print the message			*/
    fprintf(stderr, "Thread %ld: Exit from the method %s\n", gettid(), methodName);

    return ;
}
