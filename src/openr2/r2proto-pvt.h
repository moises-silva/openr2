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

#ifndef _OPENR2_PROTO_PVT_H_
#define _OPENR2_PROTO_PVT_H_

/* include public definitions */
#include "r2proto.h"

#if defined(__cplusplus)
extern "C" {
#endif

/* MFC-R2 protocol explanation */
/* TODO */

/* DTMF R2 protocol explanation */
/*
 * Call Setup
 *
 * DTMF R2 is used AFAIK in Venezuela for outgoing dialing. However the described protocol behaviour seen here
 * does not match the seen in Venezuela. I added initial DTMF R2 for Venezuela for incoming calls but later I got
 * rid of it because it never worked reliably. Later a company from Chile (e-contact.cl) was interested in DTMF R2
 * but the lab PBX they setup with DTMF R2 exhibited a simpler behavior, that is the one described here.
 *
 * This is how it works:
 *
 * 1. We send Seize and put a timer to abort the call in the case
 * where we don't receive any kind of response from the other side (seize ack timer).
 *
 * 2. When receiving seize ack we dial the destiny number with DTMF tones with a default of 50ms tone ON and
 * 100ms of tone OFF. There is no ANI transmission in DTMF R2 mode.
 *
 * 3. After dialing all the digits we expect for answer or cancel the call if answer timeout expires.
 *
 *
 * OpenR2 Side 					Telco Side
 *
 * ---------------> R2 Seize ---------------------------->
 *
 * <--------------- R2 Seize Ack <------------------------
 * ++++++++++++++++ Small dial delay (around 500ms) ++++++
 *
 * ===============> DTMF DNIS Digit 1 ===================>
 * ===============> DTMF DNIS Digit 2 ===================>
 * ===============> DTMF DNIS Digit ... =================>
 * ===============> DTMF DNIS Digit N ===================>
 *
 * ++++++++++++++++ Waiting for Answer ++++++++++++++++++
 *
 * <--------------- R2 Answer DTMF (0x1) <---------------
 * 
 *
 * Call Teardown
 *
 * When clearing up the call the behavior is the same for both the backward or forward side, either side of the call wanting to
 * hangup just send CLEAR FORWARD and expect for the other side to become IDLE
 *
 * OpenR2 Side 					Telco Side
 *
 * ---------------> R2 Clear Forward --------------------->
 * <--------------> R2 Idle <------------------------------
 * ---------------> R2 Idle ------------------------------>
 *
 *
 */

struct openr2_chan_s;
struct openr2_context_s;

/* MF groups */
typedef enum {
	/* were not doing anything yet */
	OR2_MF_NO_GROUP,

	/* possible backward groups we are at */
	OR2_MF_BACK_INIT,
	OR2_MF_GA,
	OR2_MF_GB,
	OR2_MF_GC,

	/* possible forward groups we are at */
	OR2_MF_FWD_INIT,
	OR2_MF_GI,
	OR2_MF_GII,
	OR2_MF_GIII,

	/* possible DTMF groups */
	OR2_MF_DTMF_FWD_INIT,
	OR2_MF_DTMF_BACK_INIT
} openr2_mf_group_t;

/* possible backward MF states */
typedef enum {
	/* the MF engine is not turned on */
	OR2_MF_OFF_STATE = 100,

	/**** possible backward MF states ****/

	/* seize ACK has been sent, MF tones can start */
	OR2_MF_SEIZE_ACK_TXD = 200,

	/* We have requested the calling party category and are waiting for it */
	OR2_MF_CATEGORY_RQ_TXD = 201,

	/* we have sent next DNIS digit request. We're waiting for a DNIS digit or end of DNIS signal */
	OR2_MF_DNIS_RQ_TXD = 202,

	/* we have sent next ANI digit request. We're waiting either for ANI digit or end of ANI signal */
	OR2_MF_ANI_RQ_TXD = 203,

	/* We have sent change to group II signal. ANI and DNIS has been already transmited, We are expecting 
	  confirmation to finally decide if we accept or reject the call */
	OR2_MF_CHG_GII_TXD = 204,

	/* call has been accepted */
	OR2_MF_ACCEPTED_TXD = 205,

	/* We have notified that we are not accepting the call */
	OR2_MF_DISCONNECT_TXD = 206,

	/**** possible forward MF states ****/

	/* we received category request and have answered given the category to the other end */
	OR2_MF_CATEGORY_TXD = 300,

	/* we received some DNIS request and have already sent a DNIS digit */
	OR2_MF_DNIS_TXD = 301,

	/* we received some DNIS request and have already sent 'end of DNIS' signal */
	OR2_MF_DNIS_END_TXD = 302,

	/* we received some ANI request and have already sent an ANI digit */
	OR2_MF_ANI_TXD = 303,

	/* we received some ANI request and have already sent 'end of ANI' signal */
	OR2_MF_ANI_END_TXD = 304,

	/* we did not sent a tone, we are waiting for the other side to timeout
	   expecting our tone */
	OR2_MF_WAITING_TIMEOUT = 305,

	/*** DTMF R2 Related ***/

	/* Backward DTMF states */

	/* Backward still not supported :-( */

	/* Forward DTMF states */

	/* We're dialing DTMF */
	OR2_MF_DIALING_DTMF = 500,

	/* We're detecting DTMF */
	OR2_MF_DETECTING_DTMF = 501
} openr2_mf_state_t;

/* R2 state machine */
typedef enum {
	/* just some invalid state */
	OR2_INVALID_STATE = -1,

	/* channel created but no particular state yet */
	OR2_INIT = 0,

	/* we are waiting to start or receive a call */
	OR2_IDLE = 100,

	/** BACKWARD STATES **/

	/* we have been sized and have answered with a Size ACK */
	OR2_SEIZE_ACK_TXD = 200,

	/* we just decided to answer the call */
	OR2_ANSWER_TXD = 201,

	/* We just requested to hangup the call */
	OR2_CLEAR_BACK_TXD = 202,

	/* The Forward side requested to hangup the call.
	   This could be in response to a disconnection tone
	   we sent. */
	OR2_CLEAR_FWD_RXD = 203,

	/* We answered but then sent clear back as part of the 
	   block-collect-calls process and we need to resume
	   the answer state  */
	OR2_EXECUTING_DOUBLE_ANSWER = 204,

	/* We just requested to hangup the call immediately */
	OR2_FORCED_RELEASE_TXD = 205,

	/** FORWARD STATES **/
	OR2_SEIZE_TXD = 300,

	/* The callee has sent the seize ack */
	OR2_SEIZE_ACK_RXD = 301,

	/* The callee did no accepted the call */
	OR2_CLEAR_BACK_TONE_RXD = 302,

	/* The calle has accepted the call */
	OR2_ACCEPT_RXD = 303,

	/* The callee has answered the call */
	OR2_ANSWER_RXD = 304,

	/* callee is asking us to end the call */
	OR2_CLEAR_BACK_RXD = 305,

	/* The callee has answered the call but we
	   still dont get the final MF tone off */
	OR2_ANSWER_RXD_MF_PENDING = 306,

	/* Asked to hangup the call */
	OR2_CLEAR_FWD_TXD = 307,

	/* callee is asking us to end the call immediately */
	OR2_FORCED_RELEASE_RXD = 308,

	/* we sent clear forward and then got clear back */
	OR2_CLEAR_BACK_AFTER_CLEAR_FWD_RXD = 309,

	/* We sent seize, but before getting seize ack a clear forward was attempted */
	OR2_SEIZE_TXD_CLEAR_FWD_PENDING = 310,

	/* We detected glare and we're waiting for the persist seize to expire or receive clear forward */
	OR2_DOUBLE_SEIZURE_CLEAR_FWD_PENDING = 311,

	/* Blocked line */
	OR2_BLOCKED = 400,

	/* We seized the line just before they seized it too! */
	OR2_DOUBLE_SEIZURE = 500,
} openr2_cas_state_t;

/* Call States */
typedef enum {
	/* ready to accept or make calls */
	OR2_CALL_IDLE, 
	/* We started collecting information for an incoming call */
	OR2_CALL_COLLECTING,
	/* dialing in progress */
	OR2_CALL_DIALING, 
	/* ANI, DNIS and Category transmission done, accept it or reject it */
	OR2_CALL_OFFERED, 
	/* Call has been accepted ( not answered ) */
	OR2_CALL_ACCEPTED,
	/* Call has been answered and the speech path is ready */
	OR2_CALL_ANSWERED,
	/* Call has been disconnected */
	OR2_CALL_DISCONNECTED
} openr2_call_state_t;

/* names for the GA MF tones */
typedef struct {
	openr2_mf_tone_t request_next_dnis_digit;
	openr2_mf_tone_t request_dnis_minus_1;
	openr2_mf_tone_t request_dnis_minus_2;
	openr2_mf_tone_t request_dnis_minus_3;
	openr2_mf_tone_t request_all_dnis_again;
	openr2_mf_tone_t request_next_ani_digit;
	openr2_mf_tone_t request_category;
	openr2_mf_tone_t request_category_and_change_to_gc;
	openr2_mf_tone_t request_change_to_g2;
	openr2_mf_tone_t address_complete_charge_setup;
	openr2_mf_tone_t network_congestion;
} openr2_mf_ga_tones_t;

/* names for the GB MF tones */
typedef struct {
	openr2_mf_tone_t accept_call_with_charge;
	openr2_mf_tone_t accept_call_no_charge;
	openr2_mf_tone_t busy_number;
	openr2_mf_tone_t network_congestion;
	openr2_mf_tone_t unallocated_number;
	openr2_mf_tone_t line_out_of_order;
	openr2_mf_tone_t special_info_tone;
	openr2_mf_tone_t number_changed;
} openr2_mf_gb_tones_t;

/* names for the GC MF tones */
typedef struct {
	openr2_mf_tone_t request_next_ani_digit;
	openr2_mf_tone_t request_change_to_g2;
	openr2_mf_tone_t request_next_dnis_digit_and_change_to_ga;
	openr2_mf_tone_t network_congestion;
} openr2_mf_gc_tones_t;

/* names for the GI MF tones */
typedef struct {
	openr2_mf_tone_t no_more_dnis_available;
	openr2_mf_tone_t no_more_ani_available;
	openr2_mf_tone_t caller_ani_is_restricted;
} openr2_mf_g1_tones_t;

typedef struct {
	openr2_mf_tone_t national_subscriber;
	openr2_mf_tone_t national_priority_subscriber;
	openr2_mf_tone_t international_subscriber;
	openr2_mf_tone_t international_priority_subscriber;
	openr2_mf_tone_t collect_call;
	openr2_mf_tone_t test_equipment;
	openr2_mf_tone_t pay_phone;
} openr2_mf_g2_tones_t;

const char *openr2_proto_get_rx_cas_string(struct openr2_chan_s *r2chan);
const char *openr2_proto_get_tx_cas_string(struct openr2_chan_s *r2chan);
openr2_cas_signal_t openr2_proto_get_rx_cas(struct openr2_chan_s *r2chan);
openr2_cas_signal_t openr2_proto_get_tx_cas(struct openr2_chan_s *r2chan);
const char *openr2_proto_get_call_state_string(struct openr2_chan_s *r2chan);
const char *openr2_proto_get_r2_state_string(struct openr2_chan_s *r2chan);
const char *openr2_proto_get_mf_state_string(struct openr2_chan_s *r2chan);
const char *openr2_proto_get_mf_group_string(struct openr2_chan_s *r2chan);
int openr2_proto_get_tx_mf_signal(struct openr2_chan_s *r2chan);
int openr2_proto_get_rx_mf_signal(struct openr2_chan_s *r2chan);
int openr2_proto_make_call(struct openr2_chan_s *r2chan, const char *ani, 
		const char *dnis, openr2_calling_party_category_t category, int ani_restricted);
int openr2_proto_accept_call(struct openr2_chan_s *r2chan, openr2_call_mode_t accept);
int openr2_proto_answer_call(struct openr2_chan_s *r2chan);
int openr2_proto_answer_call_with_mode(struct openr2_chan_s *r2chan, openr2_answer_mode_t mode);
int openr2_proto_disconnect_call(struct openr2_chan_s *r2chan, openr2_call_disconnect_cause_t cause);
int openr2_proto_handle_cas(struct openr2_chan_s *r2chan);
int openr2_proto_set_idle(struct openr2_chan_s *r2chan);
int openr2_proto_set_blocked(struct openr2_chan_s *r2chan);
int openr2_proto_set_cas_signal(struct openr2_chan_s *r2chan, openr2_cas_signal_t signal);
int openr2_proto_configure_context(struct openr2_context_s *r2context, openr2_variant_t variant, int max_ani, int max_dnis);
void openr2_proto_handle_mf_tone(struct openr2_chan_s *r2chan, int tone);
void openr2_proto_handle_dtmf_end(struct openr2_chan_s *r2chan);
int openr2_proto_handle_alarm_state(struct openr2_chan_s *r2chan);
void openr2_proto_destroy(struct openr2_chan_s *r2chan);

#if defined(__cplusplus)
} /* endif extern "C" */
#endif

#endif /* endif defined _OPENR2_PROTO_PVT_H_ */
