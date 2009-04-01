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

#include <inttypes.h>
#include "openr2/queue.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define OR2_ALAW_AMI_MASK 0x55
#define OR2_MAX_DTMF_DIGITS 128

typedef struct
{
    int32_t phase_rate;
    float gain;
} openr2_tone_gen_tone_descriptor_t;

/*!
    Cadenced dual tone generator descriptor.
*/
typedef struct
{
    openr2_tone_gen_tone_descriptor_t tone[4];
    int duration[4];
    int repeat;
} openr2_tone_gen_descriptor_t;

/*!
    Cadenced dual tone generator state descriptor. This defines the state of
    a single working instance of a generator.
*/
typedef struct
{
    openr2_tone_gen_tone_descriptor_t tone[4];

    uint32_t phase[4];
    int duration[4];
    int repeat;

    int current_section;
    int current_position;
} openr2_tone_gen_state_t;

/*!
    Floating point Goertzel filter descriptor.
*/
typedef struct
{
    float fac;
    int samples;
} openr2_goertzel_descriptor_t;

/*!
    Floating point Goertzel filter state descriptor.
*/
typedef struct
{
    float v2;
    float v3;
    float fac;
    int samples;
    int current_sample;
} openr2_goertzel_state_t;

/*!
    MFC/R2 tone generator descriptor.
*/
typedef struct
{
    /*! The tone generator. */
    openr2_tone_gen_state_t tone;
    /*! TRUE if generating forward tones, otherwise generating reverse tones. */
    int fwd;
    /*! The current digit being generated. */
    int digit;
} openr2_mf_tx_state_t;

/*!
    MFC/R2 tone detector descriptor.
*/
typedef struct
{
    /*! TRUE is we are detecting forward tones. FALSE if we are detecting backward tones */
    int fwd;
    /*! Tone detector working states */
    openr2_goertzel_state_t out[6];
    /*! The current sample number within a processing block. */
    int current_sample;
    /*! The currently detected digit. */
    int current_digit;
} openr2_mf_rx_state_t;


/*!
    DTMF generator state descriptor. This defines the state of a single
    working instance of a DTMF generator.
*/
typedef struct
{
    openr2_tone_gen_state_t tones;
    float low_level;
    float high_level;
    int on_time;
    int off_time;
    union
    {
        queue_state_t queue;
        uint8_t buf[QUEUE_STATE_T_SIZE(OR2_MAX_DTMF_DIGITS)];
    } queue;
} openr2_dtmf_tx_state_t;

/* MF Rx routines */
openr2_mf_rx_state_t *openr2_mf_rx_init(openr2_mf_rx_state_t *s, int fwd);
int openr2_mf_rx(openr2_mf_rx_state_t *s, const int16_t amp[], int samples);

/* MF Tx routines */
openr2_mf_tx_state_t *openr2_mf_tx_init(openr2_mf_tx_state_t *s, int fwd);
int openr2_mf_tx(openr2_mf_tx_state_t *s, int16_t amp[], int samples);
int openr2_mf_tx_put(openr2_mf_tx_state_t *s, char digit);

/* DTMF Tx routines */
int openr2_dtmf_tx(openr2_dtmf_tx_state_t *s, int16_t amp[], int max_samples);
size_t openr2_dtmf_tx_put(openr2_dtmf_tx_state_t *s, const char *digits, int len);
void openr2_dtmf_tx_set_timing(openr2_dtmf_tx_state_t *s, int on_time, int off_time);
void openr2_dtmf_tx_set_level(openr2_dtmf_tx_state_t *s, int level, int twist);
openr2_dtmf_tx_state_t *openr2_dtmf_tx_init(openr2_dtmf_tx_state_t *s);

static __inline__ int openr2_top_bit(unsigned int bits)
{
    int res;
    
#if defined(__i386__)  ||  defined(__x86_64__)
    __asm__ (" xorl %[res],%[res];\n"
             " decl %[res];\n"
             " bsrl %[bits],%[res]\n"
             : [res] "=&r" (res)
             : [bits] "rm" (bits));
    return res;
#elif defined(__ppc__)  ||   defined(__powerpc__)
    __asm__ ("cntlzw %[res],%[bits];\n"
             : [res] "=&r" (res)
             : [bits] "r" (bits));
    return 31 - res;
#else
    if (bits == 0)
        return -1;
    res = 0;
    if (bits & 0xFFFF0000)
    {
        bits &= 0xFFFF0000;
        res += 16;
    }
    if (bits & 0xFF00FF00)
    {
        bits &= 0xFF00FF00;
        res += 8;
    }
    if (bits & 0xF0F0F0F0)
    {
        bits &= 0xF0F0F0F0;
        res += 4;
    }
    if (bits & 0xCCCCCCCC)
    {
        bits &= 0xCCCCCCCC;
        res += 2;
    }
    if (bits & 0xAAAAAAAA)
    {
        bits &= 0xAAAAAAAA;
        res += 1;
    }
    return res;
#endif
}

static __inline__ int16_t openr2_alaw_to_linear(uint8_t alaw)
{
    int i;
    int seg;

    alaw ^= OR2_ALAW_AMI_MASK;
    i = ((alaw & 0x0F) << 4);
    seg = (((int) alaw & 0x70) >> 4);
    if (seg)
        i = (i + 0x108) << (seg - 1);
    else
        i += 8;
    return (int16_t) ((alaw & 0x80)  ?  i  :  -i);
}

static __inline__ uint8_t openr2_linear_to_alaw(int linear)
{
    int mask;
    int seg;
    
    if (linear >= 0)
    {
        /* Sign (bit 7) bit = 1 */
        mask = OR2_ALAW_AMI_MASK | 0x80;
    }
    else
    {
        /* Sign (bit 7) bit = 0 */
        mask = OR2_ALAW_AMI_MASK;
        linear = -linear - 1;
    }

    /* Convert the scaled magnitude to segment number. */
    seg = openr2_top_bit(linear | 0xFF) - 7;
    if (seg >= 8)
    {
        if (linear >= 0)
        {
            /* Out of range. Return maximum value. */
            return (uint8_t) (0x7F ^ mask);
        }
        /* We must be just a tiny step below zero */
        return (uint8_t) (0x00 ^ mask);
    }
    /* Combine the sign, segment, and quantization bits. */
    return (uint8_t) (((seg << 4) | ((linear >> ((seg)  ?  (seg + 3)  :  4)) & 0x0F)) ^ mask);
}

#if defined(__cplusplus)
}
#endif

#endif /* _OPENR2_ENGINE_H_ */
