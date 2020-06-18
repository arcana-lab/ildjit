/*
 * Copyright (C) 2006  Simone Campanoni, Speziale Ettore
 *
 * iljit - This is a Just-in-time for the CIL language specified with the
 *         ECMA-335
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 */
#ifndef LIB_CULTURE_H
#define LIB_CULTURE_H

#include <ilmethod.h>

/**
 * The culture library
 */
typedef struct t_cultureManager {

    /**
     * Description of the CultureInfo class
     */
    TypeDescriptor *cultureInfoType;

    /**
     * Returns the invariant culture CultureInfo
     */
    Method invariantCultureGetter;

    /**
     * Initialize the library
     */
    void (*initialize)();

    /**
     * Gets the CultureInfo representing the invariant culture
     *
     * @return the invariant culture
     */
    void* (*getInvariantCulture)(void);
} t_cultureManager;

/**
 * Build the culture library
 */
void init_cultureManager (struct t_cultureManager* self);

#endif /* LIB_CULTURE_H */
