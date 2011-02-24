/*
 * OpenR2 
 * MFC/R2 call setup library
 *
 * r2engine.h - MFC/R2 tone generation and detection.
 *              DTMF tone generation
 *
 * Borrowed and slightly modified from the LGPL SpanDSP library, 
 * Written by Steve Underwood <steveu@coppice.org>
 *
 * Copyright (C) 2001 Steve Underwood
 *
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2.1,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef _OPENR2_ENGINE_H_
#define _OPENR2_ENGINE_H_

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif
#include "openr2/r2declare.h"
#include "openr2/r2context.h" /* just for openr2_digits_rx_callback_t */

#if defined(__cplusplus)
extern "C" {
#endif

#define OR2_ALAW_AMI_MASK 0x55

typedef struct openr2_mf_rx_state openr2_mf_rx_state_t;
typedef struct openr2_mf_tx_state openr2_mf_tx_state_t;
typedef struct openr2_dtmf_tx_state openr2_dtmf_tx_state_t;
typedef struct openr2_dtmf_rx_state openr2_dtmf_rx_state_t;

/* MF Rx routines */
OR2_DECLARE(openr2_mf_rx_state_t) *openr2_mf_rx_init(openr2_mf_rx_state_t *s, int fwd);
OR2_DECLARE(int) openr2_mf_rx(openr2_mf_rx_state_t *s, const int16_t amp[], int samples);

/* MF Tx routines */
OR2_DECLARE(openr2_mf_tx_state_t) *openr2_mf_tx_init(openr2_mf_tx_state_t *s, int fwd);
OR2_DECLARE(int) openr2_mf_tx(openr2_mf_tx_state_t *s, int16_t amp[], int samples);
OR2_DECLARE(int) openr2_mf_tx_put(openr2_mf_tx_state_t *s, char digit);

/* DTMF Tx routines */
OR2_DECLARE(int) openr2_dtmf_tx(openr2_dtmf_tx_state_t *s, int16_t amp[], int max_samples);
OR2_DECLARE(size_t) openr2_dtmf_tx_put(openr2_dtmf_tx_state_t *s, const char *digits, int len);
OR2_DECLARE(void) openr2_dtmf_tx_set_timing(openr2_dtmf_tx_state_t *s, int on_time, int off_time);
OR2_DECLARE(void) openr2_dtmf_tx_set_level(openr2_dtmf_tx_state_t *s, int level, int twist);
OR2_DECLARE(openr2_dtmf_tx_state_t *) openr2_dtmf_tx_init(openr2_dtmf_tx_state_t *s);

/* DTMF Rx routines */
OR2_DECLARE(openr2_dtmf_rx_state_t *) openr2_dtmf_rx_init(openr2_dtmf_rx_state_t *s, openr2_digits_rx_callback_t callback, void *user_data);
OR2_DECLARE(int) openr2_dtmf_rx(openr2_dtmf_rx_state_t *s, const int16_t amp[], int samples);
OR2_DECLARE(int) openr2_dtmf_rx_status(openr2_dtmf_rx_state_t *s);

#if defined(__cplusplus)
}
#endif

#endif /* _OPENR2_ENGINE_H_ */
