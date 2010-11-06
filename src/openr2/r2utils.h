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

#ifndef _OPENR2_UTILS_H_
#define _OPENR2_UTILS_H_

#if defined(__cplusplus)
extern "C" {
#endif

#include "openr2/r2declare.h"

#define OR2_LIB_INTERFACE 4
#define OR2_LIB_REVISION 0
#define OR2_LIB_AGE 0
OR2_DECLARE(const char *) openr2_get_version(void);
OR2_DECLARE(const char *) openr2_get_revision(void);
OR2_DECLARE(int) openr2_strncasecmp(const char *s1, const char *s2, size_t n);

#if defined(__cplusplus)
} /* endif extern "C" */
#endif

#endif /* endif defined _OPENR2_UTILS_H_ */
