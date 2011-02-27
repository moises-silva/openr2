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

#ifndef _OPENR2_H_
#define _OPENR2_H_

/* Be nice and check for users trying to use openr2 2.x with Asterisk version <= 1.8 */
#ifdef ASTERISK_VERSION_NUM
#if ASTERISK_VERSION_NUM < 11000
#error "You cannot use openr2 2.x with Asterisk < 1.10, please use latest openr2 1.x version"
#endif
#endif

#include <openr2/r2context.h>
#include <openr2/r2chan.h>
#include <openr2/r2proto.h>
#include <openr2/r2log.h>
#include <openr2/r2utils.h>
#include <openr2/r2thread.h>
#include <openr2/r2engine.h>

#endif /* endif defined _OPENR2_H_ */

