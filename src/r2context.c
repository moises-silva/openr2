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

#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include "openr2/r2chan.h"
#include "openr2/r2log.h"
#include "openr2/r2proto.h"
#include "openr2/r2context.h"

static void on_call_init_default(openr2_chan_t *r2chan)
{
	OR2_CHAN_STACK;
	openr2_log(r2chan, OR2_LOG_NOTICE, "call starting at chan %d\n", openr2_chan_get_number(r2chan));
}

static void on_call_offered_default(openr2_chan_t *r2chan, const char *ani, const char *dnis, openr2_calling_party_category_t category)
{
	OR2_CHAN_STACK;
	openr2_log(r2chan, OR2_LOG_NOTICE, "call ready at chan %d with ANI = %s, DNIS = %s, Category = %s\n", 
			openr2_chan_get_number(r2chan), ani, dnis, openr2_proto_get_category_string(category));
}

static void on_call_accepted_default(openr2_chan_t *r2chan)
{
	OR2_CHAN_STACK;
	openr2_log(r2chan, OR2_LOG_NOTICE, "call has been accepted at chan %d\n", openr2_chan_get_number(r2chan));
}

static void on_call_answered_default(openr2_chan_t *r2chan)
{
	OR2_CHAN_STACK;
	openr2_log(r2chan, OR2_LOG_NOTICE, "call has been answered at chan %d\n", openr2_chan_get_number(r2chan));
}

static void on_call_disconnect_default(openr2_chan_t *r2chan, openr2_call_disconnect_reason_t reason)
{
	OR2_CHAN_STACK;
	openr2_log(r2chan, OR2_LOG_NOTICE, "call has been disconnected at chan %d: %s\n", openr2_chan_get_number(r2chan), openr2_proto_get_disconnect_string(reason));
}

static void on_call_end_default(openr2_chan_t *r2chan)
{
	OR2_CHAN_STACK;
	openr2_log(r2chan, OR2_LOG_NOTICE, "call has end on channel %d\n", openr2_chan_get_number(r2chan));
}

static void on_call_read_default(openr2_chan_t *r2chan, const unsigned char *buf, int buflen)
{
	OR2_CHAN_STACK;
	openr2_log(r2chan, OR2_LOG_NOTICE, "read %d bytes in call at chan %d\n", openr2_chan_get_number(r2chan));
}

static void on_os_error_default(openr2_chan_t *r2chan, int oserrorcode)
{
	OR2_CHAN_STACK;
	openr2_log(r2chan, OR2_LOG_ERROR, "OS error at chan %d: %s (%d)\n", openr2_chan_get_number(r2chan), strerror(oserrorcode), oserrorcode);
}

static void on_zap_alarm_default(openr2_chan_t *r2chan, int alarm)
{
	OR2_CHAN_STACK;
	openr2_log(r2chan, OR2_LOG_WARNING, "Zap alarm at chan %d\n", openr2_chan_get_number(r2chan));
}

static void on_protocol_error_default(openr2_chan_t *r2chan, openr2_protocol_error_t error)
{
	OR2_CHAN_STACK;
	openr2_log(r2chan, OR2_LOG_ERROR, "Protocol error at chan %d: %s (%d)\n", openr2_proto_get_error(error));
}

static void on_line_idle_default(openr2_chan_t *r2chan)
{
	OR2_CHAN_STACK;
	openr2_log(r2chan, OR2_LOG_NOTICE, "Far end unblocked!\n");
}

static void on_line_blocked_default(openr2_chan_t *r2chan)
{
	OR2_CHAN_STACK;
	openr2_log(r2chan, OR2_LOG_NOTICE, "Far end blocked!\n");
}

openr2_context_t *openr2_context_new(openr2_mflib_interface_t *mflib, openr2_event_interface_t *evmanager, 
		              openr2_transcoder_interface_t *transcoder, openr2_variant_t variant, int max_ani, int max_dnis)
{
	/* TODO: set some error value when returning NULL */

	/* all interfaces required */
	if (!mflib || !evmanager || !transcoder) {
		return NULL;
	}	

	/* check the MF iface */
	if (!mflib->mf_read_init || !mflib->mf_write_init
	     || !mflib->mf_detect_tone || !mflib->mf_generate_tone
	     || !mflib->mf_select_tone
	     || !mflib->mf_want_generate
	    /* || !mflib->mf_read_dispose || !mflib->mf_write_dispose */ /* these 2 are optional */
	     ) {
		return NULL;
	}	

	/* check the transcoder interface */
	if (!transcoder->alaw_to_linear || !transcoder->linear_to_alaw) {
		return NULL;
	}

	/* fix callmgmt if necessary */
	if (!evmanager->on_call_init) {
		evmanager->on_call_init = on_call_init_default;
	}	
	if (!evmanager->on_call_offered) {
		evmanager->on_call_offered = on_call_offered_default;
	}	
	if (!evmanager->on_call_accepted) {
		evmanager->on_call_accepted = on_call_accepted_default;
	}	
	if (!evmanager->on_call_answered) {
		evmanager->on_call_answered = on_call_answered_default;
	}	
	if (!evmanager->on_call_disconnect) {
		evmanager->on_call_disconnect = on_call_disconnect_default;
	}	
	if (!evmanager->on_call_end) {
		evmanager->on_call_end = on_call_end_default;
	}
	if (!evmanager->on_call_read) {
		evmanager->on_call_read = on_call_read_default;
	}	
	if (!evmanager->on_zap_alarm) {
		evmanager->on_zap_alarm = on_zap_alarm_default;
	}	
	if (!evmanager->on_os_error) {
		evmanager->on_os_error = on_os_error_default;
	}	
	if (!evmanager->on_protocol_error) {
		evmanager->on_protocol_error = on_protocol_error_default;
	}	
	if (!evmanager->on_context_log) {
		evmanager->on_context_log = openr2_log_context_default;
	}	
	if (!evmanager->on_line_idle) {
		evmanager->on_line_idle = on_line_idle_default;
	}
	if (!evmanager->on_line_blocked) {
		evmanager->on_line_blocked = on_line_blocked_default;
	}
	openr2_context_t *r2context = calloc(1, sizeof(*r2context));
	if (!r2context) {
		return NULL;
	}
	r2context->mflib = mflib;
	r2context->evmanager = evmanager;
	r2context->transcoder = transcoder;
	r2context->variant = variant;
	r2context->loglevel = OR2_LOG_ERROR | OR2_LOG_WARNING | OR2_LOG_NOTICE;
	if (openr2_proto_configure_context(r2context, variant, max_ani, max_dnis)) {
		free(r2context);
		return NULL;
	}
	return r2context;
}

/* Is this really needed? in anycase, read events from zaptel are likely to wake us up
   so probably we could trust on that instead of having the user to call this function? */
int openr2_context_get_time_to_next_event(openr2_context_t *r2context)
{
	OR2_CONTEXT_STACK;
	int res, ms;
	struct timeval currtime;
	res = gettimeofday(&currtime, NULL);
	if (-1 == res) {
		return -1;
	}
	/* iterate over all the channels to get the next event time */
	openr2_chan_t *current = r2context->chanlist;
	openr2_chan_t *winner = NULL;
	while ( current ) {
		/* ignore any entry with no callback */
		if ( !current->sched_timer.callback ) {
			current = current->next;
			continue;
		}
		/* if no winner, set this as winner */
		if (!winner) {
			winner = current;
		}
		current = current->next;
		/* if no more chans, return the winner */
		if ( !current ) {
			ms = ( ( ( winner->sched_timer.time.tv_sec - currtime.tv_sec   ) * 1000 ) + 
			       ( ( winner->sched_timer.time.tv_usec - currtime.tv_usec ) / 1000 ) );
			if ( ms < 0 )
				return 0;
			return ms;
			
		}
		/* if the winner timer is after the current timer, then
		   we have a new winner */
		if ( timercmp(&winner->sched_timer.time, &current->sched_timer.time, >) ) {
			winner = current;
		}
	}
	return -1;
}

void openr2_context_add_channel(openr2_context_t *r2context, openr2_chan_t *r2chan)
{
	OR2_CONTEXT_STACK;
	/* put the channel at the head of the list*/
	openr2_chan_t *head = r2context->chanlist;
	r2context->chanlist = r2chan;
	r2chan->next = head;
	/* set the channel log level to our level. Users can override this */
	openr2_chan_set_log_level(r2chan, r2context->loglevel);
}

void openr2_context_remove_channel(openr2_context_t *r2context, openr2_chan_t *r2chan)
{
	OR2_CONTEXT_STACK;
	openr2_chan_t *curr = r2context->chanlist;
	openr2_chan_t *prev;
	while ( curr ) {
		if ( curr == r2chan ) {
			if ( prev ) {
				prev->next = curr->next;
			}	
			if ( curr == r2context->chanlist ) {
				r2context->chanlist = NULL;
			}	
			break;
		}
		prev = curr;
		curr = curr->next;
	}
}

void openr2_context_delete(openr2_context_t *r2context)
{
	OR2_CONTEXT_STACK;
	openr2_chan_t *current, *next;
	current = r2context->chanlist;
	while ( current ) {
		next = current->next;
		openr2_chan_delete(current);
		current = next;
	}
	free(r2context);
}

openr2_liberr_t openr2_context_get_last_error(openr2_context_t *r2context)
{
	OR2_CONTEXT_STACK;
	return r2context->last_error;
}

const char *openr2_context_error_string(openr2_liberr_t error)
{
	switch ( error ) {
	case OR2_LIBERR_SYSCALL_FAILED: return "System call failed";
	case OR2_LIBERR_INVALID_CHAN_SIGNALING: return "Invalid channel signaling";
	case OR2_LIBERR_CANNOT_SET_IDLE: return "Failed to set IDLE state on channel";
	default: return "*Unknown*";
	}
}

openr2_variant_t openr2_context_get_variant(openr2_context_t *r2context)
{
	OR2_CONTEXT_STACK;
	return r2context->variant;
}

void openr2_context_set_ani_first(openr2_context_t *r2context, int ani_first)
{
	OR2_CONTEXT_STACK;
	r2context->get_ani_first = ani_first ? 1 : 0;
}

int openr2_context_get_ani_first(openr2_context_t *r2context)
{
	OR2_CONTEXT_STACK;
	return r2context->get_ani_first;
}

void openr2_context_set_log_level(openr2_context_t *r2context, openr2_log_level_t level)
{
	OR2_CONTEXT_STACK;
	r2context->loglevel = level;
}

openr2_log_level_t openr2_context_get_log_level(openr2_context_t *r2context)
{
	OR2_CONTEXT_STACK;
	return r2context->loglevel;
}

void openr2_context_set_mf_threshold(openr2_context_t *r2context, int threshold)
{
	OR2_CONTEXT_STACK;
	r2context->mf_threshold = threshold;
}

int openr2_context_get_mf_threshold(openr2_context_t *r2context)
{
	OR2_CONTEXT_STACK;
	return r2context->mf_threshold;
}

void openr2_context_set_log_directory(openr2_context_t *r2context, const char *directory)
{
	OR2_CONTEXT_STACK;
	strncpy(r2context->logdir, directory, sizeof(r2context->logdir)-1);
	r2context->logdir[sizeof(r2context->logdir)-1] = 0;
}

char *openr2_context_get_log_directory(openr2_context_t *r2context, char *directory, int len)
{
	OR2_CONTEXT_STACK;
	strncpy(directory, r2context->logdir, len-1);
	directory[len-1] = 0;
	return directory;
}

int openr2_context_get_max_ani(openr2_context_t *r2context)
{
	OR2_CONTEXT_STACK;
	return r2context->max_ani;
}

int openr2_context_get_max_dnis(openr2_context_t *r2context)
{
	OR2_CONTEXT_STACK;
	return r2context->max_dnis;
}

void openr2_context_set_mf_back_timeout(openr2_context_t *r2context, int ms)
{
	OR2_CONTEXT_STACK;
	/* ignore any timeout less than 0 */
	if (ms < 0) {
		return;
	}
	r2context->timers.mf_back_cycle = ms;
}

int openr2_context_get_mf_back_timeout(openr2_context_t *r2context)
{
	OR2_CONTEXT_STACK;
	return r2context->timers.mf_back_cycle;
}


