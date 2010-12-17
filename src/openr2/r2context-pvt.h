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

#ifndef _OPENR2_CONTEXT_PVT_H_
#define _OPENR2_CONTEXT_PVT_H_

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif
#include <stdarg.h>
#include "r2thread.h"
#include "r2log.h"
#include "r2proto-pvt.h"

#if defined(__cplusplus)
extern "C" {
#endif

/* we dont include openr2_chan_t because r2chan.h 
   already include us */
struct openr2_chan_s;

/* R2 protocol timers */
typedef struct {
	/* Max amount of time our backward MF signal can last */
	int mf_back_cycle;
	/* Amount of time we set a MF signal ON to resume the MF cycle 
	   with a MF pulse */
	int mf_back_resume_cycle;
	/* Safety FORWARD timer */
	int mf_fwd_safety;
	/* How much time do we wait for a response to our seize signal */
	int r2_seize;
	/* How much to wait for an answer once the call has been accepted */
	int r2_answer;
	/* How much to wait for metering pulse detection */
	int r2_metering_pulse;
	/* Interval between ANSWER - CLEAR BACK - ANSWER when double answer is in effect */
	int r2_double_answer;
	/* Minimum delay time between the Accept tone signal and the R2 answer signal */
	int r2_answer_delay;
	/* time to wait for CAS signaling before handling the new signal */
	int cas_persistence_check;
	/* safety timer before starting to dial DTMF */
	int dtmf_start_dial;
	/* Put the channel back to IDLE state after sending a CLEAR FORWARD */
	int r2_set_call_down;
} openr2_timers_t;


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

	/* this interface provide I/O
	   functions to the R2 channels */
	openr2_io_interface_t *io;

	/* Type of I/O interface */
	openr2_io_type_t io_type;

	/* this interface provides DTMF functions
	   to the R2 channels */
	openr2_dtmf_interface_t *dtmfeng;

	/* R2 variant to use in this context channels */
	openr2_variant_t variant;

	/* CAS signals configured for the variant in use */
	openr2_cas_signal_t cas_signals[OR2_NUM_CAS_SIGNALS];

	/* C and D bit are not required for R2 and set to 01, 
	   thus, not used for the R2 signaling */
	openr2_cas_signal_t cas_nonr2_bits;

	/* Mask to easily get the AB bits which are used for 
	   R2 signaling */
	openr2_cas_signal_t cas_r2_bits;

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

	/* Skip category request and go to GII/B signals right away */
	int skip_category;

	/* whether or not accept the call bypassing the use of group B and II tones */
	int immediate_accept;

	/* use double answer with all channels */
	int double_answer;

	/* MF threshold time in ms */
	int mf_threshold;

	/* use DTMF for outbound dialing */
	int dial_with_dtmf;

	/* use DTMF for inbound */
	int detect_dtmf;
	
	/* How much time a DTMF digit should be ON */
	int dtmf_on;

	/* How much time the dtmf engine should be OFF between digits */
	int dtmf_off;

	/* R2 logging mask */
	openr2_log_level_t loglevel;

	/* R2 logging directory */
	char logdir[OR2_MAX_PATH];

	/* whether or not the advanced configuration file was used */
	int configured_from_file;

	/* access token to the timers */
	openr2_mutex_t *timers_lock;

	/* list of channels that belong to this context */
	struct openr2_chan_s *chanlist;

} openr2_context_t;


void openr2_context_add_channel(openr2_context_t *r2context, struct openr2_chan_s *r2chan);
void openr2_context_remove_channel(openr2_context_t *r2context, struct openr2_chan_s *r2chan);
#include "r2context.h"

#if defined(__cplusplus)
} /* endif extern "C" */
#endif

#endif /* endif defined _OPENR2_CONTEXT_PVT_H_ */

