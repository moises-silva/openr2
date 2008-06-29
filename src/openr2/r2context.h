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

#ifndef _OPENR2_CONTEXT_H_
#define _OPENR2_CONTEXT_H_

#include <inttypes.h>
#include <stdarg.h>
#include "r2proto.h"
#include "r2log.h"

#define OR2_MAX_LOGDIR 255

/* we dont include openr2_chan_t because r2chan.h 
   already include us */
struct openr2_chan_s;
struct openr2_context_s;

/* MF interface */
typedef void *(*openr2_mf_read_init_func)(void *read_handle, int forward_signals);
typedef void *(*openr2_mf_write_init_func)(void *write_handle, int forward_signals);
typedef int (*openr2_mf_detect_tone_func)(void *read_handle, const int16_t buffer[], int samples);
typedef int (*openr2_mf_generate_tone_func)(void *write_handle, int16_t buffer[], int samples);
typedef int (*openr2_mf_select_tone_func)(void *write_handle, char signal);
typedef int (*openr2_mf_want_generate_func)(void *write_handle, int signal);
typedef void (*openr2_mf_read_dispose_func)(void *read_handle);
typedef void (*openr2_mf_write_dispose_func)(void *write_handle);
typedef struct {
	/* init routines to detect and generate tones */
	openr2_mf_read_init_func mf_read_init;
	openr2_mf_write_init_func mf_write_init;

	/* detect and generation routines */
	openr2_mf_detect_tone_func mf_detect_tone;
	openr2_mf_generate_tone_func mf_generate_tone;

	/* choose the tone to transmit */
	openr2_mf_select_tone_func mf_select_tone;

	/* whether or not want mf_generate_tone() called */
	openr2_mf_want_generate_func mf_want_generate;

	/* routines to dispose resources allocated by handles. (optional) */
	openr2_mf_read_dispose_func mf_read_dispose;
	openr2_mf_write_dispose_func mf_write_dispose;
} openr2_mflib_interface_t;

/* Event Management interface. Users should provide
   this interface to handle library events like call starting,
   new call, read audio etc. */
typedef void (*openr2_handle_new_call_func)(struct openr2_chan_s *r2chan);
typedef void (*openr2_handle_call_offered_func)(struct openr2_chan_s *r2chan, const char *ani, const char *dnis, openr2_calling_party_category_t category);
typedef void (*openr2_handle_call_accepted_func)(struct openr2_chan_s *r2chan);
typedef void (*openr2_handle_call_answered_func)(struct openr2_chan_s *r2chan);
typedef void (*openr2_handle_call_disconnect_func)(struct openr2_chan_s *r2chan, openr2_call_disconnect_cause_t cause);
typedef void (*openr2_handle_call_end_func)(struct openr2_chan_s *r2chan);
typedef void (*openr2_handle_call_read_func)(struct openr2_chan_s *r2chan, const unsigned char *buf, int buflen);
typedef void (*openr2_handle_os_error_func)(struct openr2_chan_s *r2chan, int oserrorcode);
typedef void (*openr2_handle_hardware_alarm_func)(struct openr2_chan_s *r2chan, int alarm);
typedef void (*openr2_handle_protocol_error_func)(struct openr2_chan_s *r2chan, openr2_protocol_error_t error);
typedef void (*openr2_handle_line_blocked_func)(struct openr2_chan_s *r2chan);
typedef void (*openr2_handle_line_idle_func)(struct openr2_chan_s *r2chan);
typedef void (*openr2_handle_context_logging_func)(struct openr2_context_s *r2context, openr2_log_level_t level, const char *fmt, va_list ap);
typedef struct {
	/* A new call has just started. We will start to 
	   receive the ANI and DNIS */
	openr2_handle_new_call_func on_call_init;

	/* New call is ready to be accepted or rejected */
	openr2_handle_call_offered_func on_call_offered;

	/* Call has been accepted */
	openr2_handle_call_accepted_func on_call_accepted;

	/* Call has been answered */
	openr2_handle_call_answered_func on_call_answered;

	/* The far end has sent a disconnect signal */
	openr2_handle_call_disconnect_func on_call_disconnect;

	/* Disconnection process has end completely */
	openr2_handle_call_end_func on_call_end;

	/* Call has something to say */
	openr2_handle_call_read_func on_call_read;

	/* Hardware interface alarm event */
	openr2_handle_hardware_alarm_func on_hardware_alarm;

	/* some operating system error ocurred. 
	   Usually all the user can do in this
	   event is display some error or log it */
	openr2_handle_os_error_func on_os_error;

	/* protocol error */
	openr2_handle_protocol_error_func on_protocol_error;

	/* line BLOCKED event */
	openr2_handle_line_blocked_func on_line_blocked;

	/* line IDLE event */
	openr2_handle_line_idle_func on_line_idle;

	/* logging handler */
	openr2_handle_context_logging_func on_context_log;
} openr2_event_interface_t;

/* Transcoding interface. Users should provide this interface
   to provide transcoding services from linear to alaw and 
   viceversa */
typedef int16_t (*openr2_alaw_to_linear_func)(uint8_t alaw);
typedef uint8_t (*openr2_linear_to_alaw_func)(int linear);
typedef struct {
	openr2_alaw_to_linear_func alaw_to_linear;
	openr2_linear_to_alaw_func linear_to_alaw;
} openr2_transcoder_interface_t;

/* R2 protocol timers */
typedef struct {
	/* Max amount of time our transmitted MF signal can last */
	int mf_back_cycle;
	/* Amount of time we set a MF signal ON to resume the MF cycle */
	int mf_back_resume_cycle;
	/* Safety FORWARD timer */
	int mf_fwd_safety;
	/* How much time do we wait for a response to our seize signal */
	int r2_seize;
	/* How much to wait for an answer once the call has been accepted */
	int r2_answer;
	/* How much to wait for metering pulse detection */
	int r2_metering_pulse;
} openr2_timers_t;

/* Library errors */
typedef enum {
	/* Failed system call */
	OR2_LIBERR_SYSCALL_FAILED,
	/* Invalid channel signaling when creating it */
	OR2_LIBERR_INVALID_CHAN_SIGNALING,
	/* cannot set to IDLE the channel when creating it */
	OR2_LIBERR_CANNOT_SET_IDLE
} openr2_liberr_t;

/* R2 library context. Holds the R2 channel list,
   protocol variant, client interfaces etc */
typedef struct openr2_context_s {

	/* last library error occurred on the context */
	openr2_liberr_t last_error;

	/* this interface provide MF functions 
	   to the R2 channels */
	openr2_mflib_interface_t *mflib;

	/* this interface provides event management 
	   functions */
	openr2_event_interface_t *evmanager;

	/* this interface provide transcoding 
	   functions to the R2 channels */
	openr2_transcoder_interface_t *transcoder;

	/* R2 variant to use in this context channels */
	openr2_variant_t variant;

	/* ABCD signals configured for the variant in use */
	openr2_abcd_signal_t abcd_signals[OR2_NUM_ABCD_SIGNALS];

	/* C and D bit are not required for R2 and set to 01, 
	   thus, not used for the R2 signaling */
	openr2_abcd_signal_t abcd_nonr2_bits;

	/* Mask to easily get the AB bits which are used for 
	   R2 signaling */
	openr2_abcd_signal_t abcd_r2_bits;

	/* Backward MF tones */
	openr2_mf_ga_tones_t mf_ga_tones;
	openr2_mf_gb_tones_t mf_gb_tones;
	openr2_mf_gc_tones_t mf_gc_tones;

	/* Forward MF tones */
	openr2_mf_g1_tones_t mf_g1_tones;
	openr2_mf_g2_tones_t mf_g2_tones;

	/* R2 timers */
	openr2_timers_t timers;

	/* Max amount of DNIS digits that a channel on this context expect */
	int max_dnis;

	/* Max amount of ANI digits that a channel on this context expect */
	int max_ani;

	/* whether or not to get the ANI before getting DNIS */
	int get_ani_first;

	/* MF threshold time in ms */
	int mf_threshold;

	/* R2 logging mask */
	openr2_log_level_t loglevel;

	/* R2 logging directory */
	char logdir[OR2_MAX_LOGDIR];

	/* list of channels that belong to this context */
	struct openr2_chan_s *chanlist;

} openr2_context_t;

#if defined(__cplusplus)
extern "C" {
#endif

int openr2_context_get_time_to_next_event(openr2_context_t *r2context);
openr2_context_t *openr2_context_new(openr2_mflib_interface_t *mflib, openr2_event_interface_t *callmgmt, 
		              openr2_transcoder_interface_t *transcoder, openr2_variant_t variant, int max_ani, int max_dnis);
void openr2_context_delete(openr2_context_t *r2context);
void openr2_context_add_channel(openr2_context_t *r2context, struct openr2_chan_s *r2chan);
void openr2_context_remove_channel(openr2_context_t *r2context, struct openr2_chan_s *r2chan);
openr2_liberr_t openr2_context_get_last_error(openr2_context_t *r2context);
const char *openr2_context_error_string(openr2_liberr_t error);
openr2_variant_t openr2_context_get_variant(openr2_context_t *r2context);
int openr2_context_get_max_ani(openr2_context_t *r2context);
int openr2_context_get_max_dnis(openr2_context_t *r2context);
void openr2_context_set_ani_first(openr2_context_t *r2context, int ani_first);
int openr2_context_get_ani_first(openr2_context_t *r2context);
void openr2_context_set_log_level(openr2_context_t *r2context, openr2_log_level_t level);
openr2_log_level_t openr2_context_get_log_level(openr2_context_t *r2context);
void openr2_context_set_mf_threshold(openr2_context_t *r2context, int threshold);
int openr2_context_get_mf_threshold(openr2_context_t *r2context);
int openr2_context_set_log_directory(openr2_context_t *r2context, char *directory);
char *openr2_context_get_log_directory(openr2_context_t *r2context, char *directory, int len);
void openr2_context_set_mf_back_timeout(openr2_context_t *r2context, int ms);
int openr2_context_get_mf_back_timeout(openr2_context_t *r2context);
void openr2_context_set_metering_pulse_timeout(openr2_context_t *r2context, int ms);
int openr2_context_get_metering_pulse_timeout(openr2_context_t *r2context);

#if defined(__cplusplus)
} /* endif extern "C" */
#endif

#endif /* endif defined _OPENR2_CONTEXT_H_ */

