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

#ifndef _OPENR2_CHAN_PVT_H_
#define _OPENR2_CHAN_PVT_H_

#include <stdio.h>
#include <pthread.h>
#include <sys/time.h>
#include "r2engine.h"
#include "r2log.h"
#include "r2chan.h"
#include "r2proto-pvt.h"

#if defined(__cplusplus)
extern "C" {
#endif

/* getting half second of silence we declare DTMF DNIS string as ended */
#define OR2_DTMF_MAX_SILENCE_SAMPLES 4000

struct openr2_chan_s;
struct openr2_context_s;

/* function type to be called when a scheduled event 
   for the channel is triggered */
typedef void (*openr2_callback_t)(struct openr2_chan_s *r2chan);

/* scheduled event */
typedef struct {
	struct timeval time;
	openr2_callback_t callback;
	const char *name;
	int id;
} openr2_sched_timer_t;

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

	/* DTMF dial startup */
	int dtmf_start_dial;
} openr2_chan_timer_ids_t;

typedef enum r2chan_flags_e {
	OR2_CHAN_CALL_DNIS_CALLBACK = (1 << 0)
} r2chan_flags_t;

/* R2 channel. Hold the states of the R2 signaling, I/O device etc.
   The R2 variant will be inherited from the R2 context 
   this channel belongs to */
typedef struct openr2_chan_s {

	/* hold this for operations on the channel */
	pthread_mutex_t lock;	

	/* whether or not we created the FD */
	int fd_created;

	/* I/O device fd */
	openr2_io_fd_t fd;

	/* I/O buffer size */
	int io_buf_size;

	/* I/O device number */
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

	/* only one of this states is effective while in-call.
	   We either are in a fwd state or backward state  */
	openr2_mf_state_t mf_state;

	/* which MF group we this channel is at */
	openr2_mf_group_t mf_group; 

	/* Our CAS state */
	openr2_cas_state_t r2_state;

	/* Call state for this channel */
	openr2_call_state_t call_state;

	/* last raw R2 signal read on this channel */
	int cas_read;

	/* last raw R2 signal written to this channel */
	int cas_write;

	/* signal being checked for persistence */
	int cas_persistence_check_signal;

	/* Meaning of last R2 signal read on this channel */
	openr2_cas_signal_t cas_rx_signal;

	/* Meaning of last R2 signal written to this channel */
	openr2_cas_signal_t cas_tx_signal;

	/* Whether or not this channel is in alarm */
	int inalarm;

	/* Private buffer to store the string CAS representation */
	char cas_buff[10];

	/* R2 context this channel belongs to */
	struct openr2_context_s *r2context;

	/* received ANI */
	char ani[OR2_MAX_ANI];
	char *ani_ptr;
	unsigned ani_len;

	/* received DNIS */
	char dnis[OR2_MAX_DNIS];
	int dnis_index;
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

	/* DTMF rx and tx handle */
	void *dtmf_write_handle;
	void *dtmf_read_handle;

	/* whether or not we are in the middle of dialing or detecting DTMF */
	int dialing_dtmf;
	int detecting_dtmf;
	int dtmf_silence_samples;

	/* default DTMF tone generation handle */
	openr2_dtmf_tx_state_t default_dtmf_write_handle;

	/* default DTMF tone reception handle */
	openr2_dtmf_rx_state_t default_dtmf_read_handle;

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

	/* generic flags */
	int32_t flags;

	/* linking */
	struct openr2_chan_s *next;
} openr2_chan_t;

#define openr2_chan_lock(r2chan) pthread_mutex_lock(&(r2chan)->lock)
#define openr2_chan_unlock(r2chan) pthread_mutex_unlock(&(r2chan)->lock)
#define OR2_INVALID_IO_HANDLE NULL
int openr2_chan_add_timer(openr2_chan_t *r2chan, int ms, openr2_callback_t callback, const char *name);
void openr2_chan_cancel_timer(openr2_chan_t *r2chan, int *timer_id);
void openr2_chan_cancel_all_timers(openr2_chan_t *r2chan);

#if defined(__cplusplus)
} /* endif extern "C" */
#endif

#endif /* endif defined _OPENR2_CHAN_PVT_H_ */

