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
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <pthread.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include "openr2/r2utils-pvt.h"

static pthread_mutex_t localtime_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t ctime_lock = PTHREAD_MUTEX_INITIALIZER;

/* VERSION should be always defined */
OR2_EXPORT_SYMBOL
const char *openr2_get_version(void)
{
#ifdef VERSION
	return VERSION;
#else
	return "wtf?"
#endif
}

/* GIT_REVISION is currently never set, need some autotools magic to export it to the build flags */
OR2_EXPORT_SYMBOL
const char *openr2_get_revision()
{
#ifdef GIT_REVISION
	return GIT_REVISION;
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
	pthread_mutex_lock(&localtime_lock);
	lib_tp = localtime(timep);
	if (!lib_tp) {
		pthread_mutex_unlock(&localtime_lock);
		return NULL;
	}
	memcpy(result, lib_tp, sizeof(*result));
	pthread_mutex_unlock(&localtime_lock);
	return result;
}

char *openr2_ctime_r(const time_t *timep, char *buf)
{
	char *lib_buf = NULL;
	size_t len;
	if (!buf) {
		return NULL;
	}
	pthread_mutex_lock(&ctime_lock);
	lib_buf = ctime(timep);
	if (!lib_buf) {
		pthread_mutex_unlock(&ctime_lock);
		return NULL;
	}
	len = strlen(lib_buf);
	memcpy(buf, lib_buf, len);
	buf[len] = 0;
	pthread_mutex_unlock(&ctime_lock);
	return buf;
}

OR2_EXPORT_SYMBOL
int openr2_strncasecmp(const char *s1, const char *s2, size_t n)
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

