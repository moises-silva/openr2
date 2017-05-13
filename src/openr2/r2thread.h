/* 
 * Cross Platform Thread/Mutex abstraction
 * Copyright(C) 2007 Michael Jerris
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so.
 *
 * This work is provided under this license on an "as is" basis, without warranty of any kind,
 * either expressed or implied, including, without limitation, warranties that the covered code
 * is free of defects, merchantable, fit for a particular purpose or non-infringing. The entire
 * risk as to the quality and performance of the covered code is with you. Should any covered
 * code prove defective in any respect, you (not the initial developer or any other contributor)
 * assume the cost of any necessary servicing, repair or correction. This disclaimer of warranty
 * constitutes an essential part of this license. No use of any covered code is authorized hereunder
 * except under this disclaimer. 
 *
 * Contributors: 
 *
 * Moises Silva <moises.silva@gmail.com>
 * Arnaldo Pereira <arnaldo@sangoma.com>
 *
 */


#ifndef _R2THREAD_H
#define _R2THREAD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "r2declare.h"

#ifdef WIN32
#include <stdio.h>
#include <windows.h>
#include <process.h>
#include <inttypes.h>
#define OR2_INVALID_SOCKET INVALID_HANDLE_VALUE
#define OR2_THREAD_CALLING_CONVENTION __stdcall
typedef HANDLE openr2_socket_t;

struct openr2_mutex {
	CRITICAL_SECTION mutex;
};

#else /* WIN32 */
#define OR2_INVALID_SOCKET -1
typedef int openr2_socket_t;
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>

/* it seems some of the flags we use to compile (like -std=c99??) may cause some pthread defs to be missing
   should we get rid of those defs completely instead of defining __USE_UNIX98? */
#ifndef __USE_UNIX98
#define __USE_UNIX98
#endif
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <poll.h>

#define OR2_THREAD_CALLING_CONVENTION

struct openr2_mutex {
	pthread_mutex_t mutex;
};

#endif


typedef void *(*openr2_malloc_func_t)(void *pool, size_t len);
typedef void *(*openr2_calloc_func_t)(void *pool, size_t elements, size_t len);
typedef void *(*openr2_realloc_func_t)(void *pool, void *buff, size_t len);
typedef void (*openr2_free_func_t)(void *pool, void *ptr);
struct openr2_memory_handler {
    void *pool;
    openr2_malloc_func_t malloc;
    openr2_calloc_func_t calloc;
    openr2_realloc_func_t realloc;
    openr2_free_func_t free;
};

typedef struct openr2_memory_handler openr2_memory_handler_t;
extern openr2_memory_handler_t g_openr2_mem_handler;

#define openr2_malloc(chunksize) g_openr2_mem_handler.malloc(g_openr2_mem_handler.pool, chunksize)
#define openr2_realloc(buff, chunksize) g_openr2_mem_handler.realloc(g_openr2_mem_handler.pool, buff, chunksize)
#define openr2_calloc(elements, chunksize) g_openr2_mem_handler.calloc(g_openr2_mem_handler.pool, elements, chunksize)
#define openr2_free(chunk) g_openr2_mem_handler.free(g_openr2_mem_handler.pool, chunk)
#define openr2_safe_free(it) if (it) { openr2_free(it); it = NULL; }
#define openr2_array_len(array) sizeof(array)/sizeof(array[0])

typedef enum { OR2_SUCCESS, OR2_FAIL, OR2_TIMEOUT } openr2_status_t;

typedef struct openr2_mutex openr2_mutex_t;
typedef struct openr2_thread openr2_thread_t;
typedef struct openr2_interrupt openr2_interrupt_t;
typedef void *(*openr2_thread_function_t) (openr2_thread_t *, void *);

struct openr2_interrupt {
    openr2_socket_t device;
#ifdef WIN32
    /* for generic interruption */
    HANDLE event;
#else
    /* for generic interruption */
    int readfd;
    int writefd;
#endif
};

struct openr2_thread {
#ifdef WIN32
    void *handle;
#else
    pthread_t handle;
#endif
    void *private_data;
    openr2_thread_function_t function;
    size_t stack_size;
#ifndef WIN32
    pthread_attr_t attribute;
#endif
};

openr2_status_t openr2_thread_create_detached(openr2_thread_function_t func, void *data);
openr2_status_t openr2_thread_create_detached_ex(openr2_thread_function_t func, void *data, size_t stack_size);

openr2_status_t openr2_mutex_create(openr2_mutex_t **mutex);
openr2_status_t openr2_mutex_destroy(openr2_mutex_t **mutex);

#define openr2_mutex_lock(_x) _openr2_mutex_lock(_x)
openr2_status_t _openr2_mutex_lock(openr2_mutex_t *mutex);

#define openr2_mutex_trylock(_x) _openr2_mutex_trylock(_x)
openr2_status_t _openr2_mutex_trylock(openr2_mutex_t *mutex);

#define openr2_mutex_unlock(_x) _openr2_mutex_unlock(_x)
openr2_status_t _openr2_mutex_unlock(openr2_mutex_t *mutex);

openr2_status_t openr2_interrupt_create(openr2_interrupt_t **cond, openr2_socket_t device);
openr2_status_t openr2_interrupt_destroy(openr2_interrupt_t **cond);
openr2_status_t openr2_interrupt_signal(openr2_interrupt_t *cond);
openr2_status_t openr2_interrupt_wait(openr2_interrupt_t *cond, int ms);
openr2_status_t openr2_interrupt_multiple_wait(openr2_interrupt_t *interrupts[], size_t size, int ms);

/* when pthread is available, return thread_id. -1 otherwise */
unsigned long openr2_thread_self(void);

#ifdef __cplusplus
}
#endif

#endif

/* For Emacs:
 * Local Variables:
 * mode:c
 * indent-tabs-mode:t
 * tab-width:4
 * c-basic-offset:4
 * End:
 * For VIM:
 * vim:set softtabstop=4 shiftwidth=4 tabstop=4:
 */

