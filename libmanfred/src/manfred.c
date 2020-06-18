/*
 * Copyright (C) 2010 - 2011 Campanoni Simone
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
#include <sys/types.h>
#include <errno.h>
#include <base_symbol.h>
#include <compiler_memory_manager.h>
#include <platform_API.h>
#include <codetool_types.h>

// My headers
#include <manfred.h>
#include <manfred_load.h>
#include <manfred_scanner.h>
#include <manfred_parser.tab.h>
#include <manfred_system.h>
// End

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define INITIAL_LOADED_SIZE 100
#define SCANNER_BUFFER_SIZE 4096

static inline JITINT8 * internal_getProgramDirectory (manfred_t *this);
static inline void internal_allocateSymbols (manfred_t *_this, JITUINT32 size);
static inline ir_symbol_t * methodDeserialize (void *mem, JITUINT32 memBytes);
static inline ir_symbol_t * nullDeserialize (void *mem, JITUINT32 memBytes);

static inline void serializeMethod (Method method, FILE *program_file);
static inline JITNINT internal_mkdir (JITINT8 *dir);

extern int manfred_yyparse ();

void manfred_yyerror (char *s) {
    fprintf(stderr, "MANFRED: error while loading program : %s\n", s);
    abort();
}

void MANFRED_addToSymbolToSave (manfred_t *this, ir_symbol_t *symbol) {
    xanList_syncAppend(this->symbolsToProcess, (void *) symbol);
}

void MANFRED_appendMethodToProfile (manfred_t *_this, Method method) {
    xanList_lock(_this->methodsTrace);
    if (xanList_find(_this->methodsTrace, method) == NULL) {
        xanList_append(_this->methodsTrace, method);
    }
    xanList_unlock(_this->methodsTrace);

    return ;
}

ir_symbol_t * MANFRED_loadSymbol (manfred_t *this, JITUINT32 number) {

    /* Assertions.
     */
    assert(this != NULL);

    /* Allocate space for the symbol.
     */
    internal_allocateSymbols(this, number+1);
    assert(this->loadedSymbol != NULL);

    /* Load the symbol.
     */
    if ((this->loadedSymbol)[number] == NULL) {
        JITUINT32 	tag;
        JITUINT32	fileSize;
        JITBOOLEAN	hasBeenCached;
        ir_symbol_t	*s;
        void		*fileInMemory;

        /* Check whether the serialization of the symbol has been cached.
         */
        hasBeenCached	= IRSYMBOL_isSerializationOfSymbolCached(number);
        if (!hasBeenCached) {
            JITUINT32	bytesRead;
            FILE 		*symbolFile;

            /* Fetch the name of the file to open.
             */
            JITINT8 *programDirectory = internal_getProgramDirectory(this);
            size_t symbolFileNameLen = STRLEN(programDirectory) + 9 + 4 + 1;
            JITINT8 *symbolFileName = alloca(symbolFileNameLen  * sizeof(JITINT8));
            SNPRINTF(symbolFileName, symbolFileNameLen, "%s%u.sym", programDirectory, number);

            /* Open the symbol file.
             */
            symbolFile = fopen((const char *) symbolFileName, "r");
            if (symbolFile == NULL) {
                print_err("MANFRED: error on opening an IR file. ", errno);
                fprintf(stderr, "	File %s\n", symbolFileName);
                abort();
            }

            /* Fetch the number of bytes written in the file.
             */
            fileSize		= PLATFORM_sizeOfFile(symbolFile);

            /* Read the type of the symbol.
             */
            if (fscanf(symbolFile, "%u", &tag) <= 0) {
                print_err("MANFRED: error on scanning the IR file. ", errno);
                fprintf(stderr, "	File %s\n", symbolFileName);
                abort();
            }

            /* Remove the new line character.
             */
            fseek(symbolFile, 1, SEEK_CUR);

            /* Remove what have been read from the size of the file.
             */
            fileSize	-= ILDJIT_numberOfDigits(tag);
            fileSize	-= 1;

            /* Serialize the file in memory.
             */
            fileInMemory				= allocFunction(fileSize + 1);
            bytesRead				= fread(fileInMemory, sizeof(char), fileSize, symbolFile);
            if (bytesRead != fileSize) {
                fprintf(stderr, "MANFRED: Error on reading the symbol file %s.\n" , symbolFileName);
                fprintf(stderr, "Only %u bytes have been read instead of %u\n", bytesRead, fileSize);
                print_err("ERROR: " , errno);
                abort();
            }
            ((char *)fileInMemory)[fileSize]	= '\0';

            /* Cache the serialization.
             */
            IRSYMBOL_cacheSymbolSerialization(number, tag, fileInMemory, fileSize + 1);

            /* Close the file.
             */
            fclose(symbolFile);

        } else {

            /* Fetch the cached serialization.
             */
            IRSYMBOL_getSerializationOfSymbolCached(number, &tag, &fileInMemory, &fileSize);
            fileSize--;
        }

        /* Deserialize the symbol.
         */
        s					= IRSYMBOL_deserializeSymbol(tag, fileInMemory, fileSize);
        assert(s != NULL);

        /* Set the symbol.
         */
        s->symbolIdentifier		= number;
        (this->loadedSymbol)[number] 	= s;

        /* Free the memory.
         */
        freeFunction(fileInMemory);
    }
    assert((this->loadedSymbol)[number] != NULL);

    return (this->loadedSymbol)[number];
}

static inline ir_value_t nullResolve (ir_symbol_t *symbol) {
    ir_value_t value;

    value.v = (JITNUINT) NULL;
    return value;
}

static inline void nullSerialize (ir_symbol_t * symbol, void **mem, JITUINT32 *memBytesAllocated) {

    return ;
}

static inline void nullDump (ir_symbol_t *symbol, FILE *fileToWrite) {

    fprintf(fileToWrite, "NULL pointer");
}

static inline ir_symbol_t * nullDeserialize (void *mem, JITUINT32 memBytes) {
    return manfred->nullSymbol;
}

static inline ir_value_t methodResolve (ir_symbol_t *symbol) {
    ir_value_t value;
    Method method;

    /* Initialize the variables				*/
    method = (Method) symbol->data;
    assert(method != NULL);

    value.v = (JITNUINT) method;
    return value;
}

static inline void methodSerialize (ir_symbol_t * symbol, void **mem, JITUINT32 *memBytesAllocated) {
    Method 		method;
    JITUINT32	bytesWritten;
    JITINT8		*methodName;

    /* Fetch the method.
     */
    method 		= (Method) symbol->data;
    assert(method != NULL);
    methodName	= method->getName(method);
    assert(methodName != NULL);
    JITBOOLEAN isAnonymous = method->isAnonymous(method);

    /* Compute the size of memory to allocate.
     */
    (*memBytesAllocated)	= (ILDJIT_maxNumberOfDigits(IRUINT32) + 1);
    if (isAnonymous) {
        (*memBytesAllocated)	+= STRLEN(methodName) + 2;

    } else {
        (*memBytesAllocated)	+= (ILDJIT_maxNumberOfDigits(IRUINT32) + 1);
    }
    (*memBytesAllocated)	+= 1;

    /* Allocate the memory.
     */
    (*mem)		= allocFunction(*memBytesAllocated);
    bytesWritten	= 0;

    /* Serialize the symbol.
     */
    bytesWritten	+= ILDJIT_writeIntegerValueInMemory(*mem, bytesWritten, *memBytesAllocated, (JITINT32) isAnonymous, JITTRUE);
    if (isAnonymous) {
        bytesWritten	+= ILDJIT_writeStringInMemory(*mem, bytesWritten, *memBytesAllocated, methodName, JITTRUE);
    } else {
        MethodDescriptor *methodID = method->getID(method);
        ir_symbol_t *methodIDSymbol = (manfred->cliManager)->translator.getMethodDescriptorSymbol((manfred->cliManager), methodID);
        bytesWritten	+= ILDJIT_writeIntegerValueInMemory(*mem, bytesWritten, *memBytesAllocated, methodIDSymbol->id, JITTRUE);
    }

    /* Stop the memory at the last byte written.
     */
    (*memBytesAllocated)	= bytesWritten;

    return ;
}

static inline void methodDump (ir_symbol_t *symbol, FILE *fileToWrite) {
    Method method;

    /* Initialize the variables				*/
    method = (Method) symbol->data;
    assert(method != NULL);

    fprintf(fileToWrite, "Method id %s", method->getFullName(method));
}

static inline ir_symbol_t * internal_getMethodSymbol (Method method) {
    return IRSYMBOL_createSymbol(METHOD_SYMBOL, (void *) method);
}

static inline ir_symbol_t * methodDeserialize (void *mem, JITUINT32 memBytes) {
    JITINT32 	isAnonymous;
    JITUINT32	bytesRead;
    Method 		method;

    t_methods *methods = &((manfred->cliManager)->methods);

    bytesRead		= ILDJIT_readIntegerValueFromMemory(mem, 0, &isAnonymous);
    if (isAnonymous) {
        JITINT8 	*name;

        /* Read the method name.
         */
        name		= NULL;
        bytesRead	+= ILDJIT_readStringFromMemory(mem, bytesRead, &name);

        /* Fetch the method.
         */
        method = methods->fetchOrCreateAnonymousMethod(methods, name);
        assert(method != NULL);

    } else {
        JITINT32 symbolID;
        bytesRead		+= ILDJIT_readIntegerValueFromMemory(mem, bytesRead, &symbolID);
        ir_symbol_t *loadedSymbol = MANFRED_loadSymbol(manfred, symbolID);
        MethodDescriptor *methodID = (MethodDescriptor *) (JITNUINT) (IRSYMBOL_resolveSymbol(loadedSymbol).v);
        method = methods->fetchOrCreateMethod(methods, methodID, JITTRUE); //FIXME

        /* Ensure that VTable and IMT is correctly initialized	*/
        if (methodID->attributes->is_virtual) {
            xanList_append(manfred->methodsToInitialize, method);
        }
    }

    return internal_getMethodSymbol(method);
}

static inline JITINT8 * internal_getProgramDirectory (manfred_t *this) {

    /* Assertions.
     */
    assert(this != NULL);

    /* Check if we need to open the cache directory.
     */
    if (this->programDirectory == NULL) {
        JITINT8 *user_dir;
        JITINT8 *manfred_directory = (JITINT8 *) "/.ildjit/manfred/";
        JITINT8 *buf;
        JITINT8 *buf2;
        size_t 	size;

        /* Fetch the root directory		*/
        user_dir	= (JITINT8 *)getenv("ILDJIT_CACHE");
        if (user_dir == NULL) {
            user_dir	= PLATFORM_getUserDir();
        }
        assert(user_dir != NULL);

        /* Create the base directory		*/
        size 		= STRLEN(user_dir) + 1 + 8;
        buf 		= allocFunction(size);
        SNPRINTF(buf, size, "%s%s", user_dir, "/.ildjit");
        if (internal_mkdir(buf) != 0) {
            print_err("ERROR during the creation of the directory '.ildjit'. ", errno);
            abort();
        }
        size 		+= 8;
        buf2 		= allocFunction(size);
        SNPRINTF(buf2, size, "%s/manfred/", buf);
        if (internal_mkdir(buf2) != 0) {
            print_err("ERROR during the creation of the manfred directory. ", errno);
            abort();
        }
        freeFunction(buf);
        freeFunction(buf2);

        /* Fetch the program name.
         */
        JITINT8 *program_name 	= IRPROGRAM_getProgramName();
        assert(program_name != NULL);

        /* Make the final directory name.
         */
        size 			= STRLEN(user_dir) + STRLEN(manfred_directory) + STRLEN(program_name) + 2;
        this->programDirectory 	= (JITINT8 *) allocFunction(size);
        SNPRINTF(this->programDirectory, size, "%s%s%s/", user_dir, manfred_directory, program_name);
    }

    return this->programDirectory;
}

JITBOOLEAN MANFRED_isProgramPreCompiled (manfred_t *self) {
    JITINT8 	*program_directory;
    DIR 		*ildjit_dir;

    /* Assertions.
     */
    assert(self != NULL);

    /* Check whether the condition has been already checked.
     */
    if (self->isProgramPreCompiled) {
        return (self->isProgramPreCompiled == 1);
    }

    /* Get the program directory.
     */
    program_directory 	= internal_getProgramDirectory(self);
    assert(program_directory != NULL);

    /* Open the directory.
     */
    ildjit_dir 		= opendir((const char *) program_directory);

    /* Check if the program has been precompiled.
     */
    if (ildjit_dir == NULL) {
        self->isProgramPreCompiled	= 2;
        return JITFALSE;
    }
    closedir(ildjit_dir);
    self->isProgramPreCompiled	= 1;

    return JITTRUE;
}

void serializeTypeInfos (TypeDescriptor *type,  FILE *program_file) {
    if (type != NULL) {
        ir_symbol_t  *typeSymbol = (manfred->cliManager)->translator.getTypeDescriptorSymbol(manfred->cliManager, type);
        fprintf(program_file, "%u", typeSymbol->id);
    } else {
        fprintf(program_file, "%u", manfred->nullSymbol->id);
    }
}

void serializeSignature (ir_signature_t *signature, FILE *program_file) {
    fprintf(program_file,"[");
    fprintf(program_file,"[");
    IRMETHOD_dumpIRType(signature->resultType, program_file);
    serializeTypeInfos(signature->ilResultType, program_file);
    fprintf(program_file, "]");
    fprintf(program_file, "(");
    if (signature->parametersNumber > 0) {
        JITUINT32 	count;
        TypeDescriptor	*ilType;
        fprintf(program_file,"[");
        IRMETHOD_dumpIRType(signature->parameterTypes[0], program_file);
        ilType	= NULL;
        if (signature->ilParamsTypes != NULL) {
            ilType	= signature->ilParamsTypes[0];
        }
        serializeTypeInfos(ilType, program_file);
        fprintf(program_file, "]");
        for (count = 1; count < signature->parametersNumber; count++) {
            fprintf(program_file,"[");
            IRMETHOD_dumpIRType(signature->parameterTypes[count], program_file);
            ilType	= NULL;
            if (signature->ilParamsTypes != NULL) {
                ilType	= signature->ilParamsTypes[count];
            }
            serializeTypeInfos(ilType, program_file);
            fprintf(program_file, "]");
        }
    }
    fprintf(program_file, ")");
    fprintf(program_file, "]");
}

void serializeIRItem (ir_item_t *item, FILE *program_file) {

    if (item->type != NOPARAM) {
        fprintf(program_file, "[");
        IRMETHOD_dumpIRType(item->type, program_file);
        if (item->type == IRCLASSID) {
            TypeDescriptor *type = (TypeDescriptor *) (JITNUINT) (item->value).v;
            ir_symbol_t *typeSymbol = (manfred->cliManager)->translator.getTypeDescriptorSymbol(manfred->cliManager, type);
            fprintf(program_file, "%u", typeSymbol->id);
        } else if (item->type == IRMETHODID) {
            Method method = (Method) (JITNUINT) (item->value).v;
            ir_symbol_t *methodSymbol = internal_getMethodSymbol(method);
            fprintf(program_file, "%d", methodSymbol->id);
        } else if (item->type == IRSTRING) {
            fprintf(program_file, "NOPARAM");
        } else if (item->type == IRSIGNATURE) {
            ir_signature_t *signature = (ir_signature_t *) (JITNUINT) (item->value).v;
            serializeSignature(signature, program_file);
        } else if (item->type == IRTYPE) {
            IRMETHOD_dumpIRType((JITUINT32) (item->value).v, program_file);
            serializeTypeInfos(item->type_infos, program_file);
        } else if (item->type == IRSYMBOL) {
            ir_symbol_t *symbol = (ir_symbol_t *) (JITNUINT) (item->value).v;
            assert(symbol != NULL);
            IRMETHOD_dumpIRType(item->internal_type, program_file);
            fprintf(program_file, "%u", symbol->id);
            fprintf(program_file, " ");
            serializeTypeInfos(item->type_infos, program_file);
        } else if (item->type == IRLABELITEM) {
            fprintf(program_file, "%llu", (item->value).v);
        } else if (item->type == IROFFSET) {
            IRMETHOD_dumpIRType(item->internal_type, program_file);
            fprintf(program_file, "%llu", (item->value).v);
            fprintf(program_file, " ");
            serializeTypeInfos(item->type_infos, program_file);
        } else {
            fprintf(program_file, "%llu", (item->value).v);
            fprintf(program_file, " ");
            serializeTypeInfos(item->type_infos, program_file);
        }
        fprintf(program_file, "]");
    } else {
        fprintf(program_file, "NOPARAM");
    }
}

void serializeInstructions (ir_method_t *method, FILE *program_file) {
    JITUINT32 numberOfInstruction = IRMETHOD_getInstructionsNumber(method);
    JITUINT32 count;

    for (count = 0; count < numberOfInstruction; count++) {
        JITUINT32 count2;

        ir_instruction_t* instruction = IRMETHOD_getInstructionAtPosition(method, count);
        assert(count == instruction->ID);
        fprintf(program_file, "%s ", IRMETHOD_getInstructionTypeName(instruction->type));
        serializeIRItem(&(instruction->result), program_file);
        ir_item_t *parameters[3] = { (&instruction->param_1), (&instruction->param_2), (&instruction->param_3) };
        for (count2 = 0; count2 < 3; count2++) {
            fprintf(program_file, " ");
            serializeIRItem(parameters[count2], program_file);
        }
        if (instruction->callParameters  != NULL) {
            fprintf(program_file, " (");
            XanListItem *item = xanList_first(instruction->callParameters);
            if (item != NULL) {
                ir_item_t *irItem = (ir_item_t *) item->data;
                serializeIRItem(irItem, program_file);
                item = item->next;
                while (item != NULL) {
                    irItem = (ir_item_t *) item->data;
                    fprintf(program_file, " ");
                    serializeIRItem(irItem, program_file);
                    item = item->next;
                }
            }
            fprintf(program_file, ") ");
        } else {
            fprintf(program_file, " () ");
        }
        fprintf(program_file,"%u ", instruction->byte_offset);
        fprintf(program_file,"%u", instruction->flags);
        fprintf(program_file, "\n");
    }

    return ;
}

static inline void internal_saveSymbols (manfred_t *this, JITINT8 *programDirectory) {
    XanListItem 		*item;
    size_t 			symbolFileNameLen;
    JITINT8 		*symbolFileName;

    /* Assertions.
     */
    assert(this != NULL);
    assert(programDirectory != NULL);

    /* Allocate the memory to store the name of the symbol.
     */
    symbolFileNameLen 	= STRLEN(programDirectory) + 9 + 4 + 1;
    symbolFileName 		= alloca(symbolFileNameLen  * sizeof(JITINT8));

    /* Serialize the symbols.
     */
    item = xanList_first(this->symbolsToProcess);
    while (item != NULL) {
        ir_symbol_t 	*symbol;
        FILE	 	*symbolFile;
        void		*symInMemory;
        JITUINT32	symInMemoryBytes;

        /* Fetch the symbol.
         */
        symbol 	= (ir_symbol_t *) item->data;
        assert(symbol != NULL);

        /* Open the file.
         */
        SNPRINTF(symbolFileName, symbolFileNameLen, "%s%u.sym", programDirectory, symbol->id);
        symbolFile 	= fopen((const char *) symbolFileName, "w");
        if (symbolFile == NULL) {
            fprintf(stderr, "ILDJIT: ERROR on opening file %s\n", symbolFileName);
            print_err("", errno);
            abort();
        }

        /* Dump the symbol tag.
         */
        fprintf(symbolFile, "%u\n", symbol->tag);

        /* Serialize the symbol.
         */
        symInMemory		= NULL;
        symInMemoryBytes	= 0;
        IRSYMBOL_serializeSymbol(symbol, &symInMemory, &symInMemoryBytes);
        if (symInMemoryBytes > 0) {
            JITUINT32	bytesWritten;
            assert(symInMemory != NULL);
            bytesWritten		= fwrite(symInMemory, 1, symInMemoryBytes, symbolFile);
            if (bytesWritten != symInMemoryBytes) {
                fprintf(stderr, "ILDJIT: ERROR on writing file %s. Only %u Bytes out of %u have been written.\n", symbolFileName, bytesWritten, symInMemoryBytes);
                print_err("", errno);
                abort();
            }

            /* Free the memory.
             */
            freeFunction(symInMemory);
        }

        /* Close the file.
         */
        fclose(symbolFile);

        /* Fetch the next element.
         */
        item 	= item->next;
    }

    return ;
}

void serializeLocal (ir_local_t *local, FILE *program_file) {
    fprintf(program_file,"[");
    IRMETHOD_dumpIRType(local->internal_type, program_file);
    serializeTypeInfos(local->type_info, program_file);
    fprintf(program_file,"]");
}

void serializeMethod (Method method, FILE *program_file) {
    XanListItem 	*item;
    ir_symbol_t 	*methodSymbol;

    methodSymbol 	= internal_getMethodSymbol(method);
    XanList *cctors = method->getCctorMethodsToCall(method);
    XanList *locals = method->getLocals(method);
    ir_signature_t *signature = method->getSignature(method);
    ir_method_t* ir_method = method->getIRMethod(method);
    JITUINT32 max_vars = method->getMaxVariables(method);

    fprintf(program_file, "BEGIN %u\n", methodSymbol->id);

    fprintf(program_file, "CCTOR (");
    item = xanList_first(cctors);
    if (item != NULL) {
        Method cctor = (Method) item->data;
        ir_symbol_t *cctorSymbol = internal_getMethodSymbol(cctor);
        fprintf(program_file, "%u", cctorSymbol->id);
        item = item->next;
        while (item != NULL) {
            cctor = (Method) item->data;
            cctorSymbol = internal_getMethodSymbol(cctor);
            fprintf(program_file, " ");
            fprintf(program_file, "%u",cctorSymbol->id);
            item = item->next;
        }
    }
    fprintf(program_file, ")\n");

    fprintf(program_file, "SIGNATURE ");
    serializeSignature(signature, program_file);
    fprintf(program_file, "\n");

    fprintf(program_file, "LOCALS (");
    item = xanList_first(locals);
    if (item != NULL) {
        ir_local_t *local = (ir_local_t *) item->data;
        serializeLocal(local, program_file);
        item = item->next;
        while (item != NULL) {
            local = (ir_local_t *) item->data;
            fprintf(program_file, " ");
            serializeLocal(local, program_file);
            item = item->next;
        }
    }
    fprintf(program_file, ")\n");

    fprintf(program_file, "MAXVAR ");
    fprintf(program_file, "%u", max_vars);
    fprintf(program_file, "\n");

    serializeInstructions(ir_method, program_file);

    fprintf(program_file, "END\n");

    return ;
}

void MANFRED_saveProfile (manfred_t *_this, JITINT8 *directoryName) {
    JITINT8 *programDirectory;
    JITINT32 status;

    /* Fetch the directory name.
     */
    if (directoryName == NULL) {
        programDirectory = internal_getProgramDirectory(_this);
    } else {
        programDirectory = directoryName;
    }
    assert(programDirectory != NULL);

    /* Destroy directories in case it exists already.
     */
    PLATFORM_rmdir_recursively(programDirectory);

    /* Create the directory.
     */
    status = internal_mkdir(programDirectory);
    if (status != 0) {
        print_err("MANFRED: Error creating the code cache directory. ", errno);
        abort();
    }

    size_t codesLen = STRLEN(programDirectory)+10+1;
    JITINT8 *codes = alloca(codesLen  * sizeof(JITINT8) );
    SNPRINTF(codes, codesLen, "%sprofile.ir", programDirectory);
    FILE *methodsIRFile = fopen((const char *) codes, "w");
    assert(methodsIRFile != NULL);

    if ((_this->cliManager->IRVM->behavior).verbose) {
        printf("Caching traced methods\n");
    }
    XanListItem *item = xanList_first(_this->methodsTrace);
    while (item!=NULL) {
        Method 		method;
        ir_method_t	*irMethod;

        /* Fetch the method.
         */
        method 		= (Method) item->data;
        assert(method != NULL);
        irMethod	= method->getIRMethod(method);
        assert(irMethod != NULL);

        if (method->isIrImplemented(method)) {
            if ((_this->cliManager->IRVM->behavior).verbose) {
                JITINT8 	*name;
                name		= IRMETHOD_getSignatureInString(irMethod);
                if (name == NULL) {
                    name	= irMethod->name;
                }
                printf("	Caching %s\n", name);
            }
            ir_symbol_t *methodSymbol = internal_getMethodSymbol(method);
            fprintf(methodsIRFile, "%u", methodSymbol->id);
            if (item->next != NULL) {
                fprintf(methodsIRFile, "\n");
            }
        }
        item = item->next;
    }

    /* Close the file.
     */
    fclose(methodsIRFile);

    /* Save all symbols.
     */
    internal_saveSymbols(_this, programDirectory);

    return ;
}

void MANFRED_saveProgram (manfred_t *_this, JITINT8 *directoryName) {
    t_methods 	*methods;
    JITINT8 	*programDirectory;
    JITINT32 	status;

    /* Assertions.
     */
    assert(_this != NULL);

    /* Fetch the set of methods.
     */
    methods = &((manfred->cliManager)->methods);
    if ((_this->cliManager->IRVM->behavior).verbose) {
        printf("ILDJIT: Start saving IR program\n");
    }

    /* Fetch the directory name.
     */
    if (directoryName == NULL) {
        programDirectory = internal_getProgramDirectory(_this);
    } else {
        programDirectory = directoryName;
    }
    assert(programDirectory != NULL);

    /* Clean the previous produced files.
     */
    if (_this->loadedSymbol != NULL) {
        freeFunction(_this->loadedSymbol);
        _this->loadedSymbol = NULL;
    }
    status = PLATFORM_rmdir_recursively(programDirectory);
    if (status != 0) {
        JITINT8 buf[DIM_BUF];
        SNPRINTF(buf, DIM_BUF, "MANFRED: ERROR on removing the cache directory %s. ", programDirectory);
        print_err((char *) buf, errno);
        abort();
    }

    /* Create the directory.
     */
    status = internal_mkdir(programDirectory);
    if (status != 0) {
        JITINT8 buf[DIM_BUF];
        SNPRINTF(buf, DIM_BUF, "MANFRED: ERROR on creating the cache directory %s. ", programDirectory);
        print_err((char *) buf, errno);
        abort();
    }

    /* Create the methods file.
     */
    size_t codesLen = STRLEN(programDirectory)+10+1;
    JITINT8 *codes = alloca(codesLen  * sizeof(JITINT8) );
    SNPRINTF(codes, codesLen, "%smethods.ir", programDirectory);
    FILE *methodsIRFile = fopen((const char *) codes, "w");
    if (methodsIRFile == NULL) {
        JITINT8 buf[DIM_BUF];
        SNPRINTF(buf, DIM_BUF, "MANFRED: ERROR on opening the file %s. ", codes);
        print_err((char *) buf, errno);
        abort();
    }

    /* Serialize to the IR cache CIL methods.
     */
    if ((_this->cliManager->IRVM->behavior).verbose) {
        printf("ILDJIT: 	Caching normal methods\n");
    }
    XanListItem *item = xanList_first(methods->container);
    while (item!=NULL) {
        ir_method_t	*irMethod;
        Method 		method;
        method = (Method) item->data;
        irMethod	= method->getIRMethod(method);
        assert(irMethod != NULL);

        if (	(method->isIrImplemented(method))		&&
                (IRMETHOD_hasMethodInstructions(irMethod))	) {
            if ((_this->cliManager->IRVM->behavior).verbose) {
                JITINT8 	*name;
                name		= IRMETHOD_getSignatureInString(irMethod);
                if (name == NULL) {
                    name	= irMethod->name;
                }
                printf("ILDJIT:			Caching %s\n", name);
            }
            serializeMethod(method, methodsIRFile);
            fprintf(methodsIRFile, "\n");
        }
        item = item->next;
    }

    /* Serialize to the IR cache not-CIL methods.
     */
    if ((_this->cliManager->IRVM->behavior).verbose) {
        printf("ILDJIT: 	Caching anonymous methods\n");
    }
    XanHashTableItem *hashItem = xanHashTable_first(methods->anonymousContainer);
    while (hashItem!=NULL) {
        ir_method_t	*irMethod;
        Method 		method;
        method = (Method) hashItem->element;
        irMethod	= method->getIRMethod(method);
        assert(irMethod != NULL);

        if (	(method->isIrImplemented(method))		&&
                (IRMETHOD_hasMethodInstructions(irMethod))	) {
            if ((_this->cliManager->IRVM->behavior).verbose) {
                JITINT8 	*name;
                ir_method_t	*irMethod;
                irMethod	= method->getIRMethod(method);
                assert(irMethod != NULL);
                name		= IRMETHOD_getSignatureInString(irMethod);
                if (name == NULL) {
                    name	= irMethod->name;
                }
                printf("ILDJIT:			Caching %s\n", name);
            }
            serializeMethod(method, methodsIRFile);
            fprintf(methodsIRFile, "\n");
        }
        hashItem = xanHashTable_next(methods->anonymousContainer, hashItem);
    }

    /* Close the file.
     */
    fclose(methodsIRFile);
    if ((_this->cliManager->IRVM->behavior).verbose) {
        printf("ILDJIT: End\n");
    }

    /* Save the symbols.
     */
    internal_saveSymbols(_this, programDirectory);

    return ;
}

JITINT32 file_select (const struct dirent *entry) {
    JITINT32 result = 1;

    if ((strcmp(entry->d_name, ".") == 0) || (strcmp(entry->d_name, "..") == 0)  || (strcmp(entry->d_name, "methods.ir") == 0)) {
        result = 0;
    }
    return result;
}

XanList * MANFRED_loadProfile (manfred_t *_this, JITINT8 *directoryName) {
    JITINT8 *programDirectory;
    struct dirent **entries;
    JITINT8 line[50];

    if (directoryName == NULL) {
        programDirectory = internal_getProgramDirectory(_this);
    } else {
        programDirectory = directoryName;
    }
    assert(programDirectory != NULL);

    XanList *result = xanList_new(allocFunction, freeFunction, NULL);
    JITUINT32 count = PLATFORM_scandir((char *) programDirectory, &entries, file_select, NULL);
    if (count == -1) {
        return result;
    }

    /* Allocate the space for the name of the file to open.
     */
    size_t codesLen = STRLEN(programDirectory)+10+1;
    JITINT8 *codes = alloca(codesLen);
    SNPRINTF(codes, codesLen, "%sprofile.ir", programDirectory);

    /* Open the file.
     */
    FILE *methodsIRFile = fopen((const char *) codes, "r");
    if (methodsIRFile == NULL) {

        /* Free the memory.
         */
        while (count--){
            freeFunction(entries[count]);
        }
        freeFunction(entries);

        return result;
    }
    assert(methodsIRFile != NULL);

    /* Allocate space for loaded symbols.
     */
    internal_allocateSymbols(_this, count+1);
    while (fgets((char *) line, sizeof(line),methodsIRFile) != NULL) {
        JITUINT32 methodSymbolID = atoi((char *) line);
        assert(methodSymbolID != 0);
        ir_symbol_t *methodSymbol = IRSYMBOL_loadSymbol(methodSymbolID);
        Method method = (Method) (JITNUINT) (IRSYMBOL_resolveSymbol(methodSymbol).v);
        xanList_append(result, method);
    }

    XanListItem *item = xanList_first(_this->methodsToInitialize);
    while (item != NULL) {
        Method method = (Method) item->data;
        MethodDescriptor *methodID = method->getID(method);
        (manfred->cliManager)->layoutManager.prepareVirtualMethodInvocation(&((manfred->cliManager)->layoutManager), methodID);
        item = item->next;
    }
    fclose(methodsIRFile);

    /* Free the memory.
     */
    while (count--){
        freeFunction(entries[count]);
    }
    freeFunction(entries);

    return result;
}

void MANFRED_loadProgram (manfred_t *_this, JITINT8 *directoryName) {
    JITINT8 	*programDirectory;
    struct dirent 	**entries;
    JITUINT32 	count;
    JITUINT32 	symID;

    _this->isLoading = JITTRUE;

    if (directoryName == NULL) {
        programDirectory = internal_getProgramDirectory(_this);
    } else {
        programDirectory = directoryName;
    }
    assert(programDirectory != NULL);

    count 			= PLATFORM_scandir((char *) programDirectory, &entries, file_select, NULL);
    if (count < 0) {
        print_err("ERROR: Manfred was not able to scan the code cache directory. ", errno);
        fprintf(stderr, "Directory used: %s\n", programDirectory);
        abort();
    }
    internal_allocateSymbols(_this, count+1);

    size_t codesLen = STRLEN(programDirectory)+10+1;
    JITINT8 *codes = alloca(codesLen  * sizeof(JITINT8) );
    SNPRINTF(codes, codesLen, "%smethods.ir", programDirectory);
    FILE *methodsIRFile = fopen((const char *) codes, "r");
    if (methodsIRFile == NULL) {
        print_err("ERROR: Manfred was not able to load the \"methods.ir\" file.\n", errno);
        fprintf(stderr, "Directory used: %s\n", programDirectory);
        abort();
    }

    YY_BUFFER_STATE buffer = manfred_yy_create_buffer(methodsIRFile, SCANNER_BUFFER_SIZE);
    manfred_yy_switch_to_buffer(buffer);
    manfred_yyparse();
    manfred_yy_delete_buffer(buffer);

    XanListItem *item = xanList_first(_this->methodsToInitialize);
    while (item != NULL) {
        Method method = (Method) item->data;
        MethodDescriptor *methodID = method->getID(method);
        (manfred->cliManager)->layoutManager.prepareVirtualMethodInvocation(&((manfred->cliManager)->layoutManager), methodID);
        item = item->next;
    }
    fclose(methodsIRFile);
    _this->isLoading = JITFALSE;

    /* Load the symbols.
     */
    for (symID=1; symID < count; symID++) {
        IRSYMBOL_loadSymbol(symID);
    }

    /* Free the memory.
     */
    while (count--) {
        freeFunction(entries[count]);
    }
    freeFunction(entries);

    return ;
}

void MANFRED_shutdown (manfred_t *self) {
    if (self->programDirectory != NULL) {
        freeFunction(self->programDirectory);
        self->programDirectory		= NULL;
    }
    if (self->symbolsToProcess != NULL) {
        xanList_destroyList(self->symbolsToProcess);
        self->symbolsToProcess		= NULL;
    }
    if (self->methodsToInitialize != NULL) {
        xanList_destroyList(self->methodsToInitialize);
        self->methodsToInitialize	= NULL;
    }
    if (self->methodsTrace != NULL) {
        xanList_destroyList(self->methodsTrace);
        self->methodsTrace		= NULL;
    }
    if (self->loadedSymbol != NULL) {
        freeFunction(self->loadedSymbol);
        self->loadedSymbol		= NULL;
    }

    return ;
}

void MANFRED_init (manfred_t *self, CLIManager_t *cliManager) {

    manfred = self;
    manfred->cliManager = cliManager;
    self->programDirectory = NULL;
    self->loadedSymbol = NULL;
    self->isLoading = JITFALSE;
    self->symbolsToProcess = xanList_new(allocFunction, freeFunction, NULL);
    self->methodsToInitialize = xanList_new(allocFunction, freeFunction, NULL);
    self->methodsTrace = xanList_new(allocFunction, freeFunction, NULL);

    IRSYMBOL_registerSymbolManager(NULL_SYMBOL, nullResolve, nullSerialize, nullDump, nullDeserialize);
    IRSYMBOL_registerSymbolManager(METHOD_SYMBOL, methodResolve, methodSerialize, methodDump, methodDeserialize);
    self->nullSymbol = IRSYMBOL_createSymbol(NULL_SYMBOL, NULL);

    return ;
}

static inline JITNINT internal_mkdir (JITINT8 *dir) {
    JITINT8 tmp[DIM_BUF];
    JITINT8 *p = NULL;
    size_t len;
    JITNINT status;

    status = 0;
    SNPRINTF(tmp, DIM_BUF, "%s", dir);
    len = STRLEN(tmp);
    if (tmp[len - 1] == '/') {
        tmp[len - 1] = 0;
    }

    for (p = tmp + 1; *p; p++) {
        if (*p != '/') {
            continue;
        }
        *p = 0;
        status	= PLATFORM_mkdir((const char *) tmp, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        if (	(status != 0)		&&
                (errno == EEXIST)	) {
            errno	= 0;
            status	= 0;
        }
        if (status != 0) {
            fprintf(stderr, "ILDJIT: ERROR while creating the directory %s\n", tmp);
            return status;
        }
        *p = '/';
    }
    status = PLATFORM_mkdir((const char *) tmp, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if (	(status != 0)		&&
            (errno == EEXIST)	) {
        errno	= 0;
        status	= 0;
    }
    return status;
}

void libmanfredCompilationFlags (char *buffer, JITINT32 bufferLength) {

    /* Assertions				*/
    assert(buffer != NULL);

    snprintf((char *) buffer, sizeof(char) * bufferLength, " ");
#ifdef DEBUG
    strncat((char *) buffer, "DEBUG ", sizeof(char) * bufferLength);
#endif
#ifdef PRINTDEBUG
    strncat((char *) buffer, "PRINTDEBUG ", sizeof(char) * bufferLength);
#endif
#ifdef PROFILE
    strncat((char *) buffer, "PROFILE ", sizeof(char) * bufferLength);
#endif
}

void libmanfredCompilationTime (char *buffer, JITINT32 bufferLength) {

    /* Assertions				*/
    assert(buffer != NULL);

    snprintf(buffer, sizeof(char) * bufferLength, "%s %s", __DATE__, __TIME__);
}

char * libmanfredVersion () {
    return VERSION;
}

static inline void internal_allocateSymbols (manfred_t *_this, JITUINT32 size) {
    if (_this->loadedSymbolSize >= size) {
        return ;
    }
    if (_this->loadedSymbol == NULL) {
        _this->loadedSymbol	= (ir_symbol_t **) allocFunction(sizeof(ir_symbol_t *) * size);
    }

    /* Expand the memory.
     */
    _this->loadedSymbol 	= (ir_symbol_t **) dynamicReallocFunction(_this->loadedSymbol, sizeof(ir_symbol_t *) * size);

    /* Erase the new memory.
     */
    memset(&(_this->loadedSymbol[_this->loadedSymbolSize]), 0, (size - _this->loadedSymbolSize) * sizeof(ir_symbol_t *));

    /* Set the new size.
     */
    _this->loadedSymbolSize	= size;

    return ;
}
