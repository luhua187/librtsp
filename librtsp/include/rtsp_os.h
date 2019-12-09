#ifndef __RTSP_OS__H__
#define __RTSP_OS__H__


#if defined (WIN32) || defined(_WIN32)  //windows windows windows
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <io.h>
#include <conio.h>
#include <process.h>
#include <fcntl.h>
#include <direct.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <sys/stat.h>
#include <assert.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <windows.h>


typedef char					int8;
typedef unsigned char			uint8;
typedef short					int16;
typedef unsigned short			uint16;
typedef int						int32;
typedef unsigned int			uint32;
typedef long long				int64;
typedef unsigned long long		uint64;
typedef float					float32;
typedef double					float64;

typedef HANDLE                  thread_t;
typedef HANDLE                   mutex_t;

static  void mutex_lock(mutex_t *mutex)
{
     WaitForSingleObject(*mutex, INFINITE);
}

static void mutex_unlock(mutex_t *mutex)
{
    ReleaseMutex(*mutex);
}

static void socket_close(int fd)
{
    closesocket(fd);
}

#define snprintf _snprintf
#define  inline __inline
#define  strtoll _strtoi64
#define  strdup  _strdup

static void sleep(int n)
{
	Sleep(1000*n);
}

#else            // linux linux linux  


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <termios.h>
#include <pthread.h>
#include <semaphore.h>
#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#include <malloc.h>
#include <mntent.h>
#include <dirent.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <linux/magic.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/vfs.h>
#include <sys/syscall.h>
#include <sys/un.h>
#include <sys/prctl.h>
#include <sys/ioctl.h>
#include <sys/epoll.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <errno.h>
#include <assert.h>


typedef char					int8;
typedef unsigned char			uint8;
typedef short					int16;
typedef unsigned short			uint16;
typedef int						int32;
typedef unsigned int			uint32;
typedef long long				int64;
typedef unsigned long long		uint64;
typedef float					float32;
typedef double					float64;


typedef pthread_t               thread_t;
typedef pthread_mutex_t         mutex_t;


static inline void mutex_lock(mutex_t *mutex)
{
     pthread_mutex_lock(mutex);
}

static inline void mutex_unlock(mutex_t *mutex)
{
    pthread_mutex_unlock(mutex);
}

static inline void socket_close(int fd)
{
    close(fd);
}

#endif

#endif