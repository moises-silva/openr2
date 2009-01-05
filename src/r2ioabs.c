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
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include "r2hwcompat.h"
#include "r2log-pvt.h"
#include "r2chan-pvt.h"
#include "r2context-pvt.h"
#include "r2utils-pvt.h"
#include "r2ioabs.h"

openr2_io_fd_t openr2_io_open(openr2_context_t *r2context, int channo)
{
	int chanfd, res;
	if (!r2context->io) {
		r2context->last_error = OR2_LIBERR_NO_IO_IFACE_AVAILABLE;
		return NULL;
	}

	/* open the zap generic channel interface */
	chanfd = open(OR2_HW_CHANNEL_FILE_NAME, O_RDWR | O_NONBLOCK);
	if (-1 == chanfd) {
		r2context->last_error = OR2_LIBERR_SYSCALL_FAILED;
		openr2_log2(r2context, OR2_LOG_ERROR, "Failed to open zap control device (%s)\n", strerror(errno));
		return NULL;
	}

	/* choose the requested channel */
	res = ioctl(chanfd, OR2_HW_OP_SPECIFY, &channo);
	if (res) {
		r2context->last_error = OR2_LIBERR_SYSCALL_FAILED;
		openr2_log2(r2context, OR2_LOG_ERROR, "Failed to choose channel %d (%s)\n", channo, strerror(errno));
		close(chanfd);
		return NULL;
	}
	return (openr2_io_fd_t)(long)chanfd;
}

int openr2_io_close(openr2_chan_t *r2chan)
{
	int myerrno = 0;
	if (close((int)(long)r2chan->fd)) {
		myerrno = errno;
		EMI(r2chan)->on_os_error(r2chan, myerrno);
		openr2_log(r2chan, OR2_LOG_ERROR, "Failed to close I/O descriptor: %s\n", strerror(myerrno));
		return -1;
	}
	return 0;
}

int openr2_io_set_cas(openr2_chan_t *r2chan, int cas)
{
	int myerrno = 0;
	if (ioctl((int)(long)r2chan->fd, OR2_HW_OP_SET_TX_BITS, &cas)) {
		myerrno = errno;
		EMI(r2chan)->on_os_error(r2chan, myerrno);
		openr2_log(r2chan, OR2_LOG_ERROR, "Setting CAS bits failed: %s\n", strerror(myerrno));
		return -1;
	}
	return 0;
}

int openr2_io_flush_write_buffers(openr2_chan_t *r2chan)
{
	int myerrno = 0;
	int flush_write = OR2_HW_FLUSH_WRITE;
	if (ioctl((int)(long)r2chan->fd, OR2_HW_OP_FLUSH, &flush_write)) {
		myerrno = errno;
		EMI(r2chan)->on_os_error(r2chan, myerrno);
		openr2_log(r2chan, OR2_LOG_ERROR, "Flush write buffer failed: %s\n", strerror(myerrno));
		return -1;
	}
	return 0;
}

int openr2_io_get_cas(openr2_chan_t *r2chan, int *cas)
{
	int myerrno = 0;
	if (ioctl((int)(long)r2chan->fd, OR2_HW_OP_GET_RX_BITS, cas)) {
		myerrno = errno;
		EMI(r2chan)->on_os_error(r2chan, myerrno);
		openr2_log(r2chan, OR2_LOG_ERROR, "Getting CAS bits failed: %s\n", strerror(myerrno));
		return -1;
	}
	return 0;
}

int openr2_io_read(openr2_chan_t *r2chan, const void *buf, int size)
{
	int myerrno = 0;
	int bytes = -1;
	if ((bytes = read((int)(long)r2chan->fd, (void *)buf, size))) {
		myerrno = errno;
		EMI(r2chan)->on_os_error(r2chan, myerrno);
		openr2_log(r2chan, OR2_LOG_ERROR, "Failed to read: %s\n", strerror(myerrno));
		return -1;
	}
	return bytes;
}

int openr2_io_write(openr2_chan_t *r2chan, const void *buf, int size)
{
	int myerrno = 0;
	int bytes = -1;
	if ((bytes = write((int)(long)r2chan->fd, buf, size))) {
		myerrno = errno;
		EMI(r2chan)->on_os_error(r2chan, myerrno);
		openr2_log(r2chan, OR2_LOG_ERROR, "Failed to write: %s\n", strerror(myerrno));
		errno = myerrno;
	}
	return bytes;
}

int openr2_io_setup(openr2_chan_t *r2chan)
{
	int res, zapval, channo, chanfd;
	unsigned i;
	OR2_HW_GAINS chan_gains;
	OR2_HW_BUFFER_INFO chan_buffers;
	OR2_HW_PARAMS chan_params;
	openr2_context_t *r2context = r2chan->r2context;

	chanfd = (int)(long)r2chan->fd;
	res = ioctl(chanfd, OR2_HW_OP_CHANNO, &channo);
	if (res) {
		r2context->last_error = OR2_LIBERR_SYSCALL_FAILED;
		openr2_log2(r2context, OR2_LOG_ERROR, "Failed to get channel number from descriptor %d (%s)\n", chanfd, strerror(errno));
		return -1;
	}

	/* let's check the signaling */
	res = ioctl(chanfd, OR2_HW_OP_GET_PARAMS, &chan_params);
	if (res) {
		r2context->last_error = OR2_LIBERR_SYSCALL_FAILED;
		openr2_log2(r2context, OR2_LOG_ERROR, "Failed to get signaling information for channel %d (%s)\n", channo, strerror(errno));
		return -1;
	}

	if (OR2_HW_SIG_CAS != chan_params.sigtype) {
		r2context->last_error = OR2_LIBERR_INVALID_CHAN_SIGNALING;
		openr2_log2(r2context, OR2_LOG_ERROR, "chan %d does not has CAS signaling\n", channo);
		return -1;
	}

	/* setup buffers and gains  */
	res = ioctl(chanfd, OR2_HW_OP_GET_BUFINFO, &chan_buffers);
	if (res) {
		r2context->last_error = OR2_LIBERR_SYSCALL_FAILED;
		openr2_log2(r2context, OR2_LOG_ERROR, "Failed to retrieve buffer information for chan %d (%s)\n", 
			    channo, strerror(errno));
		return -1;
	}
	chan_buffers.txbufpolicy = OR2_HW_POLICY_IMMEDIATE;
	chan_buffers.rxbufpolicy = OR2_HW_POLICY_IMMEDIATE;
	chan_buffers.numbufs = 4;
	chan_buffers.bufsize = OR2_CHAN_READ_SIZE;
	res = ioctl(chanfd, OR2_HW_OP_SET_BUFINFO, &chan_buffers);
	if (res) {
		r2context->last_error = OR2_LIBERR_SYSCALL_FAILED;
		openr2_log2(r2context, OR2_LOG_ERROR, "Failed to set buffer information for chan %d (%s)\n", channo, strerror(errno));
		return -1;
	}

	chan_gains.chan = 0;
	for (i = 0; i < 256; i++) {
		chan_gains.rxgain[i] = chan_gains.txgain[i] = i;
	}

	res = ioctl(chanfd, OR2_HW_OP_SET_GAINS, &chan_gains);
	if (res) {
		r2context->last_error = OR2_LIBERR_SYSCALL_FAILED;
		openr2_log2(r2context, OR2_LOG_ERROR, "Failed to set gains on channel %d (%s)\n", channo, strerror(errno));
		return -1;
	}

	zapval = OR2_HW_LAW_ALAW;
	res = ioctl(chanfd, OR2_HW_OP_SET_LAW, &zapval);
	if (res) {
		r2context->last_error = OR2_LIBERR_SYSCALL_FAILED;
		openr2_log2(r2context, OR2_LOG_ERROR, "Failed to set ALAW codec on channel %d (%s)\n", channo, strerror(errno));
		return -1;
	}

	zapval = 0;
	res = ioctl(chanfd, OR2_HW_OP_SET_ECHO_CANCEL, &zapval);
	if (res) {
		r2context->last_error = OR2_LIBERR_SYSCALL_FAILED;
		openr2_log2(r2context, OR2_LOG_ERROR, "Failed to put echo-cancel off on channel %d (%s)\n", channo, strerror(errno));
		return -1;
	}
	r2chan->io_buf_size = OR2_CHAN_READ_SIZE;
	return 0;
}

