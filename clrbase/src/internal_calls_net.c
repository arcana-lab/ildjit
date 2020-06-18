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
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <platform_API.h>	/* insert here all platform-specific headers */

#ifdef HAVE_LINUX_IRDA_H
#include <linux/irda.h>
#endif

// My headers
#include <internal_calls_net.h>
#include <internal_calls_utilities.h>
// End

#define GET_TIME(curr_sec, curr_nsec)  { \
		struct timeval tv; \
		gettimeofday(&tv, 0); \
		curr_sec = ((JITINT64) (tv.tv_sec)) + EPOCH_ADJUST; \
		curr_nsec = (JITUINT32) (tv.tv_usec * 1000);  \
}

extern t_system *ildjitSystem;

JITINT16 System_Net_IPAddress_HostToNetworkOrderS (JITINT16 host) {
    JITINT16 result;

    METHOD_BEGIN(ildjitSystem, "System.Net.IPAddress.HostToNetworkOrder");

    result = htons(host);

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Net.IPAddress.HostToNetworkOrder");
    return result;
}

JITINT32 System_Net_IPAddress_HostToNetworkOrderI (JITINT32 host) {
    JITINT32 result;

    METHOD_BEGIN(ildjitSystem, "System.Net.IPAddress.HostToNetworkOrder");

    result = htonl(host);

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Net.IPAddress.HostToNetworkOrder");
    return result;
}

JITINT64 System_Net_IPAddress_HostToNetworkOrderL (JITINT64 host) {
    JITINT64 result;
    JITINT32 first_half;
    JITINT32 second_half;

    union {
        unsigned long long ull;
        char c[8];
    } x;
    x.ull = 0x0123456789abcdefULL;

    METHOD_BEGIN(ildjitSystem, "System.Net.IPAddress.HostToNetworkOrder");
    first_half = (host & 0x00000000FFFFFFFFULL);
    second_half = (host & 0xFFFFFFFF00000000ULL) >> 32;
    first_half = htonl(first_half);
    second_half = htonl(second_half);

    if (x.c[0] == (char) 0xef) {
        result = (((JITINT64) first_half) << 32) | (((JITINT64) second_half) & 0x00000000FFFFFFFF);
    } else {
        result = (((JITINT64) second_half) << 32) | (((JITINT64) first_half) & 0x00000000FFFFFFFF);
    }

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Net.IPAddress.HostToNetworkOrder");
    return result;
}

JITINT16 System_Net_IPAddress_NetworkToHostOrderS (JITINT16 network) {
    JITINT16 result;

    METHOD_BEGIN(ildjitSystem, "System.Net.IPAddress.NetworkToHostOrder");

    result = ntohs(network);

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Net.IPAddress.NetworkToHostOrder");
    return result;
}
JITINT32 System_Net_IPAddress_NetworkToHostOrderI (JITINT32 network) {
    JITINT32 result;

    METHOD_BEGIN(ildjitSystem, "System.Net.IPAddress.NetworkToHostOrder");

    result = ntohl(network);

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Net.IPAddress.NetworkToHostOrder");
    return result;
}
JITINT64 System_Net_IPAddress_NetworkToHostOrderL (JITINT64 network) {

    JITINT64 result;
    JITINT32 first_half;
    JITINT32 second_half;

    union {
        unsigned long long ull;
        char c[8];
    } x;
    x.ull = 0x0123456789abcdefULL;

    METHOD_BEGIN(ildjitSystem, "System.Net.IPAddress.NetworkToHostOrder");
    first_half = (network & 0x00000000FFFFFFFFULL);
    second_half = (network & 0xFFFFFFFF00000000ULL) >> 32;
    first_half = ntohl(first_half);
    second_half = ntohl(second_half);

    if (x.c[0] == (char) 0xef) {
        result = (((JITINT64) first_half) << 32) | (((JITINT64) second_half) & 0x00000000FFFFFFFF);
    } else {
        result = (((JITINT64) second_half) << 32) | (((JITINT64) first_half) & 0x00000000FFFFFFFF);
    }

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Net.IPAddress.NetworkToHostOrder");
    return result;
}

JITNINT Platform_SocketMethods_GetInvalidHandle () {

    METHOD_BEGIN(ildjitSystem, "Platform.SocketMethods.GetInvalidHandle");


    /* Return			*/
    METHOD_END(ildjitSystem, "Platform.SocketMethods.GetInvalidHandle");
    return INVALID_HANDLE;
}

JITBOOLEAN Platform_SocketMethods_AddressFamilySupported (JITINT32 af) {
    JITBOOLEAN result = 0;

    METHOD_BEGIN(ildjitSystem, "Platform.SocketMethods.AddressFamilySupported");

    if (af == JIT_AF_INET) {
        result = 1;
    }
#ifdef JIT_IPV6_PRESENT
    if (af == JIT_AF_INET6) {
        result = 1;
    }
#endif
#ifdef JIT_IRDA_PRESENT
    if (af == JIT_AF_IRDA) {
        result = 1;
    }
#endif

    /* Return			*/
    METHOD_END(ildjitSystem, "Platform.SocketMethods.AddressFamilySupported");

    return result;
}

JITBOOLEAN Platform_DnsMethods_InternalGetHostByName (void* host, void** h_name, void** h_aliases, void** h_addr_list) {
    JITINT8         *literalUTF8;
    JITINT8         *pathAnsi;
    JITINT16        *literalUTF16;
    JITINT32 length;
    //t_stringManager	*stringManager = &((ildjitSystem->cliManager).CLR.stringManager);
    //t_arrayManager	*arrayManager = &((ildjitSystem->cliManager).CLR.arrayManager); //Don't know why, but using these as (stringManager->function()) immediatly SEGFAULTs the compiler
    struct hostent* h_ent = NULL;

    METHOD_BEGIN(ildjitSystem, "Platform.DnsMethods.InternalGetHostByName");
    if (host != NULL) {
        length = (ildjitSystem->cliManager).CLR.stringManager.getLength(host);
        if (length > 0) {
            literalUTF16 = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(host);
            assert(literalUTF16 != NULL);
            literalUTF8 = allocMemory(length * sizeof(char));
            (ildjitSystem->cliManager).CLR.stringManager.fromUTF16ToUTF8(literalUTF16, literalUTF8, length);

            pathAnsi = literalUTF8;
        }
    }

    if (!pathAnsi) { //The string is null or mem error
        METHOD_END(ildjitSystem, "Platform.DnsMethods.InternalGetHostByName");
        return 0;
    }

    h_ent = gethostbyname((char*) pathAnsi);
    freeMemory(literalUTF8);
    if (!h_ent) {
        METHOD_END(ildjitSystem, "Platform.DnsMethods.InternalGetHostByName");
        return 0;
    }

    if (!(ildjitSystem->cliManager).CLR.netManager.toIPHostEntry(h_ent, h_name, h_aliases, h_addr_list)) {
        METHOD_END(ildjitSystem, "Platform.DnsMethods.InternalGetHostByName");
        return 0;
    }

    /*	*h_name =  CLIMANAGER_StringManager_newInstanceFromUTF8((JITINT8*)h_ent->h_name, strlen(h_ent->h_name));

            length = -1;
            list = xanList_new(malloc, free, NULL);
            assert(list != NULL);
            while(h_ent->h_aliases[++length] != NULL) //Adding aliases to a list
            {
                    void* temp_string = CLIMANAGER_StringManager_newInstanceFromUTF8((JITINT8*)h_ent->h_aliases[length], strlen(h_ent->h_aliases[length]));
                    assert(temp_string != NULL);
                    list->append(list, temp_string);
            }

       *h_aliases = (ildjitSystem->cliManager).CLR.arrayManager.newInstanceFromList(list, (ildjitSystem->cliManager).CLR.stringManager.StringType);
            if(!(*h_aliases)) //The string is null or mem error
            {
              METHOD_END(ildjitSystem, "Platform.DnsMethods.InternalGetHostByName");
              return 0;
            }

            list->destroyList(list);

            length = -1;
            int64Type = (ildjitSystem->cliManager).metadataManager.getTypeDescriptorFromElementType(&((ildjitSystem->cliManager).metadataManager),ELEMENT_TYPE_I8);
            while(h_ent->h_addr_list[++length] != NULL); //Counting addresses
       *h_addr_list = (ildjitSystem->cliManager).CLR.arrayManager.newInstanceFromType(int64Type, length);
            if(!(*h_addr_list)) //The string is null or mem error
            {
              METHOD_END(ildjitSystem, "Platform.DnsMethods.InternalGetHostByName");
              return 0;
            }

            while(length--) //Populating the array
            {
              JITINT64 *element = allocMemory(sizeof(JITINT64));
              if(!(element)) //Mem error
              {
                METHOD_END(ildjitSystem, "Platform.DnsMethods.InternalGetHostByName");
                return 0;
              }
       *element = (JITINT64) *((JITINT32*)h_ent->h_addr_list[length]) & 0x00000000FFFFFFFFLL;
              (ildjitSystem->cliManager).CLR.arrayManager.setValueArrayElement(*h_addr_list, length, element);
            }*/
    /* Return			*/
    METHOD_END(ildjitSystem, "Platform.DnsMethods.InternalGetHostByName");

    return 1;
}

JITBOOLEAN Platform_DnsMethods_InternalGetHostByAddr (JITINT64 address, void** h_name, void** h_aliases, void** h_addr_list) {
    //t_stringManager	*stringManager = &((ildjitSystem->cliManager).CLR.stringManager);
    //t_arrayManager	*arrayManager = &((ildjitSystem->cliManager).CLR.arrayManager); //Don't know why, but using these as (stringManager->function()) immediatly SEGFAULTs the compiler
    struct hostent* h_ent = NULL;
    struct in_addr addr;

    METHOD_BEGIN(ildjitSystem, "Platform.DnsMethods.InternalGetHostByAddr");
    addr.s_addr = (unsigned long) address;
    h_ent = gethostbyaddr((const char *) &addr, sizeof(addr), AF_INET);

    if (!h_ent) {
        METHOD_END(ildjitSystem, "Platform.DnsMethods.InternalGetHostByAddr");
        return 0;
    }

    if (!(ildjitSystem->cliManager).CLR.netManager.toIPHostEntry(h_ent, h_name, h_aliases, h_addr_list)) {
        METHOD_END(ildjitSystem, "Platform.DnsMethods.InternalGetHostByAddr");
        return 0;
    }
    METHOD_END(ildjitSystem, "Platform.DnsMethods.InternalGetHostByAddr");

    return 1;
}

void* Platform_DnsMethods_InternalGetHostName () {
    JITINT8 hostname[HOST_NAME_MAX];
    void*           hostname_string;

    METHOD_BEGIN(ildjitSystem, "Platform.DnsMethods.InternalGetHostName");
    if (gethostname((char *) hostname, HOST_NAME_MAX)) {
        METHOD_END(ildjitSystem, "Platform.DnsMethods.InternalGetHostName");
        return NULL;
    }

    hostname_string = CLIMANAGER_StringManager_newInstanceFromUTF8(hostname, strlen((char *) hostname));

    METHOD_END(ildjitSystem, "Platform.DnsMethods.InternalGetHostName");

    return hostname_string;

}

JITBOOLEAN Platform_SocketMethods_Create (JITINT32 af, JITINT32 st, JITINT32 pt, JITNINT* handle) {
    JITBOOLEAN result;

    METHOD_BEGIN(ildjitSystem, "Platform.SocketMethods.Create");

    if (af == JIT_AF_INET) {
        af = AF_INET;
    }
#ifdef JIT_IPV6_PRESENT
    else if (af == JIT_AF_INET6) {
        af = AF_INET6;
    }
#endif
#ifdef JIT_IRDA_PRESENT
    else if (af == JIT_AF_IRDA) {
        af = AF_IRDA;
    }
#endif

    switch (st) {
        case JIT_SOCK_STREAM: {
            st = SOCK_STREAM;
        }
        break;
        case JIT_SOCK_DGRAM: {
            st = SOCK_DGRAM;
        }
        break;
        case JIT_SOCK_RAW: {
            st = SOCK_RAW;
        }
        break;
#ifdef SOCK_SEQPACKET
        case JIT_SOCK_SEQPACKET: {
            st = SOCK_SEQPACKET;
        }
        break;
#endif
        default: {
            st = -1;
        }
        break;
    }

    *handle = socket(af, st, pt);
#ifdef HAVE_FCNTL
    if ( *handle >= 0 ) {
        fcntl( *handle, F_SETFD, FD_CLOEXEC );
    }
#endif
    if (*handle >= 0) {
        result = 1;
    } else {
        result = 0;
    }
    METHOD_BEGIN(ildjitSystem, "Platform.SocketMethods.Create");

    return result;
}

JITBOOLEAN Platform_SocketMethods_Bind (JITNINT handle, void* addr) {
    //JITBOOLEAN	result;
    SockAddr sock_addr;
    int sa_len;

    METHOD_BEGIN(ildjitSystem, "Platform.SocketMethods.Bind");
    if (!(ildjitSystem->cliManager).CLR.netManager.convertAddressToSockAddr(addr, &sock_addr, &sa_len)) {
        METHOD_END(ildjitSystem, "Platform.SocketMethods.Bind");
        return 0;
    }

    if (bind((int) handle, &sock_addr.addr, sa_len) == 0) {
        METHOD_END(ildjitSystem, "Platform.SocketMethods.Bind");
        return 1;
    }


    METHOD_END(ildjitSystem, "Platform.SocketMethods.Bind");
    return 0;
}

JITBOOLEAN Platform_SocketMethods_Shutdown (JITNINT handle, JITINT32 how) {
    JITBOOLEAN result;

    METHOD_BEGIN(ildjitSystem, "Platform.SocketMethods.Shutdown");
    result = (shutdown((int) handle, how) == 0);
    METHOD_END(ildjitSystem, "Platform.SocketMethods.Shutdown");

    return result;
}

JITBOOLEAN Platform_SocketMethods_Listen (JITNINT handle, JITINT32 backlog) {
    JITBOOLEAN result;

    METHOD_BEGIN(ildjitSystem, "Platform.SocketMethods.Listen");
    result = (listen((int) handle, backlog) == 0);
    METHOD_END(ildjitSystem, "Platform.SocketMethods.Listen");

    return result;
}

JITBOOLEAN Platform_SocketMethods_Accept (JITNINT handle, void* addr, JITNINT* new_handle) {
    SockAddr sa_addr;
    int sa_len;

    METHOD_BEGIN(ildjitSystem, "Platform.SocketMethods.Accept");
    sa_len = sizeof(SockAddr);
    memset(&sa_addr, 0, sizeof(sa_addr));
    *new_handle = accept((int) handle, &sa_addr.addr, (socklen_t*) &sa_len);
    if (*new_handle < 0) {
        METHOD_END(ildjitSystem, "Platform.SocketMethods.Accept");
        return 0;
    }

    if (!(ildjitSystem->cliManager).CLR.netManager.convertSockAddrToAddress(addr, &sa_addr)) {
        close(*new_handle);
        errno = EINVAL;
        METHOD_END(ildjitSystem, "Platform.SocketMethods.Accept");
        return 0;
    }

    METHOD_END(ildjitSystem, "Platform.SocketMethods.Accept");

    return 1;
}

JITBOOLEAN Platform_SocketMethods_Connect (JITNINT handle, void* addr) {
    SockAddr sa_addr;
    int sa_len;

    METHOD_BEGIN(ildjitSystem, "Platform.SocketMethods.Connect");

    if (!(ildjitSystem->cliManager).CLR.netManager.convertAddressToSockAddr(addr, &sa_addr, &sa_len)) {
        METHOD_END(ildjitSystem, "Platform.SocketMethods.Connect");
        return 0;
    }

    if (!(connect((int) handle, &sa_addr.addr, sa_len) == 0)) {
        METHOD_END(ildjitSystem, "Platform.SocketMethods.Connect");
        return 0;
    }

    METHOD_END(ildjitSystem, "Platform.SocketMethods.Connect");

    return 1;
}

JITINT32 Platform_SocketMethods_Receive (JITNINT handle, void* buffer, JITINT32 offset, JITINT32 size, JITINT32 flags) {
    JITUINT8*       buf;
    JITINT32 result;

    METHOD_BEGIN(ildjitSystem, "Platform.SocketMethods.Receive");
    buf = (ildjitSystem->cliManager).CLR.arrayManager.getArrayElement(buffer, offset);

    /*if (buf == NULL)
       {
       METHOD_END(ildjitSystem, "Platform.SocketMethods.Receive");
       return 0;
       }*/

    result = recv(handle, buf, size, flags);

    METHOD_END(ildjitSystem, "Platform.SocketMethods.Receive");

    return result;
}

JITINT32 Platform_SocketMethods_ReceiveFrom (JITNINT handle, void* buffer, JITINT32 offset, JITINT32 size, JITINT32 flags, void* addr_return) {
    JITUINT8*       buf;
    JITINT32 result;
    SockAddr sa_addr;
    int sa_len;

    METHOD_BEGIN(ildjitSystem, "Platform.SocketMethods.ReceiveFrom");
    buf = (ildjitSystem->cliManager).CLR.arrayManager.getArrayElement(buffer, offset);

    /*if (buf == NULL)
       {
       METHOD_END(ildjitSystem, "Platform.SocketMethods.Receive");
       return -1;
       }*/

    sa_len = sizeof(SockAddr);
    memset(&sa_addr, 0, sizeof(sa_addr));
    result = (JITINT32) recvfrom(handle, buf, size, flags,
                                 &sa_addr.addr, (socklen_t*) &sa_len);
    if (result < 0) {
        METHOD_END(ildjitSystem, "Platform.SocketMethods.ReceiveFrom");
        return result;
    }

    if (!(ildjitSystem->cliManager).CLR.netManager.convertSockAddrToAddress(addr_return, &sa_addr)) {
        errno = EINVAL;
        METHOD_END(ildjitSystem, "Platform.SocketMethods.ReceiveFrom");
        return -1;
    }

    METHOD_END(ildjitSystem, "Platform.SocketMethods.ReceiveFrom");
    return result;
}

JITINT32 Platform_SocketMethods_Send (JITNINT handle, void* buffer, JITINT32 offset, JITINT32 size, JITINT32 flags) {
    JITUINT8*       buf;
    JITINT32 result;

    METHOD_BEGIN(ildjitSystem, "Platform.SocketMethods.Send");
    buf = (ildjitSystem->cliManager).CLR.arrayManager.getArrayElement(buffer, offset);

    /*if (buf == NULL)
       {
       METHOD_END(ildjitSystem, "Platform.SocketMethods.Send");
       return 0;
       }*/
#ifdef MSG_NOSIGNAL
    result = send(handle, buf, size, flags | MSG_NOSIGNAL);
#else
    result = send(handle, buf, size, flags);
#endif
    METHOD_END(ildjitSystem, "Platform.SocketMethods.Send");

    return result;
}

JITINT32 Platform_SocketMethods_SendTo (JITNINT handle, void* buffer, JITINT32 offset, JITINT32 size, JITINT32 flags, void* addr_return) {
    JITUINT8*       buf;
    JITINT32 result;
    SockAddr sa_addr;
    int sa_len;

    METHOD_BEGIN(ildjitSystem, "Platform.SocketMethods.SendTo");
    buf = (ildjitSystem->cliManager).CLR.arrayManager.getArrayElement(buffer, offset);

    if (!(ildjitSystem->cliManager).CLR.netManager.convertAddressToSockAddr(addr_return, &sa_addr, &sa_len)) {
        errno = EINVAL;
        METHOD_END(ildjitSystem, "Platform.SocketMethods.SendTo");
        return 0;
    }

    /*if (buf == NULL)
       {
       METHOD_END(ildjitSystem, "Platform.SocketMethods.SendTo");
       return -1;
       }*/
#ifdef MSG_NOSIGNAL
    result = (JITINT32) sendto(handle, buf, size, flags | MSG_NOSIGNAL,
                               &sa_addr.addr, (socklen_t) sa_len);
#else
    result = (JITINT32) sendto(handle, buf, size, flags,
                               &sa_addr.addr, (socklen_t) sa_len);
#endif
    METHOD_END(ildjitSystem, "Platform.SocketMethods.SendTo");
    return result;
}

JITBOOLEAN Platform_SocketMethods_Close (JITNINT handle) {
    JITBOOLEAN result;

    METHOD_BEGIN(ildjitSystem, "Platform.SocketMethods.GetInvalidHandle");

    result = (close((int) handle) == 0);

    /* Return			*/
    METHOD_END(ildjitSystem, "Platform.SocketMethods.GetInvalidHandle");
    return result;
}

JITINT32 Platform_SocketMethods_Select (void* readarray, void* writearray, void* errorarray, JITINT64 timeout) {
    JITNINT *readarrayn = NULL;
    JITNINT *writearrayn = NULL;
    JITNINT *errorarrayn = NULL;
    fd_set readSet, writeSet, exceptSet;
    fd_set *readPtr, *writePtr, *exceptPtr;
    int highest = -1;
    int fd, result;
    JITUINT32 numRead = 0;
    JITUINT32 numWrite = 0;
    JITUINT32 numExcept = 0;
    JITINT64 currtime_sec, endtime_sec;
    JITUINT32 currtime_nsec, endtime_nsec;
    struct timeval difftime;
    JITINT32 index;

    METHOD_BEGIN(ildjitSystem, "Platform.SocketMethods.Select");

    if (readarray) {
        readarrayn = (ildjitSystem->cliManager).CLR.arrayManager.getArrayElement(readarray, 0);
        numRead = (ildjitSystem->cliManager).CLR.arrayManager.getArrayLength(readarray, 0);
    }
    if (writearray) {
        writearrayn = (ildjitSystem->cliManager).CLR.arrayManager.getArrayElement(writearray, 0);
        numWrite = (ildjitSystem->cliManager).CLR.arrayManager.getArrayLength(writearray, 0);
    }
    if (errorarray) {
        errorarrayn = (ildjitSystem->cliManager).CLR.arrayManager.getArrayElement(errorarray, 0);
        numExcept = (ildjitSystem->cliManager).CLR.arrayManager.getArrayLength(errorarray, 0);
    }

    /* Convert the read array into an "fd_set" */
    FD_ZERO(&readSet);
    if (readarrayn) {
        readPtr = &readSet;
        for (index = 0; index < numRead; ++index) {
            fd = (int) (readarrayn[index]);
            if (fd != -1) {
                FD_SET(fd, &readSet);
                if (fd > highest) {
                    highest = fd;
                }
            }
        }
    } else {
        readPtr = 0;
    }

    /* Convert the write array into an "fd_set" */
    FD_ZERO(&writeSet);
    if (writearrayn) {
        writePtr = &writeSet;
        for (index = 0; index < numWrite; ++index) {
            fd = (int) (writearrayn[index]);
            if (fd != -1) {
                FD_SET(fd, &writeSet);
                if (fd > highest) {
                    highest = fd;
                }
            }
        }
    } else {
        writePtr = 0;
    }

    /* Convert the except array into an "fd_set" */
    FD_ZERO(&exceptSet);
    if (errorarrayn) {
        exceptPtr = &exceptSet;
        for (index = 0; index < numExcept; ++index) {
            fd = (int) (errorarrayn[index]);
            if (fd != -1) {
                FD_SET(fd, &exceptSet);
                if (fd > highest) {
                    highest = fd;
                }
            }
        }
    } else {
        exceptPtr = 0;
    }

    if (timeout >= 0) {
        /* Get the current time of day and determine the end time */
        GET_TIME(currtime_sec, currtime_nsec)
        endtime_sec = currtime_sec + (long) (timeout / (JITINT64) 1000000);
        endtime_nsec = currtime_nsec +
                       (long) ((timeout % (JITINT64) 1000000) * (JITINT64) 1000);
        if (endtime_nsec >= 1000000000L) {
            ++(endtime_sec);
            endtime_nsec -= 1000000000L;
        }

        /* Loop while we are interrupted by signals */
        for (;; ) {
            /* How long until the timeout? */
            difftime.tv_sec = (time_t) (endtime_sec - currtime_sec);
            if (endtime_nsec >= currtime_nsec) {
                difftime.tv_usec =
                    (long) ((endtime_nsec - currtime_nsec) / 1000);
            } else {
                difftime.tv_usec =
                    (endtime_nsec + 1000000000L - currtime_nsec) / 1000;
                difftime.tv_sec -= 1;
            }

            /* Perform a trial select, which may be interrupted */
            result = select(highest + 1, readPtr, writePtr,
                            exceptPtr, &difftime);

            if (result >= 0 || errno != EINTR) {
                break;
            }

            /* We were interrupted, so get the current time again.
               We have to this because many ildjitSystems do not update
               the 5th paramter of "select" to reflect how much time
               is left to go */
            GET_TIME(currtime_sec, currtime_nsec)

            /* Are we past the end time? */
            if (currtime_sec > endtime_sec ||
                    (currtime_sec == endtime_sec &&
                     currtime_nsec >= endtime_nsec)) {
                /* We are, so simulate timeout */
                result = 0;
                break;
            }
        }
    } else {
        /* Infinite select */
        while ((result = select(highest + 1, readPtr, writePtr, exceptPtr,
                                (struct timeval *) 0)) < 0) {
            /* Keep looping while we are being interrupted by signals */
            if (errno != EINTR) {
                break;
            }
        }
    }

    /* Update the descriptor sets if something fired */
    if (result > 0) {
        /* Update the read descriptors */
        if (readPtr) {
            for (index = 0; index < numRead; ++index) {
                fd = (int) (readarrayn[index]);
                if (fd != -1 && !FD_ISSET(fd, &readSet)) {
                    readarrayn[index] = INVALID_HANDLE;
                }
            }
        }

        /* Update the write descriptors */
        if (writePtr) {
            for (index = 0; index < numWrite; ++index) {
                fd = (int) (writearrayn[index]);
                if (fd != -1 && !FD_ISSET(fd, &writeSet)) {
                    writearrayn[index] = INVALID_HANDLE;
                }
            }
        }

        /* Update the except descriptors */
        if (exceptPtr) {
            for (index = 0; index < numExcept; ++index) {
                fd = (int) (errorarrayn[index]);
                if (fd != -1 && !FD_ISSET(fd, &exceptSet)) {
                    errorarrayn[index] = INVALID_HANDLE;
                }
            }
        }
    }

    /* Return			*/
    METHOD_END(ildjitSystem, "Platform.SocketMethods.Select");



    return (JITINT32) result;
}

JITBOOLEAN Platform_SocketMethods_SetBlocking (JITNINT handle, JITBOOLEAN blocking) {
    JITBOOLEAN result;

    METHOD_BEGIN(ildjitSystem, "Platform.SocketMethods.SetBlocking");

#if defined(FIONBIO) && defined(HAVE_IOCTL)
    result = ioctl((int) handle, FIONBIO, (void *) &blocking) >= 0;
#else
    errno = EINVAL;
    result = 0;
#endif
    METHOD_END(ildjitSystem, "Platform.SocketMethods.SetBlocking");

    return result;
}

JITINT32 Platform_SocketMethods_GetAvailable (JITNINT handle) {
    JITINT32 result;

    METHOD_BEGIN(ildjitSystem, "Platform.SocketMethods.GetAvailable");

#if defined(FIONREAD) && defined(HAVE_IOCTL)

    ioctl((int) handle, FIONREAD, (void *) &result);

    if (result < 0) {
        result = -1;
    }
#else
    result = 0;
#endif
    METHOD_END(ildjitSystem, "Platform.SocketMethods.GetAvailable");

    return result;
}

JITBOOLEAN Platform_SocketMethods_GetSockName (JITNINT handle, void* addr_return) {
    SockAddr sa_addr;
    socklen_t sa_len;
    JITBOOLEAN result;

    METHOD_BEGIN(ildjitSystem, "Platform.SocketMethods.GetSockName");


    /* Accept the incoming connection */
    sa_len = sizeof(SockAddr);
    memset(&sa_addr, 0, sizeof(sa_addr));
    if (getsockname((int) handle, &sa_addr.addr, &sa_len) < 0) {
        result = 0;
    } else {
        /* Convert the platform address into its serialized form */
        if (!(ildjitSystem->cliManager).CLR.netManager.convertSockAddrToAddress(addr_return, &sa_addr)) {
            errno = EINVAL;
            result = 0;
        }
    }
    METHOD_END(ildjitSystem, "Platform.SocketMethods.GetSockName");

    result = 1;

    return result;
}

JITBOOLEAN Platform_SocketMethods_SetSocketOption (JITNINT handle, JITINT32 level, JITINT32 name, JITINT32 value) {
    JITBOOLEAN result;
    JITINT32 nativeLevel, nativeName;

    /* Initialize the variables	*/
    nativeName = 0;
    nativeLevel = 0;

    METHOD_BEGIN(ildjitSystem, "Platform.SocketMethods.SetSocketOption");
#ifdef HAVE_SETSOCKOPT
    if ((ildjitSystem->cliManager).CLR.netManager.convertSocketOptsToNative(level, name, &nativeLevel,&nativeName)==0) {
        errno = EINVAL;
        result = 0;
    } else {
        result = (setsockopt((int) handle,nativeLevel,nativeName,
                             (void *) &value,       sizeof(value))== 0);
    }
#else
    errno = EINVAL;
    result = 0;
#endif
    METHOD_END(ildjitSystem, "Platform.SocketMethods.SetSocketOption");

    return result;
}

JITBOOLEAN Platform_SocketMethods_GetSocketOption (JITNINT handle, JITINT32 level, JITINT32 name, JITINT32* value) {
    JITBOOLEAN result;
    JITINT32 option = 0;
    JITINT32 len = sizeof(JITINT32);
    JITINT32 nativeLevel, nativeName;

    /* Initialize the variables	*/
    nativeName = 0;
    nativeLevel = 0;

    METHOD_BEGIN(ildjitSystem, "Platform.SocketMethods.GetSocketOption");
#ifdef HAVE_GETSOCKOPT
    if ((ildjitSystem->cliManager).CLR.netManager.convertSocketOptsToNative(level, name, &nativeLevel,&nativeName)==0) {
        errno = EINVAL;
        result = 0;
    }
    if (getsockopt((int) handle,nativeLevel,nativeName, (void *) &option, (socklen_t*) &len) == 0) {
        *value = (JITINT32) option;
        result = 1;
    } else {
        result = 0;
    }
#else
    errno = EINVAL;
    result = 0;
#endif
    METHOD_END(ildjitSystem, "Platform.SocketMethods.GetSocketOption");

    return result;
}

JITBOOLEAN Platform_SocketMethods_SetLingerOption (JITNINT handle, JITBOOLEAN enabled, JITINT32 seconds) {
    JITBOOLEAN result;

#if defined(HAVE_SETSOCKOPT) && defined(SO_LINGER)
    struct linger _linger;
#endif
    METHOD_BEGIN(ildjitSystem, "Platform.SocketMethods.SetLingerOption");
#if defined(HAVE_SETSOCKOPT) && defined(SO_LINGER)
    _linger.l_onoff = enabled;
    _linger.l_linger = seconds;
    if (setsockopt((int) handle, SOL_SOCKET, SO_LINGER, (void *) &(_linger), sizeof(struct linger)) < 0) {
        result = 0;
    } else {
        result = 1;
    }
#else
    errno = EINVAL;
    result = 0;
#endif
    METHOD_END(ildjitSystem, "Platform.SocketMethods.SetLingerOption");

    return result;
}

JITBOOLEAN Platform_SocketMethods_GetLingerOption (JITNINT handle, JITBOOLEAN* enabled, JITINT32* seconds) {
    JITBOOLEAN result;

#if defined(HAVE_SETSOCKOPT) && defined(SO_LINGER)
    struct linger _linger;
    int size = sizeof(struct linger);
#endif
    METHOD_BEGIN(ildjitSystem, "Platform.SocketMethods.GetLingerOption");
#if defined(HAVE_SETSOCKOPT) && defined(SO_LINGER)
    if (getsockopt((int) handle, SOL_SOCKET, SO_LINGER,
                   (void *) &(_linger), (socklen_t*) &size) < 0) {
        result = 0;
        *enabled = 0;
        *seconds = 0;
    } else {
        *enabled = _linger.l_onoff;
        *seconds = _linger.l_linger;
        result = 1;
    }
#else
    errno = EINVAL;
    result = 0;
#endif
    METHOD_END(ildjitSystem, "Platform.SocketMethods.GetLingerOption");

    return result;
}

JITBOOLEAN Platform_SocketMethods_SetMulticastOption (JITNINT handle, JITINT32 af, JITINT32 name, void* group, void* mcint) {
    SockAddr group_sa, mcint_sa;
    MreqIp req;
    int sa_len1, sa_len2;
    JITBOOLEAN result = 0;

    METHOD_BEGIN(ildjitSystem, "Platform.SocketMethods.SetMulticastOption");
    memset(&req,0,sizeof(MreqIp));
    switch (af) {
        case JIT_AF_INET:
            if (name == JIT_MULTICAST_ADD) {
                if ((ildjitSystem->cliManager).CLR.netManager.convertAddressToSockAddr(group, &group_sa, &sa_len1) && (ildjitSystem->cliManager).CLR.netManager.convertAddressToSockAddr(mcint, &mcint_sa, &sa_len2)) {
                    req.ipv4_req.imr_multiaddr.s_addr = group_sa.ipv4_addr.sin_addr.s_addr;
                    req.ipv4_req.imr_interface.s_addr = mcint_sa.ipv4_addr.sin_addr.s_addr;
                    if (name == JIT_MULTICAST_ADD) {
                        if (!setsockopt((int) handle, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                                        &req.ipv4_req, sizeof(struct ip_mreq))) {
                            result = 1;
                        }
                    } else if (name == JIT_MULTICAST_DROP) {
                        if (!setsockopt((int) handle, IPPROTO_IP, IP_DROP_MEMBERSHIP,
                                        &req.ipv4_req, sizeof(struct ip_mreq))) {
                            result = 1;
                        }
                    }
                }
            }
            break;
#ifdef JIT_IPV6_PRESENT
        case JIT_AF_INET6:
            if (name == JIT_MULTICAST_ADD) {
                if ((ildjitSystem->cliManager).CLR.netManager.convertAddressToSockAddr(group, &group_sa, &sa_len1) && (ildjitSystem->cliManager).CLR.netManager.convertAddressToSockAddr(mcint, &mcint_sa, &sa_len2)) {
                    char buf[1024];
                    struct ifconf ifc;
                    struct ifreq *ifr;
                    int nInterfaces;
                    int i;

                    ifc.ifc_len = sizeof(buf);
                    ifc.ifc_buf = buf;
                    if (ioctl((int) handle, SIOCGIFCONF, &ifc) >= 0) {
                        ifr = ifc.ifc_req;
                        nInterfaces = ifc.ifc_len / sizeof(struct ifreq);
                        for (i = 0; i < nInterfaces; i++) {
                            struct ifreq *item = &ifr[i];
                            if (!memcmp(((struct sockaddr_in6 *) &item->ifr_addr)->sin6_addr.s6_addr, mcint_sa.ipv6_addr.sin6_addr.s6_addr, sa_len2)) {
                                req.ipv6_req.ipv6mr_interface = i;
                            }
                        }
                        memcpy(req.ipv6_req.ipv6mr_multiaddr.s6_addr, group_sa.ipv6_addr.sin6_addr.s6_addr, sizeof(struct in6_addr));
                        //req.ipv6_req.ipv6mr_multiaddr.s6_addr = group_sa.ipv6_addr.sin6_addr.s6_addr;
                        //req.ipv6_req.ipv6mr_interface.s6_addr = mcint_sa.ipv6_addr.sin6_addr.s6_addr;
                        if (name == JIT_MULTICAST_ADD) {
                            if (!setsockopt((int) handle, IPPROTO_IPV6, IPV6_JOIN_GROUP,
                                            &req.ipv6_req, sizeof(struct ipv6_mreq))) {
                                result = 1;
                            }
                        } else if (name == JIT_MULTICAST_DROP) {
                            if (!setsockopt((int) handle, IPPROTO_IP, IPV6_LEAVE_GROUP,
                                            &req.ipv6_req, sizeof(struct ipv6_mreq))) {
                                result = 1;
                            }
                        }
                    }
                }
            }
            break;
#endif
        default:
            break;
    }
    METHOD_END(ildjitSystem, "Platform.SocketMethods.SetMulticastOption");

    return result;
}

JITBOOLEAN Platform_SocketMethods_GetMulticastOption (JITNINT handle, JITINT32 af, JITINT32 name, void* group, void* mcint) {

    METHOD_BEGIN(ildjitSystem, "Platform.SocketMethods.GetMulticastOption");
    // NOT SUPPORTED ON LINUX
    METHOD_END(ildjitSystem, "Platform.SocketMethods.GetMulticastOption");

    return 0;
}

JITBOOLEAN Platform_SocketMethods_CanStartThreads () {
    METHOD_BEGIN(ildjitSystem, "Platform.SocketMethods.CanStartThreads");
    METHOD_END(ildjitSystem, "Platform.SocketMethods.CanStartThreads");

    return (JITBOOLEAN) 1;
}

JITBOOLEAN Platform_SocketMethods_DiscoverIrDADevices (JITNINT handle, void* buf) {
    int size;
    JITBOOLEAN result = 0;

#ifdef JIT_IRDA_PRESENT
    struct irda_device_list dev_list;
#endif
    METHOD_BEGIN(ildjitSystem, "Platform.SocketMethods.DiscoverIrDADevices");
#if defined(JIT_IRDA_PRESENT) && defined(HAVE_GETSOCKOPT)
    size = (ildjitSystem->cliManager).CLR.arrayManager.getArrayLengthInBytes(buf);
    if (getsockopt((int) handle,SOL_IRDA,IRLMP_ENUMDEVICES, (void *) &dev_list, (socklen_t*) &size) == 0) {
        memcpy(buf, (void*) &dev_list, size);
        result = 1;
    }
#else
    result = 0;
#endif
    METHOD_END(ildjitSystem, "Platform.SocketMethods.DiscoverIrDADevices");

    return result;
}

JITINT32 Platform_SocketMethods_GetErrno () {

    METHOD_BEGIN(ildjitSystem, "Platform.SocketMethods.GetErrno");
    METHOD_END(ildjitSystem, "Platform.SocketMethods.GetErrno");

    return (JITINT32) errno;
}

void* Platform_SocketMethods_GetErrnoMessage (JITINT32 error) {
    void* result;
    int errno_value;
    char* message;

    METHOD_BEGIN(ildjitSystem, "Platform.SocketMethods.GetErrnoMessage");
    if (error >= 0 && error < errnoFromMapSize) {
        errno_value = errnoFromMapTable[error];
    } else {
        errno_value = -1;
    }

    message = strerror(errno_value);
    result = CLIMANAGER_StringManager_newInstanceFromUTF8((JITINT8*) message, strlen(message));

    METHOD_END(ildjitSystem, "Platform.SocketMethods.GetErrnoMessage");

    return result;
}

JITBOOLEAN Platform_SocketMethods_QueueCompletionItem (void* callback, void* state) {
    JITBOOLEAN*     result;
    TypeDescriptor* threadpool_type;
    MethodDescriptor* queuecomplitem_method;
    XanList* method_list, *parameters_array_list;
    void* parameters_array;
    t_methods *methods = &((ildjitSystem->cliManager).methods);

    METHOD_BEGIN(ildjitSystem, "Platform.SocketMethods.QueueCompletionItem");
    parameters_array_list = xanList_new(allocFunction, freeFunction, NULL);
    xanList_append(parameters_array_list, callback);
    xanList_append(parameters_array_list, state);
    parameters_array = (ildjitSystem->cliManager).CLR.arrayManager.newInstanceFromList(parameters_array_list, ((ildjitSystem->cliManager).metadataManager->System_Object)->descriptor);
    xanList_destroyList(parameters_array_list);
    threadpool_type = (ildjitSystem->cliManager).metadataManager->getTypeDescriptorFromName((ildjitSystem->cliManager).metadataManager, (JITINT8*) "System.Threading", (JITINT8*) "ThreadPool");
    method_list = threadpool_type->getMethodFromName(threadpool_type, (JITINT8*) "QueueCompletionItem");
    queuecomplitem_method = (MethodDescriptor *) xanList_first(method_list)->data;
    result = (JITBOOLEAN*) InvokeMethod(methods->fetchOrCreateMethod(methods, queuecomplitem_method, JITTRUE), NULL, parameters_array);
    METHOD_END(ildjitSystem, "Platform.SocketMethods.QueueCompletionItem");

    return *result;
}

void* Platform_SocketMethods_CreateManualResetEvent () {
    void*   result;
    TypeDescriptor* manualresetevent_type;
    MethodDescriptor* ctor_method;
    XanList* method_list, *parameters_array_list;
    JITBOOLEAN parameter;
    void* parameters_array;
    t_methods *methods = &((ildjitSystem->cliManager).methods);

    METHOD_BEGIN(ildjitSystem, "Platform.SocketMethods.CreateManualResetEvent");
    parameter = 0;
    parameters_array_list = xanList_new(allocFunction, freeFunction, NULL);
    xanList_append(parameters_array_list, &parameter);
    parameters_array = (ildjitSystem->cliManager).CLR.arrayManager.newInstanceFromList(parameters_array_list, ((ildjitSystem->cliManager).metadataManager->System_Boolean)->descriptor);
    xanList_destroyList(parameters_array_list);
    manualresetevent_type = (ildjitSystem->cliManager).metadataManager->getTypeDescriptorFromName((ildjitSystem->cliManager).metadataManager, (JITINT8*) "System.Threading", (JITINT8*) "ManualResetEvent");
    result = (ildjitSystem->garbage_collectors).gc.allocObject(manualresetevent_type, 0);
    assert(result != NULL);
    method_list = manualresetevent_type->getConstructors(manualresetevent_type);
    ctor_method = (MethodDescriptor *) xanList_first(method_list)->data;
    InvokeMethod(methods->fetchOrCreateMethod(methods, ctor_method, JITTRUE), result, parameters_array);
    METHOD_END(ildjitSystem, "Platform.SocketMethods.CreateManualResetEvent");

    return result;
}

void Platform_SocketMethods_WaitHandleSet (void* wait_handle) {
    TypeDescriptor* manualresetevent_type;
    MethodDescriptor* set_method;
    XanList* method_list; //, *parameters_array_list;

    //JITBOOLEAN parameter;
    //void* parameters_array;

    t_methods *methods = &((ildjitSystem->cliManager).methods);
    METHOD_BEGIN(ildjitSystem, "Platform.SocketMethods.WaitHandleSet");
    /*	parameters_array_list = xanList_new(allocFunction, freeFunction, NULL);
    	parameters_array_list->append(parameters_array_list, wait_handle);
    	parameters_array = (ildjitSystem->cliManager).CLR.arrayManager.newInstanceFromList(parameters_array_list, ((ildjitSystem->cliManager).metadataManager.System_Object)->descriptor);
    	parameters_array_list->destroyList(parameters_array_list);*/
    manualresetevent_type = (ildjitSystem->cliManager).metadataManager->getTypeDescriptorFromName((ildjitSystem->cliManager).metadataManager, (JITINT8*) "System.Threading", (JITINT8*) "ManualResetEvent");
    method_list = manualresetevent_type->getMethodFromName(manualresetevent_type, (JITINT8*) "Set");
    set_method = (MethodDescriptor *) xanList_first(method_list)->data;
    InvokeMethod(methods->fetchOrCreateMethod(methods, set_method, JITTRUE), wait_handle, NULL);
    METHOD_END(ildjitSystem, "Platform.SocketMethods.WaitHandleSet");

    return;

}

JITBOOLEAN Platform_PortMethods_IsValid (JITINT32 type, JITINT32 portNumber) {
    JITBOOLEAN result = 0;
    JITINT8 name[64];

    METHOD_BEGIN(ildjitSystem, "Platform.PortMethods.IsValid");
    if (!((type != JIT_SERIAL_REGULAR && type != JIT_SERIAL_INFRARED &&
            type != JIT_SERIAL_USB && type != JIT_SERIAL_RFCOMM) ||
            (portNumber < 1 || portNumber > 256))) {
        (ildjitSystem->cliManager).CLR.netManager.GetSerialName(name, type, portNumber);
#ifdef HAVE_ACCESS
        result = (access((char *)name, 0) >= 0);
#else
        {
            FILE *file = fopen(filename, "r");
            if (file) {
                fclose(file);
                result = 1;
            }
        }
#endif
    }
    METHOD_END(ildjitSystem, "Platform.PortMethods.IsValid");

    return result;
}

JITBOOLEAN Platform_PortMethods_IsAccessible (JITINT32 type, JITINT32 portNumber) {
    JITBOOLEAN result = 0;
    JITINT8 name[64];

    METHOD_BEGIN(ildjitSystem, "Platform.PortMethods.IsAccessible");
    if (!((type != JIT_SERIAL_REGULAR && type != JIT_SERIAL_INFRARED &&
            type != JIT_SERIAL_USB && type != JIT_SERIAL_RFCOMM) ||
            (portNumber < 1 || portNumber > 256))) {
        (ildjitSystem->cliManager).CLR.netManager.GetSerialName(name, type, portNumber);
#ifdef HAVE_ACCESS
#ifdef W_OK
        result = (access((char *)name, W_OK) >= 0);
#else
        result = (access(name, 2) >= 0);
#endif
#else
        {
            FILE *file = fopen(filename, "r");
            if (file) {
                fclose(file);
                result = 1;
            }
        }
#endif
    }
    METHOD_END(ildjitSystem, "Platform.PortMethods.IsAccessible");

    return result;
}

JITNINT Platform_PortMethods_Open (JITINT32 type, JITINT32 portNumber, void* parameters) {
    JITSerial *serial;
    int isvalid = 0;
    JITINT8 name[64];
    TypeDescriptor *ports_methods, *parameters_type;
    FieldDescriptor* field;
    SerialParameters* parameters_sp;
    JITINT32 field_offset;

    METHOD_BEGIN(ildjitSystem, "Platform.PortMethods.Open");
    /* Bail out if the serial port specification is not valid */
    if (!((type != JIT_SERIAL_REGULAR && type != JIT_SERIAL_INFRARED &&
            type != JIT_SERIAL_USB && type != JIT_SERIAL_RFCOMM) ||
            (portNumber < 1 || portNumber > 256))) {
        (ildjitSystem->cliManager).CLR.netManager.GetSerialName(name, type, portNumber);
#ifdef HAVE_ACCESS
        isvalid = (access((char *)name, 0) >= 0);
#else
        {
            FILE *file = fopen(filename, "r");
            if (file) {
                fclose(file);
                isvalid = 1;
            }
        }
#endif
    }

    if (!isvalid) {
        METHOD_END(ildjitSystem, "Platform.PortMethods.Open");
        return 0;
    }

    /* Allocate space for the control structure and initialize it */
    if ((serial = (JITSerial *) malloc(sizeof(JITSerial))) == 0) {
        METHOD_END(ildjitSystem, "Platform.PortMethods.Open");
        return 0;
    }

    //parameters_type = (ildjitSystem->cliManager).metadataManager.getTypeDescriptorFromName(&((ildjitSystem->cliManager).metadataManager), (JITINT8 *) "Platform.PortMethods", (JITINT8 *) "Parameters");
    //parameters_type = (ildjitSystem->cliManager).metadataManager.getTypeDescriptorFromName(&((ildjitSystem->cliManager).metadataManager), (JITINT8 *) "Platform", (JITINT8 *) "PortMethods");
    ports_methods = (ildjitSystem->cliManager).metadataManager->getTypeDescriptorFromName((ildjitSystem->cliManager).metadataManager, (JITINT8 *) "Platform", (JITINT8 *) "PortMethods");

    parameters_type = ports_methods->getNestedTypeFromName(ports_methods, (JITINT8 *)"Parameters");

    if (!parameters_type) {
        METHOD_END(ildjitSystem, "Platform.PortMethods.Open");
        return 0;
    }

    field = parameters_type->getFieldFromName(parameters_type, (JITINT8 *) "baudRate");
    field_offset = ildjitSystem->garbage_collectors.gc.fetchFieldOffset(field);
    parameters_sp = (SerialParameters*) (parameters + field_offset);

    serial->fd = -1;
    serial->readTimeout = parameters_sp->readTimeout;
    serial->writeTimeout = parameters_sp->writeTimeout;

    /* Attempt to open the designated serial port */
    //GetSerialName(name, type, portNumber);
    if ((serial->fd = open((char *)name, O_RDWR | O_NONBLOCK, 0)) == -1) {
        free(serial);
        METHOD_END(ildjitSystem, "Platform.PortMethods.Open");
        return 0;
    }

    /* Set the initial serial port parameters */
    (ildjitSystem->cliManager).CLR.netManager.modifySerial(serial, parameters_sp);

    METHOD_END(ildjitSystem, "Platform.PortMethods.Open");
    /* The serial port is ready to go */
    return (JITNINT)serial;
}

void Platform_PortMethods_Close (JITNINT handle) {
    JITSerial       *ser;

    METHOD_BEGIN(ildjitSystem, "Platform.PortMethods.Close");
    ser = (JITSerial*) handle;
    close(ser->fd);
    free(ser);
    METHOD_END(ildjitSystem, "Platform.PortMethods.Close");

    return;
}

void Platform_PortMethods_Modify (JITNINT handle, void *parameters) {
    JITSerial       *ser;
    TypeDescriptor *parameters_type, *ports_methods;
    FieldDescriptor* field;
    SerialParameters* parameters_sp;
    JITINT32 field_offset;

    METHOD_BEGIN(ildjitSystem, "Platform.PortMethods.Modify");

    //parameters_type = (ildjitSystem->cliManager).metadataManager.getTypeDescriptorFromName(&((ildjitSystem->cliManager).metadataManager), (JITINT8 *) "Platform.PortMethods", (JITINT8 *) "Parameters");
    ports_methods = (ildjitSystem->cliManager).metadataManager->getTypeDescriptorFromName((ildjitSystem->cliManager).metadataManager, (JITINT8 *) "Platform", (JITINT8 *) "PortMethods");
    parameters_type = ports_methods->getNestedTypeFromName(ports_methods, (JITINT8 *)"Parameters");

    if (!parameters_type) {
        METHOD_END(ildjitSystem, "Platform.PortMethods.Modify");
        return;
    }

    field = parameters_type->getFieldFromName(parameters_type, (JITINT8 *) "baudRate");
    field_offset = ildjitSystem->garbage_collectors.gc.fetchFieldOffset(field);
    parameters_sp = (SerialParameters*) (parameters + field_offset);

    ser = (JITSerial *) (JITNUINT)handle;

    (ildjitSystem->cliManager).CLR.netManager.modifySerial(ser, parameters_sp);

    METHOD_END(ildjitSystem, "Platform.PortMethods.Modify");

    return;
}

JITINT32 Platform_PortMethods_GetBytesToRead (JITNINT handle) {
    unsigned int value = 0;
    JITSerial       *ser;

    METHOD_BEGIN(ildjitSystem, "Platform.PortMethods.GetBytesToRead");
    ser = (JITSerial *) (JITNUINT)handle;
#ifdef TIOCINQ
    ioctl(ser->fd, TIOCINQ, &value);
    METHOD_END(ildjitSystem, "Platform.PortMethods.GetBytesToRead");
    return (JITINT32) value;
#else
    METHOD_END(ildjitSystem, "Platform.PortMethods.GetBytesToRead");
    return 0;
#endif
}

JITINT32 Platform_PortMethods_GetBytesToWrite (JITNINT handle) {
    unsigned int value = 0;
    JITSerial       *ser;

    METHOD_BEGIN(ildjitSystem, "Platform.PortMethods.GetBytesToWrite");
    ser = (JITSerial *) (JITNUINT)handle;
#ifdef TIOCOUTQ
    ioctl(ser->fd, TIOCOUTQ, &value);
    METHOD_END(ildjitSystem, "Platform.PortMethods.GetBytesToWrite");
    return (JITINT32) value;
#else
    METHOD_END(ildjitSystem, "Platform.PortMethods.GetBytesToWrite");
    return 0;
#endif

}

JITINT32 Platform_PortMethods_ReadPins (JITNINT handle) {
    unsigned int value = 0;
    JITSerial       *ser;
    JITINT32 pins = 0;

    METHOD_BEGIN(ildjitSystem, "Platform.PortMethods.ReadPins");
    ser = (JITSerial *) (JITNUINT)handle;
#ifdef TIOCMGET
    ioctl(ser->fd, TIOCMGET, &value);
#ifdef TIOCM_CAR
    if ((value & TIOCM_CAR) != 0) {
        pins |= JIT_PIN_CD;
    }
#endif
#ifdef TIOCM_CTS
    if ((value & TIOCM_CTS) != 0) {
        pins |= JIT_PIN_CTS;
    }
#endif
#ifdef TIOCM_DSR
    if ((value & TIOCM_DSR) != 0) {
        pins |= JIT_PIN_DSR;
    }
#endif
#ifdef TIOCM_DTR
    if ((value & TIOCM_DTR) != 0) {
        pins |= JIT_PIN_DTR;
    }
#endif
#ifdef TIOCM_RTS
    if ((value & TIOCM_RTS) != 0) {
        pins |= JIT_PIN_RTS;
    }
#endif
#ifdef TIOCM_RNG
    if ((value & TIOCM_RNG) != 0) {
        pins |= JIT_PIN_RING;
    }
#endif
#endif
    METHOD_END(ildjitSystem, "Platform.PortMethods.ReadPins");

    return pins;
}

void Platform_PortMethods_WritePins (JITNINT handle, JITINT32 mask, JITINT32 value) {
    JITSerial       *ser;

    METHOD_BEGIN(ildjitSystem, "Platform.PortMethods.WritePins");

    ser = (JITSerial *) (JITNUINT)handle;

    if ((mask & value & JIT_PIN_BREAK) != 0) {
        tcsendbreak(ser->fd, 0);
    }
#if defined(TIOCMBIS) && defined(TIOCMBIC)
    if ((mask & (JIT_PIN_DTR | JIT_PIN_RTS)) != 0) {
        unsigned int current = 0;
        unsigned int setFlags = 0;
        unsigned int resetFlags = 0;
        ioctl(ser->fd, TIOCMGET, &current);
        if ((mask & JIT_PIN_DTR) != 0) {
            if ((value & JIT_PIN_DTR) != 0) {
                if ((current & TIOCM_DTR) == 0) {
                    setFlags |= TIOCM_DTR;
                }
            } else {
                if ((current & TIOCM_DTR) != 0) {
                    resetFlags |= TIOCM_DTR;
                }
            }
        }
        if ((mask & JIT_PIN_RTS) != 0) {
            if ((value & JIT_PIN_RTS) != 0) {
                if ((current & TIOCM_RTS) == 0) {
                    setFlags |= TIOCM_RTS;
                }
            } else {
                if ((current & TIOCM_RTS) != 0) {
                    resetFlags |= TIOCM_RTS;
                }
            }
        }
        if (setFlags != 0) {
            ioctl(ser->fd, TIOCMBIS, &setFlags);
        }
        if (resetFlags != 0) {
            ioctl(ser->fd, TIOCMBIC, &resetFlags);
        }
    }
#endif

    METHOD_END(ildjitSystem, "Platform.PortMethods.WritePins");
}

void Platform_PortMethods_GetRecommendedBufferSizes (JITINT32 *read_buffer_size, JITINT32 *write_buffer_size,JITINT32 *received_bytes_threshold) {

    METHOD_BEGIN(ildjitSystem, "Platform.PortMethods.GetRecommendedBufferSizes");
    *read_buffer_size = 1024;
    *write_buffer_size = 1024;
    *received_bytes_threshold = 768;
    METHOD_END(ildjitSystem, "Platform.PortMethods.GetRecommendedBufferSizes");

    return;
}

void Platform_PortMethods_DiscardInBuffer (JITNINT handle) {
    JITSerial       *ser;

    METHOD_BEGIN(ildjitSystem, "Platform.PortMethods.DiscardInBuffer");

    ser = (JITSerial *) (JITNUINT)handle;
    tcflush(ser->fd, TCIFLUSH);

    METHOD_END(ildjitSystem, "Platform.PortMethods.DiscardInBuffer");

    return;

}

void Platform_PortMethods_DiscardOutBuffer (JITNINT handle) {
    JITSerial       *ser;

    METHOD_BEGIN(ildjitSystem, "Platform.PortMethods.DiscardOutBuffer");

    ser = (JITSerial *) handle;
    tcflush(ser->fd, TCOFLUSH);

    METHOD_END(ildjitSystem, "Platform.PortMethods.DiscardOutBuffer");

    return;
}

void Platform_PortMethods_DrainOutBuffer (JITNINT handle) {
    JITSerial       *ser;

    METHOD_BEGIN(ildjitSystem, "Platform.PortMethods.DrainOutBuffer");

    ser = (JITSerial *) handle;
    tcdrain(ser->fd);

    METHOD_END(ildjitSystem, "Platform.PortMethods.DrainOutBuffer");

    return;
}

JITINT32 Platform_PortMethods_Read (JITNINT handle, void* buffer, JITINT32 offset, JITINT32 count) {
    void* buf;
    JITSerial       *ser;
    JITINT32 result;
    struct timeval tv;
    struct timeval *seltv = NULL;
    fd_set readSet;

    METHOD_BEGIN(ildjitSystem, "Platform.PortMethods.Read");

    ser = (JITSerial *) handle;
    buf = (ildjitSystem->cliManager).CLR.arrayManager.getArrayElement(buffer, 0);

    if ( ser->readTimeout > 0 ) {
        // convert milliseconds to seconds and left over microseconds
        tv.tv_sec = ser->readTimeout / 1000;
        tv.tv_usec = (ser->readTimeout % 1000) * 1000;
        seltv = &tv;
    }
    // else timeout is infinite if <= 0

    FD_ZERO(&readSet);
    FD_SET(ser->fd, &readSet);
    if (select(ser->fd + 1, &readSet, (fd_set *) 0, (fd_set *) 0,
               seltv) != 1) {
        METHOD_END(ildjitSystem, "Platform.PortMethods.Read");
        return 0;
    }

    result = (JITINT32) (read(ser->fd, buf, (int) count));

    METHOD_END(ildjitSystem, "Platform.PortMethods.Read");

    return result;
}

void Platform_PortMethods_Write (JITNINT handle, void* buffer, JITINT32 offset, JITINT32 count) {
    //NO TIMEOUT
    void* buf;
    JITSerial       *ser;

    METHOD_BEGIN(ildjitSystem, "Platform.PortMethods.Write");

    ser = (JITSerial *) handle;
    buf = (ildjitSystem->cliManager).CLR.arrayManager.getArrayElement(buffer, offset);

    write(ser->fd, buf, (int) count);

    METHOD_END(ildjitSystem, "Platform.PortMethods.Write");

    return;
}

JITINT32 Platform_PortMethods_WaitForPinChange (JITNINT handle) {
    JITSerial       *ser;
    unsigned int waitFor = 0;

    METHOD_BEGIN(ildjitSystem, "Platform.PortMethods.WaitForPinChange");
    ser = (JITSerial *) handle;
#ifdef TIOCMIWAIT
#ifdef TIOCM_DSR
    waitFor |= TIOCM_DSR;
#endif
#ifdef TIOCM_CAR
    waitFor |= TIOCM_CAR;
#endif
#ifdef TIOCM_RTS
    waitFor |= TIOCM_RTS;
#endif
#ifdef TIOCM_RNG
    waitFor |= TIOCM_RNG;
#endif
    if (ioctl(ser->fd, TIOCMIWAIT, &waitFor) == 0) {
        METHOD_END(ildjitSystem, "Platform.PortMethods.WaitForPinChange");
        return 1;
    }

    METHOD_END(ildjitSystem, "Platform.PortMethods.WaitForPinChange");
    return 0;
#else
    METHOD_END(ildjitSystem, "Platform.PortMethods.WaitForPinChange");
    return -1;
#endif

}

JITINT32 Platform_PortMethods_WaitForInput (JITNINT handle, JITINT32 timeout) {
    JITSerial       *ser;
    struct timeval tv;
    struct timeval *seltv = NULL;
    fd_set readSet;

    METHOD_BEGIN(ildjitSystem, "Platform.PortMethods.WaitForInput");
    ser = (JITSerial *) handle;

    if ( timeout > 0 ) {
        // convert milliseconds to seconds and left over microseconds
        tv.tv_sec = timeout / 1000;
        tv.tv_usec = (timeout % 1000) * 1000;
        seltv = &tv;
    }
    // else timeout is infinite if <= 0

    FD_ZERO(&readSet);
    FD_SET(ser->fd, &readSet);
    if (select(ser->fd + 1, &readSet, (fd_set *) 0, (fd_set *) 0,
               seltv) == 1) {
        METHOD_END(ildjitSystem, "Platform.PortMethods.WaitForInput");
        return 1;
    }

    METHOD_END(ildjitSystem, "Platform.PortMethods.WaitForInput");

    return 0;
}

void Platform_PortMethods_Interrupt (void* thread) {

    METHOD_BEGIN(ildjitSystem, "Platform.PortMethods.Interrupt");
    METHOD_END(ildjitSystem, "Platform.PortMethods.Interrupt");

    return;
}
