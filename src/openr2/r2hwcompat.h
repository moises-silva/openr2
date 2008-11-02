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
#ifndef __OR2_HW_COMPAT_H__
#define __OR2_HW_COMPAT_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* Start checking from newer -> older order */
#if defined (HAVE_DAHDI_USER_H)
#include <dahdi/user.h>
#elif defined (HAVE_ZAPTEL_ZAPTEL_H)
#include <zaptel/zaptel.h>
#elif defined (HAVE_LINUX_ZAPTEL_H)
#include <linux/zaptel.h>
#else
#error "wtf? Some zaptel/dahdi implementation should be present"
#endif

#if defined (HAVE_DAHDI_USER_H)
typedef struct dahdi_gains OR2_HW_GAINS;
typedef struct dahdi_bufferinfo OR2_HW_BUFFER_INFO;
typedef struct dahdi_params OR2_HW_PARAMS;

	#define OR2_HW_POLICY_IMMEDIATE DAHDI_POLICY_IMMEDIATE
	#define OR2_HW_LAW_ALAW DAHDI_LAW_ALAW
	#define OR2_HW_FLUSH_WRITE DAHDI_FLUSH_WRITE

	#define OR2_HW_IO_MUX_READ DAHDI_IOMUX_READ
	#define OR2_HW_IO_MUX_WRITE DAHDI_IOMUX_WRITE
	#define OR2_HW_IO_MUX_SIG_EVENT DAHDI_IOMUX_SIGEVENT
	#define OR2_HW_IO_MUX_NO_WAIT DAHDI_IOMUX_NOWAIT

	#define OR2_HW_EVENT_BITS_CHANGED DAHDI_EVENT_BITSCHANGED
	#define OR2_HW_EVENT_ALARM DAHDI_EVENT_ALARM
	#define OR2_HW_EVENT_NO_ALARM DAHDI_EVENT_NOALARM

	#define OR2_HW_OP_CHANNO DAHDI_CHANNO
	#define OR2_HW_OP_GET_PARAMS DAHDI_GET_PARAMS
	#define OR2_HW_OP_GET_BUFINFO DAHDI_GET_BUFINFO
	#define OR2_HW_OP_SET_BUFINFO DAHDI_SET_BUFINFO
	#define OR2_HW_OP_SET_GAINS DAHDI_SETGAINS
	#define OR2_HW_OP_SET_LAW DAHDI_SETLAW
	#define OR2_HW_OP_SET_ECHO_CANCEL DAHDI_ECHOCANCEL
	#define OR2_HW_OP_SET_TX_BITS DAHDI_SETTXBITS
	#define OR2_HW_OP_GET_RX_BITS DAHDI_GETRXBITS
	#define OR2_HW_OP_FLUSH DAHDI_FLUSH
	#define OR2_HW_OP_SPECIFY DAHDI_SPECIFY
	#define OR2_HW_OP_IO_MUX DAHDI_IOMUX
	#define OR2_HW_OP_GET_EVENT DAHDI_GETEVENT

	#define OR2_HW_SIG_CAS DAHDI_SIG_CAS

	#define OR2_HW_CHANNEL_FILE_NAME "/dev/dahdi/channel"

#elif defined(HAVE_ZAPTEL_ZAPTEL_H) || defined(HAVE_LINUX_ZAPTEL_H)

typedef ZT_GAINS OR2_HW_GAINS;
typedef ZT_BUFFERINFO OR2_HW_BUFFER_INFO;
typedef ZT_PARAMS OR2_HW_PARAMS;

	#define OR2_HW_POLICY_IMMEDIATE ZT_POLICY_IMMEDIATE
	#define OR2_HW_LAW_ALAW ZT_LAW_ALAW
	#define OR2_HW_FLUSH_WRITE ZT_FLUSH_WRITE

	#define OR2_HW_IO_MUX_READ ZT_IOMUX_READ
	#define OR2_HW_IO_MUX_WRITE ZT_IOMUX_WRITE
	#define OR2_HW_IO_MUX_SIG_EVENT ZT_IOMUX_SIGEVENT
	#define OR2_HW_IO_MUX_NO_WAIT ZT_IOMUX_NOWAIT

	#define OR2_HW_EVENT_BITS_CHANGED ZT_EVENT_BITSCHANGED
	#define OR2_HW_EVENT_ALARM ZT_EVENT_ALARM
	#define OR2_HW_EVENT_NO_ALARM ZT_EVENT_NOALARM

	#define OR2_HW_OP_CHANNO ZT_CHANNO
	#define OR2_HW_OP_GET_PARAMS ZT_GET_PARAMS
	#define OR2_HW_OP_GET_BUFINFO ZT_GET_BUFINFO
	#define OR2_HW_OP_SET_BUFINFO ZT_SET_BUFINFO
	#define OR2_HW_OP_SET_GAINS ZT_SETGAINS
	#define OR2_HW_OP_SET_LAW ZT_SETLAW
	#define OR2_HW_OP_SET_ECHO_CANCEL ZT_ECHOCANCEL
	#define OR2_HW_OP_SET_TX_BITS ZT_SETTXBITS
	#define OR2_HW_OP_GET_RX_BITS ZT_GETRXBITS
	#define OR2_HW_OP_FLUSH ZT_FLUSH
	#define OR2_HW_OP_SPECIFY ZT_SPECIFY
	#define OR2_HW_OP_IO_MUX ZT_IOMUX
	#define OR2_HW_OP_GET_EVENT ZT_GETEVENT

	#define OR2_HW_SIG_CAS ZT_SIG_CAS

	#define OR2_HW_CHANNEL_FILE_NAME "/dev/zap/channel"

#endif

#endif /* __OR2_HW_COMPAT_H__ */
