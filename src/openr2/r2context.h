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

#ifndef _OPENR2_CONTEXT_H_
#define _OPENR2_CONTEXT_H_

#include <inttypes.h>
#include <stdarg.h>
#include "r2proto.h"
#include "r2log.h"

#if defined(__cplusplus)
extern "C" {
#endif

#include "r2exports.h"

#define OR2_MAX_PATH 255

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
   this interface to handle library events like call starting, new call, read audio etc. */
typedef void (*openr2_handle_new_call_func)(openr2_chan_t *r2chan);
typedef void (*openr2_handle_call_offered_func)(openr2_chan_t *r2chan, const char *ani, const char *dnis, openr2_calling_party_category_t category);
typedef void (*openr2_handle_call_accepted_func)(openr2_chan_t *r2chan, openr2_call_mode_t mode);
typedef void (*openr2_handle_call_answered_func)(openr2_chan_t *r2chan);
typedef void (*openr2_handle_call_disconnect_func)(openr2_chan_t *r2chan, openr2_call_disconnect_cause_t cause);
typedef void (*openr2_handle_call_end_func)(openr2_chan_t *r2chan);
typedef void (*openr2_handle_call_read_func)(openr2_chan_t *r2chan, const unsigned char *buf, int buflen);
typedef void (*openr2_handle_os_error_func)(openr2_chan_t *r2chan, int oserrorcode);
typedef void (*openr2_handle_hardware_alarm_func)(openr2_chan_t *r2chan, int alarm);
typedef void (*openr2_handle_protocol_error_func)(openr2_chan_t *r2chan, openr2_protocol_error_t error);
typedef void (*openr2_handle_line_blocked_func)(openr2_chan_t *r2chan);
typedef void (*openr2_handle_line_idle_func)(openr2_chan_t *r2chan);
typedef void (*openr2_handle_billing_pulse_received_func)(openr2_chan_t *r2chan);
typedef void (*openr2_handle_call_log_created_func)(openr2_chan_t *r2chan, const char *name);
typedef int (*openr2_handle_dnis_digit_received_func)(openr2_chan_t *r2chan, char digit);
typedef void (*openr2_handle_ani_digit_received_func)(openr2_chan_t *r2chan, char digit);
typedef void (*openr2_handle_context_logging_func)(openr2_context_t *r2context, const char *file, const char *function, unsigned int line, openr2_log_level_t level, const char *fmt, va_list ap);
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

	/* New DNIS digit arrived. If the user return any non zero
	   value OpenR2 will request more DNIS (if max DNIS still not reached),
	   if 0 is returned no more dnis will be requested regardless of the max DNIS limit*/
	openr2_handle_dnis_digit_received_func on_dnis_digit_received;

	/* New ANI digit arrived */
	openr2_handle_ani_digit_received_func on_ani_digit_received;

	/* Billing pulse arrived */
	openr2_handle_billing_pulse_received_func on_billing_pulse_received;

	/* New call log was created */
	openr2_handle_call_log_created_func on_call_log_created;
} openr2_event_interface_t;

#define OR2_IO_READ      (1 << 0)
#define OR2_IO_WRITE     (1 << 1)
#define OR2_IO_OOB_EVENT (1 << 2)

/* Out of Band events */
typedef enum {
	OR2_OOB_EVENT_NONE,
	OR2_OOB_EVENT_CAS_CHANGE,
	OR2_OOB_EVENT_ALARM_ON,
	OR2_OOB_EVENT_ALARM_OFF
} openr2_oob_event_t;

typedef openr2_io_fd_t (*openr2_io_open_func)(openr2_context_t* r2context, int channo);
typedef int (*openr2_io_close_func)(openr2_chan_t *r2chan);
typedef int (*openr2_io_set_cas_func)(openr2_chan_t *r2chan, int cas);
typedef int (*openr2_io_get_cas_func)(openr2_chan_t *r2chan, int *cas);
typedef int (*openr2_io_flush_write_buffers_func)(openr2_chan_t *r2chan);
typedef int (*openr2_io_write_func)(openr2_chan_t *r2chan, const void *buf, int size);
typedef int (*openr2_io_read_func)(openr2_chan_t *r2chan, const void *buf, int size);
typedef int (*openr2_io_setup_func)(openr2_chan_t *r2chan);
typedef int (*openr2_io_wait_func)(openr2_chan_t *r2chan, int *flags, int block);
typedef int (*openr2_io_get_oob_event_func)(openr2_chan_t *r2chan, openr2_oob_event_t *event);
typedef struct {
	openr2_io_open_func open;
	openr2_io_close_func close;
	openr2_io_set_cas_func set_cas;
	openr2_io_get_cas_func get_cas;
	openr2_io_flush_write_buffers_func flush_write_buffers;
	openr2_io_write_func write;
	openr2_io_read_func read;
	openr2_io_setup_func setup;
	openr2_io_wait_func wait;
	openr2_io_get_oob_event_func get_oob_event;
} openr2_io_interface_t;

typedef enum {
	OR2_IO_DEFAULT = 0, /* Default is Zaptel */
	/* OR2_IO_OPENZAP (libopenzap I/O) */
	/* OR2_IO_SANGOMA (libsangoma I/O) */
	OR2_IO_ZT, /* Zaptel or DAHDI I/O */
	OR2_IO_CUSTOM = 9 /* any unsupported vendor I/O (pika, digivoice, kohmp etc) */
} openr2_io_type_t;

/* Transcoding interface. Users should provide this interface
   to provide transcoding services from linear to alaw and 
   viceversa */
typedef int16_t (*openr2_alaw_to_linear_func)(uint8_t alaw);
typedef uint8_t (*openr2_linear_to_alaw_func)(int linear);
typedef struct {
	openr2_alaw_to_linear_func alaw_to_linear;
	openr2_linear_to_alaw_func linear_to_alaw;
} openr2_transcoder_interface_t;


/* DTMF transmitter part of the openr2_dtmf_interface_t */
typedef void *(*openr2_dtmf_tx_init_func)(void *dtmf_write_handle);
typedef void (*openr2_dtmf_tx_set_timing_func)(void *dtmf_write_handle, int on_time, int off_time);
typedef int (*openr2_dtmf_tx_put_func)(void *dtmf_write_handle, const char *digits, int len);
typedef int (*openr2_dtmf_tx_func)(void *dtmf_write_handle, int16_t amp[], int max_samples);

/* DTMF receiver part of the openr2_dtmf_interface_t */
typedef void (*openr2_digits_rx_callback_t)(void *user_data, const char *digits, int len);
typedef void *(*openr2_dtmf_rx_init_func)(void *dtmf_read_handle, openr2_digits_rx_callback_t callback, void *user_data);
typedef int (*openr2_dtmf_rx_status_func)(void *dtmf_read_handle);
typedef int (*openr2_dtmf_rx_func)(void *dtmf_read_handle, const int16_t amp[], int samples);

typedef struct {
	/* DTMF Transmitter */
	openr2_dtmf_tx_init_func dtmf_tx_init;
	openr2_dtmf_tx_set_timing_func dtmf_tx_set_timing;
	openr2_dtmf_tx_put_func dtmf_tx_put;
	openr2_dtmf_tx_func dtmf_tx;

	/* DTMF Detector */
	openr2_dtmf_rx_init_func dtmf_rx_init;
	openr2_dtmf_rx_status_func dtmf_rx_status;
	openr2_dtmf_rx_func dtmf_rx;
} openr2_dtmf_interface_t;

/* Library errors */
typedef enum {
	/* Failed system call */
	OR2_LIBERR_SYSCALL_FAILED,
	/* Invalid channel signaling when creating it */
	OR2_LIBERR_INVALID_CHAN_SIGNALING,
	/* cannot set to IDLE the channel when creating it */
	OR2_LIBERR_CANNOT_SET_IDLE,
	/* No I/O interface is available */
	OR2_LIBERR_NO_IO_IFACE_AVAILABLE,
	/* Invalid channel number provided  */
	OR2_LIBERR_INVALID_CHAN_NUMBER,
	/* Out of memory */
	OR2_LIBERR_OUT_OF_MEMORY,
	/* Invalid interface provided */
	OR2_LIBERR_INVALID_INTERFACE
} openr2_liberr_t;

OR2_DECLARE(int) openr2_context_get_time_to_next_event(openr2_context_t *r2context);
OR2_DECLARE(openr2_context_t *) openr2_context_new(openr2_variant_t variant, openr2_event_interface_t *callmgmt, int max_ani, int max_dnis);
OR2_DECLARE(void) openr2_context_delete(openr2_context_t *r2context);
OR2_DECLARE(openr2_liberr_t) openr2_context_get_last_error(openr2_context_t *r2context);
OR2_DECLARE(const char *) openr2_context_error_string(openr2_liberr_t error);
OR2_DECLARE(openr2_variant_t) openr2_context_get_variant(openr2_context_t *r2context);
OR2_DECLARE(int) openr2_context_get_max_ani(openr2_context_t *r2context);
OR2_DECLARE(int) openr2_context_get_max_dnis(openr2_context_t *r2context);
OR2_DECLARE(void) openr2_context_set_ani_first(openr2_context_t *r2context, int ani_first);
OR2_DECLARE(int) openr2_context_get_ani_first(openr2_context_t *r2context);
OR2_DECLARE(void) openr2_context_set_skip_category_request(openr2_context_t *r2context, int skipcategory);
OR2_DECLARE(int) openr2_context_get_skip_category_request(openr2_context_t *r2context);
OR2_DECLARE(void) openr2_context_set_immediate_accept(openr2_context_t *r2context, int immediate_accept);
OR2_DECLARE(int) openr2_context_get_immediate_accept(openr2_context_t *r2context);
OR2_DECLARE(void) openr2_context_set_log_level(openr2_context_t *r2context, openr2_log_level_t level);
OR2_DECLARE(openr2_log_level_t) openr2_context_get_log_level(openr2_context_t *r2context);
OR2_DECLARE(void) openr2_context_set_mf_threshold(openr2_context_t *r2context, int threshold);
OR2_DECLARE(int) openr2_context_get_mf_threshold(openr2_context_t *r2context);
OR2_DECLARE(int) openr2_context_set_log_directory(openr2_context_t *r2context, char *directory);
OR2_DECLARE(char *) openr2_context_get_log_directory(openr2_context_t *r2context, char *directory, int len);
OR2_DECLARE(void) openr2_context_set_mf_back_timeout(openr2_context_t *r2context, int ms);
OR2_DECLARE(int) openr2_context_get_mf_back_timeout(openr2_context_t *r2context);
OR2_DECLARE(void) openr2_context_set_metering_pulse_timeout(openr2_context_t *r2context, int ms);
OR2_DECLARE(int) openr2_context_get_metering_pulse_timeout(openr2_context_t *r2context);
OR2_DECLARE(void) openr2_context_set_double_answer(openr2_context_t *r2context, int enable);
OR2_DECLARE(int) openr2_context_get_double_answer(openr2_context_t *r2context);
OR2_DECLARE(int) openr2_context_configure_from_advanced_file(openr2_context_t *r2context, const char *filename);
OR2_DECLARE(int) openr2_context_set_io_type(openr2_context_t *r2context, openr2_io_type_t io_type, openr2_io_interface_t *io_interface);
OR2_DECLARE(void) openr2_context_set_dtmf_detection(openr2_context_t *r2context, int enable);
OR2_DECLARE(int) openr2_context_get_dtmf_detection(openr2_context_t *r2context);
OR2_DECLARE(void) openr2_context_set_dtmf_dialing(openr2_context_t *r2context, int enable, int dtmf_on, int dtmf_off);
OR2_DECLARE(int) openr2_context_get_dtmf_dialing(openr2_context_t *r2context, int *dtmf_on, int *dtmf_off);
OR2_DECLARE(int) openr2_context_set_dtmf_interface(openr2_context_t *r2context, openr2_dtmf_interface_t *dtmf_interface);
OR2_DECLARE(int) openr2_context_set_mflib_interface(openr2_context_t *r2context, openr2_mflib_interface_t *mflib);
OR2_DECLARE(int) openr2_context_set_transcoder_interface(openr2_context_t *r2context, openr2_transcoder_interface_t *transcoder);

#ifdef __OR2_COMPILING_LIBRARY__
#undef openr2_chan_t 
#undef openr2_context_t
#endif

#if defined(__cplusplus)
} /* endif extern "C" */
#endif

#endif /* endif defined _OPENR2_CONTEXT_H_ */

