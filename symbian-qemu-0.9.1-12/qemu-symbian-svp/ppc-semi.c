/*
 *  PowerPC Semihosting syscall interface.
 *  Implements a subset of NetBSD syscalls.
 *
 *  Copyright (c) 2007 CodeSourcery.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>

#include "cpu.h"
#include "qemu-common.h"
#include "sysemu.h"
#include "gdbstub.h"
#include "softmmu-semi.h"

#define HOSTED_EXIT     1
#define HOSTED_READ     3
#define HOSTED_WRITE    4
#define HOSTED_OPEN     5
#define HOSTED_CLOSE    6
#define HOSTED_UNLINK	10
#define HOSTED_BRK      17
#define HOSTED_ACCESS   33
#define HOSTED_DUP      41
/* This is not quite right: 54 is ioctl, but the only thing we use ioctl
   for right now is to implement isatty.  Their argument lists are
   compatible as far as the first argument goes, so we just fake it and
   declare ioctl's syscall number as isatty's.  */
#define HOSTED_ISATTY	54
#define HOSTED_SYSTEM	77      /* obsolete vlimit */
#define HOSTED_GETTIMEOFDAY 116
#define HOSTED_RENAME	128
#define HOSTED_LSEEK    199
#define HOSTED_STAT	387
#define HOSTED_FSTAT	388

typedef uint32_t gdb_mode_t;
typedef uint32_t gdb_time_t;

struct ppc_gdb_stat {
  uint32_t    gdb_st_dev;     /* device */
  uint32_t    gdb_st_ino;     /* inode */
  gdb_mode_t  gdb_st_mode;    /* protection */
  uint32_t    gdb_st_nlink;   /* number of hard links */
  uint32_t    gdb_st_uid;     /* user ID of owner */
  uint32_t    gdb_st_gid;     /* group ID of owner */
  uint32_t    gdb_st_rdev;    /* device type (if inode device) */
  uint64_t    gdb_st_size;    /* total size, in bytes */
  uint64_t    gdb_st_blksize; /* blocksize for filesystem I/O */
  uint64_t    gdb_st_blocks;  /* number of blocks allocated */
  gdb_time_t  gdb_st_atime;   /* time of last access */
  gdb_time_t  gdb_st_mtime;   /* time of last modification */
  gdb_time_t  gdb_st_ctime;   /* time of last change */
} __attribute__((packed));

struct ppc_netbsd_stat {
  uint32_t nbsd_st_dev;         /* device */
  uint32_t nbsd_st_mode;        /* protection */
  uint64_t nbsd_st_ino;         /* inode */
  uint32_t nbsd_st_nlink;       /* number of hard links */
  uint32_t nbsd_st_uid;         /* user ID of owner */
  uint32_t nbsd_st_gid;         /* group ID of owner */
  uint32_t nbsd_st_rdev;        /* device type */

  int32_t nbsd_st_atime;        /* time of last access */
  int32_t nbsd_st_atimensec;
  int32_t nbsd_st_mtime;        /* time of last modification */
  int32_t nbsd_st_mtimensec;
  int32_t nbsd_st_ctime;        /* time of last change */
  int32_t nbsd_st_ctimensec;

  uint64_t nbsd_st_size;        /* total size, in bytes */
  int64_t nbsd_st_blocks;       /* number of blocks allocated */
  uint32_t nbsd_st_blksize;      /* blocksize for filesystem I/O */
  uint32_t nbsd_st_flags;       /* user-defined flags */
  uint32_t nbsd_st_gen;         /* filesystem generation number */
  uint32_t nbsd_st_spare[2];
};

struct gdb_timeval {
  gdb_time_t tv_sec;  /* second */
  uint64_t tv_usec;   /* microsecond */
} __attribute__((packed));

#define TARGET_O_RDONLY   0
#define TARGET_O_WRONLY   1
#define TARGET_O_RDWR     2
#define TARGET_O_ACCMODE  3
#define TARGET_O_APPEND   0x0008
#define TARGET_O_CREAT    0x0200
#define TARGET_O_TRUNC    0x0400
#define TARGET_O_EXCL     0x0800

#define GDB_O_RDONLY   0x0
#define GDB_O_WRONLY   0x1
#define GDB_O_RDWR     0x2
#define GDB_O_APPEND   0x8
#define GDB_O_CREAT  0x200
#define GDB_O_TRUNC  0x400
#define GDB_O_EXCL   0x800

static int translate_openflags_gdb(int flags)
{
    int gf;

    if ((flags & TARGET_O_ACCMODE) == TARGET_O_WRONLY)
        gf = GDB_O_WRONLY;
    else if ((flags & TARGET_O_ACCMODE) == TARGET_O_RDWR)
        gf = GDB_O_RDWR;
    else
        gf = GDB_O_RDONLY;

    if (flags & TARGET_O_APPEND) gf |= GDB_O_APPEND;
    if (flags & TARGET_O_CREAT) gf |= GDB_O_CREAT;
    if (flags & TARGET_O_TRUNC) gf |= GDB_O_TRUNC;
    if (flags & TARGET_O_EXCL) gf |= GDB_O_EXCL;

    return gf;
}

static int translate_openflags_host(int flags)
{
    int hf;

    if ((flags & TARGET_O_ACCMODE) == TARGET_O_WRONLY)
        hf = O_WRONLY;
    else if ((flags & TARGET_O_ACCMODE) == TARGET_O_RDWR)
        hf = O_RDWR;
    else
        hf = O_RDONLY;

    if (flags & TARGET_O_APPEND) hf |= O_APPEND;
    if (flags & TARGET_O_CREAT) hf |= O_CREAT;
    if (flags & TARGET_O_TRUNC) hf |= O_TRUNC;
    if (flags & TARGET_O_EXCL) hf |= O_EXCL;

    return hf;
}

static void translate_to_netbsd_stat(CPUState *env, target_ulong addr,
                                     struct stat *s, target_ulong saddr)
{
    struct ppc_netbsd_stat *p;

    if (!(p = lock_user(VERIFY_WRITE, addr, sizeof(*p), 0)))
        /* FIXME - should this return an error code? */
        return;

    if (s) {
      p->nbsd_st_dev = cpu_to_be32(s->st_dev);
      p->nbsd_st_ino = cpu_to_be64(s->st_ino);
      p->nbsd_st_mode = cpu_to_be32(s->st_mode);
      p->nbsd_st_nlink = cpu_to_be32(s->st_nlink);
      p->nbsd_st_uid = cpu_to_be32(s->st_uid);
      p->nbsd_st_gid = cpu_to_be32(s->st_gid);
      p->nbsd_st_rdev = cpu_to_be32(s->st_rdev);
      p->nbsd_st_size = cpu_to_be64(s->st_size);
#ifdef _WIN32
      /* Windows stat is missing some fields.  */
      p->nbsd_st_blksize = 0;
      p->nbsd_st_blocks = 0;
#else
      p->nbsd_st_blksize = cpu_to_be32(s->st_blksize);
      p->nbsd_st_blocks = cpu_to_be64(s->st_blocks);
#endif
      p->nbsd_st_atime = cpu_to_be32(s->st_atime);
      p->nbsd_st_mtime = cpu_to_be32(s->st_mtime);
      p->nbsd_st_ctime = cpu_to_be32(s->st_ctime);
    } else {
      struct ppc_gdb_stat *g;

      if (g = lock_user (VERIFY_READ, saddr, sizeof(*g), 1)) {
        p->nbsd_st_dev = cpu_to_be32(g->gdb_st_dev);
        p->nbsd_st_ino = cpu_to_be64(g->gdb_st_ino);
        p->nbsd_st_mode = cpu_to_be32(g->gdb_st_mode);
        p->nbsd_st_nlink = cpu_to_be32(g->gdb_st_nlink);
        p->nbsd_st_uid = cpu_to_be32(g->gdb_st_uid);
        p->nbsd_st_gid = cpu_to_be32(g->gdb_st_gid);
        p->nbsd_st_rdev = cpu_to_be32(g->gdb_st_rdev);
        p->nbsd_st_size = cpu_to_be64(g->gdb_st_size);
        p->nbsd_st_blksize = cpu_to_be32(g->gdb_st_blksize);
        p->nbsd_st_blocks = cpu_to_be64(g->gdb_st_blocks);
        p->nbsd_st_atime = cpu_to_be32(g->gdb_st_atime);
        p->nbsd_st_mtime = cpu_to_be32(g->gdb_st_mtime);
        p->nbsd_st_ctime = cpu_to_be32(g->gdb_st_ctime);

        unlock_user (g, saddr, 0);
      }
    }

    unlock_user(p, addr, sizeof(*p));
}

#define ARG(n) env->gpr[3 + (n)]

static void ppc_semi_cb(CPUState *env, target_ulong ret, target_ulong err)
{
    if (ret == -1) {
        env->gpr[3] = errno;
        env->crf[0] |= 1;
    } else {
        env->gpr[3] = ret;
        env->crf[0] &= ~1;
    }
}

static void ppc_stat_cb(CPUState *env, target_ulong ret, target_ulong err)
{
  /* Do this before we overwrite r3 with the return value.  */
  if (ret != -1) {
    target_ulong buffer = env->gpr[1] - sizeof(struct ppc_gdb_stat);
    translate_to_netbsd_stat (env, ARG(1), NULL, buffer);
  }
  ppc_semi_cb(env, ret, err);
}

static void ppc_isatty_cb(CPUState *env, target_ulong ret, target_ulong err)
{
  /* isatty returns 0 or 1; translate that to system call-ish conventions.
     Don't bother propagating errno.  */
  env->gpr[3] = ret ? 0 : -1;
}

void do_ppc_semihosting(CPUPPCState *env)
{
    int nr;
    void *p, *q;
    uint32_t len;
    uint32_t result;

    nr = env->gpr[0];
    switch (nr) {
    case HOSTED_EXIT:
        /* FIXME: ideally we want to inform gdb about program
	   exit whenever gdb is connected, even if syscalls
	   are not handled by gdb.  */
        if (use_gdb_syscalls())
            gdb_exit(env, ARG(0));
        exit(ARG(0));
    case HOSTED_OPEN:
        if (use_gdb_syscalls()) {
            gdb_do_syscall(ppc_semi_cb, "open,%s,%x,%x",
                           ARG(0), target_strlen(ARG(0)),
                           translate_openflags_gdb(ARG(1)), ARG(2));
            return;
        } else {
            if (!(p = lock_user_string(ARG(0)))) {
                /* FIXME - check error code? */
                result = -1;
            } else {
	        result = uninterrupted_open(p, translate_openflags_host(ARG(1)), ARG(2));
                unlock_user(p, ARG(0), 0);
            }
        }
        break;
    case HOSTED_CLOSE:
        {
            /* Ignore attempts to close stdin/out/err.  */
            int fd = ARG(0);
            if (fd > 2) {
                if (use_gdb_syscalls()) {
                    gdb_do_syscall(ppc_semi_cb, "close,%x", ARG(0));
                    return;
                } else {
		    result = uninterrupted_close(fd);
                }
            } else {
                result = 0;
            }
            break;
        }
    case HOSTED_READ:
        len = ARG(2);
        if (use_gdb_syscalls()) {
            gdb_do_syscall(ppc_semi_cb, "read,%x,%x,%x",
                           ARG(0), ARG(1), len);
            return;
        } else {
            if (!(p = lock_user(VERIFY_WRITE, ARG(1), len, 0))) {
                /* FIXME - check error code? */
                result = -1;
            } else {
	        result = uninterrupted_read(ARG(0), p, len);
                unlock_user(p, ARG(1), len);
            }
        }
        break;
    case HOSTED_WRITE:
        len = ARG(2);
        if (use_gdb_syscalls()) {
            gdb_do_syscall(ppc_semi_cb, "write,%x,%x,%x",
                           ARG(0), ARG(1), len);
            return;
        } else {
            if (!(p = lock_user(VERIFY_READ, ARG(1), len, 1))) {
                /* FIXME - check error code? */
                result = -1;
            } else {
	        result = uninterrupted_write(ARG(0), p, len);
                unlock_user(p, ARG(0), 0);
            }
        }
        break;
    case HOSTED_UNLINK:
      if (use_gdb_syscalls()) {
        gdb_do_syscall(ppc_semi_cb, "unlink,%s",
                       ARG(0), (int)ARG(1));
        return;
      } else {
        if (!(p = lock_user_string(ARG(0)))) {
          /* FIXME - check error code? */
          result = -1;
        } else {
          result = unlink(p);
          unlock_user(p, ARG(0), 0);
        }
      }
      break;
    case HOSTED_BRK:
        /* Nasty hack: use sp -1M as heap limit. */
        if (ARG(0) < env->gpr[1] - 0x100000) {
            result = 0;
        } else {
            result = -1;
            errno = ENOMEM;
        }
        break;
    case HOSTED_LSEEK:
        {
          uint64_t off;
          off = (uint32_t)ARG(2) | ((uint64_t)ARG(1) << 32);
          if (use_gdb_syscalls()) {
            gdb_do_syscall(ppc_semi_cb, "lseek,%x,%lx,%x",
                           ARG(0), off, ARG(3));
            return;
          } else {
            off = lseek(ARG(0), off, ARG(3));
            if (off == (uint64_t) -1) {
              result = -1;
            } else {
              env->gpr[3] = off >> 32;
              env->gpr[4] = (uint32_t) off;
              env->crf[0] &= ~1;
              return;
            }
          }
        }
        break;
    case HOSTED_STAT:
      if (use_gdb_syscalls()) {
        target_ulong buffer = env->gpr[1] - sizeof (struct ppc_gdb_stat);
        gdb_do_syscall(ppc_stat_cb, "stat,%s,%x", ARG(0), buffer);
        return;
      } else {
        struct stat s;
        if (!(p = lock_user_string(ARG(0)))) {
          /* FIXME - check error code? */
          result = -1;
        } else {
          result = stat(p, &s);
          unlock_user(p, ARG(0), 0);
        }
        if (result == 0) {
          translate_to_netbsd_stat(env, ARG(1), &s, 0);
        }
      }
      break;
    case HOSTED_FSTAT:
      if (use_gdb_syscalls()) {
        target_ulong buffer = env->gpr[1] - sizeof (struct ppc_gdb_stat);
        gdb_do_syscall(ppc_stat_cb, "fstat,%x,%x", ARG(0), buffer);
        return;
      } else {
        struct stat s;
        result = fstat(ARG(0), &s);
        if (result == 0) {
          translate_to_netbsd_stat(env, ARG(1), &s, 0);
        }
      }
      break;
    case HOSTED_RENAME:
      if (use_gdb_syscalls()) {
        gdb_do_syscall(ppc_semi_cb, "rename,%s,%s",
                       ARG(0), (int)ARG(1), ARG(2), (int)ARG(3));
        return;
      } else {
        p = lock_user_string(ARG(0));
        q = lock_user_string(ARG(2));
        if (!p || !q) {
          /* FIXME - check error code? */
          result = -1;
        } else {
          result = rename(p, q);
        }
        unlock_user(p, ARG(0), 0);
        unlock_user(q, ARG(2), 0);
      }
      break;
    case HOSTED_ISATTY:
        if (use_gdb_syscalls()) {
            gdb_do_syscall(ppc_isatty_cb, "isatty,%x", ARG(0));
            return;
        } else {
            /* Don't bother propagating errno.  */
            env->gpr[3] = isatty(ARG(0)) ? 0 : -1;
            return;
        }
        break;
    case HOSTED_SYSTEM:
        if (use_gdb_syscalls()) {
            gdb_do_syscall(ppc_semi_cb, "system,%s",
                           ARG(0), (int)ARG(1));
            return;
        } else {
            if (!(p = lock_user_string(ARG(0)))) {
                /* FIXME - check error code? */
                result = -1;
            } else {
                result = system(p);
                unlock_user(p, ARG(0), 0);
            }
        }
        break;
    case HOSTED_GETTIMEOFDAY:
        result = -1;
        errno = ENOSYS;
#ifdef FIXME
        if (use_gdb_syscalls()) {
            gdb_do_syscall(ppc_semi_cb, "gettimeofday,%x,%x",
                           ARG(0), ARG(1));
            return;
        } else {
            qemu_timeval tv;
            struct gdb_timeval *p;
            result = qemu_gettimeofday(&tv);
            if (result != 0) {
                if (!(p = lock_user(VERIFY_WRITE,
                                    ARG(0), sizeof(struct gdb_timeval), 0))) {
                    /* FIXME - check error code? */
                    result = -1;
                } else {
                    p->tv_sec = cpu_to_be32(tv.tv_sec);
                    p->tv_usec = cpu_to_be64(tv.tv_usec);
                    unlock_user(p, ARG(0), sizeof(struct gdb_timeval));
                }
            }
        }
#endif
        break;
    default:
        cpu_abort(env, "Unsupported semihosting syscall %d\n", nr);
        result = 0;
    }
    if (result == -1) {
        env->gpr[3] = errno;
        env->crf[0] |= 1;
    } else {
        env->gpr[3] = result;
        env->crf[0] &= ~1;
    }
}
