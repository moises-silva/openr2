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
 * Contributors:
 *
 * Arnaldo Pereira <arnaldo@sangoma.com>
 *
 */

#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include <sys/stat.h>
#include <errno.h>
#include "openr2/r2declare.h"
#include "openr2/r2thread.h"
#include "openr2/r2engine.h"
#include "openr2/r2log-pvt.h"
#include "openr2/r2proto-pvt.h"
#include "openr2/r2utils-pvt.h"
#include "openr2/r2chan-pvt.h"
#include "openr2/r2context-pvt.h"
#include "openr2/r2ioabs.h"

static void on_call_init_default(openr2_chan_t *r2chan)
{
	OR2_CHAN_STACK;
	openr2_log(r2chan, OR2_CHANNEL_LOG, OR2_LOG_NOTICE, "call starting at chan %d\n", openr2_chan_get_number(r2chan));
}

static void on_call_offered_default(openr2_chan_t *r2chan, const char *ani, const char *dnis, openr2_calling_party_category_t category)
{
	OR2_CHAN_STACK;
	openr2_log(r2chan, OR2_CHANNEL_LOG, OR2_LOG_NOTICE, "call ready at chan %d with ANI = %s, DNIS = %s, Category = %s\n", 
			openr2_chan_get_number(r2chan), ani, dnis, openr2_proto_get_category_string(category));
}

static void on_call_accepted_default(openr2_chan_t *r2chan, openr2_call_mode_t mode)
{
	OR2_CHAN_STACK;
	openr2_log(r2chan, OR2_CHANNEL_LOG, OR2_LOG_NOTICE, "call has been accepted at chan %d\n", openr2_chan_get_number(r2chan));
}

static void on_call_answered_default(openr2_chan_t *r2chan)
{
	OR2_CHAN_STACK;
	openr2_log(r2chan, OR2_CHANNEL_LOG, OR2_LOG_NOTICE, "call has been answered at chan %d\n", openr2_chan_get_number(r2chan));
}

static void on_call_disconnect_default(openr2_chan_t *r2chan, openr2_call_disconnect_cause_t cause)
{
	OR2_CHAN_STACK;
	openr2_log(r2chan, OR2_CHANNEL_LOG, OR2_LOG_NOTICE, "call has been disconnected at chan %d: %s\n", openr2_chan_get_number(r2chan), openr2_proto_get_disconnect_string(cause));
}

static void on_call_end_default(openr2_chan_t *r2chan)
{
	OR2_CHAN_STACK;
	openr2_log(r2chan, OR2_CHANNEL_LOG, OR2_LOG_NOTICE, "call has end on channel %d\n", openr2_chan_get_number(r2chan));
}

static void on_call_read_default(openr2_chan_t *r2chan, const unsigned char *buf, int buflen)
{
	OR2_CHAN_STACK;
	openr2_log(r2chan, OR2_CHANNEL_LOG, OR2_LOG_NOTICE, "read %d bytes in call at chan %d\n", openr2_chan_get_number(r2chan));
}

static void on_os_error_default(openr2_chan_t *r2chan, int oserrorcode)
{
	OR2_CHAN_STACK;
	openr2_log(r2chan, OR2_CHANNEL_LOG, OR2_LOG_ERROR, "OS error at chan %d: %s (%d)\n", openr2_chan_get_number(r2chan), strerror(oserrorcode), oserrorcode);
}

static void on_hardware_alarm_default(openr2_chan_t *r2chan, int alarm)
{
	OR2_CHAN_STACK;
	openr2_log(r2chan, OR2_CHANNEL_LOG, OR2_LOG_WARNING, "Zap alarm at chan %d\n", openr2_chan_get_number(r2chan));
}

static void on_protocol_error_default(openr2_chan_t *r2chan, openr2_protocol_error_t error)
{
	OR2_CHAN_STACK;
	openr2_log(r2chan, OR2_CHANNEL_LOG, OR2_LOG_ERROR, "Protocol error at chan %d: %s (%d)\n", openr2_proto_get_error(error));
}

static void on_line_idle_default(openr2_chan_t *r2chan)
{
	OR2_CHAN_STACK;
	openr2_log(r2chan, OR2_CHANNEL_LOG, OR2_LOG_NOTICE, "Far end unblocked!\n");
}

static void on_line_blocked_default(openr2_chan_t *r2chan)
{
	OR2_CHAN_STACK;
	openr2_log(r2chan, OR2_CHANNEL_LOG, OR2_LOG_NOTICE, "Far end blocked!\n");
}

static int on_dnis_digit_received_default(openr2_chan_t *r2chan, char digit)
{
	OR2_CHAN_STACK;
	openr2_log(r2chan, OR2_CHANNEL_LOG, OR2_LOG_DEBUG, "DNIS digit %c received on chan %d\n", digit, openr2_chan_get_number(r2chan));
	/* default behavior is to ask for as much dnis as possible */
	return 1;
}

static void on_ani_digit_received_default(openr2_chan_t *r2chan, char digit)
{
	OR2_CHAN_STACK;
	openr2_log(r2chan, OR2_CHANNEL_LOG, OR2_LOG_DEBUG, "ANI digit %c received on chan %d\n", digit, openr2_chan_get_number(r2chan));
}

static void on_billing_pulse_received_default(openr2_chan_t *r2chan)
{
	OR2_CHAN_STACK;
	openr2_log(r2chan, OR2_CHANNEL_LOG, OR2_LOG_NOTICE, "Billing pulse received on chan %d\n", openr2_chan_get_number(r2chan));
}

static void on_call_log_created_default(openr2_chan_t *r2chan, const char *logname)
{
	OR2_CHAN_STACK;
	openr2_log(r2chan, OR2_CHANNEL_LOG, OR2_LOG_NOTICE, "Log %s created on chan %d\n", logname, openr2_chan_get_number(r2chan));
}

static int want_generate_default(openr2_mf_tx_state_t *state, int signal)
{
	return signal ? 1 : 0;
}

static const openr2_mf_read_init_func mf_read_init_default = (openr2_mf_read_init_func)openr2_mf_rx_init;
static const openr2_mf_write_init_func mf_write_init_default = (openr2_mf_write_init_func)openr2_mf_tx_init;
static const openr2_mf_detect_tone_func mf_detect_tone_default = (openr2_mf_detect_tone_func)openr2_mf_rx;
static const openr2_mf_generate_tone_func mf_generate_tone_default = (openr2_mf_generate_tone_func)openr2_mf_tx;
static const openr2_mf_select_tone_func mf_select_tone_default = (openr2_mf_select_tone_func)openr2_mf_tx_put;
static const openr2_mf_want_generate_func mf_want_generate_default = (openr2_mf_want_generate_func)want_generate_default;

static const openr2_alaw_to_linear_func alaw_to_linear_default = openr2_alaw_to_linear;
static const openr2_linear_to_alaw_func linear_to_alaw_default = openr2_linear_to_alaw;

static openr2_mflib_interface_t default_mf_interface = {
	/* .mf_read_init */ (openr2_mf_read_init_func)openr2_mf_rx_init,
	/* .mf_write_init */ (openr2_mf_write_init_func)openr2_mf_tx_init,
	/* .mf_detect_tone */ (openr2_mf_detect_tone_func)openr2_mf_rx,
	/* .mf_generate_tone */ (openr2_mf_generate_tone_func)openr2_mf_tx,
	/* .mf_select_tone */ (openr2_mf_select_tone_func)openr2_mf_tx_put,
	/* .mf_want_generate */ (openr2_mf_want_generate_func)want_generate_default,
	/* .mf_read_dispose */ NULL,
	/* .mf_write_dispose */ NULL
};

static openr2_transcoder_interface_t default_transcoder = {
	/* .alaw_to_linear */ openr2_alaw_to_linear,
	/* .linear_to_alaw */ openr2_linear_to_alaw
};

static openr2_event_interface_t default_evmanager = {
	/* .on_call_init */ on_call_init_default,
	/* .on_call_offered */ on_call_offered_default,
	/* .on_call_accepted */ on_call_accepted_default,
	/* .on_call_answered */ on_call_answered_default,
	/* .on_call_disconnect */ on_call_disconnect_default,
	/* .on_call_end */ on_call_end_default,
	/* .on_call_read */ on_call_read_default,
	/* .on_hardware_alarm */ on_hardware_alarm_default,
	/* .on_os_error */ on_os_error_default,
	/* .on_protocol_error */ on_protocol_error_default,
	/* .on_line_blocked */ on_line_blocked_default,
	/* .on_line_idle */ on_line_idle_default,
	/* .on_context_log */ openr2_log_context_default,
	/* .on_dnis_digit_received */ on_dnis_digit_received_default,
	/* .on_ani_digit_received */ on_ani_digit_received_default,
	/* .on_billing_pulse_received */ on_billing_pulse_received_default,
	/* .on_call_log_created */ on_call_log_created_default
};

static openr2_dtmf_interface_t default_dtmf_engine = {
	/* .dtmf_tx_init */ (openr2_dtmf_tx_init_func)openr2_dtmf_tx_init,
	/* .dtmf_tx_set_timing */ (openr2_dtmf_tx_set_timing_func)openr2_dtmf_tx_set_timing,
	/* .dtmf_tx_put */ (openr2_dtmf_tx_put_func)openr2_dtmf_tx_put,
	/* .dtmf_tx */ (openr2_dtmf_tx_func)openr2_dtmf_tx,

	/* .dtmf_rx_init */ (openr2_dtmf_rx_init_func)openr2_dtmf_rx_init,
	/* .dtmf_rx_status */ (openr2_dtmf_rx_status_func)openr2_dtmf_rx_status,
	/* .dtmf_rx */ (openr2_dtmf_rx_func)openr2_dtmf_rx
};

OR2_DECLARE(openr2_context_t *) openr2_context_new(openr2_variant_t variant, openr2_event_interface_t *evmanager, int max_ani, int max_dnis)
{
	openr2_context_t *r2context = NULL;
	if (!evmanager) {
		evmanager = &default_evmanager;
	} else {
		/* fix callmgmt */
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
		if (!evmanager->on_hardware_alarm) {
			evmanager->on_hardware_alarm = on_hardware_alarm_default;
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
		if (!evmanager->on_dnis_digit_received) {
			evmanager->on_dnis_digit_received = on_dnis_digit_received_default;
		}
		if (!evmanager->on_ani_digit_received) {
			evmanager->on_ani_digit_received = on_ani_digit_received_default;
		}
		if (!evmanager->on_billing_pulse_received) {
			evmanager->on_billing_pulse_received = on_billing_pulse_received_default;
		}
		if (!evmanager->on_call_log_created) {
			evmanager->on_call_log_created = on_call_log_created_default;
		}
	}
	r2context = calloc(1, sizeof(*r2context));
	if (!r2context) {
		r2context->last_error = OR2_LIBERR_OUT_OF_MEMORY;
		return NULL;
	}

	r2context->mflib = &default_mf_interface;
	r2context->transcoder = &default_transcoder;
	r2context->variant = variant;
	r2context->evmanager = evmanager;
	r2context->dtmfeng = &default_dtmf_engine;
	r2context->loglevel = OR2_LOG_ERROR | OR2_LOG_WARNING | OR2_LOG_NOTICE;
	openr2_mutex_create(&r2context->timers_lock);
	if (openr2_proto_configure_context(r2context, variant, max_ani, max_dnis)) {
		free(r2context);
		return NULL;
	}
	if (openr2_context_set_io_type(r2context, OR2_IO_DEFAULT, NULL) == -1) {
		free(r2context);
		return NULL;
	}
	return r2context;
}

OR2_DECLARE(int) openr2_context_set_mflib_interface(openr2_context_t *r2context, openr2_mflib_interface_t *mflib)
{
	/* fix the MF iface */
	if (!mflib) {
		mflib = &default_mf_interface;
		return 0;
	} 
	if (!mflib->mf_read_init) {
		r2context->last_error = OR2_LIBERR_INVALID_INTERFACE;
		return -1;
	}
	if (!mflib->mf_write_init) {
		r2context->last_error = OR2_LIBERR_INVALID_INTERFACE;
		return -1;
	}
	if (!mflib->mf_detect_tone) {
		r2context->last_error = OR2_LIBERR_INVALID_INTERFACE;
		return -1;
	}
	if (!mflib->mf_generate_tone) {
		r2context->last_error = OR2_LIBERR_INVALID_INTERFACE;
		return -1;
	}
	if (!mflib->mf_select_tone) {
		r2context->last_error = OR2_LIBERR_INVALID_INTERFACE;
		return -1;
	}
	if (!mflib->mf_want_generate) {
		r2context->last_error = OR2_LIBERR_INVALID_INTERFACE;
		return -1;
	}
	/* dispose routines are allowed to be NULL */
	r2context->mflib = mflib;
	return 0;
}

OR2_DECLARE(int) openr2_context_set_transcoder_interface(openr2_context_t *r2context, openr2_transcoder_interface_t *transcoder)
{
	/* fix the transcoder interface */
	if (!transcoder) {
		transcoder = &default_transcoder;
		return 0;
	} 
	if (!transcoder->alaw_to_linear) {
		r2context->last_error = OR2_LIBERR_INVALID_INTERFACE;
		return -1;
	} 
	if (!transcoder->linear_to_alaw) {
		r2context->last_error = OR2_LIBERR_INVALID_INTERFACE;
		return -1;
	}
	r2context->transcoder = transcoder;
	return 0;
}

OR2_DECLARE(int) openr2_context_set_dtmf_interface(openr2_context_t *r2context, openr2_dtmf_interface_t *dtmf_interface)
{
	if (!dtmf_interface) {
		r2context->dtmfeng = &default_dtmf_engine;
		return 0;
	}

	/* check dtmf transmitter implementation */
	if (!dtmf_interface->dtmf_tx_init) {
		return -1;
	}
	if (!dtmf_interface->dtmf_tx_set_timing) {
		return -1;
	}
	if (!dtmf_interface->dtmf_tx_put) {
		return -1;
	}
	if (!dtmf_interface->dtmf_tx) {
		return -1;
	}

	/* check dtmf transmitter implementation */
	if (!dtmf_interface->dtmf_rx_init) {
		return -1;
	}
	if (!dtmf_interface->dtmf_rx_status) {
		return -1;
	}
	if (!dtmf_interface->dtmf_rx) {
		return -1;
	}

	r2context->dtmfeng = dtmf_interface;
	return 0;
}

/* Is this really needed? in anycase, read events from hardware are likely to wake us up
   so probably we could trust on that instead of having the user to call this function? */
OR2_DECLARE(int) openr2_context_get_time_to_next_event(openr2_context_t *r2context)
{
	int res, ms;
	struct timeval currtime;
	/* iterate over all the channels to get the next event time,
	   during this loop, timers cannot be deleted or created */
	openr2_chan_t *current = r2context->chanlist;
	openr2_chan_t *winner = NULL;
	OR2_CONTEXT_STACK;

	openr2_mutex_lock(r2context->timers_lock);

	res = gettimeofday(&currtime, NULL);
	if (-1 == res) {
		openr2_log2(r2context, OR2_CONTEXT_LOG, OR2_LOG_ERROR, "Failed to get next context event time: %s\n", strerror(errno));

		openr2_mutex_unlock(r2context->timers_lock);
		return -1;
	}

	while (current) {
		/* ignore any entry with no sched timer */
		if (current->timers_count < 1) {
			current = current->next;
			continue;
		}
		/* if no winner, set this as winner */
		if (!winner) {
			winner = current;
		}
		current = current->next;
		/* if no more chans, return the winner */
		if (!current) {
			ms = (((winner->sched_timers[0].time.tv_sec - currtime.tv_sec) * 1000) + 
			     ((winner->sched_timers[0].time.tv_usec - currtime.tv_usec) / 1000));

			openr2_mutex_unlock(r2context->timers_lock);

			/* if the time has passed already, return 0 to attend immediately */
			if (ms < 0) {
				return 0;
			}	
			return ms;
		}
		/* if the winner timer is after the current timer, then we have a new winner */
		if (openr2_timercmp(&winner->sched_timers[0].time, &current->sched_timers[0].time, >) ) {
			winner = current;
		}
	}

	openr2_mutex_unlock(r2context->timers_lock);
	return -1;
}

void openr2_context_add_channel(openr2_context_t *r2context, openr2_chan_t *r2chan)
{
	/* put the channel at the head of the list*/
	openr2_chan_t *head = r2context->chanlist;
	OR2_CONTEXT_STACK;
	r2context->chanlist = r2chan;
	r2chan->next = head;
	/* set the channel log level to our level. Users can override this */
	openr2_chan_set_log_level(r2chan, r2context->loglevel);
}

void openr2_context_remove_channel(openr2_context_t *r2context, openr2_chan_t *r2chan)
{
	openr2_chan_t *curr = r2context->chanlist;
	openr2_chan_t *prev = NULL;
	OR2_CONTEXT_STACK;
	while (curr) {
		if(curr == r2chan) {
			if (prev) {
				prev->next = curr->next;
			}	
			if (curr == r2context->chanlist) {
				r2context->chanlist = NULL;
			}	
			break;
		}
		prev = curr;
		curr = curr->next;
	}
}

OR2_DECLARE(void) openr2_context_delete(openr2_context_t *r2context)
{
	openr2_chan_t *current, *next;
	OR2_CONTEXT_STACK;
	current = r2context->chanlist;
	while ( current ) {
		next = current->next;
		openr2_chan_delete(current);
		current = next;
	}
	openr2_mutex_destroy(&r2context->timers_lock);
	free(r2context);
}

OR2_DECLARE(openr2_liberr_t) openr2_context_get_last_error(openr2_context_t *r2context)
{
	OR2_CONTEXT_STACK;
	return r2context->last_error;
}

OR2_DECLARE(const char *) openr2_context_error_string(openr2_liberr_t error)
{
	switch ( error ) {
	case OR2_LIBERR_SYSCALL_FAILED: return "System call failed";
	case OR2_LIBERR_INVALID_CHAN_SIGNALING: return "Invalid channel signaling";
	case OR2_LIBERR_CANNOT_SET_IDLE: return "Failed to set IDLE state on channel";
	case OR2_LIBERR_NO_IO_IFACE_AVAILABLE: return "No I/O interface available for channel";
	case OR2_LIBERR_INVALID_CHAN_NUMBER: return "Invalid channel number";
	case OR2_LIBERR_OUT_OF_MEMORY: return "Out of memory";
	case OR2_LIBERR_INVALID_INTERFACE: return "Invalid interface";
	default: return "*Unknown*";
	}
}

OR2_DECLARE(openr2_variant_t) openr2_context_get_variant(openr2_context_t *r2context)
{
	OR2_CONTEXT_STACK;
	return r2context->variant;
}

OR2_DECLARE(void) openr2_context_set_ani_first(openr2_context_t *r2context, int ani_first)
{
	OR2_CONTEXT_STACK;
	if (ani_first < 0) {
		return;
	}
	r2context->get_ani_first = ani_first ? 1 : 0;
}

OR2_DECLARE(int) openr2_context_get_ani_first(openr2_context_t *r2context)
{
	OR2_CONTEXT_STACK;
	return r2context->get_ani_first;
}

OR2_DECLARE(void) openr2_context_set_skip_category_request(openr2_context_t *r2context, int skipcategory)
{
	OR2_CONTEXT_STACK;
	if (skipcategory < 0) {
		return;
	}
	r2context->skip_category = skipcategory ? 1 : 0;
}

OR2_DECLARE(int) openr2_context_get_skip_category_request(openr2_context_t *r2context)
{
	OR2_CONTEXT_STACK;
	return r2context->skip_category;
}

OR2_DECLARE(void) openr2_context_set_immediate_accept(openr2_context_t *r2context, int immediate_accept)
{
	OR2_CONTEXT_STACK;
	if (immediate_accept < 0) {
		return;
	}
	r2context->immediate_accept = immediate_accept ? 1 : 0;
}

OR2_DECLARE(int) openr2_context_get_immediate_accept(openr2_context_t *r2context)
{
	OR2_CONTEXT_STACK;
	return r2context->immediate_accept;
}

OR2_DECLARE(void) openr2_context_set_log_level(openr2_context_t *r2context, openr2_log_level_t level)
{
	OR2_CONTEXT_STACK;
	r2context->loglevel = level;
}

OR2_DECLARE(openr2_log_level_t) openr2_context_get_log_level(openr2_context_t *r2context)
{
	OR2_CONTEXT_STACK;
	return r2context->loglevel;
}

OR2_DECLARE(void) openr2_context_set_mf_threshold(openr2_context_t *r2context, int threshold)
{
	OR2_CONTEXT_STACK;
	if (threshold < 0) {
		threshold = 0;
	}
	r2context->mf_threshold = threshold;
}

OR2_DECLARE(int) openr2_context_get_mf_threshold(openr2_context_t *r2context)
{
	OR2_CONTEXT_STACK;
	return r2context->mf_threshold;
}

OR2_DECLARE(void) openr2_context_set_dtmf_detection(openr2_context_t *r2context, int enable)
{
	OR2_CONTEXT_STACK;
	if (enable < 0) {
		return;
	}
	r2context->detect_dtmf = enable ? 1 : 0;
}

OR2_DECLARE(int) openr2_context_get_dtmf_detection(openr2_context_t *r2context)
{
	OR2_CONTEXT_STACK;
	return r2context->detect_dtmf;
}

OR2_DECLARE(void) openr2_context_set_dtmf_dialing(openr2_context_t *r2context, int enable, int dtmf_on, int dtmf_off)
{
	OR2_CONTEXT_STACK;
	if (enable < 0) {
		return;
	}
	r2context->dial_with_dtmf = enable ? 1 : 0;
	if (r2context->dial_with_dtmf) {
		r2context->dtmf_on = dtmf_on > 0 ? dtmf_on : OR2_DEFAULT_DTMF_ON;
		r2context->dtmf_off = dtmf_off > 0 ? dtmf_off : OR2_DEFAULT_DTMF_OFF;
	}
}

OR2_DECLARE(int) openr2_context_get_dtmf_dialing(openr2_context_t *r2context, int *dtmf_on, int *dtmf_off)
{
	OR2_CONTEXT_STACK;
	if (dtmf_on) {
		*dtmf_on = r2context->dtmf_on;
	}
	if (dtmf_off) {
		*dtmf_off = r2context->dtmf_off;
	}	
	return r2context->dial_with_dtmf;
}

OR2_DECLARE(int) openr2_context_set_log_directory(openr2_context_t *r2context, char *directory)
{
	struct stat buff;
#ifdef WIN32
	mode_t perms = 0;
#else
	mode_t perms = (S_IRWXU) | (S_IRGRP | S_IXGRP) | (S_IROTH | S_IXOTH); /* rwx | rx | rx */
#endif
	OR2_CONTEXT_STACK;
	if (!directory) {
		return -1;
	}
	/* make sure it will fit */
	if (strlen(directory) >= sizeof(r2context->logdir)) {
		return -1;
	}
	/* no point on checking whether or not is a directory, if it
	   exists as a non-directory entry, oh well, logging will fail */
	if (stat(directory, &buff)) {
		/* stat failed, let's see if its because of a directory does not exists */
		if (errno != ENOENT) {
			return -1;
		}
		/* let's try to create the directory */
		if (openr2_mkdir_recursive(directory, perms)) {
			return -1;
		}
	}
	strncpy(r2context->logdir, directory, sizeof(r2context->logdir)-1);
	r2context->logdir[sizeof(r2context->logdir)-1] = 0;
	return 0;
}

OR2_DECLARE(char *) openr2_context_get_log_directory(openr2_context_t *r2context, char *directory, int len)
{
	OR2_CONTEXT_STACK;
	if (!directory) {
		return NULL;
	}
	strncpy(directory, r2context->logdir, len-1);
	directory[len-1] = 0;
	return directory;
}

OR2_DECLARE(int) openr2_context_get_max_ani(openr2_context_t *r2context)
{
	OR2_CONTEXT_STACK;
	return r2context->max_ani;
}

OR2_DECLARE(int) openr2_context_get_max_dnis(openr2_context_t *r2context)
{
	OR2_CONTEXT_STACK;
	return r2context->max_dnis;
}

OR2_DECLARE(void) openr2_context_set_mf_back_timeout(openr2_context_t *r2context, int ms)
{
	OR2_CONTEXT_STACK;
	/* ignore any timeout less than 0 */
	if (ms < 0) {
		return;
	}
	r2context->timers.mf_back_cycle = ms;
}

OR2_DECLARE(int) openr2_context_get_mf_back_timeout(openr2_context_t *r2context)
{
	OR2_CONTEXT_STACK;
	return r2context->timers.mf_back_cycle;
}

OR2_DECLARE(void) openr2_context_set_metering_pulse_timeout(openr2_context_t *r2context, int ms)
{
	OR2_CONTEXT_STACK;
	/* ignore any timeout less than 0 */
	if (ms < 0) {
		return;
	}
	r2context->timers.r2_metering_pulse = ms;
}

OR2_DECLARE(int) openr2_context_get_metering_pulse_timeout(openr2_context_t *r2context)
{
	OR2_CONTEXT_STACK;
	return r2context->timers.r2_metering_pulse;
}

OR2_DECLARE(void) openr2_context_set_double_answer(openr2_context_t *r2context, int enable)
{
	OR2_CONTEXT_STACK;
	if (enable < 0) {
		return;
	}
	r2context->double_answer = enable;
}

OR2_DECLARE(int) openr2_context_get_double_answer(openr2_context_t *r2context)
{
	OR2_CONTEXT_STACK;
	return r2context->double_answer ? 1 : 0;
}

OR2_DECLARE(int) openr2_context_set_io_type(openr2_context_t *r2context, openr2_io_type_t io_type, openr2_io_interface_t *io_interface)
{
	openr2_io_interface_t *internal_io_interface = NULL;
	OR2_CONTEXT_STACK;
	switch (io_type) {
	case OR2_IO_CUSTOM:
		/* sanity check for all members */
		if (!io_interface) {
			openr2_log2(r2context, OR2_CONTEXT_LOG, OR2_LOG_ERROR, "I/O interface cannot be null!\n");
			return -1;
		}
		if (!io_interface->open) {
			openr2_log2(r2context, OR2_CONTEXT_LOG, OR2_LOG_ERROR, "Unspecified I/O interface method: open\n");
			return -1;
		}
		if (!io_interface->close) {
			openr2_log2(r2context, OR2_CONTEXT_LOG, OR2_LOG_ERROR, "Unspecified I/O interface method: close\n");
			return -1;
		}
		if (!io_interface->set_cas) {
			openr2_log2(r2context, OR2_CONTEXT_LOG, OR2_LOG_ERROR, "Unspecified I/O interface method: set_cas\n");
			return -1;
		}
		if (!io_interface->get_cas) {
			openr2_log2(r2context, OR2_CONTEXT_LOG, OR2_LOG_ERROR, "Unspecified I/O interface method: get_cas\n");
			return -1;
		}
		if (!io_interface->flush_write_buffers) {
			openr2_log2(r2context, OR2_CONTEXT_LOG, OR2_LOG_ERROR, "Unspecified I/O interface method: flush_write_buffers\n");
			return -1;
		}
		if (!io_interface->write) {
			openr2_log2(r2context, OR2_CONTEXT_LOG, OR2_LOG_ERROR, "Unspecified I/O interface method: write\n");
			return -1;
		}
		if (!io_interface->read) {
			openr2_log2(r2context, OR2_CONTEXT_LOG, OR2_LOG_ERROR, "Unspecified I/O interface method: read\n");
			return -1;
		}
		if (!io_interface->setup) {
			openr2_log2(r2context, OR2_CONTEXT_LOG, OR2_LOG_ERROR, "Unspecified I/O interface method: setup\n");
			return -1;
		}
		if (!io_interface->wait) {
			openr2_log2(r2context, OR2_CONTEXT_LOG, OR2_LOG_ERROR, "Unspecified I/O interface method: wait\n");
			return -1;
		}
		if (!io_interface->get_oob_event) {
			openr2_log2(r2context, OR2_CONTEXT_LOG, OR2_LOG_ERROR, "Unspecified I/O interface method: get_oob_event\n");
			return -1;
		}
		r2context->io = io_interface;
		r2context->io_type = io_type;
		return 0;
	case OR2_IO_ZT:
		/* check that zaptel interface is available */
		internal_io_interface = openr2_io_get_zt_interface();
		if (!internal_io_interface) {
			openr2_log2(r2context, OR2_CONTEXT_LOG, OR2_LOG_ERROR, "Unavailable Zaptel or DAHDI I/O interface.\n");
			return -1;
		}
		r2context->io_type = io_type;
		r2context->io = internal_io_interface;
		return 0;
	case OR2_IO_DEFAULT:
		/* check first if zaptel interface is available */
		internal_io_interface = openr2_io_get_zt_interface();
		if (!internal_io_interface) {
			/* use dummy io interface and print a notice message. we expect the user to set his implementation, by later
			   calling openr2_context_set_io_type() again */
			openr2_log2(r2context, OR2_CONTEXT_LOG, OR2_LOG_NOTICE, "Unavailable default I/O interface, using dummy.\n");
			internal_io_interface = openr2_io_get_dummy_interface();
		}
		r2context->io_type = io_type;
		r2context->io = internal_io_interface;
		return 0;
	default:
		break;
	}
	openr2_log2(r2context, OR2_CONTEXT_LOG, OR2_LOG_ERROR, "Invalid I/O type %d\n", io_type);
	return -1;
}

#define LOADTONE(mytone) \
	else if (1 == sscanf(line, #mytone "=%c", (char *)&intvalue)) { \
		openr2_log2(r2context, OR2_CONTEXT_LOG, OR2_LOG_DEBUG, "Found value %d for tone %s\n", intvalue, #mytone); \
		if (strchr("1234567890BCDEF", intvalue)) { \
			openr2_log2(r2context, OR2_CONTEXT_LOG, OR2_LOG_DEBUG, "Changing tone %s from %02X to %02X\n", \
			#mytone, r2context->mytone, intvalue); \
			r2context->mytone = intvalue; \
		} else { \
			openr2_log2(r2context, OR2_CONTEXT_LOG, OR2_LOG_DEBUG, "Disabling tone %s, its value was %02X\n", \
			#mytone, r2context->mytone); \
			r2context->mytone = OR2_MF_TONE_INVALID; \
		} \
	}

#define LOADTIMER(mytimer) \
	else if (1 == sscanf(line, #mytimer "=%d", &intvalue)) { \
		openr2_log2(r2context, OR2_CONTEXT_LOG, OR2_LOG_DEBUG, "Found value %d for timer %s\n", intvalue, #mytimer); \
		if (intvalue >= 0) { \
			openr2_log2(r2context, OR2_CONTEXT_LOG, OR2_LOG_DEBUG, "Changing timer %s from %d to %d\n", \
			#mytimer, r2context->mytimer, intvalue); \
			r2context->mytimer = intvalue; \
		} \
	}

#define LOADSETTING(mysetting) \
	else if (1 == sscanf(line, #mysetting "=%d", &intvalue)) { \
		openr2_log2(r2context, OR2_CONTEXT_LOG, OR2_LOG_DEBUG, "Found value %d for setting %s\n", intvalue, #mysetting); \
		if (intvalue >= 0) { \
			openr2_log2(r2context, OR2_CONTEXT_LOG, OR2_LOG_DEBUG, "Changing setting %s from %d to %d\n", \
			#mysetting, r2context->mysetting, intvalue); \
			r2context->mysetting = intvalue; \
		} \
	}

OR2_DECLARE(int) openr2_context_configure_from_advanced_file(openr2_context_t *r2context, const char *filename)
{
	FILE *variant_file;
	int intvalue = 0;
	char line[255];
	OR2_CONTEXT_STACK;
	if (!filename) {
		return -1;
	}
	variant_file = fopen(filename, "r");
	if (!variant_file) {
		openr2_log2(r2context, OR2_CONTEXT_LOG, OR2_LOG_ERROR, "Failed to open R2 variant file '%s'\n", filename);
		return -1;
	}
	openr2_log2(r2context, OR2_CONTEXT_LOG, OR2_LOG_NOTICE, "Reading R2 definitions from protocol file '%s'\n", filename);
	while (fgets(line, sizeof(line), variant_file))	{

		if ('#' == line[0] || '\n' == line[0] || ' ' == line[0]) {
			continue;
		}

		/* Group A tones */
		LOADTONE(mf_ga_tones.request_next_ani_digit)
		LOADTONE(mf_ga_tones.request_next_dnis_digit)
		LOADTONE(mf_ga_tones.request_dnis_minus_1)
		LOADTONE(mf_ga_tones.request_dnis_minus_2)
		LOADTONE(mf_ga_tones.request_dnis_minus_3)
		LOADTONE(mf_ga_tones.request_all_dnis_again)
		LOADTONE(mf_ga_tones.request_category)
		LOADTONE(mf_ga_tones.request_category_and_change_to_gc)
		LOADTONE(mf_ga_tones.request_change_to_g2)
		LOADTONE(mf_ga_tones.address_complete_charge_setup)
		LOADTONE(mf_ga_tones.network_congestion)

		/* Group B tones */
		LOADTONE(mf_gb_tones.accept_call_with_charge)
		LOADTONE(mf_gb_tones.accept_call_no_charge)
		LOADTONE(mf_gb_tones.busy_number)
		LOADTONE(mf_gb_tones.network_congestion)
		LOADTONE(mf_gb_tones.unallocated_number)
		LOADTONE(mf_gb_tones.line_out_of_order)
		LOADTONE(mf_gb_tones.special_info_tone)
		LOADTONE(mf_gb_tones.reject_collect_call)
		LOADTONE(mf_gb_tones.number_changed)

		/* Group C tones */
		LOADTONE(mf_gc_tones.request_next_ani_digit)
		LOADTONE(mf_gc_tones.request_change_to_g2)
		LOADTONE(mf_gc_tones.request_next_dnis_digit_and_change_to_ga)

		/* Group I tones */
		LOADTONE(mf_g1_tones.no_more_dnis_available)
		LOADTONE(mf_g1_tones.no_more_ani_available)
		LOADTONE(mf_g1_tones.caller_ani_is_restricted)

		/* Group II tones */
		LOADTONE(mf_g2_tones.national_subscriber)
		LOADTONE(mf_g2_tones.national_priority_subscriber)
		LOADTONE(mf_g2_tones.international_subscriber)
		LOADTONE(mf_g2_tones.international_priority_subscriber)
		LOADTONE(mf_g2_tones.collect_call)

		/* Timers */
		LOADTIMER(timers.mf_back_cycle)
		LOADTIMER(timers.mf_back_resume_cycle)
		LOADTIMER(timers.mf_fwd_safety)
		LOADTIMER(timers.r2_seize)
		LOADTIMER(timers.r2_answer)
		LOADTIMER(timers.r2_metering_pulse)
		LOADTIMER(timers.r2_double_answer)
		LOADTIMER(timers.r2_answer_delay)
		LOADTIMER(timers.cas_persistence_check)
		LOADTIMER(timers.dtmf_start_dial)

		/* misc settings */
		LOADSETTING(mf_threshold)
	}
	r2context->configured_from_file = 1;
	fclose(variant_file);
	return 0;
}
#undef LOADTONE
#undef LOADTIMER

