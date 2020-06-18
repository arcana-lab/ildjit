/*
 * Copyright (C) 2009  Campanoni Simone
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
#include <platform_API.h>

// My headers
#include <lib_culture.h>
#include <cli_manager.h>
// End

/* Protect metadata loading */
static pthread_once_t cm_metadataLock = PTHREAD_ONCE_INIT;

/* Public functions prototypes */
static void* cm_getInvariantCulture (void);
static void cm_initialize (void);
static void cm_loadMetadata (void);

extern CLIManager_t *cliManager;

/* Build the culture manager */
void init_cultureManager (t_cultureManager* self) {
    self->getInvariantCulture = cm_getInvariantCulture;
    self->initialize = cm_initialize;
}

/* Lazy initialization of the culture manager */
static void cm_initialize (void) {
    PLATFORM_pthread_once(&cm_metadataLock, cm_loadMetadata);
}

/* Load culture-related metadata */
static void cm_loadMetadata (void) {
    t_cultureManager* self;
    t_methods* methods;
    MethodDescriptor *invariantCultureGetterID;
    Method invariantCultureGetter;

    self = &((cliManager->CLR).cultureManager);
    methods = &(cliManager->methods);

    self->cultureInfoType = cliManager->metadataManager->getTypeDescriptorFromName(cliManager->metadataManager, (JITINT8 *) "System.Globalization", (JITINT8 *) "CultureInfo");

    XanList *methods_list = self->cultureInfoType->getMethodFromName(self->cultureInfoType, (JITINT8 *) "get_InvariantCulture");
    invariantCultureGetterID = (MethodDescriptor *) xanList_first(methods_list)->data;

    invariantCultureGetter = methods->fetchOrCreateMethod(methods, invariantCultureGetterID, JITTRUE);
    self->invariantCultureGetter = invariantCultureGetter;

    /* Tag the method as callable externally.
     */
    IRMETHOD_setMethodAsCallableExternally(&(self->invariantCultureGetter->IRMethod), JITTRUE);

    return ;
}

/* Get the invariant culture */
static void* cm_getInvariantCulture (void) {
    t_cultureManager* self;
    Method invariantCultureGetter;
    void* invariantCulture;

    self = &((cliManager->CLR).cultureManager);

    PLATFORM_pthread_once(&cm_metadataLock, cm_loadMetadata);

    invariantCultureGetter = self->invariantCultureGetter;
    IRMETHOD_translateMethodToMachineCode(&(invariantCultureGetter->IRMethod));
    IRVM_run(cliManager->IRVM, *(invariantCultureGetter->jit_function), NULL, &invariantCulture);

    return invariantCulture;
}
