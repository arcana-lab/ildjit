/*
 * Copyright (C) 2008 - 2012  Campanoni Simone
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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <ildjit.h>

// My headers
#include <clr_system.h>
#include <internal_calls_utilities.h>
// End

extern t_system *ildjitSystem;

MethodDescriptor *ILMethodSemGetByType (CLRMember *clrMember, JITINT32 type) {
    MethodDescriptor                                *method;

    /* Fetch the semantic methods	*
     * list				*/
    if (clrMember->type == CLRPROPERTY) {
        PropertyDescriptor *property = (PropertyDescriptor *) clrMember->ID;
        switch (type) {
            case 0x01:
                method = property->getSetter(property);
                break;
            case 0x02:
                method = property->getGetter(property);
                break;
            case 0x04:
                method = property->getOther(property);
                break;
            default:
                return NULL;
        }
    } else if (clrMember->type == CLREVENT) {
        EventDescriptor *event = (EventDescriptor *) clrMember->ID;
        switch (type) {
            case 0x08:
                method = event->getAddOn(event);
                break;
            case 0x10:
                method = event->getRemoveOn(event);
                break;
            case 0x20:
                method = event->getFire(event);
                break;
            case 0x04:
                method = event->getOther(event);
                break;
            default:
                return NULL;
        }
    } else {
        return NULL;
    }
    return method;
}

void * InvokeMethod (Method method, void *_this, void *parametersArray) {
    JITUINT32 		numParameters;
    JITUINT32 		signNumParameters;
    ir_signature_t          *IRSignature;
    MethodDescriptor        *cil_method;
    JITUINT32 		count;
    JITINT32 		error;
    JITBOOLEAN 		hasThis;
    JITBOOLEAN 		is_primitive_result;
    void                    *exception_uncaught;
    void                    *result;
    void                    **args;
    void			*resultToReturn;

    /* Assertions						*/
    assert(method != NULL);
    PDEBUG("Internal call: internal_InvokeMethod: Start\n");

    /* Initialize the variables				*/
    IRSignature = NULL;
    exception_uncaught = NULL;
    args = NULL;
    result = NULL;
    is_primitive_result = 1;
    numParameters = 0;
    signNumParameters = 0;
    hasThis = 0;

    /* Compile the method		                        */
    (ildjitSystem->pipeliner).synchInsertMethod(&(ildjitSystem->pipeliner), method, MAX_METHOD_PRIORITY);
    assert(method->getJITFunction(method) != NULL);
    if (	(!(ildjitSystem->IRVM).behavior.aot) 		&&
            ((ildjitSystem->IRVM).behavior.pgc == 1)	) {
        MANFRED_appendMethodToProfile(&(ildjitSystem->manfred), method);
    }

    /* Fetch the CIL signature of the method to call	*/
    cil_method = method->getID(method);
    assert(cil_method != NULL);
    PDEBUG("Internal call: internal_InvokeMethod:   Method to call	= %s\n", cil_method->getSignatureInString(cil_method));

    /* Fetch the number of parameters			*/
    if (parametersArray != NULL) {
        numParameters = (ildjitSystem->cliManager).CLR.arrayManager.getArrayLength(parametersArray, 0);
    }

    /* Fetch the number of parameters declared inside the   *
     * signature of the method to call			*/
    signNumParameters = method->getParametersNumber(method);

    /* Consider the "this" parameter                        */
    if (!(cil_method->attributes->is_static)) {
        numParameters++;
        hasThis = JITTRUE;
    }

    /* Check that the parameters given in input are equal	*
     * to the parameters declared in the signature of the	*
     * method to call					*/
    if (numParameters != signNumParameters) {
        ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "ArgumentException");
        abort();
    }
    assert(numParameters == signNumParameters);

    /* Fetch the IR signature of the method to call		*/
    IRSignature = method->getSignature(method);
    assert(IRSignature != NULL);

    /* Make the arguments of the call			*/
    args = NULL;
    if (numParameters > 0) {
        PDEBUG("Internal call: internal_InvokeMethod: there are parameters\n");
        args = (void **) allocMemory(sizeof(void *) * numParameters);
        assert(args != NULL);
        memset(args, 0, sizeof(void *) * numParameters);
    }

    if (hasThis) {
        assert(_this != NULL);
        assert(numParameters > 0);
        assert(args != NULL);
        PDEBUG("Internal call: internal_InvokeMethod: the parameter 0 is THIS \n");
        args[0] = &(_this);
    }

    /* Make the parameters to give at the invocation of the	*
     * method to call				*/
    XanList *params = cil_method->getParams(cil_method);
    XanListItem *item = xanList_first(params);
    for (count = hasThis; count < numParameters; count++) {

        /* The type of the current parameter is a normal object	*/
        ParamDescriptor *currentParam;

        /* Retrieve the parameter informations			*/
        currentParam = (ParamDescriptor *) item->data;
        assert(currentParam != NULL);

        if (currentParam->type->isByRef) {
            /* The current parameter is a byref element	*/
            PDEBUG("Internal call: internal_InvokeMethod: The parameter #%d is byref\n", count);

            /* Initialize the current argument		*/
            args[count] = (void *) allocMemory(sizeof(void *));
            assert(args[count] != NULL);

            /* Test if the parameter is a null reference	*/
            if (*((void **) (parametersArray + (count - hasThis))) == NULL) {
                PDEBUG("Internal call: internal_InvokeMethod: the parameter #%d is a NULL reference\n", count - hasThis);

                /* Preconditions			*/
                assert(currentParam->type != NULL);

                (*((JITNUINT *) args[count])) = (JITNUINT) NULL;

                /* test if the current parameter is a valueType */
                if (!is_byReference_or_Object_IRType(IRSignature->parameterTypes[count]) ) {
                    if (IRSignature->parameterTypes[count] == IRVALUETYPE) {
                        TypeDescriptor  *value_type_infos;

                        /* retrieve the metadata infos associated with the current
                         * valueType */
                        TypeDescriptor *param_type = currentParam->type;
                        value_type_infos = param_type;
                        assert(value_type_infos != NULL);

                        /* box the valueType */
                        (*((JITNUINT *) args[count])) = (JITNUINT) ILDJIT_boxNullValueType(value_type_infos);
                        assert((void *) (*((JITNUINT *) args[count])) != NULL);

                    } else {
                        /* box the valueType */
                        (*((JITNUINT *) args[count])) = (JITNUINT) ILDJIT_boxNullPrimitiveType(IRSignature->parameterTypes[count]);
                        assert((void *) (*((JITNUINT *) args[count])) != NULL);
                    }
                }
            } else {
                PDEBUG("Internal call: internal_InvokeMethod: the parameter #%d is NOT a NULL reference\n", count - hasThis);
                (*((void **) args[count])) = (ildjitSystem->cliManager).CLR.arrayManager.getArrayElement(parametersArray, count - hasThis);
            }

        } else {
            PDEBUG("Internal call: internal_InvokeMethod: The parameter #%d is NOT byref \n", count - hasThis);

            /* Test if the parameter is a null reference */
            if ( (void *) (((JITNUINT *) parametersArray )[count - hasThis]) == NULL) {
                PDEBUG("Internal call: internal_InvokeMethod: the parameter #%d is a NULL reference\n", count - hasThis);

                /* preconditions */
                assert(currentParam->type != NULL);

                /* initialize the current argument */
                args[count] = NULL;

                if (!is_byReference_or_Object_IRType(IRSignature->parameterTypes[count]) ) {
                    if (IRSignature->parameterTypes[count] == IRVALUETYPE) {
                        TypeDescriptor  *value_type_infos;

                        /* retrieve the metadata infos associated with the current
                         * valueType */
                        TypeDescriptor *param_type = currentParam->type;
                        value_type_infos = param_type;
                        assert(value_type_infos != NULL);

                        /* box the valueType */
                        args[count] = ILDJIT_boxNullValueType(value_type_infos);
                        assert(args[count] != NULL);

                    } else {

                        /* box the valueType */
                        args[count] = ILDJIT_boxNullPrimitiveType(IRSignature->parameterTypes[count]);
                        assert(args[count] != NULL);
                    }
                }

            } else {
                PDEBUG("Internal call: internal_InvokeMethod: the parameter #%d is NOT a NULL reference\n", count - hasThis);
                args[count] = (ildjitSystem->cliManager).CLR.arrayManager.getArrayElement(parametersArray, count - hasThis);

                JITUINT32 current_param_IRType = currentParam->getIRType(currentParam);
                PDEBUG("Internal call: internal_InvokeMethod: the param IR type is %d\n", current_param_IRType);
                switch (current_param_IRType) {
                    case IROBJECT:
                        PDEBUG("Internal call: internal_InvokeMethod: nothing to be done here, the parameter is OK\n");
                        break;
                    default:
                        PDEBUG("Internal call: internal_InvokeMethod: the parameter requires a builtin type, dereferencing...\n");
                        args[count] = *((void **) (ildjitSystem->cliManager).CLR.arrayManager.getArrayElement(parametersArray, count - hasThis));
                        break;
                }
            }
        }
        item = item->next;
    }

    TypeDescriptor *result_type = cil_method->getResult(cil_method)->type;
    if (IRSignature->resultType == IRVALUETYPE) {
        PDEBUG("Internal call: internal_InvokeMethod: result type is VALUETYPE\n");
        TypeDescriptor  *value_type_infos;

        /* Retrieve the metadata infos associated with the current valueType	*/
        value_type_infos = result_type;
        assert(value_type_infos != NULL);

        /* Box the valueType							*/
        result = ILDJIT_boxNullValueType(value_type_infos);
        assert(result != NULL);

    } else {

        /* Allocate the result area					*/
        PDEBUG("Internal call: internal_InvokeMethod: making the result area\n");
        result = allocMemory(sizeof(JITUINT64));

        switch (IRSignature->resultType) {
            case IRINT8:
            case IRINT16:
            case IRINT32:
            case IRINT64:
            case IRNINT:
            case IRUINT8:
            case IRUINT16:
            case IRUINT32:
            case IRUINT64:
            case IRNUINT:
            case IRFLOAT32:
            case IRFLOAT64:
            case IRNFLOAT:
                break;
            case IROBJECT:
            case IRMPOINTER:
            case IRTPOINTER:
                is_primitive_result = 0;
                break;
            default:
                is_primitive_result = 0;
        }
    }

    /* Check if we need to execute the code.
     */
    if ((ildjitSystem->program).disableExecution) {
        return 0;
    }

    /* Call the method				*/
    error = IRVM_run(&(ildjitSystem->IRVM), *(method->jit_function), args, result);

    /* Check if an exception occur				*/
    if (error == 0) {
        /* An exception occurs				*/
        /* Take the exception occurs			*/
        exception_uncaught = ((ildjitSystem->program).thread_exception_infos)->exception_thrown;
        assert(exception_uncaught != NULL);

        /* Rethrow the exception			*/
        IRVM_throwException(&(ildjitSystem->IRVM), exception_uncaught);
        print_err("ERROR = The execution should not come at this point. ", 0);
        abort();
    }

    /* Fetch the result				*/
    if (is_primitive_result) {
        resultToReturn	= result;
    } else {
        resultToReturn	= *((void **) result);
        freeFunction(result);
    }

    return resultToReturn;
}

JITINT32 reflectionStringCmp (JITINT8 *name1, JITINT8 *name2, JITBOOLEAN ignoreCase) {
    JITINT8 ch1;
    JITINT8 ch2;

    /* Assertions			*/
    assert(name1 != NULL);
    assert(name2 != NULL);

    if (!ignoreCase) {
        return STRCMP(name1, name2);
    }

    while (*name1 != '\0' && *name2 != '\0') {
        ch1 = *name1++;
        if (ch1 >= 'A' && ch1 <= 'Z') {
            ch1 = (ch1 - 'A' + 'a');
        }
        ch2 = *name2++;
        if (ch2 >= 'A' && ch2 <= 'Z') {
            ch2 = (ch2 - 'A' + 'a');
        }
        if (ch1 < ch2) {
            return -1;
        } else if (ch1 > ch2) {
            return 1;
        }
    }
    if (*name1 != '\0') {
        return 1;
    } else if (*name2 != '\0') {
        return -1;
    }
    return 0;
}

void * GetTypeName (void *object, JITINT16 fullyQualified) {
    CLRType                 *class;
    void                    *string;
    JITINT8                 *name;
    JITUINT32 		length;

    /* Assertions				*/
    assert(object != NULL);

    /* Initialize the variable		*/
    name = NULL;
    class = NULL;
    string = NULL;

    /* Fetch the ILClass stored inside the  *
     * current instance of ClrType		*/
    class = ((ildjitSystem->cliManager).CLR.reflectManager).getPrivateDataFromType(object);
    assert(class != NULL);

    /* Make the name of the CLR type	*/
    if (fullyQualified) {
        name = class->ID->getCompleteName(class->ID);
    } else {
        name = class->ID->getName(class->ID);
    }

    length = STRLEN(name);

    /* Make the System.String		*/
    string = CLIMANAGER_StringManager_newInstanceFromUTF8(name, length);
    assert(string != NULL);

    /* Return				*/
    return string;
}

void * GetTypeNamespace (void *object) {
    CLRType                 *class;
    void                    *string;
    JITINT8                 *name;
    JITUINT32 length;

    /* Assertions				*/
    assert(object != NULL);

    /* Initialize the variable		*/
    name = NULL;
    class = NULL;
    string = NULL;

    /* Fetch the ILClass stored inside the  *
     * current instance of ClrType		*/
    class = ((ildjitSystem->cliManager).CLR.reflectManager).getPrivateDataFromType(object);
    assert(class != NULL);

    name = class->ID->getTypeNameSpace(class->ID);
    length = STRLEN(name);

    /* Make the System.String		*/
    string = CLIMANAGER_StringManager_newInstanceFromUTF8(name, length);
    assert(string != NULL);

    /* Return				*/
    return string;
}
