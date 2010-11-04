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

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include <time.h>
#include "openr2/r2declare.h"
#include "openr2/r2thread.h"
#include "openr2/r2log-pvt.h"
#include "openr2/r2utils-pvt.h"
#include "openr2/r2chan-pvt.h"
#include "openr2/r2context-pvt.h"

void openr2_log_channel_default(openr2_chan_t *r2chan, const char *file, const char *function, unsigned int line, openr2_log_level_t level, const char *fmt, va_list ap)
{
	struct timeval currtime;
	struct tm currtime_tm;
	time_t currsec = time(NULL);
	int res = gettimeofday(&currtime, NULL);
	if (-1 == res) {
		fprintf(stderr, "gettimeofday failed!\n");
		return;
	} 
	if (NULL == openr2_localtime_r(&currsec, &currtime_tm)) {
		fprintf(stderr, "openr2_localtime_r failed!\n");
		return;
	}
	/* Avoid infinite recursion: Don't call openr2_chan_get_number 
	   because that will call openr2_log */
	printf("[%02d:%02d:%03lu][%s] Channel %d -- ", currtime_tm.tm_min, currtime_tm.tm_sec, 
			currtime.tv_usec/1000, openr2_log_get_level_string(level), r2chan->number);
	if (r2chan->r2context->configured_from_file) {
		printf("M -- ");
	}
	vprintf(fmt, ap);
}

void openr2_log_context_default(openr2_context_t *r2context, const char *file, const char *function, unsigned int line, openr2_log_level_t level, const char *fmt, va_list ap)
{
	printf("[%s] Context -- ", openr2_log_get_level_string(level));
	if (r2context->configured_from_file) {
		printf("M -- ");
	}
	vprintf(fmt, ap);
}

static void log_at_file(openr2_chan_t *r2chan, const char *fmt, va_list ap)
{
	struct timeval currtime;
	struct tm currtime_tm;
	time_t currsec = time(NULL);
	int res = gettimeofday(&currtime, NULL);
	if (-1 == res) {
		fprintf(stderr, "gettimeofday failed!\n");
		return;
	} 
	if (NULL == openr2_localtime_r(&currsec, &currtime_tm)) {
		fprintf(stderr, "openr2_localtime_r failed!\n");
		return;
	}
	/* Avoid infinite recurstion: Don't call openr2_chan_get_number 
	   because that will call openr2_log */
	fprintf(r2chan->logfile, "[%02d:%02d:%02d:%03lu] [Thread: %s] [Chan %d] - ", currtime_tm.tm_hour, currtime_tm.tm_min, 
			currtime_tm.tm_sec, currtime.tv_usec/1000, "FIXME", r2chan->number);
	if (r2chan->r2context->configured_from_file) {
		fprintf(r2chan->logfile, "M - ");
	}	
	vfprintf(r2chan->logfile, fmt, ap);
}

void openr2_log(openr2_chan_t *r2chan, const char *file, const char *function, unsigned int line, openr2_log_level_t level, const char *fmt, ...)
{
	va_list ap;
	va_list aplog;
	if (r2chan->logfile) {
		va_start(aplog, fmt);
		log_at_file(r2chan, fmt, aplog);
		va_end(aplog);
	}
	/* Avoid infinite recursion: Don't call openr2_chan_get_log_level 
	   because that will call openr2_log */
	if (level & r2chan->loglevel) {
		va_start(ap, fmt);
		r2chan->on_channel_log(r2chan, file, function, line, level, fmt, ap);
		va_end(ap);
	}	
}

void openr2_log2(struct openr2_context_s *r2context, const char *file, const char *function, unsigned int line, openr2_log_level_t level, const char *fmt, ...)
{
	va_list ap;
	/* Avoid infinite recursion: Don't call openr2_context_get_log_level 
	   because that will call openr2_log2 */
	if (level & r2context->loglevel) {
		va_start(ap, fmt);
		r2context->evmanager->on_context_log(r2context, file, function, line, level, fmt, ap);
		va_end(ap);
	}	
}

FT_DECLARE(const char *) openr2_log_get_level_string(openr2_log_level_t level)
{
	switch (level) {
	case OR2_LOG_ERROR:
		return "ERROR";
	case OR2_LOG_WARNING:
		return "WARNING";
	case OR2_LOG_NOTICE:
		return "NOTICE";
	case OR2_LOG_DEBUG:
		return "DEBUG";
	case OR2_LOG_MF_TRACE:
		return "MF TRACE";
	case OR2_LOG_CAS_TRACE:
		return "CAS TRACE";
	case OR2_LOG_STACK_TRACE:
		return "STACK TRACE";
	case OR2_LOG_NOTHING:
		return "NOTHING";
	case OR2_LOG_EX_DEBUG:
		return "EXDEBUG";
	default:
		return "*UNKNOWN*";
	};
}

FT_DECLARE(openr2_log_level_t) openr2_log_get_level(const char *levelstr)
{
	if (!openr2_strncasecmp("ALL", levelstr, sizeof("ALL")-1)) {
		return OR2_LOG_ALL;
	} else if (!openr2_strncasecmp("ERROR", levelstr, sizeof("ERROR")-1)) {
		return OR2_LOG_ERROR;	
	} else if (!openr2_strncasecmp("WARNING", levelstr, sizeof("WARNING")-1)) {
		return OR2_LOG_WARNING;
	} else if (!openr2_strncasecmp("NOTICE", levelstr, sizeof("NOTICE")-1)) {
		return OR2_LOG_NOTICE;
	} else if (!openr2_strncasecmp("DEBUG", levelstr, sizeof("DEBUG")-1)) {
		return OR2_LOG_DEBUG;
	} else if (!openr2_strncasecmp("EXDEBUG", levelstr, sizeof("EXDEBUG")-1)) {
		return OR2_LOG_EX_DEBUG;
	} else if (!openr2_strncasecmp("MF", levelstr, sizeof("MF")-1)) {
		return OR2_LOG_MF_TRACE;
	} else if (!openr2_strncasecmp("CAS", levelstr, sizeof("CAS")-1)) {
		return OR2_LOG_CAS_TRACE;
	} else if (!openr2_strncasecmp("STACK", levelstr, sizeof("STACK")-1)) {
		return OR2_LOG_STACK_TRACE;
	} else if (!openr2_strncasecmp("NOTHING", levelstr, sizeof("NOTHING")-1)){
		return OR2_LOG_NOTHING;
	}
	return (openr2_log_level_t)-1;
}


