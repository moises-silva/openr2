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

#ifndef _OPENR2_PROTO_H_
#define _OPENR2_PROTO_H_

#if defined(__cplusplus)
extern "C" {
#endif

struct openr2_chan_s;
struct openr2_context_s;

#define OR2_MAX_ANI 80
#define OR2_MAX_DNIS 80

/* Number of ABCD signals. ABCD signaling is
   known as line supervisory signaling  */
#define OR2_NUM_ABCD_SIGNALS 8
typedef enum {
	/* Invalid signal */
	OR2_ABCD_INVALID = -1,
	/* The line is ready to receive or make calls */
	OR2_ABCD_IDLE = 0,
	/* The line is not ready to receive or make calls */
	OR2_ABCD_BLOCK,
	/* We set to this state our ABCD bits when we want to let know
	   the other side that we want to start a new call */
	OR2_ABCD_SEIZE,
	/* We answer with this ABCD bits when we are sized and we want to proceed
	   with the MF signaling. After setting the bits to this state, the other end
	   will start sending MF signaling */
	OR2_ABCD_SEIZE_ACK,
	/* We want to HANGUP the call */
	OR2_ABCD_CLEAR_BACK,
	/* Hangup immediately. I have only seen this in Brazil, where the central
	   waits several seconds (30?) after Clear Back to release the line, with
	   this signal no waiting will be done. When receiving this signal openr2 
	   behaves the same as with clear back, no waiting is done with clear back
	   anyway, should we? */
	OR2_ABCD_FORCED_RELEASE,
	/* They want to HANGUP the call */
	OR2_ABCD_CLEAR_FORWARD,
	/* We set this to let know the other end we are ANSWERing the call and the
	   speech path is open */
	OR2_ABCD_ANSWER
} openr2_abcd_signal_t;

/* 
   This are known as Multi Frequency signals ( MF signals). the same 15 inter-register signals 
   are used for the distinct groups with distinct meanings for each group.
 
   Group A, B and C are the Backward groups, that is, the possible groups where the state machine
   of the callee can be at.

   Groups I, II and III are the Forward groups, that is, the possible groups where the state machine
   of the caller can be at.

   During Group A or C, the callee will receive DNIS or ANI digits ( depending on the R2 variant )
   and will request more digits. The caller end will be at Group I or III sending the requested digits.

   During Group B the callee either accept or reject the call. The caller will be in Group II
   waiting for the caller decision.

   Group C is a special case for Mexico, there is where ANI is requested. The caller
   will be in Group III during this phase.
 */

/* MF signals. The MF interface must understand this
   code points to match them to the proper pair of
   frequencies. This code points were chosen to match
   the SpanDSP Library. Usage of Zaptel MF requires to
   match SpandSP and Zaptel identifiers */
typedef enum {
	OR2_MF_TONE_INVALID = 0,
	OR2_MF_TONE_1 = '1',
	OR2_MF_TONE_2 = '2',
	OR2_MF_TONE_3 = '3',
	OR2_MF_TONE_4 = '4',
	OR2_MF_TONE_5 = '5',
	OR2_MF_TONE_6 = '6',
	OR2_MF_TONE_7 = '7',
	OR2_MF_TONE_8 = '8',
	OR2_MF_TONE_9 = '9',
	OR2_MF_TONE_10 = '0',
	OR2_MF_TONE_11 = 'B',
	OR2_MF_TONE_12 = 'C',
	OR2_MF_TONE_13 = 'D',
	OR2_MF_TONE_14 = 'E',
	OR2_MF_TONE_15 = 'F'
} openr2_mf_tone_t;

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
	OR2_MF_GIII
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
	OR2_MF_WAITING_TIMEOUT = 305
} openr2_mf_state_t;

/* R2 state machine */
typedef enum {
	/* just some invalid state */
	OR2_INVALID_STATE = -1,

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

	/* Blocked line */
	OR2_BLOCKED = 400
} openr2_abcd_state_t;

/* MFC/R2 variants */
typedef enum {
	OR2_VAR_ARGENTINA = 0,
	OR2_VAR_BRAZIL = 20,
	OR2_VAR_CHINA = 40,
	OR2_VAR_CZECH = 60,
	OR2_VAR_COLOMBIA = 70,
	OR2_VAR_ECUADOR = 80,
	OR2_VAR_ITU = 100,
	OR2_VAR_MEXICO = 120,
	OR2_VAR_PHILIPPINES = 140,
	OR2_VAR_VENEZUELA = 160,
	OR2_VAR_UNKNOWN = 999
} openr2_variant_t;

/* at any given time we either are stopped (idle or blocked), forward or backward */
typedef enum {
	OR2_DIR_STOPPED,
	OR2_DIR_FORWARD,
	OR2_DIR_BACKWARD
} openr2_direction_t;

/* Call States */
typedef enum {
	/* ready to accept or make calls */
	OR2_CALL_IDLE, 
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

/* Call modes */
typedef enum {
	OR2_CALL_WITH_CHARGE,
	OR2_CALL_NO_CHARGE,
	OR2_CALL_SPECIAL,
	OR2_CALL_UNKNOWN
} openr2_call_mode_t;

/* Disconnect causes */
typedef enum {
	OR2_CAUSE_BUSY_NUMBER,
	OR2_CAUSE_NETWORK_CONGESTION,
	OR2_CAUSE_UNALLOCATED_NUMBER,
	OR2_CAUSE_OUT_OF_ORDER,
	OR2_CAUSE_UNSPECIFIED,
	OR2_CAUSE_NO_ANSWER,
	OR2_CAUSE_NORMAL_CLEARING,
	OR2_CAUSE_COLLECT_CALL_REJECTED,
	OR2_CAUSE_FORCED_RELEASE
} openr2_call_disconnect_cause_t;

/* possible causes of protocol error */
typedef enum {
	OR2_INVALID_CAS_BITS,
	OR2_INVALID_MF_TONE,
	OR2_BACK_MF_TIMEOUT,
	OR2_SEIZE_TIMEOUT,
	OR2_FWD_SAFETY_TIMEOUT,
	OR2_BROKEN_MF_SEQUENCE,
	OR2_ANSWER_TIMEOUT,
	OR2_INVALID_R2_STATE,
	OR2_INVALID_MF_STATE,
	OR2_INVALID_MF_GROUP,
	OR2_LIBRARY_BUG,
	OR2_INTERNAL_ERROR
} openr2_protocol_error_t;

/* names for the GA MF tones */
typedef struct {
	openr2_mf_tone_t request_next_dnis_digit;
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
	openr2_mf_tone_t reject_collect_call;
} openr2_mf_gb_tones_t;

/* names for the GC MF tones */
typedef struct {
	openr2_mf_tone_t request_next_ani_digit;
	openr2_mf_tone_t request_change_to_g2;
	openr2_mf_tone_t request_next_dnis_digit_and_change_to_ga;
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
} openr2_mf_g2_tones_t;

typedef enum {
	OR2_CALLING_PARTY_CATEGORY_NATIONAL_SUBSCRIBER,
	OR2_CALLING_PARTY_CATEGORY_NATIONAL_PRIORITY_SUBSCRIBER,
	OR2_CALLING_PARTY_CATEGORY_INTERNATIONAL_SUBSCRIBER,
	OR2_CALLING_PARTY_CATEGORY_INTERNATIONAL_PRIORITY_SUBSCRIBER,
	OR2_CALLING_PARTY_CATEGORY_COLLECT_CALL,
	OR2_CALLING_PARTY_CATEGORY_UNKNOWN
} openr2_calling_party_category_t;

int openr2_proto_make_call(struct openr2_chan_s *r2chan, const char *ani, const char *dnis, openr2_calling_party_category_t category);
int openr2_proto_accept_call(struct openr2_chan_s *r2chan, openr2_call_mode_t accept);
int openr2_proto_answer_call(struct openr2_chan_s *r2chan);
int openr2_proto_disconnect_call(struct openr2_chan_s *r2chan, openr2_call_disconnect_cause_t cause);
int openr2_proto_handle_abcd_change(struct openr2_chan_s *r2chan);
int openr2_proto_set_idle(struct openr2_chan_s *r2chan);
int openr2_proto_set_blocked(struct openr2_chan_s *r2chan);
int openr2_proto_set_abcd_signal(struct openr2_chan_s *r2chan, openr2_abcd_signal_t signal);
int openr2_proto_configure_context(struct openr2_context_s *r2context, openr2_variant_t variant, int max_ani, int max_dnis);
const char *openr2_proto_get_error(openr2_protocol_error_t reason);
const char *openr2_proto_get_disconnect_string(openr2_call_disconnect_cause_t cause);
void openr2_proto_handle_mf_tone(struct openr2_chan_s *r2chan, int tone);

const char *openr2_proto_get_category_string(openr2_calling_party_category_t category);
openr2_calling_party_category_t openr2_proto_get_category(const char *category);

const char *openr2_proto_get_variant_string(openr2_variant_t variant);
openr2_variant_t openr2_proto_get_variant(const char *variant);
const char *openr2_proto_get_call_mode_string(openr2_call_mode_t mode);

const char *openr2_proto_get_rx_state_string(struct openr2_chan_s *r2chan);
const char *openr2_proto_get_tx_state_string(struct openr2_chan_s *r2chan);
const char *openr2_proto_get_call_state_string(struct openr2_chan_s *r2chan);
const char *openr2_proto_get_r2_state_string(struct openr2_chan_s *r2chan);
const char *openr2_proto_get_mf_state_string(struct openr2_chan_s *r2chan);
const char *openr2_proto_get_mf_group_string(struct openr2_chan_s *r2chan);
int openr2_proto_get_mf_tx(struct openr2_chan_s *r2chan);
int openr2_proto_get_mf_rx(struct openr2_chan_s *r2chan);


#if defined(__cplusplus)
} /* endif extern "C" */
#endif

#endif /* endif defined _OPENR2_PROTO_H_ */
