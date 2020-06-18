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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

// My headers
#include <metadata/tables/metadata_table_manager.h>
#include <iljitmm-system.h>
// End

void * private_get_table (t_metadata_tables *tables, JITUINT16 table_ID) {
    static t_metadata_tables *reference = NULL;

    if (tables != NULL) {
        reference = tables;
        return NULL;

    }
    assert(tables == NULL);
    assert(reference != NULL);

    switch (table_ID) {
        case ASSEMBLY_TABLE:
            return &(reference->assembly_table);
        case ASSEMBLY_REF_TABLE:
            return &(reference->assembly_ref_table);
        case CUSTOM_ATTRIBUTE_TABLE:
            return &(reference->assembly_ref_table);
        case EVENT_TABLE:
            return &(reference->event_table);
        case EVENT_MAP_TABLE:
            return &(reference->event_map_table);
        case EXPORTED_TYPE_TABLE:
            return &(reference->exported_type_table);
        case FIELD_TABLE:
            return &(reference->field_table);
        case FILE_TABLE:
            return &(reference->file_table);
        case INTERFACE_IMPL_TABLE:
            return &(reference->interface_impl_table);
        case MANIFEST_RESOURCE_TABLE:
            return &(reference->manifest_resource_table);
        case MEMBER_REF_TABLE:
            return &(reference->member_ref_table);
        case METHOD_DEF_TABLE:
            return &(reference->method_def_table);
        case MODULE_TABLE:
            return &(reference->module_table);
        case MODULE_REF_TABLE:
            return &(reference->module_ref_table);
        case PARAM_TABLE:
            return &(reference->param_table);
        case PROPERTY_TABLE:
            return &(reference->property_table);
        case PROPERTY_MAP_TABLE:
            return &(reference->property_map_table);
        case STANDALONE_SIG_TABLE:
            return &(reference->standalone_sig_table);
        case TYPE_DEF_TABLE:
            return &(reference->type_def_table);
        case TYPE_REF_TABLE:
            return &(reference->type_ref_table);
        case TYPE_SPEC_TABLE:
            return &(reference->type_spec_table);
        case GENERIC_PARAM_TABLE:
            return &(reference->generic_param_table);
        case GENERIC_PARAM_CONSTRAINT_TABLE:
            return &(reference->generic_param_constraint_table);
        case METHOD_SPEC_TABLE:
            return &(reference->method_spec_table);
        default:
            print_err("METADATA MANAGER: MTM_UTILS: ERROR= Table not know. ", 0);
            abort();
    }
    return NULL;
}

JITBOOLEAN private_is_type_or_method_def_token_32 (void) {
    t_type_def_table *td_table;
    t_method_def_table *md_table;
    JITBOOLEAN td;
    JITBOOLEAN md;

    td_table = (t_type_def_table *) private_get_table(NULL, TYPE_DEF_TABLE);
    md_table = (t_method_def_table *) private_get_table(NULL, METHOD_DEF_TABLE);
    assert(td_table != NULL);
    assert(md_table != NULL);

    td = (td_table->cardinality <= TYPE_OR_METHOD_DEF_TOKEN_SIZE_BOUND);
    md = (md_table->cardinality <= TYPE_OR_METHOD_DEF_TOKEN_SIZE_BOUND);

    if ( td && md) {
        return 0;
    }
    return 1;
}

JITBOOLEAN private_is_type_def_or_ref_token_32 (void) {
    t_type_def_table *td_table;
    t_type_ref_table *tr_table;
    t_type_ref_table *ts_table;
    JITBOOLEAN td;
    JITBOOLEAN tr;
    JITBOOLEAN ts;

    td_table = (t_type_def_table *) private_get_table(NULL, TYPE_DEF_TABLE);
    tr_table = (t_type_ref_table *) private_get_table(NULL, TYPE_REF_TABLE);
    ts_table = (t_type_ref_table *) private_get_table(NULL, TYPE_SPEC_TABLE);
    assert(td_table != NULL);
    assert(tr_table != NULL);
    assert(ts_table != NULL);

    td = (td_table->cardinality <= TYPE_DEF_OR_REF_TOKEN_SIZE_BOUND);
    tr = (tr_table->cardinality <= TYPE_DEF_OR_REF_TOKEN_SIZE_BOUND);
    ts = (ts_table->cardinality <= TYPE_DEF_OR_REF_TOKEN_SIZE_BOUND);

    if ( td && tr && ts) {
        return 0;
    }
    return 1;
}

JITBOOLEAN private_is_method_def_or_ref_token_32 (void) {
    t_member_ref_table *mr_table;
    t_method_def_table *md_table;
    JITBOOLEAN mr;
    JITBOOLEAN md;

    mr_table = (t_member_ref_table *) private_get_table(NULL, MEMBER_REF_TABLE);
    md_table = (t_method_def_table *) private_get_table(NULL, METHOD_DEF_TABLE);
    assert(mr_table != NULL);
    assert(md_table != NULL);

    md = (md_table->cardinality <= METHOD_DEF_OR_REF_TOKEN_SIZE_BOUND);
    mr = (mr_table->cardinality <= METHOD_DEF_OR_REF_TOKEN_SIZE_BOUND);

    if ( mr && md) {
        return 0;
    }
    return 1;
}

JITUINT32 getBlobLength (JITINT8 *buf, JITUINT32 *bytesRead) {
    JITUINT32 length;
    JITUINT8 byte;
    JITUINT8 secondByte;

    /* Assertions			*/
    assert(buf != NULL);
    assert(bytesRead != NULL);

    byte = (buf[0] & 0xFF);
    PDEBUG("METADATA STREAM MANAGER: getBlobLength:         First byte	= 0x%X\n", byte);
    if ((buf[0] & 0x80) == 0x0) {
        length = (JITUINT32) byte;
        (*bytesRead) = 1;
    } else if (      ((buf[0] & 0x80) == 0x80)       &&
                     ((buf[0] & 0x40) == 0x0)        ) {
        secondByte = (buf[1] & 0xFF);
        PDEBUG("METADATA STREAM MANAGER: getBlobLength:		Second byte	= 0x%X\n", secondByte);
        length = ( ((((JITUINT16) (byte & 0x3F)) << 8) + secondByte));
        (*bytesRead) = 2;
    } else if (     ((buf[0] & 0x80) == 0x80)       &&
                    ((buf[0] & 0x40 ) == 0x40)      &&
                    ((buf[0] & 0x20 ) == 0x0)       ) {
        //length	= ((((buf[0] << 2) >> 2) << (24 + buf[1]) ) << (16 + buf[2]) ) << (8 + buf[3]);
        length = ((buf[0] & 0x1f) << 24) + (buf[1] << 16) + (buf[2] << 8) + buf[3];
        (*bytesRead) = 4;
    } else {
        print_err("METADATA STREAM MANAGER: getBlobLength: ERROR = Length of the blob corrupted. ", 0);
        abort();
    }
    return length;
}

inline JITUINT32 uncompressValue (JITINT8 *signature, JITUINT32 *uncompressed ) {
    JITUINT32 bytes_read;
    JITUINT32 value;
    JITUINT8 temp[8];

    /* assertions			*/
    assert(signature != NULL);
    assert(uncompressed != NULL);

    /* check the size of the integers inside the signature.	*/
    if ((signature[0] & 0x80) == 0x0) {
        /* signature of 1 byte, value = [6..0] bits	*/
        PDEBUG("METADATA MANAGER: uncompressValue: one byte signature\n");
        bytes_read = 1;
        (*uncompressed) = signature[0];
    } else if (     ((signature[0] & 0x80) == 0x80)         &&
                    ((signature[0] & 0x40) == 0x0)          ) {
        PDEBUG("METADATA MANAGER: uncompressValue: two byte signature\n");
        /* signature of 2 byte, value = [13..0] bits	*/
        bytes_read = 2;
        memset(temp, 0, 4);
        temp[0] = signature[1];
        temp[1] = signature[0];
        read_from_buffer(temp, 0, 2, &(value));
        (*uncompressed) = (value & 0x3FFF);
    } else if (     ((signature[0] & 0x80) == 0x80)         &&
                    ((signature[0] & 0x40) == 0x40)         &&
                    ((signature[0] & 0x20) == 0x0)          ) {
        /* signature of 4 bytes, value = [28..0] bits	*/
        bytes_read = 4;
        memset(temp, 0, 4);
        temp[0] = signature[3];
        temp[1] = signature[2];
        temp[2] = signature[1];
        temp[3] = signature[0];
        read_from_buffer(temp, 0, 4, &(value));
        (*uncompressed) = (JITUINT32)(value & 0x1FFFFFFF);
    } else {
        print_err("METADATA MANAGER: uncompressValue: uncompress_value: error decoding failure of the signature, the integer are not compressed as described in the ecma-335 standard. ", 0);
        abort();
    }

    return bytes_read;
}

