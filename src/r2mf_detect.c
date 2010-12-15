/*
 * OpenR2 
 * MFC/R2 call setup library
 *
 * Moises Silva <moy@sangoma.com>
 * Copyright (C) 2010 Moises Silva
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

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "openr2/openr2.h"
#include "openr2/r2engine.h"

#define FORMAT_INVALID 0 
#define FORMAT_ALAW 1
#define FORMAT_SLINEAR 2 

#define CHUNK_SAMPLES 160

#define USAGE "USAGE: %s [alaw|slinear] [alaw or slinear file path]\n"

int main(int argc, char *argv[])
{
	struct stat statbuf;
	FILE *audiofp;
	short slinear_buffer[CHUNK_SAMPLES];
	char alaw_buffer[CHUNK_SAMPLES];
	char *chunk_buffer;
	size_t chunksize = 0;
	int format = FORMAT_INVALID;
	int i = 0;
	char digit = 0;
	char bwd_currdigit = 0;
	char fwd_currdigit = 0;
	openr2_mf_rx_state_t  fwd_rxstate;
	openr2_mf_rx_state_t  bwd_rxstate;

	printf("Running MF Detection Test - alaw or slinear 8000hz only\n");

	if (argc < 3) {
		fprintf(stderr, USAGE, argv[0]);
		exit(1);
	}

	if (!openr2_strncasecmp(argv[1], "alaw", sizeof("alaw")-1)) {
		format = FORMAT_ALAW;
		chunksize = sizeof(alaw_buffer);
		chunk_buffer = alaw_buffer;
	} else if (!openr2_strncasecmp(argv[1], "slinear", sizeof("slinear")-1)) {
		format = FORMAT_SLINEAR;
		chunksize = sizeof(slinear_buffer);
		chunk_buffer = (char *)slinear_buffer;
	} else {
		fprintf(stderr, USAGE, argv[0]);
		exit(1);
	}

	printf("Using file %s\n", argv[2]);
	if (stat(argv[2], &statbuf)) {
		perror("could not stat audio file");
		exit(1);
	}

	audiofp = fopen(argv[2], "r");
	if (!audiofp) {
		perror("could not open audio file");
		exit(1);
	}

	if (!openr2_mf_rx_init(&bwd_rxstate, 0)) {
		fprintf(stderr, "could not create backward rx state\n");
		exit(1);
	}

	if (!openr2_mf_rx_init(&fwd_rxstate, 1)) {
		fprintf(stderr, "could not create forward rx state\n");
		exit(1);
	}

	while (fread(chunk_buffer, chunksize, 1, audiofp) == 1) {
		if (format == FORMAT_ALAW) {
			/* chunksize == bytes == samples */
			for (i = 0; i < chunksize; i++) {
				slinear_buffer[i] = openr2_alaw_to_linear(chunk_buffer[i]);
			}
		} 

		digit = openr2_mf_rx(&bwd_rxstate, slinear_buffer, CHUNK_SAMPLES);
		if (digit) {
			bwd_currdigit = digit;
			printf("Backward %c ON\n", bwd_currdigit);
		} else if (bwd_currdigit) {
			printf("Backward %c OFF\n", bwd_currdigit);
			bwd_currdigit = 0;
		}

		digit = openr2_mf_rx(&fwd_rxstate, slinear_buffer, CHUNK_SAMPLES);
		if (digit) {
			fwd_currdigit = digit;
			printf("Forward %c ON\n", fwd_currdigit);
		} else if (fwd_currdigit) {
			printf("Forward %c OFF\n", fwd_currdigit);
			fwd_currdigit = 0;
		}

	}

	fclose(audiofp);
	return 0;
}

