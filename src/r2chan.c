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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#include "openr2/r2thread.h"
#include "openr2/r2log-pvt.h"
#include "openr2/r2utils-pvt.h"
#include "openr2/r2proto-pvt.h"
#include "openr2/r2chan-pvt.h"
#include "openr2/r2context-pvt.h"
#include "openr2/r2ioabs.h"

/* helpers to lock the channel when setting and getting properties */
#define OR2_CHAN_SET_PROP(property,value) openr2_chan_lock(r2chan); \
				    r2chan->property = value; \
				    openr2_chan_unlock(r2chan);

#define OR2_CHAN_RET_PROP(type,property) type retproperty; \
				       openr2_chan_lock(r2chan); \
				       retproperty = r2chan->property; \
				       openr2_chan_unlock(r2chan); \
				       return retproperty;


static openr2_chan_t *__openr2_chan_new(openr2_context_t *r2context, int channo, int openchan, openr2_io_fd_t chanfd)
{
	openr2_chan_t *r2chan = NULL;
	int alarm_state = 0;
#ifdef OR2_MF_DEBUG
	char logfile[1024];
#endif
	r2chan = calloc(1, sizeof(*r2chan));
	if (!r2chan) {
		r2context->last_error = OR2_LIBERR_SYSCALL_FAILED;
		openr2_log2(r2context, OR2_CONTEXT_LOG, OR2_LOG_ERROR, "Failed to allocate memory for r2chan %d\n", channo);
		return NULL;
	}
#ifdef OR2_MF_DEBUG
	/* open the channel log */
	snprintf(logfile, sizeof(logfile)-1, "openr2-chan-%d-tx.raw", channo);
	logfile[sizeof(logfile)-1] = 0;
	r2chan->mf_write_fd = open(logfile, O_CREAT | O_TRUNC | O_WRONLY, 0666);
	if (-1 == r2chan->mf_read_fd) {
		r2context->last_error = OR2_LIBERR_SYSCALL_FAILED;
		openr2_log2(r2context, OR2_CONTEXT_LOG, OR2_LOG_ERROR, "Failed to open MF-tx debug file %s for chan %d: %s\n", logfile, strerror(errno), channo);
		free(r2chan);
		return NULL;
	}
	snprintf(logfile, sizeof(logfile)-1, "openr2-chan-%d-rx.raw", channo);
	logfile[sizeof(logfile)-1] = 0;
	r2chan->mf_read_fd = open(logfile, O_CREAT | O_TRUNC | O_WRONLY, 0666);
	if (-1 == r2chan->mf_read_fd) {
		r2context->last_error = OR2_LIBERR_SYSCALL_FAILED;
		openr2_log2(r2context, OR2_CONTEXT_LOG, OR2_LOG_ERROR, "Failed to open MF-rx debug file %s for chan %d: %s\n", logfile, strerror(errno), channo);
		close(r2chan->mf_write_fd);
		free(r2chan);
		return NULL;
	}
#endif

	openr2_mutex_create(&r2chan->lock);

	/* no persistence check has been done */
	r2chan->cas_persistence_check_signal = -1;

	/* start with read disabled, we only read when there is a call being setup */
	r2chan->read_enabled = 0;

	/* set the owner context */
	r2chan->r2context = r2context;

	/* DTMF and MF tone detection default hooks handles */
	r2chan->dtmf_write_handle = &r2chan->default_dtmf_write_handle;
	r2chan->dtmf_read_handle = &r2chan->default_dtmf_read_handle;

	r2chan->mf_write_handle = &r2chan->default_mf_write_handle;
	r2chan->mf_read_handle = &r2chan->default_mf_read_handle;

	/* set default logger and default logging level */
	r2chan->on_channel_log = openr2_log_channel_default;

	/* set CAS indicators to invalid */
	r2chan->cas_rx_signal = OR2_CAS_INVALID;
	r2chan->cas_tx_signal = OR2_CAS_INVALID;

	/* start the timer id in 1 to avoid confusion when memset'ing */
	r2chan->timer_id = 1;

	/* we do not start blocked nor idle  */
	r2chan->r2_state = OR2_INIT;

	/* open channel only if requested */
	if (openchan) {
		/* channel fd */
		r2chan->fd = openr2_io_open(r2context, channo);
		if (!r2chan->fd) {
			openr2_chan_delete(r2chan);
			return NULL;
		}
		/* setup the I/O for R2 signaling */
		if (openr2_io_setup(r2chan)) {
			openr2_chan_delete(r2chan);
			return NULL;
		}
		r2chan->fd_created = 1;
	} else {
		r2chan->fd = chanfd;
		r2chan->fd_created = 0;
	}	

	r2chan->number = channo;
	r2chan->io_buf_size = OR2_CHAN_READ_SIZE;

	/* add ourselves to the list of channels in the context */
	openr2_context_add_channel(r2context, r2chan);

	/* check for alarms */
	openr2_io_get_alarm_state(r2chan, &alarm_state);
	if (alarm_state) {
		r2chan->inalarm = alarm_state;
		openr2_proto_handle_alarm_state(r2chan);
	}

	return r2chan;
}

OR2_DECLARE(void) openr2_chan_set_span_id(openr2_chan_t *r2chan, int span_id)
{
	openr2_chan_lock(r2chan);
	openr2_log(r2chan, OR2_CHANNEL_LOG, OR2_LOG_DEBUG, "Setting span_id: %d\n", span_id);
	r2chan->span_id = span_id;
	openr2_chan_unlock(r2chan);
}

OR2_DECLARE(int) openr2_chan_set_dtmf_handles(openr2_chan_t *r2chan, void *dtmf_read_handle, void *dtmf_write_handle)
{
	openr2_chan_lock(r2chan);

	if (!dtmf_write_handle) {

		openr2_chan_unlock(r2chan);

		return -1;
	}
	if (!dtmf_read_handle) {

		openr2_chan_unlock(r2chan);

		return -1;
	}

	r2chan->dtmf_write_handle = dtmf_write_handle;
	r2chan->dtmf_read_handle = dtmf_read_handle;

	openr2_chan_unlock(r2chan);
	return 0;
}

OR2_DECLARE(openr2_chan_t *) openr2_chan_new(openr2_context_t *r2context, int channo)
{
	if (channo <=0 ) {
		openr2_log2(r2context, OR2_CONTEXT_LOG, OR2_LOG_ERROR, "Invalid channel number %d\n", channo);
		r2context->last_error = OR2_LIBERR_INVALID_CHAN_NUMBER;
		return NULL;
	}
	return __openr2_chan_new(r2context, channo, 1, NULL);
}

OR2_DECLARE(openr2_chan_t *) openr2_chan_new_from_fd(openr2_context_t *r2context, openr2_io_fd_t chanfd, int channo)
{
	openr2_chan_t *r2chan = NULL;
	if (channo <=0 ) {
		openr2_log2(r2context, OR2_CONTEXT_LOG, OR2_LOG_ERROR, "Invalid channel number %d\n", channo);
		r2context->last_error = OR2_LIBERR_INVALID_CHAN_NUMBER;
		return NULL;
	}
	r2chan = __openr2_chan_new(r2context, channo, 0, chanfd);
	if (!r2chan) {
		return NULL;
	}
	return r2chan;
}

OR2_DECLARE(int) openr2_chan_set_mflib_handles(openr2_chan_t *r2chan, void *mf_write_handle, void *mf_read_handle)
{
	openr2_chan_lock(r2chan);
	if (mf_write_handle) {
		r2chan->mf_write_handle = mf_write_handle;
	}
	if (mf_read_handle) {
		r2chan->mf_read_handle = mf_read_handle;
	}
	openr2_chan_unlock(r2chan);
	return 0;
}

static int openr2_chan_handle_oob_event(openr2_chan_t *r2chan, openr2_oob_event_t event)
{
	switch (event) {
	case OR2_OOB_EVENT_CAS_CHANGE:
		openr2_log(r2chan, OR2_CHANNEL_LOG, OR2_LOG_DEBUG, "Handling OOB CAS event\n");
		openr2_proto_handle_cas(r2chan);
		break;
	case OR2_OOB_EVENT_ALARM_ON:
	case OR2_OOB_EVENT_ALARM_OFF:
		openr2_log(r2chan, OR2_CHANNEL_LOG, OR2_LOG_DEBUG, (event == OR2_OOB_EVENT_ALARM_ON) ? "Alarm Raised\n" : "Alarm Cleared\n");
		r2chan->inalarm = (event == OR2_OOB_EVENT_ALARM_ON) ? 1 : 0;

		/* give the user first a chance to do something */
		EMI(r2chan)->on_hardware_alarm(r2chan, r2chan->inalarm);

		openr2_proto_handle_alarm_state(r2chan);
		break;
	default:
		openr2_log(r2chan, OR2_CHANNEL_LOG, OR2_LOG_NOTICE, "Unhandled OOB event %d\n", event);
		break;
	}
	return 0;
}

/*! \brief must be called with chan lock held */
static int openr2_chan_handle_timers(openr2_chan_t *r2chan)
{
	struct timeval nowtv;
	openr2_sched_timer_t to_dispatch[OR2_MAX_SCHED_TIMERS];
	int res, ms, t, i, timerid;

	res = gettimeofday(&nowtv, NULL);
	if (res == -1) {
		openr2_log(r2chan, OR2_CHANNEL_LOG, OR2_LOG_ERROR, "Yikes! gettimeofday failed, me may miss events!!\n");
		return -1;
	}
	i = 0;

	/* get the timers to dispatch */
	for (t = 0; t < r2chan->timers_count; t++) {
		ms = ((r2chan->sched_timers[t].time.tv_sec - nowtv.tv_sec) * 1000) +
		     ((r2chan->sched_timers[t].time.tv_usec - nowtv.tv_usec)/1000);
		if (ms <= 0) {
			memcpy(&to_dispatch[i], &r2chan->sched_timers[t], sizeof(to_dispatch[0]));
			i++;
		}	
	}
	
	/* cancell them */
	for (t = 0 ; t < i; t++) {
		timerid = to_dispatch[t].id;
		openr2_chan_cancel_timer(r2chan, &timerid);
	}

	/* dispatch them */
	for (t = 0; t < i; t++) {
		openr2_log(r2chan, OR2_CHANNEL_LOG, OR2_LOG_DEBUG, "calling timer %d (%s) callback\n", to_dispatch[t].id, to_dispatch[t].name);
		to_dispatch[t].callback(r2chan);
	}
	return 0;
}

OR2_DECLARE(int) openr2_chan_run_schedule(openr2_chan_t *r2chan)
{
	int ret = 0;
	openr2_chan_lock(r2chan);
	ret = openr2_chan_handle_timers(r2chan);
	openr2_chan_unlock(r2chan);
	return ret;
}

/*! \brief simple mask to determine what the user wants to process */
#define OR2_CHAN_PROCESS_OOB (1 << 0)
#define OR2_CHAN_PROCESS_MF (1 << 1)

#define HANDLE_IO_WRITE_RESULT(wrote) \
			if (!wrote) { \
				openr2_log(r2chan, OR2_CHANNEL_LOG, OR2_LOG_ERROR, "No bytes written to channel %d when %d bytes were requested\n", r2chan->number, res); \
				goto done; \
			} \
			if (wrote == -1) { \
				retcode = -1; \
				goto done; \
			} \
			if (wrote != res) { \
				openr2_log(r2chan, OR2_CHANNEL_LOG, OR2_LOG_ERROR, "Just wrote %d bytes to channel %d when %d bytes were requested\n", wrote, r2chan->number, res); \
			}

/*! \brief main processing of signaling to check for incoming events, respond to them and dispatch user events */
static int openr2_chan_process(openr2_chan_t *r2chan, int processing_mask)
{
	int interesting_events, res, tone_result, wrote;
	openr2_oob_event_t event;
	unsigned i;
	uint8_t read_buf[OR2_CHAN_READ_SIZE];
	int16_t tone_buf[OR2_CHAN_READ_SIZE];
	/* just one return point in this function, set retcode and call goto done when done */
	int retcode = 0;

	openr2_chan_lock(r2chan);
	openr2_chan_handle_timers(r2chan);

tryagain:
	/* check for CAS and ALARM events only if requested */
	interesting_events = (processing_mask & OR2_CHAN_PROCESS_OOB) ? OR2_IO_OOB_EVENT : 0;

	/* we also want to be notified about read-ready if we have read enabled and the user requested MF processing */
	if (r2chan->read_enabled && (processing_mask & OR2_CHAN_PROCESS_MF)) {
		/* XXX read enabled is NOT enough, we should also check if the MF engine is turned on or the channel is answered XXX*/
		interesting_events |= OR2_IO_READ;
	}

	/* we also want to be notified about write-ready if we're in the MF process and have some tone to write */
	if (!(processing_mask & OR2_CHAN_PROCESS_MF)) {
		/* mf should be ignored, therefore OR2_IO_WRITE must not be enabled regardless of other flags */
	} else if (r2chan->dialing_dtmf) {
		interesting_events |= OR2_IO_WRITE;
	} else if (OR2_MF_OFF_STATE != r2chan->mf_state && 
			MFI(r2chan)->mf_want_generate(r2chan->mf_write_handle, r2chan->mf_write_tone) ) {
		interesting_events |= OR2_IO_WRITE;
	}

	if (r2chan->inalarm) {
		/* if we're in alarm, clear any other events and just poll for OOB */
		interesting_events = OR2_IO_OOB_EVENT;
	}

	/* ask the I/O layer to poll for the requested events immediately, no blocking */
	res = openr2_io_wait(r2chan, &interesting_events, 0);
	if (res) {
		retcode = -1;
		goto done;
	}

	/* if there is no interesting events, do nothing */
	if (!interesting_events) {
		retcode = 0;
		goto done;
	}

	/* if there is an OOB event, probably CAS bits just changed */
	if (OR2_IO_OOB_EVENT & interesting_events) {
		res = openr2_io_get_oob_event(r2chan, &event);
		if (!res && event != OR2_OOB_EVENT_NONE) {
			openr2_chan_handle_oob_event(r2chan, event);
		}
	}

	if (r2chan->read_enabled && (OR2_IO_READ & interesting_events)) {
		res = openr2_io_read(r2chan, read_buf, sizeof(read_buf));
		if (-1 == res) {
			retcode = -1;
			goto done;
		}
		if (!res) {
			/* if nothing was read, continue, may be there is a priority event (ie DAHDI read ELAST) */
			goto tryagain;
		}
		/* if the DTMF or MF detector is enabled, we are supposed to detect tones */
		if (r2chan->mf_state != OR2_MF_OFF_STATE) {
			/* assuming ALAW codec */
			for (i = 0; i < (uint32_t) res; i++) {
				tone_buf[i] = TI(r2chan)->alaw_to_linear(read_buf[i]);
			}	
#ifdef OR2_MF_DEBUG	
			write(r2chan->mf_read_fd, tone_buf, res*2);
#endif
			if (r2chan->detecting_dtmf) {
				DTMF(r2chan)->dtmf_rx(r2chan->dtmf_read_handle, tone_buf, res);
				res = DTMF(r2chan)->dtmf_rx_status(r2chan->dtmf_read_handle);
				if (!res) {
					r2chan->dtmf_silence_samples += OR2_CHAN_READ_SIZE;
					if (r2chan->dtmf_silence_samples == OR2_DTMF_MAX_SILENCE_SAMPLES) {
						openr2_log(r2chan, OR2_CHANNEL_LOG, OR2_LOG_DEBUG, "Done with DTMF detection\n");
						openr2_proto_handle_dtmf_end(r2chan);
						goto checkwrite;
					}
				}
			} else {
				tone_result = MFI(r2chan)->mf_detect_tone(r2chan->mf_read_handle, tone_buf, res);
				if ( tone_result != -1 ) {
					openr2_proto_handle_mf_tone(r2chan, tone_result);
				} 
			}
		} else if (r2chan->answered) {
			EMI(r2chan)->on_call_read(r2chan, read_buf, res);
		}
	}

checkwrite:

	/* we only write MF or DTMF tones here. Speech write is responsibility of the user, she should call openr2_chan_write for that */
	if (r2chan->dialing_dtmf && (OR2_IO_WRITE & interesting_events)) {
		res = DTMF(r2chan)->dtmf_tx(r2chan->dtmf_write_handle, tone_buf, r2chan->io_buf_size);
		if (res <= 0) {
			openr2_log(r2chan, OR2_CHANNEL_LOG, OR2_LOG_DEBUG, "Done with DTMF generation\n");
			openr2_proto_handle_dtmf_end(r2chan);
			goto tryagain;
		}
		for (i = 0; i < (uint32_t) res; i++) {
			read_buf[i] = TI(r2chan)->linear_to_alaw(tone_buf[i]);
		}
		wrote = openr2_io_write(r2chan, read_buf, res);
		HANDLE_IO_WRITE_RESULT(wrote);
	} else if ((OR2_MF_OFF_STATE != r2chan->mf_state) &&
			(OR2_IO_WRITE & interesting_events)) {
		res = MFI(r2chan)->mf_generate_tone(r2chan->mf_write_handle, tone_buf, r2chan->io_buf_size);
		/* if there are no samples to convert and write then continue,
		   the generate routine already took care of it */
		if (!res) {
			goto tryagain;
		}
		/* an error on tone generation, lets just bail out and hope for the best */
		if (-1 == res) {
			openr2_log(r2chan, OR2_CHANNEL_LOG, OR2_LOG_ERROR, "Failed to generate MF tone.\n");
			retcode = -1;
			goto done;
		}
#ifdef OR2_MF_DEBUG
		write(r2chan->mf_write_fd, tone_buf, res*2);
#endif
		for (i = 0; i < (uint32_t) res; i++) {
			read_buf[i] = TI(r2chan)->linear_to_alaw(tone_buf[i]);
		}
		wrote = openr2_io_write(r2chan, read_buf, res);
		HANDLE_IO_WRITE_RESULT(wrote);
	}

	goto tryagain;

done:
	openr2_chan_unlock(r2chan);
	return retcode;
}

OR2_DECLARE(int) openr2_chan_process_mf_signaling(openr2_chan_t *r2chan)
{
	return openr2_chan_process(r2chan, OR2_CHAN_PROCESS_MF);
}

OR2_DECLARE(int) openr2_chan_process_oob_events(openr2_chan_t *r2chan)
{
	return openr2_chan_process(r2chan, OR2_CHAN_PROCESS_OOB);
}

OR2_DECLARE(int) openr2_chan_process_cas_signaling(openr2_chan_t *r2chan)
{
	return openr2_proto_handle_cas(r2chan);
}

OR2_DECLARE(int) openr2_chan_process_signaling(openr2_chan_t *r2chan)
{
	return openr2_chan_process(r2chan, OR2_CHAN_PROCESS_MF | OR2_CHAN_PROCESS_OOB);
}

int openr2_chan_add_timer(openr2_chan_t *r2chan, int ms, openr2_callback_t callback, const char *name)
{
	int myerrno;
	struct timeval tv;
	openr2_sched_timer_t newtimer;
	int res;
	int i;

	openr2_mutex_lock(r2chan->r2context->timers_lock);

	res = gettimeofday(&tv, NULL);
	if (-1 == res) {
		myerrno = errno;

		openr2_mutex_unlock(r2chan->r2context->timers_lock);

		openr2_log(r2chan, OR2_CHANNEL_LOG, OR2_LOG_ERROR, "Failed to get time of day to schedule timer!!");
		EMI(r2chan)->on_os_error(r2chan, myerrno);
		return -1;
	}
	if (r2chan->timers_count == OR2_MAX_SCHED_TIMERS) {

		openr2_mutex_unlock(r2chan->r2context->timers_lock);

		openr2_log(r2chan, OR2_CHANNEL_LOG, OR2_LOG_ERROR, "No more time slots, failed to schedule timer, this is bad!\n");
		return -1;
	}
	/* build the new timer */
	newtimer.time.tv_sec = tv.tv_sec + (ms / 1000);
	newtimer.time.tv_usec = tv.tv_usec + (ms % 1000) * 1000;
	/* more than 1000000 microseconds, then increment one second */
	 if (newtimer.time.tv_usec > 1000000) {
		 newtimer.time.tv_sec += 1;
		 newtimer.time.tv_usec -= 1000000;
	}
	newtimer.callback = callback;
	newtimer.name = name;
	newtimer.id = ++r2chan->timer_id;
	/* find the proper slot for the timer */
	for (i = 0; i < r2chan->timers_count; i++) {
		if (openr2_timercmp(&newtimer.time, &r2chan->sched_timers[i].time, <)) {
			memmove(&r2chan->sched_timers[i+1], 
				&r2chan->sched_timers[i], 
				(r2chan->timers_count-i) * sizeof(r2chan->sched_timers[0]));
			memcpy(&r2chan->sched_timers[i], &newtimer, sizeof(newtimer));
			break;
		}
	}
	/* this means the new timer will be triggered at the end */
	if (i == r2chan->timers_count) {
		memcpy(&r2chan->sched_timers[i], &newtimer, sizeof(newtimer));
	}
	r2chan->timers_count++;

	openr2_mutex_unlock(r2chan->r2context->timers_lock);
	openr2_log(r2chan, OR2_CHANNEL_LOG, OR2_LOG_EX_DEBUG, "scheduled timer id %d (%s)\n", newtimer.id, newtimer.name);
	return newtimer.id;
}

void openr2_chan_cancel_timer(openr2_chan_t *r2chan, int *timer_id)
{
	int i = 0;
	openr2_log(r2chan, OR2_CHANNEL_LOG, OR2_LOG_EX_DEBUG, "Attempting to cancel timer %d\n", *timer_id);
	if (*timer_id < 1) {
		openr2_log(r2chan, OR2_CHANNEL_LOG, OR2_LOG_EX_DEBUG, "Cannot cancel timer %d\n", *timer_id);
		return;
	}

	openr2_mutex_lock(r2chan->r2context->timers_lock);

	for ( ; i < r2chan->timers_count; i++) {
		if (r2chan->sched_timers[i].id == *timer_id) {
			openr2_log(r2chan, OR2_CHANNEL_LOG, OR2_LOG_EX_DEBUG, "timer id %d found, cancelling it now\n", *timer_id);
			/* clear the timer and move down the list */
			memset(&r2chan->sched_timers[i], 0, sizeof(r2chan->sched_timers[0]));
			if (i < (r2chan->timers_count - 1)) {
				memmove(&r2chan->sched_timers[i], &r2chan->sched_timers[i+1], 
				       (r2chan->timers_count - (i + 1))*sizeof(r2chan->sched_timers[0]));
			}
			r2chan->timers_count--;
			*timer_id = 0;
			break;
		}
	}

	openr2_mutex_unlock(r2chan->r2context->timers_lock);
}

void openr2_chan_cancel_all_timers(openr2_chan_t *r2chan)
{
	openr2_mutex_lock(r2chan->r2context->timers_lock);

	r2chan->timers_count = 0;
	r2chan->timer_id = 1;
	memset(&r2chan->timer_ids, 0, sizeof(r2chan->timer_ids));
	memset(r2chan->sched_timers, 0, sizeof(r2chan->sched_timers));

	openr2_mutex_unlock(r2chan->r2context->timers_lock);
}

OR2_DECLARE(void) openr2_chan_delete(openr2_chan_t *r2chan)
{
	openr2_chan_lock(r2chan);

	/* let know the protocol layer this channel is going down */
	openr2_proto_destroy(r2chan);

	if (r2chan->fd_created) {
		openr2_io_close(r2chan);
	}
	if (r2chan->logfile) {
		fclose(r2chan->logfile);
	}
#ifdef OR2_MF_DEBUG
	close(r2chan->mf_write_fd);
	close(r2chan->mf_read_fd);
#endif
	openr2_chan_unlock(r2chan);
	free(r2chan);
}

OR2_DECLARE(int) openr2_chan_accept_call(openr2_chan_t *r2chan, openr2_call_mode_t mode)
{
	int retcode = 0;
	openr2_chan_lock(r2chan);
	retcode = openr2_proto_accept_call(r2chan, mode);
	openr2_chan_unlock(r2chan);
	return retcode;
}

OR2_DECLARE(int) openr2_chan_answer_call(openr2_chan_t *r2chan)
{
	int retcode = 0;
	openr2_chan_lock(r2chan);
	retcode = openr2_proto_answer_call(r2chan);
	openr2_chan_unlock(r2chan);
	return retcode;
}

OR2_DECLARE(int) openr2_chan_answer_call_with_mode(openr2_chan_t *r2chan, openr2_answer_mode_t mode)
{
	int retcode = 0;
	openr2_chan_lock(r2chan);
	retcode = openr2_proto_answer_call_with_mode(r2chan, mode);
	openr2_chan_unlock(r2chan);
	return retcode;
}

OR2_DECLARE(int) openr2_chan_disconnect_call(openr2_chan_t *r2chan, openr2_call_disconnect_cause_t cause)
{
	int retcode = 0;
	openr2_chan_lock(r2chan);
	retcode = openr2_proto_disconnect_call(r2chan, cause);
	openr2_chan_unlock(r2chan);
	return retcode;
}

OR2_DECLARE(int) openr2_chan_set_idle(openr2_chan_t *r2chan)
{
	int retcode = 0;
	openr2_chan_lock(r2chan);
	retcode = openr2_proto_set_idle(r2chan);
	openr2_chan_unlock(r2chan);
	return retcode;
}

OR2_DECLARE(int) openr2_chan_set_blocked(openr2_chan_t *r2chan)
{
	int retcode = 0;
	openr2_chan_lock(r2chan);
	retcode = openr2_proto_set_blocked(r2chan);
	openr2_chan_unlock(r2chan);
	return retcode;
}

OR2_DECLARE(openr2_cas_signal_t) openr2_chan_get_rx_cas(openr2_chan_t *r2chan)
{
	openr2_cas_signal_t cas;
	openr2_chan_lock(r2chan);
	cas = openr2_proto_get_rx_cas(r2chan);
	openr2_chan_unlock(r2chan);
	return cas;
}

OR2_DECLARE(openr2_cas_signal_t) openr2_chan_get_tx_cas(openr2_chan_t *r2chan)
{
	openr2_cas_signal_t cas;
	openr2_chan_lock(r2chan);
	cas = openr2_proto_get_tx_cas(r2chan);
	openr2_chan_unlock(r2chan);
	return cas;
}

OR2_DECLARE(void) openr2_chan_get_cas(openr2_chan_t *r2chan, openr2_cas_signal_t *rxcas, openr2_cas_signal_t *txcas)
{
	openr2_chan_lock(r2chan);
	*rxcas = openr2_proto_get_rx_cas(r2chan);
	*txcas = openr2_proto_get_tx_cas(r2chan);
	openr2_chan_unlock(r2chan);
}

OR2_DECLARE(const char *) openr2_chan_get_rx_cas_string(openr2_chan_t *r2chan)
{
	const char *retstr = 0;
	openr2_chan_lock(r2chan);
	retstr = openr2_proto_get_rx_cas_string(r2chan);
	openr2_chan_unlock(r2chan);
	return retstr;
}

OR2_DECLARE(const char *) openr2_chan_get_tx_cas_string(openr2_chan_t *r2chan)
{
	const char *retstr = 0;
	openr2_chan_lock(r2chan);
	retstr = openr2_proto_get_tx_cas_string(r2chan);
	openr2_chan_unlock(r2chan);
	return retstr;
}

OR2_DECLARE(const char *) openr2_chan_get_call_state_string(openr2_chan_t *r2chan)
{
	const char *retstr = 0;
	openr2_chan_lock(r2chan);
	retstr = openr2_proto_get_call_state_string(r2chan);
	openr2_chan_unlock(r2chan);
	return retstr;
}

OR2_DECLARE(const char *) openr2_chan_get_r2_state_string(openr2_chan_t *r2chan)
{
	const char *retstr = 0;
	openr2_chan_lock(r2chan);
	retstr = openr2_proto_get_r2_state_string(r2chan);
	openr2_chan_unlock(r2chan);
	return retstr;
}

OR2_DECLARE(const char *) openr2_chan_get_mf_state_string(openr2_chan_t *r2chan)
{
	const char *retstr = 0;
	openr2_chan_lock(r2chan);
	retstr = openr2_proto_get_mf_state_string(r2chan);
	openr2_chan_unlock(r2chan);
	return retstr;
}

OR2_DECLARE(const char *) openr2_chan_get_mf_group_string(openr2_chan_t *r2chan)
{
	const char *retstr = 0;
	openr2_chan_lock(r2chan);
	retstr = openr2_proto_get_mf_group_string(r2chan);
	openr2_chan_unlock(r2chan);
	return retstr;
}

OR2_DECLARE(int) openr2_chan_get_tx_mf_signal(openr2_chan_t *r2chan)
{
	int retcode = 0;
	openr2_chan_lock(r2chan);
	retcode = openr2_proto_get_tx_mf_signal(r2chan);
	openr2_chan_unlock(r2chan);
	return retcode;
}

OR2_DECLARE(int) openr2_chan_get_rx_mf_signal(openr2_chan_t *r2chan)
{
	int retcode = 0;
	openr2_chan_lock(r2chan);
	retcode = openr2_proto_get_rx_mf_signal(r2chan);
	openr2_chan_unlock(r2chan);
	return retcode;
}

OR2_DECLARE(int) openr2_chan_write(openr2_chan_t *r2chan, const unsigned char *buf, int buf_size)
{
	int myerrno;
	int res = 0;
	int wrote = 0;
	openr2_chan_lock(r2chan);
	while (wrote < buf_size) {
		res = openr2_io_write(r2chan, buf, buf_size);
		if (res == -1 && errno != EAGAIN) {
			myerrno = errno;
			openr2_log(r2chan, OR2_CHANNEL_LOG, OR2_LOG_ERROR, "Failed to write to channel\n");
			EMI(r2chan)->on_os_error(r2chan, myerrno);
			break;
		} else if (res == -1) {
			/* EAGAIN received, so let's try again */
			continue;
		}
		wrote += res;
	}
	openr2_chan_unlock(r2chan);
	return wrote;
}

OR2_DECLARE(int) openr2_chan_make_call(openr2_chan_t *r2chan, const char *ani, const char *dnis, 
		openr2_calling_party_category_t category, int ani_restricted)
{
	int retcode = 0;
	openr2_chan_lock(r2chan);
	retcode = openr2_proto_make_call(r2chan, ani, dnis, category, ani_restricted);
	openr2_chan_unlock(r2chan);
	return retcode;
}

OR2_DECLARE(openr2_direction_t) openr2_chan_get_direction(openr2_chan_t *r2chan)
{
	OR2_CHAN_RET_PROP(openr2_direction_t,direction);
}

OR2_DECLARE(void) openr2_chan_set_logging_func(openr2_chan_t *r2chan, openr2_chan_logging_func_t logcallback)
{
	OR2_CHAN_SET_PROP(on_channel_log,logcallback);
}

OR2_DECLARE(openr2_io_fd_t) openr2_chan_get_fd(openr2_chan_t *r2chan)
{
	OR2_CHAN_RET_PROP(openr2_io_fd_t,fd);
}

OR2_DECLARE(int) openr2_chan_get_number(openr2_chan_t *r2chan)
{
	OR2_CHAN_RET_PROP(int,number);
}

OR2_DECLARE(openr2_context_t *) openr2_chan_get_context(openr2_chan_t *r2chan)
{
	OR2_CHAN_RET_PROP(openr2_context_t*,r2context);
}

OR2_DECLARE(void) openr2_chan_set_client_data(openr2_chan_t *r2chan, void *data)
{
	OR2_CHAN_SET_PROP(client_data,data);
}

OR2_DECLARE(void *) openr2_chan_get_client_data(openr2_chan_t *r2chan)
{
	OR2_CHAN_RET_PROP(void*,client_data);
}

OR2_DECLARE(int) openr2_chan_get_time_to_next_event(openr2_chan_t *r2chan)
{
	int res, ms;
	struct timeval currtime;
	int myerrno;
	ms = -1;

	openr2_chan_lock(r2chan);	
	openr2_mutex_lock(r2chan->r2context->timers_lock);

	/* if no timers, return 'infinite' */
	if (!r2chan->timers_count) {
		ms = -1;
		goto done;
	}

	res = gettimeofday(&currtime, NULL);

	if (-1 == res) {
		myerrno = errno;
		openr2_log(r2chan, OR2_CHANNEL_LOG, OR2_LOG_ERROR, "Failed to get next event from channel. gettimeofday failed!\n");
		EMI(r2chan)->on_os_error(r2chan, myerrno);
		ms = -1;
		goto done;
	}

	ms = (((r2chan->sched_timers[0].time.tv_sec - currtime.tv_sec) * 1000) + 
	     ((r2chan->sched_timers[0].time.tv_usec - currtime.tv_usec) / 1000));

	if (ms < 0) {
		ms = 0;
	}	

done:

	openr2_mutex_unlock(r2chan->r2context->timers_lock);
	openr2_chan_unlock(r2chan);

	return ms;
}

OR2_DECLARE(openr2_log_level_t) openr2_chan_set_log_level(openr2_chan_t *r2chan, openr2_log_level_t level)
{
	openr2_log_level_t retlevel = OR2_LOG_NOTHING;
	openr2_chan_lock(r2chan);
	retlevel = r2chan->loglevel;
	r2chan->loglevel = level;
	openr2_chan_unlock(r2chan);
	return retlevel;
}

OR2_DECLARE(openr2_log_level_t) openr2_chan_get_log_level(openr2_chan_t *r2chan)
{
	OR2_CHAN_RET_PROP(openr2_log_level_t,loglevel);
}

OR2_DECLARE(void) openr2_chan_enable_read(openr2_chan_t *r2chan)
{
	OR2_CHAN_SET_PROP(read_enabled,1);
}

OR2_DECLARE(void) openr2_chan_disable_read(openr2_chan_t *r2chan)
{
	OR2_CHAN_SET_PROP(read_enabled,0);
}

OR2_DECLARE(int) openr2_chan_get_read_enabled(openr2_chan_t *r2chan)
{
	OR2_CHAN_RET_PROP(int,read_enabled);
}

OR2_DECLARE(void) openr2_chan_enable_call_files(openr2_chan_t *r2chan)
{
	OR2_CHAN_SET_PROP(call_files,1);
}

OR2_DECLARE(void) openr2_chan_disable_call_files(openr2_chan_t *r2chan)
{
	OR2_CHAN_SET_PROP(call_files,0);
}

OR2_DECLARE(int) openr2_chan_get_call_files_enabled(openr2_chan_t *r2chan)
{
	OR2_CHAN_RET_PROP(int,call_files);
}

OR2_DECLARE(const char *) openr2_chan_get_dnis(openr2_chan_t *r2chan)
{
	OR2_CHAN_RET_PROP(const char *,dnis);
}

OR2_DECLARE(const char *) openr2_chan_get_ani(openr2_chan_t *r2chan)
{
	OR2_CHAN_RET_PROP(const char *,ani);
}

