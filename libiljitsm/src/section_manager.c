/*
 * Copyright (C) 2006 - 2010  Campanoni Simone
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
   *@file section_manager.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <xanlib.h>
#include <jitsystem.h>
#include <compiler_memory_manager.h>

// My header
#include <section_manager.h>
#include <iljitsm-system.h>
#include <config.h>
// End

#define DIM_BUF 2056

JITINT32 sections_init (t_section **sections) {
    (*sections) = (t_section *) sharedAllocFunction(sizeof(t_section));
    if ((*sections) == NULL) {
        print_err("SECTION MANAGER: ERROR= Cannot initializer the sections. ", errno);
        return -1;
    }
    (*sections)->next = NULL;
    return 0;
}

void sections_shutdown (t_section **sections) {
    t_section *temp, *temp_old;

    PDEBUG("SECTION MANAGER: Shutting down\n");
    if ((*sections) == NULL) {
        PDEBUG("SECTION MANAGER: No sections\n");
        return;
    }
    temp = (*sections);
    while (temp != NULL) {
        temp_old = temp;
        temp = get_next_section(temp);
        assert(temp_old != NULL);
        freeFunction(temp_old);
    }
    PDEBUG("SECTION MANAGER: Done\n");
}

t_section * add_new_section (t_section *sections) {
    t_section *temp;

    /* Assertions           */
    assert(sections != NULL);

    temp = sections;
    while (temp->next != NULL) {
        temp = temp->next;
    }
    assert(temp->next == NULL);
    temp->next = (t_section *) sharedAllocFunction(sizeof(t_section));
    if (temp->next == NULL) {
        print_err("SECTION MANAGER: ERROR= Cannot allocate memory for the new section. ", errno);
        return NULL;
    }
    assert(temp->next != NULL);
    temp->next->next = NULL;
    return temp->next;
}

t_section * get_first_section (t_section *sections) {
    /* Assertions           */
    assert(sections != NULL);

    return sections->next;
}

t_section * get_next_section (t_section *current_section) {
    /* Assertions           */
    assert(current_section != NULL);

    return current_section->next;
}

JITINT32 add_offset_to_real_address_for_sections (t_section *sections, JITUINT32 offset) {
    t_section *current;

    if (sections == NULL) {
        return -1;
    }

    /* Assertions           */
    assert(sections != NULL);

    current = get_first_section(sections);
    while (current != NULL) {
        current->real_address += offset;
        current = get_next_section(current);
    }
    return 0;
}

JITINT32 convert_virtualaddress_to_realaddress (t_section *sections, JITUINT32 address) {
    t_section *current;

    /* Assertions           */
    assert(sections != NULL);

    current = get_first_section(sections);
    while (current != NULL) {
        assert(current != NULL);
        if (    (address >= current->virtual_address)                                   &&
                (address < ( (current->virtual_address) + (current->virtual_size)))     ) {
            JITUINT32 offset;
            offset = address - (current->virtual_address);
            return current->real_address + offset;
        }
        current = get_next_section(current);
    }
    return -1;
}

void libiljitsmCompilationFlags (char *buffer, JITINT32 bufferLength) {

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

void libiljitsmCompilationTime (char *buffer, JITINT32 bufferLength) {

    /* Assertions				*/
    assert(buffer != NULL);

    snprintf(buffer, sizeof(char) * bufferLength, "%s %s", __DATE__, __TIME__);
}

char * libiljitsmVersion () {
    return VERSION;
}
