/*
 * OpenR2 
 * MFC/R2 call setup library
 *
 * Moises Silva <moy@sangoma.com>
 * Copyright (C) 2009 Moises Silva
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

#ifndef _OPENR2_IO_ABS_H_
#define _OPENR2_IO_ABS_H_

#include <errno.h>
#include "openr2/r2context.h"

#if defined(__cplusplus)
extern "C" {
#endif

#include "r2exports.h"

/* --custom-io (disables verification of presence of either zaptel, dahdi or openzap) 
 * and therefore an I/O interface definition should be provided at runtime
 * otherwise r2 channels cannot be created
 * */
openr2_io_fd_t openr2_io_open(openr2_context_t *r2context, int channo);
int openr2_io_close(openr2_chan_t *r2chan);
int openr2_io_set_cas(openr2_chan_t *r2chan, int cas);
int openr2_io_get_cas(openr2_chan_t *r2chan, int *cas);
int openr2_io_flush_write_buffers(openr2_chan_t *r2chan);
int openr2_io_write(openr2_chan_t *r2chan, const void *buf, int size);
int openr2_io_read(openr2_chan_t *r2chan, const void *buf, int size);
int openr2_io_setup(openr2_chan_t *r2chan);
int openr2_io_wait(openr2_chan_t *r2chan, int *flags, int wait);
int openr2_io_get_oob_event(openr2_chan_t *r2chan, openr2_oob_event_t *event);
openr2_io_interface_t *openr2_io_get_zt_interface(void);

#if defined(__cplusplus)
} /* endif extern "C" */
#endif

#endif /* endif defined _OPENR2_IO_ABS_H_ */
