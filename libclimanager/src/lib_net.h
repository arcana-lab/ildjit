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
#ifndef LIB_NET_H
#define LIB_NET_H

#include <jitsystem.h>
#include <metadata/metadata_types.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <config.h>
/* read&delete this note:
   #include <netinet/in.h>
   #include <netdb.h>
   #include <sys/ioctl.h>
   #include <termios.h>
   moved to platform_api.h as platform-specific headers. */
#include <platform_API.h>
/*
   #define HAVE_SETSOCKOPT 1
   #define HAVE_GETSOCKOPT 1
 #define HAVE_IOCTL 1*/
#define INVALID_HANDLE (JITNINT) (-1)
#ifdef IN6ADDR_ANY_INIT
#define JIT_IPV6_PRESENT                1
#endif
#ifdef HAVE_LINUX_IRDA_H
#define JIT_IRDA_PRESENT                1
#include <linux/irda.h>
#endif
/*
 * C# Address Families
 */
#define JIT_AF_INET                     2
#define JIT_AF_INET6                    23
#define JIT_AF_IRDA                     26
/*
 * C# Socket types
 */
#define JIT_SOCK_UNKNOWN                 -1
#define JIT_SOCK_STREAM                   1
#define JIT_SOCK_DGRAM                    2
#define JIT_SOCK_RAW                      3
#define JIT_SOCK_RDM                      4
#define JIT_SOCK_SEQPACKET                5
/*
 * C# Socket option levels
 */
#define JIT_SOL_IP                      0
#define JIT_SOL_TCP                     6
#define JIT_SOL_UDP                     17
#define JIT_SOL_SOCKET          65535
/*
 *  C# Socket option names
 */
#define JIT_SO_ADD_MEMBERSHIP   12              /* IP options */
#define JIT_SO_DROP_MEMBERSHIP  13
#define JIT_SO_NO_DELAY                 1       /* TCP options */
#define JIT_SO_EXPEDITED                        2
#define JIT_SO_NO_CHECKSUM              1       /* UDP options */
#define JIT_SO_CHKSUM_COVERAGE  20
#define JIT_SO_REUSE_ADDRESS            0x0004  /* Socket options */
#define JIT_SO_KEEP_ALIVE               0x0008
#define JIT_SO_SEND_BUFFER              0x1001
#define JIT_SO_RECV_BUFFER              0x1002
#define JIT_SO_SEND_TIMEOUT             0x1005
#define JIT_SO_RECV_TIMEOUT             0x1006
#define JIT_SO_BROADCAST 32

//C# Multicast options
#define JIT_MULTICAST_IF 0x09
#define JIT_MULTICAST_TTL 0x0A
#define JIT_MULTICAST_LOOP 0x0B
#define JIT_MULTICAST_ADD 0x0C
#define JIT_MULTICAST_DROP 0x0D

/*
 * Serial port types.
 */
#define JIT_SERIAL_REGULAR              0
#define JIT_SERIAL_INFRARED             1
#define JIT_SERIAL_USB                  2
#define JIT_SERIAL_RFCOMM               3

/*
 * Bits for various serial pins.
 */
#define JIT_PIN_BREAK                   (1<<0)
#define JIT_PIN_CD                              (1<<1)
#define JIT_PIN_CTS                             (1<<2)
#define JIT_PIN_DSR                             (1<<3)
#define JIT_PIN_DTR                             (1<<4)
#define JIT_PIN_RTS                             (1<<5)
#define JIT_PIN_RING                            (1<<6)

/*
 * Parity values.
 */
#define JIT_PARITY_NONE                 0
#define JIT_PARITY_ODD                  1
#define JIT_PARITY_EVEN                 2
#define JIT_PARITY_MARK                 3
#define JIT_PARITY_SPACE                4

/*
 * Serial port handshake modes.
 */
#define JIT_HANDSHAKE_NONE              0
#define JIT_HANDSHAKE_XONOFF            1
#define JIT_HANDSHAKE_RTS               2
#define JIT_HANDSHAKE_RTS_XONOFF        3

#define EPOCH_ADJUST    ((JITINT64) 62135596800LL)

typedef union {
    struct sockaddr addr;
    struct sockaddr_in ipv4_addr;
#ifdef JIT_IPV6_PRESENT
    struct sockaddr_in6 ipv6_addr;
#endif
#ifdef JIT_IRDA_PRESENT
    struct sockaddr_irda irda_addr;
#endif

} SockAddr;

typedef union {
    struct ip_mreq ipv4_req;
#ifdef JIT_IPV6_PRESENT
    struct ipv6_mreq ipv6_req;
#endif
} MreqIp;

typedef struct {
    JITINT32 baudRate;
    JITINT32 parity;
    JITINT32 dataBits;
    JITINT32 stopBits;
    JITINT32 handshake;
    JITINT8 parityReplace;
    JITINT32 discardNull;
    JITINT32 readBufferSize;
    JITINT32 writeBufferSize;
    JITINT32 receivedBytesThreshold;
    JITINT32 readTimeout;
    JITINT32 writeTimeout;

} SerialParameters;

//TEMPORANEA????????????
typedef struct tagILSerial {
    int fd;
    JITINT32 readTimeout;
    JITINT32 writeTimeout;

} JITSerial;

/**
 *
 *
 */
typedef struct {

    JITBOOLEAN (*toIPHostEntry)(struct hostent* h_ent, void** h_name, void** h_aliases, void** h_addr_list);
    JITBOOLEAN (*convertAddressToSockAddr)(void* addr, SockAddr* sock_addr, JITINT32* addrlen);
    JITBOOLEAN (*convertSockAddrToAddress)(void* addr, SockAddr* sock_addr);
    JITINT32 (*convertSocketOptsToNative)(JITINT32 level, JITINT32 name, JITINT32 *nativeLevel, JITINT32 *nativeName);
    void (*GetSerialName)(JITINT8 *name, JITINT32 type, JITINT32 portNumber);
    void (*modifySerial)(JITSerial *handle, SerialParameters *parameters);
    void (*initialize)(void);

} t_netManager;

void init_netManager (t_netManager *netManager);

#endif
