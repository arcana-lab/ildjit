/*
 * Copyright (C) 2006 Simone Campanoni, Di Biagio Andrea
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
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <jit_metadata.h>
#include <iljit-utils.h>
#include <decoder.h>
#include <ir_virtual_machine.h>
#include <garbage_collector.h>

// My headers
#include <iljit.h>
#include <general_tools.h>
#include <cil_ir_translator.h>
#include <exception_manager.h>
#include <system_manager.h>
// End

/* functions headers */
static inline void * _createCILExceptionObject (t_system *system, TypeDescriptor *classID);
void * _get_Permanent_OutOfMemoryException ();
void * exception_handler (int exception_type);

extern t_system *ildjitSystem;

void EXCEPTION_initExceptionManager (void) {

    /* Retrieve the infos about each CIL exception.
     */
    (ildjitSystem->exception_system)._System_Exception_ID = (ildjitSystem->cliManager).metadataManager->getTypeDescriptorFromName((ildjitSystem->cliManager).metadataManager, (JITINT8 *) "System", (JITINT8 *) "Exception");

    (ildjitSystem->exception_system)._System_ArithmeticException_ID = (ildjitSystem->cliManager).metadataManager->getTypeDescriptorFromName((ildjitSystem->cliManager).metadataManager, (JITINT8 *) "System", (JITINT8 *) "ArithmeticException");

    (ildjitSystem->exception_system)._System_ArrayTypeMismatchException_ID = (ildjitSystem->cliManager).metadataManager->getTypeDescriptorFromName((ildjitSystem->cliManager).metadataManager, (JITINT8 *) "System", (JITINT8 *) "ArrayTypeMismatchException");

    (ildjitSystem->exception_system)._System_DivideByZeroException_ID = (ildjitSystem->cliManager).metadataManager->getTypeDescriptorFromName((ildjitSystem->cliManager).metadataManager, (JITINT8 *) "System", (JITINT8 *) "DivideByZeroException");

    (ildjitSystem->exception_system)._System_ExecutionEngineException_ID = (ildjitSystem->cliManager).metadataManager->getTypeDescriptorFromName((ildjitSystem->cliManager).metadataManager, (JITINT8 *) "System", (JITINT8 *) "ExecutionEngineException");

    (ildjitSystem->exception_system)._System_FieldAccessException_ID = (ildjitSystem->cliManager).metadataManager->getTypeDescriptorFromName((ildjitSystem->cliManager).metadataManager, (JITINT8 *) "System", (JITINT8 *) "FieldAccessException");

    (ildjitSystem->exception_system)._System_IndexOutOfRangeException_ID = (ildjitSystem->cliManager).metadataManager->getTypeDescriptorFromName((ildjitSystem->cliManager).metadataManager, (JITINT8 *) "System", (JITINT8 *) "IndexOutOfRangeException");

    /*
            (ildjitSystem->exception_system)._System_InvalidAddressException_ID=(ildjitSystem->cliManager).metadataManager->getTypeDescriptorFromName((ildjitSystem->cliManager).metadataManager,(JITINT8 *)"System", (JITINT8 *)"InvalidAddressException");
     */

    (ildjitSystem->exception_system)._System_InvalidCastException_ID = (ildjitSystem->cliManager).metadataManager->getTypeDescriptorFromName((ildjitSystem->cliManager).metadataManager, (JITINT8 *) "System", (JITINT8 *) "InvalidCastException");

    (ildjitSystem->exception_system)._System_InvalidOperationException_ID = (ildjitSystem->cliManager).metadataManager->getTypeDescriptorFromName((ildjitSystem->cliManager).metadataManager, (JITINT8 *) "System", (JITINT8 *) "InvalidOperationException");

    (ildjitSystem->exception_system)._System_MethodAccessException_ID = (ildjitSystem->cliManager).metadataManager->getTypeDescriptorFromName((ildjitSystem->cliManager).metadataManager, (JITINT8 *) "System", (JITINT8 *) "MethodAccessException");

    (ildjitSystem->exception_system)._System_MissingFieldException_ID = (ildjitSystem->cliManager).metadataManager->getTypeDescriptorFromName((ildjitSystem->cliManager).metadataManager, (JITINT8 *) "System", (JITINT8 *) "MissingFieldException");

    (ildjitSystem->exception_system)._System_MissingMethodException_ID = (ildjitSystem->cliManager).metadataManager->getTypeDescriptorFromName((ildjitSystem->cliManager).metadataManager, (JITINT8 *) "System", (JITINT8 *) "MissingMethodException");


    (ildjitSystem->exception_system)._System_NullReferenceException_ID = (ildjitSystem->cliManager).metadataManager->getTypeDescriptorFromName((ildjitSystem->cliManager).metadataManager, (JITINT8 *) "System", (JITINT8 *) "NullReferenceException");

    (ildjitSystem->exception_system)._System_OverflowException_ID = (ildjitSystem->cliManager).metadataManager->getTypeDescriptorFromName((ildjitSystem->cliManager).metadataManager, (JITINT8 *) "System", (JITINT8 *) "OverflowException");

    (ildjitSystem->exception_system)._System_OutOfMemoryException_ID = (ildjitSystem->cliManager).metadataManager->getTypeDescriptorFromName((ildjitSystem->cliManager).metadataManager, (JITINT8 *) "System", (JITINT8 *) "OutOfMemoryException");

    /*
            (ildjitSystem->exception_system)._System_SecurityException_ID=(ildjitSystem->cliManager).metadataManager->getTypeDescriptorFromName((ildjitSystem->cliManager).metadataManager,(JITINT8 *)"System", (JITINT8 *)"SecurityException");
     */

    (ildjitSystem->exception_system)._System_StackOverflowException_ID = (ildjitSystem->cliManager).metadataManager->getTypeDescriptorFromName((ildjitSystem->cliManager).metadataManager, (JITINT8 *) "System", (JITINT8 *) "StackOverflowException");


    (ildjitSystem->exception_system)._System_TypeLoadException_ID = (ildjitSystem->cliManager).metadataManager->getTypeDescriptorFromName((ildjitSystem->cliManager).metadataManager, (JITINT8 *) "System", (JITINT8 *) "TypeLoadException");

    (ildjitSystem->exception_system)._System_ArgumentException_ID = (ildjitSystem->cliManager).metadataManager->getTypeDescriptorFromName((ildjitSystem->cliManager).metadataManager, (JITINT8 *) "System", (JITINT8 *) "ArgumentException");

    (ildjitSystem->exception_system)._System_ArgumentOutOfRangeException_ID = (ildjitSystem->cliManager).metadataManager->getTypeDescriptorFromName((ildjitSystem->cliManager).metadataManager, (JITINT8 *) "System", (JITINT8 *) "ArgumentOutOfRangeException");

    //BEGIN TODO UNCOMMENT ONLY IF THE TYPE System.VerificationException IS IMPLEMENTED
    /*
       (ildjitSystem->exception_system)._System_VerificationException_ID=(ildjitSystem->cliManager).metadataManager->getTypeDescriptorFromName((ildjitSystem->cliManager).metadataManager,(JITINT8 *)"System", (JITINT8 *)"VerificationException");
     */
    //END TODO

    /* Setup the exception handler.
     */
    IRVM_setExceptionHandler(&(ildjitSystem->IRVM), exception_handler);

    return ;
}

void EXCEPTION_allocateOutOfMemoryException (void) {

    /*	MAKING A permanent instance of OutOfMemoryException	*/
    (ildjitSystem->exception_system)._OutOfMemoryException = (ildjitSystem->garbage_collectors).gc.allocPermanentObject((ildjitSystem->exception_system)._System_OutOfMemoryException_ID, 0);
    if ((ildjitSystem->exception_system)._OutOfMemoryException == NULL) {
        print_err("EXCEPTION_MANAGER: initExeceptionSystem : Unable to create a permanent instance of OutOfMemoryException ", 0);
        abort();
    }

    return;
}

static inline void * _createCILExceptionObject (t_system *system, TypeDescriptor *classID) {
    void            *object_created;        /*	THE OBJECT CREATED */

    /* assertions */
    assert(system != NULL);

    /* This kind of error should never happen with verified code!
     * In the case of unverified code, we should throw a System.TypeLoadException object.
     * while we are trying to create a CIL exception we assume that the local variable
     * classLocated should never be NULL. Then we use assertion to verify this conditions */
    assert(classID != NULL);

    /* once found the informations regarding the class, we must create the object.
     * To do this, we must call the `allocObject` function of the garbage collector */
    object_created = (system->garbage_collectors).gc.allocObject(classID, 0);

    /*
     * A GC should return a NULL pointer only if there isn't enough memory to create an istance
     * of an object. We shall assume that it's always possible to build a new exception
     * instance for a CIL exception type.
     * An OutOfMemoryException is a particular exception that shall be managed in a different manner
     * with respect to the other CIL exceptions. Without the right amount of memory isn't possible
     * to create the exception object. The idea is to manage this problem creating `ahead of time`
     * the instance of an OutOfMemoryException exception. If the garbage collector return a null pointer
     * it must be assumed that the memory allocator is unable to find an empty slot to allocate a new instance.
     * However it's possible that the memory allocator still mantains sufficient space in memory to allocate
     * a new instance of OutOfMemoryException. If the latter is the case, a classic allocObject performed by
     * the GC is sufficient. Otherwise (in all the other cases) we must return the permanently allocated instance
     * of OutOfMemory created ahead of time during the start-up phase when initializing the exception system.
     */
    if (object_created == NULL) {
        PDEBUG("EXCEPTION_MANAGER : createCILExceptionObject : The GC returned a NULL pointer \n");
        PDEBUG("EXCEPTION_MANAGER : createCILExceptionObject : Trying to create "
               "an OutOfMemoryException... \n");
        object_created = (system->garbage_collectors).gc.allocObject((system->exception_system)._System_OutOfMemoryException_ID, 0 );

        if (object_created == NULL) {
            PDEBUG("EXCEPTION_MANAGER : createCILExceptionObject : insufficient memory!"
                   "Unable to create an OutOfMemoryException instance. \n");
            PDEBUG("EXCEPTION_MANAGER : createCILExceptionObject : returning the "
                   "permanently allocated instance of OutOfMemoryException \n");
            object_created = _get_Permanent_OutOfMemoryException(system);
            setConstructed(1);
        }

        /* take a snapshot of the current stack trace element */
        updateTheStackTrace();
    }

    /* Note that the every object returned by this function is not constructed. There must be a way in the
     * libjit exception catcher to manage the correct call to the specific constructor.
     */

    /* check the postconditions */
    assert(object_created != NULL);

    return object_created;
}

void updateTheStackTrace () {
    /*	SET THE NEW STACK TRACE */
    PDEBUG("EXCEPTION_MANAGER : updateTheStackTrace : Updating the stack_trace \n");
    ((ildjitSystem->program).thread_exception_infos)->stackTrace = IRVM_getStackTrace();
}

/*   A CUSTOM EXCEPTION HANDLER. */
void * exception_handler (int exception_type) {
    t_system                *system;
    void                    *exceptionObject;

    /* Fetch the system		*/
    system = getSystem(NULL);
    assert(system != NULL);
    PDEBUG("EXCEPTION_MANAGER: exception_handler: Start\n");

    /* Initialize the variables	*/
    exceptionObject = NULL;

    /* Set the flag that show this  *
    * exception is not constructed	*/
    setConstructed(0);

    /* Create the correct CIL       *
     * Exception OBJECT		*/
    switch (exception_type) {
        case COMPILATION_RESULT_OK:
            PDEBUG("EXCEPTION_MANAGER: exception_handler:   Raised an exception of type `OK`\n");
            break;
        case COMPILATION_RESULT_OVERFLOW:
            PDEBUG("EXCEPTION_MANAGER: exception_handler:   Raised an exception of type `System.OverflowException`\n");
            exceptionObject = _createCILExceptionObject(system, (system->exception_system)._System_OverflowException_ID);
            break;
        case COMPILATION_RESULT_ARITHMETIC:
            PDEBUG("EXCEPTION_MANAGER: exception_handler:   Raised an exception of type `System.ArithmeticException`\n");
            exceptionObject = _createCILExceptionObject(system, (system->exception_system)._System_ArithmeticException_ID);
            break;
        case COMPILATION_RESULT_CALLED_NESTED:
            PDEBUG("EXCEPTION_MANAGER: exception_handler:	Raised anexception of type `System.ExecutionEngineException`\n");
            exceptionObject = _createCILExceptionObject(system, (system->exception_system)._System_ExecutionEngineException_ID);
            break;
        case COMPILATION_RESULT_DIVISION_BY_ZERO:
            PDEBUG("EXCEPTION_MANAGER: exception_handler:   Raised an exception of type `System.DivideByZeroException`\n");
            exceptionObject = _createCILExceptionObject(system, (system->exception_system)._System_DivideByZeroException_ID);
            break;
        case COMPILATION_RESULT_COMPILE_ERROR:
            PDEBUG("EXCEPTION_MANAGER: exception_handler:   Raised an exception of type `System.ExecutionEngineException`\n");
            exceptionObject = _createCILExceptionObject(system, (system->exception_system)._System_ExecutionEngineException_ID );
            break;
        case COMPILATION_RESULT_OUT_OF_MEMORY:
            PDEBUG("EXCEPTION_MANAGER : exception_handler . Raised an exception of type `System.StackOverflowException`\n");
            exceptionObject = _createCILExceptionObject(system, (system->exception_system)._System_OutOfMemoryException_ID);
            break;
        case COMPILATION_RESULT_NULL_REFERENCE:
            PDEBUG("EXCEPTION_MANAGER : exception_handler . Raised an exception of type `System.NullReferenceException`\n");
            exceptionObject = _createCILExceptionObject(system, (system->exception_system)._System_NullReferenceException_ID);
            break;
        case COMPILATION_RESULT_NULL_FUNCTION:
            PDEBUG("EXCEPTION_MANAGER : exception_handler . Raised an exception of type `System.ExecutionEngineException`\n");
            exceptionObject = _createCILExceptionObject(system, (system->exception_system)._System_ExecutionEngineException_ID);
            break;
        default:
            PDEBUG("EXCEPTION_MANAGER : exception_handler . Raised an exception of type `System.ExecutionEngineException`\n");
            exceptionObject = _createCILExceptionObject(system, (system->exception_system)._System_ExecutionEngineException_ID);
    }

    /* Set the exception		*/
    setCurrentThreadException(exceptionObject);

    /* Update the stack trace	*/
    updateTheStackTrace();

    /* Return		*/
    PDEBUG("EXCEPTION_MANAGER: exception_handler: Exit\n");
    return exceptionObject;
}

JITBOOLEAN isConstructed () {
    t_system        *system;

    /* initialization of the local variables */
    system = getSystem(NULL);

    /* assertions */
    assert(system != NULL);
    assert((system->program).thread_exception_infos != NULL);

    PDEBUG("EXCEPTION_MANAGER : is_constructed? %d \n",
           ((system->program).thread_exception_infos)->isConstructed );
    return ((system->program).thread_exception_infos)->isConstructed;
}

void setConstructed (JITBOOLEAN value) {
    t_system        *system;

    /* Initialization of the local variables */
    system = getSystem(NULL);

    /* Assertions   */
    assert(system != NULL);
    assert((system->program).thread_exception_infos != NULL);
    PDEBUG("EXCEPTION_MANAGER: setConstructed: Start\n");

    PDEBUG("EXCEPTION_MANAGER: setConstructed:      Set the flag of construction of the exception to %d \n", value);
    ((system->program).thread_exception_infos)->isConstructed = value;

    /* Return	*/
    PDEBUG("EXCEPTION_MANAGER: setConstructed: Exit\n");
    return;
}

void * getCurrentThreadException () {
    t_system        *system;

    /* initialization of the local variables */
    system = getSystem(NULL);

    /* assertions */
    assert(system != NULL);
    assert((system->program).thread_exception_infos != NULL);

    PDEBUG("EXCEPTION_MANAGER : get_current_thread_exception %p \n",
           ((system->program).thread_exception_infos)->exception_thrown );
    return ((system->program).thread_exception_infos)->exception_thrown;
}

void setCurrentThreadException (void * new_exception_object) {
#ifdef DEBUG
    TypeDescriptor          *exception_type;
#endif

    /* Assertions			*/
    assert(new_exception_object != NULL);
    PDEBUG("EXCEPTION_MANAGER: set_current_thread_exception: Start\n");

    /* Initialization of the local variables */
#ifdef DEBUG
    exception_type = (ildjitSystem->garbage_collectors).gc.getType(new_exception_object);
    assert((ildjitSystem->program).thread_exception_infos != NULL);
    assert(exception_type != NULL);
    PDEBUG("EXCEPTION_MANAGER: set_current_thread_exception:        setting an exception object of type %s\n", exception_type->getCompleteName(exception_type));
#endif

    /* Set the exception.
     */
    ((ildjitSystem->program).thread_exception_infos)->exception_thrown = new_exception_object;

    PDEBUG("EXCEPTION_MANAGER: set_current_thread_exception: Exit\n");
    return ;
}

void print_stack_trace (t_system *system) {
    void                    *exception_uncaught;
    TypeDescriptor                  *type_of_the_object;
    JITINT8                 *message;
    IRVM_stackTrace *stack_trace;
    JITUINT32 stack_size;
    JITUINT32 count;

    /* Initialize the local variables */
    exception_uncaught = NULL;
    message = (JITINT8 *) "Out of memory.";

    /*	ASSERTIONS	*/
    assert(system   != NULL);
    assert(message != NULL);

    /* retrieve the uncaught exception object */
    exception_uncaught = ((system->program).thread_exception_infos)->exception_thrown;
    stack_trace = ((system->program).thread_exception_infos)->stackTrace;
    assert(exception_uncaught != NULL);
    assert(stack_trace != NULL);

    /* Retrieve the exception type & binary of the exception object */

    type_of_the_object = (system->garbage_collectors).gc.getType(exception_uncaught);

    /* more assertions */
    assert(type_of_the_object != NULL);

    PDEBUG("EXCEPTION_MANAGER : print_stack_trace : Type == %s \n", type_of_the_object->getCompleteName(type_of_the_object) );

#ifdef DEBUG
    if (!(system->garbage_collectors).gc.isSubtype(exception_uncaught, (system->exception_system)._System_Exception_ID) ) {
        PDEBUG("EXCEPTION_MANAGER : print_stack_trace : error while attempting to reference "
               "an object having a type that is not a subtype of System.Exception \n");
        PDEBUG("EXCEPTION_MANAGER : print_stack_trace : the code is not verified! \n");
        abort();
    }
#endif

    if ( (type_of_the_object != (system->exception_system)._System_OutOfMemoryException_ID)
            && (exception_uncaught != (system->exception_system)._OutOfMemoryException) ) {
        FieldDescriptor                 *message_field;
        JITINT32 message_field_offset;
        void                    *field;
        void                    *field_value;
#ifdef DEBUG
        TypeDescriptor          *field_class_ID;
#endif


        /* At first we shall retrieve the error message associated with this exception object. */
        /* The error message is stored into a Field named `message` as a String object. To access
         * this field, we use the functionalities offered by the garbage collector. */

        /* The `message` field is stored at a certain offset from the beginning of the exception object.
         * Using the `get_field_from_name` function offered by general_tools.h, we should retrieve the
         * FieldID element associated with the `message` field.*/

        /* Note that if the exception thrown is a `System.OutOfMemoryException`, we assume that the object
         * is simply created but not constructed yet. In this case, we have to NOT fetch the message
         * information within the exception object */

        message_field = (system->exception_system)._System_Exception_ID->getFieldFromName((system->exception_system)._System_Exception_ID, (JITINT8 *) "message");
        assert(message_field != NULL);


        /* In order, to print correctly the stack-trace, we have to use properly
         * the message_field_ID information */
        /* Using this info., we can retrieve the field offset with respect to
         * the offset of the exception object */
        message_field_offset = (system->garbage_collectors).gc.fetchFieldOffset(message_field);
        PDEBUG("EXCEPTION_MANAGER : print_stack_trace : message_field_offset = %d\n", message_field_offset);

        /* Finally we can fetch the filed element using the informations gathered until now */
        field = (exception_uncaught + message_field_offset);
        assert(field != NULL);


        field_value = (void *) (*(JITNUINT *) field);
        if (field_value == NULL) {
            /* la stringa è un null reference */
            message = (JITINT8 *) " [NO MESSAGE] ";
        } else {
            JITINT16        *messageUTF16;
            JITINT32 	messageLength;
#ifdef DEBUG
            TypeDescriptor 	*type_of_the_class;
#endif

#ifdef DEBUG
            field_class_ID = (system->garbage_collectors).gc.getType(field_value);
            assert(field_class_ID != NULL);
            type_of_the_class = (system->garbage_collectors).gc.getType(field_value);
            assert(type_of_the_class != NULL);
            PDEBUG("EXCEPTION_MANAGER : print_stack_trace : class_name = %s\n", type_of_the_class->getCompleteName(type_of_the_class));
            PDEBUG("EXCEPTION_MANAGER : print_stack_trace : message_pointer = %p\n", field_value);
#endif
            messageUTF16 = (system->cliManager).CLR.stringManager.toLiteral(field_value);
            messageLength = (system->cliManager).CLR.stringManager.getLength(field_value);
            message = allocFunction(messageLength + 1);
            assert(message != NULL);
            (system->cliManager).CLR.stringManager.fromUTF16ToUTF8(messageUTF16, (JITINT8 *) message, messageLength);
        }
    }

    fprintf(stdout, "Uncaught exception: %s : %s\n", type_of_the_object->getCompleteName(type_of_the_object), message);

    /* RETRIEVE THE STACK_TRACE SIZE*/
    stack_size = IRVM_getStackTraceSize(stack_trace);

    /* For each known method of the stack trace, we must print on
     * the standard output the associated informations */
    for (count = 0; count < stack_size; count++) {
        Method current_method;

        /* Fetch the method from the stack-trace                        */
        current_method = (Method) IRVM_getStackTraceFunctionAt(&(ildjitSystem->IRVM), stack_trace, count);
        if (current_method != NULL) {
            JITUINT32 CIL_Offset;
            MethodDescriptor      *CIL_method;

            /* Fetch the CIL method signature                                       */
            CIL_method = current_method->getID(current_method);
            assert(CIL_method != NULL);

            /* retrieve the CIL-offset associated with the stack-trace element */
            CIL_Offset = IRVM_getStackTraceOffset(&(ildjitSystem->IRVM), stack_trace, count);

            /* print the information to the standard output */
            assert(CIL_method != NULL);
            fprintf(stdout, "\tat %s\t[CIL-offset: %d]\n", CIL_method->getSignatureInString(CIL_method), CIL_Offset);
        }
    }

    /* release the memory allocated for the stack_trace element */
    IRVM_destroyStackTrace(stack_trace);

    /* Return					*/
    return;
}

void * get_OutOfMemoryException () {
    t_system        *system;

    /* retrieve the system element */
    system = getSystem(NULL);

    /* assertions */
    assert(system != NULL);

    return _createCILExceptionObject( system, (system->exception_system)._System_OutOfMemoryException_ID);
}

void throw_thread_exception_ByName (JITINT8 *typeNameSpace, JITINT8 *typeName) {
    JITUINT32 max_length;
    t_system        *system;
    void            *exception_object;

    /* retrieve the system element */
    system = getSystem(NULL);

    /* initialize the exception_object variable */
    exception_object = NULL;

    /* assertions */
    assert(typeNameSpace != NULL);
    assert(typeName != NULL);
    assert(system != NULL);

    /* verify if the type_name_space is equivalent to "System" */
    max_length = STRLEN(typeNameSpace);
    if (max_length != strlen("System")) {
        char    *message;

        /* initialize the error message */
        message = "EXCEPTION_MANAGER : throw_thread_exception_ByName :"
                  "typeNameSpace.length != \"System\".length\n";
        print_err(message, 0);
        exit(1);
    }
    if (STRNCMP(typeNameSpace, "System", max_length * sizeof(char)) != 0) {
        char    *message;

        message = "EXCEPTION_MANAGER : throw_thread_exception_ByName : typeNameSpace != \"System\"\n";
        print_err(message, 0);
        exit(1);
    }

    /* verify which exception we shall throw */
    if (STRCMP(typeName, "Exception") == 0) {
        exception_object = _createCILExceptionObject(
                               system, (system->exception_system)._System_Exception_ID );
    } else if (STRCMP(typeName, "ArithmeticException") == 0) {
        exception_object = _createCILExceptionObject(
                               system, (system->exception_system)._System_ArithmeticException_ID );
    } else if (STRCMP(typeName, "DivideByZeroException") == 0) {
        exception_object = _createCILExceptionObject(
                               system, (system->exception_system)._System_DivideByZeroException_ID);
    } else if (STRCMP(typeName, "ExecutionEngineException") == 0) {
        exception_object = _createCILExceptionObject(
                               system, (system->exception_system)._System_ExecutionEngineException_ID);
    } else if (STRCMP(typeName, "IndexOutOfRangeException") == 0) {
        exception_object = _createCILExceptionObject(
                               system, (system->exception_system)._System_IndexOutOfRangeException_ID);
    } else if (STRCMP(typeName, "InvalidAddressException") == 0) {
        PDEBUG("\"System.InvalidAddressException\", Not yet implemented by 'pnetlib'! \n");
        exit(1);
    } else if (STRCMP(typeName, "InvalidCastException") == 0) {
        exception_object = _createCILExceptionObject(
                               system, (system->exception_system)._System_InvalidCastException_ID);
    } else if (STRCMP(typeName, "MethodAccessException") == 0) {
        exception_object = _createCILExceptionObject(
                               system, (system->exception_system)._System_MethodAccessException_ID);
    } else if (STRCMP(typeName, "MissingFieldException") == 0) {
        exception_object = _createCILExceptionObject(
                               system, (system->exception_system)._System_MissingFieldException_ID);
    } else if (STRCMP(typeName, "MissingMethodException") == 0) {
        exception_object = _createCILExceptionObject(
                               system, (system->exception_system)._System_MissingMethodException_ID);
    } else if (STRCMP(typeName, "NullReferenceException") == 0) {
        exception_object = _createCILExceptionObject(
                               system, (system->exception_system)._System_NullReferenceException_ID);
    } else if (STRCMP(typeName, "OverflowException") == 0) {
        exception_object = _createCILExceptionObject(
                               system, (system->exception_system)._System_OverflowException_ID);
    } else if (STRCMP(typeName, "OutOfMemoryException") == 0) {
        exception_object = _createCILExceptionObject(
                               system, (system->exception_system)._System_OutOfMemoryException_ID);
    } else if (STRCMP(typeName, "SecurityException") == 0) {
        PDEBUG("\"System.SecurityException\", Not implemented by 'pnetlib'! \n");
        abort();
    } else if (STRCMP(typeName, "StackOverflowException") == 0) {
        exception_object = _createCILExceptionObject(
                               system, (system->exception_system)._System_StackOverflowException_ID);
    } else if (STRCMP(typeName, "TypeLoadException") == 0) {
        exception_object = _createCILExceptionObject(system, (system->exception_system)._System_TypeLoadException_ID );
    } else if (STRCMP(typeName, "ArgumentException") == 0) {
        exception_object = _createCILExceptionObject(system, (system->exception_system)._System_ArgumentException_ID);
    } else if (STRCMP(typeName, "ArgumentOutOfRangeException") == 0) {
        exception_object = _createCILExceptionObject(system, (system->exception_system)._System_ArgumentOutOfRangeException_ID);
    } else {
        PDEBUG("CIL exception type (System.%s) Not supported\n", typeName);
        print_err("CIL exception type Not supported\n", 0);
        abort();
    }
    assert(exception_object != NULL);

    /* throw the exception object */
    throw_thread_exception(exception_object);

    abort();
}

void throw_thread_exception (void *exception_object) {
    TypeDescriptor          *exception_type;

    /* Assertions */
    assert(exception_object != NULL);

    /* retrieve the exception type */
    exception_type = (ildjitSystem->garbage_collectors).gc.getType(exception_object);
    assert(exception_type != NULL);

    /* verify if exception_object is a "sub-type" of System.Exception */
    if (!(ildjitSystem->garbage_collectors).gc.isSubtype(exception_object, (ildjitSystem->exception_system)._System_Exception_ID)     ) {
        print_err("EXCEPTION_MANAGER: throw_thread_exception : ERROR = invalid type. \n"
                  "The object appears to be NOT an exception object. ", 0);
        abort();
    }

    /* update the thread exception infos. */
    if (exception_type != (ildjitSystem->exception_system)._System_OutOfMemoryException_ID) {
        assert(isConstructed() == 1);
        setConstructed(0);
    } else {
        assert(isConstructed() == 1);
        PDEBUG("EXCEPTION_MANAGER : throw_thread_exception : UPDATING THE STACK_TRACE!");
        updateTheStackTrace();
    }

    /* Set the exception		*/
    setCurrentThreadException(exception_object);

    /* Update the stack trace	*/
    updateTheStackTrace();

    /* Throw the exception */
    PDEBUG("EXCEPTION_MANAGER : throw_thread_exception : throwing a runtime exception \n");
    IRVM_throwException(&(ildjitSystem->IRVM), exception_object);
    abort();

    /* Return			*/
    return;
}

void * _get_Permanent_OutOfMemoryException (t_system *system) {

    /* Assertions */
    assert(system != NULL);

    PDEBUG("EXCEPTION_MANAGER : _get_Permanent_OutOfMemoryException : "
           "fetching the permanent OutOfMemoryException object \n");
    return (system->exception_system)._OutOfMemoryException;
}
