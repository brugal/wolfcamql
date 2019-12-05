#ifndef cg_thread_h_included
#define cg_thread_h_included

#ifdef _WIN32

// can't include <windows.h> because of conflicts
//#include <windows.h>

typedef struct thread_t {
    void *handle;
    void *(*proc)(void *);
    void *arg;
    unsigned int identifier;
} thread_t;

typedef struct thread_attr_t {
} thread_attr_t;

typedef struct thread_mutex_t {
    //CRITICAL_SECTION cs;
    void *cs;
} thread_mutex_t;

typedef struct thread_mutexattr_t {
} thread_mutexattr_t;

typedef struct semaphore_t {
    void *s;
} semaphore_t;

#else // ifdef _WIN32

#include <pthread.h>
#include <semaphore.h>

typedef pthread_t thread_t;
typedef pthread_attr_t thread_attr_t;
typedef pthread_mutex_t thread_mutex_t;
typedef pthread_mutexattr_t thread_mutexattr_t;

typedef struct semaphore_t {
    sem_t *s;
} semaphore_t;



#endif  // ifdef _WIN32

// all return 0 for success

int thread_create (thread_t *thread, const thread_attr_t *attr, void *(*start_routine)(void *), void *arg);

#define thread_exit(arg)  return arg

int thread_join (thread_t thread, void **retval);

int thread_mutex_init (thread_mutex_t *mutex, const thread_mutexattr_t *attr);
int thread_mutex_destroy (thread_mutex_t *mutex);
int thread_mutex_lock (thread_mutex_t *mutex);
int thread_mutex_unlock (thread_mutex_t *mutex);

int semaphore_init (semaphore_t *sem, unsigned int value);
int semaphore_destroy (semaphore_t *sem);
int semaphore_wait (semaphore_t *sem);
int semaphore_post (semaphore_t *sem);

#endif  // cg_thread_h_included
