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
 * Cleiber Marques da Silva <cleibermarques@hotmail.com>
 * Humberto Figuera <hfiguera@gmail.com>
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include "openr2/r2ioabs.h"
#include "openr2/r2log-pvt.h"
#include "openr2/r2utils-pvt.h"
#include "openr2/r2proto-pvt.h"
#include "openr2/r2chan-pvt.h"
#include "openr2/r2context-pvt.h"

#define R2(r2chan, signal) (r2chan)->r2context->cas_signals[OR2_CAS_##signal]

#define GA_TONE(r2chan) (r2chan)->r2context->mf_ga_tones
#define GB_TONE(r2chan) (r2chan)->r2context->mf_gb_tones
#define GC_TONE(r2chan) (r2chan)->r2context->mf_gc_tones

#define GI_TONE(r2chan) (r2chan)->r2context->mf_g1_tones
#define GII_TONE(r2chan) (r2chan)->r2context->mf_g2_tones

#define TIMER(r2chan) (r2chan)->r2context->timers

/* Note that we compare >= because even if max_dnis is zero
   we could get 1 digit, want it or not :-) */
#define DNIS_COMPLETE(r2chan) ((r2chan)->dnis_len >= (r2chan)->r2context->max_dnis)

static void r2config_argentina(openr2_context_t *r2context)
{
	OR2_CONTEXT_STACK;
	r2context->mf_g1_tones.no_more_dnis_available = OR2_MF_TONE_INVALID;
	r2context->mf_g1_tones.caller_ani_is_restricted = OR2_MF_TONE_15;
	r2context->mf_g1_tones.no_more_ani_available = OR2_MF_TONE_12;
	r2context->timers.r2_metering_pulse = 400;
}

static void r2config_brazil(openr2_context_t *r2context)
{
	OR2_CONTEXT_STACK;
	r2context->mf_g1_tones.no_more_dnis_available = OR2_MF_TONE_INVALID;
	r2context->mf_g1_tones.caller_ani_is_restricted = OR2_MF_TONE_12;

	r2context->mf_g2_tones.collect_call = OR2_MF_TONE_8;

	r2context->mf_ga_tones.address_complete_charge_setup = OR2_MF_TONE_INVALID;
	r2context->mf_ga_tones.request_dnis_minus_1 = OR2_MF_TONE_9;

	r2context->mf_gb_tones.accept_call_with_charge = OR2_MF_TONE_1;
	r2context->mf_gb_tones.busy_number = OR2_MF_TONE_2;
	r2context->mf_gb_tones.accept_call_no_charge = OR2_MF_TONE_5;
	r2context->mf_gb_tones.special_info_tone = OR2_MF_TONE_6; /* holding? */
	r2context->mf_gb_tones.reject_collect_call = OR2_MF_TONE_7;
	r2context->mf_gb_tones.number_changed = OR2_MF_TONE_3;
	/* use out of order for unallocated, I could not find a special
	   tone for unallocated number on the Brazil spec */
	r2context->mf_gb_tones.unallocated_number = OR2_MF_TONE_7; /* was 8, but TONE_7 for unallocated in Brazil according to Marcia from ANATEL */
}

static void r2config_china(openr2_context_t *r2context)
{
	OR2_CONTEXT_STACK;
	/* In the ITU line signaling specifications, the C and D bits are set to 0 and
	   1 respectively, in China they are both set to 1. However, they are never
	   used, so their value never changes during a call */
	r2context->cas_nonr2_bits = 0x3;    /* 0011 */

	r2context->mf_ga_tones.request_next_ani_digit = OR2_MF_TONE_1;
	r2context->mf_ga_tones.request_category = OR2_MF_TONE_6;
	r2context->mf_ga_tones.address_complete_charge_setup = OR2_MF_TONE_INVALID;

	r2context->mf_gb_tones.accept_call_with_charge = OR2_MF_TONE_1;
	r2context->mf_gb_tones.busy_number = OR2_MF_TONE_2;
	r2context->mf_gb_tones.special_info_tone = OR2_MF_TONE_INVALID;

	r2context->mf_g1_tones.no_more_dnis_available = OR2_MF_TONE_INVALID;
}

static void r2config_itu(openr2_context_t *r2context)
{
	OR2_CONTEXT_STACK;
	return;
}

static void r2config_mexico(openr2_context_t *r2context)
{
	OR2_CONTEXT_STACK;

	/* Telmex, Avantel and most telcos in Mexico send DNIS first and ANI at the end, however
	   this can be modified by the user because I know of at least 1 telco (Maxcom)
	   which requires the ANI first and the DNIS later */
	r2context->get_ani_first = 0;

	/* Mexico use a special signal to request 
	   calling party category AND switch to Group C */
	r2context->mf_ga_tones.request_category = OR2_MF_TONE_INVALID;
	r2context->mf_ga_tones.request_category_and_change_to_gc = OR2_MF_TONE_6;
	r2context->mf_ga_tones.address_complete_charge_setup = OR2_MF_TONE_INVALID;

	/* GA next ANI is replaces by GC next ANI signal */
	r2context->mf_ga_tones.request_next_ani_digit = OR2_MF_TONE_INVALID;

	/* Group B */
	r2context->mf_gb_tones.accept_call_with_charge = OR2_MF_TONE_1;
	r2context->mf_gb_tones.accept_call_no_charge = OR2_MF_TONE_5;
	r2context->mf_gb_tones.busy_number = OR2_MF_TONE_2;
	r2context->mf_gb_tones.unallocated_number = OR2_MF_TONE_2;
	r2context->mf_gb_tones.special_info_tone = OR2_MF_TONE_INVALID;

	/* GROUP C */
	r2context->mf_gc_tones.request_next_ani_digit = OR2_MF_TONE_1;
	r2context->mf_gc_tones.request_change_to_g2 = OR2_MF_TONE_3;
	r2context->mf_gc_tones.request_next_dnis_digit_and_change_to_ga = OR2_MF_TONE_5;
	
	/* Mexico has no signal when running out of DNIS, 
	   timeout is used instead*/
	r2context->mf_g1_tones.no_more_dnis_available = OR2_MF_TONE_INVALID;
}

static void r2config_venezuela(openr2_context_t *r2context)
{
	OR2_CONTEXT_STACK;

	r2context->mf_ga_tones.request_next_ani_digit = OR2_MF_TONE_9;

	r2context->mf_g1_tones.caller_ani_is_restricted = OR2_MF_TONE_12;
	r2context->mf_g1_tones.no_more_dnis_available = OR2_MF_TONE_INVALID;
}

static void r2config_colombia(openr2_context_t *r2context)
{
	OR2_CONTEXT_STACK;

	r2context->mf_ga_tones.request_next_ani_digit = OR2_MF_TONE_1;
	r2context->mf_ga_tones.request_category = OR2_MF_TONE_6;
	r2context->mf_g1_tones.caller_ani_is_restricted = OR2_MF_TONE_12;

	r2context->mf_gb_tones.accept_call_with_charge = OR2_MF_TONE_1;
	r2context->mf_gb_tones.busy_number = OR2_MF_TONE_2;
	r2context->mf_gb_tones.accept_call_no_charge = OR2_MF_TONE_5;
	r2context->mf_gb_tones.unallocated_number = OR2_MF_TONE_6;
}

/* These are the R2 signals to be sent in th A and B CAS bits, the
   CD bits are usually static in 01, therefore 0x8 means 0x9 in the
   line (0x8 + 0x1) or (0x8 + whatever CD bits are set to) */
static const int standard_cas_signals[OR2_NUM_CAS_SIGNALS] =
{
	/* OR2_CAS_IDLE */ 0x8,           /* 1001 */
	/* OR2_CAS_BLOCK */ 0xC,          /* 1101 */
	/* OR2_CAS_SEIZE */ 0x0,          /* 0001 */
	/* OR2_CAS_SEIZE_ACK */ 0xC,      /* 1101 */
	/* OR2_CAS_CLEAR_BACK */ 0xC,     /* 1101 */
	/* OR2_CAS_FORCED_RELEASE */ 0x0, /* 0001 */
	/* OR2_CAS_CLEAR_FORWARD */ 0x8,  /* 1001 */
	/* OR2_CAS_ANSWER */ 0x4,         /* 0101 */

/* Another approach probably more correct is to redefine the standard_cas_signals
   when configuring the variant, changing OR2_CAS_ANSWER from 0x4 to 0x0, however
   that may be more risky and I dont want to affect current working code */
	/* For DTMF/R2 */
	/* OR2_CAS_SEIZE_ACK_DTMF */ 0x4, /* 0101 */
	/* OR2_CAS_ACCEPT_DTMF */ 0x8,    /* 1001 */
	/* OR2_CAS_ANSWER_DTMF */ 0x0     /* 0001 */

};

static const char *cas_names[OR2_NUM_CAS_SIGNALS] =
{
	/* OR2_CAS_IDLE */ "IDLE",
	/* OR2_CAS_BLOCK */ "BLOCK",
	/* OR2_CAS_SEIZE */ "SEIZE",
	/* OR2_CAS_SEIZE_ACK */ "SEIZE ACK",
	/* OR2_CAS_CLEAR_BACK */ "CLEAR BACK",
	/* OR2_CAS_FORCED_RELEASE */ "FORCED RELEASE",
	/* OR2_CAS_CLEAR_FORWARD */ "CLEAR FORWARD",
	/* OR2_CAS_ANSWER */ "ANSWER",

	/* For DTMF/R2 */

	/* These names should be reviewed, I just made them up */

	/* OR2_CAS_SEIZE_ACK_DTMF */ "ACK DTMF",
	/* OR2_CAS_ACCEPT_DTMF */ "ACCEPT DTMF",
	/* OR2_CAS_ANSWER_DTMF */ "ANSWER DTMF"
};

static openr2_variant_entry_t r2variants[] =
{
	/* ARGENTINA */ 
	{
		.id = OR2_VAR_ARGENTINA,
		.name = "AR",
		.country = "Argentina",
		.config = r2config_argentina,
	},	
	/* BRAZIL */ 
	{
		.id = OR2_VAR_BRAZIL,
		.name = "BR",
		.country = "Brazil",
		.config = r2config_brazil
	},
	/* CHINA */ 
	{
		.id = OR2_VAR_CHINA,
		.name = "CN",
		.country = "China",
		.config = r2config_china
	},	
	/* CZECH */ 
	{
		.id = OR2_VAR_CZECH,
		.name = "CZ",
		.country = "Czech Republic",
		.config = r2config_itu
	},		
	/* COLOMBIA */ 
	{
		.id = OR2_VAR_COLOMBIA,
		.name = "CO",
		.country = "Colombia",
		.config = r2config_colombia
	},		
	/* ECUADOR */ 
	{
		.id = OR2_VAR_ECUADOR,
		.name = "EC",
		.country = "Ecuador",
		.config = r2config_itu,
	},	
	/* ITU */
	{
		.id = OR2_VAR_ITU,
		.name = "ITU",
		.country = "International Telecommunication Union",
		.config = r2config_itu
	},
	/* MEXICO */ 
	{
		.id = OR2_VAR_MEXICO,
		.name = "MX",
		.country = "Mexico",
		.config = r2config_mexico
	},
	/* PHILIPPINES */ 
	{
		.id = OR2_VAR_PHILIPPINES,
		.name = "PH",
		.country = "Philippines",
		.config = r2config_itu
	},
	/* VENEZUELA */ 
	{
		.id = OR2_VAR_VENEZUELA,
		.name = "VE",
		.country = "Venezuela",
		.config = r2config_venezuela
	}
};

static void turn_off_mf_engine(openr2_chan_t *r2chan)
{
	OR2_CHAN_STACK;
	/* this is not needed for DTMF R2 mf engine, but does not hurt either */
	openr2_chan_cancel_timer(r2chan, &r2chan->timer_ids.mf_back_cycle);

	/* this is not needed for MFC R2 mf engine, but does not hurt either */
	r2chan->dialing_dtmf = 0;

	/* set the MF state to OFF */
	r2chan->mf_state = OR2_MF_OFF_STATE;
}

static int set_cas_signal(openr2_chan_t *r2chan, openr2_cas_signal_t signal)
{
	OR2_CHAN_STACK;
	int res, cas;
	if (signal == OR2_CAS_INVALID) {
		openr2_log(r2chan, OR2_LOG_ERROR, "Cannot set INVALID signal\n");
		return -1;
	}
	cas = r2chan->r2context->cas_signals[signal];
	openr2_log(r2chan, OR2_LOG_CAS_TRACE, "CAS Tx >> [%s] 0x%02X\n", cas_names[signal], cas);
	r2chan->cas_write = cas;
	r2chan->cas_tx_signal = signal;
	/* set the NON R2 bits to 1 */
	cas |= r2chan->r2context->cas_nonr2_bits; 
	res = openr2_io_set_cas(r2chan, cas);
	if (res) {
		openr2_log(r2chan, OR2_LOG_ERROR, "CAS I/O failure.\n");
		return -1;
	} 
	openr2_log(r2chan, OR2_LOG_CAS_TRACE, "CAS Raw Tx >> 0x%02X\n", cas);
	return 0;
}

/* Here we configure R2 as ITU and finally call a country specific function to alter the protocol description according
   to the specified R2 variant. The ITU blue book Q400 - Q490 defines other tones, but lets just use this for starters,
   other tones will be added as needed */
int openr2_proto_configure_context(openr2_context_t *r2context, openr2_variant_t variant, int max_ani, int max_dnis)
{
	OR2_CONTEXT_STACK;
	unsigned i = 0;
	unsigned limit = sizeof(r2variants)/sizeof(r2variants[0]);
	/* if we don't know that variant, return failure */
	for (i = 0; i < limit; i++) {
		if (variant == r2variants[i].id) {
			break;
		}
	}
	if (i == limit) {
		return -1;
	}

	/* set default standard CAS signals */
	memcpy(r2context->cas_signals, standard_cas_signals, sizeof(standard_cas_signals));

	/* Default Non-R2 bit required to be on is D */
	r2context->cas_nonr2_bits = 0x1;    /* 0001 */

	/* Default R2 bits are A and B */
	r2context->cas_r2_bits = 0xC; /*  1100 */

	/* set default values for the protocol timers */
	r2context->timers.mf_back_cycle = 5000;
	r2context->timers.mf_back_resume_cycle = 150;

	/* this was 10000 but someone from mx reported that Telmex needs more time in international calls */
	r2context->timers.mf_fwd_safety = 30000;

	r2context->timers.r2_seize = 8000;
	r2context->timers.r2_answer = 60000; 
	r2context->timers.r2_metering_pulse = 0;
	r2context->timers.r2_answer_delay = 150;
	r2context->timers.r2_double_answer = 400;

	/* Max ANI and DNIS */
	r2context->max_dnis = (max_dnis >= OR2_MAX_DNIS) ? OR2_MAX_DNIS - 1 : max_dnis;
	r2context->max_ani = (max_ani >= OR2_MAX_ANI) ? OR2_MAX_ANI - 1 : max_ani;

	/* the forward R2 side always send DNIS first but
	   most variants continue by asking ANI first
	   and continuing with DNIS at the end  */
	r2context->get_ani_first = 1;

	/* accept the call bypassing the use of group B and II tones */
	r2context->immediate_accept = 0;

	/* Group A tones. Requests of ANI, DNIS and Calling Party Category */
	r2context->mf_ga_tones.request_next_dnis_digit = OR2_MF_TONE_1;
	r2context->mf_ga_tones.request_dnis_minus_1 = OR2_MF_TONE_2;
	r2context->mf_ga_tones.request_dnis_minus_2 = OR2_MF_TONE_7;
	r2context->mf_ga_tones.request_dnis_minus_3 = OR2_MF_TONE_8;
	r2context->mf_ga_tones.request_all_dnis_again = OR2_MF_TONE_INVALID;
	r2context->mf_ga_tones.request_next_ani_digit = OR2_MF_TONE_5;
	r2context->mf_ga_tones.request_category = OR2_MF_TONE_5;
	r2context->mf_ga_tones.request_category_and_change_to_gc = OR2_MF_TONE_INVALID;
	r2context->mf_ga_tones.request_change_to_g2 = OR2_MF_TONE_3;
        /* It's unusual, but an ITU-compliant switch can accept in Group A */
	r2context->mf_ga_tones.address_complete_charge_setup = OR2_MF_TONE_6;
	r2context->mf_ga_tones.network_congestion = OR2_MF_TONE_4;

	/* Group B tones. Decisions about what to do with the call */
	r2context->mf_gb_tones.accept_call_with_charge = OR2_MF_TONE_6;
	r2context->mf_gb_tones.accept_call_no_charge = OR2_MF_TONE_7;
	r2context->mf_gb_tones.busy_number = OR2_MF_TONE_3;
	r2context->mf_gb_tones.network_congestion = OR2_MF_TONE_4;
	r2context->mf_gb_tones.unallocated_number = OR2_MF_TONE_5;
	r2context->mf_gb_tones.line_out_of_order = OR2_MF_TONE_8;
	r2context->mf_gb_tones.special_info_tone = OR2_MF_TONE_2;
	r2context->mf_gb_tones.reject_collect_call = OR2_MF_TONE_INVALID;
	r2context->mf_gb_tones.number_changed = OR2_MF_TONE_INVALID;

	/* Group C tones. Similar to Group A but for Mexico */
	r2context->mf_gc_tones.request_next_ani_digit = OR2_MF_TONE_INVALID;
	r2context->mf_gc_tones.request_change_to_g2 = OR2_MF_TONE_INVALID;
	r2context->mf_gc_tones.request_next_dnis_digit_and_change_to_ga = OR2_MF_TONE_INVALID;

	/* Group I tones. Attend requests of Group A  */
	r2context->mf_g1_tones.no_more_dnis_available = OR2_MF_TONE_15;
	r2context->mf_g1_tones.no_more_ani_available = OR2_MF_TONE_15;
	/* even though ITU does not define this signal, many countries do
	   and it does not hurt to add it anyway */
	r2context->mf_g1_tones.caller_ani_is_restricted = OR2_MF_TONE_12;

	/* Group II tones. */
	r2context->mf_g2_tones.national_subscriber = OR2_MF_TONE_1;
	r2context->mf_g2_tones.national_priority_subscriber = OR2_MF_TONE_2;
	r2context->mf_g2_tones.international_subscriber = OR2_MF_TONE_7;
	r2context->mf_g2_tones.international_priority_subscriber = OR2_MF_TONE_9;
	r2context->mf_g2_tones.collect_call = OR2_MF_TONE_INVALID;

	/* now configure the country specific variations */
	r2variants[i].config(r2context);
	return 0;
}

static const char *r2state2str(openr2_cas_state_t r2state)
{
	switch (r2state) {
	case OR2_IDLE:
		return "Idle";
	case OR2_SEIZE_ACK_TXD:
		return "Seize ACK Transmitted";
	case OR2_ANSWER_TXD:
		return "Answer Transmitted";
	case OR2_CLEAR_BACK_TXD:
		return "Clear Back Transmitted";
	case OR2_FORCED_RELEASE_TXD:
		return "Forced Release Transmitted";
	case OR2_CLEAR_FWD_RXD:
		return "Clear Forward Received";
	case OR2_SEIZE_TXD:
		return "Seize Transmitted";
	case OR2_SEIZE_IN_DTMF_TXD:
		return "Seize DTMF Transmitted";
	case OR2_SEIZE_ACK_RXD:
		return "Seize ACK Received";
	case OR2_SEIZE_ACK_IN_DTMF_RXD:
		return "Seize ACK DTMF Received";
	case OR2_CLEAR_BACK_TONE_RXD:
		return "Clear Back Tone Received";
	case OR2_ACCEPT_RXD:
		return "Accept Received";
	case OR2_ACCEPT_IN_DTMF_RXD:
		return "DTMF Accept Received";
	case OR2_ANSWER_RXD:
		return "Answer Received";
	case OR2_CLEAR_BACK_RXD:
		return "Clear Back Received";
	case OR2_FORCED_RELEASE_RXD:
		return "Forced Release Received";
	case OR2_ANSWER_RXD_MF_PENDING:
		return "Answer Received with MF Pending";
	case OR2_CLEAR_FWD_TXD:
		return "Clear Forward Transmitted";
	case OR2_BLOCKED:
		return "Blocked";
	default: 
		return "*Unknown*";
	}
}

static const char *mfstate2str(openr2_mf_state_t mf_state)
{
	switch (mf_state) {
	case OR2_MF_OFF_STATE:
		return "MF Engine Off";

	case OR2_MF_SEIZE_ACK_TXD:
		return "Seize ACK Transmitted";
	case OR2_MF_CATEGORY_RQ_TXD:
		return "Category Request Transmitted";
	case OR2_MF_DNIS_RQ_TXD:
		return "DNIS Request Transmitted";
	case OR2_MF_ANI_RQ_TXD:
		return "ANI Request Transmitted";
	case OR2_MF_CHG_GII_TXD:
		return "Change To Group II Request Transmitted";
	case OR2_MF_ACCEPTED_TXD:
		return "Accepted Call Transmitted";
	case OR2_MF_DISCONNECT_TXD:
		return "Disconnect Tone Transmitted";

	case OR2_MF_CATEGORY_TXD:
		return "Category Transmitted";
	case OR2_MF_DNIS_TXD:
		return "DNIS Digit Transmitted";
	case OR2_MF_DNIS_END_TXD:
		return "End of DNIS Transmitted";
	case OR2_MF_ANI_TXD:
		return "ANI Digit Transmitted";
	case OR2_MF_ANI_END_TXD:
		return "End of ANI Transmitted";
	case OR2_MF_WAITING_TIMEOUT:
		return "Waiting Far End Timeout";

	case OR2_MF_DIALING_DTMF:
		return "Dialing DTMF";

	default:
		return "*Unknown*";
	}
}

OR2_EXPORT_SYMBOL
const char *openr2_proto_get_error(openr2_protocol_error_t error)
{
	switch ( error ) {
	case OR2_INVALID_CAS_BITS:
		return "Invalid CAS";
	case OR2_INVALID_MF_TONE:
		return "Invalid Multi Frequency Tone";
	case OR2_BACK_MF_TIMEOUT:
		return "Multi Frequency Cycle Timeout";
	case OR2_SEIZE_TIMEOUT:
		return "Seize Timeout";
	case OR2_ANSWER_TIMEOUT:
		return "Answer Timeout";
	case OR2_INVALID_R2_STATE:
		return "Invalid R2 state";
	case OR2_INVALID_MF_STATE:
		return "Invalid Multy Frequency State";
	case OR2_INVALID_MF_GROUP:
		return "Invalid R2 Group";
	case OR2_FWD_SAFETY_TIMEOUT:
		return "Forward Safety Timeout";
	case OR2_BROKEN_MF_SEQUENCE:
		return "Broken MF Sequence";
	case OR2_LIBRARY_BUG:
		return "OpenR2 Library BUG";
	case OR2_INTERNAL_ERROR:
		return "OpenR2 Internal Error";
	default:
		return "*Unknown*";
	}
}

static const char *mfgroup2str(openr2_mf_group_t mf_group)
{
	switch ( mf_group ) {
	case OR2_MF_NO_GROUP:
		return "No Group";

	case OR2_MF_BACK_INIT:
		return "Backward MF init";
	case OR2_MF_GA:
		return "Backward Group A";
	case OR2_MF_GB:
		return "Backward Group B";
	case OR2_MF_GC:
		return "Backward Group C";

	case OR2_MF_FWD_INIT:
		return "Forward MF init";
	case OR2_MF_GI:
		return "Forward Group I";
	case OR2_MF_GII:
		return "Forward Group II";
	case OR2_MF_GIII:
		return "Forward Group III";

	case OR2_MF_DTMF_FWD_INIT:
		return "Forward DTMF init";

	default:
		return "*Unknown*";
	}
}

static const char *callstate2str(openr2_call_state_t state)
{
	switch (state) {
	case OR2_CALL_IDLE:
		return "Idle";
	case OR2_CALL_DIALING:
		return "Dialing";
	case OR2_CALL_OFFERED:
		return "Offered";
	case OR2_CALL_ACCEPTED:
		return "Accepted";
	case OR2_CALL_ANSWERED:
		return "Answered";
	case OR2_CALL_DISCONNECTED:
		return "Disconnected";
	}	
	return "*Unknown*";
}

OR2_EXPORT_SYMBOL
const char *openr2_proto_get_disconnect_string(openr2_call_disconnect_cause_t cause)
{
	switch (cause) {
	case OR2_CAUSE_BUSY_NUMBER:
		return "Busy Number";
	case OR2_CAUSE_NETWORK_CONGESTION:
		return "Network Congestion";
	case OR2_CAUSE_UNALLOCATED_NUMBER:
		return "Unallocated Number";
	case OR2_CAUSE_NUMBER_CHANGED:
		return "Number Changed";
	case OR2_CAUSE_OUT_OF_ORDER:
		return "Line Out Of Order";
	case OR2_CAUSE_UNSPECIFIED:
		return "Not Specified";
	case OR2_CAUSE_NORMAL_CLEARING:
		return "Normal Clearing";
	case OR2_CAUSE_NO_ANSWER:
		return "No Answer";
	case OR2_CAUSE_COLLECT_CALL_REJECTED:
		return "Collect Call Rejected";
	case OR2_CAUSE_FORCED_RELEASE:
		return "Forced Release";
	default:
		return "*Unknown*";
	}
}

static void openr2_proto_init(openr2_chan_t *r2chan)
{
	OR2_CHAN_STACK;
	int rc = 0;
	int myerrno = 0;
	/* cancel any event we could be waiting for */
	openr2_chan_cancel_all_timers(r2chan);

	/* initialize all the proto and call stuff */
	r2chan->read_enabled = 1;
	r2chan->ani[0] = '\0';
	r2chan->ani_len = 0;
	r2chan->ani_ptr = NULL;
	r2chan->dnis[0] = '\0';
	r2chan->dnis_len = 0;
	r2chan->dnis_index = 0;
	r2chan->caller_ani_is_restricted = 0;
	r2chan->caller_category = OR2_MF_TONE_INVALID;
	r2chan->r2_state = OR2_IDLE;
	turn_off_mf_engine(r2chan);
	r2chan->mf_group = OR2_MF_NO_GROUP;
	r2chan->call_state = OR2_CALL_IDLE;
	r2chan->direction = OR2_DIR_STOPPED;
	r2chan->answered = 0;
	r2chan->category_sent = 0;
	r2chan->mf_write_tone = 0;
	r2chan->mf_read_tone = 0;
	if (r2chan->logfile) {
		rc = fclose(r2chan->logfile);
		r2chan->logfile = NULL;
		if (rc) {
			myerrno = errno;
			EMI(r2chan)->on_os_error(r2chan, myerrno);
			openr2_log(r2chan, OR2_LOG_ERROR, "Closing log file failed: %s\n", strerror(myerrno));
		}
	}
}

int openr2_proto_set_idle(openr2_chan_t *r2chan)
{
	OR2_CHAN_STACK;
	openr2_proto_init(r2chan);
	if (set_cas_signal(r2chan, OR2_CAS_IDLE)) {
		r2chan->r2context->last_error = OR2_LIBERR_CANNOT_SET_IDLE;
		openr2_log(r2chan, OR2_LOG_ERROR, "failed to set channel %d to IDLE state.\n");
		return -1;
	}
	return 0;
}

int openr2_proto_set_blocked(openr2_chan_t *r2chan)
{
	OR2_CHAN_STACK;
	openr2_proto_init(r2chan);
	r2chan->r2_state = OR2_BLOCKED;
	if (set_cas_signal(r2chan, OR2_CAS_BLOCK)) {
		openr2_log(r2chan, OR2_LOG_ERROR, "failed to set channel %d to BLOCKED state.\n");
		return -1;
	}
	return 0;
}

/* This function is called when our side will no longer do R2 signaling
   ie, on call end and on protocol error, to fix the rx signal state */
static void fix_rx_signal(openr2_chan_t *r2chan)
{
	/* if the last received signal is clear forward, we may not see
	   a bit change to idle if clear forward and idle
	   use the same bit pattern (AFAIK this is always the case), 
	   change it to idle now that the call is end */
	if (R2(r2chan, CLEAR_FORWARD) == R2(r2chan, IDLE)
	   && r2chan->cas_rx_signal == OR2_CAS_CLEAR_FORWARD) {
		r2chan->cas_rx_signal = OR2_CAS_IDLE;
	} 
	/* also could happen protocol error because of an idle signal
	   in an invalid stage. On protocol error the cas_rx_signal 
	   is set to invalid to promote displaying the hex value, but its 
	   time to restore it */
	else if (r2chan->cas_read == R2(r2chan, IDLE)) {
		r2chan->cas_rx_signal = OR2_CAS_IDLE;
	}
}

static void handle_protocol_error(openr2_chan_t *r2chan, openr2_protocol_error_t reason)
{
	OR2_CHAN_STACK;
	openr2_log(r2chan, OR2_LOG_ERROR, 
			"Protocol error. Reason = %s, R2 State = %s, "
			"MF state = %s, MF Group = %s, CAS = 0x%02X\n"
			"DNIS = %s, ANI = %s, MF = 0x%02X\n", 
			openr2_proto_get_error(reason), 
			r2state2str(r2chan->r2_state), 
			mfstate2str(r2chan->mf_state), 
			mfgroup2str(r2chan->mf_group),
			r2chan->cas_read,
			r2chan->dnis, r2chan->ani,
			r2chan->mf_read_tone ? r2chan->mf_read_tone : 0x20);
	/* mute anything we may have */
	MFI(r2chan)->mf_select_tone(r2chan->mf_write_handle, 0);
	openr2_proto_set_idle(r2chan);
	EMI(r2chan)->on_protocol_error(r2chan, reason);
	fix_rx_signal(r2chan);
}

static void open_logfile(openr2_chan_t *r2chan, int backward)
{
	time_t currtime;
	struct tm loctime;
	char stringbuf[512];
	char currdir[512];
	char timestr[30];
	int res = 0;
	char *cres = NULL;
	int myerrno = 0;
	if (!r2chan->r2context->logdir) {
		cres = getcwd(currdir, sizeof(currdir));
		if (!cres) {
			myerrno = errno;
			EMI(r2chan)->on_os_error(r2chan, myerrno);
			openr2_log(r2chan, OR2_LOG_WARNING, "Could not get cwd: %s\n", strerror(myerrno));
			return;
		}
	}
	currtime = time(NULL);
	if ((time_t)-1 == currtime) {
		myerrno = errno;
		EMI(r2chan)->on_os_error(r2chan, myerrno);
		openr2_log(r2chan, OR2_LOG_ERROR, "Cannot get time: %s\n", strerror(myerrno));
		return;
	}
	if (!openr2_localtime_r(&currtime, &loctime)) {
		openr2_log(r2chan, OR2_LOG_ERROR, "Failed to get local time\n");
		return;
	}
	res = snprintf(stringbuf, sizeof(stringbuf), "%s/chan-%d-%s-%ld-%d%02d%02d%02d%02d%02d.call", 
			r2chan->r2context->logdir ? r2chan->r2context->logdir : currdir, 
			r2chan->number, 
			backward ? "backward" : "forward",
			r2chan->call_count++,
			(1900 + loctime.tm_year), (1 + loctime.tm_mon), loctime.tm_mday, 
			loctime.tm_hour, loctime.tm_min, loctime.tm_sec);
	if (res >= sizeof(stringbuf)) {
		openr2_log(r2chan, OR2_LOG_WARNING, "Failed to create file name of length %d.\n", res);
		return;
	} 
	/* sanity check */
	if (r2chan->logfile) {
		openr2_log(r2chan, OR2_LOG_WARNING, "Yay, still have a log file, closing ...\n");
		res = fclose(r2chan->logfile);
		r2chan->logfile = NULL;
		if (res) {
			myerrno = errno;
			EMI(r2chan)->on_os_error(r2chan, myerrno);
			openr2_log(r2chan, OR2_LOG_ERROR, "Closing log file failed: %s\n", strerror(myerrno));
		}
	}
	r2chan->logfile = fopen(stringbuf, "w");
	if (!r2chan->logfile) {
		myerrno = errno;
		EMI(r2chan)->on_os_error(r2chan, myerrno);
		openr2_log(r2chan, OR2_LOG_ERROR, "fopen failed: %s\n", strerror(myerrno));
	} else {
		currtime = time(NULL);
		if (openr2_ctime_r(&currtime, timestr)) {
			timestr[strlen(timestr)-1] = 0; /* remove end of line */
			openr2_log(r2chan, OR2_LOG_DEBUG, "Call started at %s on chan %d\n", timestr, r2chan->number);
		} else {
			openr2_log(r2chan, OR2_LOG_ERROR, "Failed to get call starting time\n");
		}
	}
}

static void handle_incoming_call(openr2_chan_t *r2chan)
{
	OR2_CHAN_STACK;
	if (r2chan->call_files) {
		open_logfile(r2chan, 1);
	}
	/* we have received the line seize, we expect the first MF tone. 
	   let's init our MF engine, if we fail initing the MF engine
	   there is no point sending the seize ack, lets ignore the
	   call, the other end should timeout anyway */
	if (!MFI(r2chan)->mf_write_init(r2chan->mf_write_handle, 0)) {
		openr2_log(r2chan, OR2_LOG_ERROR, "Failed to init MF writer\n");
		handle_protocol_error(r2chan, OR2_INTERNAL_ERROR);
		return;
	}
	if (!MFI(r2chan)->mf_read_init(r2chan->mf_read_handle, 1)) {
		openr2_log(r2chan, OR2_LOG_ERROR, "Failed to init MF reader\n");
		handle_protocol_error(r2chan, OR2_INTERNAL_ERROR);
		return;
	}
	if (set_cas_signal(r2chan, OR2_CAS_SEIZE_ACK)) {
		openr2_log(r2chan, OR2_LOG_ERROR, "Failed to send seize ack!, incoming call not proceeding!\n");
		handle_protocol_error(r2chan, OR2_INTERNAL_ERROR);
		return;
	}
	r2chan->r2_state = OR2_SEIZE_ACK_TXD;
	r2chan->mf_state = OR2_MF_SEIZE_ACK_TXD;
	r2chan->mf_group = OR2_MF_BACK_INIT;
	r2chan->direction = OR2_DIR_BACKWARD;
	/* notify the user that a new call is starting to arrive */
	EMI(r2chan)->on_call_init(r2chan);
}

static void mf_fwd_safety_timeout_expired(openr2_chan_t *r2chan)
{
	OR2_CHAN_STACK;
	handle_protocol_error(r2chan, OR2_FWD_SAFETY_TIMEOUT);
}

static void mf_back_cycle_timeout_expired(openr2_chan_t *r2chan);
static void prepare_mf_tone(openr2_chan_t *r2chan, int tone)
{
	OR2_CHAN_STACK;
	int ret;
	/* put silence only if we have a write tone */
	if (!tone && r2chan->mf_write_tone) {
		openr2_log(r2chan, OR2_LOG_MF_TRACE, "MF Tx >> %c [OFF]\n", r2chan->mf_write_tone);
		if (openr2_io_flush_write_buffers(r2chan)) {
			return;
		}
	} 
	/* just choose the tone if the last chosen tone is different */
	if (r2chan->mf_write_tone != tone) {
		ret = MFI(r2chan)->mf_select_tone(r2chan->mf_write_handle, tone);
		if (-1 == ret) {
			/* this is not a protocol error, but there is nothing else we can do anyway */
			openr2_log(r2chan, OR2_LOG_ERROR, "failed to select MF tone\n");
			handle_protocol_error(r2chan, OR2_INTERNAL_ERROR);
			return;
		}
		if (tone) {
			openr2_log(r2chan, OR2_LOG_MF_TRACE, "MF Tx >> %c [ON]\n", tone);
			if (r2chan->direction == OR2_DIR_BACKWARD) {
				/* schedule a new timer that will handle the timeout for our backward request */
				r2chan->timer_ids.mf_back_cycle = openr2_chan_add_timer(r2chan, TIMER(r2chan).mf_back_cycle, 
				mf_back_cycle_timeout_expired, "mf_back_cycle");
			}
		}	
		r2chan->mf_write_tone = tone;
	}	
}

/* this function just accepts from -3 to 1 as valid offsets */
static void mf_send_dnis(openr2_chan_t *r2chan, int offset)
{
	OR2_CHAN_STACK;
	int a_offset = abs(offset);
	switch (offset) {
	case -1:
	case -2:
	case -3:
		/* get a previous DNIS */
		r2chan->dnis_index = r2chan->dnis_index >= a_offset ? (r2chan->dnis_index - a_offset) : 0;
		break;
	case 0:
		/* do nothing to dnis_index, the current DNIS index has the requested DNIS to send */
		break;
	case 1:
		/* get the next DNIS digit */
		r2chan->dnis_index++;
		break;
	default:
		/* a bug in the library definitely */
		openr2_log(r2chan, OR2_LOG_ERROR, "BUG: invalid DNIS offset\n");
		handle_protocol_error(r2chan, OR2_LIBRARY_BUG);
		return;
	}
	/* if there are still some DNIS to send out */
	if (r2chan->dnis[r2chan->dnis_index]) {
		openr2_log(r2chan, OR2_LOG_DEBUG, "Sending DNIS digit %c\n", r2chan->dnis[r2chan->dnis_index]);
		r2chan->mf_state = OR2_MF_DNIS_TXD;
		prepare_mf_tone(r2chan, r2chan->dnis[r2chan->dnis_index]);
	/* if no more DNIS, and there is a signal for it, use it */
	} else if (GI_TONE(r2chan).no_more_dnis_available) {
		openr2_log(r2chan, OR2_LOG_DEBUG, "Sending unavailable DNIS signal\n");
		r2chan->mf_state = OR2_MF_DNIS_END_TXD;
		prepare_mf_tone(r2chan, GI_TONE(r2chan).no_more_dnis_available);
	} else {
		openr2_log(r2chan, OR2_LOG_DEBUG, "No more DNIS. Doing nothing, waiting for timeout.\n");
		/* the callee should timeout to detect end of DNIS and
		   resume the MF signaling */
		r2chan->mf_state = OR2_MF_WAITING_TIMEOUT;
		/* even when we are waiting the other end to timeout we
		   cannot wait forever, put a timer to make sure of that */
		r2chan->timer_ids.mf_fwd_safety = openr2_chan_add_timer(r2chan, TIMER(r2chan).mf_fwd_safety, 
				mf_fwd_safety_timeout_expired, "mf_fwd_safety");
	}
}

static void report_call_disconnection(openr2_chan_t *r2chan, openr2_call_disconnect_cause_t cause)
{
	OR2_CHAN_STACK;
	openr2_log(r2chan, OR2_LOG_NOTICE, "Far end disconnected. Reason: %s\n", openr2_proto_get_disconnect_string(cause));
	r2chan->call_state = OR2_CALL_DISCONNECTED;
	EMI(r2chan)->on_call_disconnect(r2chan, cause);
}

static void report_call_end(openr2_chan_t *r2chan)
{
	OR2_CHAN_STACK;
	openr2_log(r2chan, OR2_LOG_DEBUG, "Call ended\n");
	openr2_proto_set_idle(r2chan);
	EMI(r2chan)->on_call_end(r2chan);
	fix_rx_signal(r2chan);
}

static void r2_metering_pulse(openr2_chan_t *r2chan)
{
	OR2_CHAN_STACK;
	report_call_disconnection(r2chan, OR2_CAUSE_NORMAL_CLEARING);
}

#define CAS_LOG_RX(signal_name) r2chan->cas_rx_signal = OR2_CAS_##signal_name; \
		openr2_log(r2chan, OR2_LOG_CAS_TRACE, "CAS Rx << [%s] 0x%02X\n", \
		(OR2_CAS_##signal_name != OR2_CAS_INVALID) \
		? cas_names[OR2_CAS_##signal_name] : openr2_proto_get_rx_cas_string(r2chan), cas); 
/* verify if the received bits are a disconnection signal of some sort */
static int check_backward_disconnection(openr2_chan_t *r2chan, int cas, 
		openr2_call_disconnect_cause_t *cause, openr2_cas_state_t *state)
{
	if (cas == R2(r2chan, CLEAR_BACK)) {
		CAS_LOG_RX(CLEAR_BACK);
		*state = OR2_CLEAR_BACK_RXD;
		*cause = OR2_CAUSE_NORMAL_CLEARING;
		return -1;
	}
	/* this is apparently just used in Brazil, but I don't think it's a bad idea to
	   to have it here for other variants as well just in case. If we ever find a reason to
	   just accept this signal for Brazil, we need just to check the variant here 
	   as well, or use some sort of per-variant flag to accept it */
	if (cas == R2(r2chan, FORCED_RELEASE)) {
		CAS_LOG_RX(FORCED_RELEASE);
		*state = OR2_FORCED_RELEASE_RXD;
		*cause = OR2_CAUSE_FORCED_RELEASE;
		return -1;
	}	
	return 0;
}

static void persistence_check_expired(openr2_chan_t *r2chan)
{
	int cas, res, myerrno;
	int rawcas;
	r2chan->timer_ids.cas_persistence_check = 0;
	res = openr2_io_get_cas(r2chan, &rawcas);
	if (res) {
		openr2_log(r2chan, OR2_LOG_ERROR, "Getting CAS bits from I/O device for persistence check failed: %s\n", strerror(myerrno));
		return;
	}
	/* pick up only the R2 bits */
	cas = rawcas & r2chan->r2context->cas_r2_bits;
	/* If the R2 bits are the same as the last time we read, handle the event now */
	if (r2chan->cas_persistence_check_signal == cas) {
		openr2_log(r2chan, OR2_LOG_DEBUG, "CAS signal 0x%02X has persisted, handling ...\n", r2chan->cas_persistence_check_signal);
		openr2_proto_handle_cas(r2chan);
	} 
	/* if the bits are a new pattern, schedule the persistence check again */
	else if (r2chan->cas_read != cas){
		openr2_log(r2chan, OR2_LOG_DEBUG, "False positive CAS signal 0x%02X, ignoring but handling new signal ...\n", 
				r2chan->cas_persistence_check_signal);
		openr2_log(r2chan, OR2_LOG_CAS_TRACE, "CAS Raw Rx << 0x%02X (in persistence check handler)\n", rawcas);
		openr2_log(r2chan, OR2_LOG_DEBUG, "Bits changed from 0x%02X to 0x%02X (in persistence check handler)\n", 
				r2chan->cas_read, cas);
		r2chan->cas_persistence_check_signal = cas;
		r2chan->timer_ids.cas_persistence_check = openr2_chan_add_timer(r2chan, TIMER(r2chan).cas_persistence_check,
				                                                persistence_check_expired, "cas_persistence_check");
	}
	/* else, we just returned to the state we were on, let's pretend this never happened */
	else {
		openr2_log(r2chan, OR2_LOG_DEBUG, "False positive CAS signal 0x%02X, ignoring ...\n", r2chan->cas_persistence_check_signal);
		r2chan->cas_persistence_check_signal = -1;
	}
}

static void r2_answer_timeout_expired(openr2_chan_t *r2chan);
int openr2_proto_handle_cas(openr2_chan_t *r2chan)
{
	OR2_CHAN_STACK;
	int cas, res;
	openr2_cas_state_t out_r2_state = OR2_INVALID_STATE;
	openr2_call_disconnect_cause_t out_disconnect_cause = OR2_CAUSE_NORMAL_CLEARING;

	/* if we have CAS persistence check and we're here because of the timer expired
	   then we don't need to read the CAS again, let's go directly to handle the bits */
	if (r2chan->cas_persistence_check_signal != -1 && r2chan->timer_ids.cas_persistence_check == 0) {
		openr2_log(r2chan, OR2_LOG_NOTICE, "Handling persistent pattern 0x%02x\n", r2chan->cas_persistence_check_signal);
		cas = r2chan->cas_persistence_check_signal;
		r2chan->cas_persistence_check_signal = -1;
		goto handlecas;
	} 

	res = openr2_io_get_cas(r2chan, &cas);
	if (res) {
		openr2_log(r2chan, OR2_LOG_ERROR, "Getting CAS from I/O device failed\n");
		return -1;
	}
	if (r2chan->cas_persistence_check_signal != -1) {
		openr2_log(r2chan, OR2_LOG_CAS_TRACE, "CAS Raw Rx << 0x%02X\n", cas);
	}	
	/* pick up only the R2 bits */
	cas &= r2chan->r2context->cas_r2_bits;
	/* If the R2 bits are the same as the last time we read just ignore them */
	if (r2chan->cas_read == cas) {
		if (r2chan->timer_ids.cas_persistence_check) {
			openr2_chan_cancel_timer(r2chan, &r2chan->timer_ids.cas_persistence_check);
			openr2_log(r2chan, OR2_LOG_DEBUG, "False positive CAS signal 0x%02X, ignoring ...\n", r2chan->cas_persistence_check_signal);
			r2chan->cas_persistence_check_signal = -1;
		}
		return 0;
	} else {
		openr2_log(r2chan, OR2_LOG_DEBUG, "Bits changed from 0x%02X to 0x%02X\n", r2chan->cas_read, cas);
	}
	if (TIMER(r2chan).cas_persistence_check) {
		openr2_log(r2chan, OR2_LOG_DEBUG, "CAS Persistence check is enabled, waiting %d ms\n", TIMER(r2chan).cas_persistence_check);
		openr2_chan_cancel_timer(r2chan, &r2chan->timer_ids.cas_persistence_check);
		r2chan->cas_persistence_check_signal = cas;
		r2chan->timer_ids.cas_persistence_check = openr2_chan_add_timer(r2chan, TIMER(r2chan).cas_persistence_check,
				                                                persistence_check_expired, "cas_persistence_check");
		return 0;
	}

handlecas:

	r2chan->cas_read = cas;
	/* if were in alarm, just save the read bits and set the 
	   CAS signal to invalid, since the bits cannot mean anything
	   when in alarm */
	if (r2chan->inalarm) {
		r2chan->cas_rx_signal = OR2_CAS_INVALID;
		return 0;
	}
	/* ok, bits have changed, we need to know in which 
	   CAS state we are to know what to do */
	switch (r2chan->r2_state) {
	case OR2_IDLE:
		if (cas == R2(r2chan, BLOCK)) {
			CAS_LOG_RX(BLOCK);
			EMI(r2chan)->on_line_blocked(r2chan);
			return 0;
		}
		if (cas == R2(r2chan, IDLE)) {
			CAS_LOG_RX(IDLE);
			EMI(r2chan)->on_line_idle(r2chan);
			return 0;
		}
		if (cas == R2(r2chan, SEIZE)) {
			CAS_LOG_RX(SEIZE);
			/* we are in IDLE and just received a seize request
			   lets handle this new call */
			handle_incoming_call(r2chan);
		} else {
			CAS_LOG_RX(INVALID);
			handle_protocol_error(r2chan, OR2_INVALID_CAS_BITS);
		}
		break;
	case OR2_SEIZE_ACK_TXD:
	case OR2_ANSWER_TXD:
		/* if call setup already started or the call is answered 
		   the only valid bit pattern is a clear forward, everything
		   else is protocol error */
		if (cas == R2(r2chan, CLEAR_FORWARD)) {
			CAS_LOG_RX(CLEAR_FORWARD);
			r2chan->r2_state = OR2_CLEAR_FWD_RXD;
			report_call_disconnection(r2chan, OR2_CAUSE_NORMAL_CLEARING);
		} else {
			CAS_LOG_RX(INVALID);
			handle_protocol_error(r2chan, OR2_INVALID_CAS_BITS);
		}
		break;
	case OR2_SEIZE_TXD:
		/* if we transmitted a seize we expect the seize ACK */
		if (cas == R2(r2chan, SEIZE_ACK)) {
			/* When the other side send us the seize ack, MF tones
			   can start, we start transmitting DNIS */
			CAS_LOG_RX(SEIZE_ACK);
			openr2_chan_cancel_timer(r2chan, &r2chan->timer_ids.r2_seize);
			r2chan->r2_state = OR2_SEIZE_ACK_RXD;
			r2chan->mf_group = OR2_MF_GI;
			MFI(r2chan)->mf_write_init(r2chan->mf_write_handle, 1);
			MFI(r2chan)->mf_read_init(r2chan->mf_read_handle, 0);
			mf_send_dnis(r2chan, 0);
		} else if (check_backward_disconnection(r2chan, cas, &out_disconnect_cause, &out_r2_state)) {
			openr2_log(r2chan, OR2_LOG_DEBUG, "Disconnection before seize ack detected!");
			/* I believe we just fall here with release forced since clear back signal is usually (always?) the
			   same as Seize ACK and therefore there will be not a bit patter change in that case. 
			   I believe the correct behavior for this case is to just proceed with disconnection without waiting 
			   for any other MF activity, the call is going down anyway */
			r2chan->r2_state = out_r2_state;
			report_call_disconnection(r2chan, out_disconnect_cause);
		} else {
			CAS_LOG_RX(INVALID);
			handle_protocol_error(r2chan, OR2_INVALID_CAS_BITS);
		}
		break;
	case OR2_CLEAR_BACK_TXD:
	case OR2_FORCED_RELEASE_TXD:
		if (cas == R2(r2chan, CLEAR_FORWARD)) {
			CAS_LOG_RX(CLEAR_FORWARD);
			report_call_end(r2chan);
		} else {
			CAS_LOG_RX(INVALID);
			handle_protocol_error(r2chan, OR2_INVALID_CAS_BITS);
		}
		break;
	case OR2_ACCEPT_RXD:
		/* once we got MF ACCEPT tone, we expect the CAS Answer 
		   or some disconnection signal, anything else, protocol error */
		if (cas == R2(r2chan, ANSWER)) {
			CAS_LOG_RX(ANSWER);
			openr2_chan_cancel_timer(r2chan, &r2chan->timer_ids.r2_answer);
			r2chan->r2_state = OR2_ANSWER_RXD;
			r2chan->call_state = OR2_CALL_ANSWERED;
			turn_off_mf_engine(r2chan);
			r2chan->answered = 1;
			EMI(r2chan)->on_call_answered(r2chan);
		} else if (check_backward_disconnection(r2chan, cas, &out_disconnect_cause, &out_r2_state)) {
			r2chan->r2_state = out_r2_state;
			report_call_disconnection(r2chan, out_disconnect_cause);
		} else {
			CAS_LOG_RX(INVALID);
			handle_protocol_error(r2chan, OR2_INVALID_CAS_BITS);
		}
		break;
	case OR2_SEIZE_ACK_RXD:
		/* This state means we're during call setup (ANI/DNIS transmission) and the ACCEPT signal
		   has not been received, which requires some special handling, read below for more info ... */
		if (cas == R2(r2chan, ANSWER)) {
			openr2_log(r2chan, OR2_LOG_DEBUG, "Answer before accept detected!\n");
			/* sometimes, since CAS signaling is faster than MF detectors we
			   may receive the ANSWER signal before actually receiving the
			   MF tone that indicates the call has been accepted (OR2_ACCEPT_RXD). We
			   must not turn off the tone detector because the tone off condition is still missing */
			CAS_LOG_RX(ANSWER);
			r2chan->r2_state = OR2_ANSWER_RXD_MF_PENDING;
		} else if (check_backward_disconnection(r2chan, cas, &out_disconnect_cause, &out_r2_state)) {
			openr2_log(r2chan, OR2_LOG_DEBUG, "Disconnection before accept detected!\n");
			/* I believe we just fall here with release forced since clear back signal is usually (always?) the
			   same as Seize ACK and therefore there will be not a bit patter change in that case. 
			   I believe the correct behavior for this case is to just proceed with disconnection without waiting 
			   for any other MF activity, the call is going down anyway */
			r2chan->r2_state = out_r2_state;
			report_call_disconnection(r2chan, out_disconnect_cause);
		} else {
			CAS_LOG_RX(INVALID);
			handle_protocol_error(r2chan, OR2_INVALID_CAS_BITS);
		}
		break;
	case OR2_ANSWER_RXD_MF_PENDING:
	case OR2_ANSWER_RXD:
		if (cas == R2(r2chan, CLEAR_BACK)) {
			CAS_LOG_RX(CLEAR_BACK);
			r2chan->r2_state = OR2_CLEAR_BACK_RXD;
			if (TIMER(r2chan).r2_metering_pulse) {
				/* if the variant may have metering pulses, this clear back could be not really
				   a clear back but a metering pulse, lets put the timer. If the CAS signal does not
				   come back to ANSWER then is really a clear back */
				r2chan->timer_ids.r2_metering_pulse = openr2_chan_add_timer(r2chan, TIMER(r2chan).r2_metering_pulse, 
						r2_metering_pulse, "r2_metering_pulse");
			} else {
				report_call_disconnection(r2chan, OR2_CAUSE_NORMAL_CLEARING);
			}
		/* For DTMF R2, for some strange reason they send CLEAR_FORWARD even when they are the backward side!! */
		} else if (cas == R2(r2chan, CLEAR_FORWARD)) {
			CAS_LOG_RX(CLEAR_FORWARD);
			r2chan->r2_state = OR2_CLEAR_FWD_RXD;
			/* should we test for metering pulses here? */
			report_call_disconnection(r2chan, OR2_CAUSE_NORMAL_CLEARING);
		} else if (cas == R2(r2chan, FORCED_RELEASE)) {
			CAS_LOG_RX(FORCED_RELEASE);
			r2chan->r2_state = OR2_FORCED_RELEASE_RXD;
			report_call_disconnection(r2chan, OR2_CAUSE_FORCED_RELEASE);
		} else {
			CAS_LOG_RX(INVALID);
			handle_protocol_error(r2chan, OR2_INVALID_CAS_BITS);
		}
		break;
	case OR2_CLEAR_BACK_TONE_RXD:
		if (cas == R2(r2chan, IDLE)) {
			CAS_LOG_RX(IDLE);
			report_call_end(r2chan);
		} else {
			CAS_LOG_RX(INVALID);
			handle_protocol_error(r2chan, OR2_INVALID_CAS_BITS);
		}
		break;
	case OR2_CLEAR_FWD_TXD:
		if (cas == R2(r2chan, IDLE)) {
			CAS_LOG_RX(IDLE);
			report_call_end(r2chan);
		} else if (check_backward_disconnection(r2chan, cas, &out_disconnect_cause, &out_r2_state)) {
			report_call_end(r2chan);
		} else {
			CAS_LOG_RX(INVALID);
			handle_protocol_error(r2chan, OR2_INVALID_CAS_BITS);
		}
		break;
	case OR2_CLEAR_BACK_RXD:
		/* we got clear back but we have not transmitted clear fwd yet, then, the only
		   reason for CAS change is a possible metering pulse, if we are not detecting a metering
		   pulse then is a protocol error */
		if (TIMER(r2chan).r2_metering_pulse && cas == R2(r2chan, ANSWER)) {
			/* cancel the metering timer and let's pretend this never happened */
			CAS_LOG_RX(ANSWER);
			openr2_chan_cancel_timer(r2chan, &r2chan->timer_ids.r2_metering_pulse);
			r2chan->r2_state = OR2_ANSWER_RXD;
			/* TODO: we could notify the user here with a callback, but I have not seen a use for this yet */
			openr2_log(r2chan, OR2_LOG_NOTICE, "Metering pulse received");
			EMI(r2chan)->on_billing_pulse_received(r2chan);
		} else {
			CAS_LOG_RX(INVALID);
			handle_protocol_error(r2chan, OR2_INVALID_CAS_BITS);
		}
		break;
	case OR2_BLOCKED:
		/* we're blocked, unless they are setting IDLE, we don't care */
		if (cas == R2(r2chan, IDLE)) {
			CAS_LOG_RX(IDLE);
			EMI(r2chan)->on_line_idle(r2chan);
		} else {
			CAS_LOG_RX(INVALID);
			openr2_log(r2chan, OR2_LOG_NOTICE, "Doing nothing on CAS change, we're blocked.\n");
		}	
		break;

	/* DTMF R2 states */
	case OR2_SEIZE_IN_DTMF_TXD:
		if (cas == R2(r2chan, SEIZE_ACK_DTMF)) {
			CAS_LOG_RX(SEIZE_ACK_DTMF);
			openr2_log(r2chan, OR2_LOG_NOTICE, "DTMF/R2 call acknowledge!\n");
			r2chan->r2_state = OR2_SEIZE_ACK_IN_DTMF_RXD;
			/* this is kind of a seize ack, cancel seize ack timer */
			openr2_chan_cancel_timer(r2chan, &r2chan->timer_ids.r2_seize);
		} else {
			CAS_LOG_RX(INVALID);
			handle_protocol_error(r2chan, OR2_INVALID_CAS_BITS);
		}
		break;

	case OR2_SEIZE_ACK_IN_DTMF_RXD:
		if (cas == R2(r2chan, ACCEPT_DTMF)) {
			CAS_LOG_RX(ACCEPT_DTMF);
			openr2_log(r2chan, OR2_LOG_NOTICE, "DTMF/R2 call accepted!\n");
			/* They have accepted the call. We do nothing but wait for answer. */
			r2chan->r2_state = OR2_ACCEPT_IN_DTMF_RXD;
			r2chan->timer_ids.r2_answer = openr2_chan_add_timer(r2chan, TIMER(r2chan).r2_answer, 
									    r2_answer_timeout_expired, "r2_answer");
			EMI(r2chan)->on_call_accepted(r2chan, OR2_CALL_UNKNOWN);
		} else {
			CAS_LOG_RX(INVALID);
			handle_protocol_error(r2chan, OR2_INVALID_CAS_BITS);
		}
		break;

	case OR2_ACCEPT_IN_DTMF_RXD:
		if (cas == R2(r2chan, ANSWER_DTMF)) {
			CAS_LOG_RX(ANSWER_DTMF);
			openr2_chan_cancel_timer(r2chan, &r2chan->timer_ids.r2_answer);
			r2chan->r2_state = OR2_ANSWER_RXD;
			r2chan->call_state = OR2_CALL_ANSWERED;
			r2chan->answered = 1;
			EMI(r2chan)->on_call_answered(r2chan);
		} else {
			CAS_LOG_RX(INVALID);
			handle_protocol_error(r2chan, OR2_INVALID_CAS_BITS);
		}
		break;

	default:
		CAS_LOG_RX(INVALID);
		handle_protocol_error(r2chan, OR2_INVALID_R2_STATE);
		break;
	}
	return 0;
}

static const char *get_string_from_mode(openr2_call_mode_t mode)
{
	switch (mode) {
	case OR2_CALL_WITH_CHARGE:
		return "Call With Charge";
	case OR2_CALL_NO_CHARGE:
		return "Call With No Charge";
	case OR2_CALL_SPECIAL:
		return "Special Call";
	default:
		return "*UNKNOWN*";
	}
}

static int get_tone_from_mode(openr2_chan_t *r2chan, openr2_call_mode_t mode)
{
	switch (mode) {
	case OR2_CALL_WITH_CHARGE:
		return GB_TONE(r2chan).accept_call_with_charge;
	case OR2_CALL_NO_CHARGE:
		return GB_TONE(r2chan).accept_call_no_charge;
	case OR2_CALL_SPECIAL:
		return GB_TONE(r2chan).special_info_tone;
	default:
		openr2_log(r2chan, OR2_LOG_WARNING, "Unkown call mode (%d), defaulting to %s\n", get_string_from_mode(OR2_CALL_NO_CHARGE));
		return GB_TONE(r2chan).accept_call_no_charge;
	}
}

int openr2_proto_accept_call(openr2_chan_t *r2chan, openr2_call_mode_t mode)
{
	OR2_CHAN_STACK;
	if (OR2_CALL_OFFERED != r2chan->call_state) {
		openr2_log(r2chan, OR2_LOG_WARNING, "Cannot accept call if the call has not been offered!\n");
		return -1;
	}
	r2chan->mf_state = OR2_MF_ACCEPTED_TXD;
	prepare_mf_tone(r2chan, get_tone_from_mode(r2chan, mode));		
	return 0;
}

static int send_clear_backward(openr2_chan_t *r2chan)
{
	OR2_CHAN_STACK;
	r2chan->r2_state = OR2_CLEAR_BACK_TXD;
	turn_off_mf_engine(r2chan);
	return set_cas_signal(r2chan, OR2_CAS_CLEAR_BACK);	
}

static int send_forced_release(openr2_chan_t *r2chan)
{
	OR2_CHAN_STACK;
	r2chan->r2_state = OR2_FORCED_RELEASE_TXD;
	turn_off_mf_engine(r2chan);
	return set_cas_signal(r2chan, OR2_CAS_FORCED_RELEASE);	
}

static void double_answer_handler(openr2_chan_t *r2chan)
{
	OR2_CHAN_STACK;
	if (r2chan->r2_state == OR2_ANSWER_TXD) {
		if (send_clear_backward(r2chan)) {
			openr2_log(r2chan, OR2_LOG_ERROR, "Failed to send Clear Backward!, cannot send double answer!\n");
			return;
		}
		r2chan->r2_state = OR2_EXECUTING_DOUBLE_ANSWER;
		r2chan->timer_ids.r2_double_answer = openr2_chan_add_timer(r2chan, TIMER(r2chan).r2_double_answer, 
				                     double_answer_handler, "r2_double_answer");
	} else if (r2chan->r2_state == OR2_EXECUTING_DOUBLE_ANSWER) {
		if (set_cas_signal(r2chan, OR2_CAS_ANSWER)) {
			openr2_log(r2chan, OR2_LOG_ERROR, "Cannot re-send ANSWER signal, failed to answer call!\n");
			return;
		}
		r2chan->r2_state = OR2_ANSWER_TXD;
	} else {
		openr2_log(r2chan, OR2_LOG_ERROR, "BUG: double_answer_handler called with an invalid state\n");
	}
}

static int openr2_proto_do_answer(openr2_chan_t *r2chan)
{
	OR2_CHAN_STACK;
	if (r2chan->call_state != OR2_CALL_ACCEPTED) {
		openr2_log(r2chan, OR2_LOG_ERROR, "Cannot answer call if the call is not accepted first\n");
		return -1;
	}
	if (set_cas_signal(r2chan, OR2_CAS_ANSWER)) {
		openr2_log(r2chan, OR2_LOG_ERROR, "Cannot send ANSWER signal, failed to answer call!\n");
		return -1;
	}
	r2chan->call_state = OR2_CALL_ANSWERED;
	r2chan->r2_state = OR2_ANSWER_TXD;
	r2chan->answered = 1;
	return 0;
}

int openr2_proto_answer_call(openr2_chan_t *r2chan)
{
	OR2_CHAN_STACK;
	if (openr2_proto_do_answer(r2chan)) {
		return -1;
	}
	if (r2chan->r2context->double_answer) {
		r2chan->timer_ids.r2_double_answer = openr2_chan_add_timer(r2chan, TIMER(r2chan).r2_double_answer, 
				double_answer_handler, "r2_double_answer");
	}
	return 0;
}

int openr2_proto_answer_call_with_mode(openr2_chan_t *r2chan, openr2_answer_mode_t mode)
{
	OR2_CHAN_STACK;
	if (openr2_proto_do_answer(r2chan)) {
		return -1;
	}
	if (OR2_ANSWER_DOUBLE == mode) {
		r2chan->timer_ids.r2_double_answer = openr2_chan_add_timer(r2chan, TIMER(r2chan).r2_double_answer, 
				double_answer_handler, "r2_double_answer");
	}
	return 0;
}

static void request_calling_party_category(openr2_chan_t *r2chan)
{
	OR2_CHAN_STACK;
	int tone = GA_TONE(r2chan).request_category ? GA_TONE(r2chan).request_category : GA_TONE(r2chan).request_category_and_change_to_gc;
	r2chan->mf_group = GA_TONE(r2chan).request_category ? OR2_MF_GA : OR2_MF_GC;
	r2chan->mf_state = OR2_MF_CATEGORY_RQ_TXD;
	prepare_mf_tone(r2chan, tone);
}

static openr2_calling_party_category_t tone2category(openr2_chan_t *r2chan)
{
	OR2_CHAN_STACK;
	if (GII_TONE(r2chan).national_subscriber == r2chan->caller_category) {
		return OR2_CALLING_PARTY_CATEGORY_NATIONAL_SUBSCRIBER;

	} else if (GII_TONE(r2chan).national_priority_subscriber == r2chan->caller_category) {
		return OR2_CALLING_PARTY_CATEGORY_NATIONAL_PRIORITY_SUBSCRIBER;

	} else if (GII_TONE(r2chan).international_subscriber == r2chan->caller_category) {
		return OR2_CALLING_PARTY_CATEGORY_INTERNATIONAL_SUBSCRIBER;

	} else if (GII_TONE(r2chan).international_priority_subscriber == r2chan->caller_category) {
		return OR2_CALLING_PARTY_CATEGORY_INTERNATIONAL_PRIORITY_SUBSCRIBER;
	} else if (GII_TONE(r2chan).collect_call == r2chan->caller_category) {
		return OR2_CALLING_PARTY_CATEGORY_COLLECT_CALL;
	} else {
		return OR2_CALLING_PARTY_CATEGORY_UNKNOWN;
	}
}

static void bypass_change_to_g2(openr2_chan_t *r2chan)
{
	OR2_CHAN_STACK;
	/* Most variants of MFC/R2 offer a way to go directly to the call accepted state,
	   bypassing the use of group B and II tones */
	int accept_tone = GA_TONE(r2chan).address_complete_charge_setup;
	r2chan->mf_state = OR2_MF_ACCEPTED_TXD;
	r2chan->call_state = OR2_CALL_OFFERED;
	openr2_log(r2chan, OR2_LOG_DEBUG, "By-passing B/II signals, accept the call with signal 0x%X\n", accept_tone);
	prepare_mf_tone(r2chan, accept_tone);
	EMI(r2chan)->on_call_offered(r2chan, r2chan->caller_ani_is_restricted ? NULL : r2chan->ani, r2chan->dnis, tone2category(r2chan));
}

static void request_change_to_g2(openr2_chan_t *r2chan)
{
	OR2_CHAN_STACK;
	/* request to change to group 2 can come from either from Group C (only for Mexico)
	   or Group A (All the world, including Mexico) */
	int change_tone = (OR2_MF_GC == r2chan->mf_group) ? GC_TONE(r2chan).request_change_to_g2
		                                          : GA_TONE(r2chan).request_change_to_g2;
	r2chan->mf_group = OR2_MF_GB;
	r2chan->mf_state = OR2_MF_CHG_GII_TXD;
	openr2_log(r2chan, OR2_LOG_DEBUG, "Requesting change to Group II with signal 0x%X\n", change_tone);
	prepare_mf_tone(r2chan, change_tone);
}

static void try_change_to_g2(openr2_chan_t *r2chan)
{
	OR2_CHAN_STACK;
	if (r2chan->r2context->immediate_accept) {
		bypass_change_to_g2(r2chan);
		return;
	}
	request_change_to_g2(r2chan);
}

static void try_request_calling_party_category(openr2_chan_t *r2chan)
{
	OR2_CHAN_STACK;
	if (r2chan->r2context->skip_category) {
		try_change_to_g2(r2chan);
		return;
	}
	request_calling_party_category(r2chan);
}

static void set_silence(openr2_chan_t *r2chan)
{
	OR2_CHAN_STACK;
	prepare_mf_tone(r2chan, 0);
	r2chan->mf_write_tone = 0;
}

static void mf_back_resume_cycle(openr2_chan_t *r2chan)
{
	OR2_CHAN_STACK;
	set_silence(r2chan);
}

static void mf_back_cycle_timeout_expired(openr2_chan_t *r2chan)
{
	OR2_CHAN_STACK;
	if (OR2_MF_TONE_INVALID == GI_TONE(r2chan).no_more_dnis_available
	     && r2chan->mf_group == OR2_MF_GA
	     && r2chan->mf_state == OR2_MF_DNIS_RQ_TXD) {
	
		/*
		TODO:	
		how dow we know that the other end is in tone off condition?,
		we dont know that, we could have timeout even before they put off their tone
		if they never detect our tone */



		openr2_log(r2chan, OR2_LOG_DEBUG, "MF cycle timed out, no more DNIS\n");
		/* the other end has run out of DNIS digits and were in a R2 variant that
		   does not support 'No More DNIS available' signal (ain't that silly?), and
		   those R2 variants let the backward end to timeout and resume the MF dance,
		   that's why we timed out waiting for more DNIS. Let's resume the MF signaling
		   and ask the calling party category (if needed). Since they are now in a silent 
		   state we will not get a 'tone off' condition, hence we need a timeout to mute 
		   our tone */
		r2chan->timer_ids.mf_back_resume_cycle = openr2_chan_add_timer(r2chan, TIMER(r2chan).mf_back_resume_cycle, 
				                                               mf_back_resume_cycle, "mf_back_resume_cycle");
		if (!r2chan->r2context->get_ani_first) {
			/* we were not asked to get the ANI first, hence when this
		           timeout occurs we know for sure we have not retrieved ANI yet,
		           let's retrieve it now. */
			try_request_calling_party_category(r2chan);
		} else {
			/* ANI must have been retrieved already (before DNIS),
			   let's go directly to GII or directly accept the call without changing
			   to GII if immediate_accept has been setted, the final stage */
			try_change_to_g2(r2chan);
		}
	} else {
		openr2_log(r2chan, OR2_LOG_WARNING, "MF back cycle timed out!\n");
		handle_protocol_error(r2chan, OR2_BACK_MF_TIMEOUT);
	}	
}

static void request_next_dnis_digit(openr2_chan_t *r2chan)
{
	OR2_CHAN_STACK;
	openr2_mf_tone_t request_tone = (OR2_MF_GC == r2chan->mf_group) 
		                      ? GC_TONE(r2chan).request_next_dnis_digit_and_change_to_ga
		                      : GA_TONE(r2chan).request_next_dnis_digit;
	r2chan->mf_group = OR2_MF_GA;
	r2chan->mf_state = OR2_MF_DNIS_RQ_TXD;
	openr2_log(r2chan, OR2_LOG_DEBUG, "Requesting next DNIS with signal 0x%X.\n", request_tone);
	prepare_mf_tone(r2chan, request_tone);
}

static void mf_receive_expected_dnis(openr2_chan_t *r2chan, int tone)
{
	OR2_CHAN_STACK;
	int rc;
	if (OR2_MF_TONE_10 <= tone && OR2_MF_TONE_9 >= tone) {
		openr2_log(r2chan, OR2_LOG_DEBUG, "Getting DNIS digit %c\n", tone);
		r2chan->dnis[r2chan->dnis_len++] = tone;
		r2chan->dnis[r2chan->dnis_len] = '\0';
		openr2_log(r2chan, OR2_LOG_DEBUG, "DNIS so far: %s, expected length: %d\n", r2chan->dnis, r2chan->r2context->max_dnis);
		rc = EMI(r2chan)->on_dnis_digit_received(r2chan, tone);
		if (DNIS_COMPLETE(r2chan) || !rc) {
			if (!rc) {
				openr2_log(r2chan, OR2_LOG_DEBUG, "User requested us to stop getting DNIS!\n");
			} else {
				openr2_log(r2chan, OR2_LOG_DEBUG, "Done getting DNIS!\n");
			}
			/* if this is the first and last DNIS digit we have or
			   we were not required to get the ANI first, request it now, 
			   otherwise is time to go to GII signals */
			if (1 == r2chan->dnis_len || !r2chan->r2context->get_ani_first) {
				try_request_calling_party_category(r2chan);
			} else {
				try_change_to_g2(r2chan);
			} 
		} else if (1 == r2chan->dnis_len && r2chan->r2context->get_ani_first) {
			try_request_calling_party_category(r2chan);
		} else {
			request_next_dnis_digit(r2chan);
		}
	} else if (GI_TONE(r2chan).no_more_dnis_available == tone) {
		/* not sure if we ever could get no more dnis as first DNIS tone
		   but let's handle it just in case */
		if (0 == r2chan->dnis_len || !r2chan->r2context->get_ani_first) {
			try_request_calling_party_category(r2chan);
		} else {
			if (r2chan->r2context->immediate_accept) {
				bypass_change_to_g2(r2chan);
			} else {
				request_change_to_g2(r2chan);
			}
		}
	} else {
		/* we were supposed to handle DNIS, but the tone
		   is not in the range of valid digits */
		handle_protocol_error(r2chan, OR2_INVALID_MF_TONE);
	}
}

static int category2tone(openr2_chan_t *r2chan, openr2_calling_party_category_t category)
{
	OR2_CHAN_STACK;
	switch (category) {
	case OR2_CALLING_PARTY_CATEGORY_NATIONAL_SUBSCRIBER:
		return GII_TONE(r2chan).national_subscriber;
	case OR2_CALLING_PARTY_CATEGORY_NATIONAL_PRIORITY_SUBSCRIBER:
		return GII_TONE(r2chan).national_priority_subscriber;
	case OR2_CALLING_PARTY_CATEGORY_INTERNATIONAL_SUBSCRIBER:
		return GII_TONE(r2chan).international_subscriber;
	case OR2_CALLING_PARTY_CATEGORY_INTERNATIONAL_PRIORITY_SUBSCRIBER:
		return GII_TONE(r2chan).international_priority_subscriber;
	case OR2_CALLING_PARTY_CATEGORY_COLLECT_CALL:
		return GII_TONE(r2chan).collect_call;
	default:
		return GII_TONE(r2chan).national_subscriber;;
	}
}

static void mf_receive_expected_ani(openr2_chan_t *r2chan, int tone)
{
	OR2_CHAN_STACK;
	int next_ani_request_tone = GC_TONE(r2chan).request_next_ani_digit ? 
		                    GC_TONE(r2chan).request_next_ani_digit : 
				    GA_TONE(r2chan).request_next_ani_digit;
	/* no tone, just request next ANI if needed, otherwise
	   switch to Group B/II  */
	if (!tone || (OR2_MF_TONE_10 <= tone && OR2_MF_TONE_9 >= tone)) {
		/* if we have a tone, save it */
		if (tone) {
			openr2_log(r2chan, OR2_LOG_DEBUG, "Getting ANI digit %c\n", tone);
			r2chan->ani[r2chan->ani_len++] = tone;
			r2chan->ani[r2chan->ani_len] = '\0';
			openr2_log(r2chan, OR2_LOG_DEBUG, "ANI so far: %s, expected length: %d\n", r2chan->ani, r2chan->r2context->max_ani);
			EMI(r2chan)->on_ani_digit_received(r2chan, tone);
		}
		/* if we don't have a tone, or the ANI len is not enough yet, 
		   ask for more ANI */
		if (!tone || r2chan->r2context->max_ani > r2chan->ani_len) {
			r2chan->mf_state = OR2_MF_ANI_RQ_TXD;
			prepare_mf_tone(r2chan, next_ani_request_tone);
		} else {
			openr2_log(r2chan, OR2_LOG_DEBUG, "Done getting ANI!\n");
			if (!r2chan->r2context->get_ani_first || DNIS_COMPLETE(r2chan)) {
				if (r2chan->r2context->immediate_accept) {
					bypass_change_to_g2(r2chan);
				} else {
					request_change_to_g2(r2chan);
				}
			} else {
				request_next_dnis_digit(r2chan);
			}	
		}
	/* they notify us about no more ANI available or the ANI 
	   is restricted AKA private */
	} else if ( tone == GI_TONE(r2chan).no_more_ani_available ||
		    tone == GI_TONE(r2chan).caller_ani_is_restricted ) {
		openr2_log(r2chan, OR2_LOG_DEBUG, "Got end of ANI\n");
		if ( tone == GI_TONE(r2chan).caller_ani_is_restricted ) {
			openr2_log(r2chan, OR2_LOG_DEBUG, "ANI is restricted\n");
			r2chan->caller_ani_is_restricted = 1;
		}	
		if (!r2chan->r2context->get_ani_first || DNIS_COMPLETE(r2chan)) {
			if (r2chan->r2context->immediate_accept) {
				bypass_change_to_g2(r2chan);
			} else {
				request_change_to_g2(r2chan);
			}
		} else {
			request_next_dnis_digit(r2chan);
		}	
	} else {
		handle_protocol_error(r2chan, OR2_INVALID_MF_TONE);
	}
}

static void handle_forward_mf_tone(openr2_chan_t *r2chan, int tone)
{
	OR2_CHAN_STACK;
	/* Cancel MF back timer since we got a response from the forward side */
	openr2_chan_cancel_timer(r2chan, &r2chan->timer_ids.mf_back_cycle);
	switch (r2chan->mf_group) {
	/* we just sent the seize ACK and we are starting with the MF dance */
	case OR2_MF_BACK_INIT:
		switch (r2chan->mf_state) {
		case OR2_MF_SEIZE_ACK_TXD:
			/* after sending the seize ack we expect DNIS  */
			mf_receive_expected_dnis(r2chan, tone);
			break;
		default:
			handle_protocol_error(r2chan, OR2_INVALID_MF_STATE);
			break;
		}
		break;
	/* We are now at Group A signals, requesting DNIS or ANI, 
	   depending on the protocol variant */
	case OR2_MF_GA:
		/* ok we're at GROUP A signals. Let's see what was 
		   the last thing we did */
		switch (r2chan->mf_state) {
		/* we requested more DNIS */
		case OR2_MF_DNIS_RQ_TXD:
			/* then we receive more DNIS :) */
			mf_receive_expected_dnis(r2chan, tone);
			break;
		/* we requested the calling party category */
		case OR2_MF_CATEGORY_RQ_TXD:
			r2chan->caller_category = tone;
			if (r2chan->r2context->max_ani > 0) {
				mf_receive_expected_ani(r2chan, 0);
			} else {
				/* switch to Group B/II, we're ready to answer! */
				if (r2chan->r2context->immediate_accept) {
					bypass_change_to_g2(r2chan);
				} else {
					request_change_to_g2(r2chan);
				}
			}
			break;
		/* we requested more ANI */
		case OR2_MF_ANI_RQ_TXD:
			mf_receive_expected_ani(r2chan, tone);
			break;
		/* hu? WTF we were doing?? */
		default:
			handle_protocol_error(r2chan, OR2_INVALID_MF_STATE);
			break;
		}
		break;
	case OR2_MF_GB:
		switch (r2chan->mf_state) {
		case OR2_MF_CHG_GII_TXD:
			/* we cannot do anything by ourselves. The user has
			   to decide what to do. Let's inform him/her that
			   a new call is ready to be accepted or rejected */
			r2chan->call_state = OR2_CALL_OFFERED;
			EMI(r2chan)->on_call_offered(r2chan, r2chan->caller_ani_is_restricted ? NULL : r2chan->ani, r2chan->dnis, tone2category(r2chan));
			break;
		default:
			handle_protocol_error(r2chan, OR2_INVALID_MF_STATE);
			break;
		}
		break;
	/* Viva Mexico Cabrones!, solo Mexico tiene Grupo C ;)  */
	/* Group C is only for Mexico */
	case OR2_MF_GC:
		/* at this point, we either sent a category request, 
		   usually preceding the ANI request or we already sent
		   an ANI request. Anything else, is protocol error */
		switch (r2chan->mf_state) {
		/* we requested the calling party category */
		case OR2_MF_CATEGORY_RQ_TXD:
			r2chan->caller_category = tone;
			if (r2chan->r2context->max_ani > 0) {
				mf_receive_expected_ani(r2chan, 0);
			} else {
				/* switch to Group B/II, we're ready to answer! */
				request_change_to_g2(r2chan);
			}
			break;
		/* we requested more ANI */
		case OR2_MF_ANI_RQ_TXD:
			mf_receive_expected_ani(r2chan, tone);
			break;
		/* yikes, we have an error! */
		default:
			handle_protocol_error(r2chan, OR2_INVALID_MF_STATE);
			break;
		}
		break;
	default:
		handle_protocol_error(r2chan, OR2_INVALID_MF_GROUP);
		break;
	}
}

static void ready_to_answer(openr2_chan_t *r2chan)
{
	OR2_CHAN_STACK;
	/* mode not important here, the BACKWARD side accepted the call so they already know that. 
	   (we could save the tone they used to accept and pass the call type) */
	EMI(r2chan)->on_call_accepted(r2chan, OR2_CALL_UNKNOWN);
}

static void handle_forward_mf_silence(openr2_chan_t *r2chan)
{
	OR2_CHAN_STACK;
	/* we just got silence. Let's silence our side as well.
	   depending on the MF hooks implementation this may be
	   immediate or not */
	set_silence(r2chan);
	switch (r2chan->mf_group) {
	case OR2_MF_GA:
		switch (r2chan->mf_state) {
			/* a bypass of the change to B/II signals has been setted, 
			   proceed to accept the call */
			case OR2_MF_ACCEPTED_TXD:
				turn_off_mf_engine(r2chan);
				r2chan->call_state = OR2_CALL_ACCEPTED;
				r2chan->timer_ids.r2_answer_delay = openr2_chan_add_timer(r2chan, TIMER(r2chan).r2_answer_delay, 
						                                          ready_to_answer, "r2_answer_delay");
				break;
			default:
				/* no further action required. The other end should 
				   handle our previous request */
				break;
		}
		break;
	case OR2_MF_GB:
		switch (r2chan->mf_state) {
		case OR2_MF_CHG_GII_TXD:
			/* no further action required */
			break;
		case OR2_MF_ACCEPTED_TXD:
			/* MF dance has ended. The call has not been answered
			   but the user must decide when to answer. We don't
			   notify immediately because we still don't stop our
			   tone. I tried waiting for ZT_IOMUX_WRITEEMPTY event
			   but even then, it seems sometimes the other end requires
			   a bit of time to detect our tone off condition. If we
			   notify the user of the call accepted and he tries to
			   answer, setting the CAS bits before the other end
			   detects our tone off condition can lead to the other
			   end to never really detect our CAS answer state or
			   consider it a protocol error */
			turn_off_mf_engine(r2chan);
			r2chan->call_state = OR2_CALL_ACCEPTED;
			r2chan->timer_ids.r2_answer_delay = openr2_chan_add_timer(r2chan, TIMER(r2chan).r2_answer_delay, 
					                                          ready_to_answer, "r2_answer_delay");
			break;
		case OR2_MF_DISCONNECT_TXD:	
			/* we did not accept the call and sent some disconnect tone 
			   (busy, network congestion etc). The other end will take care
			   of clearing up the call.  */
			openr2_chan_cancel_all_timers(r2chan);
			break;
		default:
			handle_protocol_error(r2chan, OR2_INVALID_MF_STATE);
			break;
		}
		break;
	case OR2_MF_GC:
		/* no further action required. The other end should 
		   handle our previous request */
		break;
	default:
		handle_protocol_error(r2chan, OR2_INVALID_MF_GROUP);
	}
}

static void handle_backward_mf_tone(openr2_chan_t *r2chan, int tone)
{
	OR2_CHAN_STACK;
	/* if we had a safety timer, clean it up */
	openr2_chan_cancel_timer(r2chan, &r2chan->timer_ids.mf_fwd_safety);

	/* we are the forward side, each time we receive a tone from the
	   backward side, we just mute our tone */
	set_silence(r2chan);
}

static void mf_send_category(openr2_chan_t *r2chan)
{
	OR2_CHAN_STACK;
	r2chan->mf_state = OR2_MF_CATEGORY_TXD;
	r2chan->category_sent = 1;
	openr2_log(r2chan, OR2_LOG_DEBUG, "Sending category %s\n", 
			openr2_proto_get_category_string(tone2category(r2chan)));
	prepare_mf_tone(r2chan, r2chan->caller_category);
}

static void mf_send_ani(openr2_chan_t *r2chan)
{
	/* TODO: Handle sending of previous ANI digits */
	OR2_CHAN_STACK;
	/* if the pointer to ANI is null, that means the caller ANI is restricted */
	if (NULL == r2chan->ani_ptr) {
		openr2_log(r2chan, OR2_LOG_DEBUG, "Sending Restricted ANI\n");
		r2chan->mf_state = OR2_MF_ANI_END_TXD;
		prepare_mf_tone(r2chan, GI_TONE(r2chan).caller_ani_is_restricted);
	/* ok, ANI is not restricted, let's see 
	   if there are still some ANI to send out */
	} else if (*r2chan->ani_ptr) {
		openr2_log(r2chan, OR2_LOG_DEBUG, "Sending ANI digit %c\n", *r2chan->ani_ptr);
		r2chan->mf_state = OR2_MF_ANI_TXD;
		prepare_mf_tone(r2chan, *r2chan->ani_ptr);
		r2chan->ani_ptr++;
	/* if no more ANI, and there is a signal for it, use it */
	} else if (GI_TONE(r2chan).no_more_ani_available) {
		openr2_log(r2chan, OR2_LOG_DEBUG, "Sending more ANI unavailable\n");
		r2chan->mf_state = OR2_MF_ANI_END_TXD;
		prepare_mf_tone(r2chan, GI_TONE(r2chan).no_more_ani_available);
	} else {
		openr2_log(r2chan, OR2_LOG_DEBUG, "No more ANI, expecting timeout from the other side\n");
		/* the callee should timeout to detect end of ANI and
		   resume the MF signaling */
		r2chan->mf_state = OR2_MF_WAITING_TIMEOUT;
		/* even when we are waiting the other end to timeout we
		   cannot wait forever, put a timer to make sure of that */
		r2chan->timer_ids.mf_fwd_safety = openr2_chan_add_timer(r2chan, TIMER(r2chan).mf_fwd_safety, mf_fwd_safety_timeout_expired, "mf_fwd_safety");
	}
}

static void r2_answer_timeout_expired(openr2_chan_t *r2chan)
{
	OR2_CHAN_STACK;
	report_call_disconnection(r2chan, OR2_CAUSE_NO_ANSWER);
}

static openr2_call_mode_t get_mode_from_tone(openr2_chan_t *r2chan, int tone)
{
	OR2_CHAN_STACK;
	if (tone == GB_TONE(r2chan).accept_call_with_charge) {
		return OR2_CALL_WITH_CHARGE;
	} else if (GB_TONE(r2chan).accept_call_no_charge) {
		return OR2_CALL_NO_CHARGE;
	} else if (GB_TONE(r2chan).special_info_tone) {
		return OR2_CALL_SPECIAL;
	} else {
		openr2_log(r2chan, OR2_LOG_WARNING, "Unknown call type\n");
		return OR2_CALL_UNKNOWN;
	}	
}

static void handle_accept_tone(openr2_chan_t *r2chan, openr2_call_mode_t mode)
{
	OR2_CHAN_STACK;
	openr2_mf_state_t previous_mf_state;
	openr2_call_state_t previous_call_state;
        if (r2chan->r2_state == OR2_ANSWER_RXD_MF_PENDING) {
                /* they answered before we even detected they accepted,
                   lets just call on_call_accepted and immediately
                   on_call_answered */

                /* first accepted */
		previous_mf_state = r2chan->mf_state;
		previous_call_state = r2chan->call_state;
                r2chan->r2_state = OR2_ACCEPT_RXD;
                EMI(r2chan)->on_call_accepted(r2chan, mode);

		/* if the on_call_accepted callback calls some openr2 API
		   it can change the state and we no longer want to continue answering */
		if (r2chan->r2_state != OR2_ACCEPT_RXD 
		    || r2chan->mf_state != previous_mf_state
		    || r2chan->call_state != previous_call_state) {
			openr2_log(r2chan, OR2_LOG_NOTICE, "Not proceeding with ANSWERED callback\n");
			return;
		}
                /* now answered */
                openr2_chan_cancel_timer(r2chan, &r2chan->timer_ids.r2_answer);
                r2chan->r2_state = OR2_ANSWER_RXD;
                r2chan->call_state = OR2_CALL_ANSWERED;
		turn_off_mf_engine(r2chan);
                r2chan->answered = 1;
                EMI(r2chan)->on_call_answered(r2chan);
        } else {
                /* They have accepted the call. We do nothing but
                   wait for answer. */
                r2chan->r2_state = OR2_ACCEPT_RXD;
                r2chan->timer_ids.r2_answer = openr2_chan_add_timer(r2chan, TIMER(r2chan).r2_answer, 
				                                    r2_answer_timeout_expired, "r2_answer");
                EMI(r2chan)->on_call_accepted(r2chan, mode);
        }
}

static int handle_dnis_request(openr2_chan_t *r2chan, int tone)
{
	OR2_CHAN_STACK;
	if (tone == GA_TONE(r2chan).request_next_dnis_digit) {
		mf_send_dnis(r2chan, 1);
		return 1;
	} else if (tone == GA_TONE(r2chan).request_dnis_minus_1) {
		mf_send_dnis(r2chan, -1);
		return 1;
	} else if (tone == GA_TONE(r2chan).request_dnis_minus_2) {
		mf_send_dnis(r2chan, -2);
		return 1;
	} else if (tone == GA_TONE(r2chan).request_dnis_minus_3){
		mf_send_dnis(r2chan, -3);
		return 1;
	} else if (tone == GA_TONE(r2chan).request_all_dnis_again) {
		r2chan->dnis_index = 0;
		mf_send_dnis(r2chan, 0);
		return 1;
	}
	return 0;
}

static void handle_group_a_request(openr2_chan_t *r2chan, int tone)
{
	OR2_CHAN_STACK;
	openr2_mf_tone_t request_category_tone = GA_TONE(r2chan).request_category ?
						 GA_TONE(r2chan).request_category :
						 GA_TONE(r2chan).request_category_and_change_to_gc;
	if (handle_dnis_request(r2chan, tone)) {
		openr2_log(r2chan, OR2_LOG_DEBUG, "Group A DNIS request handled\n");
	} else if (r2chan->category_sent && (tone == GA_TONE(r2chan).request_next_ani_digit)) {
		mf_send_ani(r2chan);
	} else if (tone == request_category_tone) {
		if (request_category_tone == GA_TONE(r2chan).request_category_and_change_to_gc) {
			r2chan->mf_group = OR2_MF_GIII;
		}
		mf_send_category(r2chan);
	} else if (tone == GA_TONE(r2chan).request_change_to_g2) {
		r2chan->mf_group = OR2_MF_GII;
		mf_send_category(r2chan);
        } else if (tone == GA_TONE(r2chan).address_complete_charge_setup) {
		handle_accept_tone(r2chan, OR2_CALL_WITH_CHARGE);
	} else if (tone == GA_TONE(r2chan).network_congestion) {
		r2chan->r2_state = OR2_CLEAR_BACK_TONE_RXD;
		report_call_disconnection(r2chan, OR2_CAUSE_NETWORK_CONGESTION);
	} else {
		handle_protocol_error(r2chan, OR2_INVALID_MF_TONE);
	}
}

static void handle_group_c_request(openr2_chan_t *r2chan, int tone)
{
	OR2_CHAN_STACK;
	if (tone == GC_TONE(r2chan).request_next_ani_digit) {
		mf_send_ani(r2chan);
	} else if (tone == GC_TONE(r2chan).request_change_to_g2) {
		/* requesting change to Group II means we should
		   send the calling party category again?  */
		r2chan->mf_group = OR2_MF_GII;
		mf_send_category(r2chan);
	} else if (tone == GC_TONE(r2chan).request_next_dnis_digit_and_change_to_ga) {
		r2chan->mf_group = OR2_MF_GI;
		mf_send_dnis(r2chan, 1);
	} else {
		handle_protocol_error(r2chan, OR2_INVALID_MF_TONE);
	}
}

static void handle_group_b_request(openr2_chan_t *r2chan, int tone)
{
	OR2_CHAN_STACK;
	if (tone == GB_TONE(r2chan).accept_call_with_charge 
	    || tone == GB_TONE(r2chan).accept_call_no_charge
	    || tone == GB_TONE(r2chan).special_info_tone) {
	    handle_accept_tone(r2chan, get_mode_from_tone(r2chan, tone));
	} else if (tone == GB_TONE(r2chan).busy_number){
		r2chan->r2_state = OR2_CLEAR_BACK_TONE_RXD;
		report_call_disconnection(r2chan, OR2_CAUSE_BUSY_NUMBER);
	} else if (tone == GB_TONE(r2chan).network_congestion) {
		r2chan->r2_state = OR2_CLEAR_BACK_TONE_RXD;
		report_call_disconnection(r2chan, OR2_CAUSE_NETWORK_CONGESTION);
	} else if (tone == GB_TONE(r2chan).unallocated_number) {
		r2chan->r2_state = OR2_CLEAR_BACK_TONE_RXD;
		report_call_disconnection(r2chan, OR2_CAUSE_UNALLOCATED_NUMBER);
	} else if (tone == GB_TONE(r2chan).number_changed) {
		r2chan->r2_state = OR2_CLEAR_BACK_TONE_RXD;
		report_call_disconnection(r2chan, OR2_CAUSE_NUMBER_CHANGED);
	} else if (tone == GB_TONE(r2chan).line_out_of_order) {
		r2chan->r2_state = OR2_CLEAR_BACK_TONE_RXD;
		report_call_disconnection(r2chan, OR2_CAUSE_OUT_OF_ORDER);
	} else if (tone == GB_TONE(r2chan).reject_collect_call) {
		r2chan->r2_state = OR2_CLEAR_BACK_TONE_RXD;
		report_call_disconnection(r2chan, OR2_CAUSE_COLLECT_CALL_REJECTED);
	} else {
		handle_protocol_error(r2chan, OR2_INVALID_MF_TONE);
	}
}

static void handle_backward_mf_silence(openr2_chan_t *r2chan, int tone)
{
	OR2_CHAN_STACK;
	/* the backward side has muted its tone, it is time to take
	   action depending on the tone they sent */
	switch (r2chan->mf_group) {
	case OR2_MF_GI:
		handle_group_a_request(r2chan, tone);
		break;
	case OR2_MF_GII:
		handle_group_b_request(r2chan, tone);
		break;
	case OR2_MF_GIII:
		handle_group_c_request(r2chan, tone);
		break;
	default:
		handle_protocol_error(r2chan, OR2_INVALID_MF_GROUP);
		break;
	}
}

static int timediff(struct timeval *t1, struct timeval *t2)
{
	int msdiff = 0;
	if (t1->tv_sec == t2->tv_sec) {
		return ((t1->tv_usec - t2->tv_usec)/1000);
	}
	msdiff  = (t1->tv_usec) ? (t1->tv_usec/1000)          : 0;
	msdiff += (t2->tv_usec) ? (1000 - (t2->tv_usec/1000)) : 0;
	return msdiff;
}

static int check_threshold(openr2_chan_t *r2chan, int tone)
{
	OR2_CHAN_STACK;
	int res = 0;
	int tone_threshold = 0;
	struct timeval currtime = {0, 0};
	if (r2chan->r2context->mf_threshold) {
		if (r2chan->mf_threshold_tone != tone) {
			res = gettimeofday(&r2chan->mf_threshold_time, NULL);
			if (-1 == res) {
				openr2_log(r2chan, OR2_LOG_ERROR, "gettimeofday failed when setting threshold time\n");
				return -1;
			}
			r2chan->mf_threshold_tone = tone;
		}
		res = gettimeofday(&currtime, NULL);
		if (-1 == res) {
			openr2_log(r2chan, OR2_LOG_ERROR, "gettimeofday failed when checking tone length\n");
			return -1;
		}
		tone_threshold = timediff(&currtime, &r2chan->mf_threshold_time);
		if (tone_threshold < r2chan->r2context->mf_threshold) {
			if (tone) {
				openr2_log(r2chan, OR2_LOG_EX_DEBUG, "Tone %c ignored\n", tone);
			} else {
				openr2_log(r2chan, OR2_LOG_EX_DEBUG, "Tone off ignored\n");
			}
			return -1;
		}
	}
	return 0;
}

void openr2_proto_handle_mf_tone(openr2_chan_t *r2chan, int tone)
{
	OR2_CHAN_STACK;
	if (tone) {

		/* since we get a continuous tone, this tone might be one that we
		   already handled but since the other end has not detected our tone
		   has not stopped its own tone. If it is the same
		   just ignore it, we already handled it */
		if (r2chan->mf_read_tone == tone) {
			return;
		}

		/* safety check. Each rx tone should be muted before changing the tone
		   hence the read tone right now should be 0 */
		if (r2chan->mf_read_tone != 0) {
			openr2_log(r2chan, OR2_LOG_ERROR, "Broken MF sequence got %c but never got tone off for tone %c!\n", tone, r2chan->mf_read_tone);
			handle_protocol_error(r2chan, OR2_BROKEN_MF_SEQUENCE);
			return;
		}

		/* do threshold checking if enabled */
		if (check_threshold(r2chan, tone)) {
			return;
		}

		openr2_log(r2chan, OR2_LOG_MF_TRACE, "MF Rx << %c [ON]\n", tone);
		r2chan->mf_read_tone = tone;

		/* handle the incoming MF tone */
		if ( OR2_DIR_BACKWARD == r2chan->direction ) {
			handle_forward_mf_tone(r2chan, tone);
		} else if ( OR2_DIR_FORWARD == r2chan->direction ) {
			handle_backward_mf_tone(r2chan, tone);
		} else {
			openr2_log(r2chan, OR2_LOG_ERROR, "BUG: invalid direction of R2 channel\n");
			handle_protocol_error(r2chan, OR2_LIBRARY_BUG);
		}

	} else {

		/* If we already detected the silence condition, ignore this one */
		if (0 == r2chan->mf_read_tone) {
			return;
		}
		if (check_threshold(r2chan, 0)) {
			return;
		}
		/* handle the silence condition */
		openr2_log(r2chan, OR2_LOG_MF_TRACE, "MF Rx << %c [OFF]\n", r2chan->mf_read_tone);
		if (OR2_DIR_BACKWARD == r2chan->direction) {
			handle_forward_mf_silence(r2chan);
		} else if (OR2_DIR_FORWARD == r2chan->direction) {
			/* when we are in forward we take action when the other side
			   silence its tone, not when receiving the tone */
			handle_backward_mf_silence(r2chan, r2chan->mf_read_tone);
		} else {
			openr2_log(r2chan, OR2_LOG_ERROR, "BUG: invalid direction of R2 channel\n");
			handle_protocol_error(r2chan, OR2_LIBRARY_BUG);
		}
		r2chan->mf_read_tone = 0;

	}
}

static void seize_timeout_expired(openr2_chan_t *r2chan)
{
	OR2_CHAN_STACK;
	openr2_log(r2chan, OR2_LOG_WARNING, "Seize Timeout Expired!\n");
	handle_protocol_error(r2chan, OR2_SEIZE_TIMEOUT);
}

static void dtmf_seize_expired(openr2_chan_t *r2chan)
{
	OR2_CHAN_STACK;
	openr2_log(r2chan, OR2_LOG_NOTICE, "Dialing %s with DTMF/R2 (tone on = %d, tone off = %d)\n", 
			r2chan->dnis, r2chan->r2context->dtmf_on, r2chan->r2context->dtmf_off);
	r2chan->dialing_dtmf = 1;
	r2chan->mf_state = OR2_MF_DIALING_DTMF;
}

int openr2_proto_make_call(openr2_chan_t *r2chan, const char *ani, const char *dnis, openr2_calling_party_category_t category)
{
	OR2_CHAN_STACK;
	const char *digit;
	int copy_ani = 1, copy_dnis = 1;
	openr2_log(r2chan, OR2_LOG_DEBUG, "Attempting to make call (ANI=%s, DNIS=%s, category=%s)\n", ani ? ani : "(restricted)", 
			dnis, openr2_proto_get_category_string(category));
	/* we can dial only if we're in IDLE */
	if (r2chan->call_state != OR2_CALL_IDLE) {
		openr2_log(r2chan, OR2_LOG_ERROR, "Call state should be IDLE but is '%s'\n", openr2_proto_get_call_state_string(r2chan));
		return -1;
	}
	/* try to handle last minute changes if any. 
	   This will detect IDLE lines if the last time the user checked
	   it was in some other state */
	openr2_proto_handle_cas(r2chan);
	if (r2chan->cas_read != r2chan->r2context->cas_signals[OR2_CAS_IDLE]) {
		openr2_log(r2chan, OR2_LOG_ERROR, "Trying to dial out in a non-idle channel\n");
		return -1;
	}
	/* make sure both ANI and DNIS are numeric */
	if (ani) {
		digit = ani;
		while (*digit) {
			if (!isdigit(*digit)) {
				openr2_log(r2chan, OR2_LOG_NOTICE, "Char '%c' is not a digit, ANI will be restricted.\n", *digit);	
				copy_ani = 0;
				ani = NULL;
				break;
			}
			digit++;
		}
	} else {
		copy_ani = 0;
	}	
	digit = dnis;
	while (*digit) {
		if (!isdigit(*digit)) {
			openr2_log(r2chan, OR2_LOG_NOTICE, "Char '%c' is not a digit, DNIS will not be sent.\n", *digit);	
			copy_dnis = 0;
			break;
			/* should we proceed with the call without DNIS? */
		}
		digit++;
	}
	if (r2chan->call_files) {
		open_logfile(r2chan, 0);
	}
	if (set_cas_signal(r2chan, OR2_CAS_SEIZE)) {
		openr2_log(r2chan, OR2_LOG_ERROR, "Failed to seize line!, cannot make a call!\n");
		return -1;
	}
	/* cannot wait forever for seize ack, put a timer */
	r2chan->timer_ids.r2_seize = openr2_chan_add_timer(r2chan, TIMER(r2chan).r2_seize, seize_timeout_expired, "r2_seize");
	if (copy_ani) {
		strncpy(r2chan->ani, ani, sizeof(r2chan->ani)-1);
		r2chan->ani[sizeof(r2chan->ani)-1] = '\0';
	} else {
		r2chan->ani[0] = '\0';
	}
	r2chan->ani_ptr = ani ? r2chan->ani : NULL;
	if (copy_dnis) {
		strncpy(r2chan->dnis, dnis, sizeof(r2chan->dnis)-1);
		r2chan->dnis[sizeof(r2chan->dnis)-1] = '\0';
	} else {
		r2chan->dnis[0] = '\0';
	}	
	r2chan->dnis_index = 0;
	r2chan->call_state = OR2_CALL_DIALING;
	r2chan->direction = OR2_DIR_FORWARD;
	r2chan->caller_category = category2tone(r2chan, category);
	if (!r2chan->r2context->dial_with_dtmf) {
		r2chan->r2_state = OR2_SEIZE_TXD;
		r2chan->mf_group = OR2_MF_DTMF_FWD_INIT;
	} else {
		r2chan->r2_state = OR2_SEIZE_IN_DTMF_TXD;
		r2chan->mf_group = OR2_MF_DTMF_FWD_INIT;
		if (!DTMF(r2chan)->dtmf_tx_init(r2chan->dtmf_write_handle)) {
			openr2_log(r2chan, OR2_LOG_ERROR, "Failed to initialize DTMF transmitter, cannot make call!!\n");
			return -1;
		}
		DTMF(r2chan)->dtmf_tx_set_timing(r2chan->dtmf_write_handle, r2chan->r2context->dtmf_on, r2chan->r2context->dtmf_off);
		if (DTMF(r2chan)->dtmf_tx_put(r2chan->dtmf_write_handle, r2chan->dnis, -1)) {
			openr2_log(r2chan, OR2_LOG_ERROR, "Failed to initialize DTMF transmit queue, cannot make call!!\n");
			return -1;
		}
		/* put just a small timer to start dialing */
		r2chan->timer_ids.dtmf_start_dial = openr2_chan_add_timer(r2chan, 
				TIMER(r2chan).dtmf_start_dial, dtmf_seize_expired, "dtmf_start_dial");
	}
	return 0;
}

void openr2_proto_handle_dtmf_end(openr2_chan_t *r2chan)
{
	OR2_CHAN_STACK;
	turn_off_mf_engine(r2chan);
}

static void send_disconnect(openr2_chan_t *r2chan, openr2_call_disconnect_cause_t cause)
{
	int tone;
	/* TODO: should we verify that the tone exists? (ie not OR2_MF_TONE_INVALID)? 
	   and use default line out of order if not or better yet, warning about it?
	   particularly I am thinking on the case where collect calls do not apply on
	   some countries and the user may still use OR2_CAUSE_COLLECT_CALL_REJECTED, silly,
	   but could confuse users */
	r2chan->mf_state = OR2_MF_DISCONNECT_TXD;
	switch (cause) {
	case OR2_CAUSE_BUSY_NUMBER:
		tone = GB_TONE(r2chan).busy_number;
		break;
	case OR2_CAUSE_NETWORK_CONGESTION:
		tone = GB_TONE(r2chan).network_congestion;
		break;
	case OR2_CAUSE_UNALLOCATED_NUMBER:
		tone = GB_TONE(r2chan).unallocated_number;
		break;
	case OR2_CAUSE_NUMBER_CHANGED:
		tone = GB_TONE(r2chan).number_changed ? GB_TONE(r2chan).number_changed : GB_TONE(r2chan).unallocated_number;
		break;
	case OR2_CAUSE_OUT_OF_ORDER:
		tone = GB_TONE(r2chan).line_out_of_order;
		break;
	case OR2_CAUSE_COLLECT_CALL_REJECTED:
		tone = GB_TONE(r2chan).reject_collect_call;
		break;
	default:
		tone = GB_TONE(r2chan).line_out_of_order;
		break;
	}
	prepare_mf_tone(r2chan, tone);
}

static int send_clear_forward(openr2_chan_t *r2chan)
{
	OR2_CHAN_STACK;
	r2chan->r2_state = OR2_CLEAR_FWD_TXD;
	turn_off_mf_engine(r2chan);
	return set_cas_signal(r2chan, OR2_CAS_CLEAR_FORWARD);
}

static void dtmf_r2_set_call_down(openr2_chan_t *r2chan)
{
	report_call_end(r2chan);
}

/* BUG BUG BUG: As of now, when the call is in OR2_CALL_OFFERED state, the user has to call
   openr2_chan_disconnect_call to reject a call with a reason, this will cause a MF tone to
   be sent to the forward side to let them know we are rejecting the call, at that moment
   the R2 state machine is at OR2_SEIZE_ACK_TXD, given that we did not send ANSWER signal yet,
   the forward end then will set the signal OR2_CAS_CLEAR_FORWARD which we will receive and
   then call report_call_disconnection, where users are expected to call openr2_chan_disconnect_call 
   again!. No much harm done, I think, but is odd and not consistent, we should fix it. 
   The report_call_disconnection callback should be probably only called when the user did not requested
   the call disconnection, if the user was the one requesting the call disconnection then just report_call_end
   should be called. */
int openr2_proto_disconnect_call(openr2_chan_t *r2chan, openr2_call_disconnect_cause_t cause)
{
	OR2_CHAN_STACK;
	/* cannot drop a call when there is none to drop */
	if (r2chan->call_state == OR2_CALL_IDLE) {
		return -1;
	}
	if (r2chan->direction == OR2_DIR_BACKWARD) {
		if (r2chan->call_state == OR2_CALL_OFFERED) {
			/* if the call has been offered we need to give a reason 
			   to disconnect using a MF tone. That should make the other
			   end send us a clear forward  */
			send_disconnect(r2chan, cause);
		} else if (r2chan->r2_state == OR2_CLEAR_FWD_RXD){
			/* if the user want to hangup the call and the other end
			   already said they want too, then just report the call end event */
			report_call_end(r2chan);
		} else {
			if (OR2_CAUSE_FORCED_RELEASE == cause) {
				/* forced release requested */
				if (send_forced_release(r2chan)) {
					openr2_log(r2chan, OR2_LOG_ERROR, "Failed to send Forced Release!, " 
							"cannot disconnect the call nicely!, may be try again?\n");
					return -1;
				}
			} else {
				/* this is a normal clear backward */
				if (send_clear_backward(r2chan)) {
					openr2_log(r2chan, OR2_LOG_ERROR, "Failed to send Clear Backward!, "
							"cannot disconnect call nicely!, may be try again?\n");
					return -1;
				}
			}
		}
	} else {
		/* 
		 * DTMF call clear down is simply going to IDLE (which happens to be the same as clear forward)
		 * however for the sake of clarity we send clear forward and wait 100ms before reporting call end
		 */
		if (r2chan->r2context->dial_with_dtmf) {
			/* TODO: is it worth to configure this timer? */
			openr2_chan_add_timer(r2chan, 100, dtmf_r2_set_call_down, "dtmf_r2_set_call_down");
		} 
		
		if (send_clear_forward(r2chan)) {
			openr2_log(r2chan, OR2_LOG_ERROR, "Failed to send Clear Forward!, cannot disconnect call nicely! may be try again?\n");
			return -1;
		}
	}
	return 0;
}

OR2_EXPORT_SYMBOL
const char *openr2_proto_get_category_string(openr2_calling_party_category_t category)
{
	switch (category) {
	case OR2_CALLING_PARTY_CATEGORY_NATIONAL_SUBSCRIBER:
		return "National Subscriber";
	case OR2_CALLING_PARTY_CATEGORY_NATIONAL_PRIORITY_SUBSCRIBER:
		return "National Priority Subscriber";
	case OR2_CALLING_PARTY_CATEGORY_INTERNATIONAL_SUBSCRIBER:
		return "International Subscriber";
	case OR2_CALLING_PARTY_CATEGORY_INTERNATIONAL_PRIORITY_SUBSCRIBER:
		return "International Priority Subscriber";
	case OR2_CALLING_PARTY_CATEGORY_COLLECT_CALL:
		return "Collect Call";
	default:
		return "*Unknown*";
	}
}

OR2_EXPORT_SYMBOL
openr2_calling_party_category_t openr2_proto_get_category(const char *category)
{
	if (!openr2_strncasecmp(category, "NATIONAL_SUBSCRIBER", sizeof("NATIONAL_SUBSCRIBER")-1)) {
		return OR2_CALLING_PARTY_CATEGORY_NATIONAL_SUBSCRIBER;
	} else if (!openr2_strncasecmp(category, "NATIONAL_PRIORITY_SUBSCRIBER", sizeof("NATIONAL_PRIORITY_SUBSCRIBER")-1)) {
		return OR2_CALLING_PARTY_CATEGORY_NATIONAL_PRIORITY_SUBSCRIBER;
	} else if (!openr2_strncasecmp(category, "INTERNATIONAL_SUBSCRIBER", sizeof("INTERNATIONAL_SUBSCRIBER")-1)) {
		return OR2_CALLING_PARTY_CATEGORY_INTERNATIONAL_SUBSCRIBER;
	} else if (!openr2_strncasecmp(category, "INTERNATIONAL_PRIORITY_SUBSCRIBER", sizeof("INTERNATIONAL_PRIORITY_SUBSCRIBER")-1)) {
		return OR2_CALLING_PARTY_CATEGORY_INTERNATIONAL_PRIORITY_SUBSCRIBER;
	} else if (!openr2_strncasecmp(category, "COLLECT_CALL", sizeof("COLLECT_CALL")-1)) {
		return OR2_CALLING_PARTY_CATEGORY_COLLECT_CALL;
	}	
	return OR2_CALLING_PARTY_CATEGORY_UNKNOWN;
}

OR2_EXPORT_SYMBOL
openr2_variant_t openr2_proto_get_variant(const char *variant_name)
{
	int i;
	int limit = sizeof(r2variants)/sizeof(r2variants[0]);
	for (i = 0; i < limit; i++) {
		if (!openr2_strncasecmp(r2variants[i].name, variant_name, sizeof(r2variants[i].name)-1)) {
			return r2variants[i].id;
		}
	}
	return OR2_VAR_UNKNOWN;
}

OR2_EXPORT_SYMBOL
const char *openr2_proto_get_variant_string(openr2_variant_t variant)
{
	int i;
	int limit = sizeof(r2variants)/sizeof(r2variants[0]);
	for (i = 0; i < limit; i++) {
		if (variant == r2variants[i].id) {
			return r2variants[i].name;
		}
	}
	return "*Unknown*";
}

const char *openr2_proto_get_rx_cas_string(openr2_chan_t *r2chan)
{
	OR2_CHAN_STACK;
	if (r2chan->cas_rx_signal != OR2_CAS_INVALID && !r2chan->inalarm) {
		return cas_names[r2chan->cas_rx_signal];
	}
	/* this is obviously not thread-safe, oh well ... */
	snprintf(r2chan->cas_buff, sizeof(r2chan->cas_buff), "0x%02X", r2chan->cas_read);
	return r2chan->cas_buff;
}

const char *openr2_proto_get_tx_cas_string(openr2_chan_t *r2chan)
{
	OR2_CHAN_STACK;
	return cas_names[r2chan->cas_tx_signal];
}

const char *openr2_proto_get_call_state_string(openr2_chan_t *r2chan)
{
	OR2_CHAN_STACK;
	return callstate2str(r2chan->call_state);
}

const char *openr2_proto_get_r2_state_string(openr2_chan_t *r2chan)
{
	OR2_CHAN_STACK;
	return r2state2str(r2chan->r2_state);
}

const char *openr2_proto_get_mf_state_string(openr2_chan_t *r2chan)
{
	OR2_CHAN_STACK;
	return mfstate2str(r2chan->mf_state);
}

const char *openr2_proto_get_mf_group_string(openr2_chan_t *r2chan)
{
	OR2_CHAN_STACK;
	return mfgroup2str(r2chan->mf_group);
}

OR2_EXPORT_SYMBOL
const char *openr2_proto_get_call_mode_string(openr2_call_mode_t mode)
{
	return get_string_from_mode(mode);
}

int openr2_proto_get_tx_mf_signal(openr2_chan_t *r2chan)
{
	OR2_CHAN_STACK;
	return r2chan->mf_write_tone;
}

int openr2_proto_get_rx_mf_signal(openr2_chan_t *r2chan)
{
	OR2_CHAN_STACK;
	return r2chan->mf_read_tone;
}

OR2_EXPORT_SYMBOL
const openr2_variant_entry_t *openr2_proto_get_variant_list(int *numvariants)
{
	if (!numvariants) {
		return NULL;
	}
	*numvariants = sizeof(r2variants)/sizeof(r2variants[0]);
	return r2variants;
}

