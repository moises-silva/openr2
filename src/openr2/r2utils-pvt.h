/*
 * OpenR2 
 * MFC/R2 call setup library
 *
 * Moises Silva <moises.silva@gmail.com>
 * Copyright (C) 2008 Moises Silva
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _OPENR2_UTILS_PVT_H_
#define _OPENR2_UTILS_PVT_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h> /* mode_t */
#endif
#ifdef WIN32
#include <direct.h>
#define F_OK 0
#define mkdir(x,y) (mkdir)(x)
#define mode_t int
#endif
#include "r2utils.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define ARRAY_LEN(array) (sizeof(array)/sizeof(array[0]))
#define STR_LEN(string) ((sizeof(string)/sizeof(string[0]))-1)

#define openr2_set_flag(r2chan, flag) (r2chan)->flags |= flag
#define openr2_clear_flag(r2chan, flag) (r2chan)->flags &= ~flag
#define openr2_test_flag(r2chan, flag) ((r2chan)->flags & flag)

#define OR2_EXPORT_SYMBOL __attribute__((visibility("default")))

#define openr2_timercmp(a, b, CMP)                                           \
 (((a)->tv_sec == (b)->tv_sec) ?                                             \
  ((a)->tv_usec CMP (b)->tv_usec) :                                          \
  ((a)->tv_sec CMP (b)->tv_sec))

#define openr2_timerclear(tvp) ((tvp)->tv_sec = (tvp)->tv_usec = 0)

/* quick access to context Multi Frequency Interface */
#define MFI(r2chan) (r2chan)->r2context->mflib

/* quick access to context Event Management Interface */
#define EMI(r2chan) (r2chan)->r2context->evmanager

/* quick access to the Transcoding Interface */
#define TI(r2chan) (r2chan)->r2context->transcoder

/* quick access to the DTMF Interface */
#define DTMF(r2chan) (r2chan)->r2context->dtmfeng

int openr2_mkdir_recursive(char *dir, mode_t mode);

/* I added this ones because -std=c99 -pedantic causes
   localtime_r, ctime_r and strncasecmp to not be defined */
struct tm *openr2_localtime_r(const time_t *timep, struct tm *result);
char *openr2_ctime_r(const time_t *timep, char *buf);

/* gettimeofday defined in r2utils for WIN32 */
#ifdef WIN32
int gettimeofday(struct timeval *tp, void *nothing);
#endif

#define openr2_assert(assertion, msg) \
	do { \
		if (!(assertion)) { \
			openr2_log_generic(OR2_GENERIC_LOG, OR2_LOG_ERROR, msg); \
		} \
	} while (0);

#if defined(__cplusplus)
} /* endif extern "C" */
#endif

#endif /* endif defined _OPENR2_UTILS_H_ */
