/*
 * Copyright (C) 2006 - 2009 Simone Campanoni  Speziale Ettore
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
#ifndef LIB_DELEGATES
#define LIB_DELEGATES

#include <jitsystem.h>
#include <ilmethod.h>

/**
 * The delegates manager
 *
 * This object is used to support delegates management. It exposes methods to
 * build delegates methods at runtime and other utility methods used by the
 * internal calls of the delegates framework.
 *
 * @author Speziale Ettore
 */
typedef struct _DelegatesManager {
    IR_ITEM_VALUE buildDelegateFunctionPointer;

    /**
     * The System.Delegate.MulticastDelegate type
     */
    TypeDescriptor* mdType;

    /**
     * The System.Runtime.Remoting.Messaging.AsyncResult type
     */
    TypeDescriptor* arType;

    /**
     * The System.MulticastDelegate..ctor method
     *
     * This method is used to build a delegate that points to a static
     * method.
     */
    Method mdSCtor;

    /**
     * The System.MulticastDelegate..ctor method
     *
     * This method is used to build a delegate that points to a instance
     * method.
     */
    Method mdCtor;

    /**
     * The System.Runtime.Remoting.Messaging.AsyncResult constructor.
     */
    Method arCtor;

    /**
     * The System.MulticastDelegate.DynamicInvoke method
     *
     * This method is used to perform synchronous calls with a delegate.
     */
    Method dynamicInvoke;

    /**
     * The System.Runtime.Remoting.Messaging.AsyncResult.EndInvoke emthod
     */
    Method endInvoke;

    /**
     * Offset of the method field in a System.Delegate
     */
    JITINT32 methodFieldOffset;

    /**
     * Build given method body
     *
     * Fill given method body with custom IR instruction in order to exec a
     * specific delegates operation. Valid methods are ctor, Invoke,
     * BeginInvoke and EndInvoke. This method doesn't make any signature
     * check on given method. These kind of checks are done at runtime.
     * If a method with an invalid name is given abort execution.
     *
     * Remember that this method isn't thread safe.
     *
     * @param self this object
     * @param method the method to build
     */
    void (*buildMethod)(struct _DelegatesManager* self, Method method);

    /**
     * Get the System.MulticastDelegate type
     *
     * Get the System.MulticastDelegate. This method is thread safe.
     *
     * @param self this object
     *
     * @return the System.MulticastDelegate type
     */
    TypeDescriptor* (*getMulticastDelegate)(struct _DelegatesManager* self);

    /**
     * Get the System.Runtime.Remoting.Messaging.AsyncResult type
     *
     * Get the System.Runtime.Remoting.Messaging.AsyncResult type. This
     * method is thread safe.
     *
     * @param self this object
     *
     * @return the System.Runtime.Remoting.Messaging.AsyncResult type
     */
    TypeDescriptor* (*getAsyncResult)(struct _DelegatesManager* self);

    /**
     * Get System.MulticastDelegate..ctor method
     *
     * This function returns the System.MulticastDelegate constructor used
     * for delegates that points to static methods. This method is thread
     * safe.
     *
     * @param self this object
     *
     * @return the constructor for static delegates
     */
    Method (*getMDSCtor)(struct _DelegatesManager* self);

    /**
     * Get System.MulticastDelegate..ctor method
     *
     * This function returns the System.MulticastDelegate constructor used
     * for delegates that points to an instance method. This method is
     * thread safe.
     *
     * @param self this object
     *
     * @return the constructor for instance delegates
     */
    Method (*getMDCtor)(struct _DelegatesManager* self);

    /**
     * Get System.Runtime.Remoting.Messaging.AsyncResult..ctor method
     *
     * @param self this object
     *
     * @return the constructor of
     *         System.Runtime.Remoting.Messaging.AsyncResult
     */
    Method (*getARCtor)(struct _DelegatesManager* self);

    /**
     * Get System.MulticastDelegate.DynamicInvoke method
     *
     * This function returns the System.MulticastDelegate.DynamicInvoke
     * method. This method is thread safe.
     *
     * @param self this object
     *
     * @return the method System.MulticastDelegate.DynamicInvoke
     */
    Method (*getDynamicInvoke)(struct _DelegatesManager* self);

    /**
     * Get System.Runtime.Remoting.Messaging.AsyncResult.EndInvoke method
     *
     * @param self this object
     *
     * @return the System.Runtime.Remoting.Messaging.AsyncResult.EndInvoke
     *         method
     */
    Method (*getEndInvoke)(struct _DelegatesManager* self);

    /**
     * Gets the description of the method referenced by the delegate
     *
     * @param self this object
     * @param delegate the delegate whose wrapped method has to be returned
     * @param callee filled with the method referenced by the delegate
     */
    MethodDescriptor *(*getCalleeMethod)(struct _DelegatesManager* self, void* delegate);

    void (*initialize)(void);
    /**
     * Destroy the given DelegatesManager and free any used resources
     *
     * This function behave like destructor in object oriented languages. The self
     * pointer is like the this pointer in Java or C# languages. This function also
     * release the heap memory used by the object itself. It isn't recommended to
     * use the self object after this function call.
     *
     * @param self pointer to this object
     */
    void (*destroy)(struct _DelegatesManager* self);
} t_delegatesManager;

/**
 * Build a new DelegatesManager
 *
 * This function acts as a constructor for DelegatesManager object in OO
 * languages. Note that the new object is allocated on the heap. Remeber to
 * call destroyDelegatesManager in order to avoid DelegatesManager related
 * memory leaks.
 *
 * @return a new DelegatesManager object
 */
/**
 * Build a new DelegatesManager
 *
 * This function acts as a constructor for DelegatesManager object in OO
 * languages. Note that the new object is allocated on the heap. Remeber to
 * call destroyDelegatesManager in order to avoid DelegatesManager related
 * memory leaks.
 *
 * @return a new DelegatesManager object
 */
void  init_delegatesManager (t_delegatesManager *manager, IR_ITEM_VALUE buildDelegateFunctionPointer);

#endif /* LIB_DELEGATES */
