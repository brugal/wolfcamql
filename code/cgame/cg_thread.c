#ifdef Q3_VM
#error only for native libraries
#endif

#include "cg_thread.h"

#ifdef _WIN32

#include <windows.h>
#include <process.h>
#include <stdlib.h>
//#include <excpt.h>

#endif  // ifdef _WIN32

#include <errno.h>

void Com_Printf( const char *msg, ... ) __attribute__ ((format (printf, 1, 2)));
int Com_sprintf (char *dest, int size, const char *fmt, ...) __attribute__ ((format (printf, 3, 4)));

#ifdef _WIN32

//static DWORD WINAPI genericThreadFunction (LPVOID lpParam)
static unsigned int __stdcall genericThreadFunction (void *lpParam)
{
    thread_t *thread = (thread_t *)lpParam;
    void *threadRet;
    unsigned int ret;

    threadRet = thread->proc(thread->arg);

    // 2019-11-27 threadRet is always NULL
    ret = (uintptr_t)threadRet % UINT_MAX;

    _endthreadex(ret);

    return ret;
}

int thread_create (thread_t *thread, const thread_attr_t *attr, void *(*start_routine)(void *), void *arg)
{

    thread->proc = start_routine;
    thread->arg = arg;

    thread->handle = (void *)_beginthreadex(NULL,
                                    0,  // stack size
                                    genericThreadFunction,
                                    thread,
                                    0,  // initflag
                                    &thread->identifier);

    if (!thread->handle) {
        Com_Printf("%s couldn't create thread  error: %d\n", __FUNCTION__, errno);
        return errno;
    }

    return 0;
}
#else // ifdef _WIN32

int thread_create (thread_t *thread, const thread_attr_t *attr, void *(*start_routine)(void *), void *arg)
{
    int r;

    r = pthread_create(thread, attr, start_routine, arg);
    if (r) {
        Com_Printf("%s couldn't create thread  error: %d\n", __FUNCTION__, r);
    }

    return r;
}

#endif  // ifdef _WIN32


// thread_exit as a #define

#ifdef _WIN32

int thread_join (thread_t thread, void **retval)
{
    DWORD r;

    if (!thread.handle) {
        Com_Printf("%s invalid handle\n", __FUNCTION__);
        return 1;
    }

    r = WaitForSingleObject(thread.handle, INFINITE);
    if (r) {
        Com_Printf("%s failed (0x%x) error: %ld\n", __FUNCTION__, (unsigned int)r, GetLastError());
        return r;
    }

    r = CloseHandle(thread.handle);
    if (!r) {
        Com_Printf("%s failed to close handle error: %ld\n", __FUNCTION__, GetLastError());
        return 1;
    }

    thread.handle = NULL;

    return 0;
}

#else  // ifdef _WIN32

int thread_join (thread_t thread, void **retval)
{
    int r;

    r = pthread_join(thread, retval);
    if (r) {
        Com_Printf("%s failed  error: %d\n", __FUNCTION__, r);
    }

    return r;
}

#endif // ifdef _WIN32

#ifdef _WIN32

int thread_mutex_init (thread_mutex_t *mutex, const thread_mutexattr_t *attr)
{
    if (mutex->cs) {
        Com_Printf("%s mutex %p already allocated\n", __FUNCTION__, mutex);
        return 1;
    }

#if 0
    mutex->cs = calloc(1, sizeof(CRITICAL_SECTION));
    if (!mutex->cs) {
        Com_Printf("%s couldn't allocate memory for mutex %p\n", __FUNCTION__, mutex);
        return 0;
    }

    {  //__try {
        InitializeCriticalSection((CRITICAL_SECTION *)&mutex->cs);
    }
    if (0) { //__except (EXCEPTION_EXECUTE_HANDLER) {
        if (GetExceptionCode() == STATUS_NO_MEMORY) {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            Com_Printf("InitializeCriticalSection no memory\n");
        }
    }
#endif

    mutex->cs = CreateMutex(NULL, FALSE, NULL);
    if (!mutex->cs) {
        Com_Printf("%s failed  error: %ld\n", __FUNCTION__, GetLastError());
        return 1;
    }

    return 0;
}

#else  // ifdef _WIN32

int thread_mutex_init (thread_mutex_t *mutex, const thread_mutexattr_t *attr)
{
    int r;

    r = pthread_mutex_init(mutex, attr);

    if (r) {
        Com_Printf("%s couldn't create mutex  error: %d\n", __FUNCTION__, r);
    }

    return r;
}

#endif  // ifdef _WIN32

#ifdef _WIN32

int thread_mutex_destroy (thread_mutex_t *mutex)
{
    BOOL r;

    if (!mutex->cs) {
        Com_Printf("%s invalid mutex %p\n", __FUNCTION__, mutex);
        return 1;
    }

#if 0
    free(mutex->cs);
    DeleteCriticalSection((CRITICAL_SECTION *)&mutex->cs);
#endif

    r = CloseHandle(mutex->cs);

    if (!r) {
        Com_Printf("%s failed  error: %ld\n", __FUNCTION__, GetLastError());
        return 1;
    }

    mutex->cs = NULL;

    return 0;
}

#else  // ifdef _WIN32

int thread_mutex_destroy (thread_mutex_t *mutex)
{
    int r;

    r = pthread_mutex_destroy(mutex);
    if (r) {
        Com_Printf("%s failed  error: %d\n", __FUNCTION__, r);
    }

    return r;
}

#endif  // ifdef _WIN32

#ifdef _WIN32

int thread_mutex_lock (thread_mutex_t *mutex)
{
    DWORD r;

    if (!mutex->cs) {
        Com_Printf("%s invalid mutex %p\n", __FUNCTION__, mutex);
        return 1;
    }

    //EnterCriticalSection((CRITICAL_SECTION *)&mutex->cs);
    r = WaitForSingleObject(mutex->cs, INFINITE);
    if (r) {
        Com_Printf("%s failed (0x%x) error: %ld\n", __FUNCTION__, (unsigned int)r, GetLastError());
        return r;
    }

    return r;
}

#else  // ifdef _WIN32

int thread_mutex_lock (thread_mutex_t *mutex)
{
    int r;

    r = pthread_mutex_lock(mutex);
    if (r) {
        Com_Printf("%s failed  error: %d\n", __FUNCTION__, r);
    }

    return r;
}

#endif  // ifdef _WIN32

#ifdef _WIN32
int thread_mutex_unlock (thread_mutex_t *mutex)
{
    BOOL r;

    if (!mutex->cs) {
        Com_Printf("%s invalid mutex %p\n", __FUNCTION__, mutex);
        return 1;
    }

    //LeaveCriticalSection((CRITICAL_SECTION *)&mutex->cs);
    r = ReleaseMutex(mutex->cs);
    if (!r) {
        Com_Printf("%s failed  error: %ld\n", __FUNCTION__, GetLastError());
        return 1;
    }

    return 0;
}

#else  // ifdef _WIN32

int thread_mutex_unlock (thread_mutex_t *mutex)
{
    int r;

    r = pthread_mutex_unlock(mutex);
    if (r) {
        Com_Printf("%s failed  error: %d\n", __FUNCTION__, r);
    }

    return r;
}

#endif  // ifdef _WIN32

#ifdef _WIN32

int semaphore_init (semaphore_t *sem, unsigned int value)
{
    sem->s = CreateSemaphore(NULL, (LONG)value, 500, NULL);
    if (!sem->s) {
        Com_Printf("%s failed  error: %ld\n", __FUNCTION__, GetLastError());
        return 1;
    }
    return 0;
}

#else  // ifdef _WIN32

/* sem_init() and sem_destroy() deprecated in Mac OS X.  Use sem_open(),
 * sem_close(), and sem_unlink() instead.
 */

#include <fcntl.h>
#include <sys/stat.h>

int semaphore_init (semaphore_t *sem, unsigned int value)
{
    int r;
    char semName[256];
    static int callNumber = 0;

    callNumber++;
    Com_sprintf(semName, 256, "/wcsem%d", callNumber);

    sem->s = sem_open(semName, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0);

    if (sem->s == SEM_FAILED) {
        Com_Printf("%s sem_open() failed  error: %d\n", __FUNCTION__, errno);
        return 1;
    }

    r = sem_unlink(semName);
    if (r != 0) {
        Com_Printf("%s sem_unlink() failed  error: %d\n", __FUNCTION__, errno);
        return 1;
    }

    return 0;
}

#endif  // ifdef _WIN32

#ifdef _WIN32

int semaphore_destroy (semaphore_t *sem)
{
    BOOL r;

    if (!sem->s) {
        Com_Printf("%s invalid semaphore\n", __FUNCTION__);
        return 1;
    }

    r = CloseHandle(sem->s);
    if (!r) {
        Com_Printf("%s failed to close handle error: %ld\n", __FUNCTION__, GetLastError());
        return 1;
    }

    sem->s = NULL;
    return 0;
}

#else  // ifdef _WIN32

int semaphore_destroy (semaphore_t *sem)
{
    int r;

    r = sem_close(sem->s);
    if (r != 0) {
        Com_Printf("%s failed  error: %d\n", __FUNCTION__, errno);
        return 1;
    }

    return r;
}

#endif  // ifdef _WIN32

#ifdef _WIN32

int semaphore_wait (semaphore_t *sem)
{
    DWORD r;

    r = WaitForSingleObject(sem->s, INFINITE);
    if (r) {
        Com_Printf("%s failed (0x%x) error: %ld\n", __FUNCTION__, (unsigned int)r, GetLastError());
        return r;
    }

    return r;
}

#else  // ifdef _WIN32

int semaphore_wait (semaphore_t *sem)
{
    int r;

    r = sem_wait(sem->s);
    if (r != 0) {
        Com_Printf("%s failed  error: %d\n", __FUNCTION__, errno);
        return 1;
    }

    return r;
}

#endif  // ifdef _WIN32

#ifdef _WIN32

int semaphore_post (semaphore_t *sem)
{
    BOOL r;

    r = ReleaseSemaphore(sem->s, 1, NULL);

    if (!r) {
        Com_Printf("%s failed -- error %ld\n", __FUNCTION__, GetLastError());
        return 1;
    }

    return 0;
}

#else  // ifdef _WIN32

int semaphore_post (semaphore_t *sem)
{
    int r;

    r = sem_post(sem->s);
    if (r != 0) {
        Com_Printf("%s failed  error: %d\n", __FUNCTION__, errno);
        return 1;
    }

    return r;
}

#endif  // ifdef _WIN32
