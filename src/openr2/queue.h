/*
 * OpenR2
 * MFC/R2 call setup library
 *
 * queue.c - simple in-process message queuing
 *
 * Borrowed and slightly modified from the LGPL SpanDSP  library,
 * Written by Steve Underwood <steveu@coppice.org>
 *
 * Copyright (C) 2004 Steve Underwood
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

#ifndef _OPENR2_QUEUE_H_
#define _OPENR2_QUEUE_H_

/*! Flag bit to indicate queue reads are atomic operations. This must be set
    if the queue is to be used with the message oriented functions. */
#define QUEUE_READ_ATOMIC   0x0001
/*! Flag bit to indicate queue writes are atomic operations. This must be set
    if the queue is to be used with the message oriented functions. */
#define QUEUE_WRITE_ATOMIC  0x0002

/*!
    Queue descriptor. This defines the working state for a single instance of
    a byte stream or message oriented queue.
*/
typedef struct
{
    /*! \brief Flags indicating the mode of the queue. */
    int flags;
    /*! \brief The length of the data buffer. */
    int len;
    /*! \brief The buffer input pointer. */
    volatile int iptr;
    /*! \brief The buffer output pointer. */
    volatile int optr;
#if defined(FULLY_DEFINE_QUEUE_STATE_T)
    /*! \brief The data buffer, sized at the time the structure is created. */
    uint8_t data[];
#endif
} queue_state_t;

#define QUEUE_STATE_T_SIZE(len) (sizeof(queue_state_t) + len + 1)

#if defined(__cplusplus)
extern "C"
{
#endif

#include <inttypes.h>

/*! Check if a queue is empty.
    \brief Check if a queue is empty.
    \param s The queue context.
    \return TRUE if empty, else FALSE. */
int queue_empty(queue_state_t *s);

/*! Check the available free space in a queue's buffer.
    \brief Check available free space.
    \param s The queue context.
    \return The number of bytes of free space. */
int queue_free_space(queue_state_t *s);

/*! Check the contents of a queue.
    \brief Check the contents of a queue.
    \param s The queue context.
    \return The number of bytes in the queue. */
int queue_contents(queue_state_t *s);

/*! Flush the contents of a queue.
    \brief Flush the contents of a queue.
    \param s The queue context. */
void queue_flush(queue_state_t *s);

/*! Copy bytes from a queue. This is similar to queue_read, but
    the data remains in the queue.
    \brief Copy bytes from a queue.
    \param s The queue context.
    \param buf The buffer into which the bytes will be read.
    \param len The length of the buffer.
    \return the number of bytes returned. */
int queue_view(queue_state_t *s, uint8_t *buf, int len);

/*! Read bytes from a queue.
    \brief Read bytes from a queue.
    \param s The queue context.
    \param buf The buffer into which the bytes will be read.
    \param len The length of the buffer.
    \return the number of bytes returned. */
int queue_read(queue_state_t *s, uint8_t *buf, int len);

/*! Read a byte from a queue.
    \brief Read a byte from a queue.
    \param s The queue context.
    \return the byte, or -1 if the queue is empty. */
int queue_read_byte(queue_state_t *s);

/*! Write bytes to a queue.
    \brief Write bytes to a queue.
    \param s The queue context.
    \param buf The buffer containing the bytes to be written.
    \param len The length of the buffer.
    \return the number of bytes actually written. */
int queue_write(queue_state_t *s, const uint8_t *buf, int len);

/*! Write a byte to a queue.
    \brief Write a byte to a queue.
    \param s The queue context.
    \param byte The byte to be written.
    \return the number of bytes actually written. */
int queue_write_byte(queue_state_t *s, uint8_t byte);

/*! Test the length of the message at the head of a queue.
    \brief Test message length.
    \param s The queue context.
    \return The length of the next message, in byte. If there are
            no messages in the queue, -1 is returned. */
int queue_state_test_msg(queue_state_t *s);

/*! Read a message from a queue. If the message is longer than the buffer
    provided, only the first len bytes of the message will be returned. The
    remainder of the message will be discarded.
    \brief Read a message from a queue.
    \param s The queue context.
    \param buf The buffer into which the message will be read.
    \param len The length of the buffer.
    \return The number of bytes returned. If there are
            no messages in the queue, -1 is returned. */
int queue_read_msg(queue_state_t *s, uint8_t *buf, int len);

/*! Write a message to a queue.
    \brief Write a message to a queue.
    \param s The queue context.
    \param buf The buffer from which the message will be written.
    \param len The length of the message.
    \return The number of bytes actually written. */
int queue_write_msg(queue_state_t *s, const uint8_t *buf, int len);

/*! Initialise a queue.
    \brief Initialise a queue.
    \param s The queue context. If is imperative that the context this
           points to is immediately followed by a buffer of the required
           size + 1 octet.
    \param len The length of the queue's buffer.
    \param flags Flags controlling the operation of the queue.
           Valid flags are QUEUE_READ_ATOMIC and QUEUE_WRITE_ATOMIC.
    \return A pointer to the context if OK, else NULL. */
queue_state_t *queue_init(queue_state_t *s, int len, int flags);

/*! Delete a queue.
    \brief Delete a queue.
    \param s The queue context.
    \return 0 if deleted OK, else -1. */
int queue_free(queue_state_t *s);

#if defined(__cplusplus)
}
#endif

#endif
/*- End of file ------------------------------------------------------------*/
