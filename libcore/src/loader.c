/*
 * Copyright (C) 2006 - 2011  Campanoni Simone
 *
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
/**
   *@file loader.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <iljit-utils.h>
#include <error_codes.h>
#include <compiler_memory_manager.h>
#include <section_manager.h>
#include <metadata/tables/metadata_table_manager.h>
#include <metadata/streams/metadata_streams_manager.h>
#include <metadata_manager.h>
#include <ir_virtual_machine.h>
#include <platform_API.h>
#include <ilbinary.h>

// My header
#include <code_generator.h>
#include <iljit.h>
#include <system_manager.h>
#include <iljit_plugin_manager.h>
#include <loader.h>
#include <cil_ir_translator.h>
#include <exception_manager.h>
#include <translation_pipeline.h>
#include <general_tools.h>
// End

t_plugins *plugin;

/**
 * @brief Initial condition
 *
 * Check if the initial condition are satisfied
 */
static inline void check_initial_conditions (t_system *system);

/**
 * @brief Search a decoder
 *
 * Search a decoder that it can decode an assembly with the identificator equal to the string pointed by the binary_ID parameter
 */
static inline t_plugins * decoder_searcher (XanList *plugins, JITINT8 *binary_ID);

/**
 * @brief Load the CIL instructions
 *
 * Fetch the CIL instructions of the main method
 */
static inline JITINT16 internal_setProgramEntryPoint (t_system *system);

static inline void load_IR (t_system *system);

/**
 * @brief Decode the assemblies reference
 *
 * Decode all the assemblies referenced by the assembly stored inside the system parameter
 */
static inline JITBOOLEAN load_assemblies (t_system *system);

static inline t_binary_information * internal_fetchBinaryByName (t_system *system, JITINT8 *assemblyName);
static inline t_binary_information * internal_fetchBinaryByNameAndPrefix (t_system *system, JITINT8 *assemblyName, JITINT8 *assemblyPrefix);
static inline void binary_init(t_binary_information *binary_info);

JITBOOLEAN loader (t_system *system) {
    JITINT16		error;
    JITINT8			binary_ID[2];
    JITINT8			*prefix, *name;
    t_binary_information	*binary_info;
    XanListItem		*item;

    /* Assertions.
     */
    assert(system != NULL);
    PDEBUG("LOADER (%d): Start\n", getpid());

    /* Prepare the path and the basename of the entrypoint binary.
     */
    prefix = (JITINT8*)realpath((system->arg).argv[0], NULL);
    if (prefix == NULL) {
        perror((system->arg).argv[0]);
        return NO_READ_POSSIBLE;
    }
    name					= allocFunction(MAX_NAME);
    STRNCPY(name, strrchr((char*)prefix, '/') + 1, MAX_NAME);
    (*(strrchr((char*)prefix, '/')))	= '\0';

    /* Lock the list of the decoded binaries.
     */
    xanList_lock((system->cliManager).binaries);

    /* Print the current list of decoded binaries.
     */
#ifdef DEBUG
    PDEBUG("LOADER (%d): Binaries already loaded (%d)\n", getpid(), xanList_length((system->cliManager).binaries));
    item = xanList_first((system->cliManager).binaries);
    while ( item != NULL ) {
        PDEBUG("%s/%s\n", ((t_binary_information*)item->data)->prefix, ((t_binary_information*)item->data)->name);
        item = item->next;
    }
#endif

    /* Check if current binary is already decoded.
     */
    binary_info = CLIMANAGER_fetchBinaryByNameAndPrefix(&(system->cliManager), name, prefix);
    if ( binary_info == NULL ) {
        PDEBUG("BOOTSTRAPPER: Binary %s/%s is not already present in the list of binaries decoded\n", prefix, name);

        /* Allocate the binary data structure.
         */
        binary_info = ILBINARY_new();

        /* Copy the names into the binary data structure.
         */
        ILBINARY_setBinaryName(binary_info, name);
        STRNCPY(binary_info->prefix, prefix, PATH_MAX);

        /* Setup the binary.
         */
        binary_init(binary_info);
        PDEBUG("LOADER (%d):         Binary	= %s\n", getpid(), binary_info->name);

        /* Read the binary ID.
         */
        PDEBUG("LOADER (%d):         Read the binary ID\n", getpid());
        if (ILBINARY_decodeFileType(binary_info, binary_ID) != 0) {
            return NO_READ_POSSIBLE;
        }
        PDEBUG("			byte 0:%c\n			byte 1:%c\n", binary_ID[0], binary_ID[1]);

        /* Search the decoder for the binary.
         */
        PDEBUG("LOADER (%d):         Search the decoder\n", getpid());
        plugin  = decoder_searcher(system->plugins, binary_ID);
        if (plugin == NULL) {
            return NO_DECODER_FOUND;
        }

        /* Set the binary entry point.
         * This operation must proceed running the decoder because the entry point might be used by the decoder.
         */
        CLIMANAGER_setEntryPoint(&(system->cliManager), binary_info);

        /* Run the decoder.
         */
        PDEBUG("LOADER (%d):         Run the decoder found\n", getpid());
        PLATFORM_lockMutex(&(((system->cliManager).entryPoint.binary->binary).mutex));
        if (plugin->decode_image((system->cliManager).entryPoint.binary) != 0) {
            PLATFORM_unlockMutex(&(((system->cliManager).entryPoint.binary->binary).mutex));
            print_err("LOADER: ERROR: The decoder return an error. ", 0);
            return DECODER_RETURN_AN_ERROR;
        }
        PLATFORM_unlockMutex(&(((system->cliManager).entryPoint.binary->binary).mutex));
        PDEBUG("LOADER (%d):                 OK\n", getpid());

        /* Add the reference of the assembly to the decoder.
         */
        if (xanList_find(plugin->assemblies_decoded, (system->cliManager).entryPoint.binary) == NULL) {
            PDEBUG("LOADER (%d):		Add the current assembly to the list of the decoder\n", getpid());
            xanList_insert(plugin->assemblies_decoded, (system->cliManager).entryPoint.binary);
        }

        /* Insert the binary just loaded inside the list.
         */
        xanList_insert((system->cliManager).binaries, binary_info);

        /* Load assemblies referenced.
         */
        PDEBUG("LOADER:         Decode all the assemblies referenced\n");
        PLATFORM_lockMutex(&(((system->cliManager).entryPoint.binary->binary).mutex));
        if (load_assemblies(system) != 0) {
            return CANNOT_DECODE_ASSEMBLIES_REFERENCED;
        }
        PLATFORM_unlockMutex(&(((system->cliManager).entryPoint.binary->binary).mutex));

        /* Check the initial condition.
         */
#ifdef DEBUG
        check_initial_conditions(system);
#endif

    } else {
        PDEBUG("BOOTSTRAPPER: Binary %s/%s already present in the list of binaries decoded\n", prefix, name);

        /* Setup the binary					*/
        binary_init(binary_info);

        /* Set the binary entry point				*/
        CLIMANAGER_setEntryPoint(&(system->cliManager), binary_info);

        /* Check the initial condition				*/
        check_initial_conditions(system);
        PDEBUG("LOADER (%d):         Binary	= %s\n", getpid(), binary_info->name);
    }

    /* Open all the other required binaries		*/
    item = xanList_first((system->cliManager).binaries);
    while ( item != NULL ) {
        t_binary_information *temp;

        /* Fetch the binary				*/
        temp = item->data;
        assert(temp != NULL);

        /* Only need to open libraries			*/
        if (	(!str_has_suffix((char*)temp->name, ".exe"))	&&
                (!str_has_suffix((char*)temp->name, ".cil")) 	&&
                (*(temp->binary.reader) == NULL)		) {

            PDEBUG("LOADER:         Open stream for binary %s/%s\n", temp->prefix, temp->name);
            binary_init(temp);
        }

        /* Fetch the next element			*/
        item = item->next;
    }

    /* Unlock the list of the decoded binaries.
     */
    xanList_unlock((system->cliManager).binaries);

    /* Free the memory.
     */
    freeFunction(prefix);
    freeFunction(name);

    /* Initialize the exception manager.
     */
    EXCEPTION_initExceptionManager();

    /* Load the IR code.
     */
    PDEBUG("LOADER:         Load IR code\n");
    load_IR(system);

    /* Allocate the out of memory exception.
     */
    EXCEPTION_allocateOutOfMemoryException();

    /* Set the entry point of the program.
     */
    PDEBUG("LOADER:         Load CIL into memory structs\n");
    if ((error = internal_setProgramEntryPoint(system)) != 0) {
        return error;
    }

    PDEBUG("LOADER: Exit\n");
    return 0;
}

static inline JITBOOLEAN load_assemblies (t_system *system) {
    XanList                 *assembliesList;
    XanListItem             *item;

    /* Assertions.
     */
    assert(system != NULL);
    assert((system->cliManager).entryPoint.binary != NULL);
    PDEBUG("LOADER: load_assemblies: Start\n");

    /* Make the list of the assemblies to decode.
     */
    assembliesList = xanList_new(allocFunction, freeFunction, NULL);
    assert(assembliesList != NULL);

    /* Load and decode CIL assembies.
     */
    xanList_insert(assembliesList, (system->cliManager).entryPoint.binary);
    item = xanList_first(assembliesList);
    while (item != NULL) {
        t_binary_information            *binary;
        t_metadata                      *metadata;
        t_streams_metadata              *streams;
        t_string_stream                 *string_stream;
        t_metadata_tables               *tables;
        t_assembly_ref_table            *assembly_ref_table;
        t_row_assembly_ref_table        *row;
        JITUINT32 count;

        /* Fetch the binary information			*/
        binary = (t_binary_information *) xanList_getData(item);
        assert(binary != NULL);

        /* Fetch the Metadata				*/
        metadata = &(binary->metadata);
        assert(metadata != NULL);

        /* Fetch the Streams				*/
        streams = &(metadata->streams_metadata);
        assert(streams != NULL);

        /* Fetch the String Stream			*/
        string_stream = &(streams->string_stream);
        assert(string_stream != NULL);

        /* Fetch all the tables				*/
        tables = &((streams->not_stream).tables);
        assert(tables != NULL);

        /* Fetch the Assemblies Ref table		*/
        assembly_ref_table = get_table(tables, ASSEMBLY_REF_TABLE);
        assert(assembly_ref_table != NULL);

        /* Add all the assembly references of the current one to the queue	*/
        for (count = 1; count <= assembly_ref_table->cardinality; count++) {
            JITINT8                 *assemblyName;
            t_binary_information    *newBinary;

            /* Fetch a row from the Assembly Ref table			*/
            row = get_row(tables, ASSEMBLY_REF_TABLE, count);
            assert(row != NULL);

            /* Check to see if the current assembly referenced it is        *
             * already decoded						*/
            if (row->binary_info != NULL) {
                PDEBUG("LOADER: load_assemblies:        assembly %s already decoded\n", ((t_binary_information*)row->binary_info)->name);
                continue;
            }

            /* Fetch the name of the assembly to decode			*/
            assemblyName = get_string(string_stream, row->name);
            assert(assemblyName != NULL);

            /* Load the assembly						*/
            PDEBUG("LOADER: load_assemblies:        Loading the assembly %s\n", assemblyName);
            newBinary = loadAssembly(system, assemblyName);
            if (newBinary == NULL) {
                abort();
            }
            assert(newBinary != NULL);

            // Lock the mutex of the binary */
            PLATFORM_lockMutex(&((newBinary->binary).mutex));
            PDEBUG("LOADER: load_assemblies:        Loaded the assembly %s\n", assemblyName);

            /* Store the binary reference from the  *
             * current assemby reference to the new	*
             * binary just decoded			*/
            row->binary_info = newBinary;

            /* Insert the current binary just	*
            * decoded in the list of the binary	*
            * to check the assemblyRef table	*/
            xanList_insert(assembliesList, newBinary);
            // Unock the mutex of the binary */
            PLATFORM_unlockMutex(&((newBinary->binary).mutex));
        }

        /* Fetch the next element from the list.
         */
        item = item->next;
    }

    /* Free the memory.
     */
    xanList_destroyList(assembliesList);

    PDEBUG("LOADER: load_assemblies: Exit\n");
    return 0;
}

static inline void check_initial_conditions (t_system *system) {
    assert(system									!= NULL);
    assert((system->cliManager).entryPoint.binary					!= NULL);
    assert((*((system->cliManager).entryPoint.binary->binary.reader))->stream	!= NULL);
    assert(system->plugins								!= NULL);
}

JITBOOLEAN loader_shutdown (t_system *system) {
    PDEBUG("LOADER: Shutdown\n");
    return 0;
}

static inline t_plugins * decoder_searcher (XanList *plugins, JITINT8 *binary_ID) {
    t_plugins       *plugin;
    XanListItem     *temp_plugins;
    JITBOOLEAN found;

    /* Assertions           */
    assert(plugins != NULL);
    assert(binary_ID != NULL);

    /* Search the plugin	*/
    plugin = NULL;
    found = 0;
    temp_plugins = xanList_first(plugins);
    while (temp_plugins != NULL) {
        JITINT8 *current_binary_ID;

        /* Fetch the next plugin			*/
        plugin = (t_plugins *) temp_plugins->data;
        assert(plugin != NULL);

        /* Read the ID binary				*/
        current_binary_ID = plugin->get_ID_binary();
        assert(current_binary_ID != NULL);
        PDEBUG("LOADER:	        Current plugin has the follow binary ID=\"%s\"", current_binary_ID);

        /* Is the current plugin ok to read the binary?	*/
        if (binary_ID[0] == current_binary_ID[0] && binary_ID[1] == current_binary_ID[1]) {
            /* Found loader		*/
            PDEBUG("It is OK   :-)\n");
            found = 1;
            break;
        }
        PDEBUG("It is not ok\n");
        temp_plugins = temp_plugins->next;
    }

    if (found == 1) {
        return plugin;
    }
    return NULL;
}

static inline JITINT16 internal_setProgramEntryPoint (t_system *system) {

    /* Assertion				*/
    assert(system != NULL);

    /* Fetch the binary informations	*/
    t_binary_information *binary_info = (system->cliManager).entryPoint.binary;

    /* Check if the bytecode is a DLL	*/
    if (binary_info->PE_info.isDLL != 0x0) {
        return 0;
    }

    /* Fetch the method information of the entry point	*/
    MethodDescriptor *method = (system->cliManager).metadataManager->getEntryPointMethod((system->cliManager).metadataManager, binary_info);

    /* Insert the new method				*/
    t_methods *methods = &((system->cliManager).methods);
    (system->program).entry_point = methods->fetchOrCreateMethod(methods, method, JITTRUE);

    return 0;
}



/***************************************************************************************************************************
                                SECONDARY LEVEL FUNCTIONS
***************************************************************************************************************************/
t_binary_information * loadAssembly (t_system *system, JITINT8 *assemblyName) {
    t_binary_information            *newBinary;
    t_binary_information            *newBinary2;
    t_assembly_ref_table            *assembly_ref_table;
    t_row_assembly_ref_table        *row;
    JITINT8 binary_ID[2];
    char buffer[DIM_BUF];
    JITINT8				prefix[MAX_NAME];
    JITINT8                         *newAssemblyName;
    JITINT32 count;

    /* Assertions.
     */
    assert(system != NULL);
    assert(assemblyName != NULL);
    PDEBUG("LOADER: loadAssembly: Start\n");
    PDEBUG("LOADER: loadAssembly:	Assembly	= %s\n", assemblyName);

    /* Set the path prefix for the CIL file.
     */
    setupPrefixFromPath(system->cilLibraryPath, (char *) assemblyName, (char *) prefix);

    /* Check if the binary is already loaded.
     */
    newBinary = internal_fetchBinaryByNameAndPrefix(system, assemblyName, prefix);
    if (newBinary != NULL) {
        PDEBUG("LOADER: loadAssembly:	The assembly is already decoded\n");
        PDEBUG("LOADER: loadAssembly: Exit\n");
        return newBinary;
    }

    /* Alloc a new binary_information structure			*/
    newBinary       = ILBINARY_new();
    ILBINARY_setBinaryName(newBinary, assemblyName);
    STRNCPY(newBinary->prefix, prefix, MAX_NAME);

    /* Open the file where the assembly is stored			*/
    binary_init(newBinary);

    /* Read the binary ID			*/
    PDEBUG("LOADER: loadAssembly:	Read the bynary ID\n");
    if (ILBINARY_decodeFileType(newBinary, binary_ID) != 0) {
        snprintf(buffer, sizeof(char) * DIM_BUF, "LOADER: ERROR=During read ID of the binary \"%s\". ", assemblyName);
        print_err(buffer, 0);
        fclose((*(newBinary->binary.reader))->stream);
        freeMemory(newBinary);
        return NULL;
    }

    /* Search the decoder for the binary	*/
    PDEBUG("LOADER: loadAssembly:	Search the decoder\n");
    plugin = decoder_searcher(system->plugins, binary_ID);
    if (plugin == NULL) {
        snprintf(buffer, sizeof(char) * DIM_BUF, "LOADER: ERROR=During searching a plugin. ");
        print_err(buffer, 0);
        fclose((*(newBinary->binary.reader))->stream);
        freeMemory(newBinary);
        return NULL;
    }

    /* Decoder launcher			*/
    PDEBUG("LOADER: loadAssembly:	Run the decoder found\n");
    if (plugin->decode_image(newBinary) != 0) {
        snprintf(buffer, sizeof(char) * DIM_BUF, "LOADER: ERROR=During decoding the binary \"%s\". ", assemblyName);
        print_err(buffer, 0);
        fclose((*(newBinary->binary.reader))->stream);
        freeMemory(newBinary);
        return NULL;
    }

    /* Add the reference of the assembly to the decoder	*/
    if (xanList_find(plugin->assemblies_decoded, newBinary) == NULL) {
        PDEBUG("LOADER:		Add the current assembly to the list of the decoder\n");
        xanList_insert(plugin->assemblies_decoded, newBinary);
    }

    /* Add the new binary decoded inside the system list	*/
    PDEBUG("LOADER: loadAssembly:   Insert the assembly %s inside the binaries list\n", assemblyName);
    xanList_insert((system->cliManager).binaries, newBinary);

    /* Load all the assemblyRef				*/
    PDEBUG("LOADER: loadAssembly:   Load all the assembly reference of %s\n", assemblyName);
    assembly_ref_table = get_table(&((newBinary->metadata).streams_metadata.not_stream.tables), ASSEMBLY_REF_TABLE);
    if (assembly_ref_table != NULL) {
        for (count = 1; count <= assembly_ref_table->cardinality; count++) {

            /* Fetch a row from the Assembly Ref table			*/
            row = get_row(&((newBinary->metadata).streams_metadata.not_stream.tables), ASSEMBLY_REF_TABLE, count);
            assert(row != NULL);

            /* Check to see if the current assembly referenced it is        *
             * already decoded						*/
            if (row->binary_info != NULL) {
                continue;
            }

            /* Fetch the name of the assembly to decode			*/
            newAssemblyName = get_string(&((newBinary->metadata).streams_metadata.string_stream), row->name);
            assert(newAssemblyName != NULL);
            PDEBUG("LOADER: loadAssembly:           The assembly %s refers to the assembly %s\n", assemblyName, newAssemblyName);

            /* Check if the binary is already decoded			*/
            newBinary2 = internal_fetchBinaryByName(system, newAssemblyName);
            if (newBinary2 == NULL) {

                /* Load the assembly						*/
                PDEBUG("LOADER: loadAssembly:                   Decoding the assembly %s\n", newAssemblyName);
                newBinary2 = loadAssembly(system, newAssemblyName);
                if (newBinary2 == NULL) {
                    abort();
                }
            } else {
                PDEBUG("LOADER: loadAssembly:                   The assembly %s was already loaded\n", newAssemblyName);
            }

            /* Store the binary reference from the  *
             * current assemby reference to the new	*
             * binary just decoded			*/
            row->binary_info = newBinary2;
        }
    }
    assert(newBinary != NULL);

    PDEBUG("LOADER: loadAssembly:   The assembly %s is decoded and loaded in memory\n", assemblyName);
    PDEBUG("LOADER: loadAssembly: Exit\n");
    return newBinary;
}

static inline t_binary_information * internal_fetchBinaryByName (t_system *system, JITINT8 *assemblyName) {
    XanListItem             *item;
    t_binary_information    *binary;

    /* Assertions			*/
    assert(system != NULL);
    assert((system->cliManager).binaries != NULL);
    assert(assemblyName	!= NULL);

    item = xanList_first((system->cliManager).binaries);
    while (item != NULL) {
        binary = xanList_getData(item);
        assert(binary != NULL);

        if (STRCMP(binary->name, assemblyName) == 0) {
            return binary;
        }

        item = item->next;
    }
    return NULL;
}

static inline t_binary_information * internal_fetchBinaryByNameAndPrefix (t_system *system, JITINT8 *assemblyName, JITINT8 *assemblyPrefix) {
    XanListItem             *item;
    t_binary_information    *binary;

    /* Assertions.
     */
    assert(system != NULL);
    assert((system->cliManager).binaries != NULL);
    assert(assemblyName	!= NULL);
    assert(assemblyPrefix	!= NULL);

    /* Search for the binary.
     */
    item = xanList_first((system->cliManager).binaries);
    while (item != NULL) {
        binary = xanList_getData(item);
        assert(binary != NULL);

        if ( 	(STRCMP(binary->name, assemblyName) == 0) 	&&
                (STRCMP(binary->prefix, assemblyPrefix) == 0) 	) {
            return binary;
        }

        item = item->next;
    }

    return NULL;
}

static inline void binary_init (t_binary_information *binary_info) {
    char	full_path[256];
    FILE	*cilFile;

    /* Assertions.
     */
    assert(binary_info != NULL);
    assert(binary_info->binary.reader != NULL);

    /* Set the name of the path where the CIL file is stored.
     */
    full_path[0] = '\0';
    if ( strlen((char*)binary_info->prefix) != 0 ) {
        if (STRCAT(full_path, binary_info->prefix) == NULL) {
            abort();
        }
        if (STRCAT(full_path, "/") == NULL) {
            abort();
        }
    }

    /* Set the name of the CIL file including the full path.
     */
    if (STRCAT(full_path, binary_info->name) == NULL) {
        abort();
    }

    /* Open the CIL file.
     */
    cilFile		= fopen(full_path, "r");
    if (cilFile == NULL) {
        if (STRCAT(full_path, ".dll") == NULL) {
            abort();
        }
        cilFile		= fopen(full_path, "r");
    }
    if (cilFile == NULL) {
        char		buffer[DIM_BUF];
        snprintf(buffer, sizeof(char) * DIM_BUF, "LOADER: ERROR = Cannot find the binary \"%s\". ", binary_info->name);
        print_err(buffer, errno);
        print_err("Please check the environment variable ILDJIT_PATH which contains all the paths in which the CIL libraries are stored. ", 0);
        abort();
    }

    /* Set the CIL file.
     */
    ILBINARY_setBinaryFile(binary_info, cilFile);
    assert((*(binary_info->binary.reader))->offset == 0);
    assert((*(binary_info->binary.reader))->relative_offset == 0);

    return ;
}

static inline void load_IR (t_system *system) {
    TranslationPipeline     *pipeliner;

    /* Assertions				*/
    assert(system != NULL);

    /* Check if we are using a static compilation scheme.
     */
    if (	(!((system->IRVM).behavior.aot))		&&
            (!((system->IRVM).behavior.staticCompilation))	) {
        return ;
    }

    /* Cache the creation of virtual method tables.
     * This is necessary because in static compilation schemes, we do not have entry point addresses of methods till all machine code has been generated.
     */
    LAYOUT_setCachingRequestsForVirtualMethodTables(&((system->cliManager).layoutManager), JITTRUE);

    /* Fetch the pipeliner.
     */
    pipeliner = &(system->pipeliner);
    assert(pipeliner != NULL);

    /* Check if we need to load the profiled methods.
     */
    if ((system->IRVM).behavior.pgc) {
        XanList 	*profiled_methods;
        XanListItem	*item;
        JITUINT32	numberOfElements;

        /* Fetch the profiled methods           */
        profiled_methods 	= MANFRED_loadProfile(&(system->manfred), NULL);
        numberOfElements	= xanList_length(profiled_methods);

        /* Check if there is a profile.
         */
        if (numberOfElements > 0) {

            /* Postpone the execution of static constructors.
             */
            STATICMEMORY_cacheConstructorsToCall(&(system->staticMemoryManager));

            /* Postpone the generation of machine code.
             */
            CODE_cacheCodeGeneration(&(system->codeGenerator));

            /* Compile the profiled methods		*/
            item = xanList_first(profiled_methods);
            while (item != NULL) {
                Method method;
                method = item->data;
                pipeliner->insertMethod(pipeliner, method, MIN_METHOD_PRIORITY);
                item = item->next;
            }

            /* Wait the AOT compiler		*/
            pipeliner->waitEmptyPipeline(pipeliner);
        }

        /* Free the memory.
         */
        xanList_destroyList(profiled_methods);

        if (numberOfElements > 0) {
            return ;
        }
        fprintf(stderr, "WARNING: The profile guided compilation option will be ignored since there was no profile in the code cache.\n");
        (system->IRVM).behavior.pgc	= 0;
    }

    /* Check if we need to load the IR from the code cache.
     */
    if (MANFRED_isProgramPreCompiled(&(system->manfred))) {

        /* Load the cached IR.
         */
        MANFRED_loadProgram(&(system->manfred), NULL);
    }

    return ;
}
