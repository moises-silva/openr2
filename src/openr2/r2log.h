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

#ifndef _OPENR2_LOG_H_
#define _OPENR2_LOG_H_

#if defined(__cplusplus)
extern "C" {
#endif

#include "openr2/r2declare.h"

typedef enum {
	OR2_LOG_NOTHING     = 0,
	OR2_LOG_ERROR       = ( 1 << 0 ), 
	OR2_LOG_WARNING     = ( 1 << 1 ),
	OR2_LOG_NOTICE      = ( 1 << 2 ),
	OR2_LOG_DEBUG       = ( 1 << 3 ),
	OR2_LOG_MF_TRACE    = ( 1 << 4 ),
	OR2_LOG_CAS_TRACE   = ( 1 << 5 ),
	OR2_LOG_STACK_TRACE = ( 1 << 6 ),
	OR2_LOG_ALL         = ( 0xFFF ),
	OR2_LOG_EX_DEBUG    = ( OR2_LOG_ALL + 1 )
} openr2_log_level_t;	

typedef void (*openr2_generic_logging_func_t)(const char *file, const char *function, unsigned int line, openr2_log_level_t level, const char *fmt, va_list ap);

/* default first args to log functions */
#define OR2_LOG_PREP __FILE__, __FUNCTION__, __LINE__

/* must be used on each openr2_log2() call */
#define OR2_CONTEXT_LOG OR2_LOG_PREP

/* to be used on openr2_log() */
#define OR2_CHANNEL_LOG OR2_LOG_PREP

/* generic log, not channel nor context */
#define OR2_GENERIC_LOG OR2_LOG_PREP

OR2_DECLARE(const char *) openr2_log_get_level_string(openr2_log_level_t level);
OR2_DECLARE(openr2_log_level_t) openr2_log_get_level(const char *levelstr);
OR2_DECLARE(void) openr2_generic_set_logging_func(openr2_generic_logging_func_t logcallback);

#if defined(__cplusplus)
} /* endif extern "C" */
#endif

#endif /* endif defined _OPENR2_LOG_H_ */
