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
 * Moises Silva <moy@sangoma.com>
 * Arnaldo M Pereira <arnaldo@sangoma.com>
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "openr2/r2thread.h"

#define SNG_LOG(level, fmt, ...) do { \
  printf( "foobar * " fmt "\n", ## __VA_ARGS__ ); \
} while (0)

#ifdef WIN32
/* required for TryEnterCriticalSection definition.  Must be defined before windows.h include */
#define _WIN32_WINNT 0x0400
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_STRING_H                                                                                                                   
#include <string.h>
#endif                                                                                                                                 

#define _openr2_assert(assertion, msg) \
    if (!(assertion)) { \
        SNG_LOG(SNG_LOGLEVEL_CRIT, msg); \
		return; \
    }

#define _openr2_assert_return(assertion, retval, msg) \
    if (!(assertion)) { \
        SNG_LOG(SNG_LOGLEVEL_CRIT, msg); \
		return retval; \
    }

static __inline__ void *openr2_std_malloc(void *pool, size_t size)
{
    void *ptr = malloc(size);
    pool = NULL; /* fix warning */
    _openr2_assert_return(ptr != NULL, NULL, "Out of memory");
    return ptr;
}

static __inline__ void *openr2_std_calloc(void *pool, size_t elements, size_t size)
{
    void *ptr = calloc(elements, size);
    pool = NULL;
    _openr2_assert_return(ptr != NULL, NULL, "Out of memory");
    return ptr;
}

static __inline__ void *openr2_std_realloc(void *pool, void *buff, size_t size)
{
    buff = realloc(buff, size);
    pool = NULL;
    _openr2_assert_return(buff != NULL, NULL, "Out of memory");
    return buff;
}

static __inline__ void openr2_std_free(void *pool, void *ptr)
{
    pool = NULL;
    _openr2_assert_return(ptr != NULL, , "Attempted to free null pointer");
    free(ptr);
}

openr2_memory_handler_t g_openr2_mem_handler =
{
	/*.pool =*/ NULL,
	/*.malloc =*/ openr2_std_malloc,
	/*.calloc =*/ openr2_std_calloc,
	/*.realloc =*/ openr2_std_realloc,
	/*.free =*/ openr2_std_free
};

size_t thread_default_stacksize = 0;

static void * SNG_THREAD_CALLING_CONVENTION thread_launch(void *args)
{
	void *exit_val;
    openr2_thread_t *thread = (openr2_thread_t *)args;
	exit_val = thread->function(thread, thread->private_data);
#ifndef WIN32
	pthread_attr_destroy(&thread->attribute);
#endif
	openr2_safe_free(thread);

	return exit_val;
}

openr2_status_t openr2_thread_create_detached(openr2_thread_function_t func, void *data)
{
	return openr2_thread_create_detached_ex(func, data, thread_default_stacksize);
}

openr2_status_t openr2_thread_create_detached_ex(openr2_thread_function_t func, void *data, size_t stack_size)
{
	openr2_thread_t *thread = NULL;
	openr2_status_t status = SNG_FAIL;

	if (!func || !(thread = (openr2_thread_t *)openr2_malloc(sizeof(openr2_thread_t)))) {
		goto done;
	}

	thread->private_data = data;
	thread->function = func;
	thread->stack_size = stack_size;

#if defined(WIN32)
	thread->handle = (void *)_beginthreadex(NULL, (unsigned)thread->stack_size, (unsigned int (__stdcall *)(void *))thread_launch, thread, 0, NULL);
	if (!thread->handle) {
		goto fail;
	}
	CloseHandle(thread->handle);

	status = SNG_SUCCESS;
	goto done;
#else
	
	if (pthread_attr_init(&thread->attribute) != 0)	goto fail;

	if (pthread_attr_setdetachstate(&thread->attribute, PTHREAD_CREATE_DETACHED) != 0) goto failpthread;

	if (thread->stack_size && pthread_attr_setstacksize(&thread->attribute, thread->stack_size) != 0) goto failpthread;

	if (pthread_create(&thread->handle, &thread->attribute, thread_launch, thread) != 0) goto failpthread;

	status = SNG_SUCCESS;
	goto done;
 failpthread:
	pthread_attr_destroy(&thread->attribute);
#endif

 fail:
	if (thread) {
		openr2_safe_free(thread);
	}
 done:
	return status;
}


openr2_status_t openr2_mutex_create(openr2_mutex_t **mutex)
{
	openr2_status_t status = SNG_FAIL;
#ifndef WIN32
	pthread_mutexattr_t attr;
#endif
	openr2_mutex_t *check = NULL;

	check = (openr2_mutex_t *)openr2_malloc(sizeof(**mutex));
	if (!check)
		goto done;
#ifdef WIN32
	InitializeCriticalSection(&check->mutex);
#else
	if (pthread_mutexattr_init(&attr))
		goto done;

	if (pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE))
		goto fail;

	if (pthread_mutex_init(&check->mutex, &attr))
		goto fail;

	goto success;

 fail:
	pthread_mutexattr_destroy(&attr);
	goto done;

 success:
#endif
	*mutex = check;
	status = SNG_SUCCESS;

 done:
	return status;
}

openr2_status_t openr2_mutex_destroy(openr2_mutex_t **mutex)
{
	openr2_mutex_t *mp = *mutex;
	*mutex = NULL;
	if (!mp) {
		return SNG_FAIL;
	}
#ifdef WIN32
	DeleteCriticalSection(&mp->mutex);
#else
	if (pthread_mutex_destroy(&mp->mutex))
		return SNG_FAIL;
#endif
	openr2_safe_free(mp);
	return SNG_SUCCESS;
}

openr2_status_t _openr2_mutex_lock(openr2_mutex_t *mutex)
{
#ifdef WIN32
	EnterCriticalSection(&mutex->mutex);
#else
	int err;
	if ((err = pthread_mutex_lock(&mutex->mutex))) {
		SNG_LOG(SNG_LOGLEVEL_ERROR, "Failed to lock mutex %d:%s\n", err, strerror(err));
		return SNG_FAIL;
	}
#endif
	return SNG_SUCCESS;
}

openr2_status_t _openr2_mutex_trylock(openr2_mutex_t *mutex)
{
#ifdef WIN32
	if (!TryEnterCriticalSection(&mutex->mutex))
		return SNG_FAIL;
#else
	if (pthread_mutex_trylock(&mutex->mutex))
		return SNG_FAIL;
#endif
	return SNG_SUCCESS;
}

openr2_status_t _openr2_mutex_unlock(openr2_mutex_t *mutex)
{
#ifdef WIN32
	LeaveCriticalSection(&mutex->mutex);
#else
	if (pthread_mutex_unlock(&mutex->mutex))
		return SNG_FAIL;
#endif
	return SNG_SUCCESS;
}


openr2_status_t openr2_interrupt_create(openr2_interrupt_t **ininterrupt, openr2_socket_t device)
{
	openr2_interrupt_t *interrupt = NULL;
#ifndef WIN32
	int fds[2];
#endif

	_openr2_assert_return(ininterrupt != NULL, SNG_FAIL, "interrupt double pointer is null!\n");

	interrupt = openr2_calloc(1, sizeof(*interrupt));
	if (!interrupt) {
		SNG_LOG(SNG_LOGLEVEL_ERROR, "Failed to allocate interrupt memory\n");
		return SNG_FAIL;
	}

	interrupt->device = device;
#ifdef WIN32
	interrupt->event = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (!interrupt->event) {
		SNG_LOG(SNG_LOGLEVEL_ERROR, "Failed to allocate interrupt event\n");
		goto failed;
	}
#else
	if (pipe(fds)) {
		SNG_LOG(SNG_LOGLEVEL_ERROR, "Failed to allocate interrupt pipe: %s\n", strerror(errno));
		goto failed;
	}
	interrupt->readfd = fds[0];
	interrupt->writefd = fds[1];
#endif

	*ininterrupt = interrupt;
	return SNG_SUCCESS;

failed:
	if (interrupt) {
#ifndef WIN32
		if (interrupt->readfd) {
			close(interrupt->readfd);
			close(interrupt->writefd);
			interrupt->readfd = -1;
			interrupt->writefd = -1;
		}
#endif
		openr2_safe_free(interrupt);
	}
	return SNG_FAIL;
}

#define ONE_BILLION 1000000000

openr2_status_t openr2_interrupt_wait(openr2_interrupt_t *interrupt, int ms)
{
	int num = 1;
#ifdef WIN32
	DWORD res = 0;
	HANDLE ints[2];
#else
	int res = 0;
	struct pollfd ints[2];
	char pipebuf[255];
#endif

	_openr2_assert_return(interrupt != NULL, SNG_FAIL, "Condition is null!\n");


	/* start implementation */
#ifdef WIN32
	ints[0] = interrupt->event;
	if (interrupt->device != SNG_INVALID_SOCKET) {
		num++;
		ints[1] = interrupt->device;
	}
	res = WaitForMultipleObjects(num, ints, FALSE, ms >= 0 ? ms : INFINITE);
	switch (res) {
	case WAIT_TIMEOUT:
		return SNG_TIMEOUT;
	case WAIT_FAILED:
	case WAIT_ABANDONED: /* is it right to fail with abandoned? */
		return SNG_FAIL;
	default:
		if (res >= (sizeof(ints)/sizeof(ints[0]))) {
			SNG_LOG(SNG_LOGLEVEL_ERROR, "Error waiting for freetdm interrupt event (WaitForSingleObject returned %d)\n", res);
			return SNG_FAIL;
		}
		return SNG_SUCCESS;
	}
#else
pollagain:
	ints[0].fd = interrupt->readfd;
	ints[0].events = POLLIN;
	ints[0].revents = 0;

	if (interrupt->device != SNG_INVALID_SOCKET) {
		num++;
		ints[1].fd = interrupt->device;
		ints[1].events = POLLIN;
		ints[1].revents = 0;
	}

	res = poll(ints, num, ms);

	if (res == -1) {
		if (errno == EINTR) {
			goto pollagain;
		}
		SNG_LOG(SNG_LOGLEVEL_CRIT, "interrupt poll failed (%s)\n", strerror(errno));
		return SNG_FAIL;
	}

	if (res == 0) {
		return SNG_TIMEOUT;
	}

	if (ints[0].revents & POLLIN) {
		res = read(ints[0].fd, pipebuf, sizeof(pipebuf));
		if (res == -1) {
			SNG_LOG(SNG_LOGLEVEL_CRIT, "reading interrupt descriptor failed (%s)\n", strerror(errno));
		}
	}

	return SNG_SUCCESS;
#endif
}

openr2_status_t openr2_interrupt_signal(openr2_interrupt_t *interrupt)
{
	_openr2_assert_return(interrupt != NULL, SNG_FAIL, "Interrupt is null!\n");
#ifdef WIN32
	if (!SetEvent(interrupt->event)) {
		SNG_LOG(SNG_LOGLEVEL_ERROR, "Failed to signal interrupt\n");
		return SNG_FAIL;

	}
#else
	int err;
	struct pollfd testpoll;
	testpoll.revents = 0;
	testpoll.events = POLLIN;
	testpoll.fd = interrupt->readfd;
	err = poll(&testpoll, 1, 0);
	if (err == 0 && !(testpoll.revents & POLLIN)) {
		/* we just try to notify if there is nothing on the read fd already, 
		 * otherwise users that never call interrupt wait eventually will 
		 * eventually have the pipe buffer filled */
		if ((err = write(interrupt->writefd, "w", 1)) != 1) {
			SNG_LOG(SNG_LOGLEVEL_ERROR, "Failed to signal interrupt: %s\n", strerror(errno));
			return SNG_FAIL;
		}
	}
#endif
	return SNG_SUCCESS;
}

openr2_status_t openr2_interrupt_destroy(openr2_interrupt_t **ininterrupt)
{
	openr2_interrupt_t *interrupt = NULL;
	_openr2_assert_return(ininterrupt != NULL, SNG_FAIL, "Interrupt null when destroying!\n");
	interrupt = *ininterrupt;
#ifdef WIN32
	CloseHandle(interrupt->event);
#else
	close(interrupt->readfd);
	close(interrupt->writefd);

	interrupt->readfd = -1;
	interrupt->writefd = -1;
#endif
	openr2_safe_free(interrupt);
	*ininterrupt = NULL;
	return SNG_SUCCESS;
}

openr2_status_t openr2_interrupt_multiple_wait(openr2_interrupt_t *interrupts[], size_t size, int ms)
{
	int numdevices = 0;
	unsigned i;

#if defined(__WINDOWS__)
	DWORD res = 0;
	HANDLE ints[20];
	if (size > (openr2_array_len(ints)/2)) {
		/* improve if needed: dynamically allocate the list of interrupts *only* when exceeding the default size */
		SNG_LOG(SNG_LOGLEVEL_CRIT, "Unsupported size of interrupts: %d, implement me!\n", size);
		return SNG_FAIL;
	}

	for (i = 0; i < size; i++) {
		ints[i] = interrupts[i]->event;
		if (interrupts[i]->device != SNG_INVALID_SOCKET) {

			ints[size+numdevices] = interrupts[i]->device;
			numdevices++;
		}
	}

	res = WaitForMultipleObjects((DWORD)size+numdevices, ints, FALSE, ms >= 0 ? ms : INFINITE);

	switch (res) {
	case WAIT_TIMEOUT:
		return SNG_TIMEOUT;
	case WAIT_FAILED:
	case WAIT_ABANDONED: /* is it right to fail with abandoned? */
		return SNG_FAIL;
	default:
		if (res >= (size+numdevices)) {
			SNG_LOG(SNG_LOGLEVEL_ERROR, "Error waiting for freetdm interrupt event (WaitForSingleObject returned %d)\n", res);
			return SNG_FAIL;
		}
		/* fall-through to SNG_SUCCESS at the end of the function */
	}
#elif defined(__linux__) || defined(__FreeBSD__)
	int res = 0;
	char pipebuf[255];
	struct pollfd ints[size*2];

	memset(&ints, 0, sizeof(ints));
pollagain:
	for (i = 0; i < size; i++) {
		ints[i].events = POLLIN;
		ints[i].revents = 0;
		ints[i].fd = interrupts[i]->readfd;
		if (interrupts[i]->device != SNG_INVALID_SOCKET) {
			ints[size+numdevices].events = POLLIN;
			ints[size+numdevices].revents = 0;
			ints[size+numdevices].fd = interrupts[i]->device;

			numdevices++;
		}
	}

	res = poll(ints, size + numdevices, ms);

	if (res == -1) {
		if (errno == EINTR) {
			goto pollagain;
		}
		SNG_LOG(SNG_LOGLEVEL_CRIT, "interrupt poll failed (%s)\n", strerror(errno));
		return SNG_FAIL;
	}

	if (res == 0) {
		return SNG_TIMEOUT;
	}

	/* check for events in the pipes, NOT in the devices */
	for (i = 0; i < size; i++) {
		if (ints[i].revents & POLLIN) {
			res = read(ints[i].fd, pipebuf, sizeof(pipebuf));
			if (res == -1) {
				SNG_LOG(SNG_LOGLEVEL_CRIT, "reading interrupt descriptor failed (%s)\n", strerror(errno));
			}
		}
	}
#else
#endif
	return SNG_SUCCESS;
}

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
