/*
 * Copyright (C) 2006 - 2010 Campanoni Simone
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
   *@file section_manager.h
 */
#ifndef SECTION_MANAGER_H
#define SECTION_MANAGER_H

/**
 * @mainpage ILDJIT Section Manager Library
 * \author Simone Campanoni
 * \version 0.3.0
 *
 * Section Manager library for the ILDJIT project
 *
 * Project web page: \n
 *      http://ildjit.sourceforge.net
 */

// My headers
#include <iljit-utils.h>
#include <jitsystem.h>
// End

/**
 * @brief Section
 *
 * This struct contains all inforamtion of a section
 */
typedef struct _t_section {
    char name[9];
    JITUINT16 rva_relocation_number;
    JITUINT32 virtual_address;
    JITUINT32 virtual_size;
    JITUINT32 real_address;                         /**< Offset where the section start from the begin to the file */
    JITUINT32 real_size;                            /**< Size of the section */
    JITUINT32 rva_relocation_pointer;
    JITUINT32 characteristic;
    struct _t_section *next;
} t_section;


/**
 *
 * Init the struct sections
 */
JITINT32 sections_init (t_section **sections);

/**
 *
 * Free all memory allocated in the struct pointed by the sections parameter
 */
void sections_shutdown (t_section **sections);

/**
 *
 * Add a new section to the sections list
 */
t_section * add_new_section (t_section *sections);

/**
 *
 * Return the first section of the list sections
 */
t_section * get_first_section (t_section *sections);

/**
 *
 * Return the next section from the section pointed by the current_section parameter
 */
t_section * get_next_section (t_section *current_section);

/**
 * \brief Add an offset to the real address field on each sections
 * @param sections It is the first section so each one returned from the get_first_section function
 */
JITINT32 add_offset_to_real_address_for_sections (t_section *sections, JITUINT32 offset);

/**
 * @param sections Pointer to the first sections
 */
JITINT32 convert_virtualaddress_to_realaddress (t_section *sections, JITUINT32 address);

char * libiljitsmVersion ();
void libiljitsmCompilationFlags (char *buffer, JITINT32 bufferLength);
void libiljitsmCompilationTime (char *buffer, JITINT32 bufferLength);

#endif
