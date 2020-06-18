/*
 * Copyright (C) 2009 - 2012  Campanoni Simone
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
#include <framework_garbage_collector.h>

// My headers
#include <internal_calls_object.h>
#include <internal_calls_utilities.h>
// End

extern t_system *ildjitSystem;

JITBOOLEAN System_Object_Equals (void *obj1, void *obj2) {
    JITBOOLEAN result;

    /* Assertions			*/
    assert(obj1 != NULL);
    assert(obj2 != NULL);

    METHOD_BEGIN(ildjitSystem, "System.Object.Equals");

    if (obj1 != obj2) {
        result = JITFALSE;
    } else {
        result = JITTRUE;
    }

    METHOD_END(ildjitSystem, "System.Object.Equals");
    return result;
}

JITINT32 System_Object_GetHashCode (void *obj) {

    /* Assertions		*/
    METHOD_BEGIN(ildjitSystem, "System.Object.GetHashCode");

    /* Return the Hash code	*/
    METHOD_END(ildjitSystem, "System.Object.GetHashCode");
    return (JITINT32) (JITNUINT) obj;
}

void * System_Object_MemberwiseClone (void *obj) {
    JITNUINT size;
    void                    *newObj;

    /* Assertions			*/
    assert(obj != NULL);
    METHOD_BEGIN(ildjitSystem, "System.Object.MemberwiseClone");

    /* Fetch the object information	*/
    size = (ildjitSystem->garbage_collectors).gc.getSize(obj);
    assert(size > 0);

    /* Make the new object		*/
    newObj = ((ildjitSystem->garbage_collectors).gc.gc_plugin)->allocObject(size);
    assert(newObj != NULL);

    /* Clone the object		*/
    memcpy(newObj, obj - HEADER_FIXED_SIZE, size);
    newObj += HEADER_FIXED_SIZE;

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Object.MemberwiseClone");
    return newObj;
}

void * System_Object_GetType (void *object) {
    TypeDescriptor          *ilType;
    void                    *objectClass;

    /* Assertions				*/
    assert(object != NULL);
    assert((ildjitSystem->garbage_collectors).gc.getType != NULL);
    METHOD_BEGIN(ildjitSystem, "System.Object.GetType");

    /* Initialize the variables		*/
    objectClass = NULL;

    /* Fetch the type of the object given	*
     * as incoming argument			*/
    ilType = GC_getObjectType(object);
    assert(ilType != NULL);

    /* Fetch the instance of System.Type    *
     * of the class of the object given as  *
     * incoming argument			*/
    objectClass = ((ildjitSystem->cliManager).CLR.reflectManager).getType(ilType);
    assert(objectClass != NULL);

    /* Return the System.Type of the object	*/
    METHOD_END(ildjitSystem, "System.Object.GetType");
    return objectClass;
}

/* Get the hash code of object */
JITINT32 System_Object_InternalGetHashCode (void* object) {
    JITINT32 hashCode;

    METHOD_BEGIN(ildjitSystem, "System.Object.InternalGetHashCode");

    hashCode = System_Object_GetHashCode(object);

    METHOD_END(ildjitSystem, "System.Object.InternalGetHashCode");

    return hashCode;
}
