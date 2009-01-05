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

#ifdef __OR2_COMPILING_LIBRARY__
struct openr2_chan_s;
#define openr2_chan_t struct openr2_chan_s
struct openr2_context_s;
#define openr2_context_t struct openr2_context_s
#else
#ifndef OR2_PUBLIC_TYPES_DEFINED
#define OR2_PUBLIC_TYPES_DEFINED
typedef void* openr2_chan_t;
typedef void* openr2_context_t;
#endif
#endif

#ifndef OR2_FD
#define OR2_FD
typedef void* openr2_io_fd_t;
#endif

