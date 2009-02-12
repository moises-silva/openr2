/*
 * OpenR2 
 * MFC/R2 call setup library
 *
 * Moises Silva <moy@sangoma.com>
 * Copyright (C) 2008-2009 Moises Silva
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
#ifndef __OR2_ZAP_COMPAT_H__
#define __OR2_ZAP_COMPAT_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* Start checking in order from newer to older */
#if defined (HAVE_DAHDI_USER_H)
#include <dahdi/user.h>
#elif defined (HAVE_ZAPTEL_ZAPTEL_H)
#include <zaptel/zaptel.h>
#elif defined (HAVE_LINUX_ZAPTEL_H)
#include <linux/zaptel.h>
#else
#define OR2_ZAP_UNAVAILABLE
#endif

#if defined (HAVE_DAHDI_USER_H)
typedef struct dahdi_gains ZT_GAINS;
typedef struct dahdi_bufferinfo ZT_BUFFERINFO;
typedef struct dahdi_params ZT_PARAMS;

	#define ZT_POLICY_IMMEDIATE DAHDI_POLICY_IMMEDIATE
	#define ZT_LAW_ALAW DAHDI_LAW_ALAW
	#define ZT_FLUSH_WRITE DAHDI_FLUSH_WRITE

	#define ZT_IOMUX_READ DAHDI_IOMUX_READ
	#define ZT_IOMUX_WRITE DAHDI_IOMUX_WRITE
	#define ZT_IOMUX_SIGEVENT DAHDI_IOMUX_SIGEVENT
	#define ZT_IOMUX_NOWAIT DAHDI_IOMUX_NOWAIT

	#define ZT_EVENT_BITSCHANGED DAHDI_EVENT_BITSCHANGED
	#define ZT_EVENT_ALARM DAHDI_EVENT_ALARM
	#define ZT_EVENT_NOALARM DAHDI_EVENT_NOALARM

	#define ZT_CHANNO DAHDI_CHANNO
	#define ZT_GET_PARAMS DAHDI_GET_PARAMS
	#define ZT_GET_BUFINFO DAHDI_GET_BUFINFO
	#define ZT_SET_BUFINFO DAHDI_SET_BUFINFO
	#define ZT_SETGAINS DAHDI_SETGAINS
	#define ZT_SETLAW DAHDI_SETLAW
	#define ZT_ECHOCANCEL DAHDI_ECHOCANCEL
	#define ZT_SETTXBITS DAHDI_SETTXBITS
	#define ZT_GETRXBITS DAHDI_GETRXBITS
	#define ZT_FLUSH DAHDI_FLUSH
	#define ZT_SPECIFY DAHDI_SPECIFY
	#define ZT_IOMUX DAHDI_IOMUX
	#define ZT_GETEVENT DAHDI_GETEVENT

	#define ZT_SIG_CAS DAHDI_SIG_CAS

	#define OR2_ZAP_FILE_NAME "/dev/dahdi/channel"

#elif defined(HAVE_ZAPTEL_ZAPTEL_H) || defined(HAVE_LINUX_ZAPTEL_H)

	#define OR2_ZAP_FILE_NAME "/dev/zap/channel"

#endif

#endif /* __OR2_ZAP_COMPAT_H__ */
