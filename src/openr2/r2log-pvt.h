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

#ifndef _OPENR2_LOG_PVT_H_
#define _OPENR2_LOG_PVT_H_

#include <stdarg.h>
#include "r2log.h"

#if defined(__cplusplus)
extern "C" {
#endif

struct openr2_chan_s;
struct openr2_context_s;

void openr2_log_generic_default(const char *file, const char *function, unsigned int line, openr2_log_level_t level, const char *fmt, va_list ap);
void openr2_log_channel_default(struct openr2_chan_s *r2chan, const char *file, const char *function, unsigned int line, openr2_log_level_t level, const char *fmt, va_list ap);
void openr2_log_context_default(struct openr2_context_s *r2context, const char *file, const char *function, unsigned int line, openr2_log_level_t level, const char *fmt, va_list ap);
void openr2_log(struct openr2_chan_s *r2chan, const char *file, const char *function, unsigned int line, openr2_log_level_t level, const char *fmt, ...);
void openr2_log2(struct openr2_context_s *r2context, const char *file, const char *function, unsigned int line, openr2_log_level_t level, const char *fmt, ...);
void openr2_log_generic(const char *file, const char *function, unsigned int line, openr2_log_level_t level, const char *fmt, ...);

#if defined(__cplusplus)
} /* endif extern "C" */
#endif

#endif /* endif defined _OPENR2_LOG_PVT_H_ */

