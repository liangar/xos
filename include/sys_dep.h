#pragma once

#ifdef WIN32

#ifndef _SYSINCLUDEWIN_H
#define _SYSINCLUDEWIN_H

#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <wchar.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <direct.h>
#include <string.h>
#include <stdlib.h>
#include <process.h>
#include <malloc.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>
#include <limits.h>
#include <crtdbg.h>
#endif

#else				// #ifdef WIN32

#ifndef _SYSINCLUDELINUX_H
#define _SYSINCLUDELINUX_H

#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/sendfile.h>
#include <sys/poll.h>
#include <sys/mman.h>
#include <sys/vfs.h>
#include <sys/sysinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <time.h>
#include <utime.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <dirent.h>
#include <stdarg.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <syslog.h>
#include <dlfcn.h>
#include <sched.h>
#include <pthread.h>

#ifdef HAS_EVENTFD
#include <sys/eventfd.h>
#endif

#endif

#endif
