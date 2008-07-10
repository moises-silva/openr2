/*
 * OpenR2 
 * MFC/R2 call setup library
 *
 * Moises Silva <moises.silva@gmail.com>
 * Copyright (C) Moises Silva
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

#include <sys/types.h> /* mode_t */

/* quick access to context Multi Frequency Interface */
#define MFI(r2chan) (r2chan)->r2context->mflib

/* quick access to context Event Management Interface */
#define EMI(r2chan) (r2chan)->r2context->evmanager

/* quick access to the Transcoding Interface */
#define TI(r2c) (r2chan)->r2context->transcoder

const char *openr2_get_version();
const char *openr2_get_revision();

int openr2_mkdir_recursive(char *dir, mode_t mode);

