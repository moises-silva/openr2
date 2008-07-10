/*
 * OpenR2 
 * MFC/R2 call setup library
 *
 * Moises Silva <moises.silva@gmail.com>
 * Copyright (C) Moises Silva
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
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

/* VERSION should be always defined */
const char *openr2_get_version()
{
#ifdef VERSION
	return VERSION;
#else
	return "wtf?"
#endif
}

/* REVISION will be only defined if built via SVN */
const char *openr2_get_revision()
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

