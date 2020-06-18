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
#include <errno.h>
#include <sys/types.h>
#include <platform_API.h>
#include <ildjit.h>

// My headers
#include <internal_calls_IO.h>
#include <internal_calls_utilities.h>
#include <clr_system.h>
// End


#ifdef __MINGW32__      /* S_ISLNK only is not supported by minGW. Supported by platform_API.*/
#define S_ISLNK      PLATFORM_S_ISLNK
#endif

/* Mono file types */
typedef enum {
    MonoFileType_Unknown = 0x0,
    MonoFileType_Disk = 0x1,
    MonoFileType_Char = 0x2,
    MonoFileType_Pipe = 0x3,
    MonoFileType_Remote = 0x8000
} MonoFileType;

/* Number of fields in the MonoIOStat value type */
#define MONO_IO_STAT_FIELD_COUNT 6

/* Info about the MonoIOStat value type */
typedef struct {
    JITINT32 fieldOffsets[MONO_IO_STAT_FIELD_COUNT];
} monoIOStatInfo_t;

monoIOStatInfo_t monoIOStat;

/* Fields in MonoIOStat */
typedef enum {
    MONO_IO_STAT_Name = 0,
    MONO_IO_STAT_Attributes = 1,
    MONO_IO_STAT_Length = 2,
    MONO_IO_STAT_CreationTime = 3,
    MONO_IO_STAT_LastAccessTime = 4,
    MONO_IO_STAT_LastWriteTime = 5
} MonoIOStatFields;

/* Allows quick field setting */
#define MONO_IO_STAT_SET_FIELD(stats, field, type, value) \
	*((type*) (stats + monoIOStat.fieldOffsets[field])) = value;

/* Check for MonoIO related metadata loading */
#define CHECK_MONO_IO_METADATA() \
	PLATFORM_pthread_once(&monoIOLock, monoIOLoadMetadata)

/* Guard of the MonoIO metadata */
static pthread_once_t monoIOLock = PTHREAD_ONCE_INIT;

/* Mono error codes */
typedef enum {
    MONO_ERROR_SUCCESS = 0,
    MONO_ERROR_FILE_NOT_FOUND = 2,
    MONO_ERROR_PATH_NOT_FOUND = 3,
    MONO_ERROR_TOO_MANY_OPEN_FILES = 4,
    MONO_ERROR_ACCESS_DENIED = 5,
    MONO_ERROR_INVALID_HANDLE = 6,
    MONO_ERROR_INVALID_DRIVE = 15,
    MONO_ERROR_NOT_SAME_DEVICE = 17,
    MONO_ERROR_NO_MORE_FILES = 18,
    MONO_ERROR_WRITE_FAULT = 29,
    MONO_ERROR_READ_FAULT = 30,
    MONO_ERROR_GEN_FAILURE = 31,
    MONO_ERROR_SHARING_VIOLATION = 32,
    MONO_ERROR_LOCK_VIOLATION = 33,
    MONO_ERROR_HANDLE_DISK_FULL = 39,
    MONO_ERROR_FILE_EXISTS = 80,
    MONO_ERROR_CANNOT_MAKE = 82,
    MONO_ERROR_INVALID_PARAMETER = 87,
    MONO_ERROR_BROKEN_PIPE = 109,
    MONO_ERROR_INVALID_NAME = 123,
    MONO_ERROR_DIR_NOT_EMPTY = 145,
    MONO_ERROR_ALREADY_EXISTS = 183,
    MONO_ERROR_NO_SIGNAL_SENT = 205,
    MONO_ERROR_FILENAME_EXCED_RANGE = 206,
    MONO_ERROR_ENCRYPTION_FAILED = 6000
} MonoIOError;

/* Used to automate properties conversion from PNET to MONO */
typedef JITINT32 (*PnetLongStatGetter)(void*, JITINT64*);
typedef void (*MonoLongStatSetter)(void*, JITINT64);

/* Write inside a MonoIOStat value type */
static void MonoIOStat_setAttributes (void* stats, JITINT32 attributes);
static void MonoIOStat_setLength (void* stats, JITINT64 length);
static void MonoIOStat_setCreationTime (void* stats, JITINT64 length);
static void MonoIOStat_setLastAccessTime (void* stats, JITINT64 length);
static void MonoIOStat_setLastWriteTime (void* stats, JITINT64 length);

static void monoIOLoadMetadata (void);

/* Test if something is a normal file.  */
#ifndef S_ISREG
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#endif

/* Test if something is a directory.  */
#ifndef S_ISDIR
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#endif

/* Test if something is a character special file.  */
#ifndef S_ISCHR
#define S_ISCHR(m) (((m) & S_IFMT) == S_IFCHR)
#endif

/* Test if something is a block special file.  */
#ifndef S_ISBLK
#define S_ISBLK(m) (((m) & S_IFMT) == S_IFBLK)
#endif

/* Test if something is a socket.  */
#ifndef S_ISSOCK
# ifdef S_IFSOCK
#   define S_ISSOCK(m) (((m) & S_IFMT) == S_IFSOCK)
# else
#   define S_ISSOCK(m) 0
# endif
#endif

/* Test if something is a FIFO.  */
#ifndef S_ISFIFO
# ifdef S_IFIFO
#  define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)
# else
#  define S_ISFIFO(m) 0
# endif
#endif

/* Approximate O_NONBLOCK.  */
#ifndef O_NONBLOCK
#define O_NONBLOCK O_NDELAY
#endif

/* Approximate O_NOCTTY.  */
#ifndef O_NOCTTY
#define O_NOCTTY 0
#endif

/* Define well known filenos if the system does not define them.  */
#ifndef STDIN_FILENO
# define STDIN_FILENO   0
#endif
#ifndef STDOUT_FILENO
# define STDOUT_FILENO  1
#endif
#ifndef STDERR_FILENO
# define STDERR_FILENO  2
#endif

/* Get platform directory pathname information */
typedef struct {
    JITUINT16 dirSep;
    JITUINT16 altDirSep;
    JITUINT16 volumeSep;
    JITUINT16 pathSep;
    const char     *invalidPathChars;

} ILPathInfo;

static inline void ILGetPathInfo (ILPathInfo *info);
JITNINT internal_OpenFile (const char *path, JITINT32 mode, JITINT32 access, JITINT32 share);
static inline JITINT32 internal_getFileType (JITINT8 *path);
static inline JITINT32 internal_CreateDir (JITINT8 *path);
static inline JITINT32 internal_DeleteDir (JITINT8 *path);
JITINT32 internal_Write (JITNINT handle, void *buf, JITINT32 size);
JITINT32 internal_FileRead (JITNINT handle, void *buf, JITINT32 size);
JITBOOLEAN internal_CheckHandleAccess (JITNINT handle, JITINT32 access);
static inline JITINT64 internal_Seek (JITNINT handle, JITINT64 offset, JITINT32 whence);
static JITINT8 * internal_GetFileInBasePath (const char *tail1, const char *tail2);
static inline JITINT32 internal_Copy (JITINT8 *src, JITINT8 *dest);
static inline JITINT8 *GetBasePath (void);
static inline JITINT32 internal_GetLastAccModCreTime (JITINT8 *path, JITINT64 *time, JITINT32 Mode);
static inline JITINT32 internal_SetModificationTime (JITINT8 *path, JITINT64 time);
static inline JITINT32 internal_SetAccessTime (JITINT8 *path, JITINT64 time);
static JITINT32 translateToMonoErrorCode (JITINT32 pnetError);
static inline int ILSysIOConvertErrno (int error);
static inline JITINT32 internal_RenameDir (JITINT8 *old_path, JITINT8 *new_path);
static inline JITINT32 internal_ChangeDir (JITINT8 *path);

extern t_system *ildjitSystem;

/*
 * StdioMethods............................................................................................................
 */
void Platform_Stdio_StdClose (JITINT32 fd) {
    METHOD_BEGIN(ildjitSystem, "Platform.Stdio.StdClose");

    /* Return			*/
    METHOD_END(ildjitSystem, "Platform.Stdio.StdClose");
    return;
}

void Platform_Stdio_StdFlush (JITINT32 fd) {
    char buf[512];

    METHOD_BEGIN(ildjitSystem, "Platform.Stdio.StdFlush");

    /* Flush			*/
    switch (fd) {
        case 1:
            fflush(stdin);
            break;
        case 2:
            fflush(stdout);
            break;
        case 3:
            fflush(stderr);
            break;
        default:
            snprintf(buf, sizeof(char) * 512, "Platform.Stdio.StdFlush: ERROR = Stream %u is not known. ", fd);
            print_err(buf, 0);
            abort();
    }

    /* Return			*/
    METHOD_END(ildjitSystem, "Platform.Stdio.StdFlush");
    return;
}

/*
 * public static void StdWrite(int fd, byte[] value, int index, int count);
 */
void Platform_Stdio_StdWrite_iabii (JITINT32 fd, void *array, JITINT32 index, JITINT32 count) {
    JITUINT8        *string;

    /* Assertions			*/
    assert(array != NULL);
    assert((ildjitSystem->cliManager).CLR.arrayManager.getArrayElementSize(array) == 1);
    assert((ildjitSystem->cliManager).CLR.arrayManager.getArrayLength(array, 0) >= index);
    METHOD_BEGIN(ildjitSystem, "Platform.Stdio.StdWrite(int, byte[], int, int)");

    /* Check if we have to print    *
    * something                    */
    if (count == 0) {
        METHOD_END(ildjitSystem, "Platform.Stdio.StdWrite(int, byte[], int, int)");
        return;
    }

    /* Fetch the string		*/
    string = (ildjitSystem->cliManager).CLR.arrayManager.getArrayElement(array, index);
    assert(string != NULL);

    /* Print the string		*/
    if (fd == 1) {
        fwrite(string, 1, count, stdout);
    } else if (fd == 2) {
        fwrite(string, 1, count, stderr);
    } else {
        print_err("ERROR = Stream not known. ", 0);
        abort();
    }

    /* Return			*/
    METHOD_END(ildjitSystem, "Platform.Stdio.StdWrite(int, byte[], int, int)");
    return;
}

/*
 * public static void StdWrite(int fd, char[] value, int index, int count);
 */
void Platform_Stdio_StdWrite_iacii (JITINT32 fd, void *array, JITINT32 index, JITINT32 count) {
    JITUINT16       *string;

    /* Assertions			*/
    assert(array != NULL);
    METHOD_BEGIN(ildjitSystem, "Platform.Stdio.StdWrite(int, char[], int, int)");

    /* Check if we have to print    *
    * something                    */
    if (count == 0) {
        METHOD_END(ildjitSystem, "Platform.Stdio.StdWrite(int, char[], int, int)");
        return;
    }

    /* Fetch the array slot size	*/
#ifdef DEBUG
    assert((ildjitSystem->cliManager).CLR.arrayManager.getArrayElementSize(array) == 2);
    assert((ildjitSystem->cliManager).CLR.arrayManager.getArrayLength(array, 0) > index);
#endif

    /* Fetch the string		*/
    string = (ildjitSystem->cliManager).CLR.arrayManager.getArrayElement(array, index);
    assert(string != NULL);

    /* Print the string		*/
    if (fd == 1) {
        while (count > 0) {
            putc((*string) & 0xFF, stdout);
            string++;
            count--;
        }
    } else if (fd == 2) {
        while (count > 0) {
            putc((*string) & 0xFF, stderr);
            string++;
            count--;
        }
    } else {
        print_err("ERROR = Stream not known. ", 0);
        abort();
    }

    /* Return			*/
    METHOD_END(ildjitSystem, "Platform.Stdio.StdWrite(int, char[], int, int)");
    return;
}

void Platform_Stdio_StdWrite (JITINT32 fd, void *array, JITINT32 index, JITINT32 count) {
    JITINT32 slotSize;

    /* Assertions			        */
    assert(array != NULL);

    /* Fetch the array slot size	        */
    slotSize = (ildjitSystem->cliManager).CLR.arrayManager.getArrayElementSize(array);
    assert((ildjitSystem->cliManager).CLR.arrayManager.getArrayLength(array, 0) >= index);

    /* Call the right internal method       */
    if (slotSize == 1) {
        Platform_Stdio_StdWrite_iabii(fd, array, index, count);
    } else {
        assert(slotSize == 2);
        Platform_Stdio_StdWrite_iacii(fd, array, index, count);
    }

    /* Return			        */
    return;
}

JITINT32 Platform_Stdio_StdRead_iacii (JITINT32 fd, void *array, JITINT32 index, JITINT32 count) {
    JITINT32 result;
    JITUINT16 valueToStore;
    JITINT32 ch;

    /* Assertions			*/
    assert(array != NULL);
    METHOD_BEGIN(ildjitSystem, "Platform.Stdio.StdRead(int, char[], int, int)");

    /* Initialize the variables     */
    result = 0;

    if (fd != 0) {
        METHOD_END(ildjitSystem, "Platform.Stdio.StdRead");
        return -1;
    }

    while (count > 0) {
        ch = getc(stdin);
        if (ch == -1) {
            break;
        }
        valueToStore = (JITUINT16) (ch & 0xFF);
        (ildjitSystem->cliManager).CLR.arrayManager.setValueArrayElement(array, index, &valueToStore);
        index++;
        result++;
        count--;
    }

    /* Return                       */
    METHOD_END(ildjitSystem, "Platform.Stdio.StdRead(int, char[], int, int)");
    return result;
}

JITINT32 Platform_Stdio_StdRead_i (JITINT32 fd) {
    JITINT32 ch;

    METHOD_BEGIN(ildjitSystem, "Platform.Stdio.StdRead(int)");

    /* Check the file descriptor    */
    if (fd != 0) {
        METHOD_END(ildjitSystem, "Platform.Stdio.StdRead(int)");
        return -1;
    }

    /* Read the next character      */
    ch = getc(stdin);

    /* Return                       */
    METHOD_END(ildjitSystem, "Platform.Stdio.StdRead");
    return ch;
}

JITINT32 Platform_Stdio_StdPeek (JITINT32 fd) {

    METHOD_BEGIN(ildjitSystem, "Platform.Stdio.StdRead(int)");

    if (fd == 0) {
        JITINT32 ch = getc(stdin);
        if (ch != -1) {
            ungetc(ch, stdin);
        }

        /* Return                       */
        METHOD_END(ildjitSystem, "Platform.Stdio.StdRead");
        return (JITINT32) ch;
    }

    /* Return                       */
    METHOD_END(ildjitSystem, "Platform.Stdio.StdRead");
    return -1;
}

/*
 * FileMethods................................................................................................................................
 */

JITINT32 Platform_FileMethods_Copy (void *src, void *dest) {
    JITINT32 length_old = -1;
    JITINT32 length_new = -1;
    JITINT32 result;
    JITINT8         *old_literalUTF8;
    JITINT16        *old_literalUTF16;
    JITINT8         *new_literalUTF8;
    JITINT16        *new_literalUTF16;

    METHOD_BEGIN(ildjitSystem, "Platform.FileMethods.Copy(char [],char [])");

    /* Validate                     */
    if ((src) && (dest) ) {
        length_old = (ildjitSystem->cliManager).CLR.stringManager.getLength(src);
        length_new = (ildjitSystem->cliManager).CLR.stringManager.getLength(dest);

        if ((length_old > 0) || (length_new > 0)) {

            old_literalUTF16 = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(src);
            /* Assertion				*/
            assert(old_literalUTF16 != NULL);

            new_literalUTF16 = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(dest);
            /* Assertion				*/
            assert(new_literalUTF16 != NULL);

            old_literalUTF8 = allocMemory(length_old * sizeof(char));
            new_literalUTF8 = allocMemory(length_new * sizeof(char));
            (ildjitSystem->cliManager).CLR.stringManager.fromUTF16ToUTF8(old_literalUTF16, old_literalUTF8, length_old);
            (ildjitSystem->cliManager).CLR.stringManager.fromUTF16ToUTF8(new_literalUTF16, new_literalUTF8, length_new);
            result = internal_Copy(old_literalUTF8, new_literalUTF8);
            freeMemory(old_literalUTF8);
            freeMemory(new_literalUTF8);

            /* Return                       */
            METHOD_BEGIN(ildjitSystem, "Platform.FileMethods.Copy(char [],char [])");
            return result;
        }
    }

    /* Return                       */
    METHOD_BEGIN(ildjitSystem, "Platform.DirMethods.Rename(char [],char [])");
    return IL_ERRNO_ENOMEM;
}

JITINT64 Platform_FileMethods_Seek (JITNINT handle, JITINT64 offset, JITINT32 origin) {
    JITINT64 result;

    METHOD_BEGIN(ildjitSystem, "Platform.FileMethods.Seek(nint, long ,int)");

    result = internal_Seek(handle, offset, origin);

    /* Return                       */
    METHOD_END(ildjitSystem, "Platform.FileMethodos.Seek(nint, long ,int)");
    return result;
}

JITBOOLEAN Platform_FileMethods_CanSeek (JITNINT handle) {
    JITBOOLEAN result;

    METHOD_BEGIN(ildjitSystem, "Platform.FileMethods.CanSeek(nint)");

    result = (internal_Seek(handle, (JITINT64) 0, 1) != (JITINT64) (-1));

    /* Return                       */
    METHOD_END(ildjitSystem, "Platform.FileMethodos.CanSeek(nint)");
    return result;
}

JITBOOLEAN Platform_FileMethods_Write (JITNINT handle, void *array, JITINT32 offset, JITINT32 count) {
    void            *buf;
    JITBOOLEAN result;

    METHOD_BEGIN(ildjitSystem, "Platform.FileMethods.Write(nint,void*,int,int)");

    buf = (ildjitSystem->cliManager).CLR.arrayManager.getArrayElement(array, offset);
    assert(buf != NULL);
    result = (internal_Write((JITNINT) handle, buf, count) == count);

    /* Return                       */
    METHOD_END(ildjitSystem, "Platform.FileMethods.Write(nint,void*,int,int)");
    return result;
}

JITBOOLEAN Platform_FileMethods_CheckHandleAccess (JITNINT handle, JITINT32 access) {

    METHOD_BEGIN(ildjitSystem, "Platform.FileMethods.CheckHandleAccess(int, int)");

    /* Return                       */
    METHOD_END(ildjitSystem, "Platform.FileMethodos.CheckHandleAccess(int, int)");
    return internal_CheckHandleAccess((JITNINT) handle, access);
}

JITINT32 Platform_FileMethods_GetErrno (void) {
    METHOD_BEGIN(ildjitSystem, "Platform.FileMethods.GetErrno");

    /* Return                       */
    METHOD_END(ildjitSystem, "Platform.FileMethods.GetErrno");
    return errno;
}

JITBOOLEAN Platform_FileMethods_Close (JITNINT handle) {
    JITINT32 result;

    METHOD_BEGIN(ildjitSystem, "Platform.FileMethods.Close");

    while ((result = PLATFORM_close(handle)) < 0) {
        if (errno != EINTR) {
            break;
        }
    }
    xanHashTable_removeElement(filesOpened, (void *)(JITNUINT)handle);

    /* Return                       */
    METHOD_END(ildjitSystem, "Platform.FileMethods.Close");
    return result == 0;
}

JITBOOLEAN Platform_FileMethods_Open (void *string_path, JITINT32 mode, JITINT32 access, JITINT32 share, JITNINT *handle) {
    JITBOOLEAN result;
    JITINT8         *literalUTF8;
    JITINT16        *literalUTF16;
    JITINT32 length;

    METHOD_BEGIN(ildjitSystem, "Platform.FileMethods.Open");

    result = JITFALSE;
    if (string_path) {
        length = (ildjitSystem->cliManager).CLR.stringManager.getLength(string_path);
        if (length > 0) {
            literalUTF16 = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(string_path);
            assert(literalUTF16 != NULL);
            literalUTF8 = allocMemory((length + 1) * sizeof(char));
            (ildjitSystem->cliManager).CLR.stringManager.fromUTF16ToUTF8(literalUTF16, literalUTF8, length);
            (*handle) = internal_OpenFile((const char *) literalUTF8, mode, access, share);
            result = ((*handle) != Platform_FileMethods_GetInvalidHandle());
            freeMemory(literalUTF8);
        }
    }

    /* Return                       */
    METHOD_END(ildjitSystem, "Platform.FileMethods.Open");
    return result;
}

JITNINT Platform_FileMethods_GetInvalidHandle () {

    METHOD_BEGIN(ildjitSystem, "Platform.FileMethods.GetInvalidHandle");

    /* Return                       */
    METHOD_END(ildjitSystem, "Platform.FileMethods.GetInvalidHandle");
    return -1;
}

JITBOOLEAN Platform_FileMethods_ValidatePathname (void *path) {
    JITBOOLEAN result;

    METHOD_BEGIN(ildjitSystem, "Platform.FileMethods.ValidatePathName");

    /* Validate                     */
    result = JITFALSE;
    if (path != NULL) {
        if ((ildjitSystem->cliManager).CLR.stringManager.getLength(path) > 0) {
            result = JITTRUE;
        }
    }

    /* Return                       */
    METHOD_END(ildjitSystem, "Platform.FileMethods.ValidatePathName");
    return result;
}

JITINT32 Platform_FileMethods_GetFileType (void *path) {
    JITINT32 result;
    JITINT8         *literalUTF8;
    JITINT16        *literalUTF16;
    JITINT32 length;

    METHOD_BEGIN(ildjitSystem, "Platform.FileMethods.GetFileType");

    /* Validate                     */
    result = ILFileType_Unknown;
    if (path != NULL) {
        length = (ildjitSystem->cliManager).CLR.stringManager.getLength(path);
        if (length > 0) {
            literalUTF16 = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(path);
            assert(literalUTF16 != NULL);
            literalUTF8 = allocMemory((length + 1) * sizeof(char));
            (ildjitSystem->cliManager).CLR.stringManager.fromUTF16ToUTF8(literalUTF16, literalUTF8, length);
            result = internal_getFileType(literalUTF8);
            freeMemory(literalUTF8);
        }
    }

    /* Return                       */
    METHOD_END(ildjitSystem, "Platform.FileMethods.GetFileType");
    return result;
}

/*
 * FileMethods................................................................................................................................
 */

// Determine if this platform has support for asynchronous file handle operations.
JITBOOLEAN Platform_FileMethods_HasAsync () {

    METHOD_BEGIN(ildjitSystem, "Platform.FileMethods.HasAsync()");

    /* Return			*/
    METHOD_END(ildjitSystem, "Platform.FileMethodos.HasAsync()");
    return JITFALSE;
}

// Read data from a file handle into a supplied buffer.
// Returns -1 if an I/O error of some kind occurred,
// 0 at EOF, or the number of bytes read otherwise. ***
JITINT32 Platform_FileMethods_Read (JITNINT handle, void* buffer,  JITINT32 offset, JITINT32 count) {
    JITUINT8        *buf;
    JITINT32 result;

    METHOD_BEGIN(ildjitSystem, "Platform.FileMethods.Read");

    buf = (ildjitSystem->cliManager).CLR.arrayManager.getArrayElement(buffer, offset);
    assert(buf != NULL);
    result = internal_FileRead(handle, buf, count);

    METHOD_END(ildjitSystem, "Platform.FileMethods.Read");
    return result;
}

JITINT32 internal_FileRead (JITNINT handle, void *buf, JITINT32 size) {
    JITINT32 result;

    while ((result = PLATFORM_read((JITINT32) (JITNINT) handle,  buf, (JITINT32) size)) < 0) {
        // Retry if the system call was interrupted
        if (errno != EINTR) {
            break;
        }
    }
    return (JITINT32) result;
}

JITINT32 Platform_FileMethods_CreateLink (void *oldpath, void *newpath) {
    JITINT32 oldlength;
    JITINT8         *oldliteralUTF8;
    JITINT16        *oldliteralUTF16;
    JITINT32 newlength;
    JITINT8         *newliteralUTF8;
    JITINT16        *newliteralUTF16;

    METHOD_BEGIN(ildjitSystem, "Platform.FileMethodos.CreateLink(char [],char [])");

    if (oldpath && newpath) {
        oldlength = (ildjitSystem->cliManager).CLR.stringManager.getLength(oldpath);
        newlength = (ildjitSystem->cliManager).CLR.stringManager.getLength(newpath);

        if ((newlength > 0) && (oldlength)) {
            oldliteralUTF16 = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(oldpath);
            assert(oldliteralUTF16 != NULL);
            oldliteralUTF8 = allocMemory(oldlength * sizeof(char));
            (ildjitSystem->cliManager).CLR.stringManager.fromUTF16ToUTF8(oldliteralUTF16, oldliteralUTF8, oldlength);
            newliteralUTF16 = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(newpath);
            assert(newliteralUTF16 != NULL);
            newliteralUTF8 = allocMemory(newlength * sizeof(char));
            (ildjitSystem->cliManager).CLR.stringManager.fromUTF16ToUTF8(newliteralUTF16, newliteralUTF8, newlength);
            if (!newliteralUTF8 || !oldliteralUTF8) {
                METHOD_END(ildjitSystem, "Platform.FileMethodos.CreateLink(char [],char [])");
                return IL_ERRNO_ENOMEM;
            }
            if (PLATFORM_symlink((const char *) oldliteralUTF8, (const char *) newliteralUTF8) >= 0) {
                METHOD_END(ildjitSystem, "Platform.FileMethodos.CreateLink(char [],char [])");
                return IL_ERRNO_Success;
            } else {
                METHOD_END(ildjitSystem, "Platform.FileMethodos.CreateLink(char [],char [])");
                return Platform_FileMethods_GetErrno();
            }

            freeMemory(oldliteralUTF8);
            freeMemory(newliteralUTF8);
        }
    }

    METHOD_END(ildjitSystem, "Platform.FileMethodos.CreateLink(char [],char [])");
    return IL_ERRNO_ENOMEM;
}

// Flush all data that was previously written, in response
// to a user-level "Flush" request.  Normally this will do
// nothing unless the platform provides its own buffering.
// Returns false if an I/O error occurred.
JITBOOLEAN Platform_FileMethods_FlushWrite (JITNINT handle) {
    METHOD_BEGIN(ildjitSystem, "Platform.FileMethods.FlushWrite");

    METHOD_END(ildjitSystem, "Platform.FileMethodos.FlushWrite");
    return JITTRUE;
}

// Set the length of a file to a new value.  Returns false
// if an I/O error occurred.
JITBOOLEAN Platform_FileMethods_SetLength (JITNINT handle, JITINT64 value) {
    JITBOOLEAN result;

    METHOD_BEGIN(ildjitSystem, "Platform.FileMethods.SetLength");

    while ((result = PLATFORM_ftruncate((JITINT32) (JITNINT) handle, (off_t) value)) < 0) {

        /* Retry if the ildjitSystem call was interrupted */
        if (errno != EINTR) {
            break;
        }
    }
    METHOD_END(ildjitSystem, "Platform.FileMethods.SetLength");
    return result == 0;
}

JITBOOLEAN Platform_FileMethods_Lock (JITNINT handle, JITINT64 position, JITINT64 length) {
    struct flock cntl_data;

    METHOD_BEGIN(ildjitSystem, "Platform.FileMethodos.Lock");

    /* set fields individually...who knows what extras are there? */
    cntl_data.l_type = F_WRLCK;
    cntl_data.l_whence = SEEK_SET;
    /* actually, off_t changes in LFS on 32bit, so be careful */
    cntl_data.l_start = (off_t) (JITNINT) position;
    cntl_data.l_len = (off_t) (JITNINT) length;
    /* -1 is error, anything else is OK */
    METHOD_END(ildjitSystem, "Platform.FileMethodos.Lock");
    if (PLATFORM_setFileLock((JITINT32) (JITNINT) handle, &cntl_data) != -1) {
        return JITTRUE;
    }
    return JITFALSE;
}

JITBOOLEAN Platform_FileMethods_Unlock (JITNINT handle, JITINT64 position, JITINT64 length) {
    struct flock cntl_data;

    METHOD_BEGIN(ildjitSystem, "Platform.FileMethodos.Unlock");

    /* set fields individually...who knows what extras are there? */
    cntl_data.l_type = F_UNLCK;
    cntl_data.l_whence = SEEK_SET;
    /* actually, off_t changes in LFS on 32bit, so be careful */
    cntl_data.l_start = (off_t) (JITNINT) position;
    cntl_data.l_len = (off_t) (JITNINT) length;
    /* -1 is error, anything else is OK */
    METHOD_END(ildjitSystem, "Platform.FileMethodos.Unlock");
    if (PLATFORM_setFileLock((JITINT32) (JITNINT) handle, &cntl_data) != -1) {
        return JITTRUE;
    }
    return JITFALSE;
}

JITINT32 Platform_FileMethods_SetLastWriteTime (void *path, JITINT64 ticks) {
    JITINT8         *literalUTF8;
    JITINT16        *literalUTF16;
    JITINT32 length;
    JITINT32 result;

    METHOD_BEGIN(ildjitSystem, "Platform.FileMethods.SetLastAccessTime");

    result = 0;   //		ILSysIOSetErrno(IL_ERRNO_ENOMEM);
    if (path) {
        length = (ildjitSystem->cliManager).CLR.stringManager.getLength(path);
        if (length > 0) {
            literalUTF16 = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(path);
            assert(literalUTF16 != NULL);
            literalUTF8 = allocMemory(length * sizeof(char));
            (ildjitSystem->cliManager).CLR.stringManager.fromUTF16ToUTF8(literalUTF16, literalUTF8, length);
            result = internal_SetModificationTime(literalUTF8, ticks);
            freeMemory(literalUTF8);
        }
    }

    METHOD_END(ildjitSystem, "Platform.FileMethods.SetLastAccessTime");
    return result;
}

static inline JITINT32 internal_SetModificationTime (JITINT8 *path, JITINT64 time) {
    JITINT32 retVal;
    JITINT64 unix_time;
    struct utimbuf utbuf;
    struct stat statbuf;

    /* Clear errno */
    errno = 0;

    unix_time = (time / (JITINT64) 10000000) - EPOCH_ADJUST;

    /* Grab the old time data first */
    retVal = PLATFORM_stat((const char *) path, &statbuf);

    if (retVal != 0) {
        /* Throw out the Errno */
        return Platform_FileMethods_GetErrno();
    }

    /* Copy over the old atime value */
    utbuf.actime = statbuf.st_atime;

    /* Set the new mod time */
    utbuf.modtime = unix_time;

    /* And write the inode */
    retVal = PLATFORM_utime((const char *) path, &utbuf);

    if (retVal != 0) {

        /* Throw out the errno */
        return Platform_FileMethods_GetErrno();
    }

    /* Return			*/
    return IL_ERRNO_Success;
}

JITINT32 Platform_FileMethods_SetLastAccessTime (void *path, JITINT64 ticks) {
    JITINT8         *literalUTF8;
    JITINT16        *literalUTF16;
    JITINT32 length;
    JITINT32 result;

    METHOD_BEGIN(ildjitSystem, "Platform.FileMethods.SetLastAccessTime");

    result = 0;    //		ILSysIOSetErrno(IL_ERRNO_ENOMEM);
    if (path) {
        length = (ildjitSystem->cliManager).CLR.stringManager.getLength(path);
        if (length > 0) {
            literalUTF16 = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(path);
            assert(literalUTF16 != NULL);
            literalUTF8 = allocMemory(length * sizeof(char));
            (ildjitSystem->cliManager).CLR.stringManager.fromUTF16ToUTF8(literalUTF16, literalUTF8, length);
            result = internal_SetAccessTime(literalUTF8, ticks);
            freeMemory(literalUTF8);
        }
    }

    METHOD_END(ildjitSystem, "Platform.FileMethods.SetLastAccessTime");
    return result;
}

static inline JITINT32 internal_SetAccessTime (JITINT8 *path, JITINT64 time) {
    JITINT32 retVal;
    JITINT64 unix_time;
    struct utimbuf utbuf;
    struct stat statbuf;

    /* Clear errno */
    errno = 0;

    unix_time = (time / (JITINT64) 10000000) - EPOCH_ADJUST;

    /* Grab the old time data first */
    retVal = PLATFORM_stat((const char *) path, &statbuf);

    if (retVal != 0) {
        /* Throw out the Errno */
        return Platform_FileMethods_GetErrno();
    }

    /* Copy over the old mtime value */
    utbuf.modtime = statbuf.st_mtime;

    /* Set the new actime time */
    utbuf.actime = unix_time;

    /* And write the inode */
    retVal = PLATFORM_utime((const char *) path, &utbuf);

    if (retVal != 0) {
        /* Throw out the errno */
        return Platform_FileMethods_GetErrno();
    }

    /* Return			*/
    return IL_ERRNO_Success;
}

JITINT32 Platform_FileMethods_SetCreationTime (void *path, JITINT64 ticks) {
    METHOD_BEGIN(ildjitSystem, "Platform.FileMethods.SetCreationTime");
    if (!path) {
//		ILSysIOSetErrno(IL_ERRNO_ENOMEM);
        return 0;
    }
    METHOD_END(ildjitSystem, "Platform.FileMethods.SetCreationTime");
    return IL_ERRNO_ENOSYS;
}


// Get the attributes on a file.
// Errno GetAttributes(string path, out int attrs);
JITINT32 Platform_FileMethods_GetAttributes (void *string_path, JITINT32 *atts) {
    JITINT8         *path;
    JITINT16        *literalUTF16;
    JITINT32 length;
    JITINT32 retVal;
    struct stat statbuf;
    JITINT32 mode;

    METHOD_BEGIN(ildjitSystem, "Platform.FileMethods.GetFileAttributes");

    /* Initialize the variables	*/
    path = NULL;

    if (string_path) {
        length = (ildjitSystem->cliManager).CLR.stringManager.getLength(string_path);
        if (length > 0) {
            literalUTF16 = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(string_path);
            assert(literalUTF16 != NULL);
            path = allocMemory((length + 1) * sizeof(char));
            (ildjitSystem->cliManager).CLR.stringManager.fromUTF16ToUTF8(literalUTF16, path, length);
        }
    }

    /* Clear errno */
    errno = 0;

    retVal = PLATFORM_stat((const char *) path, &statbuf);

    if (retVal != 0) {

        /* Throw out the Errno */
        return Platform_FileMethods_GetErrno();
    }

    switch (statbuf.st_mode & S_IFMT) {
        case S_IFBLK:
        case S_IFCHR:
            mode = ILFileAttributes_Device;
            break;
        case S_IFDIR:
            mode = ILFileAttributes_Directory;
            break;
        default:
            mode = 0;
            break;
    }

    if (!(statbuf.st_mode & 0200)) {
        mode |= ILFileAttributes_ReadOnly;
    }

    if (mode == 0) {
        mode = ILFileAttributes_Normal;
    }

    *atts = mode;
    //freeMemory(path);
    METHOD_END(ildjitSystem, "Platform.FileMethods.GetFileAttributes");
    return IL_ERRNO_Success;
}

// Set the attributes on a file.
// Errno SetAttributes(string path, int attrs);
JITINT32 Platform_FileMethods_SetAttributes (void* string_path, JITINT32 *atts) {
    JITINT8         *path;
    JITINT16        *literalUTF16;
    JITINT32 length;

    METHOD_BEGIN(ildjitSystem, "Platform.FileMethods.SetAttributes");

    /* Initialize the variables	*/
    literalUTF16 = NULL;
    path = NULL;

    if (string_path) {
        length = (ildjitSystem->cliManager).CLR.stringManager.getLength(string_path);
        if (length > 0) {
            literalUTF16 = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(string_path);
            assert(literalUTF16 != NULL);
            path = allocMemory(length * sizeof(char));
            (ildjitSystem->cliManager).CLR.stringManager.fromUTF16ToUTF8(literalUTF16, path, length);
        }
    }

    JITINT32 isReadOnly;
    JITINT32 retVal;
    struct stat statbuf;
    JITINT32 uMask;

    /* Clear errno */
    errno = 0;

    /* Grab the old attributes first */
    retVal = PLATFORM_stat((const char *) path, &statbuf);

    if (retVal != 0) {
        /* Throw out the Errno */
        METHOD_END(ildjitSystem, "Platform.FileMethods.SetFileAttributes");
        return Platform_FileMethods_GetErrno();
    }

    /*
     * The only mode we can change is readonly.  If it already
     * matches ILSysIOGetFileAttributes definition of ReadOnly,
     * then don't change it.
     */
    isReadOnly = (atts && ILFileAttributes_ReadOnly) != 0;
    if (((statbuf.st_mode && 0200) == 0) == isReadOnly) {
        METHOD_END(ildjitSystem, "Platform.FileMethods.SetFileAttributes");
    }
    return IL_ERRNO_Success;

    /*
     * Set the write bits accordingly.
     */
    if (isReadOnly) {
        statbuf.st_mode &= ~0222;
    } else {
        statbuf.st_mode |= 0222;
        /*
         * Don't set all write bits - that would be dangerous.
         * Respect the umask if there is one.
         */
        uMask = PLATFORM_umask(0);
        PLATFORM_umask(uMask);
        /*
         * Regardless of what umask() says we ill make GetAttributes()
         * respect the new setting.
         */
        uMask &= ~0200;
        statbuf.st_mode &= ~uMask;
    }

    retVal = chmod((const char *) path, statbuf.st_mode & 0xfff);

    if (retVal != 0) {

        /* Throw out the errno */
        METHOD_END(ildjitSystem, "Platform.FileMethods.SetFileAttributes");
        return Platform_FileMethods_GetErrno();
    }

    METHOD_END(ildjitSystem, "Platform.FileMethods.SetFileAttributes");
    return IL_ERRNO_Success;
}

// Get the length of a file.
// Errno GetLength(string path, out long length);
JITINT32 Platform_FileMethods_GetLength (void *string_path, JITINT64 *length) {
    JITINT8         *path;
    JITINT32 retVal;
    struct stat statbuf;

    METHOD_BEGIN(ildjitSystem, "Platform.FileMethods.GetLength");

    /* Initialize the variables	*/
    path = NULL;

    /* Fetch the path		*/
    path = (ildjitSystem->cliManager).CLR.stringManager.getUTF8copy(string_path);

    /* Read the file size           */
    errno = 0;
    retVal = PLATFORM_stat((const char *) path, &statbuf);
    if (retVal != 0) {

        /* Free the memory	*/
        freeMemory(path);

        /* Throw out the Errno  */
        METHOD_END(ildjitSystem, "Platform.FileMethods.GetLength");
        return Platform_FileMethods_GetErrno();
    }

    /*
     * The MS docs for System.IO.FileInfo.Length say a file not
     * found exception should be thrown if the file is a
     * directory.  I will interpret that to mean "anything bar a
     * regular file".
     */
    if ((statbuf.st_mode & S_IFMT) != S_IFREG) {

        /* Free the memory	*/
        freeMemory(path);

        /* Set the errno	*/
        errno = ENOENT;

        METHOD_END(ildjitSystem, "Platform.FileMethods.GetLength");
        return Platform_FileMethods_GetErrno();
    }

    /* Get the size */
    (*length) = statbuf.st_size;

    /* Free the memory	*/
    freeMemory(path);

    METHOD_END(ildjitSystem, "Platform.FileMethods.GetLength");
    return IL_ERRNO_Success;
}

// Read the contents of a symlink.  Returns "Errno.Success" and
// "null" in "contents" if the path exists but is not a symlink.
// Errno ReadLink(String path, out String contents);
JITINT32 Platform_FileMethods_ReadLink (void *string_path, void **contents) {
    JITINT8         *literalUTF8;
    JITINT8         *pathAnsi;
    JITINT16        *literalUTF16;
    JITINT32 length;
    JITINT8 buf[1024];
    JITINT32 len;

    METHOD_BEGIN(ildjitSystem, "Platform.FileMethods.ReadLink");

    /* Initialize the variables	*/
    literalUTF16 = NULL;
    literalUTF8 = NULL;
    pathAnsi = NULL;

    if (string_path) {
        length = (ildjitSystem->cliManager).CLR.stringManager.getLength(string_path);
        if (length > 0) {
            literalUTF16 = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(string_path);
            assert(literalUTF16 != NULL);
            literalUTF8 = allocMemory(length * sizeof(char));
            (ildjitSystem->cliManager).CLR.stringManager.fromUTF16ToUTF8(literalUTF16, literalUTF8, length);

            pathAnsi = literalUTF8;

            freeMemory(literalUTF8);
        }
    }

    if (!pathAnsi) {
        METHOD_END(ildjitSystem, "Platform.FileMethods.ReadLink");
        return IL_ERRNO_ENOMEM;
    }
    len = PLATFORM_readlink((const char *) pathAnsi, (char *) buf, sizeof(buf) - 1);
    if (len >= 0) {
        buf[len] = '\0';
        *contents = CLIMANAGER_StringManager_newInstanceFromUTF8(buf, strlen((char *) buf));
        METHOD_END(ildjitSystem, "Platform.FileMethods.ReadLink");
        return IL_ERRNO_Success;
    } else if (errno == EINVAL) {
        *contents = 0;
        METHOD_END(ildjitSystem, "Platform.FileMethods.ReadLink");
        return IL_ERRNO_Success;
    }

    (*contents) = 0;

    /* Return			*/
    METHOD_END(ildjitSystem, "Platform.FileMethods.ReadLink");
    return Platform_FileMethods_GetErrno();
}

/*
 * DirMethods....................................................................................................................................
 */
/* Get info about paths on running ildjitSystem */
void Platform_DirMethods_GetPathInfo (void *pathInfo) {

    /* Running garbage collector */
    t_running_garbage_collector* garbageCollector;

    /* The array manager */
    t_arrayManager* arrayManager;

    /* System.Char description */
    TypeDescriptor* charType;

    /* Platform.PathInfo description */
    TypeDescriptor *pathInfoType;

    /* A Platform.PathInfo field description */
    FieldDescriptor *field;

    /* Offset of a Platform.PathInfo field */
    JITINT32 fieldOffset;

    /* Os specific info about paths */
    ILPathInfo rawPathInfo;

    /* An array of invalid path chars */
    void* invalidPathChars;

    /* Number of invalid path chars on current os */
    JITUINT32 invalidCharsCount;

    /* Loop counter */
    JITUINT32 i;

    /* Cache some pointers */
    garbageCollector = &ildjitSystem->garbage_collectors.gc;
    arrayManager = &((ildjitSystem->cliManager).CLR.arrayManager);
    METHOD_BEGIN(ildjitSystem, "Platform.DirMethods.GetPathInfo()");

    /* Get the path info from the underlying os */
    ILGetPathInfo(&rawPathInfo);

    /* Build a new System.Char array */
    charType                =  (ildjitSystem->cliManager).metadataManager->getTypeDescriptorFromElementType((ildjitSystem->cliManager).metadataManager, ELEMENT_TYPE_CHAR);
    assert(charType != NULL);
    invalidCharsCount = strlen(rawPathInfo.invalidPathChars);
    invalidPathChars = garbageCollector->allocArray(charType, 1, invalidCharsCount);

    /* Fill the array with invalid chars */
    for (i = 0; i < invalidCharsCount; i++) {
        arrayManager->setValueArrayElement(invalidPathChars, i, (void *) &(rawPathInfo.invalidPathChars[i]));
    }

    /* Fetch the Platform.PathInfo CIL type */
    pathInfoType = (ildjitSystem->cliManager).metadataManager->getTypeDescriptorFromName((ildjitSystem->cliManager).metadataManager, (JITINT8 *) "Platform", (JITINT8 *) "PathInfo");

    /* Copy the dirSeparator field */
    field = pathInfoType->getFieldFromName(pathInfoType, (JITINT8 *) "dirSeparator");
    fieldOffset = garbageCollector->fetchFieldOffset(field);
    *((JITINT16*) (pathInfo + fieldOffset)) = rawPathInfo.dirSep;

    /* Copy the altDirSeparator field */
    field = pathInfoType->getFieldFromName(pathInfoType, (JITINT8 *) "altDirSeparator");
    fieldOffset = garbageCollector->fetchFieldOffset(field);
    *((JITINT16*) (pathInfo + fieldOffset)) = rawPathInfo.altDirSep;

    /* Copy the volumeSeparator field */
    field = pathInfoType->getFieldFromName(pathInfoType, (JITINT8 *) "volumeSeparator");
    fieldOffset = garbageCollector->fetchFieldOffset(field);
    *((JITINT16*) (pathInfo + fieldOffset)) = rawPathInfo.volumeSep;

    /* Copy the pathSeparator field */
    field = pathInfoType->getFieldFromName(pathInfoType, (JITINT8 *) "pathSeparator");
    fieldOffset = garbageCollector->fetchFieldOffset(field);
    *((JITINT16*) (pathInfo + fieldOffset)) = rawPathInfo.pathSep;

    /* Copy the invalidPathChars field */
    field = pathInfoType->getFieldFromName(pathInfoType, (JITINT8 *) "invalidPathChars");
    fieldOffset = garbageCollector->fetchFieldOffset(field);
    (*((void**) (pathInfo + fieldOffset))) = invalidPathChars;

    /* Return				*/
    METHOD_END(ildjitSystem, "Platform.DirMethods.GetPathInfo()");
    return;
}

JITINT32 Platform_DirMethods_GetLastAccess (void *c_path, JITINT64 *lastac) {
    JITINT32 result;
    JITINT32 length;
    JITINT8         *literalUTF8;
    JITINT16        *literalUTF16;

    /*Initialize		*/
    result = IL_ERRNO_ENOMEM;

    METHOD_BEGIN(ildjitSystem, "Platform.DirMethods.GetLastAccess(char [], long)");

    /* Validate                     */
    if (c_path) {
        length = (ildjitSystem->cliManager).CLR.stringManager.getLength(c_path);
        if (length > 0) {
            literalUTF16 = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(c_path);
            assert(literalUTF16 != NULL);
            literalUTF8 = allocMemory(length + 1);
            (ildjitSystem->cliManager).CLR.stringManager.fromUTF16ToUTF8(literalUTF16, literalUTF8, length);

            /* 0 for last_access mode*/
            result = internal_GetLastAccModCreTime(literalUTF8, lastac, 0);
            freeMemory(literalUTF8);
        }
    }
    /* Return                       */
    METHOD_END(ildjitSystem, "Platform.DirMethods.GetLastAccess(char [], long)");
    return result;
}

JITINT32 Platform_DirMethods_GetLastModification (void *c_path, JITINT64 *last_mod) {
    JITINT32 result;
    JITINT32 length;
    JITINT8         *literalUTF8;
    JITINT16        *literalUTF16;

    result = IL_ERRNO_ENOMEM;

    METHOD_BEGIN(ildjitSystem, "Platform.DirMethods.GetLastModification(char [], long)");

    /* Validate                     */
    if (c_path) {
        length = (ildjitSystem->cliManager).CLR.stringManager.getLength(c_path);
        if (length > 0) {
            literalUTF16 = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(c_path);
            assert(literalUTF16 != NULL);
            literalUTF8 = allocMemory(length + 1);
            (ildjitSystem->cliManager).CLR.stringManager.fromUTF16ToUTF8(literalUTF16, literalUTF8, length);

            /* 1 for last_modification mode*/
            result = internal_GetLastAccModCreTime(literalUTF8, last_mod, 1);
            freeMemory(literalUTF8);
        }
    }
    /* Return                       */
    METHOD_END(ildjitSystem, "Platform.DirMethods.GetLastModification(char [], long)");
    return result;
}

JITINT32 Platform_DirMethods_GetCreationTime (void *c_path, JITINT64 *create_time) {
    JITINT32 result;
    JITINT32 length;
    JITINT8         *literalUTF8;
    JITINT16        *literalUTF16;

    /* Initialization                       */
    result = IL_ERRNO_ENOMEM;

    METHOD_BEGIN(ildjitSystem, "Platform.DirMethods.GetCreationTime(char [], long)");

    /* Validate                     */
    if (c_path) {
        length = (ildjitSystem->cliManager).CLR.stringManager.getLength(c_path);
        if (length > 0) {
            literalUTF16 = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(c_path);
            assert(literalUTF16 != NULL);
            literalUTF8 = allocMemory(length + 1);
            (ildjitSystem->cliManager).CLR.stringManager.fromUTF16ToUTF8(literalUTF16, literalUTF8, length);

            /* 2 for creation_time mode*/
            result = internal_GetLastAccModCreTime(literalUTF8, create_time, 2);
            freeMemory(literalUTF8);
        }
    }
    /* Return                       */
    METHOD_END(ildjitSystem, "Platform.DirMethods.GetCreationTime(char [], long)");
    return result;
}

JITINT32 Platform_DirMethods_Rename (void *old_name, void *new_name) {
    JITINT32 result;
    JITINT8         *old_literalUTF8;
    JITINT8         *new_literalUTF8;

    METHOD_BEGIN(ildjitSystem, "Platform.DirMethods.Rename(char [],char [])");

    /* Validate                     */
    if ((old_name) && (new_name) ) {
        old_literalUTF8 = (ildjitSystem->cliManager).CLR.stringManager.getUTF8copy(old_name);
        new_literalUTF8 = (ildjitSystem->cliManager).CLR.stringManager.getUTF8copy(new_name);
        result = internal_RenameDir(old_literalUTF8, new_literalUTF8);

        /* Free the memory		*/
        freeMemory(old_literalUTF8);
        freeMemory(new_literalUTF8);

        /* Return                       */
        METHOD_BEGIN(ildjitSystem, "Platform.DirMethods.Rename(char [],char [])");
        return result;
    }
    /* Return                       */
    METHOD_BEGIN(ildjitSystem, "Platform.DirMethods.Rename(char [],char [])");
    return IL_ERRNO_ENOMEM;
}

JITINT32 Platform_DirMethods_ChangeDirectory (void *new_path) {
    JITINT32 length;
    JITINT32 result;
    JITINT8         *literalUTF8;
    JITINT16        *literalUTF16;

    METHOD_BEGIN(ildjitSystem, "Platform.DirMethods.ChangeDirectory(char [])");

    /* Validate                     */
    result = IL_ERRNO_ENOENT;
    if (new_path != NULL) {
        length = (ildjitSystem->cliManager).CLR.stringManager.getLength(new_path);
        if (length > 0) {
            length++;
            literalUTF16 = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(new_path);
            assert(literalUTF16 != NULL);
            literalUTF8 = allocMemory(length * sizeof(char));
            (ildjitSystem->cliManager).CLR.stringManager.fromUTF16ToUTF8(literalUTF16, literalUTF8, length - 1);
            result = internal_ChangeDir(literalUTF8);

            /* Free the memory		*/
            freeMemory(literalUTF8);
        }
    }

    /* Return                       */
    METHOD_END(ildjitSystem, "Platform.DirMethods.ChangeDirectory(char [])");
    return result;
}

JITINT32 Platform_DirMethods_CreateDirectory (void *string_path) {
    JITINT32 result;
    JITINT32 length;
    JITINT8         *literalUTF8;
    JITINT16        *literalUTF16;

    METHOD_BEGIN(ildjitSystem, "Platform.DirMethods.CreateDirectory(char [])");


    result = IL_ERRNO_ENOENT;
    if (string_path) {
        length = (ildjitSystem->cliManager).CLR.stringManager.getLength(string_path);
        if (length > 0) {
            literalUTF16 = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(string_path);
            assert(literalUTF16 != NULL);
            literalUTF8 = allocMemory(length + 1);
            (ildjitSystem->cliManager).CLR.stringManager.fromUTF16ToUTF8(literalUTF16, literalUTF8, length);
            result = internal_CreateDir(literalUTF8);
            freeMemory(literalUTF8);
        }
    }

    /* Return                       */
    METHOD_END(ildjitSystem, "Platform.DirMethods.CreateDirectory(char [])");
    return result;
}

JITINT32 Platform_DirMethods_Delete (void *string_path) {
    JITINT32 result;
    JITINT32 length;
    JITINT8         *literalUTF8;

    METHOD_BEGIN(ildjitSystem, "Platform.DirMethods.Delete(char [])");

    result = -1;
    if (string_path != NULL) {
        length = (ildjitSystem->cliManager).CLR.stringManager.getLength(string_path);
        if (length > 0) {
            literalUTF8 = (ildjitSystem->cliManager).CLR.stringManager.getUTF8copy(string_path);
            result = internal_DeleteDir(literalUTF8);
            freeMemory(literalUTF8);
        } else {
            result = IL_ERRNO_ENOENT;
        }
    } else {
        result = IL_ERRNO_ENOENT;
    }

    /* Return                       */
    METHOD_END(ildjitSystem, "Platform.DirMethods.Delete(char [])");
    return result;
}

void * Platform_DirMethods_GetCurrentDirectory (void) {
    JITINT8         *name;
    void            *object;

    METHOD_BEGIN(ildjitSystem, "Platform.DirMethods.GetCurrentDirectory");

    /* Initialize the variables	*/
    object = NULL;
    name = NULL;

    name = (JITINT8 *) PLATFORM_get_current_dir_name();
    if (name != NULL) {
        object = CLIMANAGER_StringManager_newInstanceFromUTF8(name, strlen((char *) name));

        /* Free the memory	*/
        freeMemory(name);
    }

    /* Return                       */
    METHOD_END(ildjitSystem, "Platform.DirMethods.GetCurrentDirectory");
    return object;
}

void * Platform_DirMethods_GetSystemDirectory (void) {

    METHOD_BEGIN(ildjitSystem, "Platform.DirMethods.GetCurrentDirectory");

    /* We don't support ildjitSystem directories on this platform */
    METHOD_END(ildjitSystem, "Platform.DirMethods.GetCurrentDirectory");
    return 0;
}

void * Platform_DirMethods_GetLogicalDrives (void) {

    METHOD_BEGIN(ildjitSystem, "Platform.DirMethods.GetLogicalDrives");

    /* TODO: handle Windows drive lists */
    METHOD_END(ildjitSystem, "Platform.DirMethods.GetLogicalDrives");
    return 0;
}

/*
 * InfoMethods............................................................................................................................
 */
void *Platform_InfoMethods_GetNetBIOSMachineName () {
    JITINT8         *env;
    JITINT8 hostName[DIM_BUF + 1];
    void            *object;

    METHOD_BEGIN(ildjitSystem, "Platform.InfoMethodos.GetNetBIOSMachineName");

    /* Initialized				*/
    object = NULL;

    if (PLATFORM_gethostname((char *) hostName, DIM_BUF) == -1) {
        object = 0;   /* error */
    }
    object = CLIMANAGER_StringManager_newInstanceFromUTF8(hostName, strlen((char *) hostName));

    env = (JITINT8 *) getenv("COMPUTERNAME");
    if (env && *env != '\0') {
        object = CLIMANAGER_StringManager_newInstanceFromUTF8(env, strlen((char *) env));
    }

    /* Return					*/
    METHOD_END(ildjitSystem, "Platform.InfoMethodos.GetNetBIOSMachineName");
    return object;
}

JITINT32 Platform_InfoMethods_GetProcessorCount () {

    METHOD_BEGIN(ildjitSystem, "Platform.InfoMethodos.GetProcessorCount");

    METHOD_END(ildjitSystem, "Platform.InfoMethodos.GetProcessorCount");

    /* Simulate a uniprocessor machine              */
    return 1;
}

JITINT64 Platform_InfoMethods_GetWorkingSet () {

    METHOD_BEGIN(ildjitSystem, "Platform.InfoMethodos.GetWorkingSet");

    METHOD_END(ildjitSystem, "Platform.InfoMethodos.GetWorkingSet");
    /* There is no reliable way to determine the working set */
    return 0;
}

JITINT32 Platform_InfoMethods_GetPlatformID () {

    METHOD_BEGIN(ildjitSystem, "Platform.InfoMethodos.GetPlatformID");

    /* PlatformID.Unix */
    METHOD_END(ildjitSystem, "Platform.InfoMethodos.GetPlatformID");
    return 128;
}

JITBOOLEAN Platform_InfoMethods_IsUserInteractive () {

    METHOD_BEGIN(ildjitSystem, "Platform.InfoMethodos.IsUserInteractive");

    /* Return			*/
    METHOD_END(ildjitSystem, "Platform.InfoMethodos.IsUserInteractive");
    return PLATFORM_isatty(0) && PLATFORM_isatty(1);
}

void * Platform_InfoMethods_GetUserStorageDir () {
    JITINT8         *env;
    JITINT8         *full;
    void            *str;

    METHOD_BEGIN(ildjitSystem, "Platform.InfoMethodos.GetUserStorageDir");

    /* Try the "CLI_STORAGE_ROOT" environment variable first */
    if ((env = (JITINT8 *) getenv("CLI_STORAGE_ROOT")) != 0 && *env != '\0') {
        METHOD_END(ildjitSystem, "Platform.InfoMethodos.GetUserStorageDir");
        return CLIMANAGER_StringManager_newInstanceFromUTF8(env, strlen((char *) env));
    }

    /* Use "$HOME/.cli" instead */
    env = (JITINT8 *) getenv("HOME");
    if (     (env != NULL)                                                   &&
             ((*env) != '\0')                                                &&
             (Platform_FileMethods_GetFileType(env) == ILFileType_DIR)       ) {
        full = allocMemory((strlen((char *) env) + 6));
        if (!full) {
            ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "OutOfMemoryException");
            METHOD_END(ildjitSystem, "Platform.InfoMethodos.GetUserStorageDir");
            return 0;
        }
        strcpy((char *) full, (char *) env);
        strcat((char *) full, "/.cli");
        str = CLIMANAGER_StringManager_newInstanceFromUTF8(full, strlen((char *) full));
        freeMemory(full);
        METHOD_END(ildjitSystem, "Platform.InfoMethodos.GetUserStorageDir");
        return str;
    }

    /* We don't know how to get the user storage directory */
    METHOD_END(ildjitSystem, "Platform.InfoMethodos.GetUserStorageDir");
    return 0;
}

void * Platform_InfoMethods_GetGlobalConfigDir () {
    JITINT8         *env;
    void            *str;

    METHOD_BEGIN(ildjitSystem, "Platform.InfoMethodos.GetGlobalConfigDir");

    /* Try the "CLI_MACHINE_CONFIG_DIR" environment variable first */
    env = (JITINT8 *) getenv("CLI_MACHINE_CONFIG_DIR");
    if ((env != NULL) && (*env) != '\0') {
        METHOD_END(ildjitSystem, "Platform.InfoMethodos.GetGlobalConfigDir");
        return CLIMANAGER_StringManager_newInstanceFromUTF8(env, STRLEN(env));
    }
    /* Return a standard path such as "/usr/local/share/cscc/config" */
    env = internal_GetFileInBasePath("/share/", "/cscc/config");
    if (!env) {
        ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "OutOfMemoryException");
        METHOD_END(ildjitSystem, "Platform.InfoMethodos.GetGlobalConfigDir");
        return 0;
    }
    str = CLIMANAGER_StringManager_newInstanceFromUTF8(env, STRLEN(env));
    freeMemory(env);
    METHOD_END(ildjitSystem, "Platform.InfoMethodos.GetGlobalConfigDir");
    return str;
}

void * Platform_InfoMethods_GetPlatformName () {
    void            *str;

    METHOD_BEGIN(ildjitSystem, "Platform.InfoMethodos.GetPlatformName");

#ifdef CSCC_HOST_TRIPLET
    str = CLIMANAGER_StringManager_newInstanceFromUTF8(CSCC_HOST_TRIPLET, strlen(CSCC_HOST_TRIPLET));
#else
    str = CLIMANAGER_StringManager_newInstanceFromUTF8((JITINT8 *) "\0", strlen("\0"));
#endif

    /* Return			*/
    METHOD_END(ildjitSystem, "Platform.InfoMethodos.GetPlatformName");
    return str;
}

void * Platform_InfoMethods_GetUserName () {
    void            *str;

    METHOD_BEGIN(ildjitSystem, "Platform.InfoMethodos.GetUserName");

    //FIXME    capire se esiste tale funzioni in iljit
    if (0) { //!ILImageIsSecure(_ILClrCallerImage(thread)))
        /* We don't trust the caller, so don't tell them who the user is
           str =  CLIMANAGER_StringManager_newInstanceFromUTF8("nobody", strlen("nobody"));	*/
    } else {
        JITINT8         *env;
        // struct passwd *pwd = PLATFORM_getUserPassword(); // PLATFORM_getUserPassword() not present in libplatform API by Win security limitations.
        /*if (pwd != NULL) {
                //FIXME pwd->pw_name non risulta come stringa
                //return  CLIMANAGER_StringManager_newInstanceFromUTF8(pwd->pw_name, strlen(pwd->pw_name));
           }*/
        env = (JITINT8 *) getenv("USER");
        if (     (env != NULL)           &&
                 ((*env) != '\0')        ) {
            return CLIMANAGER_StringManager_newInstanceFromUTF8(env, strlen((char *) env));
        }
        return CLIMANAGER_StringManager_newInstanceFromUTF8((JITINT8 *) "nobody", strlen("nobody"));
    }
    /* Return			*/
    METHOD_END(ildjitSystem, "Platform.InfoMethodos.GetUserName");
    return str;
}

/*
 * internal_methods............................................................................................................................
 */
static inline JITINT32 internal_getFileType (JITINT8 *path) {
    struct stat st;

    /* Assertions                   */
    assert(path != NULL);

    if (PLATFORM_lstat((char *) path, &st) >= 0) {
        if (S_ISFIFO(st.st_mode)) {
            return ILFileType_FIFO;
        } else if (S_ISCHR(st.st_mode)) {
            return ILFileType_CHR;
        } else if (S_ISDIR(st.st_mode)) {
            return ILFileType_DIR;
        } else if (S_ISBLK(st.st_mode)) {
            return ILFileType_BLK;
        } else if (S_ISREG(st.st_mode)) {
            return ILFileType_REG;
        } else if (S_ISLNK(st.st_mode)) {
            return ILFileType_LNK;
        } else if (S_ISSOCK(st.st_mode)) {
            return ILFileType_SOCK;
        }
    }
    return ILFileType_Unknown;
}

JITNINT internal_OpenFile (const char *path, JITINT32 mode, JITINT32 access, JITINT32 share) {
    JITINT32 result;
    JITINT32 newAccess;

    /* Assertions                   */
    assert(path != NULL);

    switch (access) {
        case ILFileAccess_Read:
            newAccess = O_RDONLY;
            break;
        case ILFileAccess_Write:
            newAccess = O_WRONLY;
            break;
        case ILFileAccess_ReadWrite:
            newAccess = O_RDWR;
            break;

        default: {
            errno = EACCES;
            return (JITNINT) (-1);
        }
        /* Not reached */
    }

    switch (mode) {
        case ILFileMode_CreateNew: {
            /* Create a file only if it doesn't already exist */
            result = PLATFORM_open(path, newAccess | O_CREAT | O_EXCL, 0666);
        }
        break;

        case ILFileMode_Create: {
            /* Open in create/truncate mode */
            result = PLATFORM_open(path, newAccess | O_CREAT | O_TRUNC, 0666);
        }
        break;

        case ILFileMode_Open: {
            /* Open the file in the regular mode */
            result = PLATFORM_open(path, newAccess, 0666);
        }
        break;

        case ILFileMode_OpenOrCreate: {
            /* Open an existing file or create a new one */
            result = PLATFORM_open(path, newAccess | O_CREAT, 0666);
        }
        break;

        case ILFileMode_Truncate: {
            /* Truncate an existing file */
            result = PLATFORM_open(path, newAccess | O_TRUNC, 0666);
        }
        break;

        case ILFileMode_Append: {
            /* Open in append mode */
            result = PLATFORM_open(path, newAccess | O_CREAT | O_APPEND, 0666);
        }
        break;

        default: {
            /* We have no idea what this mode is */
            result = -1;
            errno = EACCES;
        }
        break;
    }
    if (    (result != -1)  &&
            (result != 0)   ){
        file_t  *f;
        f                   = allocFunction(sizeof(file_t));
        f->fileDescriptor   = result;
        xanHashTable_insert(filesOpened, (void *)(JITNUINT)(f->fileDescriptor), (void *)f);
    }

    return (JITNINT) result;
}

JITINT32 internal_Write (JITNINT handle, void *buf, JITINT32 size) {
    JITINT32 written = 0;
    JITINT32 result = 0;

    while (size > 0) {

        /* Write as much as we can, and retry if ildjitSystem call was interrupted */
        result = PLATFORM_write((int) (JITNINT) handle, buf, (JITINT32) size);
        if (result >= 0) {
            written += result;
            size -= result;
            buf += result;
        } else if (errno != EINTR) {
            break;
        }
    }

    if (written > 0) {
        return written;
    }

    return (result < 0) ? -1 : 0;
}

JITBOOLEAN internal_CheckHandleAccess (JITNINT handle, JITINT32 access) {
#if defined(HAVE_FCNTL) && defined(F_GETFL)
    JITINT32 flags = fcntl((JITINT32) handle, F_GETFL, 0);
    if (flags != -1) {
        switch (access) {
            case ILFileAccess_Read:
                return (flags & O_RDONLY) != 0;
            case ILFileAccess_Write:
                return (flags & O_WRONLY) != 0;
            case ILFileAccess_ReadWrite:
                return (flags & O_RDWR) == O_RDWR;
        }
    }
    return JITFALSE;
#else
    return JITFALSE;
#endif
}

static inline JITINT64 internal_Seek (JITNINT handle, JITINT64 offset, JITINT32 whence) {
    JITINT64 result;

    while ((result = (JITINT64) (PLATFORM_lseek((JITINT32) (JITNINT) handle, (off_t) offset, whence))) == (JITINT64) (-1)) {
        /* Retry if the ildjitSystem call was interrupted */
        if (errno != EINTR) {
            break;
        }
    }
    return result;
}

static inline JITINT32 internal_CreateDir (JITINT8 *path) {
    if (!path) {
        return IL_ERRNO_ENOENT;
    }
    if (PLATFORM_mkdir((const char *) path, 0777) == 0) {
        return IL_ERRNO_Success;
    }
    return ILSysIOConvertErrno(errno);
}

static inline JITINT32 internal_DeleteDir (JITINT8 *path) {
    if (!path) {
        return IL_ERRNO_ENOENT;
    }
    if (PLATFORM_rmdir((JITINT8 *) path) == 0) {
        return IL_ERRNO_Success;
    }
    if (PLATFORM_unlink((char *) path) == 0) {
        return IL_ERRNO_Success;
    }
    return ILSysIOConvertErrno(errno);
}

static inline JITINT32 internal_ChangeDir (JITINT8 *path) {

    /* Assertions                   */
    assert(path != NULL);

    if (!path) {
        return IL_ERRNO_ENOENT;
    }
    if (PLATFORM_chdir((const char *) path) == 0) {
        return IL_ERRNO_Success;
    }
    return ILSysIOConvertErrno(errno);
}

static inline JITINT32 internal_RenameDir (JITINT8 *old_path, JITINT8 *new_path) {

    /* Assertions                   */
    assert(old_path != NULL);
    assert(new_path != NULL);

    if (!old_path || !new_path) {
        return IL_ERRNO_ENOENT;
    }

    if (rename((char *) old_path, (char *) new_path) == 0) {
        return IL_ERRNO_Success;
    }
    return ILSysIOConvertErrno(errno);
}

static inline JITINT8 *GetBasePath (void) {
    return (JITINT8 *) ILDJIT_getPrefix();
}

static inline JITINT32 internal_Copy (JITINT8 *src, JITINT8 *dest) {
    FILE            *infile;
    FILE            *outfile;
    JITINT8 buffer[BUFSIZ];
    JITINT32 len;

    /* Bail out if either of the arguments is invalid */
    if (!src || !dest) {
        return IL_ERRNO_ENOENT;
    }

    /* Open the input file          */
    if ((infile = fopen((char *) src, "rb")) == NULL) {
        if ((infile = fopen((char *) src, "r")) == NULL) {
            return ILSysIOConvertErrno(errno);
        }
    }

    /* Open the output file         */
    if ((outfile = fopen((char *) dest, "wb")) == NULL) {
        if ((outfile = fopen((char *) dest, "w")) == NULL) {
            int error = ILSysIOConvertErrno(errno);
            fclose(infile);
            return error;
        }
    }

    /* Copy the file contents */
    while ((len = (int) fread(buffer, 1, sizeof(buffer), infile)) > 0) {
        fwrite(buffer, 1, len, outfile);
        if (len < sizeof(buffer)) {
            break;
        }
    }

    /* Close the files and exit     */
    fclose(infile);
    fclose(outfile);

    /* Return			*/
    return 0;
}
/*
 * la stessa sia per i file che per Dir
 */
static inline JITINT32 internal_GetLastAccModCreTime (JITINT8 *path, JITINT64 *time, JITINT32 Mode) {
    JITINT32 err;
    struct stat buf;

    /* Assertions                   */
    assert(path != NULL);

    if (!(err = PLATFORM_stat((const char *) path, &buf))) {
        switch (Mode) {
            case 0:
                *time = (buf.st_atime + EPOCH_ADJUST) * (JITINT64) 10000000;
                break;
            case 1:
                *time = (buf.st_mtime + EPOCH_ADJUST) * (JITINT64) 10000000;
                break;
            case 2:
                *time = (buf.st_ctime + EPOCH_ADJUST) * (JITINT64) 10000000;
                break;
        }

    } else {
        err = Platform_FileMethods_GetErrno();
    }
    return err;
}

/*
 * Get a file or directory, relative to the installation base path.
 */
static JITINT8 * internal_GetFileInBasePath (const char *tail1, const char *tail2) {
    JITINT8         *base;
    JITINT8         *temp;
    JITINT32 baselen;
    JITINT32 len;

    /* Get the base path for the program installation */
    base = GetBasePath();
    if (!base) {
        return NULL;
    }
    baselen = STRLEN(base);

    /* Allocate additional space for the rest of the path */
    len = baselen + (tail1 ? strlen(tail1) : 0) + (tail2 ? strlen(tail2) : 0);
    temp = reallocMemory(base, len + 1);
    if (!temp) {
        return NULL;
    }
    base = temp;

    /* Construct the final path, normalizing path separators as we go */
    len = baselen;
    while (tail1 != 0 && *tail1 != '\0') {
        if (*tail1 == '/') {
            base[len++] = DIR_SEP;
        } else {
            base[len++] = *tail1;
        }
        ++tail1;
    }
    while (tail2 != 0 && *tail2 != '\0') {
        if (*tail2 == '/') {
            base[len++] = DIR_SEP;
        } else {
            base[len++] = *tail2;
        }
        ++tail2;
    }
    base[len] = '\0';
    return base;
}

static inline void ILGetPathInfo (ILPathInfo *info) {
#if defined(IL_WIN32_NATIVE)
    info->dirSep = '\\';
    info->altDirSep = '/';
    info->volumeSep = ':';
    info->pathSep = ';';
    info->invalidPathChars = "\"<>|\r\n";
#elif defined(IL_WIN32_CYGWIN)
    info->dirSep = '/';
    info->altDirSep = '\\';
    info->volumeSep = 0;
    info->pathSep = ':';
    info->invalidPathChars = "\"<>|\r\n";
#else
    info->dirSep = '/';
    info->altDirSep = 0;
    info->volumeSep = 0;
    info->pathSep = ':';
    info->invalidPathChars = "\r\n";
#endif
}

JITINT32 internal_fileExists (JITINT8 *fileName) {
    struct stat st;

    if (fileName == NULL) {
        return JITFALSE;
    }
    memset(&st, 0, sizeof(struct stat));
    if (PLATFORM_stat((const char *) fileName, &st) >= 0) {
        return JITTRUE;
    }
    return JITFALSE;
}


/**********************************************************************************************************************************************/
#ifndef IL_ERRNO_EPERM
#define IL_ERRNO_EPERM IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ENOENT
#define IL_ERRNO_ENOENT IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ESRCH
#define IL_ERRNO_ESRCH IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EINTR
#define IL_ERRNO_EINTR IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EIO
#define IL_ERRNO_EIO IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ENXIO
#define IL_ERRNO_ENXIO IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_E2BIG
#define IL_ERRNO_E2BIG IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ENOEXEC
#define IL_ERRNO_ENOEXEC IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EBADF
#define IL_ERRNO_EBADF IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ECHILD
#define IL_ERRNO_ECHILD IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EAGAIN
#define IL_ERRNO_EAGAIN IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ENOMEM
#define IL_ERRNO_ENOMEM IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EACCES
#define IL_ERRNO_EACCES IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EFAULT
#define IL_ERRNO_EFAULT IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ENOTBLK
#define IL_ERRNO_ENOTBLK IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EBUSY
#define IL_ERRNO_EBUSY IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EEXIST
#define IL_ERRNO_EEXIST IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EXDEV
#define IL_ERRNO_EXDEV IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ENODEV
#define IL_ERRNO_ENODEV IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ENOTDIR
#define IL_ERRNO_ENOTDIR IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EISDIR
#define IL_ERRNO_EISDIR IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EINVAL
#define IL_ERRNO_EINVAL IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ENFILE
#define IL_ERRNO_ENFILE IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EMFILE
#define IL_ERRNO_EMFILE IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ENOTTY
#define IL_ERRNO_ENOTTY IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ETXTBSY
#define IL_ERRNO_ETXTBSY IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EFBIG
#define IL_ERRNO_EFBIG IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ENOSPC
#define IL_ERRNO_ENOSPC IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ESPIPE
#define IL_ERRNO_ESPIPE IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EROFS
#define IL_ERRNO_EROFS IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EMLINK
#define IL_ERRNO_EMLINK IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EPIPE
#define IL_ERRNO_EPIPE IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EDOM
#define IL_ERRNO_EDOM IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ERANGE
#define IL_ERRNO_ERANGE IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EDEADLK
#define IL_ERRNO_EDEADLK IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ENAMETOOLONG
#define IL_ERRNO_ENAMETOOLONG IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ENOLCK
#define IL_ERRNO_ENOLCK IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ENOSYS
#define IL_ERRNO_ENOSYS IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ENOTEMPTY
#define IL_ERRNO_ENOTEMPTY IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ELOOP
#define IL_ERRNO_ELOOP IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ENOMSG
#define IL_ERRNO_ENOMSG IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EIDRM
#define IL_ERRNO_EIDRM IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ECHRNG
#define IL_ERRNO_ECHRNG IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EL2NSYNC
#define IL_ERRNO_EL2NSYNC IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EL3HLT
#define IL_ERRNO_EL3HLT IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EL3RST
#define IL_ERRNO_EL3RST IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ELNRNG
#define IL_ERRNO_ELNRNG IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EUNATCH
#define IL_ERRNO_EUNATCH IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ENOCSI
#define IL_ERRNO_ENOCSI IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EL2HLT
#define IL_ERRNO_EL2HLT IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EBADE
#define IL_ERRNO_EBADE IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EBADR
#define IL_ERRNO_EBADR IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EXFULL
#define IL_ERRNO_EXFULL IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ENOANO
#define IL_ERRNO_ENOANO IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EBADRQC
#define IL_ERRNO_EBADRQC IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EBADSLT
#define IL_ERRNO_EBADSLT IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EBFONT
#define IL_ERRNO_EBFONT IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ENOSTR
#define IL_ERRNO_ENOSTR IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ENODATA
#define IL_ERRNO_ENODATA IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ETIME
#define IL_ERRNO_ETIME IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ENOSR
#define IL_ERRNO_ENOSR IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ENONET
#define IL_ERRNO_ENONET IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ENOPKG
#define IL_ERRNO_ENOPKG IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EREMOTE
#define IL_ERRNO_EREMOTE IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ENOLINK
#define IL_ERRNO_ENOLINK IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EADV
#define IL_ERRNO_EADV IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ESRMNT
#define IL_ERRNO_ESRMNT IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ECOMM
#define IL_ERRNO_ECOMM IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EPROTO
#define IL_ERRNO_EPROTO IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EMULTIHOP
#define IL_ERRNO_EMULTIHOP IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EDOTDOT
#define IL_ERRNO_EDOTDOT IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EBADMSG
#define IL_ERRNO_EBADMSG IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EOVERFLOW
#define IL_ERRNO_EOVERFLOW IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ENOTUNIQ
#define IL_ERRNO_ENOTUNIQ IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EBADFD
#define IL_ERRNO_EBADFD IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EREMCHG
#define IL_ERRNO_EREMCHG IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ELIBACC
#define IL_ERRNO_ELIBACC IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ELIBBAD
#define IL_ERRNO_ELIBBAD IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ELIBSCN
#define IL_ERRNO_ELIBSCN IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ELIBMAX
#define IL_ERRNO_ELIBMAX IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ELIBEXEC
#define IL_ERRNO_ELIBEXEC IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EILSEQ
#define IL_ERRNO_EILSEQ IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ERESTART
#define IL_ERRNO_ERESTART IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ESTRPIPE
#define IL_ERRNO_ESTRPIPE IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EUSERS
#define IL_ERRNO_EUSERS IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ENOTSOCK
#define IL_ERRNO_ENOTSOCK IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EDESTADDRREQ
#define IL_ERRNO_EDESTADDRREQ IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EMSGSIZE
#define IL_ERRNO_EMSGSIZE IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EPROTOTYPE
#define IL_ERRNO_EPROTOTYPE IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ENOPROTOOPT
#define IL_ERRNO_ENOPROTOOPT IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EPROTONOSUPPORT
#define IL_ERRNO_EPROTONOSUPPORT IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ESOCKTNOSUPPORT
#define IL_ERRNO_ESOCKTNOSUPPORT IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EOPNOTSUPP
#define IL_ERRNO_EOPNOTSUPP IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EPFNOSUPPORT
#define IL_ERRNO_EPFNOSUPPORT IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EAFNOSUPPORT
#define IL_ERRNO_EAFNOSUPPORT IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EADDRINUSE
#define IL_ERRNO_EADDRINUSE IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EADDRNOTAVAIL
#define IL_ERRNO_EADDRNOTAVAIL IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ENETDOWN
#define IL_ERRNO_ENETDOWN IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ENETUNREACH
#define IL_ERRNO_ENETUNREACH IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ENETRESET
#define IL_ERRNO_ENETRESET IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ECONNABORTED
#define IL_ERRNO_ECONNABORTED IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ECONNRESET
#define IL_ERRNO_ECONNRESET IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ENOBUFS
#define IL_ERRNO_ENOBUFS IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EISCONN
#define IL_ERRNO_EISCONN IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ENOTCONN
#define IL_ERRNO_ENOTCONN IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ESHUTDOWN
#define IL_ERRNO_ESHUTDOWN IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ETOOMANYREFS
#define IL_ERRNO_ETOOMANYREFS IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ETIMEDOUT
#define IL_ERRNO_ETIMEDOUT IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ECONNREFUSED
#define IL_ERRNO_ECONNREFUSED IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EHOSTDOWN
#define IL_ERRNO_EHOSTDOWN IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EHOSTUNREACH
#define IL_ERRNO_EHOSTUNREACH IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EALREADY
#define IL_ERRNO_EALREADY IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EINPROGRESS
#define IL_ERRNO_EINPROGRESS IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ESTALE
#define IL_ERRNO_ESTALE IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EUCLEAN
#define IL_ERRNO_EUCLEAN IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ENOTNAM
#define IL_ERRNO_ENOTNAM IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ENAVAIL
#define IL_ERRNO_ENAVAIL IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EISNAM
#define IL_ERRNO_EISNAM IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EREMOTEIO
#define IL_ERRNO_EREMOTEIO IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EDQUOT
#define IL_ERRNO_EDQUOT IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ENOMEDIUM
#define IL_ERRNO_ENOMEDIUM IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EMEDIUMTYPE
#define IL_ERRNO_EMEDIUMTYPE IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ECANCELED
#define IL_ERRNO_ECANCELED IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ENOKEY
#define IL_ERRNO_ENOKEY IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EKEYEXPIRED
#define IL_ERRNO_EKEYEXPIRED IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EKEYREVOKED
#define IL_ERRNO_EKEYREVOKED IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EKEYREJECTED
#define IL_ERRNO_EKEYREJECTED IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_EOWNERDEAD
#define IL_ERRNO_EOWNERDEAD IL_ERRNO_EPERM
#endif
#ifndef IL_ERRNO_ENOTRECOVERABLE
#define IL_ERRNO_ENOTRECOVERABLE IL_ERRNO_EPERM
#endif
static int const errnoMapTable[] = {
    0,
    IL_ERRNO_EPERM,
    IL_ERRNO_ENOENT,
    IL_ERRNO_ESRCH,
    IL_ERRNO_EINTR,
    IL_ERRNO_EIO,
    IL_ERRNO_ENXIO,
    IL_ERRNO_E2BIG,
    IL_ERRNO_ENOEXEC,
    IL_ERRNO_EBADF,
    IL_ERRNO_ECHILD,
    IL_ERRNO_EAGAIN,
    IL_ERRNO_ENOMEM,
    IL_ERRNO_EACCES,
    IL_ERRNO_EFAULT,
    IL_ERRNO_ENOTBLK,
    IL_ERRNO_EBUSY,
    IL_ERRNO_EEXIST,
    IL_ERRNO_EXDEV,
    IL_ERRNO_ENODEV,
    IL_ERRNO_ENOTDIR,
    IL_ERRNO_EISDIR,
    IL_ERRNO_EINVAL,
    IL_ERRNO_ENFILE,
    IL_ERRNO_EMFILE,
    IL_ERRNO_ENOTTY,
    IL_ERRNO_ETXTBSY,
    IL_ERRNO_EFBIG,
    IL_ERRNO_ENOSPC,
    IL_ERRNO_ESPIPE,
    IL_ERRNO_EROFS,
    IL_ERRNO_EMLINK,
    IL_ERRNO_EPIPE,
    IL_ERRNO_EDOM,
    IL_ERRNO_ERANGE,
    IL_ERRNO_EDEADLK,
    IL_ERRNO_ENAMETOOLONG,
    IL_ERRNO_ENOLCK,
    IL_ERRNO_ENOSYS,
    IL_ERRNO_ENOTEMPTY,
    IL_ERRNO_ELOOP,
    IL_ERRNO_EPERM,
    IL_ERRNO_ENOMSG,
    IL_ERRNO_EIDRM,
    IL_ERRNO_ECHRNG,
    IL_ERRNO_EL2NSYNC,
    IL_ERRNO_EL3HLT,
    IL_ERRNO_EL3RST,
    IL_ERRNO_ELNRNG,
    IL_ERRNO_EUNATCH,
    IL_ERRNO_ENOCSI,
    IL_ERRNO_EL2HLT,
    IL_ERRNO_EBADE,
    IL_ERRNO_EBADR,
    IL_ERRNO_EXFULL,
    IL_ERRNO_ENOANO,
    IL_ERRNO_EBADRQC,
    IL_ERRNO_EBADSLT,
    IL_ERRNO_EPERM,
    IL_ERRNO_EBFONT,
    IL_ERRNO_ENOSTR,
    IL_ERRNO_ENODATA,
    IL_ERRNO_ETIME,
    IL_ERRNO_ENOSR,
    IL_ERRNO_ENONET,
    IL_ERRNO_ENOPKG,
    IL_ERRNO_EREMOTE,
    IL_ERRNO_ENOLINK,
    IL_ERRNO_EADV,
    IL_ERRNO_ESRMNT,
    IL_ERRNO_ECOMM,
    IL_ERRNO_EPROTO,
    IL_ERRNO_EMULTIHOP,
    IL_ERRNO_EDOTDOT,
    IL_ERRNO_EBADMSG,
    IL_ERRNO_EOVERFLOW,
    IL_ERRNO_ENOTUNIQ,
    IL_ERRNO_EBADFD,
    IL_ERRNO_EREMCHG,
    IL_ERRNO_ELIBACC,
    IL_ERRNO_ELIBBAD,
    IL_ERRNO_ELIBSCN,
    IL_ERRNO_ELIBMAX,
    IL_ERRNO_ELIBEXEC,
    IL_ERRNO_EILSEQ,
    IL_ERRNO_ERESTART,
    IL_ERRNO_ESTRPIPE,
    IL_ERRNO_EUSERS,
    IL_ERRNO_ENOTSOCK,
    IL_ERRNO_EDESTADDRREQ,
    IL_ERRNO_EMSGSIZE,
    IL_ERRNO_EPROTOTYPE,
    IL_ERRNO_ENOPROTOOPT,
    IL_ERRNO_EPROTONOSUPPORT,
    IL_ERRNO_ESOCKTNOSUPPORT,
    IL_ERRNO_EOPNOTSUPP,
    IL_ERRNO_EPFNOSUPPORT,
    IL_ERRNO_EAFNOSUPPORT,
    IL_ERRNO_EADDRINUSE,
    IL_ERRNO_EADDRNOTAVAIL,
    IL_ERRNO_ENETDOWN,
    IL_ERRNO_ENETUNREACH,
    IL_ERRNO_ENETRESET,
    IL_ERRNO_ECONNABORTED,
    IL_ERRNO_ECONNRESET,
    IL_ERRNO_ENOBUFS,
    IL_ERRNO_EISCONN,
    IL_ERRNO_ENOTCONN,
    IL_ERRNO_ESHUTDOWN,
    IL_ERRNO_ETOOMANYREFS,
    IL_ERRNO_ETIMEDOUT,
    IL_ERRNO_ECONNREFUSED,
    IL_ERRNO_EHOSTDOWN,
    IL_ERRNO_EHOSTUNREACH,
    IL_ERRNO_EALREADY,
    IL_ERRNO_EINPROGRESS,
    IL_ERRNO_ESTALE,
    IL_ERRNO_EUCLEAN,
    IL_ERRNO_ENOTNAM,
    IL_ERRNO_ENAVAIL,
    IL_ERRNO_EISNAM,
    IL_ERRNO_EREMOTEIO,
    IL_ERRNO_EDQUOT,
    IL_ERRNO_ENOMEDIUM,
    IL_ERRNO_EMEDIUMTYPE,
    IL_ERRNO_ECANCELED,
    IL_ERRNO_ENOKEY,
    IL_ERRNO_EKEYEXPIRED,
    IL_ERRNO_EKEYREVOKED,
    IL_ERRNO_EKEYREJECTED,
    IL_ERRNO_EOWNERDEAD,
    IL_ERRNO_ENOTRECOVERABLE,
};
#define errnoMapSize (sizeof(errnoMapTable) / sizeof(int))
int ILSysIOConvertErrno (int error) {
    if (error >= 0 && error < errnoMapSize) {
        return errnoMapTable[error];
    } else {
        return IL_ERRNO_EPERM;
    }
}
#ifndef EPERM
#define EPERM -1
#endif
#ifndef ENOENT
#define ENOENT -1
#endif
#ifndef ESRCH
#define ESRCH -1
#endif
#ifndef EINTR
#define EINTR -1
#endif
#ifndef EIO
#define EIO -1
#endif
#ifndef ENXIO
#define ENXIO -1
#endif
#ifndef E2BIG
#define E2BIG -1
#endif
#ifndef ENOEXEC
#define ENOEXEC -1
#endif
#ifndef EBADF
#define EBADF -1
#endif
#ifndef ECHILD
#define ECHILD -1
#endif
#ifndef EAGAIN
#define EAGAIN -1
#endif
#ifndef ENOMEM
#define ENOMEM -1
#endif
#ifndef EACCES
#define EACCES -1
#endif
#ifndef EFAULT
#define EFAULT -1
#endif
#ifndef ENOTBLK
#define ENOTBLK -1
#endif
#ifndef EBUSY
#define EBUSY -1
#endif
#ifndef EEXIST
#define EEXIST -1
#endif
#ifndef EXDEV
#define EXDEV -1
#endif
#ifndef ENODEV
#define ENODEV -1
#endif
#ifndef ENOTDIR
#define ENOTDIR -1
#endif
#ifndef EISDIR
#define EISDIR -1
#endif
#ifndef EINVAL
#define EINVAL -1
#endif
#ifndef ENFILE
#define ENFILE -1
#endif
#ifndef EMFILE
#define EMFILE -1
#endif
#ifndef ENOTTY
#define ENOTTY -1
#endif
#ifndef ETXTBSY
#define ETXTBSY -1
#endif
#ifndef EFBIG
#define EFBIG -1
#endif
#ifndef ENOSPC
#define ENOSPC -1
#endif
#ifndef ESPIPE
#define ESPIPE -1
#endif
#ifndef EROFS
#define EROFS -1
#endif
#ifndef EMLINK
#define EMLINK -1
#endif
#ifndef EPIPE
#define EPIPE -1
#endif
#ifndef EDOM
#define EDOM -1
#endif
#ifndef ERANGE
#define ERANGE -1
#endif
#ifndef EDEADLK
#define EDEADLK -1
#endif
#ifndef ENAMETOOLONG
#define ENAMETOOLONG -1
#endif
#ifndef ENOLCK
#define ENOLCK -1
#endif
#ifndef ENOSYS
#define ENOSYS -1
#endif
#ifndef ENOTEMPTY
#define ENOTEMPTY -1
#endif
#ifndef ELOOP
#define ELOOP -1
#endif
#ifndef ENOMSG
#define ENOMSG -1
#endif
#ifndef EIDRM
#define EIDRM -1
#endif
#ifndef ECHRNG
#define ECHRNG -1
#endif
#ifndef EL2NSYNC
#define EL2NSYNC -1
#endif
#ifndef EL3HLT
#define EL3HLT -1
#endif
#ifndef EL3RST
#define EL3RST -1
#endif
#ifndef ELNRNG
#define ELNRNG -1
#endif
#ifndef EUNATCH
#define EUNATCH -1
#endif
#ifndef ENOCSI
#define ENOCSI -1
#endif
#ifndef EL2HLT
#define EL2HLT -1
#endif
#ifndef EBADE
#define EBADE -1
#endif
#ifndef EBADR
#define EBADR -1
#endif
#ifndef EXFULL
#define EXFULL -1
#endif
#ifndef ENOANO
#define ENOANO -1
#endif
#ifndef EBADRQC
#define EBADRQC -1
#endif
#ifndef EBADSLT
#define EBADSLT -1
#endif
#ifndef EBFONT
#define EBFONT -1
#endif
#ifndef ENOSTR
#define ENOSTR -1
#endif
#ifndef ENODATA
#define ENODATA -1
#endif
#ifndef ETIME
#define ETIME -1
#endif
#ifndef ENOSR
#define ENOSR -1
#endif
#ifndef ENONET
#define ENONET -1
#endif
#ifndef ENOPKG
#define ENOPKG -1
#endif
#ifndef EREMOTE
#define EREMOTE -1
#endif
#ifndef ENOLINK
#define ENOLINK -1
#endif
#ifndef EADV
#define EADV -1
#endif
#ifndef ESRMNT
#define ESRMNT -1
#endif
#ifndef ECOMM
#define ECOMM -1
#endif
#ifndef EPROTO
#define EPROTO -1
#endif
#ifndef EMULTIHOP
#define EMULTIHOP -1
#endif
#ifndef EDOTDOT
#define EDOTDOT -1
#endif
#ifndef EBADMSG
#define EBADMSG -1
#endif
#ifndef EOVERFLOW
#define EOVERFLOW -1
#endif
#ifndef ENOTUNIQ
#define ENOTUNIQ -1
#endif
#ifndef EBADFD
#define EBADFD -1
#endif
#ifndef EREMCHG
#define EREMCHG -1
#endif
#ifndef ELIBACC
#define ELIBACC -1
#endif
#ifndef ELIBBAD
#define ELIBBAD -1
#endif
#ifndef ELIBSCN
#define ELIBSCN -1
#endif
#ifndef ELIBMAX
#define ELIBMAX -1
#endif
#ifndef ELIBEXEC
#define ELIBEXEC -1
#endif
#ifndef EILSEQ
#define EILSEQ -1
#endif
#ifndef ERESTART
#define ERESTART -1
#endif
#ifndef ESTRPIPE
#define ESTRPIPE -1
#endif
#ifndef EUSERS
#define EUSERS -1
#endif
#ifndef ENOTSOCK
#define ENOTSOCK -1
#endif
#ifndef EDESTADDRREQ
#define EDESTADDRREQ -1
#endif
#ifndef EMSGSIZE
#define EMSGSIZE -1
#endif
#ifndef EPROTOTYPE
#define EPROTOTYPE -1
#endif
#ifndef ENOPROTOOPT
#define ENOPROTOOPT -1
#endif
#ifndef EPROTONOSUPPORT
#define EPROTONOSUPPORT -1
#endif
#ifndef ESOCKTNOSUPPORT
#define ESOCKTNOSUPPORT -1
#endif
#ifndef EOPNOTSUPP
#define EOPNOTSUPP -1
#endif
#ifndef EPFNOSUPPORT
#define EPFNOSUPPORT -1
#endif
#ifndef EAFNOSUPPORT
#define EAFNOSUPPORT -1
#endif
#ifndef EADDRINUSE
#define EADDRINUSE -1
#endif
#ifndef EADDRNOTAVAIL
#define EADDRNOTAVAIL -1
#endif
#ifndef ENETDOWN
#define ENETDOWN -1
#endif
#ifndef ENETUNREACH
#define ENETUNREACH -1
#endif
#ifndef ENETRESET
#define ENETRESET -1
#endif
#ifndef ECONNABORTED
#define ECONNABORTED -1
#endif
#ifndef ECONNRESET
#define ECONNRESET -1
#endif
#ifndef ENOBUFS
#define ENOBUFS -1
#endif
#ifndef EISCONN
#define EISCONN -1
#endif
#ifndef ENOTCONN
#define ENOTCONN -1
#endif
#ifndef ESHUTDOWN
#define ESHUTDOWN -1
#endif
#ifndef ETOOMANYREFS
#define ETOOMANYREFS -1
#endif
#ifndef ETIMEDOUT
#define ETIMEDOUT -1
#endif
#ifndef ECONNREFUSED
#define ECONNREFUSED -1
#endif
#ifndef EHOSTDOWN
#define EHOSTDOWN -1
#endif
#ifndef EHOSTUNREACH
#define EHOSTUNREACH -1
#endif
#ifndef EALREADY
#define EALREADY -1
#endif
#ifndef EINPROGRESS
#define EINPROGRESS -1
#endif
#ifndef ESTALE
#define ESTALE -1
#endif
#ifndef EUCLEAN
#define EUCLEAN -1
#endif
#ifndef ENOTNAM
#define ENOTNAM -1
#endif
#ifndef ENAVAIL
#define ENAVAIL -1
#endif
#ifndef EISNAM
#define EISNAM -1
#endif
#ifndef EREMOTEIO
#define EREMOTEIO -1
#endif
#ifndef EDQUOT
#define EDQUOT -1
#endif
#ifndef ENOMEDIUM
#define ENOMEDIUM -1
#endif
#ifndef EMEDIUMTYPE
#define EMEDIUMTYPE -1
#endif
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
#define errnoFromMapSize (sizeof(errnoFromMapTable) / sizeof(int))
static inline JITINT32 ILSysIOConvertFromErrno (int error) {
    if (error >= 0 && error < errnoFromMapSize) {
        return errnoFromMapTable[error];
    }
    return -1;
}

/* Get an handle for the standard error stream */
JITNINT System_IO_MonoIO_get_ConsoleError (void) {
    JITNINT standardError;

    METHOD_BEGIN(ildjitSystem, "System.IO.MonoIO.get_ConsoleError");

    standardError = STDERR_FILENO;

    METHOD_END(ildjitSystem, "System.IO.MonoIO.get_ConsoleError");

    return standardError;
}

/* Get an handle for the standard output stream */
JITNINT System_IO_MonoIO_get_ConsoleOutput (void) {
    JITNINT standardOutput;

    METHOD_BEGIN(ildjitSystem, "System.IO.MonoIO.get_ConsoleOutput");

    standardOutput = STDOUT_FILENO;

    METHOD_END(ildjitSystem, "System.IO.MonoIO.get_ConsoleOutput");

    return standardOutput;
}

/* Get an handle for the standard input stream */
JITNINT System_IO_MonoIO_get_ConsoleInput (void) {
    JITNINT standardInput;

    METHOD_BEGIN(ildjitSystem, "System.IO.MonoIO.get_ConsoleInput");

    standardInput = STDIN_FILENO;

    METHOD_END(ildjitSystem, "System.IO.MonoIO.get_ConsoleInput");

    return standardInput;
}

/* Get the type of the file represented by handle */
JITINT32 System_IO_MonoIO_GetFileType (JITNINT handle, JITINT32* error) {
    struct stat stat;
    JITINT32 type;

    METHOD_BEGIN(ildjitSystem, "System.IO.MonoIO.GetFileType");

    if (PLATFORM_fstat(handle, &stat) == 0) {
        if (S_ISCHR(stat.st_mode)) {
            type = MonoFileType_Char;
            *error = MONO_ERROR_SUCCESS;

        } else if (S_ISREG(stat.st_mode)) {
            type = MonoFileType_Disk;
            *error = MONO_ERROR_SUCCESS;

        } else {
            type = MonoFileType_Unknown;
            *error = MONO_ERROR_FILE_NOT_FOUND;
        }

    } else {
        type = MonoFileType_Unknown;
        *error = MONO_ERROR_FILE_NOT_FOUND;
    }

    METHOD_END(ildjitSystem, "System.IO.MonoIO.GetFileType");

    return type;
}

/* Seek within a file */
JITINT64 System_IO_MonoIO_Seek (JITNINT handle, JITINT64 offset, JITINT32 from, JITINT32* error) {
    JITINT64 newPosition;

    METHOD_BEGIN(ildjitSystem, "System.IO.MonoIO.Seek");

    newPosition = Platform_FileMethods_Seek(handle, offset, from);
    if (newPosition == -1) {
        *error = translateToMonoErrorCode(Platform_FileMethods_GetErrno());
    } else {
        *error = MONO_ERROR_SUCCESS;
    }

    METHOD_END(ildjitSystem, "System.IO.MonoIO.Seek");

    return newPosition;
}

/* Write to file */
JITINT32 System_IO_MonoIO_Write (JITNINT handle, void* source, JITINT32 sourceOffset, JITINT32 count, JITINT32* error) {
    JITBOOLEAN success;
    JITINT32 bytesWritten;

    METHOD_BEGIN(ildjitSystem, "System.IO.MonoIO.Write");

    success = Platform_FileMethods_Write(handle, source, sourceOffset, count);
    if (success) {
        *error = translateToMonoErrorCode(IL_ERRNO_Success);
        bytesWritten = count;
    } else {
        *error = translateToMonoErrorCode(Platform_FileMethods_GetErrno());
        bytesWritten = -1;
    }

    METHOD_END(ildjitSystem, "System.IO.MonoIO.Write");

    return bytesWritten;
}

/* Get the volume separator character */
JITINT16 System_IO_MonoIO_get_VolumeSeparatorChar (void) {
    ILPathInfo pathInfo;
    JITINT16 separator;

    METHOD_BEGIN(ildjitSystem, "System.IO.MonoIO.get_VolumeSeparatorChar");

    ILGetPathInfo(&pathInfo);
    separator = pathInfo.volumeSep;

    METHOD_END(ildjitSystem, "System.IO.MonoIO.get_VolumeSeparatorChar");

    return separator;
}

/* Gets the directory separator characters */
JITINT16 System_IO_MonoIO_get_DirectorySeparatorChar (void) {
    ILPathInfo pathInfo;
    JITINT16 separator;

    METHOD_BEGIN(ildjitSystem, "System.IO.MonoIO.get_DirectorySeparatorChar");

    ILGetPathInfo(&pathInfo);
    separator = pathInfo.dirSep;

    METHOD_END(ildjitSystem, "System.IO.MonoIO.get_DirectorySeparatorChar");

    return separator;
}

/* Gets the alternate directory separator character */
JITINT16 System_IO_MonoIO_get_AltDirectorySeparatorChar (void) {
    ILPathInfo pathInfo;
    JITINT16 separator;

    METHOD_BEGIN(ildjitSystem, "System.IO.MonoIO.get_AltDirectorySeparatorChar");

    ILGetPathInfo(&pathInfo);
    separator = pathInfo.altDirSep;

    METHOD_END(ildjitSystem, "System.IO.MonoIO.get_AltDirectorySeparatorChar");

    return separator;
}

/* Gets the path separator character */
JITINT16 System_IO_MonoIO_get_PathSeparator (void) {
    ILPathInfo pathInfo;
    JITINT16 separator;

    METHOD_BEGIN(ildjitSystem, "System.IO.MonoIO.get_PathSeparator");

    ILGetPathInfo(&pathInfo);
    separator = pathInfo.pathSep;

    METHOD_END(ildjitSystem, "System.IO.MonoIO.get_PathSeparator");

    return separator;
}

/* Close a file */
JITBOOLEAN System_IO_MonoIO_Close (JITNINT handle, JITINT32* error) {
    JITBOOLEAN success;

    METHOD_BEGIN(ildjitSystem, "System.IO.MonoIO.Close");

    success = Platform_FileMethods_Close(handle);
    if (success) {
        *error = MONO_ERROR_SUCCESS;
    } else {
        *error = translateToMonoErrorCode(Platform_FileMethods_GetErrno());
    }

    METHOD_END(ildjitSystem, "System.IO.MonoIO.Close");

    return success;
}

/* Get the current directory name */
void* System_IO_MonoIO_GetCurrentDirectory (JITINT32* error) {
    void* name;

    METHOD_BEGIN(ildjitSystem, "System.IO.MonoIO.GetCurrentDirectory");

    name = Platform_DirMethods_GetCurrentDirectory();
    *error = MONO_ERROR_SUCCESS;

    METHOD_END(ildjitSystem, "System.IO.MonoIO.GetCurrentDirectory");

    return name;
}

/* Sets the current directory */
JITBOOLEAN System_IO_MonoIO_SetCurrentDirectory (void* directory, JITINT32* error) {
    JITINT32 pnetErrno;
    JITBOOLEAN success;

    METHOD_BEGIN(ildjitSystem, "System.IO.MonoIO.SetCurrentDirectory");

    pnetErrno = Platform_DirMethods_ChangeDirectory(directory);
    if (pnetErrno == IL_ERRNO_Success) {
        success = JITTRUE;
    } else {
        success = JITFALSE;
    }
    *error = translateToMonoErrorCode(pnetErrno);

    METHOD_END(ildjitSystem, "System.IO.MonoIO.SetCurrentDirectory");

    return success;
}

/* Get the attributes of the given file */
JITINT32 System_IO_MonoIO_GetFileAttributes (void* name, JITINT32* error) {
    JITINT32 attributes;
    JITINT32 pnetError;

    METHOD_BEGIN(ildjitSystem, "System.IO.MonoIO.GetFileAttributes");

    pnetError = Platform_FileMethods_GetAttributes(name, &attributes);
    if (pnetError != IL_ERRNO_Success) {
        attributes = -1;
    }
    *error = translateToMonoErrorCode(pnetError);

    METHOD_END(ildjitSystem, "System.IO.MonoIO.GetFileAttributes");

    return attributes;
}

/* Create given directory */
JITBOOLEAN System_IO_MonoIO_CreateDirectory (void* name, JITINT32* error) {
    JITINT32 pnetError;
    JITBOOLEAN success;

    METHOD_BEGIN(ildjitSystem, "System.IO.MonoIO.CreateDirectory");

    pnetError = Platform_DirMethods_CreateDirectory(name);
    if (pnetError == IL_ERRNO_Success) {
        success = JITTRUE;
    } else {
        success = JITFALSE;
    }
    *error = translateToMonoErrorCode(pnetError);

    METHOD_END(ildjitSystem, "System.IO.MonoIO.CreateDirectory");

    return success;
}

/* Get statistics about a file */
JITBOOLEAN System_IO_MonoIO_GetFileStat (void* path, void* stats, JITUINT32* error) {
    PnetLongStatGetter statGetters[4];
    MonoLongStatSetter monoStatSetters[4];
    JITINT32 attributes;
    JITINT32 pnetError;
    JITINT32 startIndex;
    JITINT32 i;
    JITINT64 stat;
    JITBOOLEAN success;

    METHOD_BEGIN(ildjitSystem, "System.IO.MonoIO.GetFileStat");

    statGetters[0] = Platform_FileMethods_GetLength;
    statGetters[1] = Platform_DirMethods_GetCreationTime;
    statGetters[2] = Platform_DirMethods_GetLastAccess;
    statGetters[3] = Platform_DirMethods_GetLastModification;
    monoStatSetters[0] = MonoIOStat_setLength;
    monoStatSetters[1] = MonoIOStat_setCreationTime;
    monoStatSetters[2] = MonoIOStat_setLastAccessTime;
    monoStatSetters[3] = MonoIOStat_setLastWriteTime;

    success = JITTRUE;
    pnetError = Platform_FileMethods_GetAttributes(path, &attributes);
    if (pnetError != IL_ERRNO_Success) {
        success = JITFALSE;
    } else {
        MonoIOStat_setAttributes(stats, attributes);
    }

    if ((attributes & ILFileAttributes_Directory) == ILFileAttributes_Directory) {
        startIndex = 1;
    } else {
        startIndex = 0;
    }

    for (i = startIndex; i < 4 && success; i++) {
        pnetError = statGetters[i](path, &stat);
        if (pnetError == IL_ERRNO_Success) {
            monoStatSetters[i](stats, stat);
        } else {
            success = JITFALSE;
        }
    }
    *error = translateToMonoErrorCode(pnetError);

    METHOD_END(ildjitSystem, "System.IO.MonoIO.GetFileStat");

    return success;
}

/* Delete given file */
JITBOOLEAN System_IO_MonoIO_DeleteFile (void* path, JITINT32* error) {
    JITINT32 pnetError;
    JITBOOLEAN success;

    METHOD_BEGIN(ildjitSystem, "System.IO.MonoIO.DeleteFile");

    pnetError = Platform_DirMethods_Delete(path);
    if (pnetError == IL_ERRNO_Success) {
        success = JITTRUE;
    } else {
        success = JITFALSE;
    }
    *error = translateToMonoErrorCode(pnetError);

    METHOD_END(ildjitSystem, "System.IO.MonoIO.DeleteFile");

    return success;
}

/* Delete given directory */
JITBOOLEAN System_IO_MonoIO_RemoveDirectory (void* path, JITINT32* error) {
    JITINT32 pnetError;
    JITBOOLEAN success;

    METHOD_BEGIN(ildjitSystem, "System.IO.MonoIO.RemoveDirectory");

    pnetError = Platform_DirMethods_Delete(path);
    if (pnetError == IL_ERRNO_Success) {
        success = JITTRUE;
    } else {
        success = JITFALSE;
    }
    *error = translateToMonoErrorCode(pnetError);

    METHOD_END(ildjitSystem, "System.IO.MonoIO.RemoveDirectory");

    return success;
}

/* Move a file */
JITBOOLEAN System_IO_MonoIO_MoveFile (void* from, void* to, JITINT32* error) {
    JITINT32 pnetError;
    JITBOOLEAN success;

    METHOD_BEGIN(ildjitSystem, "System.IO.MonoIO.MoveFile");

    pnetError = Platform_DirMethods_Rename(from, to);
    if (pnetError == IL_ERRNO_Success) {
        success = JITTRUE;
    } else {
        success = JITFALSE;
    }
    *error = translateToMonoErrorCode(pnetError);

    METHOD_END(ildjitSystem, "System.IO.MonoIO.MoveFile");

    return success;
}

/* Open a file */
JITNINT System_IO_MonoIO_Open (void* name, JITINT32 mode, JITINT32 access, JITINT32 share, JITINT32 options, JITINT32* error) {
    JITBOOLEAN success;
    JITNINT handle;

    METHOD_BEGIN(ildjitSystem, "System.IO.MonoIO.Open");

    if (options != FileOptions_None) {
        print_err("Custom options not supported by the PNET backend", 0);
        abort();
    }

    success = Platform_FileMethods_Open(name, mode, access, share, &handle);
    if (success) {
        *error = MONO_ERROR_SUCCESS;
    } else {
        *error = translateToMonoErrorCode(Platform_FileMethods_GetErrno());
    }

    METHOD_BEGIN(ildjitSystem, "System.IO.MonoIO.Open");

    return handle;
}

/* Set the length of a file */
JITBOOLEAN System_IO_MonoIO_SetLength (JITNINT handle, JITINT64 length, JITINT32* error) {
    JITBOOLEAN success;

    METHOD_BEGIN(ildjitSystem, "System.IO.MonoIO.SetLength");

    if (PLATFORM_ftruncate(handle, length) == 0) {
        success = JITTRUE;
        *error = MONO_ERROR_SUCCESS;
    } else {
        success = JITFALSE;
        *error = translateToMonoErrorCode(errno);
    }

    METHOD_END(ildjitSystem, "System.IO.MonoIO.SetLength");

    return success;
}

/* Get the length of a file */
JITINT64 System_IO_MonoIO_GetLength (JITNINT handle, JITINT32* error) {
    struct stat stat;
    JITINT64 length;

    METHOD_BEGIN(ildjitSystem, "System.IO.MonoIO.GetLength");

    if (PLATFORM_fstat(handle, &stat) == 0) {
        length = stat.st_size;
        *error = MONO_ERROR_SUCCESS;
    } else {
        length = -1;
        *error = translateToMonoErrorCode(errno);
    }

    METHOD_END(ildjitSystem, "System.IO.MonoIO.GetLength");

    return length;
}

/* Read data from file */
JITINT32 System_IO_MonoIO_Read (JITNINT handle, void* buffer, JITINT32 offset, JITINT32 count, JITINT32* error) {
    JITINT32 bytesRead;

    METHOD_BEGIN(ildjitSystem, "System.IO.MonoIO.Read");

    bytesRead = Platform_FileMethods_Read(handle, buffer, offset, count);
    if (bytesRead == -1) {
        *error = translateToMonoErrorCode(Platform_FileMethods_GetErrno());
    } else {
        *error = MONO_ERROR_SUCCESS;
    }

    /* Return			*/
    METHOD_END(ildjitSystem, "System.IO.MonoIO.Read");
    return bytesRead;
}

/* Translate given error code from the Pnet convention to the Mono convention */
static JITINT32 translateToMonoErrorCode (JITINT32 pnetError) {
    switch (pnetError) {
        case IL_ERRNO_Success:
            return MONO_ERROR_SUCCESS;

        case IL_ERRNO_ENOENT:
            return MONO_ERROR_FILE_NOT_FOUND;

        default:
            print_err("Unknown error code", 0);
            abort();
    }
}

/* Load MonoIO related metadata */
static void monoIOLoadMetadata (void) {
    t_running_garbage_collector* garbageCollector;
    JITINT8* fieldNames[MONO_IO_STAT_FIELD_COUNT];
    TypeDescriptor *monoIOType;
    FieldDescriptor *field;
    JITUINT32 i;

    garbageCollector = &ildjitSystem->garbage_collectors.gc;

    fieldNames[0] = (JITINT8 *) "Name";
    fieldNames[1] = (JITINT8 *) "Attributes";
    fieldNames[2] = (JITINT8 *) "Length";
    fieldNames[3] = (JITINT8 *) "CreationTime";
    fieldNames[4] = (JITINT8 *) "LastAccessTime";
    fieldNames[5] = (JITINT8 *) "LastWriteTime";

    monoIOType = (ildjitSystem->cliManager).metadataManager->getTypeDescriptorFromName((ildjitSystem->cliManager).metadataManager, (JITINT8 *) "System.IO", (JITINT8 *) "MonoIOStat");

    for (i = 0; i < MONO_IO_STAT_FIELD_COUNT; i++) {
        field = monoIOType->getFieldFromName(monoIOType, fieldNames[i]);
        monoIOStat.fieldOffsets[i] = garbageCollector->fetchFieldOffset(field);
    }
}

/* Sets the Attributes field of a MonoIOStat value type */
static void MonoIOStat_setAttributes (void* stats, JITINT32 attributes) {
    CHECK_MONO_IO_METADATA();

    MONO_IO_STAT_SET_FIELD(stats, MONO_IO_STAT_Attributes, JITINT32, attributes);
}

/* Sets the Length field of a MonoIOStat value type */
static void MonoIOStat_setLength (void* stats, JITINT64 length) {
    CHECK_MONO_IO_METADATA();

    MONO_IO_STAT_SET_FIELD(stats, MONO_IO_STAT_Length, JITINT64, length);
}

/* Sets the CreationTime field of a MonoIOStat value type */
static void MonoIOStat_setCreationTime (void* stats, JITINT64 creationTime) {
    CHECK_MONO_IO_METADATA();

    MONO_IO_STAT_SET_FIELD(stats, MONO_IO_STAT_CreationTime,
                           JITINT64, creationTime);
}

/* Sets the LastAccessTime field of a MonoIOStat value type */
static void MonoIOStat_setLastAccessTime (void* stats,
        JITINT64 lastAccessTime) {
    CHECK_MONO_IO_METADATA();

    MONO_IO_STAT_SET_FIELD(stats, MONO_IO_STAT_LastAccessTime,
                           JITINT64, lastAccessTime);
}

/* Sets the LastWriteTime field of a MonoIOStat value type */
static void MonoIOStat_setLastWriteTime (void* stats, JITINT64 lastWriteTime) {
    CHECK_MONO_IO_METADATA();

    MONO_IO_STAT_SET_FIELD(stats, MONO_IO_STAT_LastWriteTime,
                           JITINT64, lastWriteTime);
}

/* Call to the isatty function to test whether a file descriptor refers to the terminal */
JITBOOLEAN System_ConsoleDriver_Isatty (JITNINT handle) {
    JITBOOLEAN isReferingToTerminal;

    METHOD_BEGIN(ildjitSystem, "System.ConsoleDriver.Isatty");

    isReferingToTerminal = isatty((JITINT32) ((JITNUINT) handle));

    METHOD_END(ildjitSystem, "System.ConsoleDriver.Isatty");

    return isReferingToTerminal;
}
