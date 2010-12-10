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
 * Contributors:
 *
 * Arnaldo Pereira <arnaldo@sangoma.com>
 *
 */

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include "openr2/r2zapcompat.h"
#include "openr2/r2log-pvt.h"
#include "openr2/r2chan-pvt.h"
#include "openr2/r2context-pvt.h"
#include "openr2/r2utils-pvt.h"
#include "openr2/r2ioabs.h"

#define DUMMY_CHAN_RETURN \
	openr2_log(r2chan, OR2_CHANNEL_LOG, OR2_LOG_ERROR, "Please recompile openr2 with DAHDI headers available.\n"); \
	return -1;

static openr2_io_fd_t dummy_open(openr2_context_t *r2context, int channo)
{
	openr2_log2(r2context, OR2_CONTEXT_LOG, OR2_LOG_ERROR, "Please recompile openr2 with DAHDI headers available.\n");
	return NULL;
}

static int dummy_close(openr2_chan_t *r2chan) { DUMMY_CHAN_RETURN }
static int dummy_set_cas(openr2_chan_t *r2chan, int cas) { DUMMY_CHAN_RETURN }
static int dummy_flush_write_buffers(openr2_chan_t *r2chan) { DUMMY_CHAN_RETURN }
static int dummy_get_cas(openr2_chan_t *r2chan, int *cas) { DUMMY_CHAN_RETURN }
static int dummy_read(openr2_chan_t *r2chan, const void *buf, int size) { DUMMY_CHAN_RETURN }
static int dummy_write(openr2_chan_t *r2chan, const void *buf, int size) { DUMMY_CHAN_RETURN }
static int dummy_setup(openr2_chan_t *r2chan) { DUMMY_CHAN_RETURN }
static int dummy_wait(openr2_chan_t *r2chan, int *flags, int wait) { DUMMY_CHAN_RETURN }
static int dummy_get_oob_event(openr2_chan_t *r2chan, openr2_oob_event_t *event) { DUMMY_CHAN_RETURN }

/* dummy io interface */
static openr2_io_interface_t dummy_io_interface = 
{
	/* .open */ dummy_open,
	/* .close */ dummy_close,
	/* .set_cas */ dummy_set_cas,
	/* .get_cas */ dummy_get_cas,
	/* .flush_write_buffers */ dummy_flush_write_buffers,
	/* .write */ dummy_write,
	/* .read */ dummy_read,
	/* .setup */ dummy_setup,
	/* .wait */ dummy_wait,
	/* .get_oob_event */ dummy_get_oob_event
};

openr2_io_interface_t *openr2_io_get_dummy_interface()
{
	return &dummy_io_interface;
}

#ifndef OR2_ZAP_UNAVAILABLE
static openr2_io_fd_t zt_open(openr2_context_t *r2context, int channo)
{
	int chanfd, res;
	/* open the zap generic channel interface */
	chanfd = open(OR2_ZAP_FILE_NAME, O_RDWR | O_NONBLOCK);
	if (-1 == chanfd) {
		r2context->last_error = OR2_LIBERR_SYSCALL_FAILED;
		openr2_log2(r2context, OR2_CONTEXT_LOG, OR2_LOG_ERROR, "Failed to open zap control device (%s)\n", strerror(errno));
		return NULL;
	}

	/* choose the requested channel */
	res = ioctl(chanfd, ZT_SPECIFY, &channo);
	if (res) {
		r2context->last_error = OR2_LIBERR_SYSCALL_FAILED;
		openr2_log2(r2context, OR2_CONTEXT_LOG, OR2_LOG_ERROR, "Failed to choose channel %d (%s)\n", channo, strerror(errno));
		close(chanfd);
		return NULL;
	}
	return (openr2_io_fd_t)(long)chanfd;
}

static int zt_close(openr2_chan_t *r2chan)
{
	int myerrno = 0;
	int fd = (long)r2chan->fd;
	if (close(fd)) {
		myerrno = errno;
		EMI(r2chan)->on_os_error(r2chan, myerrno);
		openr2_log(r2chan, OR2_CHANNEL_LOG, OR2_LOG_ERROR, "Failed to close I/O descriptor: %s\n", strerror(myerrno));
		return -1;
	}
	return 0;
}

static int zt_set_cas(openr2_chan_t *r2chan, int cas)
{
	int myerrno = 0;
	int fd = (long)r2chan->fd;
	if (ioctl(fd, ZT_SETTXBITS, &cas)) {
		myerrno = errno;
		EMI(r2chan)->on_os_error(r2chan, myerrno);
		openr2_log(r2chan, OR2_CHANNEL_LOG, OR2_LOG_ERROR, "Setting CAS bits failed: %s\n", strerror(myerrno));
		return -1;
	}
	return 0;
}

static int zt_flush_write_buffers(openr2_chan_t *r2chan)
{
	int myerrno = 0;
	int flush_write = ZT_FLUSH_WRITE;
	int fd = (long)r2chan->fd;
	if (ioctl(fd, ZT_FLUSH, &flush_write)) {
		myerrno = errno;
		EMI(r2chan)->on_os_error(r2chan, myerrno);
		openr2_log(r2chan, OR2_CHANNEL_LOG, OR2_LOG_ERROR, "Flush write buffer failed: %s\n", strerror(myerrno));
		return -1;
	}
	return 0;
}

static int zt_get_cas(openr2_chan_t *r2chan, int *cas)
{
	int myerrno = 0;
	int fd = (long)r2chan->fd;
	if (ioctl(fd, ZT_GETRXBITS, cas)) {
		myerrno = errno;
		EMI(r2chan)->on_os_error(r2chan, myerrno);
		openr2_log(r2chan, OR2_CHANNEL_LOG, OR2_LOG_ERROR, "Getting CAS bits failed: %s\n", strerror(myerrno));
		return -1;
	}
	return 0;
}

static int zt_read(openr2_chan_t *r2chan, const void *buf, int size)
{
	int myerrno = 0;
	int bytes = -1;
	int fd = (long)r2chan->fd;
	if (-1 == (bytes = read(fd, (void *)buf, size))) {
		myerrno = errno;
		if (myerrno == ELAST) {
			openr2_log(r2chan, OR2_CHANNEL_LOG, OR2_LOG_DEBUG, "read from channel %d returned ELAST, no handling as error since there must be an event taking priority\n", r2chan->number);
			return 0;
		}
		EMI(r2chan)->on_os_error(r2chan, myerrno);
		openr2_log(r2chan, OR2_CHANNEL_LOG, OR2_LOG_ERROR, "Failed to read from channel %d: %s\n", r2chan->number, strerror(myerrno));
		return -1;
	}
	return bytes;
}

static int zt_write(openr2_chan_t *r2chan, const void *buf, int size)
{
	int myerrno = 0;
	int bytes = -1;
	int fd = (long)r2chan->fd;
	if (-1 == (bytes = write(fd, buf, size))) {
		myerrno = errno;
		if (myerrno == ELAST) {
			openr2_log(r2chan, OR2_CHANNEL_LOG, OR2_LOG_DEBUG, "write to channel %d returned ELAST, no handling as error since there must be an event taking priority\n", r2chan->number);
			return 0;
		}
		EMI(r2chan)->on_os_error(r2chan, myerrno);
		openr2_log(r2chan, OR2_CHANNEL_LOG, OR2_LOG_ERROR, "Failed to write to channel %d: %s\n", r2chan->number, strerror(myerrno));
		errno = myerrno;
	}
	return bytes;
}

static int zt_setup(openr2_chan_t *r2chan)
{
	int res, zapval, channo, chanfd;
	unsigned i;
	ZT_GAINS chan_gains;
	ZT_BUFFERINFO chan_buffers;
	ZT_PARAMS chan_params;
	openr2_context_t *r2context = r2chan->r2context;

	chanfd = (int)(long)r2chan->fd;
	res = ioctl(chanfd, ZT_CHANNO, &channo);
	if (res) {
		r2context->last_error = OR2_LIBERR_SYSCALL_FAILED;
		openr2_log2(r2context, OR2_CONTEXT_LOG, OR2_LOG_ERROR, "Failed to get channel number from descriptor %d (%s)\n", chanfd, strerror(errno));
		return -1;
	}

	/* let's check the signaling */
	res = ioctl(chanfd, ZT_GET_PARAMS, &chan_params);
	if (res) {
		r2context->last_error = OR2_LIBERR_SYSCALL_FAILED;
		openr2_log2(r2context, OR2_CONTEXT_LOG, OR2_LOG_ERROR, "Failed to get signaling information for channel %d (%s)\n", channo, strerror(errno));
		return -1;
	}

	if (ZT_SIG_CAS != chan_params.sigtype) {
		r2context->last_error = OR2_LIBERR_INVALID_CHAN_SIGNALING;
		openr2_log2(r2context, OR2_CONTEXT_LOG, OR2_LOG_ERROR, "chan %d does not has CAS signaling\n", channo);
		return -1;
	}

	/* setup buffers and gains  */
	res = ioctl(chanfd, ZT_GET_BUFINFO, &chan_buffers);
	if (res) {
		r2context->last_error = OR2_LIBERR_SYSCALL_FAILED;
		openr2_log2(r2context, OR2_CONTEXT_LOG, OR2_LOG_ERROR, "Failed to retrieve buffer information for chan %d (%s)\n", 
			    channo, strerror(errno));
		return -1;
	}
	chan_buffers.txbufpolicy = ZT_POLICY_IMMEDIATE;
	chan_buffers.rxbufpolicy = ZT_POLICY_IMMEDIATE;
	chan_buffers.numbufs = 4;
	chan_buffers.bufsize = OR2_CHAN_READ_SIZE;
	res = ioctl(chanfd, ZT_SET_BUFINFO, &chan_buffers);
	if (res) {
		r2context->last_error = OR2_LIBERR_SYSCALL_FAILED;
		openr2_log2(r2context, OR2_CONTEXT_LOG, OR2_LOG_ERROR, "Failed to set buffer information for chan %d (%s)\n", channo, strerror(errno));
		return -1;
	}

	chan_gains.chan = 0;
	for (i = 0; i < 256; i++) {
		chan_gains.rxgain[i] = chan_gains.txgain[i] = i;
	}

	res = ioctl(chanfd, ZT_SETGAINS, &chan_gains);
	if (res) {
		r2context->last_error = OR2_LIBERR_SYSCALL_FAILED;
		openr2_log2(r2context, OR2_CONTEXT_LOG, OR2_LOG_ERROR, "Failed to set gains on channel %d (%s)\n", channo, strerror(errno));
		return -1;
	}

	zapval = ZT_LAW_ALAW;
	res = ioctl(chanfd, ZT_SETLAW, &zapval);
	if (res) {
		r2context->last_error = OR2_LIBERR_SYSCALL_FAILED;
		openr2_log2(r2context, OR2_CONTEXT_LOG, OR2_LOG_ERROR, "Failed to set ALAW codec on channel %d (%s)\n", channo, strerror(errno));
		return -1;
	}

	zapval = 0;
	res = ioctl(chanfd, ZT_ECHOCANCEL, &zapval);
	if (res) {
		r2context->last_error = OR2_LIBERR_SYSCALL_FAILED;
		openr2_log2(r2context, OR2_CONTEXT_LOG, OR2_LOG_ERROR, "Failed to put echo-cancel off on channel %d (%s)\n", channo, strerror(errno));
		return -1;
	}
	return 0;
}

static int zt_wait(openr2_chan_t *r2chan, int *flags, int wait)
{
	int res = 0, myerrno = 0, zapflags = 0, fd = 0;
	if (!flags || !*flags) {
		return -1;
	}
	if (!wait) {
		zapflags |= ZT_IOMUX_NOWAIT;
	}
	if (*flags & OR2_IO_READ) {
		zapflags |= ZT_IOMUX_READ;
	}
	if (*flags & OR2_IO_WRITE) {
		zapflags |= ZT_IOMUX_WRITE;
	}
	if (*flags & OR2_IO_OOB_EVENT) {
		zapflags |= ZT_IOMUX_SIGEVENT;
	}
	fd = (int)(long)r2chan->fd;
	res = ioctl(fd, ZT_IOMUX, &zapflags);
	if (res) {
		myerrno = errno;
		openr2_log(r2chan, OR2_CHANNEL_LOG, OR2_LOG_ERROR, "Failed to get I/O events\n");
		EMI(r2chan)->on_os_error(r2chan, myerrno);
		return -1;
	}
	*flags = 0;
	if (zapflags & ZT_IOMUX_SIGEVENT) {
		*flags |= OR2_IO_OOB_EVENT;
	}
	if (zapflags & ZT_IOMUX_READ) {
		*flags |= OR2_IO_READ;
	}
	if (zapflags & ZT_IOMUX_WRITE) {
		*flags |= OR2_IO_WRITE;
	}
	return 0;
}

static int zt_get_oob_event(openr2_chan_t *r2chan, openr2_oob_event_t *event)
{
	int res, zapevent;
	int fd = (long)r2chan->fd;
	if (!event) {
		return -1;
	}
	*event = OR2_OOB_EVENT_NONE;
	res = ioctl(fd, ZT_GETEVENT, &zapevent);
	if (res) {
		return -1;
	}
	if (zapevent == ZT_EVENT_BITSCHANGED) {
		*event = OR2_OOB_EVENT_CAS_CHANGE;
	} else if (zapevent == ZT_EVENT_ALARM) {
		*event = OR2_OOB_EVENT_ALARM_ON;
	} else if (zapevent == ZT_EVENT_NOALARM) {
		*event = OR2_OOB_EVENT_ALARM_OFF;
	}
	return 0;
}

static openr2_io_interface_t zt_io_interface = 
{
	.open = zt_open,
	.close = zt_close,
	.set_cas = zt_set_cas,
	.get_cas = zt_get_cas,
	.flush_write_buffers = zt_flush_write_buffers,
	.write = zt_write,
	.read = zt_read,
	.setup = zt_setup,
	.wait = zt_wait,
	.get_oob_event = zt_get_oob_event
};

openr2_io_interface_t *openr2_io_get_zt_interface()
{
	return &zt_io_interface;
}

#else

openr2_io_interface_t *openr2_io_get_zt_interface()
{
	return NULL;
}

#endif

#define IO(r2chan) int rc = 0; \
	if (!r2chan->r2context->io) {  \
		openr2_log(r2chan, OR2_CHANNEL_LOG, OR2_LOG_ERROR, \
				"%s: Cannot perform I/O operation because no valid I/O interface is available.\n", __FUNCTION__); \
		return -1; \
	} \
	rc = r2chan->r2context->io

openr2_io_fd_t openr2_io_open(openr2_context_t *r2context, int channo)
{
	if (!r2context->io) {
		openr2_log2(r2context, OR2_CONTEXT_LOG, OR2_LOG_ERROR, "Cannot create I/O channel without an I/O interface available.\n"); 
		return NULL;
	}
	return r2context->io->open(r2context, channo);
}

int openr2_io_close(openr2_chan_t *r2chan)
{
	IO(r2chan)->close(r2chan);
	return rc;
}

int openr2_io_set_cas(openr2_chan_t *r2chan, int cas)
{
	IO(r2chan)->set_cas(r2chan, cas);
	return rc;
}

#define CASINTS(cas) ((cas) & (1 << 3)) ? 1 : 0, \
	             ((cas) & (1 << 2)) ? 1 : 0, \
		     ((cas) & (1 << 1)) ? 1 : 0, \
		     ((cas) & (1 << 0)) ? 1 : 0
int openr2_io_get_cas(openr2_chan_t *r2chan, int *cas)
{
	IO(r2chan)->get_cas(r2chan, cas);
	if (!rc) {
		if (*cas != r2chan->cas_raw_read) {
			openr2_log(r2chan, OR2_CHANNEL_LOG, OR2_LOG_DEBUG, "CAS bits changed from %d%d%d%d to %d%d%d%d\n", 
					CASINTS(r2chan->cas_raw_read), CASINTS(*cas));
			r2chan->cas_raw_read = *cas;
		} else {
			openr2_log(r2chan, OR2_CHANNEL_LOG, OR2_LOG_DEBUG, "CAS bits did not change since last read (%d%d%d%d)\n", 
					CASINTS(r2chan->cas_raw_read));
		}
	}
	return rc;
}

int openr2_io_flush_write_buffers(openr2_chan_t *r2chan)
{
	IO(r2chan)->flush_write_buffers(r2chan);
	return rc;
}

int openr2_io_read(openr2_chan_t *r2chan, const void *buf, int size)
{
	IO(r2chan)->read(r2chan, buf, size);
	return rc;
}

int openr2_io_write(openr2_chan_t *r2chan, const void *buf, int size)
{
	IO(r2chan)->write(r2chan, buf, size);
	return rc;
}

int openr2_io_setup(openr2_chan_t *r2chan)
{
	IO(r2chan)->setup(r2chan);
	return rc;
}

int openr2_io_get_oob_event(openr2_chan_t *r2chan, openr2_oob_event_t *event)
{
	IO(r2chan)->get_oob_event(r2chan, event);
	return rc;
}

int openr2_io_wait(openr2_chan_t *r2chan, int *flags, int block)
{
	IO(r2chan)->wait(r2chan, flags, block);
	return rc;
}

