/*
 *  XMail by Davide Libenzi ( Intranet and Internet mail server )
 *  Copyright (C) 1999,..,2004  Davide Libenzi
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Davide Libenzi <davidel@xmailserver.org>
 *
 */

#ifndef _SYSTYPESHPUX_H
#define _SYSTYPESHPUX_H

#ifdef MACH_BIG_ENDIAN_WORDS
#define BIG_ENDIAN_CPU
#endif
#ifdef MACH_BIG_ENDIAN_BITFIELD
#define BIG_ENDIAN_BITFIELD
#endif

#define SYS_INFINITE_TIMEOUT    (2 * 1024 * 1024)
#define SYS_DEFAULT_MAXCOUNT    (INT_MAX - 1)

#define SYS_SLASH_CHAR          '/'
#define SYS_SLASH_STR           "/"
#define SYS_BASE_FS_STR         ""
#define SYS_MAX_PATH            256

#define SYS_LLU_FMT             "%llu"
#define SYS_LLX_FMT             "%llX"

#define SYS_INVALID_HANDLE      ((SYS_HANDLE) 0)
#define SYS_INVALID_SOCKET      ((SYS_SOCKET) (-1))
#define SYS_INVALID_SEMAPHORE   ((SYS_SEMAPHORE) 0)
#define SYS_INVALID_MUTEX       ((SYS_MUTEX) 0)
#define SYS_INVALID_EVENT       ((SYS_EVENT) 0)
#define SYS_INVALID_THREAD      ((SYS_THREAD) 0)
#define SYS_INVALID_NET_ADDRESS ((NET_ADDRESS) INADDR_NONE)

#define SYS_THREAD_ONCE_INIT    PTHREAD_ONCE_INIT

#define SysSNPrintf             snprintf
#define stricmp                 strcasecmp
#define strnicmp                strncasecmp

#define SYS_fd_set              fd_set
#define SYS_FD_ZERO             FD_ZERO
#define SYS_FD_CLR              FD_CLR
#define SYS_FD_SET              FD_SET
#define SYS_FD_ISSET            FD_ISSET

typedef char SYS_INT8;
typedef unsigned char SYS_UINT8;
typedef short int SYS_INT16;
typedef unsigned short int SYS_UINT16;
typedef int32_t SYS_INT32;
typedef uint32_t SYS_UINT32;
typedef int SYS_INT64;
typedef uint64_t SYS_UINT64;
typedef unsigned long long int SYS_LONGLONG;
typedef unsigned int SYS_PTRUINT;
typedef unsigned long SYS_HANDLE;
typedef pthread_key_t SYS_TLSKEY;
typedef pthread_once_t SYS_THREAD_ONCE;
#undef SYS_SOCKET
typedef int SYS_SOCKET;
typedef void *SYS_SEMAPHORE;
typedef void *SYS_MUTEX;
typedef void *SYS_EVENT;
typedef void *SYS_THREAD;
typedef SYS_UINT32 NET_ADDRESS;

struct SYS_INET_ADDR {
	struct sockaddr_in Addr;
};

enum SysFileTypes {
	ftNormal = 1,
	ftDirectory,
	ftLink,
	ftOther,

	ftMax
};

struct SYS_FILE_INFO {
	int iFileType;
	unsigned long ulSize;
	time_t tMod;
};

#endif
