#ifndef UTILS_H
#define UTILS_H

#ifdef _WIN32
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#pragma comment(lib, "Ws2_32.lib")

typedef SOCKET socket_t;
#define close_socket(s) closesocket(s)

/* Thread abstractions */
typedef HANDLE thread_t;
static inline thread_t thread_create(LPTHREAD_START_ROUTINE func, void *arg) {
    return CreateThread(NULL, 0, func, arg, 0, NULL);
}
static inline void thread_detach(thread_t t) { CloseHandle(t); }
static inline void thread_sleep_ms(unsigned int ms) { Sleep(ms); }

/* Mutex and cond var using CRITICAL_SECTION + CONDITION_VARIABLE */
typedef CRITICAL_SECTION mutex_t;
static inline void mutex_init(mutex_t *m) { InitializeCriticalSection(m); }
static inline void mutex_lock(mutex_t *m) { EnterCriticalSection(m); }
static inline void mutex_unlock(mutex_t *m) { LeaveCriticalSection(m); }
static inline void mutex_destroy(mutex_t *m) { DeleteCriticalSection(m); }

typedef CONDITION_VARIABLE condvar_t;
static inline void cond_init(condvar_t *c) { InitializeConditionVariable(c); }
static inline void cond_wait(condvar_t *c, mutex_t *m) { SleepConditionVariableCS(c, m, INFINITE); }
static inline int cond_timedwait(condvar_t *c, mutex_t *m, DWORD ms) {
    return SleepConditionVariableCS(c, m, ms) ? 1 : 0;
}
static inline void cond_signal(condvar_t *c) { WakeConditionVariable(c); }
static inline void cond_broadcast(condvar_t *c) { WakeAllConditionVariable(c); }

#else
#error "This code targets Windows only."
#endif

#endif /* UTILS_H */
