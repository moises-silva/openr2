/*
 * OpenR2 
 * MFC/R2 call setup library
 *
 * Moises Silva <moises.silva@gmail.com>
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
#include "openr2/r2engine-pvt.h"

#define FORMAT_INVALID 0 
#define FORMAT_ALAW 1
#define FORMAT_SLINEAR 2 

#define CHUNK_MS 20
#define CHUNK_SAMPLES 160

#define USAGE "USAGE: %s [alaw|slinear] [alaw or slinear file path] [tone sequence]\n" \
	      "The tone sequence must come in pairs of <f|b><tone id> <milliseconds>\n" \
              "The tone number goes from 0 to 9 and B to F\n" \
              "The f or b prepended to the tone id determines whether is backward or forward\n" \
              "The special character 's' can be used to indicate silence instead of a tone, " \
	      "in such cases no B or F should be specified\n"

#define ms_to_samples(ms) ms * 8

static volatile int running = 0;

static int valid_tone(int tone) {
	switch (tone) {
	case OR2_MF_TONE_1:
	case OR2_MF_TONE_2:
	case OR2_MF_TONE_3:
	case OR2_MF_TONE_4:
	case OR2_MF_TONE_5:
	case OR2_MF_TONE_6:
	case OR2_MF_TONE_7:
	case OR2_MF_TONE_8:
	case OR2_MF_TONE_9:
	case OR2_MF_TONE_10:
	case OR2_MF_TONE_11:
	case OR2_MF_TONE_12:
	case OR2_MF_TONE_13:
	case OR2_MF_TONE_14:
	case OR2_MF_TONE_15:
		return 1;
	}
	return 0;
}

int main(int argc, char *argv[])
{
	FILE *audiofp;
	short slinear_buffer[CHUNK_SAMPLES];
	char alaw_buffer[CHUNK_SAMPLES];
	int format = FORMAT_INVALID;
	int i = 0;
	int j = 0;
	int res = 0;
	int ms = 0;
	char digit = 0;
	char *dir = NULL;
	char currdigit = 0;
	int total_samples = 0;
	openr2_mf_tx_state_t  fwd_txstate;
	openr2_mf_tx_state_t  bwd_txstate;
	openr2_mf_tx_state_t *txstate = NULL;

	printf("Running MF Generation Test - alaw or slinear 8000hz only\n");

	if (argc < 5) {
		fprintf(stderr, USAGE, argv[0]);
		exit(1);
	}

	if (!openr2_strncasecmp(argv[1], "alaw", sizeof("alaw")-1)) {
		format = FORMAT_ALAW;
	} else if (!openr2_strncasecmp(argv[1], "slinear", sizeof("slinear")-1)) {
		format = FORMAT_SLINEAR;
	} else {
		fprintf(stderr, USAGE, argv[0]);
		exit(1);
	}

	printf("Using file %s\n", argv[2]);
	
	audiofp = fopen(argv[2], "w");
	if (!audiofp) {
		perror("could not open audio file");
		exit(1);
	}

	if (!openr2_mf_tx_init(&bwd_txstate, 0)) {
		fprintf(stderr, "could not create backward rx state\n");
		exit(1);
	}

	if (!openr2_mf_tx_init(&fwd_txstate, 1)) {
		fprintf(stderr, "could not create forward rx state\n");
		exit(1);
	}

	running = 1;
	i = 3;
	for (i = 3; running && argc > i; i++) {
		if (strlen(argv[i]) == 2) {
			if (argv[i][0] == 'f' || argv[i][0] == 'F') {
				txstate = &fwd_txstate;
				dir = (char *)"Forward";
			} else if (argv[i][0] == 'b' || argv[i][0] == 'B') {
				txstate = &bwd_txstate;
				dir = (char *)"Backward";
			} else {
				fprintf(stderr, "Aborting generation due to invalid MF tone direction in element %d (%c)\n", i, argv[i][0]);
				break;
			}

			digit = argv[i][1];
			if (!valid_tone(digit)) {
				fprintf(stderr, "Aborting generation due to invalid MF tone in element %d (%c)\n", i, digit);
				break;
			}
			currdigit = digit;
			openr2_mf_tx_put(txstate, digit);
		} else if (strlen(argv[i]) == 1) {
			if ((argv[i][0] != 's' && argv[i][0] != 'S')) {
				fprintf(stderr, "Aborting generation due to invalid MF sequence element %d (%s)\n", i, argv[i]);
				break;
			}
			digit = 0;
			txstate = NULL;
		} else {
			fprintf(stderr, "Aborting generation due to invalid string length in MF sequence element %d (%s)\n", i, argv[i]);
			break;
		}

		i++;
		if (argc <= i) {
			fprintf(stderr, "Aborting generation due to missing length for the last MF tone (%c)\n", digit);
			break;
		}

		ms = atoi(argv[i]);
		if (ms < CHUNK_MS) {
			ms = CHUNK_MS;
		}
		total_samples = ms_to_samples(ms);

		/* we have tone and duration, we just need to generate it */
		printf("%s %c %s (samples = %d, ms = %d)\n", 
				dir, digit ? digit : currdigit, digit ? "ON" : "OFF", total_samples, ms);
		while (total_samples) {

			if (txstate) {
				res = openr2_mf_tx(txstate, slinear_buffer, CHUNK_SAMPLES);
				if (res != CHUNK_SAMPLES) {
					fprintf(stderr, "Failed to generate %d samples\n", CHUNK_SAMPLES);
					break;
				}
			} else {
				memset(slinear_buffer, 0, sizeof(slinear_buffer));
				res = CHUNK_SAMPLES;
			}

			if (format == FORMAT_ALAW) {
				for (j = 0; j < CHUNK_SAMPLES; j++) {
					alaw_buffer[j] = openr2_linear_to_alaw(slinear_buffer[j]);
				}
				fwrite(alaw_buffer, 1, res, audiofp);
			} else {
				fwrite(slinear_buffer, 2, res, audiofp);
			}

			total_samples -= CHUNK_SAMPLES;
		}
	}
	fclose(audiofp);
	return 0;
}

