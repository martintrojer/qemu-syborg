/*
 * Syborg Host Filesystem pseudo-device
 * Implements a set of syscalls that may look remarkably similar
 * to those used by SymbianOS.
 *
 * Copyright (c) 2009 CodeSourcery
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "hw.h"
#include "syborg.h"
#include "devtree.h"

//#define DEBUG_SYBORG_HOSTFS

#ifdef DEBUG_SYBORG_HOSTFS
#define DPRINTF(fmt, args...) \
do { printf("syborg_hostfs: " fmt , ##args); } while (0)
#define BADF(fmt, args...) \
do { fprintf(stderr, "syborg_hostfs: error: " fmt , ##args); exit(1);} while (0)
#else
#define DPRINTF(fmt, args...) do {} while(0)
#define BADF(fmt, args...) \
do { fprintf(stderr, "syborg_hostfs: error: " fmt , ##args);} while (0)
#endif

#define HOSTFS_PATH_MAX 65536
#ifdef _WIN32
#include <mbstring.h>
#include <wchar.h>
#include <io.h>
typedef wchar_t host_char;
typedef struct _stat host_stat;
typedef struct {
    int handle;
    struct  _wfinddata_t info;
} hostfs_dir;
#else
#include <iconv.h>
#include <langinfo.h>
#include <locale.h>
#include <dirent.h>
#include <fnmatch.h>
typedef char host_char;
typedef struct stat host_stat;
typedef struct {
    DIR *dir;
    char pattern[HOSTFS_PATH_MAX];
    char path[HOSTFS_PATH_MAX];
} hostfs_dir;
#endif
#define HOST_CHAR(var) host_char var[HOSTFS_PATH_MAX]

enum {
    HOSTFS_ID           = 0,
    HOSTFS_COMMAND      = 1,
    HOSTFS_RESULT       = 2,
    HOSTFS_ARG0         = 3,
    HOSTFS_ARG1         = 4,
    HOSTFS_ARG2         = 5,
    HOSTFS_ARG3         = 6
};

typedef struct hostfs_handle_cache {
    int handle;
    int is_fd;
    struct {
        int fd;
        hostfs_dir *d;
    } val;
    struct hostfs_handle_cache *next;
} hostfs_handle_cache;

typedef struct {
    QEMUDevice *qdev;
    uint32_t result;
    uint32_t arg[4];
    uint32_t command;
    char drive_letter;
    host_char *host_prefix;
    int host_prefix_len;
    int last_handle;
    hostfs_handle_cache *handle_cache;
    hostfs_handle_cache *free_handle;
#ifndef _WIN32
    iconv_t iconv_guest_to_host;
    iconv_t iconv_host_to_guest;
#endif
} syborg_hostfs_state;

#define HOSTFS_ATTR_READONLY    0x01
#define HOSTFS_ATTR_HIDDEN      0x02
#define HOSTFS_ATTR_DIRECTORY   0x10

#define HOST_FS_SUCCESS          0          // KErrNone
#define HOST_FS_NOT_FOUND       -1          // KErrNotFound=(-1
#define HOST_FS_GENERAL_ERROR   -2          // KErrGeneral=(-2);

#define HOST_FS_NO_MEMORY       -4          // KErrNoMemory=(-4)
#define HOST_FS_UNSUPPORTED     -5          // KErrNotSupported=(-5) 
#define HOST_FS_BAD_ARG         -6          // KErrArgument=(-6)    
#define HOST_FS_BAD_HANDLE      -8          // KErrBadHandle=(-8)
#define HOST_FS_EXISTS          -11         // KErrAlreadyExists=(-11)
#define HOST_FS_PATH_NOT_FOUND  -12         // KErrPathNotFound=(-12)
#define HOST_FS_IN_USE          -14         // KErrInUse=(-14)
#define HOST_FS_UNKNOWN         -19         // KErrUnknown=(-19)
#define HOST_FS_CORRUPT         -20         // KErrCorrupt=(-20)
#define HOST_FS_ACCESS_DENIED   -21         // KErrAccessDenied=(-21)
#define HOST_FS_LOCKED          -22         // KErrLocked=(-22)
#define HOST_FS_WRITE           -23         // KErrWrite=(-23)
#define HOST_FS_EOF             -25         // KErrEof=(-25)
#define HOST_FS_DISKFULL        -26         // KErrDiskFull=(-26)
#define HOST_FS_BAD_NAME        -28         // KErrBadName=(-28)
#define HOST_FS_ABORT           -39         // KErrAbort=(-39)
#define HOST_FS_TOO_BIG         -40         // KErrTooBig=(-40)
#define HOST_FS_DIR_FULL        -43         // KErrDirFull=(-43)
#define HOST_FS_PERMISSION_DENIED -46       // KErrPermissionDenied=(-46)

static int decode_error(int e)
{
    int r = HOST_FS_GENERAL_ERROR;
    switch (e) {
    case EPERM :                            /* Operation not permitted */
        r = HOST_FS_PERMISSION_DENIED;
        break;
#ifdef ENOFILE
    case ENOFILE :                          /* No such file or directory */
#endif
#ifdef ENOENT
#if !defined(ENOFILE) || (ENOFILE != ENOENT)
        case ENOENT:
#endif
#endif
        r = HOST_FS_PATH_NOT_FOUND;
        break;
    case ESRCH :                            /* No such process */
        r = HOST_FS_NOT_FOUND;
        break;
    case EINTR :                            /* Interrupted function call */
    case EIO :                              /* Input/output error */
    case ENXIO :                            /* No such device or address */
        break;
    case E2BIG :                            /* Arg list too long */
        r = HOST_FS_TOO_BIG;
        break;
    case ENOEXEC :                          /* Exec format error */
        break;
    case EBADF :                            /* Bad file descriptor */
        r = HOST_FS_BAD_ARG;
        break;
    case ECHILD :                           /* No child processes */
        break;
    case EAGAIN :                           /* Resource temporarily unavailable */
        r = HOST_FS_GENERAL_ERROR;
        break;
    case ENOMEM :                           /* Not enough space */
        r = HOST_FS_NO_MEMORY;
        break;
    case EACCES :                           /* Permission denied */
        r = HOST_FS_ACCESS_DENIED;
        break;
    case EFAULT :                           /* Bad address */
        break;
    case 15 :                               /* 15 - Unknown Error */
        r = HOST_FS_UNKNOWN;
        break;
    case EBUSY :                            /* strerror reports "Resource device" */
        r = HOST_FS_IN_USE;
        break;
    case EEXIST :                           /* File exists */
        r = HOST_FS_EXISTS;
        break;
    case EXDEV :                            /* Improper link (cross-device link?) */
        break;
    case ENODEV :                           /* No such device */
        r = HOST_FS_BAD_NAME;
        break;
    case ENOTDIR :                          /* Not a directory */
    case EISDIR :                           /* Is a directory */
    case EINVAL :                           /* Invalid argument */
        r = HOST_FS_BAD_ARG;
        break;
    case ENFILE :                           /* Too many open files in system */
    case EMFILE :                           /* Too many open files */
    case ENOTTY :                           /* Inappropriate I/O control operation */
        break;
    case 26 :                               /* 26 - Unknown Error */
        r = HOST_FS_UNKNOWN;
        break;
    case EFBIG :                            /* File too large */
        r = HOST_FS_TOO_BIG;
        break;
    case ENOSPC :                           /* No space left on device */
        r = HOST_FS_DISKFULL;
        break;
    case ESPIPE :                           /* Invalid seek (seek on a pipe?) */
    case EROFS :                            /* Read-only file system */
    case EMLINK :                           /* Too many links */
    case EPIPE :                            /* Broken pipe */
    case EDOM :                             /* Domain error (math functions) */
    case ERANGE :                           /* Result too large (possibly too small) */
        break;
    case EDEADLK :                          /* Resource deadlock avoided (non-Cyg) */
        break;
    case ENAMETOOLONG :                     /* Filename too long (91 in Cyg?) */
        r = HOST_FS_BAD_NAME;
        break;
    case ENOLCK :                           /* No locks available (46 in Cyg?) */
    case ENOSYS :                           /* Function not implemented (88 in Cyg?) */
        r = HOST_FS_UNSUPPORTED;
        break;
    case ENOTEMPTY :                        /* Directory not empty (90 in Cyg?) */
    case EILSEQ :                           /* Illegal byte sequence */
        break;
    }
    return r;
}

typedef enum hostfs_op {
    EDummy = 0,

    /*  Codes for CMountCB operations */
    EMkDir,
    ERmDir,
    EDelete,
    ERename,
    EReplace,
    EReadUid,
    EEntry,
    ESetEntry,
    EFileOpen,
    EDirOpen,
    
    /*  Code for CFileCB operations */
    EFileClose, 
    EFileRead,
    EFileWrite,
    EFileSetSize,
    EFileFlush,

    /*  Code for CDirCB operations */
    EDirClose, 
    EDirRead,
} syborg_hostfs_op_t;

static hostfs_handle_cache *get_new_handle(syborg_hostfs_state *s)
{
    hostfs_handle_cache *c;

    if (s->free_handle) {
        c = s->free_handle;
        s->free_handle = c->next;
    } else {
        c = qemu_malloc(sizeof(*c));
        c->handle = ++s->last_handle;
    }
    c->next = s->handle_cache;
    s->handle_cache = c;

    return c;
}

static int add_file_cache_entry(syborg_hostfs_state *s, int fd)
{
    hostfs_handle_cache *c = get_new_handle(s);
    c->is_fd = 1;
    c->val.fd = fd;
    return c->handle;
}

static int add_dir_cache_entry(syborg_hostfs_state *s, hostfs_dir *d)
{
    hostfs_handle_cache *c = get_new_handle(s);
    c->is_fd = 0;
    c->val.d = d;
    return c->handle;
}

static hostfs_handle_cache *get_handle_cache_entry(syborg_hostfs_state *s,
                                                   int handle)
{
    hostfs_handle_cache *c;

    for (c = s->handle_cache; c; c = c->next) {
        if (c->handle == handle)
            return c;
    }
    return NULL;
}

static int get_file_cache_entry(syborg_hostfs_state *s, int handle, int *fd)
{
    hostfs_handle_cache *c = get_handle_cache_entry(s, handle);
    if (!c || !c->is_fd)
        return HOST_FS_BAD_HANDLE;
    *fd = c->val.fd;
    return HOST_FS_SUCCESS;
}

static int get_dir_cache_entry(syborg_hostfs_state *s, int handle, hostfs_dir **d)
{
    hostfs_handle_cache *c = get_handle_cache_entry(s, handle);
    if (!c || c->is_fd)
        return HOST_FS_BAD_HANDLE;
    *d = c->val.d;
    return HOST_FS_SUCCESS;
}

static void remove_handle_cache_entry(syborg_hostfs_state *s, int handle)
{
    hostfs_handle_cache *c;
    hostfs_handle_cache **p;
    p = &s->handle_cache;
    c = *p;
    while (c && c->handle != handle) {
        p = &c->next;
        c = c->next;
    }
    if (!c)
        return;
    *p = c->next;
    c->next = s->free_handle;
    s->free_handle = c;
}

static void remove_file_cache_entry(syborg_hostfs_state *s, int handle)
{
    remove_handle_cache_entry(s, handle);
}

static void remove_dir_cache_entry(syborg_hostfs_state *s, int handle)
{
    remove_handle_cache_entry(s, handle);
}

static int hostfs_get_filename(syborg_hostfs_state *s, host_char *buf,
                               uint32_t addr, uint32_t len)
{
    uint8_t prefix[6];

    if (len < 3)
        return HOST_FS_BAD_NAME;
    cpu_physical_memory_read(addr, prefix, 6);
    if (prefix[1] != 0 || prefix[3] != 0 || prefix[5] != 0
        || qemu_toupper(prefix[0]) != s->drive_letter
        || prefix[2] != ':' || prefix[4] != '\\')
        return HOST_FS_BAD_NAME;
    len -= 3;
    addr += 6;
#ifdef _WIN32
    if (len + s->host_prefix_len >= HOSTFS_PATH_MAX)
        return HOST_FS_BAD_NAME;
    memcpy(buf, s->host_prefix, s->host_prefix_len * 2);
    buf += s->host_prefix_len;
    cpu_physical_memory_read(addr, (void *)buf, len * 2);
    buf[len] = 0;
#else
    uint8_t guest_path[HOSTFS_PATH_MAX];
    char *inp;
    char *outp;
    size_t inbytes;
    size_t outbytes;
    int err;
    if (len >= HOSTFS_PATH_MAX)
        return HOST_FS_BAD_NAME;
    memcpy(buf, s->host_prefix, s->host_prefix_len);
    cpu_physical_memory_read(addr, guest_path, len * 2);
    if (len > 0) {
        inp = (char *)guest_path;
        outp = buf + s->host_prefix_len;
        inbytes = len * 2;
        outbytes = HOSTFS_PATH_MAX - (s->host_prefix_len + 1);
        if (s->host_prefix_len > 0 && buf[s->host_prefix_len - 1] != '/') {
            *(outp++) = '/';
            outbytes--;
        }
        *outp = '/';
        err = iconv(s->iconv_guest_to_host, &inp, &inbytes, &outp, &outbytes);
        if (err < 0)
            return decode_error(errno);
        *outp = 0;
        for (outp = buf; *outp; outp++) {
            if (*outp == '\\')
                *outp = '/';
        }
    }
#endif
    return HOST_FS_SUCCESS;
}

typedef int (*syborg_hostfs_op_fn)(syborg_hostfs_state *);

static int hostfs_unsupported(syborg_hostfs_state *s)
{
    return HOST_FS_UNSUPPORTED;
}

static int hostfs_mkdir(syborg_hostfs_state *s)
{
    HOST_CHAR(name);
    int err;

    err = hostfs_get_filename(s, name, s->arg[0], s->arg[1]);
    if (err)
        return err;
#ifdef _WIN32
    err = _wmkdir(name);
#else
    err = mkdir(name, 0777);
#endif
    if (err < 0)
        return decode_error(errno);

    return HOST_FS_SUCCESS;
}

static int hostfs_rmdir(syborg_hostfs_state *s)
{
    HOST_CHAR(name);
    int err;

    err = hostfs_get_filename(s, name, s->arg[0], s->arg[1]);
    if (err)
        return err;
#ifdef _WIN32
    err = _wrmdir(name);
#else
    err = rmdir(name);
#endif
    if (err < 0)
        return decode_error(errno);

    return HOST_FS_SUCCESS;
}

static int hostfs_delete(syborg_hostfs_state *s)
{
    HOST_CHAR(name);
    int err;

    err = hostfs_get_filename(s, name, s->arg[0], s->arg[1]);
    if (err)
        return err;
#ifdef _WIN32
    err = _wunlink(name);
#else
    err = unlink(name);
#endif
    if (err < 0)
        return decode_error(errno);

    return HOST_FS_SUCCESS;
}

static int hostfs_rename(syborg_hostfs_state *s)
{
    HOST_CHAR(old_name);
    HOST_CHAR(new_name);
    int err;

    err = hostfs_get_filename(s, old_name, s->arg[0], s->arg[1]);
    if (err)
        return err;
    err = hostfs_get_filename(s, new_name, s->arg[2], s->arg[3]);
    if (err)
        return err;
#ifdef _WIN32
    err = _wrename(old_name, new_name);
#else
    err = rename(old_name, new_name);
#endif
    if (err < 0)
        return decode_error(errno);

    return HOST_FS_SUCCESS;
}

static int hostfs_replace(syborg_hostfs_state *s)
{
    HOST_CHAR(old_name);
    HOST_CHAR(new_name);
    int err;

    err = hostfs_get_filename(s, old_name, s->arg[0], s->arg[1]);
    if (err)
        return err;
    err = hostfs_get_filename(s, new_name, s->arg[2], s->arg[3]);
    if (err)
        return err;
#ifdef _WIN32
    if (!_waccess(new_name, F_OK)) {
        if (_wunlink(new_name)) {
            return decode_error(errno);
        }
    }
    err = _wrename(old_name, new_name);
#else
    if (!access(new_name, F_OK)) {
        if (unlink(new_name)) {
            return decode_error(errno);
        }
    }
    err = rename(old_name, new_name);
#endif
    if (err < 0)
        return decode_error(errno);

    return HOST_FS_SUCCESS;
}

static uint32 hostfs_map_file_att(uint32 val)
{
    uint32 r = 0;
    if (!(val & S_IRWXU))           /* hidden */
        r |= HOSTFS_ATTR_HIDDEN;
    else if (!(val & S_IWRITE))     /* readonly */
        r |= HOSTFS_ATTR_READONLY;
    if (S_ISDIR(val))               /* directory */
        r |= HOSTFS_ATTR_DIRECTORY;
    
    return r;
}

static int hostfs_entry(syborg_hostfs_state *s)
{
    HOST_CHAR(name);
    host_stat stat_buf;
    int err;

    err = hostfs_get_filename(s, name, s->arg[0], s->arg[1]);
    if (err)
        return err;
#ifdef _WIN32
    err = _wstat(name, &stat_buf);
#else
    err = lstat(name, &stat_buf);
#endif
    if (err < 0)
        return decode_error(errno);

    s->arg[0] = hostfs_map_file_att(stat_buf.st_mode); /*  attributes */
    s->arg[1] = stat_buf.st_mtime;              /*  modified time */
    s->arg[2] = stat_buf.st_size;               /*  file size */

    return HOST_FS_SUCCESS;
}

static int hostfs_set_entry(syborg_hostfs_state *s)
{
    return HOST_FS_UNSUPPORTED;
}

#define _EFileWrite     0x200
#define EFileOpen       0
#define EFileCreate     1
#define EFileReplace    2

static int hostfs_file_open(syborg_hostfs_state *s)
{
    HOST_CHAR(name);
    int fd, handle, flags = 0, access = O_RDWR, create = 0, mode = 0;
    host_stat stat_buf;
    int err;

    err = hostfs_get_filename(s, name, s->arg[0], s->arg[1]);
    if (err)
        return err;
    
    if (!(s->arg[2] & _EFileWrite))
        access = O_RDONLY;
    else
        access = O_RDWR;
    
    switch (s->arg[3]) {
    case EFileOpen:
        break;
    case EFileCreate:
        create = (O_CREAT|O_EXCL);
        mode = S_IRUSR | S_IWUSR;
        break;
    case EFileReplace:
        create = O_CREAT | O_TRUNC;
        mode = S_IRUSR | S_IWUSR;
        break;
    }
    flags = access|create|O_BINARY;
#ifdef _WIN32
    fd = _wopen(name, flags, mode);
#else
    DPRINTF("Opening %s %x 0%o\n", name, flags, mode);
    fd = open(name, flags, mode);
#endif
    if (fd == -1) {
        err = errno;
        return decode_error(err);
    }

#ifdef _WIN32
    if (_fstat(fd, &stat_buf)) {
#else
    if (fstat(fd, &stat_buf)) {
#endif
        err = errno;
        close(fd);
        return decode_error(err);       
    }

    handle = add_file_cache_entry(s, fd);
    if (handle < 0)
        return handle;
    
    s->arg[0] = handle;                         /* handle */
    /* make entry's stat info available in registers */
    s->arg[1] = hostfs_map_file_att(stat_buf.st_mode); /* attributes */
    s->arg[2] = stat_buf.st_mtime;              /* modified time */
    s->arg[3] = stat_buf.st_size;               /* file size */
    
    return HOST_FS_SUCCESS;
}

static int hostfs_dir_open(syborg_hostfs_state *s)
{
    host_char name[HOSTFS_PATH_MAX + 2];
    hostfs_dir *d;
    int err;
    int len;

    err = hostfs_get_filename(s, name, s->arg[0], s->arg[1]);
    if (err)
        return err;
    d = qemu_mallocz(sizeof(*d));
    if (!d)
        return HOST_FS_NO_MEMORY;
#ifdef _WIN32
    len = wcslen(name);
    if (name[len - 1] == '\\') {
        name[len++] = '*';
        name[len] = 0;
    }
    d->handle = _wfindfirst(name, &d->info);
    if (d->handle == -1) {
        err = decode_error(errno);
        qemu_free(d);
        return err;
    }
    s->arg[0] = add_dir_cache_entry(s, d);
#else
    DPRINTF("Opening %s\n", name);
    len = strlen(name) - 1;
    while (len > 0 && name[len] != '/')
      len--;
    name[len++] = 0;
    strcpy(d->path, name);
    strcpy(d->pattern, name + len);
    d->dir = opendir(name);
    if (d->pattern[0] == '*' && d->pattern[1] == 0)
        d->pattern[0] = 0;
    if (!d->dir) {
        return decode_error(errno);
    }
    s->arg[0] = add_dir_cache_entry(s, d);
    DPRINTF("Handle %d\n", s->arg[0]);
#endif
    return HOST_FS_SUCCESS;
}

static int hostfs_file_close(syborg_hostfs_state *s)
{
    int handle = s->arg[0];
    int fd;
    int err;

    err = get_file_cache_entry(s, handle, &fd);
    if (err)
        return err;
    err = close(fd);
    if (err)
        return decode_error(err);
    remove_file_cache_entry(s, handle);

    return HOST_FS_SUCCESS;
}

static int hostfs_dir_close(syborg_hostfs_state *s)
{
    int handle = s->arg[0];
    hostfs_dir *d;
    int err;

    err = get_dir_cache_entry(s, handle, &d);
    if (err)
        return err;
#ifdef _WIN32
    _findclose(d->handle);
#else
    closedir(d->dir);
#endif
    qemu_free(d);
    remove_dir_cache_entry(s, handle);

    return HOST_FS_SUCCESS;
}

static int hostfs_dir_read(syborg_hostfs_state *s)
{
    int handle = s->arg[0];
    hostfs_dir *d;
    int err;

    err = get_dir_cache_entry(s, handle, &d);
    if (err)
        return err;
#ifdef _WIN32
    {
    int name_len;
    if (d->handle == -1) {
        return HOST_FS_EOF;
    }
    name_len = wcslen(d->info.name);
    if (name_len >= s->arg[2])
        return HOST_FS_TOO_BIG;
    cpu_physical_memory_write(s->arg[1], (void *)d->info.name,
                              (name_len + 1) * 2);
    s->arg[3] = name_len;

    s->arg[0] = 0;
    if (d->info.attrib & _A_RDONLY)
        s->arg[0] |= HOSTFS_ATTR_READONLY;
    if (d->info.attrib & _A_HIDDEN)
        s->arg[0] |= HOSTFS_ATTR_HIDDEN;
    if (d->info.attrib & _A_SUBDIR)
        s->arg[0] |= HOSTFS_ATTR_DIRECTORY;
    s->arg[1] = d->info.time_write;
    s->arg[2] = d->info.size;

    err = _wfindnext(d->handle, &d->info);
    if (err)
        d->handle = -1;
    }
#else
    {
    struct dirent *de;
    uint16_t unicode_name[HOSTFS_PATH_MAX];
    char full_name[HOSTFS_PATH_MAX];
    char *inp;
    char *outp;
    size_t outbytes;
    size_t inbytes;
    struct stat stat_buf;

    de = NULL;
    while (!de) {
        de = readdir(d->dir);
        if (!de) {
            err = errno;
            if (err == EAGAIN)
                return HOST_FS_EOF;
            return decode_error(errno);
        }
        if (d->pattern[0] && fnmatch(d->pattern, de->d_name, 0))
            de = NULL;
    }
    inp = de->d_name;
    DPRINTF("dirent %s\n", de->d_name);
    outp = (char *)unicode_name;
    inbytes = strlen(inp) + 1;
    outbytes = HOSTFS_PATH_MAX;
    err = iconv(s->iconv_host_to_guest, &inp, &inbytes, &outp, &outbytes);
    if (err == -1)
        return decode_error(errno);
    outbytes = HOSTFS_PATH_MAX - outbytes;
    if (outbytes > s->arg[2] * 2)
        return HOST_FS_TOO_BIG;
    cpu_physical_memory_write(s->arg[1], (void *)unicode_name, outbytes);
    s->arg[3] = (outbytes >> 1) - 1;

    snprintf(full_name, HOSTFS_PATH_MAX, "%s/%s", d->path, de->d_name);
    err = lstat(full_name, &stat_buf);
    if (err < 0)
        return decode_error(errno);

    s->arg[0] = hostfs_map_file_att(stat_buf.st_mode); /*  attributes */
    s->arg[1] = stat_buf.st_mtime;              /*  modified time */
    s->arg[2] = stat_buf.st_size;               /*  file size */
    }
#endif
    return HOST_FS_SUCCESS;
}

static int hostfs_file_read(syborg_hostfs_state *s)
{
    uint8_t buf[0x1000];
    int handle = s->arg[0];
    int pos = s->arg[1];
    uint32_t addr = s->arg[2];
    int len = s->arg[3];
    int fd;
    int bit;
    int err;

    err = get_file_cache_entry(s, handle, &fd);
    if (err)
        return err;

    err = lseek(fd, pos, SEEK_SET);
    if (err == -1)
      return decode_error(errno);

    while (len) {
        if (len > 0x1000)
            bit = 0x1000;
        else
            bit = len;
        bit = uninterrupted_read(fd, buf, bit);
        if (bit == -1) {
            s->arg[0] = -1;
            return decode_error(errno);
        }
        if (bit == 0)
            break;
        cpu_physical_memory_write(addr, buf, bit);
        addr += bit;
        len -= bit;
    }
    s->arg[0] = s->arg[3] - len;

    return HOST_FS_SUCCESS;
}

static int hostfs_file_write(syborg_hostfs_state *s)
{
    uint8_t buf[0x1000];
    int handle = s->arg[0];
    int pos = s->arg[1];
    uint32_t addr = s->arg[2];
    int len = s->arg[3];
    int fd;
    int bit;
    int err;

    err = get_file_cache_entry(s, handle, &fd);
    if (err)
        return err;

    err = lseek(fd, pos, SEEK_SET);
    if (err == -1)
      return decode_error(errno);

    while (len) {
        if (len > 0x1000)
            bit = 0x1000;
        else
            bit = len;
        cpu_physical_memory_read(addr, buf, bit);
        bit = uninterrupted_write(fd, buf, bit);
        if (bit == -1)
            return decode_error(errno);
        if (bit == 0)
            break;
        addr += bit;
        len -= bit;
    }
    s->arg[0] = s->arg[3] - len;

    return HOST_FS_SUCCESS;
}

static int hostfs_set_size(syborg_hostfs_state * s)
{
    int handle = s->arg[0];
    int fd;
    int err;

    err = get_file_cache_entry(s, handle, &fd);
    if (err)
        return err;

    err = ftruncate(fd, s->arg[1]);
    if (err)
        return decode_error(errno);

    return HOST_FS_SUCCESS;
}

static int hostfs_flush(syborg_hostfs_state * s)
{
    int handle = s->arg[0];
    int fd;
    int err;

    err = get_file_cache_entry(s, handle, &fd);
    if (err)
        return err;

#ifdef _WIN32
    FlushFileBuffers((HANDLE)_get_osfhandle(fd));
#else
    fsync(fd);
#endif

    return HOST_FS_SUCCESS;
}

static syborg_hostfs_op_fn syborg_hostfs_ops[] =
{
    hostfs_unsupported,          /*  EDummy */
        
    /*  CMountCB operations */
    hostfs_mkdir,                /*  EMkDir, */
    hostfs_rmdir,                /*  ERmDir, */
    hostfs_delete,               /*  EDelete, */
    hostfs_rename,               /*  ERename, */
    hostfs_replace,              /*  EReplace, */
    hostfs_unsupported,          /*  EReadUid, */
    hostfs_entry,                /*  EEntry, */
    hostfs_set_entry,            /*  EEntry, */
    hostfs_file_open,            /*  EFileOpen, */
    hostfs_dir_open,             /*  EDirOpen, */
    
    /*  CFileCB operations */
    hostfs_file_close,           /*  EFileClose,  */
    hostfs_file_read,            /*  EFileRead, */
    hostfs_file_write,           /*  EFileWrite, */
    hostfs_set_size,             /*  EFileSetSize, */
    hostfs_flush,                /*  EFileFlushAll, */

    /*  CMountCB operations */
    hostfs_dir_close,            /*  EDirClose, */
    hostfs_dir_read,             /*  EDirRead, */
};

static uint32_t syborg_hostfs_read(void *opaque, target_phys_addr_t offset)
{
    syborg_hostfs_state *s = (syborg_hostfs_state *)opaque;
    
    offset &= 0xfff;
    DPRINTF("read 0x%x\n", (int)offset);
    switch(offset >>2) {
    case HOSTFS_ID:
        return SYBORG_ID_HOSTFS;
    case HOSTFS_COMMAND:
        return s->command;
    case HOSTFS_RESULT:
        return s->result;
    case HOSTFS_ARG0:
        return s->arg[0];
    case HOSTFS_ARG1:
        return s->arg[1];
    case HOSTFS_ARG2:
        return s->arg[2];
    case HOSTFS_ARG3:
        return s->arg[3];

    default:
        cpu_abort(cpu_single_env, "syborg_hostfs_read: Bad offset %x\n",
                  (int)offset);
        return 0;  
    }
}

static void syborg_hostfs_write(void *opaque, target_phys_addr_t offset,
                                uint32_t value)
{
    syborg_hostfs_state *s = (syborg_hostfs_state *)opaque;
    
    offset &= 0xfff;
    DPRINTF("Write 0x%x=0x%x\n", (int)offset, value);
    switch (offset >> 2) {
    case HOSTFS_COMMAND:
        s->command = value;
        if (s->command >= ARRAY_SIZE(syborg_hostfs_ops)) {
            DPRINTF("Bad command %d\n", s->command);
            s->result = HOST_FS_UNSUPPORTED;
        } else {
            s->result = syborg_hostfs_ops[s->command](s);
            DPRINTF("Result %d\n", s->result);
        }
        break;
    case HOSTFS_RESULT:
        s->result = value;
        break;
    case HOSTFS_ARG0:
        s->arg[0] = value;
        break;
    case HOSTFS_ARG1:
        s->arg[1] = value;
        break;
    case HOSTFS_ARG2:
        s->arg[2] = value;
        break;
    case HOSTFS_ARG3:
        s->arg[3] = value;
        break;
    default:
        cpu_abort(cpu_single_env, "syborg_hostfs_write: Bad offset %x\n",
                  (int)offset);
        break;
    }
}

static CPUReadMemoryFunc *syborg_hostfs_readfn[] = {
     syborg_hostfs_read,
     syborg_hostfs_read,
     syborg_hostfs_read
};

static CPUWriteMemoryFunc *syborg_hostfs_writefn[] = {
     syborg_hostfs_write,
     syborg_hostfs_write,
     syborg_hostfs_write
};

static void syborg_hostfs_reset(void *opaque)
{
    syborg_hostfs_state *s = opaque;
    hostfs_handle_cache *p;
    s->command = 0;
    s->arg[0] = s->arg[1] = s->arg[2] = s->arg[3] = 0;
    s->result = 0;
    /* Close all open handles.  */
    for (p = s->handle_cache; p; p = p->next) {
        if (p->is_fd) {
            close(p->val.fd);
        } else {
#ifdef _WIN32
            _findclose(p->val.d->handle);
#else
            closedir(p->val.d->dir);
#endif
            qemu_free(p->val.d);
        }
        if (!p->next) {
            p->next = s->free_handle;
            s->free_handle = s->handle_cache;
            break;
        }
    }
}

static void syborg_hostfs_save(QEMUFile *f, void *opaque)
{
    syborg_hostfs_state *s = opaque;

    qemu_put_be32(f, s->command);
    qemu_put_be32(f, s->result);
    qemu_put_be32(f, s->arg[0]);
    qemu_put_be32(f, s->arg[1]);
    qemu_put_be32(f, s->arg[2]);
    qemu_put_be32(f, s->arg[3]);
    /* If the guest has any handles open then restoring state is
       probably going to break stuff.  */
    qemu_put_be32(f, s->handle_cache != NULL);
}

static int syborg_hostfs_load(QEMUFile *f, void *opaque, int version_id)
{
    syborg_hostfs_state *s = opaque;
    int broken;

    if (version_id != 1)
        return -EINVAL;

    /* Reset the device to clear out any open handles.  */
    syborg_hostfs_reset(s);
    s->command = qemu_get_be32(f);
    s->result = qemu_get_be32(f);
    s->arg[0] = qemu_get_be32(f);
    s->arg[1] = qemu_get_be32(f);
    s->arg[2] = qemu_get_be32(f);
    s->arg[3] = qemu_get_be32(f);
    broken = qemu_get_be32(f);
    if (broken) {
        fprintf(stderr, "syborg_hostfs: Open files lost after restore\n");
        s->result = HOST_FS_GENERAL_ERROR;
    }
    return 0;
}

static void syborg_hostfs_create(QEMUDevice *dev)
{
    syborg_hostfs_state *s;
    int drive;
    s = (syborg_hostfs_state *)qemu_mallocz(sizeof(syborg_hostfs_state));
    s->qdev = dev;
    qdev_set_opaque(dev, s);
    drive = qdev_get_property_int(dev, "drive-number");
    if (drive == 0 || drive > 26) {
        fprintf(stderr, "syborg_hostfs: Bad drive-number");
        exit(1);
    }
    s->drive_letter = drive + 'A' - 1;
    {
    int i;
#ifdef _WIN32
    const char *str = qdev_get_property_string(dev, "host-path");
    int len = _mbslen(str);
    s->host_prefix = (wchar_t *)qemu_mallocz((len + 1) * 2);
    mbstowcs(s->host_prefix, str, len + 1);
    s->host_prefix_len = wcslen(s->host_prefix);
    for (i = 0; i < s->host_prefix_len; i++) {
        if (s->host_prefix[i] == '/')
            s->host_prefix[i] = '\\';
    }
#else
    s->host_prefix = qemu_strdup(qdev_get_property_string(dev, "host-path"));
    s->host_prefix_len = strlen(s->host_prefix);
    for (i = 0; i < s->host_prefix_len; i++) {
        if (s->host_prefix[i] == '\\')
            s->host_prefix[i] = '/';
    }
    setlocale(LC_ALL, "");
    DPRINTF("Host charset: %s\n", nl_langinfo(CODESET));
    s->iconv_guest_to_host = iconv_open(nl_langinfo(CODESET), "UTF-16LE");
    s->iconv_host_to_guest = iconv_open("UTF-16LE", nl_langinfo(CODESET));
#endif
    }
}

void syborg_hostfs_register(void)
{
    QEMUDeviceClass *dc;
    dc = qdev_new("syborg,hostfs", syborg_hostfs_create, 0);
    qdev_add_registers(dc, syborg_hostfs_readfn, syborg_hostfs_writefn, 0x1000);
    qdev_add_property_int(dc, "drive-number", 14);
    qdev_add_property_string(dc, "host-path", "./");
    qdev_add_savevm(dc, 1, syborg_hostfs_save, syborg_hostfs_load);
}
