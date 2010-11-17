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

#include <stdarg.h>
#include "r2declare.h"
#include "r2proto.h"
#include "r2log.h"
#include "r2context.h"

#if defined(__cplusplus)
extern "C" {
#endif

#include "r2exports.h"

/* TODO: this should be set through an API, openr2_chan_set_period or openr2_chan_set_read_size */
/*! \brief How many bytes to read each time at once from the channel */
#define OR2_CHAN_READ_SIZE 160

/* callback for logging channel related info */
typedef void (*openr2_chan_logging_func_t)(openr2_chan_t *r2chan, const char *file, const char *function, unsigned int line, openr2_log_level_t level, const char *fmt, va_list ap);

/*! \brief allocate and initialize a new channel openning the underlying hardware channel number */
OR2_DECLARE(openr2_chan_t *) openr2_chan_new(openr2_context_t *r2context, int channo);

/*! \brief allocate and initialize a new channel and associates the given I/O descriptor to it */
OR2_DECLARE(openr2_chan_t *) openr2_chan_new_from_fd(openr2_context_t *r2context, openr2_io_fd_t chanfd, int channo);

/*! \brief destroys the memory allocated when creating the channel
 * no need to call this function if the channel is already associated to a context, 
 * the context destruction will delete the channels associated to it 
 */
OR2_DECLARE(void) openr2_chan_delete(openr2_chan_t *r2chan);

/*! \brief Accepts an offered call on the given channel */
OR2_DECLARE(int) openr2_chan_accept_call(openr2_chan_t *r2chan, openr2_call_mode_t accept);

/*! \brief Answers the call on the channel, the call must have been already accepted */
OR2_DECLARE(int) openr2_chan_answer_call(openr2_chan_t *r2chan);

/*! \brief Answers the call on the channel with the given mode, the call must have been already accepted */
OR2_DECLARE(int) openr2_chan_answer_call_with_mode(openr2_chan_t *r2chan, openr2_answer_mode_t mode);

/*! \brief Disconnects the call or acnowledges a disconnect on the channel with the given reason
     the reason is ignored if its an acknowledge of hangup */
OR2_DECLARE(int) openr2_chan_disconnect_call(openr2_chan_t *r2chan, openr2_call_disconnect_cause_t cause);

/*! \brief Makes a call with the given ani, dnid and category */
OR2_DECLARE(int) openr2_chan_make_call(openr2_chan_t *r2chan, const char *ani, const char *dnid, openr2_calling_party_category_t category);

/*! \brief Return the direction of the call in the given channel */
OR2_DECLARE(openr2_direction_t) openr2_chan_get_direction(openr2_chan_t *r2chan);

/*! \brief writes the given buffer to the channel using the underlying I/O callbacks or default I/O implementation */
OR2_DECLARE(int) openr2_chan_write(openr2_chan_t *r2chan, const unsigned char *buf, int len);

/*! \brief Set the callback to call when logging */
OR2_DECLARE(void) openr2_chan_set_logging_func(openr2_chan_t *r2chan, openr2_chan_logging_func_t logcallback);

/*! \brief Return the I/O descriptor associated to the channel */
OR2_DECLARE(openr2_io_fd_t) openr2_chan_get_fd(openr2_chan_t *r2chan);

/*! \brief Get the channel number */
OR2_DECLARE(int) openr2_chan_get_number(openr2_chan_t *r2chan);

/*! \brief Get the context associated to the channel */
OR2_DECLARE(openr2_context_t *) openr2_chan_get_context(openr2_chan_t *r2chan);

/*! \brief Associate an opaque pointer to the channel */
OR2_DECLARE(void) openr2_chan_set_client_data(openr2_chan_t *r2chan, void *data);

/*! \brief Return the opaque pointer associated to the channel */
OR2_DECLARE(void *) openr2_chan_get_client_data(openr2_chan_t *r2chan);

/*! \brief Return the number of milliseconds left for the next scheduled event in the channel */
OR2_DECLARE(int) openr2_chan_get_time_to_next_event(openr2_chan_t *r2chan);

/*! \brief Set the logging level for the channel */
OR2_DECLARE(openr2_log_level_t) openr2_chan_set_log_level(openr2_chan_t *r2chan, openr2_log_level_t level);

/*! \brief Return the logging level for the channel */
OR2_DECLARE(openr2_log_level_t) openr2_chan_get_log_level(openr2_chan_t *r2chan);

/*! \brief Enable the media reading for the channel, the stack will call the I/O read operation when needed */
OR2_DECLARE(void) openr2_chan_enable_read(openr2_chan_t *r2chan);

/*! \brief Disable the media reading for the channel, the stack will NOT call the I/O read operation when needed */
OR2_DECLARE(void) openr2_chan_disable_read(openr2_chan_t *r2chan);

/*! \brief Return non-zero if the reading of media is enabled for the channel */
OR2_DECLARE(int) openr2_chan_get_read_enabled(openr2_chan_t *r2chan);

/*! \brief enable the call debugging files for this channel */
OR2_DECLARE(void) openr2_chan_enable_call_files(openr2_chan_t *r2chan);

/*! \brief disable the call debugging files for this channel */
OR2_DECLARE(void) openr2_chan_disable_call_files(openr2_chan_t *r2chan);

/*! \brief return non-zero if the call debugging files are enabled for this channel */
OR2_DECLARE(int) openr2_chan_get_call_files_enabled(openr2_chan_t *r2chan);

/*! \brief get the DNIS in the channel */
OR2_DECLARE(const char *) openr2_chan_get_dnis(openr2_chan_t *r2chan);

/*! \brief get the ANI in the channel */
OR2_DECLARE(const char *) openr2_chan_get_ani(openr2_chan_t *r2chan);

/*! \brief set the channel CAS in the idle state */
OR2_DECLARE(int) openr2_chan_set_idle(openr2_chan_t *r2chan);

/*! \brief set the channel CAS in the blocked state */
OR2_DECLARE(int) openr2_chan_set_blocked(openr2_chan_t *r2chan);

/*! \brief check for out of band events and process them if any change occured (CAS signaling and hardware Alarms) */
OR2_DECLARE(int) openr2_chan_process_oob_events(openr2_chan_t *r2chan);

/*! \brief check for CAS signaling changes and process them if any change occured */
OR2_DECLARE(int) openr2_chan_process_cas_signaling(openr2_chan_t *r2chan);

/*! \brief check for MF signaling changes and process them if any change occured */
OR2_DECLARE(int) openr2_chan_process_mf_signaling(openr2_chan_t *r2chan);

/*! \brief check for any signaling change and process them if any change occured */
OR2_DECLARE(int) openr2_chan_process_signaling(openr2_chan_t *r2chan);

/*! \brief check if there is any expired timer and execute the timeout callbacks if needed */
OR2_DECLARE(int) openr2_chan_run_schedule(openr2_chan_t *r2chan);

/*! \brief return a meaningful string for the last CAS bits received */
OR2_DECLARE(const char *) openr2_chan_get_rx_cas_string(openr2_chan_t *r2chan);

/*! \brief return a meaningful string for the last CAS bits transmitted */
OR2_DECLARE(const char *) openr2_chan_get_tx_cas_string(openr2_chan_t *r2chan);

/*! \brief return a meaningful string for the current call state */
OR2_DECLARE(const char *) openr2_chan_get_call_state_string(openr2_chan_t *r2chan);

/*! \brief return a meaningful string for the current call state */
OR2_DECLARE(const char *) openr2_chan_get_r2_state_string(openr2_chan_t *r2chan);

/*! \brief return a meaningful string for the current MF state */
OR2_DECLARE(const char *) openr2_chan_get_mf_state_string(openr2_chan_t *r2chan);

/*! \brief return a meaningful string for the current MF group */
OR2_DECLARE(const char *) openr2_chan_get_mf_group_string(openr2_chan_t *r2chan);

/*! \brief return the ASCII code for the last transmitted MF signal */
OR2_DECLARE(int) openr2_chan_get_tx_mf_signal(openr2_chan_t *r2chan);

/*! \brief return the ASCII code for the last received MF signal */
OR2_DECLARE(int) openr2_chan_get_rx_mf_signal(openr2_chan_t *r2chan);

/*! \brief set the opaque handle that will be passed back to the DTMF callbacks */
OR2_DECLARE(int) openr2_chan_set_dtmf_handles(openr2_chan_t *r2chan, void *dtmf_read_handle, void *dtmf_write_handle);

/*! \brief set the opaque handles that will be passed back to the MF generation and detection callbacks */
OR2_DECLARE(int) openr2_chan_set_mflib_handles(openr2_chan_t *r2chan, void *mf_write_handle, void *mf_read_handle);

#ifdef __OR2_COMPILING_LIBRARY__
#undef openr2_chan_t
#undef openr2_context_t
#endif
#if defined(__cplusplus)
} /* endif extern "C" */
#endif

#endif /* endif defined _OPENR2_CHAN_H_ */

