/*
 * Copyright (c) 2010, Sangoma Technologies
 * Moises Silva <moy@sangoma.com>
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 
 * * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 
 * * Neither the name of the original author; nor the names of any contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 * 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __R2DECLARE_H__
#define __R2DECLARE_H__

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(_XOPEN_SOURCE) && !defined(__FreeBSD__)
#define _XOPEN_SOURCE 600
#endif

#ifndef __WINDOWS__
#if defined(WIN32) || defined(WIN64) || defined(_MSC_VER) || defined(_WIN32) || defined(_WIN64)
#define __WINDOWS__
#endif
#endif

#ifdef WIN32
#ifdef _MSC_VER
/* disable the following warnings 
 * C4100: The formal parameter is not referenced in the body of the function. The unreferenced parameter is ignored. 
 * C4200: Non standard extension C zero sized array
 * C4204: nonstandard extension used : non-constant aggregate initializer 
 * C4706: assignment within conditional expression
 * C4819: The file contains a character that cannot be represented in the current code page
 * C4132: 'object' : const object should be initialized (fires innapropriately for prototyped forward declaration of cost var)
 * C4510: default constructor could not be generated
 * C4512: assignment operator could not be generated
 * C4610: struct  can never be instantiated - user defined constructor required
 */
#pragma warning(disable:4100 4200 4204 4706 4819 4132 4510 4512 4610 4996)
#ifndef __inline__
#define __inline__ __inline
#endif
#endif /* _MSC_VER */
#if defined(FREETDM_DECLARE_STATIC)
#define OR2_DECLARE(type)            type __stdcall
#define OR2_DECLARE_NONSTD(type)     type __cdecl
#define OR2_DECLARE_DATA
#elif defined(OR2_EXPORTS)
#define OR2_DECLARE(type)            __declspec(dllexport) type __stdcall
#define OR2_DECLARE_NONSTD(type)     __declspec(dllexport) type __cdecl
#define OR2_DECLARE_DATA             __declspec(dllexport)
#else
#define OR2_DECLARE(type)            __declspec(dllimport) type __stdcall
#define OR2_DECLARE_NONSTD(type)     __declspec(dllimport) type __cdecl
#define OR2_DECLARE_DATA             __declspec(dllimport)
#endif
#define EX_DECLARE_DATA             __declspec(dllexport)
#else /* WIN32 */
#if (defined(__GNUC__) || defined(__SUNPRO_CC) || defined (__SUNPRO_C)) && defined(HAVE_VISIBILITY)
#define OR2_DECLARE(type)        __attribute__((visibility("default"))) type
#define OR2_DECLARE_NONSTD(type) __attribute__((visibility("default"))) type
#define OR2_DECLARE_DATA     __attribute__((visibility("default")))
#else
#define OR2_DECLARE(type)        type
#define OR2_DECLARE_NONSTD(type) type
#define OR2_DECLARE_DATA
#endif
#define EX_DECLARE_DATA
#endif /* WIN32 */

#ifdef _MSC_VER
#if (_MSC_VER >= 1400)			/* VC8+ */
#ifndef _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE
#endif
#ifndef _CRT_NONSTDC_NO_DEPRECATE
#define _CRT_NONSTDC_NO_DEPRECATE
#endif
#endif
#ifndef S_IRUSR
#define S_IRUSR _S_IREAD
#endif
#ifndef S_IWUSR
#define S_IWUSR _S_IWRITE
#endif
#undef HAVE_STRINGS_H
#undef HAVE_SYS_SOCKET_H
#endif /* _MSC_VER */

#ifdef __WINDOWS__
#include <stdio.h>
#include <windows.h>
#else
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#endif

#ifdef __cplusplus
} /* extern C */
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
