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
#ifndef INTERNAL_CALLS_IO_H
#define INTERNAL_CALLS_IO_H

#include <jitsystem.h>
#include <lib_net.h>
#include <platform_API.h>

/**
 * File opening modes
 */
typedef enum {
    FileMode_CreateNew = 0x1,
    FileMode_Create = 0x2,
    FileMode_Open = 0x3,
    FileMode_OpenOrCreate = 0x4,
    FileMode_Truncate = 0x5,
    FileMode_Append = 0x6
} FileMode;

/**
 * File access modes
 */
typedef enum {
    FileAccess_Read = 0x1,
    FileAccess_Write = 0x2,
    FileAccess_ReadWrite = 0x3
} FileAccess;

/**
 * File sharing options
 */
typedef enum {
    FileShare_None = 0x0,
    FileShare_Read = 0x1,
    FileShare_Write = 0x2,
    FileShare_ReadWrite = 0x3,
    FileShare_Inheritable = 0x10
} FileShare;

/**
 * File opening options
 */
typedef enum {
    FileOptions_None = 0x0,
    FileOptions_Encrypted = 0x4000,
    FileOptions_DeleteOnClose = 0x4000000,
    FileOptions_SequentialScan = 0x8000000,
    FileOptions_RandomAccess = 0x10000000,
    FileOptions_Asynchronous = 0x40000000,
    FileOptions_WriteThrough = 0x80000000
} FileOptions;

/*
 * File Types
 */
#define ILFileType_Unknown                      0
#define ILFileType_FIFO                         1
#define ILFileType_CHR                          2
#define ILFileType_DIR                          4
#define ILFileType_BLK                          6
#define ILFileType_REG                          8
#define ILFileType_LNK                          10
#define ILFileType_SOCK                         12
#define ILFileType_WHT                          14

/*
 * File open modes.
 */
#define ILFileMode_CreateNew    1
#define ILFileMode_Create               2
#define ILFileMode_Open                 3
#define ILFileMode_OpenOrCreate 4
#define ILFileMode_Truncate             5
#define ILFileMode_Append               6



/*
 * File access modes.
 */
#define ILFileAccess_Read               0x01
#define ILFileAccess_Write              0x02
#define ILFileAccess_ReadWrite  (ILFileAccess_Read | ILFileAccess_Write)

/*
 * File share modes.
 */
#define ILFileShare_None                0x00
#define ILFileShare_Read                0x01
#define ILFileShare_Write               0x02
#define ILFileShare_ReadWrite   (ILFileShare_Read | ILFileAccess_Write)
#define ILFileShare_Inheritable 0x10

/*
 * File Attibutes.
 */
#define ILFileAttributes_ReadOnly               0x0001
#define ILFileAttributes_Hidden                 0x0002
#define ILFileAttributes_System                 0x0004
#define ILFileAttributes_Directory              0x0010
#define ILFileAttributes_Archive                0x0020
#define ILFileAttributes_Device                 0x0040
#define ILFileAttributes_Normal                 0x0080
#define ILFileAttributes_Temporary              0x0100
#define ILFileAttributes_SparseFile             0x0200
#define ILFileAttributes_ReparsePoint   0x0400
#define ILFileAttributes_Compressed             0x0800
#define ILFileAttributes_Offline                0x1000
#define ILFileAttributes_NotContentIndexed 0x2000
#define ILFileAttributes_Encrypted              0x4000

/*
 * errno value
 */
//FILEMETHODS

#define IL_ERRNO_Success                0               /* Operation succeeded */
#define IL_ERRNO_EPERM                  1               /* Operation not permitted */
#define IL_ERRNO_ENOENT                 2               /* No such file or directory */
#define IL_ERRNO_ENOFILE                IL_ERRNO_ENOENT /* No such file */
#define IL_ERRNO_ESRCH                  3               /* No such process */
#define IL_ERRNO_EINTR                  4               /* Interrupted system call */
#define IL_ERRNO_EIO                    5               /* I/O error */
#define IL_ERRNO_ENXIO                  6               /* No such device or address */
#define IL_ERRNO_E2BIG                  7               /* Arg list too long */
#define IL_ERRNO_ENOEXEC                8               /* Exec format error */
#define IL_ERRNO_EBADF                  9               /* Bad file number */
#define IL_ERRNO_ECHILD                 10              /* No child processes */
#define IL_ERRNO_EAGAIN                 11              /* Try again */
#define IL_ERRNO_ENOMEM                 12              /* Out of memory */
#define IL_ERRNO_EACCES                 13              /* Permission denied */
#define IL_ERRNO_EFAULT                 14              /* Bad address */
#define IL_ERRNO_ENOTBLK                15              /* Block device required */
#define IL_ERRNO_EBUSY                  16              /* Device or resource busy */
#define IL_ERRNO_EEXIST                 17              /* File exists */
#define IL_ERRNO_EXDEV                  18              /* Cross-device link */
#define IL_ERRNO_ENODEV                 19              /* No such device */
#define IL_ERRNO_ENOTDIR                20              /* Not a directory */
#define IL_ERRNO_EISDIR                 21              /* Is a directory */
#define IL_ERRNO_EINVAL                 22              /* Invalid argument */
#define IL_ERRNO_ENFILE                 23              /* File table overflow */
#define IL_ERRNO_EMFILE                 24              /* Too many open files */
#define IL_ERRNO_ENOTTY                 25              /* Not a typewriter */
#define IL_ERRNO_ETXTBSY                26              /* Text file busy */
#define IL_ERRNO_EFBIG                  27              /* File too large */
#define IL_ERRNO_ENOSPC                 28              /* No space left on device */
#define IL_ERRNO_ESPIPE                 29              /* Illegal seek */
#define IL_ERRNO_EROFS                  30              /* Read-only file system */
#define IL_ERRNO_EMLINK                 31              /* Too many links */
#define IL_ERRNO_EPIPE                  32              /* Broken pipe */
#define IL_ERRNO_EDOM                   33              /* Math argument out of domain of func */
#define IL_ERRNO_ERANGE                 34              /* Math result not representable */
#define IL_ERRNO_EDEADLK                35              /* Resource deadlock would occur */
#define IL_ERRNO_ENAMETOOLONG   36                      /* File name too long */
#define IL_ERRNO_ENOLCK                 37              /* No record locks available */
#define IL_ERRNO_ENOSYS                 38              /* Function not implemented */
#define IL_ERRNO_ENOTEMPTY              39              /* Directory not empty */
#define IL_ERRNO_ELOOP                  40              /* Too many symbolic links encountered */
#define IL_ERRNO_EWOULDBLOCK    IL_ERRNO_EAGAIN         /* Operation would block */
#define IL_ERRNO_ENOMSG                 42              /* No message of desired type */
#define IL_ERRNO_EIDRM                  43              /* Identifier removed */
#define IL_ERRNO_ECHRNG                 44              /* Channel number out of range */
#define IL_ERRNO_EL2NSYNC               45              /* Level 2 not synchronized */
#define IL_ERRNO_EL3HLT                 46              /* Level 3 halted */
#define IL_ERRNO_EL3RST                 47              /* Level 3 reset */
#define IL_ERRNO_ELNRNG                 48              /* Link number out of range */
#define IL_ERRNO_EUNATCH                49              /* Protocol driver not attached */
#define IL_ERRNO_ENOCSI                 50              /* No CSI structure available */
#define IL_ERRNO_EL2HLT                 51              /* Level 2 halted */
#define IL_ERRNO_EBADE                  52              /* Invalid exchange */
#define IL_ERRNO_EBADR                  53              /* Invalid request descriptor */
#define IL_ERRNO_EXFULL                 54              /* Exchange full */
#define IL_ERRNO_ENOANO                 55              /* No anode */
#define IL_ERRNO_EBADRQC                56              /* Invalid request code */
#define IL_ERRNO_EBADSLT                57              /* Invalid slot */
#define IL_ERRNO_EDEADLOCK              IL_ERRNO_EDEADLK
#define IL_ERRNO_EBFONT                 59              /* Bad font file format */
#define IL_ERRNO_ENOSTR                 60              /* Device not a stream */
#define IL_ERRNO_ENODATA                61              /* No data available */
#define IL_ERRNO_ETIME                  62              /* Timer expired */
#define IL_ERRNO_ENOSR                  63              /* Out of streams resources */
#define IL_ERRNO_ENONET                 64              /* Machine is not on the network */
#define IL_ERRNO_ENOPKG                 65              /* Package not installed */
#define IL_ERRNO_EREMOTE                66              /* Object is remote */
#define IL_ERRNO_ENOLINK                67              /* Link has been severed */
#define IL_ERRNO_EADV                   68              /* Advertise error */
#define IL_ERRNO_ESRMNT                 69              /* Srmount error */
#define IL_ERRNO_ECOMM                  70              /* Communication error on send */
#define IL_ERRNO_EPROTO                 71              /* Protocol error */
#define IL_ERRNO_EMULTIHOP              72              /* Multihop attempted */
#define IL_ERRNO_EDOTDOT                73              /* RFS specific error */
#define IL_ERRNO_EBADMSG                74              /* Not a data message */
#define IL_ERRNO_EOVERFLOW              75              /* Value too large for defined data type */
#define IL_ERRNO_ENOTUNIQ               76              /* Name not unique on network */
#define IL_ERRNO_EBADFD                 77              /* File descriptor in bad state */
#define IL_ERRNO_EREMCHG                78              /* Remote address changed */
#define IL_ERRNO_ELIBACC                79              /* Can not access a needed shared library */
#define IL_ERRNO_ELIBBAD                80              /* Accessing a corrupted shared library */
#define IL_ERRNO_ELIBSCN                81              /* .lib section in a.out corrupted */
#define IL_ERRNO_ELIBMAX                82              /* Attempting to link in too many shared libs */
#define IL_ERRNO_ELIBEXEC               83              /* Cannot exec a shared library directly */
#define IL_ERRNO_EILSEQ                 84              /* Illegal byte sequence */
#define IL_ERRNO_ERESTART               85              /* Interrupted system call should be restarted */
#define IL_ERRNO_ESTRPIPE               86              /* Streams pipe error */
#define IL_ERRNO_EUSERS                 87              /* Too many users */
#define IL_ERRNO_ENOTSOCK               88              /* Socket operation on non-socket */
#define IL_ERRNO_EDESTADDRREQ   89                      /* Destination address required */
#define IL_ERRNO_EMSGSIZE               90              /* Message too long */
#define IL_ERRNO_EPROTOTYPE             91              /* Protocol wrong type for socket */
#define IL_ERRNO_ENOPROTOOPT    92                      /* Protocol not available */
#define IL_ERRNO_EPROTONOSUPPORT 93                     /* Protocol not supported */
#define IL_ERRNO_ESOCKTNOSUPPORT 94                     /* Socket type not supported */
#define IL_ERRNO_EOPNOTSUPP             95              /* Operation not supported on transport endpoint */
#define IL_ERRNO_EPFNOSUPPORT   96                      /* Protocol family not supported */
#define IL_ERRNO_EAFNOSUPPORT   97                      /* Address family not supported by protocol */
#define IL_ERRNO_EADDRINUSE             98              /* Address already in use */
#define IL_ERRNO_EADDRNOTAVAIL  99                      /* Cannot assign requested address */
#define IL_ERRNO_ENETDOWN               100             /* Network is down */
#define IL_ERRNO_ENETUNREACH    101                     /* Network is unreachable */
#define IL_ERRNO_ENETRESET              102             /* Network dropped connection because of reset */
#define IL_ERRNO_ECONNABORTED   103                     /* Software caused connection abort */
#define IL_ERRNO_ECONNRESET             104             /* Connection reset by peer */
#define IL_ERRNO_ENOBUFS                105             /* No buffer space available */
#define IL_ERRNO_EISCONN                106             /* Transport endpoint is already connected */
#define IL_ERRNO_ENOTCONN               107             /* Transport endpoint is not connected */
#define IL_ERRNO_ESHUTDOWN              108             /* Cannot send after transport endpoint shutdown */
#define IL_ERRNO_ETOOMANYREFS   109                     /* Too many references: cannot splice */
#define IL_ERRNO_ETIMEDOUT              110             /* Connection timed out */
#define IL_ERRNO_ECONNREFUSED   111                     /* Connection refused */
#define IL_ERRNO_EHOSTDOWN              112             /* Host is down */
#define IL_ERRNO_EHOSTUNREACH   113                     /* No route to host */
#define IL_ERRNO_EALREADY               114             /* Operation already in progress */
#define IL_ERRNO_EINPROGRESS    115                     /* Operation now in progress */
#define IL_ERRNO_ESTALE                 116             /* Stale NFS file handle */
#define IL_ERRNO_EUCLEAN                117             /* Structure needs cleaning */
#define IL_ERRNO_ENOTNAM                118             /* Not a XENIX named type file */
#define IL_ERRNO_ENAVAIL                119             /* No XENIX semaphores available */
#define IL_ERRNO_EISNAM                 120             /* Is a named type file */
#define IL_ERRNO_EREMOTEIO              121             /* Remote I/O error */
#define IL_ERRNO_EDQUOT                 122             /* Quota exceeded */
#define IL_ERRNO_ENOMEDIUM              123             /* No medium found */
#define IL_ERRNO_EMEDIUMTYPE    124                     /* Wrong medium type */

#define DIR_SEP         '/'

JITINT32        Platform_Stdio_StdRead_iacii (JITINT32 fd, void *array, JITINT32 index, JITINT32 count);                //ok
JITINT32        Platform_Stdio_StdRead_i (JITINT32 fd);                                                                 //ok
void            Platform_Stdio_StdClose (JITINT32 fd);                                                                  //ok
void            Platform_Stdio_StdFlush (JITINT32 fd);                                                                  //ok
void            Platform_Stdio_StdWrite (JITINT32 fd, void *array, JITINT32 index, JITINT32 count);                     //ok
void                    Platform_Stdio_StdWrite_iabii (JITINT32 fd, void *array, JITINT32 index, JITINT32 count);       //ok
void                    Platform_Stdio_StdWrite_iacii (JITINT32 fd, void *array, JITINT32 index, JITINT32 count);       //ok
JITINT32        Platform_Stdio_StdPeek (JITINT32 fd);                                                                   //ok

JITBOOLEAN      Platform_FileMethods_SetLength (JITNINT handle, JITINT64 value);
JITBOOLEAN         Platform_FileMethods_Open (void *string_path, JITINT32 mode, JITINT32 access, JITINT32 share, JITNINT *handle);      //ok
JITBOOLEAN         Platform_FileMethods_Close (JITNINT handle);
JITBOOLEAN         Platform_FileMethods_Write (JITNINT handle, void *array, JITINT32 offset, JITINT32 count);                           //Probl Reflection
JITBOOLEAN         Platform_FileMethods_CanSeek (JITNINT handle);                                                                       //ok
JITINT32        Platform_FileMethods_GetErrno (void);                                                                                   //ok
JITNINT         Platform_FileMethods_GetInvalidHandle (void);                                                                           //ok
JITBOOLEAN              Platform_FileMethods_ValidatePathname (void *path);                                                             //ok
JITINT32        Platform_FileMethods_GetFileType (void *path);                                                                          //ok
JITINT64        Platform_FileMethods_Seek (JITNINT handle, JITINT64 offset, JITINT32 origin);                                           //ok
JITINT32        Platform_FileMethods_Copy (void *src, void *dest);                                                                      //ok
JITBOOLEAN      Platform_FileMethods_CheckHandleAccess (JITNINT handle, JITINT32 access);                                               //ok
JITBOOLEAN      Platform_FileMethods_HasAsync ();                                                                                       //ok
JITBOOLEAN      Platform_FileMethods_FlushWrite (JITNINT handle);                                                                       //ok
JITINT32        Platform_FileMethods_SetLastWriteTime (void *path, JITINT64 ticks);                                                     //ok
JITINT32        Platform_FileMethods_SetLastAccessTime (void *path, JITINT64 ticks);                                                    //ok
JITINT32        Platform_FileMethods_SetCreationTime (void *path, JITINT64 ticks);                                                      //ok
JITINT32        Platform_FileMethods_GetAttributes (void *string_path, JITINT32 *atts);                                                 //ok
JITINT32        Platform_FileMethods_SetAttributes (void* string_path, JITINT32 *attrs);                                                //ok
JITINT32        Platform_FileMethods_CreateLink (void *oldpath, void *newpath);                                                         //NOTEST
JITINT32        Platform_FileMethods_GetLength (void *string_path, JITINT64 *length);                                                   //NOTEST
JITINT32        Platform_FileMethods_ReadLink (void *path, void **contents);                                                            //NOTEST
JITBOOLEAN      Platform_FileMethods_Lock (JITNINT handle, JITINT64 position, JITINT64 length);                                         //NOTEST
JITBOOLEAN      Platform_FileMethods_Unlock (JITNINT handle, JITINT64 position, JITINT64 length);                                       //NOTEST
JITINT32        Platform_FileMethods_Read (JITNINT handle, void *buffer, JITINT32 offset, JITINT32 count);                              //ok

void            Platform_DirMethods_GetPathInfo (void *pathInfo);
void *          Platform_DirMethods_GetCurrentDirectory (void);                                                                 //ok
JITINT32        Platform_DirMethods_CreateDirectory (void *string_path);                                                        //ok
JITINT32        Platform_DirMethods_Delete (void *string_path);                                                                 //ok
JITINT32        Platform_DirMethods_Rename (void *old_name, void *new_name);                                                    //ok
JITINT32        Platform_DirMethods_ChangeDirectory (void *new_path);                                                           //ok
void *          Platform_DirMethods_GetLogicalDrives (void);                                                                    //ok
JITINT32        Platform_DirMethods_GetLastAccess (void *c_path, JITINT64 *lastac);                                             //ok
JITINT32        Platform_DirMethods_GetLastModification (void *c_path, JITINT64 *last_mod);                                     //ok
JITINT32        Platform_DirMethods_GetCreationTime (void *c_path, JITINT64 *create_time);                                      //ok
void *          Platform_DirMethods_GetSystemDirectory (void);                                                                  //ok

void *          Platform_InfoMethods_GetNetBIOSMachineName (void);                                                              //ok
JITINT32        Platform_InfoMethods_GetProcessorCount (void);                                                                  //ok
JITINT64        Platform_InfoMethods_GetWorkingSet (void);                                                                      //ok
JITINT32        Platform_InfoMethods_GetPlatformID (void);                                                                      //ok
JITBOOLEAN      Platform_InfoMethods_IsUserInteractive (void);                                                                  //ok
void *          Platform_InfoMethods_GetUserStorageDir (void);                                                                  //ok
void *          Platform_InfoMethods_GetGlobalConfigDir (void);                                                                 //ok
void *          Platform_InfoMethods_GetPlatformName (void);                                                                    //ok
void *          Platform_InfoMethods_GetUserName (void);                                                                        //ok

JITINT32 internal_fileExists (JITINT8 *fileName);

/**
 * Get an handle for the standard error stream.
 *
 * This is a Mono internal call.
 *
 * @return an handle for the standard error stream
 */
JITNINT System_IO_MonoIO_get_ConsoleError (void);

/**
 * Get an handle for the standard output stream
 *
 * This is a Mono internal call.
 *
 * @return an handle for the standard output stream
 */
JITNINT System_IO_MonoIO_get_ConsoleOutput (void);

/**
 * Get an handle for the standard input stream
 *
 * This is a Mono internal call.
 *
 * @return an handle for the standard input stream
 */
JITNINT System_IO_MonoIO_get_ConsoleInput (void);

/**
 * Get the file type of handle
 *
 * This is a Mono internal call.
 *
 * @param handle the handle to get the type
 * @param error filled with an error code, if the call fails
 *
 * @return the type of handle
 */
JITINT32 System_IO_MonoIO_GetFileType (JITNINT handle, JITINT32* error);

/**
 * Seek inside the file represented by handle
 *
 * This is a Mono internal call
 *
 * @param handle the file where to seek
 * @param offset how many bytes to seek
 * @param from seek start point
 * @param filled with an error code, if the call fails
 *
 * @return the position of the cursor inside the file after the seek
 */
JITINT64 System_IO_MonoIO_Seek (JITNINT handle, JITINT64 offset, JITINT32 from, JITINT32* error);

/**
 * Write to file
 *
 * This is a Mono internal call. Write count bytes, starting from sourceOffset,
 * of source into the file identifies by handle. The variable pointed by error
 * is filled with the operation exit status.
 *
 * @param handle identifier of the file where write data
 * @param source data to be written
 * @param sourceOffset offset inside source where the data to be written starts
 * @param count how many bytes to write
 * @param error after the call will contains the write exit status
 *
 * @return the number of written bytes
 */
JITINT32 System_IO_MonoIO_Write (JITNINT handle, void* source, JITINT32 sourceOffset, JITINT32 count, JITINT32* error);

/**
 * Gets the volume separator character
 *
 * @return the volume separator character
 */
JITINT16 System_IO_MonoIO_get_VolumeSeparatorChar (void);

/**
 * Gets the directory separator character
 *
 * @return the directory separator character
 */
JITINT16 System_IO_MonoIO_get_DirectorySeparatorChar (void);

/**
 * Gets the alternate directory separator character
 *
 * @return the alternate directory separator character
 */
JITINT16 System_IO_MonoIO_get_AltDirectorySeparatorChar (void);

/**
 * Gets the path separator character
 *
 * @return the path separator character
 */
JITINT16 System_IO_MonoIO_get_PathSeparator (void);

/**
 * Close the file represented by handle
 *
 * This is a Mono internal call
 *
 * @param handle representation of the file to close
 * @param error pointer to an error variable that will be filled
 *
 * @return JITTRUE on success, JITFALSE otherwise
 */
JITBOOLEAN System_IO_MonoIO_Close (JITNINT handle, JITINT32* error);

/**
 * Get the current directory name
 *
 * This is a Mono internal call.
 *
 * @param error filled with the operation exit status
 *
 * @return the name of the current directory
 */
void* System_IO_MonoIO_GetCurrentDirectory (JITINT32* error);

/**
 * Set the current directory
 *
 * This is a Mono internal call.
 *
 * @param directory the new current directory
 * @param error filled with the operation exit status
 *
 * @return JITTRUE on success, JITFALSE otherwise
 */
JITBOOLEAN System_IO_MonoIO_SetCurrentDirectory (void* directory, JITINT32* error);

/**
 * Get the attributes of the file with the given name
 *
 * This is a Mono internal call.
 *
 * @param name the name of the file with unknown attributes
 * @param error filled with the operation exit status
 *
 * @return the attributes of the file whit the given name, or -1 on errors
 */
JITINT32 System_IO_MonoIO_GetFileAttributes (void* name, JITINT32* error);

/**
 * Create the directory name
 *
 * This is a Mono internal call.
 *
 * @param name the name of the directory to create
 * @param error filled with the operation exit status
 *
 * @return JITTRUE on success, JITFALSE on failure
 */
JITBOOLEAN System_IO_MonoIO_CreateDirectory (void* name, JITINT32* error);

/**
 * Get statistics about the file with the given path
 *
 * This is a Mono internal call.
 *
 * @param path the path to the file to be analyzed
 * @param stats a value type to be filled with the file statistics
 * @param error filled with the operation exit status
 *
 * @return JITTRUE on success, JITFALSE otherwise
 */
JITBOOLEAN System_IO_MonoIO_GetFileStat (void* path, void* stats, JITUINT32* error);

/**
 * Delete given file
 *
 * This is a Mono internal call.
 *
 * @param path path to the file to delete
 * @param error filled with the operation exit status
 *
 * @return JITTRUE on success, JITFALSE otherwise
 */
JITBOOLEAN System_IO_MonoIO_DeleteFile (void* path, JITINT32* error);

/**
 * Delete given directory
 *
 * This is a Mono internal call.
 *
 * @param path path to the directory to delete
 * @param error filled with the operation exit status
 *
 * @return JITTTRUE on success, JITFALSE otherwise
 */
JITBOOLEAN System_IO_MonoIO_RemoveDirectory (void* path, JITINT32* error);

/**
 * Move a file
 *
 * This is a Mono internal call.
 *
 * @param from the new file path
 * @param to path to the file to move
 * @param error filled with the operation exit status
 *
 * @return JITTRUE on success, JITFALSE otherwise
 */
JITBOOLEAN System_IO_MonoIO_MoveFile (void* from, void* to, JITINT32* error);

/**
 * Open a file
 *
 * This is a Mono internal call. The four enums specify how to open the file.
 * See the documentation of FileMode, FileAccess, FileShare, and  FileOptions
 * for further info.
 *
 * @param name path to the file to open
 * @param mode specify the OS behavior while opening
 * @param access how access to the file (e.g. read, write, ...)
 * @param share how other file stream should access to the opened file
 * @param options low level file opening options (e.g. caching, encryption, ...)
 * @param error filled with the operation exit status
 *
 * @return a file handle, or -1 on errors
 */
JITNINT System_IO_MonoIO_Open (void* name, JITINT32 mode, JITINT32 access, JITINT32 share, JITINT32 options, JITINT32* error);

/**
 * Set the length of the file represented by handle
 *
 * This is a Mono internal call.
 *
 * @param handle identifier of the file where operate
 * @param length the file new length
 * @param error filled with the operation exit status
 *
 * @return JITTRUE on success, JITFALSE otherwise
 */
JITBOOLEAN System_IO_MonoIO_SetLength (JITNINT handle, JITINT64 length, JITINT32* error);

/**
 * Get the length of the file identified by handle
 *
 * This is a Mono internal call.
 *
 * @param handle identifier of the file where to operate
 * @param error filled with the operation exit status
 *
 * @return the file length, or -1 on errors
 */
JITINT64 System_IO_MonoIO_GetLength (JITNINT handle, JITINT32* error);

/**
 * Read a bunch of data from a file
 *
 * @param handle identifier of the file where to operate
 * @param buffer will hold read data
 * @param offset where start writing inside the buffer
 * @param count how many bytes to read
 * @param error filled with the operation exit status
 *
 * @return the number of read bytes, or -1 on errors
 */
JITINT32 System_IO_MonoIO_Read (JITNINT handle, void* buffer, JITINT32 offset, JITINT32 count, JITINT32* error);

/*
 * Call to the isatty function to test whether a file descriptor refers to the terminal
 *
 * This is a Mono internal call.
 *
 * @param handle identifier of the file where to operate
 *
 * @return 1 if the open file descriptor refers to a terminal, 0 otherwise
 */

JITBOOLEAN System_ConsoleDriver_Isatty (JITNINT handle);

#endif
