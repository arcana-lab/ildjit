/*
 * Copyright (C) 2006  Campanoni Simone
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
 * @file iljit_ecmasoft_decoder.c
 *
 */
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <config.h>
#include <compiler_memory_manager.h>

// My header
#include <iljit_ecmasoft_decoder.h>
// End

/**
 *
 * Return the identificator of the assembly that this plugin can decodes
 */
static inline JITINT8 * get_ID_binary (void);

char * ecmasoft_getVersion (void);
char * ecmasoft_getName (void);
char * ecmasoft_getAuthor (void);

/**
 *
 * Decode the assembly referenced by the binary_info parameter
 */
JITBOOLEAN EcmaSoft_decode_image (t_binary_information *binary_info);

/**
 *
 * Init the decoder
 */
static inline JITBOOLEAN decoder_init (t_binary_information *binary_info);

/**
 *
 * Shutdown the decoder
 */
static inline JITBOOLEAN decoder_shutdown (t_binary_information *info);

/**
 *
 * Decode the MS-DOS Header and stores the information in the binary_info struct
 */
static inline JITINT32 load_MSDOS_header (t_binary_information *binary_info);

/**
 *
 * Decode the MS-DOS Stub and stores the information in the binary_info struct
 */
static inline JITINT32 load_MSDOS_stub (t_binary_information *binary_info);

/**
 *
 * Decode the PE Header and stores the information in the binary_info struct
 */
static inline JITINT32 load_PE_header (t_binary_information *binary_info);

/**
 *
 * Decode the PE Optional Header and stores the information in the binary_info struct
 */
static inline JITINT32 load_PEOPTIONAL_header (t_binary_information *binary_info);

/**
 *
 * Decode the IAT and stores the information in the binary_info struct
 */
static inline JITINT32 load_IAT (t_binary_information *binary_info);

/**
 *
 * Decode the Relocation Table and stores the information in the binary_info struct
 */
static inline JITINT32 load_Relocation_table (t_binary_information *binary_info);

/**
 *
 * Decode the CLI Header and stores the information in the binary_info struct
 */
static inline JITINT32 load_CLI_header (t_binary_information *binary_info, JITUINT32 min_address);

/**
 *
 * Decode al the sections and stores the information in the binary_info struct
 */
static inline JITINT32 load_SECTIONS_header_table (t_binary_information *binary_info, JITUINT32 *min_address, JITUINT32 base);

/**
 *
 * Decode all the metadata and stores the information in the binary_info struct
 */
static inline JITINT32 load_metadata (t_binary_information *binary_info);


static inline JITINT32 load_assembly_references (t_binary_information *binary_info);
static inline void ecmasoft_compilationTime (char *buffer, JITINT32 bufferLength);
static inline void ecmasoft_compilationFlags (char *buffer, JITINT32 bufferLength);

t_plugin_interface decoder_interface = {
    get_ID_binary,
    EcmaSoft_decode_image,
    decoder_shutdown,
    ecmasoft_getName,
    ecmasoft_getVersion,
    ecmasoft_getAuthor,
    ecmasoft_compilationFlags,
    ecmasoft_compilationTime
};

static inline JITINT8 * get_ID_binary (void) {
    return (JITINT8 *) BINARY_ID;
}

JITBOOLEAN EcmaSoft_decode_image (t_binary_information *binary_info) {
    JITUINT32 min_address;
    JITUINT32 base;

    /* Assertions			*/
    assert(binary_info != NULL);

    /* Decoder init			*/
    decoder_init(binary_info);

    /* Load MSDOS Header		*/
    if (load_MSDOS_header(binary_info) != 0) {
        decoder_shutdown(binary_info);
        return -1;
    }
    base = binary_info->MSDOS_header.base;
#ifdef PRINTDEBUG
    print_binary_offset(&(binary_info->binary));
#endif

    /* Load MSDOS Stub		*/
    if (load_MSDOS_stub(binary_info) != 0) {
        decoder_shutdown(binary_info);
        return -1;
    }
#ifdef PRINTDEBUG
    print_binary_offset(&(binary_info->binary));
#endif

    /* Load PE Header		*/
    if (load_PE_header(binary_info) != 0) {
        decoder_shutdown(binary_info);
        return -1;
    }
#ifdef PRINTDEBUG
    print_binary_offset(&(binary_info->binary));
#endif

    /* Load PE Optional Header	*/
    if (binary_info->PE_info.optional_header_size != 0) {
        if (load_PEOPTIONAL_header(binary_info) != 0) {
            decoder_shutdown(binary_info);
            return -1;
        }
#ifdef PRINTDEBUG
        print_binary_offset(&(binary_info->binary));
#endif
    }

    /* Load all the sections	*/
    if (load_SECTIONS_header_table(binary_info, &min_address, base) != 0) {
        decoder_shutdown(binary_info);
        return -1;
    }
    ;
#ifdef PRINTDEBUG
    print_binary_offset(&(binary_info->binary));
#endif
    /* Load IAT      */
    if (load_IAT(binary_info) != 0) {
        decoder_shutdown(binary_info);
        return -1;
    }
    /* Load Relocation Table      */
    if (load_Relocation_table(binary_info) != 0) {
        decoder_shutdown(binary_info);
        return -1;
    }
    /* Load CLI Header		*/
    if (load_CLI_header(binary_info, min_address) != 0) {
        decoder_shutdown(binary_info);
        return -1;
    }
#ifdef PRINTDEBUG
    print_binary_offset(&(binary_info->binary));
#endif

    /* Load Metadata		*/
    if (load_metadata(binary_info) != 0) {
        decoder_shutdown(binary_info);
        return -1;
    }
#ifdef PRINTDEBUG
    print_binary_offset(&(binary_info->binary));
#endif

    /* Load assembly references	*/
    if (load_assembly_references(binary_info) != 0) {
        print_err("DECODER:		ERROR= Cannot load libraries. ", 0);
        decoder_shutdown(binary_info);
        return -1;
    }

    return 0;
}

static inline JITBOOLEAN decoder_init (t_binary_information *binary_info) {
    if (sections_init(&(binary_info->sections)) != 0) {
        return -1;
    }
    (*(binary_info->binary.reader))->offset              = 0;
    (*(binary_info->binary.reader))->relative_offset     = 0; //FIXME call the file_init
    return 0;
}

static inline JITBOOLEAN decoder_shutdown (t_binary_information *info) {
    PDEBUG("DECODER: Decoder shutting down\n");
    assert(info != NULL);
    sections_shutdown(&(info->sections));
    metadata_shutdown(&(info->metadata));
    PDEBUG("DECODER: Done\n");
    return 0;
}

/****************************************************************************************************************************
                                                SECOND LEVEL FUNCTIONS
****************************************************************************************************************************/
static inline JITINT32 load_MSDOS_header (t_binary_information *binary_info) {
    JITINT8 buffer[DIM_BUF];

    /* Assertions			*/
    assert(binary_info != NULL);
    assert((*(binary_info->binary.reader))->stream != NULL);

    /* Read the header		*/
    PDEBUG("DECODER: MS-DOS Header\n");
    if (il_read(buffer, 2, &(binary_info->binary)) != 0) {
        PDEBUG("DECODER: ERROR= Cannot read the binary\n");
        print_err("ECMASOFT DECODER: load_MSDOS_header: ERROR = Error reading MSDOS header. ", 0);
        return -1;
    }
    if (buffer[0] != 'M' || buffer[1] != 'Z') {
        print_err("DECODER: ERROR= The MS-DOS header is not standard. ", 0);
        PDEBUG("DECODER:	Byte 0=%c\n", buffer[0]);
        PDEBUG("DECODER:	Byte 1=%c\n", buffer[1]);
        return -1;
    }
    if (il_read(buffer + 2, 62, &(binary_info->binary)) != 0) {
        print_err("ECMASOFT DECODER: load_MSDOS_header: ERROR = Error reading MSDOS header. ", 0);
        return -1;
    }

    /* Read the "ifanew"		*/
    read_from_buffer(buffer, 60, 4, &(binary_info->MSDOS_header.base));
    if ((binary_info->MSDOS_header.base) < (*(binary_info->binary.reader))->offset) {
        print_err("DECODER: ERROR= The offset if grater than the base. ", 0);
        return -1;
    }
#ifdef PRINTDEBUG
    print_binary_offset(&(binary_info->binary));
#endif
    PDEBUG("DECODER:		base				=	0x%X		%d\n", (JITUINT32) (binary_info->MSDOS_header.base), binary_info->MSDOS_header.base);

    /* Return			*/
    return 0;
}

static inline JITINT32 load_MSDOS_stub (t_binary_information *binary_info) {
    JITINT8 buffer[DIM_BUF];
    JITUINT32 diff;

    /*	Assertion	*/
    assert(binary_info != NULL);

    /* Read the MS-DOS stub	*/
    PDEBUG("DECODER: MS-DOS Stub\n");
    if (il_read(buffer, 64, &(binary_info->binary)) != 0) {
        print_err("ECMASOFT DECODER: load_MSDOS_stub: ERROR = Error reading MSDOS stub. ", 0);
        return -1;
    }
    assert((binary_info->MSDOS_header.base) >= (*((binary_info->binary).reader))->offset);

    /* Read till the PE     *
     * signature		*/
    diff = ((binary_info->MSDOS_header.base) - (*((binary_info->binary).reader))->offset);
    if (diff > 0) {
        if (il_read(buffer, diff, &(binary_info->binary)) != 0) {
            print_err("ECMASOFT DECODER: load_MSDOS_stub: ERROR = Error reading MSDOS stub. ", 0);
            return -1;
        }
    }

    /* Return		*/
    return 0;
}

static inline JITINT32 load_PE_header (t_binary_information *binary_info) {
    JITINT8 buffer[DIM_BUF];

    /*	Assertion		*/
    assert(binary_info != NULL);

    /* Check the PE signature	*/
    PDEBUG("DECODER: PE File Header\n");
    if (il_read(buffer, 4, &(binary_info->binary)) != 0) {
        print_err("ECMASOFT DECODER: load_PE_header: ERROR = Error reading PE header. ", 0);
        return -1;
    }
    PDEBUG("DECODER:        %c%c%c%c\n", buffer[0], buffer[1], buffer[2], buffer[3]);
    if (buffer[0] != 'P' || buffer[1] != 'E' || buffer[2] != '\0' || buffer[3] != '\0') {
        PDEBUG("DECODER: ERROR= The binary hasn't the PE header standard\n");
        return -1;
    }

    /* Read the PE file header	*/
    if (il_read(buffer, 20, &(binary_info->binary)) != 0) {
        print_err("ECMASOFT DECODER: load_PE_header: ERROR = Error reading PE header. ", 0);
        return -1;
    }
    read_from_buffer(buffer, 2, 2, &(binary_info->PE_info.section_cardinality));
    read_from_buffer(buffer, 4, 4, &(binary_info->PE_info.time_stamp));
    read_from_buffer(buffer, 16, 2, &(binary_info->PE_info.optional_header_size));
    read_from_buffer(buffer, 18, 2, &(binary_info->PE_info.characteristic));
    if (((binary_info->PE_info.characteristic) & 0x2000) != 0) {
        binary_info->PE_info.isDLL = 1;
    }
    if (((binary_info->PE_info.characteristic) & 0x100) != 0) {
        binary_info->PE_info.onlyFor32BitsMachines = 1;
    }
    PDEBUG("DECODER:		Section cardinality		=	0x%X		%d\n", binary_info->PE_info.section_cardinality, binary_info->PE_info.section_cardinality);
    PDEBUG("DECODER:		Time stamp			=	0x%X	%d\n", binary_info->PE_info.time_stamp, binary_info->PE_info.time_stamp);
    PDEBUG("DECODER:		Optional header size		=	0x%X		%d\n", binary_info->PE_info.optional_header_size, binary_info->PE_info.optional_header_size);
    PDEBUG("DECODER:		DLL flags			=	0x%X		%d\n", (JITUINT32) binary_info->PE_info.isDLL, binary_info->PE_info.isDLL);
    PDEBUG("DECODER:        Check header value\n");
    if ((binary_info->PE_info.section_cardinality == 0) || (binary_info->PE_info.optional_header_size != 0 && (binary_info->PE_info.optional_header_size < 216 || binary_info->PE_info.optional_header_size > 1024))) {
        PDEBUG("DECODER: ERROR= The binary is not the CIL image\n");
        return -1;
    }
    PDEBUG("DECODER:                Optional header size OK\n");

    /* Return			*/
    return 0;
}

static inline JITINT32 load_PEOPTIONAL_header (t_binary_information *binary_info) {
    JITINT8 buffer[DIM_BUF];
    JITUINT32 offset;
    JITUINT32 field_size;

    /* Assertions			*/
    assert(binary_info != NULL);

    PDEBUG("DECODER: PE Optional header\n");
    ////////////////////////////////	Standard fields
    offset = 0;
    PDEBUG("DECODER:        Standard fields\n");
    if (il_read(buffer, binary_info->PE_info.optional_header_size, &(binary_info->binary)) != 0) {
        print_err("ECMASOFT DECODER: load_PEOPTIONAL_header: ERROR = Error reading PEOPTIONAL header. ", 0);
        return -1;
    }
    read_from_buffer(buffer, 4, 4, &(binary_info->PE_optional_header.standard_fields.code_size));
    read_from_buffer(buffer, 8, 4, &(binary_info->PE_optional_header.standard_fields.size_initialized_data));
    read_from_buffer(buffer, 12, 4, &(binary_info->PE_optional_header.standard_fields.size_unitialized_data));
    read_from_buffer(buffer, 16, 4, &(binary_info->PE_optional_header.standard_fields.entry_point_address));
    read_from_buffer(buffer, 20, 4, &(binary_info->PE_optional_header.standard_fields.base_of_code));
    read_from_buffer(buffer, 24, 4, &(binary_info->PE_optional_header.standard_fields.base_of_data));
    PDEBUG("DECODER:			Code size			=	0x%X		%ld\n", binary_info->PE_optional_header.standard_fields.code_size, (unsigned long int) binary_info->PE_optional_header.standard_fields.code_size);
    PDEBUG("DECODER:			Size initialized data		=	0x%X		%ld\n", binary_info->PE_optional_header.standard_fields.size_initialized_data, (long int) binary_info->PE_optional_header.standard_fields.size_initialized_data);
    PDEBUG("DECODER:			Size unitialized data		=	0x%X		%ld\n",       binary_info->PE_optional_header.standard_fields.size_unitialized_data, (long int) binary_info->PE_optional_header.standard_fields.size_unitialized_data);
    PDEBUG("DECODER:			Entry point RVA			=	0x%X		%ld\n",    binary_info->PE_optional_header.standard_fields.entry_point_address, (long int) binary_info->PE_optional_header.standard_fields.entry_point_address);
    PDEBUG("DECODER:			Code base RVA			=	0x%X		%ld\n",      binary_info->PE_optional_header.standard_fields.base_of_code, (long int) binary_info->PE_optional_header.standard_fields.base_of_code);
    PDEBUG("DECODER:			Data base RVA			=	0x%X		%ld\n",      binary_info->PE_optional_header.standard_fields.base_of_data, (long int) binary_info->PE_optional_header.standard_fields.base_of_data);

    ////////////////////////////////	Windows NT specific
    PDEBUG("DECODER:        Windows NT specific fields\n");
    offset = 28;
    read_from_buffer(buffer, 36, 4, &(binary_info->PE_optional_header.NT_info.file_alignment));
    read_from_buffer(buffer, 56, 4, &(binary_info->PE_optional_header.NT_info.image_size));
    read_from_buffer(buffer, 60, 4, &(binary_info->PE_optional_header.NT_info.header_size));
    read_from_buffer(buffer, 68, 2, &(binary_info->PE_optional_header.NT_info.subsystem));
    PDEBUG("DECODER:			File alignment			=	0x%X		%ld\n",     binary_info->PE_optional_header.NT_info.file_alignment, (long int) binary_info->PE_optional_header.NT_info.file_alignment);
    PDEBUG("DECODER:			Image size			=	0x%X		%ld\n", binary_info->PE_optional_header.NT_info.image_size, (long int) binary_info->PE_optional_header.NT_info.image_size);
    PDEBUG("DECODER:			Header size			=	0x%X		%ld\n",        binary_info->PE_optional_header.NT_info.header_size, (long int) binary_info->PE_optional_header.NT_info.header_size);
    PDEBUG("DECODER:			SubSystem			=	0x%X		%ld\n",  binary_info->PE_optional_header.NT_info.subsystem, (long int) binary_info->PE_optional_header.NT_info.subsystem);
    PDEBUG("DECODER:			Check values\n");
    if (binary_info->PE_optional_header.NT_info.file_alignment != 0x200 && binary_info->PE_optional_header.NT_info.file_alignment != 0x1000) {
        print_err("DECODER:             ERROR= The file alignment is not 0x200 or 0x1000 as described in the ECMA 335 spectification. ", 0);
        return -1;
    }
    PDEBUG("DECODER:                        File alignment OK\n");
    PDEBUG("DECODER:                        Directories cardinality OK\n");

    /* Data directories		*/
    PDEBUG("DECODER:        Data directories\n");
    field_size = 8;
    offset = 96;

    if (!(binary_info->PE_info.onlyFor32BitsMachines)) {
        offset += 16;
    }

    offset += field_size;

    read_from_buffer(buffer, offset, 4, &(binary_info->PE_optional_header.data_directories.import_table_RVA));
    read_from_buffer(buffer, offset+4, 4, &(binary_info->PE_optional_header.data_directories.import_table_size));
    offset += field_size;

    offset += field_size;
    offset += field_size;
    offset += field_size;

    read_from_buffer(buffer, offset, 4, &(binary_info->PE_optional_header.data_directories.relocation_table_RVA));
    read_from_buffer(buffer, offset+4, 4, &(binary_info->PE_optional_header.data_directories.relocation_table_size));
    offset += field_size;

    offset += field_size;
    offset += field_size;
    offset += field_size;
    offset += field_size;
    offset += field_size;
    offset += field_size;

    read_from_buffer(buffer, offset, 4, &(binary_info->PE_optional_header.data_directories.import_address_table_RVA));
    read_from_buffer(buffer, offset+4, 4, &(binary_info->PE_optional_header.data_directories.import_address_table_size));
    offset += field_size;

    offset += field_size;

    read_from_buffer(buffer, offset, 4, &(binary_info->PE_optional_header.data_directories.cli_header_RVA));
    read_from_buffer(buffer, offset+4, 4, &(binary_info->PE_optional_header.data_directories.cli_header_size));
    offset += field_size;

    /* Print the values		*/
    PDEBUG("DECODER:			Import table RVA	=	0x%X		%d\n",      binary_info->PE_optional_header.data_directories.import_table_RVA, binary_info->PE_optional_header.data_directories.import_table_RVA);
    PDEBUG("DECODER:			Import table size	=	0x%X		%d\n",     binary_info->PE_optional_header.data_directories.import_table_size, binary_info->PE_optional_header.data_directories.import_table_size);
    PDEBUG("DECODER:			Relocation table RVA	=	0x%X		%d\n",  binary_info->PE_optional_header.data_directories.relocation_table_RVA, binary_info->PE_optional_header.data_directories.relocation_table_RVA);
    PDEBUG("DECODER:			Relocation table size	=	0x%X		%d\n", binary_info->PE_optional_header.data_directories.relocation_table_size, binary_info->PE_optional_header.data_directories.relocation_table_size);
//    PDEBUG("DECODER:			Import address table	=	0x%X		%lld\n", binary_info->PE_optional_header.data_directories.import_address_table, binary_info->PE_optional_header.data_directories.import_address_table);
    PDEBUG("DECODER:			CLI header RVA		=	0x%X		%d\n",       binary_info->PE_optional_header.data_directories.cli_header_RVA, binary_info->PE_optional_header.data_directories.cli_header_RVA);
    PDEBUG("DECODER:			CLI header size		=	0x%X		%d\n",      binary_info->PE_optional_header.data_directories.cli_header_size, binary_info->PE_optional_header.data_directories.cli_header_size);

    /* Return			*/
    return 0;
}

static inline JITINT32 load_IAT (t_binary_information *binary_info) {
    JITINT8 buffer[40];

    if (binary_info->PE_optional_header.data_directories.import_address_table_size > 0) {
        if (il_read(buffer, 40, &(binary_info->binary))!=0) {
            print_err("ECMASOFT DECODER: load_IAT: ERROR = Error reading IAT. ", 0);
            return -1;
        }
    }

    /* Return			*/
    return 0;
}

static inline JITINT32 load_Relocation_table (t_binary_information *binary_info) {
    JITINT8 *buffer;

    if (binary_info->PE_optional_header.data_directories.relocation_table_size > 0) {
        buffer = allocFunction(binary_info->PE_optional_header.data_directories.relocation_table_size);
        if (il_read(buffer,binary_info->PE_optional_header.data_directories.relocation_table_size, &(binary_info->binary))!=0) {
            print_err("ECMASOFT DECODER: load_Relocation_table: ERROR = Error reading Relocation Table. ", 0);
            freeFunction(buffer);
            return -1;
        }
        freeFunction(buffer);
    }

    /* Return			*/
    return 0;

}

static inline JITINT32 load_CLI_header (t_binary_information *binary_info, JITUINT32 min_address) {
    JITINT8 buffer[DIM_BUF];
    JITUINT32 base;

    /*	Assertion	*/
    assert(binary_info != NULL);
    PDEBUG("DECODER:	Seeking to the CLI Header\n");

    // Seek to the beginning of the first section
    PDEBUG("DECODER:		Seek to the begin of the first section\n");
    if (seek_within_file(&(binary_info->binary), buffer, (*(binary_info->binary.reader))->offset, min_address) != 0) {
        print_err("DECODER:	ERROR= Cannot seek to the minimum address computed. ", 0);
        return -1;
    }
    assert(binary_info != NULL);
    assert((*(binary_info->binary.reader))->offset == min_address);
#ifdef PRINTDEBUG
    print_binary_offset(&(binary_info->binary));
#endif

    // Compute the address of the file where is the CLI header from the first section
    PDEBUG("DECODER:		Compute the CLI header address whitin the file from the first section\n");
    base = convert_virtualaddress_to_realaddress(binary_info->sections, binary_info->PE_optional_header.data_directories.cli_header_RVA);
    PDEBUG("DECODER:			CLI header address from the begin of the file		= 0x%X	%d\n", base, base);
    PDEBUG("DECODER:			CLI header address from the begin of the first section	= 0x%X	%d\n", base, base);
#ifdef PRINTDEBUG
    print_binary_offset(&(binary_info->binary));
#endif

    // Seek to the CLI header
    PDEBUG("DECODER:		Seek to the CLI header address\n");
    if (seek_within_file(&(binary_info->binary), buffer, (*((binary_info->binary).reader))->offset, base) != 0) {
        print_err("DECODER:	ERROR= Cannot seek to the cli header. ", 0);
        return -1;
    }
#ifdef PRINTDEBUG
    print_binary_offset(&(binary_info->binary));
#endif

    if (il_read(buffer, 72, &(binary_info->binary)) != 0) {
        print_err("ECMASOFT DECODER: load_CLI_header: ERROR = Error reading CLI header. ", 0);
        return -1;
    }
    read_from_buffer(buffer, 0, 4, &(binary_info->cli_information.cb));
    read_from_buffer(buffer, 4, 2, &(binary_info->cli_information.mayor_runtime_version));
    read_from_buffer(buffer, 6, 2, &(binary_info->cli_information.minor_runtime_version));
    read_from_buffer(buffer, 8, 4, &(binary_info->cli_information.metadata_RVA));
    read_from_buffer(buffer, 12, 4, &(binary_info->cli_information.metadata_size));
    read_from_buffer(buffer, 16, 4, &(binary_info->cli_information.flags));
    read_from_buffer(buffer, 20, 4, &(binary_info->cli_information.entry_point_token));
    read_from_buffer(buffer, 24, 4, &(binary_info->cli_information.resources_RVA));
    read_from_buffer(buffer, 28, 4, &(binary_info->cli_information.resources_size));
    read_from_buffer(buffer, 32, 4, &(binary_info->cli_information.strong_name_signature));
    read_from_buffer(buffer, 48, 4, &(binary_info->cli_information.vtable_fixups));
#ifdef PRINTDEBUG
    PDEBUG("DECODER:	CLI Header\n");
    PDEBUG("DECODER:		CB				=	0x%X		%d\n",  binary_info->cli_information.cb, binary_info->cli_information.cb);
    PDEBUG("DECODER:		Mayor runtime version		=	0x%X		%d\n", (JITUINT32) binary_info->cli_information.mayor_runtime_version, binary_info->cli_information.mayor_runtime_version);
    PDEBUG("DECODER:		Minor runtime version		=	0x%X		%d\n", (JITUINT32) binary_info->cli_information.minor_runtime_version, binary_info->cli_information.minor_runtime_version);
    PDEBUG("DECODER:		Metadata RVA			=	0x%X		%d\n", binary_info->cli_information.metadata_RVA, binary_info->cli_information.metadata_RVA);
    PDEBUG("DECODER:		Metadata size			=	0x%X		%d\n",        binary_info->cli_information.metadata_size, binary_info->cli_information.metadata_size);
    {
        char *temp_buffer = cli_flags_to_string(binary_info->cli_information.flags);
        PDEBUG("DECODER:		Flags				=	0x%X		%d	%s\n",    binary_info->cli_information.flags, binary_info->cli_information.flags, temp_buffer);
        free(temp_buffer);
    }
    PDEBUG("DECODER:		Entry point token		=	0x%X	%d\n",      binary_info->cli_information.entry_point_token, binary_info->cli_information.entry_point_token);
    PDEBUG("DECODER:		Resources RVA			=	0x%X		%d\n",        binary_info->cli_information.resources_RVA, binary_info->cli_information.resources_RVA);
    PDEBUG("DECODER:		Resources size			=	0x%X		%d\n",       binary_info->cli_information.resources_size, binary_info->cli_information.resources_size);
    PDEBUG("DECODER:		Strong name signature		=	0x%X		%lld\n", (JITUINT32) binary_info->cli_information.strong_name_signature, binary_info->cli_information.strong_name_signature);
    PDEBUG("DECODER:		VTable fixups			=	0x%X		%lld\n", (JITUINT32) binary_info->cli_information.vtable_fixups, binary_info->cli_information.vtable_fixups);
    print_binary_offset(&(binary_info->binary));
#endif

    return 0;
}

static inline JITINT32 load_SECTIONS_header_table (t_binary_information *binary_info, JITUINT32 *min_address, JITUINT32 base) {
    JITINT32 num_sections;
    JITUINT32 max_address;
    JITINT8 buffer[DIM_BUF];

    /* Assertions		*/
    assert(binary_info != NULL);

    PDEBUG("DECODER: Section headers table\n");
    num_sections = binary_info->PE_info.section_cardinality;
    max_address     = MIN_UINT;
    (*min_address)  = (JITUINT32) MAX_UINT;
    while (num_sections > 0) {
        PDEBUG("DECODER:		Section %d\n", num_sections);
        t_section *current_section;

        if (il_read(buffer, 40, &(binary_info->binary)) != 0) {
            print_err("DECODER: ERROR during reading from file. ", 0);
            return -1;
        }
        current_section = add_new_section(binary_info->sections);
        if (current_section == NULL) {
            print_err("DECODER: ERROR= current_section is NULL. ", 0);
            return -1;
        }

        // Read the section values
        strncpy(current_section->name, (char *) buffer, sizeof(char) * 8);
        read_from_buffer(buffer, 8, 4, &(current_section->virtual_size));
        read_from_buffer(buffer, 12, 4, &(current_section->virtual_address));
        read_from_buffer(buffer, 16, 4, &(current_section->real_size));
        read_from_buffer(buffer, 20, 4, &(current_section->real_address));
        read_from_buffer(buffer, 24, 4, &(current_section->rva_relocation_pointer));
        read_from_buffer(buffer, 32, 2, &(current_section->rva_relocation_number));
        read_from_buffer(buffer, 36, 4, &(current_section->characteristic));

        // Print information read
        PDEBUG("DECODER:			Name			=       %s\n", current_section->name);
        PDEBUG("DECODER:			Virtual size		=       0x%X	%d\n", current_section->virtual_size, current_section->virtual_size);
        PDEBUG("DECODER:			Virtual address		=       0x%X	%d\n", current_section->virtual_address, current_section->virtual_address);
        PDEBUG("DECODER:			Real size		=       0x%X	%d\n", current_section->real_size, current_section->real_size);
        PDEBUG("DECODER:			Real address		=       0x%X	%d\n", current_section->real_address, current_section->real_address);
        PDEBUG("DECODER:			RVA relocation pointer	=       0x%X	%d\n", current_section->rva_relocation_pointer, current_section->rva_relocation_pointer);
        PDEBUG("DECODER:			RVA relocation number	=       0x%X	%d\n", current_section->rva_relocation_number, current_section->rva_relocation_number);
        {
            char *temp = section_characteristic_to_string(current_section->characteristic);
            PDEBUG("DECODER:			Characteristic		=       0x%X	%s\n", current_section->characteristic, temp);
            free(temp);
        }
        PDEBUG("DECODER:			Max address		=       0x%X	%d\n", max_address, max_address);
        PDEBUG("DECODER:			Min address		=       0x%X	%d\n", *min_address, *min_address);
#ifdef PRINTDEBUG
        print_binary_offset(&(binary_info->binary));
#endif

        // Check some condition
        if (current_section->virtual_size > current_section->real_size) {
            PDEBUG("DECODER:			Virtual size grater than the real one\n");
            current_section->virtual_size = current_section->real_size;
        } else {
            if (!current_section->virtual_size) {
                PDEBUG("DECODER:			Virtual size is zero\n");
                current_section->virtual_size = current_section->real_size;
                if (!current_section->virtual_address) {
                    PDEBUG("DECODER:			Virtual address is zero\n");
                    current_section->virtual_address = current_section->real_address;
                }
            }
        }

        // Check values
        PDEBUG("DECODER:			Check values\n");
        if (((current_section->virtual_address + current_section->virtual_size) & (JITUINT32) MAX_UINT) < current_section->virtual_address) {
            print_err("DECODER:             Virtual size is too big. ", 0);
            return -1;
        }
        PDEBUG("DECODER:				Virtual size OK\n");
        if (((current_section->real_address + current_section->real_size) & (JITUINT32) MAX_UINT) < current_section->real_address) {
            print_err("DECODER:             Real size is too big. ", 0);
            return -1;
        }
        PDEBUG("DECODER:				Real size OK\n");

        // Classify the current section
        if (!memcmp(buffer, DOTTEXT_SECTION, 8)) {
            PDEBUG("DECODER:			Text section\n");
            base = current_section->virtual_address;
        } else if (!memcmp(buffer, CORMETA_SECTION, 8)) {
            PDEBUG("DECODER:			Cormeta section\n");
            base = current_section->virtual_address;
        } else if (!memcmp(buffer, DEBUG_SECTION, 8)) {
            PDEBUG("DECODER:			IL debug section\n");
        } else if (!memcmp(buffer, SDATA_SECTION, 8)) {
            PDEBUG("DECODER:			SData section\n");
        } else if (!memcmp(buffer, DATA_SECTION, 8)) {
            PDEBUG("DECODER:			Data section\n");
        } else if (!memcmp(buffer, TLS_SECTION, 8)) {
            PDEBUG("DECODER:			TLS section\n");
        } else if (!memcmp(buffer, RSRC_SECTION, 8)) {
            PDEBUG("DECODER:			RSRC section\n");
        } else if (!memcmp(buffer, BSS_SECTION, 8)) {
            PDEBUG("DECODER:			BSS section\n");
            if (!(current_section->real_address)) {
                // The section <.cormeta> can have an empty <.bss> section, if it's happens, i ignore it
                num_sections--;
                continue;
            }
        }

        // Check some condition
        if (current_section->real_size > 0) {
            if (current_section->real_address < (*min_address)) {
                (*min_address) = current_section->real_address;
            }
            if ((current_section->real_address + current_section->real_size) > max_address) {
                max_address = (current_section->real_address + current_section->real_size);
            }
        }
        num_sections--;
    }
    PDEBUG("DECODER:		End sections\n");
    PDEBUG("DECODER:		Max address		=       0x%X	%d\n", max_address, max_address);
    PDEBUG("DECODER:		Min address		=       0x%X	%d\n", *min_address, *min_address);
    PDEBUG("DECODER:		Checking\n");

    /* If the maximum address is less than the minimum, then there
       are no sections in the file, and it cannot possibly be IL */
    if (max_address <= (*min_address)) {
        print_err("DECODER:		ERROR= This binary is not IL because the maximum address is less than the minimum. ", 0);
        return -1;
    }
    PDEBUG("DECODER:			Maximum and minimum address are OK\n");

    /* If we don't have a runtime header yet, then bail out.
       This can happen if we are processing an object file that
       does not have a ".text$il" section within it */
    if (!base) {
        print_err("DECODER:		ERROR= This IL bitecode hasn't the <.text$il> section. ", 0);
        return -1;
    }
    PDEBUG("DECODER:			<.text$il> section OK\n");
    assert(binary_info->sections != NULL);

    return 0;
}

static inline JITINT32 load_metadata (t_binary_information *binary_info) {
    JITUINT32 metadata_real_address;
    JITINT8 buffer[DIM_BUF];

    /* Assertions		*/
    assert(binary_info != NULL);

    PDEBUG("DECODER:	Read metadata\n");
    metadata_real_address = convert_virtualaddress_to_realaddress(binary_info->sections, binary_info->cli_information.metadata_RVA);
    PDEBUG("DECODER:        Computed the metadata address = 0x%X	%d\n", metadata_real_address, metadata_real_address);
    PDEBUG("DECODER:        Seek to the %d offset\n", metadata_real_address);
    if (seek_within_file(&(binary_info->binary), buffer, (*(binary_info->binary.reader))->offset, metadata_real_address) != 0) {
        print_err("DECODER: ERROR= During seeking the file. ", 0);
        return -1;
    }
    PDEBUG("DECODER:		Call the Metadata manager\n");
    metadata_init(&(binary_info->metadata));
    if (read_metadata(&(binary_info->metadata), binary_info) != 0) {
        print_err("ECMASOFT DECODER: load_metadata: ERROR= Error reading metadata. ", 0);
        abort();
    }
    assert(binary_info->sections != NULL);

    /* Return		*/
    return 0;
}

static inline JITINT32 load_assembly_references (t_binary_information *binary_info) {
    //TODO da implementare
    return 0;
}

char * ecmasoft_getVersion (void) {
    return VERSION;
}

char * ecmasoft_getAuthor (void) {
    return "Simone Campanoni";
}

char * ecmasoft_getName (void) {
    return "EcmaSoft";
}

static inline void ecmasoft_compilationTime (char *buffer, JITINT32 bufferLength) {

    /* Assertions				*/
    assert(buffer != NULL);

    snprintf(buffer, sizeof(char) * bufferLength, "%s %s", __DATE__, __TIME__);
}

static inline void ecmasoft_compilationFlags (char *buffer, JITINT32 bufferLength) {

    /* Assertions				*/
    assert(buffer != NULL);

    snprintf(buffer, sizeof(char) * bufferLength, " ");
#ifdef DEBUG
    strncat(buffer, "DEBUG ", sizeof(char) * bufferLength);
#endif
#ifdef PRINTDEBUG
    strncat(buffer, "PRINTDEBUG ", sizeof(char) * bufferLength);
#endif
#ifdef PROFILE
    strncat(buffer, "PROFILE ", sizeof(char) * bufferLength);
#endif
}
