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

#ifndef _OPENR2_CHAN_H_
#define _OPENR2_CHAN_H_

#include <stdio.h>
#include <sys/time.h>
#include "r2proto.h"
#include "r2engine.h"
#include "r2context.h"
#include "r2log.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define OR2_CHAN_READ_SIZE 160

struct openr2_chan_s;

/* function type to be called when a scheduled event 
   for the channel is triggered */
typedef void (*openr2_callback_t)(struct openr2_chan_s *r2chan, void *cb_data);

/* scheduled event */
typedef struct {
	struct timeval time;
	openr2_callback_t callback;
	void *data;
	int id;
} openr2_sched_timer_t;

/* callback for logging channel related info */
typedef void (*openr2_logging_func_t)(struct openr2_chan_s *r2chan, openr2_log_level_t level, const char *fmt, va_list ap);

typedef struct openr2_chan_timer_ids_s {
	/* Forward safety timer id */
	int mf_fwd_safety;

	/* Seize timer id */
	int r2_seize;

	/* Answer timer id */
	int r2_answer;

	/* metering pulse timer */
	int r2_metering_pulse;

	/* double answer timer */
	int r2_double_answer;
	
	/* resume the MF dance on DNIS timeout */
	int mf_back_resume_cycle;

	/* max time the MF back compelled cycle may last */
	int mf_back_cycle;

	/* small delay before answering to give some time
	   to the other end to detect our tone off condition */
	int r2_answer_delay;

	/* CAS persistence check */
	int cas_persistence_check;
} openr2_chan_timer_ids_t;

/* R2 channel. Hold the states of the R2 signaling, zap device etc.
   The R2 variant will be inherited from the R2 context 
   this channel belongs to */
typedef struct openr2_chan_s {

	/* whether or not we created the FD */
	int fd_created;

	/* zap device fd */
	int fd;

	/* zap buffer size */
	int zap_buf_size;

	/* zap device number */
	int number;

	/* to read or not to read, that is the question */
	int read_enabled;

	/* forward, backward or stopped.  */
	openr2_direction_t direction;

	/* array of scheduled events in execution order */
	#define OR2_MAX_SCHED_TIMERS 10
	openr2_sched_timer_t sched_timers[OR2_MAX_SCHED_TIMERS];

	/* timer id incremented each time a new timer is scheduled */
	int timer_id;

	/* timer count/index */
	int timers_count;

	/* programmed timer ids */
	openr2_chan_timer_ids_t timer_ids;

	/* how much time in ms CAS signals must persist before being handled, 
	   0 means handle immediately */
	int cas_persistence_check;

	/* only one of this states is effective while in-call.
	   We either are in a fwd state or backward state  */
	openr2_mf_state_t mf_state;

	/* which MF group we this channel is at */
	openr2_mf_group_t mf_group; 

	/* only one if this states is effective while in-call
	   Our ABCD bits are either in a fwd or backward state */
	openr2_abcd_state_t r2_state;

	/* Call state for this channel */
	openr2_call_state_t call_state;

	/* last raw R2 signal read on this channel */
	int abcd_read;

	/* last raw R2 signal written to this channel */
	int abcd_write;

	/* signal being checked for persistence */
	int abcd_persistence_check;

	/* Meaning of last R2 signal read on this channel */
	openr2_abcd_signal_t abcd_rx_signal;

	/* Meaning of last R2 signal written to this channel */
	openr2_abcd_signal_t abcd_tx_signal;

	/* Private buffer to store the string ABCD representation */
	char abcd_buff[10];

	/* R2 context this channel belongs to */
	openr2_context_t *r2context;

	/* received ANI */
	char ani[OR2_MAX_ANI];
	char *ani_ptr;
	unsigned ani_len;

	/* received DNIS */
	char dnis[OR2_MAX_DNIS];
	char *dnis_ptr;
	unsigned dnis_len;

	/* 1 when the caller ANI is restricted */
	int caller_ani_is_restricted;

	/* category tone for the calling party */
	openr2_mf_tone_t caller_category;

	/* openr2 clients can store data here */
	void *client_data;

	/* MF tone generation handle */
	void *mf_write_handle;

	/* MF tone detection handle */
	void *mf_read_handle;

	/* default MF tone generation handle */
	openr2_mf_tx_state_t default_mf_write_handle;

	/* default MF tone detection handle*/
	openr2_mf_rx_state_t default_mf_read_handle;

	/* MF signal we last wrote */
	int mf_write_tone;

	/* MF signal we last read */
	int mf_read_tone;

	/* MF threshold tone */
	int mf_threshold_tone;

	/* MF read start time */
	struct timeval mf_threshold_time;

#ifdef OR2_MF_DEBUG
	/* MF audio debug logging */
	int mf_read_fd;
	int mf_write_fd;
#endif

	/* whether or not the call has been answered */
	unsigned answered;

	/* whether or not the category has been sent */
	unsigned category_sent;

	/* channel logging callback */
	openr2_logging_func_t on_channel_log;

	/* logging level */
	openr2_log_level_t loglevel;

	/* call file logging */
	int call_files;
	long call_count;
	FILE *logfile;

	/* linking */
	struct openr2_chan_s *next;

} openr2_chan_t;

openr2_chan_t *openr2_chan_new(openr2_context_t *r2context, int channo, void *mf_write_handle, void *mf_read_handle);
openr2_chan_t *openr2_chan_new_from_fd(openr2_context_t *r2context, int chanfd, void *mf_write_handle, void *mf_read_handle);
void openr2_chan_delete(openr2_chan_t *r2chan);
int openr2_chan_process_event(openr2_chan_t *r2chan);
int openr2_chan_accept_call(openr2_chan_t *r2chan, openr2_call_mode_t accept);
int openr2_chan_answer_call(openr2_chan_t *r2chan);
int openr2_chan_disconnect_call(openr2_chan_t *r2chan, openr2_call_disconnect_cause_t cause);
int openr2_chan_make_call(openr2_chan_t *r2chan, const char *ani, const char *dnid, openr2_calling_party_category_t category);
openr2_direction_t openr2_chan_get_direction(openr2_chan_t *r2chan);
int openr2_chan_write(openr2_chan_t *r2chan, const unsigned char *buf, int len);
void openr2_chan_set_logging_func(openr2_chan_t *r2chan, openr2_logging_func_t logcallback);
int openr2_chan_get_fd(openr2_chan_t *r2chan);
int openr2_chan_get_number(openr2_chan_t *r2chan);
openr2_context_t *openr2_chan_get_context(openr2_chan_t *r2chan);
void openr2_chan_set_client_data(openr2_chan_t *r2chan, void *data);
void *openr2_chan_get_client_data(openr2_chan_t *r2chan);
int openr2_chan_get_time_to_next_event(openr2_chan_t *r2chan);
openr2_log_level_t openr2_chan_set_log_level(openr2_chan_t *r2chan, openr2_log_level_t level);
openr2_log_level_t openr2_chan_get_log_level(openr2_chan_t *r2chan);
void openr2_chan_enable_read(openr2_chan_t *r2chan);
void openr2_chan_disable_read(openr2_chan_t *r2chan);
int openr2_chan_get_read_enabled(openr2_chan_t *r2chan);
void openr2_chan_enable_call_files(openr2_chan_t *r2chan);
void openr2_chan_disable_call_files(openr2_chan_t *r2chan);
int openr2_chan_get_call_files_enabled(openr2_chan_t *r2chan);
const char *openr2_chan_get_dnis(openr2_chan_t *r2chan);
const char *openr2_chan_get_ani(openr2_chan_t *r2chan);

/* TODO: set and cancel timer should not be called by users, move to private header */
int openr2_chan_add_timer(openr2_chan_t *r2chan, int ms, openr2_callback_t callback, void *cb_data);
void openr2_chan_cancel_timer(openr2_chan_t *r2chan, int *timer_id);
void openr2_chan_cancel_all_timers(openr2_chan_t *r2chan);


#if defined(__cplusplus)
} /* endif extern "C" */
#endif

#endif /* endif defined _OPENR2_CHAN_H_ */

