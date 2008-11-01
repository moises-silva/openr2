/*
 * OpenR2 
 * MFC/R2 call setup library
 *
 * Moises Silva <moy@sangoma.com>
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

#ifndef _OPENR2_LOG_H_
#define _OPENR2_LOG_H_

#if defined(__cplusplus)
extern "C" {
#endif

struct openr2_chan_s;
struct openr2_context_s;

typedef enum {
	OR2_LOG_NOTHING     = 0,
	OR2_LOG_ERROR       = ( 1 << 0 ), 
	OR2_LOG_WARNING     = ( 1 << 1 ),
	OR2_LOG_NOTICE      = ( 1 << 2 ),
	OR2_LOG_DEBUG       = ( 1 << 3 ),
	OR2_LOG_MF_TRACE    = ( 1 << 4 ),
	OR2_LOG_CAS_TRACE   = ( 1 << 5 ),
	OR2_LOG_STACK_TRACE = ( 1 << 6 ),
	OR2_LOG_ALL         = 0xFF
} openr2_log_level_t;	

#ifdef OR2_TRACE_STACKS
#define OR2_CHAN_STACK openr2_log(r2chan, OR2_LOG_STACK_TRACE, "%s()\n", __PRETTY_FUNCTION__);
#define OR2_CONTEXT_STACK openr2_log2(r2context, OR2_LOG_STACK_TRACE, "%s()\n", __PRETTY_FUNCTION__);
#else
#define OR2_CHAN_STACK 
#define OR2_CONTEXT_STACK 
#endif

void openr2_log_channel_default(struct openr2_chan_s *r2chan, openr2_log_level_t level, const char *fmt, va_list ap);
void openr2_log_context_default(struct openr2_context_s *r2context, openr2_log_level_t level, const char *fmt, va_list ap);
void openr2_log(struct openr2_chan_s *r2chan, openr2_log_level_t level, const char *fmt, ...);
void openr2_log2(struct openr2_context_s *r2context, openr2_log_level_t level, const char *fmt, ...);
const char *openr2_log_get_level_string(openr2_log_level_t level);
openr2_log_level_t openr2_log_get_level(const char *levelstr);

#if defined(__cplusplus)
} /* endif extern "C" */
#endif

#endif /* endif defined _OPENR2_LOG_H_ */
