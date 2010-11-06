/*
 * OpenR2 
 * MFC/R2 call setup library
 *
 * Moises Silva <moy@sangoma.com>
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
 * Contributors:
 *
 * Arnaldo Pereira <arnaldo@sangoma.com>
 *
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <ctype.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#include <time.h>
#include <errno.h>
#include "openr2/r2declare.h"
#include "openr2/r2thread.h"
#include "openr2/r2utils-pvt.h"

static openr2_mutex_t *localtime_lock = NULL;
static openr2_mutex_t *ctime_lock = NULL;

#ifndef HAVE_GETTIMEOFDAY

#ifdef WIN32
#include <mmsystem.h>

int gettimeofday(struct timeval *tp, void *nothing)
{
#ifdef WITHOUT_MM_LIB
    SYSTEMTIME st;
    time_t tt;
    struct tm tmtm;
    /* mktime converts local to UTC */
    GetLocalTime (&st);
    tmtm.tm_sec = st.wSecond;
    tmtm.tm_min = st.wMinute;
    tmtm.tm_hour = st.wHour;
    tmtm.tm_mday = st.wDay;
    tmtm.tm_mon = st.wMonth - 1;
    tmtm.tm_year = st.wYear - 1900;  tmtm.tm_isdst = -1;
    tt = mktime (&tmtm);
    tp->tv_sec = tt;
    tp->tv_usec = st.wMilliseconds * 1000;
#else
    /**
     ** The earlier time calculations using GetLocalTime
     ** had a time resolution of 10ms.The timeGetTime, part
     ** of multimedia apis offer a better time resolution
     ** of 1ms.Need to link against winmm.lib for this
     **/
    unsigned long Ticks = 0;
    unsigned long Sec =0;
    unsigned long Usec = 0;
    Ticks = timeGetTime();

    Sec = Ticks/1000;
    Usec = (Ticks - (Sec*1000))*1000;
    tp->tv_sec = Sec;
    tp->tv_usec = Usec;
#endif /* WITHOUT_MM_LIB */
    (void)nothing;
    return 0;
}
#endif /* WIN32 */
#endif /* HAVE_GETTIMEOFDAY */

/* VERSION should be always defined */
OR2_DECLARE(const char *) openr2_get_version(void)
{
#ifdef VERSION
	return VERSION;
#else
	return "wtf?"
#endif
}

/* REVISION will be only defined if built via SVN */
OR2_DECLARE(const char *) openr2_get_revision()
{
#ifdef REVISION
	return REVISION;
#else
	return "(release)";
#endif
}

int openr2_mkdir_recursive(char *dir, mode_t mode)
{
	char *currslash = NULL;
	char *str = dir;
	if (!dir) {
		return -1;
	}
	str++; /* in case the path starts with a slash */ 
	while ((currslash = strchr(str, '/'))) {
		*currslash = 0;
		if (mkdir(dir, mode) && errno != EEXIST) {
			return -1;
		}
		*currslash = '/';
		str = currslash + 1;
	}
	if (str[0] != 0) {
		if (mkdir(dir, mode)) {
			return -1;
		}
	}
	return 0;
}

/* TODO: find a better way to implement localtime_r and ctime_r when not available */
struct tm *openr2_localtime_r(const time_t *timep, struct tm *result)
{
	/* we could test here for localtime_r availability */
	struct tm *lib_tp = NULL;
	if (!result) {
		return NULL;
	}
	if (!localtime_lock)
		openr2_mutex_create(&localtime_lock);
	openr2_mutex_lock(localtime_lock);
	lib_tp = localtime(timep);
	if (!lib_tp) {
		openr2_mutex_unlock(localtime_lock);
		return NULL;
	}
	memcpy(result, lib_tp, sizeof(*result));
	openr2_mutex_unlock(localtime_lock);
	return result;
}

char *openr2_ctime_r(const time_t *timep, char *buf)
{
	char *lib_buf = NULL;
	size_t len;
	if (!buf) {
		return NULL;
	}
	if (!ctime_lock)
		openr2_mutex_create(&ctime_lock);
	openr2_mutex_lock(ctime_lock);
	lib_buf = ctime(timep);
	if (!lib_buf) {
		openr2_mutex_unlock(ctime_lock);
		return NULL;
	}
	len = strlen(lib_buf);
	memcpy(buf, lib_buf, len);
	buf[len] = 0;
	openr2_mutex_unlock(ctime_lock);
	return buf;
}

OR2_DECLARE(int) openr2_strncasecmp(const char *s1, const char *s2, size_t n)
{
	const unsigned char *p1 = (const unsigned char *)s1;
	const unsigned char *p2 = (const unsigned char *)s2;
	int result;
	if (p1 == p2 || n == 0) {
		return 0;
	}
	while ((result = tolower(*p1) - tolower(*p2++)) == 0) {
		if (*p1++ == '\0' || --n == 0) {
			break;
		}
	}
	return result;
}

