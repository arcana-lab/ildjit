/*
 * Copyright (C) 2008  Campanoni Simone
 *
 * iljit - This is a Just-in-time for the CIL language specified with the ECMA-335
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
#ifndef INTERNAL_CALLS_NET_H
#define INTERNAL_CALLS_NET_H

#include <jitsystem.h>
#include <errno.h>

/* this macro excludes this code in Windows because it is not ported yet.
You MUST this macro when this code will be fully win-32 ported */
#ifndef WIN32
static int const errnoFromMapTable[] = {
    0,
    EPERM,
    ENOENT,
    ESRCH,
    EINTR,
    EIO,
    ENXIO,
    E2BIG,
    ENOEXEC,
    EBADF,
    ECHILD,
    EAGAIN,
    ENOMEM,
    EACCES,
    EFAULT,
    ENOTBLK,
    EBUSY,
    EEXIST,
    EXDEV,
    ENODEV,
    ENOTDIR,
    EISDIR,
    EINVAL,
    ENFILE,
    EMFILE,
    ENOTTY,
    ETXTBSY,
    EFBIG,
    ENOSPC,
    ESPIPE,
    EROFS,
    EMLINK,
    EPIPE,
    EDOM,
    ERANGE,
    EDEADLK,
    ENAMETOOLONG,
    ENOLCK,
    ENOSYS,
    ENOTEMPTY,
    ELOOP,
    -1,
    ENOMSG,
    EIDRM,
    ECHRNG,
    EL2NSYNC,
    EL3HLT,
    EL3RST,
    ELNRNG,
    EUNATCH,
    ENOCSI,
    EL2HLT,
    EBADE,
    EBADR,
    EXFULL,
    ENOANO,
    EBADRQC,
    EBADSLT,
    -1,
    EBFONT,
    ENOSTR,
    ENODATA,
    ETIME,
    ENOSR,
    ENONET,
    ENOPKG,
    EREMOTE,
    ENOLINK,
    EADV,
    ESRMNT,
    ECOMM,
    EPROTO,
    EMULTIHOP,
    EDOTDOT,
    EBADMSG,
    EOVERFLOW,
    ENOTUNIQ,
    EBADFD,
    EREMCHG,
    ELIBACC,
    ELIBBAD,
    ELIBSCN,
    ELIBMAX,
    ELIBEXEC,
    EILSEQ,
    ERESTART,
    ESTRPIPE,
    EUSERS,
    ENOTSOCK,
    EDESTADDRREQ,
    EMSGSIZE,
    EPROTOTYPE,
    ENOPROTOOPT,
    EPROTONOSUPPORT,
    ESOCKTNOSUPPORT,
    EOPNOTSUPP,
    EPFNOSUPPORT,
    EAFNOSUPPORT,
    EADDRINUSE,
    EADDRNOTAVAIL,
    ENETDOWN,
    ENETUNREACH,
    ENETRESET,
    ECONNABORTED,
    ECONNRESET,
    ENOBUFS,
    EISCONN,
    ENOTCONN,
    ESHUTDOWN,
    ETOOMANYREFS,
    ETIMEDOUT,
    ECONNREFUSED,
    EHOSTDOWN,
    EHOSTUNREACH,
    EALREADY,
    EINPROGRESS,
    ESTALE,
    EUCLEAN,
    ENOTNAM,
    ENAVAIL,
    EISNAM,
    EREMOTEIO,
    EDQUOT,
    ENOMEDIUM,
    EMEDIUMTYPE,
};
#endif /* end WIN32 exclusion. You MUST remove this line when this code will be windows-ported.*/

#define errnoFromMapSize (sizeof(errnoFromMapTable) / sizeof(int))

//HostToNetwork
JITINT64 System_Net_IPAddress_HostToNetworkOrderL(JITINT64);
JITINT32 System_Net_IPAddress_HostToNetworkOrderI(JITINT32);
JITINT16 System_Net_IPAddress_HostToNetworkOrderS(JITINT16);

//NetworkToHost
JITINT64 System_Net_IPAddress_NetworkToHostOrderL(JITINT64);
JITINT32 System_Net_IPAddress_NetworkToHostOrderI(JITINT32);
JITINT16 System_Net_IPAddress_NetworkToHostOrderS(JITINT16);

//SocketMethods
JITNINT Platform_SocketMethods_GetInvalidHandle ();
JITBOOLEAN Platform_SocketMethods_AddressFamilySupported (JITINT32 af);
JITBOOLEAN Platform_SocketMethods_Create (JITINT32 af, JITINT32 st, JITINT32 pt, JITNINT* handle);
JITBOOLEAN Platform_SocketMethods_Bind (JITNINT handle, void* addr);
JITBOOLEAN Platform_SocketMethods_Shutdown (JITNINT handle, JITINT32 how);
JITBOOLEAN Platform_SocketMethods_Listen (JITNINT handle, JITINT32 backlog);
JITBOOLEAN Platform_SocketMethods_Accept (JITNINT handle, void* addr, JITNINT* new_handle);
JITBOOLEAN Platform_SocketMethods_Connect (JITNINT handle, void* addr);
JITINT32 Platform_SocketMethods_Receive (JITNINT handle, void* buffer, JITINT32 offset, JITINT32 size, JITINT32 flags);
JITINT32 Platform_SocketMethods_ReceiveFrom (JITNINT handle, void* buffer, JITINT32 offset, JITINT32 size, JITINT32 flags, void* addr_return);
JITINT32 Platform_SocketMethods_Send (JITNINT handle, void* buffer, JITINT32 offset, JITINT32 size, JITINT32 flags);
JITINT32 Platform_SocketMethods_SendTo (JITNINT handle, void* buffer, JITINT32 offset, JITINT32 size, JITINT32 flags, void* addr_return);
JITBOOLEAN Platform_SocketMethods_Close (JITNINT handle);
JITINT32 Platform_SocketMethods_Select (void* readarray, void* writearray, void* errorarray, JITINT64 timeout);
JITBOOLEAN Platform_SocketMethods_SetBlocking (JITNINT handle, JITBOOLEAN blocking);
JITINT32 Platform_SocketMethods_GetAvailable (JITNINT handle);
JITBOOLEAN Platform_SocketMethods_GetSockName (JITNINT handle, void* addr_return);
JITBOOLEAN Platform_SocketMethods_SetSocketOption (JITNINT handle, JITINT32 level, JITINT32 name, JITINT32 value);
JITBOOLEAN Platform_SocketMethods_GetSocketOption (JITNINT handle, JITINT32 level, JITINT32 name, JITINT32* value);
JITBOOLEAN Platform_SocketMethods_SetLingerOption (JITNINT handle, JITBOOLEAN enabled, JITINT32 seconds);
JITBOOLEAN Platform_SocketMethods_GetLingerOption (JITNINT handle, JITBOOLEAN* enabled, JITINT32* seconds);
JITBOOLEAN Platform_SocketMethods_SetMulticastOption (JITNINT handle, JITINT32 af, JITINT32 name, void* group, void* mcint);
JITBOOLEAN Platform_SocketMethods_GetMulticastOption (JITNINT handle, JITINT32 af, JITINT32 name, void* group, void* mcint);
JITBOOLEAN Platform_SocketMethods_CanStartThreads ();
JITBOOLEAN Platform_SocketMethods_DiscoverIrDADevices (JITNINT handle, void* buf);
JITINT32 Platform_SocketMethods_GetErrno ();
void* Platform_SocketMethods_GetErrnoMessage (JITINT32 error);
JITBOOLEAN Platform_SocketMethods_QueueCompletionItem (void* callback, void* state);
void* Platform_SocketMethods_CreateManualResetEvent ();
void Platform_SocketMethods_WaitHandleSet (void* wait_handle);

//DnsMethods
JITBOOLEAN Platform_DnsMethods_InternalGetHostByName (void* host, void** h_name, void** h_aliases, void** h_addr_list);
JITBOOLEAN Platform_DnsMethods_InternalGetHostByAddr (JITINT64 address, void** h_name, void** h_aliases, void** h_addr_list);
void* Platform_DnsMethods_InternalGetHostName ();

// PortMethods
JITBOOLEAN Platform_PortMethods_IsValid (JITINT32 type, JITINT32 portNumber);
JITBOOLEAN Platform_PortMethods_IsAccessible (JITINT32 type, JITINT32 portNumber);
JITNINT Platform_PortMethods_Open (JITINT32 type, JITINT32 portNumber, void* parameters);
void Platform_PortMethods_Close (JITNINT handle);
void Platform_PortMethods_Modify (JITNINT handle, void *parameters);
JITINT32 Platform_PortMethods_GetBytesToRead (JITNINT handle);
JITINT32 Platform_PortMethods_GetBytesToWrite (JITNINT handle);
JITINT32 Platform_PortMethods_ReadPins (JITNINT handle);
void Platform_PortMethods_WritePins (JITNINT handle, JITINT32 mask, JITINT32 value);
void Platform_PortMethods_GetRecommendedBufferSizes (JITINT32 *read_buffer_size, JITINT32 *write_buffer_size,JITINT32 *received_bytes_threshold);
void Platform_PortMethods_DiscardInBuffer (JITNINT handle);
void Platform_PortMethods_DiscardOutBuffer (JITNINT handle);
void Platform_PortMethods_DrainOutBuffer (JITNINT handle);
JITINT32 Platform_PortMethods_Read (JITNINT handle, void* buffer, JITINT32 offset, JITINT32 count);
void Platform_PortMethods_Write (JITNINT handle, void* buffer, JITINT32 offset, JITINT32 count);
JITINT32 Platform_PortMethods_WaitForPinChange (JITNINT handle);
JITINT32 Platform_PortMethods_WaitForInput (JITNINT handle, JITINT32 timeout);
void Platform_PortMethods_Interrupt (void* thread);

#endif
