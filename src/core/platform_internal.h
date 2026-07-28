#ifndef PLATFORM_INTERNAL_H
#define PLATFORM_INTERNAL_H

#ifdef _WIN32
#include <windows.h>

/* ── Mutex ── */
typedef SRWLOCK ctypr_mutex_t;
#define MUTEX_INIT(m)   InitializeSRWLock(m)
#define MUTEX_LOCK(m)   AcquireSRWLockExclusive(m)
#define MUTEX_UNLOCK(m) ReleaseSRWLockExclusive(m)
#define MUTEX_DESTROY(m) ((void)0)

/* ── Threads ── */
typedef HANDLE ctypr_thread_t;
#define THREAD_RETURN DWORD WINAPI
#define THREAD_FUNC(name) DWORD WINAPI name(LPVOID arg)
#define THREAD_CREATE(t, func, arg) do { \
    *(t) = CreateThread(NULL, 0, func, arg, 0, NULL); \
} while(0)
#define THREAD_JOIN(t) WaitForSingleObject(t, INFINITE)

/* strdup is not in C17 on MSVC */
#ifndef strdup
#define strdup _strdup
#endif

#else
#include <pthread.h>

/* ── Mutex ── */
typedef pthread_mutex_t ctypr_mutex_t;
#define MUTEX_INIT(m)   pthread_mutex_init(m, NULL)
#define MUTEX_LOCK(m)   pthread_mutex_lock(m)
#define MUTEX_UNLOCK(m) pthread_mutex_unlock(m)
#define MUTEX_DESTROY(m) pthread_mutex_destroy(m)

/* ── Threads ── */
typedef pthread_t ctypr_thread_t;
#define THREAD_RETURN void*
#define THREAD_FUNC(name) void* name(void* arg)
#define THREAD_CREATE(t, func, arg) pthread_create(t, NULL, func, arg)
#define THREAD_JOIN(t) pthread_join(t, NULL)

#endif

#endif
