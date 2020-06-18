/*
 * Copyright (C) 2006 Campanoni Simone, Di Biagio Andrea
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
#include <errno.h>
#include <ir_language.h>
#include <ildjit.h>

// My header
#include <general_tools.h>
#include <clr_interface.h>
// End

extern t_system *ildjitSystem;

t_system * getSystem (t_system *system) {
    return ildjitSystem;
}

/* Build a new delegate */
/* TODO: signature dynamic checking */
void BuildDelegate (void* self, void* target, void* function_entry_point) {
    Method method;
    /* The currently running garbage collector */
    t_running_garbage_collector* garbageCollector;

    /* Used to compile methods */
    TranslationPipeline* pipeliner;

    /* The delegates manager */
    t_delegatesManager* delegatesManager;

    /* The reflection manager */
    t_reflectManager* reflectManager;

    /* The System.MulticastDelegate type */
    TypeDescriptor* mdType;

    /* The target type, as a CLRType */
    TypeDescriptor *targetCLRType;

    /*
     * Set to JITTRUE iff if given object type is a subtype of
     * MulticastDelegate
     */
    JITBOOLEAN isSubtype;

    /* Super constructor */
    Method mdCtor;

    /* The name of the method to invoke, as a char array */
    JITINT8* rawMethodName;

    /* The length of the rawMethodName */
    size_t length;

    /* A System.String object that store rawMethodName */
    void* methodName;

    /* Super constructor arguments */
    void* args[3];

    /*
     * Set to JITFALSE iff a call to a libjit function doesn't generate an
     * exception
     */
    JITBOOLEAN noException;
    XanHashTable *functionsEntryPoint;

    /* Assertions */
    assert(function_entry_point != NULL);

    /* Cache some data */
    garbageCollector = &(ildjitSystem->garbage_collectors.gc);
    delegatesManager = &((ildjitSystem->cliManager).CLR.delegatesManager);
    mdType = delegatesManager->getMulticastDelegate(delegatesManager);
    functionsEntryPoint = (ildjitSystem->cliManager).methods.functionsEntryPoint;

    /* Obtain the Method relative to the given entry point */
    method = (Method) xanHashTable_lookup(functionsEntryPoint, function_entry_point);
    assert(method != NULL);
    METHOD_BEGIN(ildjitSystem, "BuildDelegate");

    /* Check that this is a delegate */
    isSubtype = garbageCollector->isSubtype(self, mdType);
    if (!isSubtype) {
        print_err("Error: this object isn't a delegate", 0);
        abort();
    }

    /*
     * If target is NULL we need to retrieve the class where method is
     * defined (the delegate points to a static method)
     */
    if (target == NULL) {
        reflectManager = &((ildjitSystem->cliManager).CLR.reflectManager);
        targetCLRType = method->getType(method);
        target = reflectManager->getType(targetCLRType);
        mdCtor = delegatesManager->getMDSCtor(delegatesManager);

        /* The delegate reference an instance method */
    } else {
        mdCtor = delegatesManager->getMDCtor(delegatesManager);
    }

    /* The second argument is the method name, wrapped in a System.String */
    rawMethodName = method->getName(method);
    length = STRLEN(rawMethodName);
    methodName = CLIMANAGER_StringManager_newInstanceFromUTF8( (JITINT8*) rawMethodName, (JITINT32) length);

    /* Compile super constructor, if not yet done */
    pipeliner = &(ildjitSystem->pipeliner);
    pipeliner->synchInsertMethod(pipeliner, mdCtor, MAX_METHOD_PRIORITY);
    if (	(!(ildjitSystem->IRVM).behavior.aot) 		&&
            ((ildjitSystem->IRVM).behavior.pgc == 1)	) {
        MANFRED_appendMethodToProfile(&(ildjitSystem->manfred), mdCtor);
    }

    /* Check if we need to execute the code.
     */
    if ((ildjitSystem->program).disableExecution) {
        METHOD_END(ildjitSystem, "BuildDelegate");
        return;
    }

    /* OK, we have all we need to call super constructor */
    args[0] = &self;
    args[1] = &target;
    args[2] = &methodName;
    noException = IRVM_run(&(ildjitSystem->IRVM), *(mdCtor->jit_function), args, NULL);

    /* Throw exception to caller if raised from callee */
    if (!noException) {
        ILDJIT_throwException(ILDJIT_getCurrentThreadException());
        abort();
    }

    METHOD_END(ildjitSystem, "BuildDelegate");
    return;
}
