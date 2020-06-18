/*
 * Copyright (C) 2008  Simone Campanoni
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
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <jitsystem.h>
#include <compiler_memory_manager.h>

// My headers
#include <lib_net.h>
#include <climanager_system.h>
#include <layout_manager.h>
// End

extern CLIManager_t *cliManager;

static inline JITBOOLEAN internal_toIPHostEntry (struct hostent* h_ent, void** h_name, void** h_aliases, void** h_addr_list);
static inline JITBOOLEAN internal_convertAddressToSockAddr (void* addr, SockAddr* sock_addr, int* addrlen);
static inline JITBOOLEAN internal_convertSockAddrToAddress (void* addr, SockAddr* sock_addr);
static inline JITINT32   internal_convertSocketOptsToNative (JITINT32 level, JITINT32 name, JITINT32 *nativeLevel, JITINT32 *nativeName);
static inline void       internal_GetSerialName (JITINT8 *name, JITINT32 type, JITINT32 portNumber);
static inline void       internal_modifySerial (JITSerial* handle, SerialParameters* parameters);
static inline void internal_lib_net_initialize (void);

//pthread_once_t netMetadataLock = PTHREAD_ONCE_INIT;

void init_netManager (t_netManager *netManager) {

    /* Initialize the functions			*/
    netManager->toIPHostEntry = internal_toIPHostEntry;
    netManager->convertAddressToSockAddr = internal_convertAddressToSockAddr;
    netManager->convertSockAddrToAddress = internal_convertSockAddrToAddress;
    netManager->convertSocketOptsToNative = internal_convertSocketOptsToNative;
    netManager->GetSerialName = internal_GetSerialName;
    netManager->modifySerial = internal_modifySerial;
    netManager->initialize = internal_lib_net_initialize;


    /* Return					*/
    return;
}

static inline JITBOOLEAN internal_toIPHostEntry (struct hostent* h_ent, void** h_name, void** h_aliases, void** h_addr_list) {
    JITINT32 length;
    XanList         *list = NULL;
    TypeDescriptor  *int64Type;

    *h_name = CLIMANAGER_StringManager_newInstanceFromUTF8((JITINT8*) h_ent->h_name, strlen(h_ent->h_name));

    length = -1;
    list = xanList_new(malloc, free, NULL);
    assert(list != NULL);
    while (h_ent->h_aliases[++length] != NULL) { //Adding aliases to a list
        void* temp_string = CLIMANAGER_StringManager_newInstanceFromUTF8((JITINT8*) h_ent->h_aliases[length], strlen(h_ent->h_aliases[length]));
        assert(temp_string != NULL);
        xanList_append(list, temp_string);
    }

    *h_aliases = (cliManager->CLR).arrayManager.newInstanceFromList(list, (cliManager->CLR).stringManager.StringType);
    if (!(*h_aliases)) { //The string is null or mem error
        return 0;
    }

    xanList_destroyList(list);

    length = -1;
    int64Type = cliManager->metadataManager->getTypeDescriptorFromElementType(cliManager->metadataManager,ELEMENT_TYPE_I8);
    while (h_ent->h_addr_list[++length] != NULL) {
        ;                                    //Counting addresses
    }
    *h_addr_list = (cliManager->CLR).arrayManager.newInstanceFromType(int64Type, length);
    if (!(*h_addr_list)) { //The string is null or mem error
        return 0;
    }

    while (length--) {              //Populating the array
        JITINT64 *element = allocMemory(sizeof(JITINT64));
        if (!(element)) {       //Mem error
            return 0;
        }
        *element = (JITINT64) *((JITINT32*) h_ent->h_addr_list[length]) & 0x00000000FFFFFFFFLL;
        (cliManager->CLR).arrayManager.setValueArrayElement(*h_addr_list, length, element);
    }
    /* Return			*/

    return 1;
}

static inline JITBOOLEAN internal_convertAddressToSockAddr (void* addr, SockAddr* sock_addr, int* addrlen) {
    int af, port, value, len;
    JITUINT8* buf;

    /* Clear the result */
    memset(sock_addr, 0, sizeof(SockAddr));

    len = (cliManager->CLR).arrayManager.getArrayLength(addr, 0);
    buf = (cliManager->CLR).arrayManager.getArrayElement(addr, 0);
    /* Get the address family, which is stored in little-endian order */
    if (len < 2) {
        return 0;
    }
    af = ((((int) (buf[1])) << 8) | ((int) (buf[0])));

    /* Determine how to convert the buffer based on the address family */
    if (af == JIT_AF_INET) {
        if (len < 8) {
            return 0;
        }
        sock_addr->ipv4_addr.sin_family = AF_INET;
        port = ((((int) (buf[2])) << 8) | ((int) (buf[3])));
        sock_addr->ipv4_addr.sin_port = htons((unsigned short) port);
        value = ((((long) (buf[4])) << 24) |
                 (((long) (buf[5])) << 16) |
                 (((long) (buf[6])) << 8) |
                 ((long) (buf[7])));
        sock_addr->ipv4_addr.sin_addr.s_addr = htonl((long) value);
        *addrlen = sizeof(struct sockaddr_in);
        return 1;
    }
#ifdef JIT_IPV6_PRESENT
    else if (af == JIT_AF_INET6) {
        if (len < 28) {
            return 0;
        }
        sock_addr->ipv6_addr.sin6_family = AF_INET6;
        port = ((((int) (buf[2])) << 8) | ((int) (buf[3])));
        sock_addr->ipv6_addr.sin6_port = htons((unsigned short) port);
        value = ((((long) (buf[4])) << 24) |
                 (((long) (buf[5])) << 16) |
                 (((long) (buf[6])) << 8) |
                 ((long) (buf[7])));
        sock_addr->ipv6_addr.sin6_flowinfo = htonl((long) value);
        memcpy(&(sock_addr->ipv6_addr.sin6_addr), buf + 8, 16);
        value = ((((long) (buf[24])) << 24) |
                 (((long) (buf[25])) << 16) |
                 (((long) (buf[26])) << 8) |
                 ((long) (buf[27])));
        *addrlen = sizeof(struct sockaddr_in6);
        return 1;
    }
#endif
#ifdef JIT_IRDA_PRESENT
    else if (af == JIT_AF_IRDA) {
        if (len < 32) {
            return 0;
        }
        sock_addr->irda_addr.sir_family = AF_IRDA;
        sock_addr->irda_addr.sir_lsap_sel = LSAP_ANY;
        value = ((((long) (buf[2])) << 24) |
                 (((long) (buf[3])) << 16) |
                 (((long) (buf[4])) << 8) |
                 ((long) (buf[5])));
        sock_addr->irda_addr.sir_addr = htonl((long) value);
        memcpy(sock_addr->irda_addr.sir_name, buf + 6, 24);
        *addrlen = sizeof(struct sockaddr_irda);
        return 1;
    }
#endif

    /* If we get here, then the address cannot be converted */
    return 0;
}

static inline JITBOOLEAN internal_convertSockAddrToAddress (void* addr, SockAddr* sock_addr) {
    int af, port, value, len;
    JITUINT8* buf;

    /* Clear the result */

    len = (cliManager->CLR).arrayManager.getArrayLength(addr, 0);
    buf = (cliManager->CLR).arrayManager.getArrayElement(addr, 0);

    /* Clear the result */
    memset(buf, 0, sizeof(JITUINT8));

    /* Map and store the address family, in little-endian order */
    if (len < 2) {
        return 0;
    }
    af = sock_addr->addr.sa_family;
    if (af == AF_INET) {
        af = JIT_AF_INET;
    }
#ifdef JIT_IPV6_PRESENT
    else if (af == AF_INET6) {
        af = JIT_AF_INET6;
    }
#endif
#ifdef JIT_IRDA_PRESENT
    else if (af == AF_IRDA) {
        af = JIT_AF_IRDA;
    }
#endif
    buf[0] = (unsigned char) af;
    buf[1] = (unsigned char) (af >> 8);

    /* Determine how to convert the address based on the address family */
    if (af == JIT_AF_INET) {
        if (len < 8) {
            return 0;
        }
        port = (int) (ntohs(sock_addr->ipv4_addr.sin_port));
        buf[2] = (unsigned char) (port >> 8);
        buf[3] = (unsigned char) port;
        value = (long) (ntohl(sock_addr->ipv4_addr.sin_addr.s_addr));
        buf[4] = (unsigned char) (value >> 24);
        buf[5] = (unsigned char) (value >> 16);
        buf[6] = (unsigned char) (value >> 8);
        buf[7] = (unsigned char) value;
        return 1;
    }
#ifdef JIT_IPV6_PRESENT
    else if (af == JIT_AF_INET6) {
        if (len < 28) {
            return 0;
        }
        port = (int) (ntohs(sock_addr->ipv6_addr.sin6_port));
        buf[2] = (unsigned char) (port >> 8);
        buf[3] = (unsigned char) port;
        value = (long) (ntohl(sock_addr->ipv6_addr.sin6_flowinfo));
        buf[4] = (unsigned char) (value >> 24);
        buf[5] = (unsigned char) (value >> 16);
        buf[6] = (unsigned char) (value >> 8);
        buf[7] = (unsigned char) value;
        memcpy(buf + 8, &(sock_addr->ipv6_addr.sin6_addr), 16);
#if HAVE_SIN6_SCOPE_ID
        value = (long) (ntohl(sock_addr->ipv6_addr.sin6_scope_id));
#else
        value = 0;
#endif
        buf[24] = (unsigned char) (value >> 24);
        buf[25] = (unsigned char) (value >> 16);
        buf[26] = (unsigned char) (value >> 8);
        buf[27] = (unsigned char) value;
        return 1;
    }
#endif
#ifdef JIT_IRDA_PRESENT
    else if (af == JIT_AF_IRDA) {
        if (len < 32) {
            return 0;
        }
        value = (long) (ntohl(sock_addr->irda_addr.sir_addr));
        buf[2] = (unsigned char) (value >> 24);
        buf[3] = (unsigned char) (value >> 16);
        buf[4] = (unsigned char) (value >> 8);
        buf[5] = (unsigned char) value;
        memcpy(buf + 6, sock_addr->irda_addr.sir_name, 24);
        return 1;
    }
#endif

    /* If we get here, then the address cannot be converted */
    return 0;
}

static inline JITINT32  internal_convertSocketOptsToNative (JITINT32 level, JITINT32 name, JITINT32 *nativeLevel, JITINT32 *nativeName) {
    switch (level) {
        case (JIT_SOL_IP): {
#ifdef SOL_IP
            (*nativeLevel) = SOL_IP;
            switch (name) {
                case JIT_SO_ADD_MEMBERSHIP: {
#ifdef SO_ATTACH_FILTER
                    (*nativeName) = SO_ATTACH_FILTER;
#else
                    return 0;
#endif
                }
                break;

                case JIT_SO_DROP_MEMBERSHIP: {
#ifdef SO_DETACH_FILTER
                    (*nativeName) = SO_DETACH_FILTER;
#else
                    return 0;
#endif
                }
                break;

                default:
                    return 0;
            }
#else
            return 0;
#endif
        }
        break;

        case (JIT_SOL_TCP): {
#ifdef SOL_TCP
            (*nativeLevel) = SOL_TCP;
            switch (name) {
                case JIT_SO_NO_DELAY: {
#ifdef TCP_NODELAY
                    (*nativeName) = TCP_NODELAY;
#else
                    return 0;
#endif
                }
                break;

                case JIT_SO_EXPEDITED: {
                    /* TODO */
                    return 0;
                }
                break;
                default:
                    return 0;
            }
#else
            print_err("SOL_TCP not available. ", 0);
            abort();
#endif
        }
        break;

        case (JIT_SOL_UDP): {
#ifdef SOL_UDP
            (*nativeLevel) = SOL_UDP;
            /* TODO */
            return 0;
#else
            print_err("SOL_UDP not available. ", 0);
            abort();
#endif
        }
        break;

        case (JIT_SOL_SOCKET): {
#ifdef SOL_SOCKET
            (*nativeLevel) = SOL_SOCKET;
            switch (name) {
                case JIT_SO_REUSE_ADDRESS: {
#ifdef SO_REUSEADDR
                    (*nativeName) = SO_REUSEADDR;
#else
                    return 0;
#endif
                }
                break;

                case JIT_SO_KEEP_ALIVE: {
#ifdef SO_KEEPALIVE
                    (*nativeName) = SO_KEEPALIVE;
#else
                    return 0;
#endif
                }
                break;

                case JIT_SO_SEND_BUFFER: {
#ifdef SO_SNDBUF
                    (*nativeName) = SO_SNDBUF;
#else
                    return 0;
#endif
                }
                break;

                case JIT_SO_RECV_BUFFER: {
#ifdef SO_RCVBUF
                    (*nativeName) = SO_RCVBUF;
#else
                    return 0;
#endif
                }
                break;

                case JIT_SO_SEND_TIMEOUT: {
#ifdef SO_SNDTIMEO
                    (*nativeName) = SO_SNDTIMEO;
#else
                    return 0;
#endif
                }
                break;

                case JIT_SO_RECV_TIMEOUT: {
#ifdef SO_RCVTIMEO
                    (*nativeName) = SO_RCVTIMEO;
#else
                    return 0;
#endif
                }
                break;

                case JIT_SO_BROADCAST: {
#ifdef SO_BROADCAST
                    (*nativeName) = SO_BROADCAST;
#else
                    return 0;
#endif
                }
                break;

                default:
                    return 0;
            }
#else
#warning "SOL_SOCKET is not defined"
            return 0;
#endif
        }
        break;

        default:
            return 0;
    }
    return 1;
}

static inline void       internal_GetSerialName (JITINT8 *name, JITINT32 type, JITINT32 portNumber) {
    if (type == JIT_SERIAL_REGULAR) {
        sprintf((char*) name, "/dev/ttyS%d", (int) (portNumber - 1));
    } else if (type == JIT_SERIAL_INFRARED) {
        sprintf((char*) name, "/dev/ircomm%d", (int) (portNumber - 1));
    } else if (type == JIT_SERIAL_RFCOMM) {
        sprintf((char*) name, "/dev/rfcomm%d", (int) (portNumber - 1));
    } else {
        sprintf((char*) name, "/dev/ttyUSB%d", (int) (portNumber - 1));
    }
}

static inline void       internal_modifySerial (JITSerial* handle, SerialParameters* parameters) {
    struct termios current;
    struct termios newValues;
    speed_t speed;

    /* Record the timeout values in the control structure */
    handle->readTimeout = parameters->readTimeout;
    handle->writeTimeout = parameters->writeTimeout;

    /* Get the current values */
    if (tcgetattr(handle->fd, &current) < 0) {
        return;
    }

    /* Modify the values to reflect the new state */
    newValues = current;
    switch (parameters->baudRate) {
#ifdef B50
        case 50:
            speed = B50;
            break;
#endif
#ifdef B75
        case 75:
            speed = B75;
            break;
#endif
#ifdef B110
        case 110:
            speed = B110;
            break;
#endif
#ifdef B134
        case 134:
            speed = B134;
            break;
#endif
#ifdef B150
        case 150:
            speed = B150;
            break;
#endif
#ifdef B200
        case 200:
            speed = B200;
            break;
#endif
#ifdef B300
        case 300:
            speed = B300;
            break;
#endif
#ifdef B600
        case 600:
            speed = B600;
            break;
#endif
#ifdef B1200
        case 1200:
            speed = B1200;
            break;
#endif
#ifdef B1800
        case 1800:
            speed = B1800;
            break;
#endif
#ifdef B2400
        case 2400:
            speed = B2400;
            break;
#endif
#ifdef B4800
        case 4800:
            speed = B4800;
            break;
#endif
#ifdef B19200
        case 319200:
            speed = B19200;
            break;
#endif
#ifdef B38400
        case 38400:
            speed = B38400;
            break;
#endif
#ifdef B57600
        case 57600:
            speed = B57600;
            break;
#endif
#ifdef B115200
        case 115200:
            speed = B115200;
            break;
#endif
#ifdef B230400
        case 230400:
            speed = B230400;
            break;
#endif
#ifdef B460800
        case 460800:
            speed = B460800;
            break;
#endif
#ifdef B500000
        case 500000:
            speed = B500000;
            break;
#endif
#ifdef B576000
        case 576000:
            speed = B576000;
            break;
#endif
#ifdef B921600
        case 921600:
            speed = B921600;
            break;
#endif
#ifdef B1000000
        case 1000000:
            speed = B1000000;
            break;
#endif
#ifdef B1152000
        case 1152000:
            speed = B1152000;
            break;
#endif
#ifdef B1500000
        case 1500000:
            speed = B1500000;
            break;
#endif
#ifdef B2000000
        case 2000000:
            speed = B2000000;
            break;
#endif
#ifdef B2500000
        case 2500000:
            speed = B2500000;
            break;
#endif
#ifdef B3000000
        case 3000000:
            speed = B3000000;
            break;
#endif
#ifdef B3500000
        case 3500000:
            speed = B3500000;
            break;
#endif
#ifdef B4000000
        case 4000000:
            speed = B4000000;
            break;
#endif
        default: {
#if defined(B115200)
            speed = B115200;
#elif defined(B38400)
            speed = B38400;
#else
            speed = B9600;
#endif
        }
        break;
    }
    cfsetospeed(&newValues, speed);
    cfsetispeed(&newValues, speed);
    switch (parameters->parity) {
        case JIT_PARITY_NONE:
        case JIT_PARITY_MARK:
        case JIT_PARITY_SPACE: {
            newValues.c_cflag &= ~(PARENB | PARODD);
        }
        break;

        case JIT_PARITY_ODD: {
            newValues.c_cflag |= (PARENB | PARODD);
        }
        break;

        case JIT_PARITY_EVEN: {
            newValues.c_cflag |= PARENB;
            newValues.c_cflag &= ~(PARODD);
        }
        break;
    }
    newValues.c_cflag &= ~CSIZE;
    switch (parameters->dataBits) {
        case 5: {
            newValues.c_cflag |= CS5;
        }
        break;

        case 6: {
            newValues.c_cflag |= CS6;
        }
        break;

        case 7: {
            newValues.c_cflag |= CS7;
        }
        break;

        case 8: {
            newValues.c_cflag |= CS8;
        }
        break;
    }
    if (parameters->stopBits != 0) {
        newValues.c_cflag |= CSTOPB;
    } else {
        newValues.c_cflag &= ~(CSTOPB);
    }
    switch (parameters->handshake) {
        case JIT_HANDSHAKE_NONE: {
            newValues.c_iflag &= ~(IXON | IXOFF);
            newValues.c_cflag &= ~(CRTSCTS);
        }
        break;

        case JIT_HANDSHAKE_XONOFF: {
            newValues.c_iflag |= (IXON | IXOFF);
            newValues.c_cflag &= ~(CRTSCTS);
        }
        break;

        case JIT_HANDSHAKE_RTS: {
            newValues.c_iflag &= ~(IXON | IXOFF);
            newValues.c_cflag |= CRTSCTS;
        }
        break;

        case JIT_HANDSHAKE_RTS_XONOFF: {
            newValues.c_iflag |= (IXON | IXOFF);
            newValues.c_cflag |= CRTSCTS;
        }
        break;
    }
    newValues.c_iflag &= ~(IGNBRK | BRKINT | IGNPAR | PARMRK | ISTRIP | INLCR |
                           IGNCR | ICRNL | IUCLC | IMAXBEL);
    if (parameters->parityReplace == 0xFF) {
        newValues.c_iflag |= PARMRK;
    }
    newValues.c_oflag &= ~(OPOST | OLCUC | ONLCR | OCRNL | ONOCR |
                           ONLRET | OFILL | OFDEL);
    newValues.c_lflag &= ~(ISIG | ICANON | ECHO | ECHOE | ECHOK | ECHONL |
                           ECHOCTL | ECHOPRT | ECHOKE | IEXTEN);

    /* If the values have changed, then set them */
    if (current.c_iflag != newValues.c_iflag ||
            current.c_oflag != newValues.c_oflag ||
            current.c_cflag != newValues.c_cflag ||
            current.c_lflag != newValues.c_lflag) {
        tcsetattr(handle->fd, TCSANOW, &newValues);
    }
}

static inline void internal_lib_net_initialize (void) {
    return;
}
