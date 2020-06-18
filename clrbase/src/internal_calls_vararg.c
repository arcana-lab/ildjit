/*
 * Copyright (C) 2009 - 2012  Campanoni Simone
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
#include <jitsystem.h>
#include <ildjit.h>

// My headers
#include <internal_calls_vararg.h>
#include <internal_calls_utilities.h>
#include <internal_calls_reflection.h>
#include <internal_calls_object.h>
#include <internal_calls_array.h>
// End

extern t_system *ildjitSystem;

void System_ArgIterator_End (void *_this) {
    JITINT32 k;

    METHOD_BEGIN(ildjitSystem, "System.ArgIterator.End");

    if ((ildjitSystem->cliManager).CLR.varargManager.getargsField(_this)) {
        k = (ildjitSystem->cliManager).CLR.arrayManager.getArrayLength((ildjitSystem->cliManager).CLR.varargManager.getargsField(_this), 0);
        (ildjitSystem->cliManager).CLR.varargManager.setposnField(_this, k);
    }


    /* Return			*/
    METHOD_END(ildjitSystem, "System.ArgIterator.End");
    return;
}


JITINT32 System_ArgIterator_GetRemainingCount (void *_this) { //OK
    JITINT32 ris;

    METHOD_BEGIN(ildjitSystem, "System.ArgIterator.GetRemainingCount");

    if ((ildjitSystem->cliManager).CLR.varargManager.getargsField(_this)) {
        ris = (ildjitSystem->cliManager).CLR.arrayManager.getArrayLength((ildjitSystem->cliManager).CLR.varargManager.getargsField(_this), 0) - (ildjitSystem->cliManager).CLR.varargManager.getposnField(_this);
    } else {
        ris = 0;
    }

    /* Return			*/
    METHOD_END(ildjitSystem, "System.ArgIterator.End");
    return ris;
}

/*
 * System.ArgIterator.ArgIterator(RuntimeArgumentHandle argList);
 */
void System_ArgIterator_ctor_RuntimeArgumentHandle (void *_this, void *argList) {
    METHOD_BEGIN(ildjitSystem, "System.ArgIterator.ctor_RuntimeArgumentHandle");

    (ildjitSystem->cliManager).CLR.varargManager.create_arg_iterator(_this, argList, 0);

    /* Return			*/
    METHOD_END(ildjitSystem, "System.ArgIterator.ctor_RuntimeArgumentHandle");
    return;
}

void System_ArgIterator_ctor_RuntimeArgumentHandlepV (void *_this, void *argList, void *ptr) { //OK

    METHOD_BEGIN(ildjitSystem, "System.ArgIterator.ctor_RuntimeArgumentHandlepV");

    (ildjitSystem->cliManager).CLR.varargManager.create_arg_iterator(_this, argList, 0);

    METHOD_END(ildjitSystem, "System.ArgIterator.ctor_RuntimeArgumentHandlepV");
    return;
}

void System_ArgIterator_GetNextArg (void *_this, void *result) {
    void            * args;         /**< The array object containing the vararg parameters */
    JITINT32 	posn;           /**< The first parameter to read */
    void            **object;       /**< An element extracted from the array */
    t_arrayManager  *arrayManager;
    t_varargManager *varargManager;
    JITINT32	arrayLength;

    /* Assertions			*/
    assert(_this != NULL);
    assert(result != NULL);

    /* Trace the method		*/
    METHOD_BEGIN(ildjitSystem, "System.ArgIterator.GetNextArg");

    /* Fetch the array manager	*/
    arrayManager = &((ildjitSystem->cliManager).CLR.arrayManager);

    /* Fetch the vararg manager	*/
    varargManager = &((ildjitSystem->cliManager).CLR.varargManager);

    /* Extract information from the ArgIterator	*/
    args = varargManager->getargsField(_this);
    posn = varargManager->getposnField(_this);

    /* Fetch the length of the array		*/
    arrayLength	= arrayManager->getArrayLength(args, 0);

    if (args  &&  posn < arrayLength) {
        object = ((void **) arrayManager->getArrayElement(args, posn));
        assert(object != NULL);
        memcpy(result, object, arrayManager->getArrayElementSize(args));
        posn++;
        varargManager->setposnField(_this, posn);

    } else {

        /* We've reached the end of the argument list */
        ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "InvalidOperationException");
        abort();
    }

    /* Return			*/
    METHOD_END(ildjitSystem, "System.ArgIterator.GetNextArg");
    return;
}

void System_ArgIterator_GetNextArgType (void *_this, void *res) {
    void                    *object;
    void                    *objtype;
    CLIManager_t		*cliManager;
    CLR_t			*clr;
    void			*argsField;
    JITINT32		posnField;

    /* Assertions.
     */
    assert(_this != NULL);
    assert(res != NULL);
    METHOD_BEGIN(ildjitSystem, "System.ArgIterator.GetNextArgType");

    /* Cache some pointers.
     */
    cliManager	= &(ildjitSystem->cliManager);
    clr 		= &(cliManager->CLR);

    /* Fetch the fields of the ArgIteration.
     */
    argsField	= (clr->varargManager).getargsField(_this);
    posnField	= (clr->varargManager).getposnField(_this);

    if (	(argsField != NULL)						&&
            (posnField < (clr->arrayManager).getArrayLength(argsField, 0))	) {
        CLRType		*clrType;

        /* Extract the next object, which is an instance of the CIL class TypedReference.
         */
        object 	= (void **) (clr->arrayManager).getArrayElement(argsField, posnField);
        assert(object != NULL);

        /* Object is a pointer to an instance of the CIL data structure TypedReference.
         * Fetch the type field of this data structure, which is an instance of the CIL class RuntimeTypeHandle.
         */
        objtype	= *(void **)(cliManager->CLR).typedReferenceManager.getType(object);
        assert(objtype != NULL);

        /* objtype is an instance of the CIL class RuntimeTypeHandle.
         * Fetch the internal representation of the CIL type.
         */
        clrType		= (clr->runtimeHandleManager).getRuntimeTypeHandleValue(&(clr->runtimeHandleManager), objtype);
        assert(clrType != NULL);

        /* Store the internal representation of the CIL type to the output, which is an instance of the CIL type RuntimeTypeHandle.
         */
        (clr->runtimeHandleManager).setRuntimeTypeHandleValue(&(clr->runtimeHandleManager), res, clrType);
        assert((clr->runtimeHandleManager).getRuntimeTypeHandleValue(&(clr->runtimeHandleManager), res) == clrType);

    } else {

        /* We've reached the end of the argument list */
        ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "InvalidOperationException");
        abort();
    }

    METHOD_END(ildjitSystem, "System.ArgIterator.GetNextArgType");
    return ;
}

void System_ArgIterator_GetNextArg_RuntimeTypeHandle (void *_this, void *type, void *result) {

    METHOD_BEGIN(ildjitSystem, "System.ArgIterator.GetNextArg");

    System_ArgIterator_GetNextArg(_this, result);

    METHOD_END(ildjitSystem, "System.ArgIterator.GetNextArg");
    return;
}
