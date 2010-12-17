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

#include "r2declare.h"
#include "r2exports.h"

#define OR2_MAX_ANI 80
#define OR2_MAX_DNIS 80
#define OR2_DEFAULT_DTMF_ON 50
#define OR2_DEFAULT_DTMF_OFF 100

#define OR2_STOP_DNIS_REQUEST 0
#define OR2_CONTINUE_DNIS_REQUEST 1

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

/* Variant configuration function and variant table entry */
typedef void (*openr2_variant_config_func)(openr2_context_t *);
typedef struct {
	openr2_variant_t id;
	const char *name;
	const char *country;
	openr2_variant_config_func config;
} openr2_variant_entry_t;

/* at any given time we either are stopped (idle or blocked), forward or backward */
typedef enum {
	OR2_DIR_STOPPED,
	OR2_DIR_FORWARD,
	OR2_DIR_BACKWARD
} openr2_direction_t;

/* Call modes */
typedef enum {
	OR2_CALL_WITH_CHARGE,
	OR2_CALL_NO_CHARGE,
	OR2_CALL_SPECIAL,
	OR2_CALL_UNKNOWN
} openr2_call_mode_t;

/* Answer modes */
typedef enum {
	OR2_ANSWER_SIMPLE,
	OR2_ANSWER_DOUBLE
} openr2_answer_mode_t;

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
	OR2_CAUSE_FORCED_RELEASE,
	OR2_CAUSE_NUMBER_CHANGED
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

/* possible calling party categories */
typedef enum {
	OR2_CALLING_PARTY_CATEGORY_NATIONAL_SUBSCRIBER = 0,
	OR2_CALLING_PARTY_CATEGORY_NATIONAL_PRIORITY_SUBSCRIBER,
	OR2_CALLING_PARTY_CATEGORY_INTERNATIONAL_SUBSCRIBER,
	OR2_CALLING_PARTY_CATEGORY_INTERNATIONAL_PRIORITY_SUBSCRIBER,
	OR2_CALLING_PARTY_CATEGORY_COLLECT_CALL,
	OR2_CALLING_PARTY_CATEGORY_TEST_EQUIPMENT,
	OR2_CALLING_PARTY_CATEGORY_PAY_PHONE,
	OR2_CALLING_PARTY_CATEGORY_UNKNOWN, 
} openr2_calling_party_category_t;

/* Number of CAS signals. CAS signaling is
   known as line supervisory signaling  

   WATCH OUT!!
   This number MUST match the number of 
   enums AND r2proto.c standard_cas_signals
   and cas_names arrays!!
*/
#define OR2_NUM_CAS_SIGNALS 8

/*
   WATCH OUT!!
   Each value of this enum should correspond to the
   proper index in standard_cas_signals and cas_names arrays!!
   */
typedef enum {
	/* Invalid signal */
	OR2_CAS_INVALID = -1,
	/* The line is ready to receive or make calls */
	OR2_CAS_IDLE = 0,
	/* The line is not ready to receive or make calls */
	OR2_CAS_BLOCK,
	/* We set to this state our CAS bits when we want to let know
	   the other side that we want to start a new call */
	OR2_CAS_SEIZE,
	/* We answer with this CAS bits when we are sized and we want to proceed
	   with the MF signaling. After setting the bits to this state, the other end
	   will start sending MF signaling */
	OR2_CAS_SEIZE_ACK,
	/* We want to HANGUP the call */
	OR2_CAS_CLEAR_BACK,
	/* Hangup immediately. I have only seen this in Brazil, where the central
	   waits several seconds (30?) after Clear Back to release the line, with
	   this signal no waiting will be done. When receiving this signal openr2 
	   behaves the same as with clear back, no waiting is done with clear back
	   anyway, should we? */
	OR2_CAS_FORCED_RELEASE,
	/* They want to HANGUP the call */
	OR2_CAS_CLEAR_FORWARD,
	/* We set this to let know the other end we are ANSWERing the call and the
	   speech path is open */
	OR2_CAS_ANSWER,
} openr2_cas_signal_t;

OR2_DECLARE(const char *) openr2_proto_get_error(openr2_protocol_error_t reason);
OR2_DECLARE(const char *) openr2_proto_get_disconnect_string(openr2_call_disconnect_cause_t cause);
OR2_DECLARE(const char *) openr2_proto_get_category_string(openr2_calling_party_category_t category);
OR2_DECLARE(openr2_calling_party_category_t)  openr2_proto_get_category(const char *category);
OR2_DECLARE(const char *) openr2_proto_get_variant_string(openr2_variant_t variant);
OR2_DECLARE(openr2_variant_t) openr2_proto_get_variant(const char *variant);
OR2_DECLARE(const char *) openr2_proto_get_call_mode_string(openr2_call_mode_t mode);
OR2_DECLARE(const openr2_variant_entry_t *) openr2_proto_get_variant_list(int *numvariants);

#ifdef __OR2_COMPILING_LIBRARY__
#undef openr2_chan_t
#undef openr2_context_t
#endif

#if defined(__cplusplus)
} /* endif extern "C" */
#endif

#endif /* endif defined _OPENR2_PROTO_H_ */
