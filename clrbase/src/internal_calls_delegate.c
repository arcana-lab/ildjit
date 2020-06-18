/*
 * Copyright (C) 2008 - 2012 Campanoni Simone
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
#include <ildjit.h>

// My headers
#include <internal_calls_delegate.h>
#include <internal_calls_utilities.h>
// End

extern t_system *ildjitSystem;

void System_Runtime_Remoting_Messaging_AsyncResult_SetOutParams (void* delegate, void* args, void* outParams) {
    t_arrayManager* arrayManager;
    t_delegatesManager* delegatesManager;
    MethodDescriptor *callee;
    void* computedParam;
    void** outParam;
    JITUINT32 inputArgs;
    JITUINT32 outArgs;

    arrayManager = &((ildjitSystem->cliManager).CLR.arrayManager);
    delegatesManager = &((ildjitSystem->cliManager).CLR.delegatesManager);

    callee = delegatesManager->getCalleeMethod(delegatesManager, delegate);

    inputArgs = 0;
    outArgs = 0;
    ParamDescriptor *result = callee->getResult(callee);
    if (result->row != NULL) {
        if (result->attributes->is_out) {
            computedParam = *((void**) arrayManager->getArrayElement(args, inputArgs));
            outParam = *((void***) arrayManager->getArrayElement(outParams, outArgs++));
            *outParam = computedParam;
        }
        inputArgs++;
    }

    XanList *params = callee->getParams(callee);
    XanListItem *item = xanList_first(params);
    while (item != NULL) {
        ParamDescriptor *param = (ParamDescriptor *) item->data;
        if (param->row != NULL) {
            if (param->attributes->is_out) {
                computedParam = *((void**) arrayManager->getArrayElement(args, inputArgs));
                outParam = *((void***) arrayManager->getArrayElement(outParams, outArgs++));
                *outParam = computedParam;
            }
            inputArgs++;
        }
        item = item->next;
    }
}
