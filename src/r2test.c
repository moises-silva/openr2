/*
 * R2Test
 * MFC/R2 call setup program
 *
 * Moises Silva <moy@sangoma.com>
 * Copyright (C) 2008 Moises Silva
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Contributors:
 *
 * Alexandre Cavalcante Alencar <alexandre.alencar@gmail.com>
 *
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdlib.h>
#ifdef HAVE_DAHDI_USER_H
#include <dahdi/user.h>
#endif
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/time.h>
#include "openr2/openr2.h"

/* max groups of channels */
#define MAX_GROUPS 50
/* max number of channels per group */
#define MAX_CHANS 30

#define CLEAN_CONF \
	for (c = 0; c < numgroups; c++) { \
		openr2_context_delete(g_confdata[c].context); \
	} 

#define STR_IS_EQUAL(x,y) !openr2_strncasecmp(x,y,sizeof(y)) 

/* counter, lock and condition for listener threads 
   used to sync when outgoing threads should start */
static int listener_count = 0;
pthread_mutex_t listener_count_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t listener_threads_done = PTHREAD_COND_INITIALIZER;

#ifdef HAVE_DAHDI_USER_H
static const char dahdi_mf_names[] = "123456789ABCDEF";

typedef struct {
	openr2_chan_t *r2chan;
	int forward;
	int signal;
	int generate;
} dahdi_mf_tx_state_t;
#endif
struct chan_group_data_s;
typedef struct {
#ifdef HAVE_DAHDI_USER_H
	dahdi_mf_tx_state_t dahdi_tx_state;
#endif
	pthread_t thread_id;
	openr2_chan_t *chan;
	FILE *audiofp;
	int audio_stop;
	int frame_counter;
	int called;
	struct chan_group_data_s *conf;
} r2chan_data_t;

typedef struct chan_group_data_s {
	openr2_calling_party_category_t category;
	openr2_context_t *context;
	openr2_variant_t variant;
	openr2_log_level_t loglevel;
	r2chan_data_t channels[MAX_CHANS];
	int lowchan;
	int highchan;
	int caller;
	int max_ani;
	int max_dnis;
	int getanifirst;
	int usedahdimf;
	int mf_threshold;
	int mf_backtimeout;
	int callfiles;
	int meteringpulse_timeout;
	int collect_calls;
	int charge_calls;
	int double_answer;
	int immediateaccept;
	int playaudio;
	char dnid[OR2_MAX_DNIS];
	char cid[OR2_MAX_ANI];
	char r2file[512];
	char audiofile[512];
} chan_group_data_t;

static chan_group_data_t g_confdata[MAX_GROUPS];

#ifdef HAVE_DAHDI_USER_H
static void *dahdi_mf_tx_init(dahdi_mf_tx_state_t *handle, int forward_signals)
{
	struct dahdi_dialoperation dahdi_operation = {
		.op = DAHDI_DIAL_OP_REPLACE
	};
	int res;
	/* choose either forward or backward signals */
	strcpy(dahdi_operation.dialstr, forward_signals ? "O" : "R");
	res = ioctl(openr2_chan_get_fd(handle->r2chan), DAHDI_DIAL, &dahdi_operation);
	if (-1 == res) {
		perror("init failed");
		return NULL;
	}
	handle->forward = forward_signals;
	handle->generate = 0;
	return handle;
}

static int dahdi_mf_tx_put(dahdi_mf_tx_state_t *handle, char signal)
{
	/* 0 is really A in DAHDI */
	signal = (signal == '0') ? 'A' : signal;
	if (signal && strchr(dahdi_mf_names, signal)) {
		handle->signal = handle->forward ? (DAHDI_TONE_MFR2_FWD_BASE + (signal - dahdi_mf_names[0])) 
			                         : (DAHDI_TONE_MFR2_REV_BASE + (signal - dahdi_mf_names[0]));
		if (signal >= 'A') {
			handle->signal -= 7;
		}
	} else if (!signal){
		handle->signal = -1;
	} else {
		return -1;
	}
	handle->generate = 1;
	return 0;
}

static int dahdi_mf_tx(dahdi_mf_tx_state_t *handle, int16_t buffer[], int samples)
{
	int res;
	res = ioctl(openr2_chan_get_fd(handle->r2chan), DAHDI_SENDTONE, &handle->signal);
	if (-1 == res) {
		perror("failed to set signal\n");
		return -1;
	}
	handle->generate = 0;
	return 0;
}

static int dahdi_mf_want_generate(dahdi_mf_tx_state_t *handle, int signal)
{
	return handle->generate;
}

static openr2_mflib_interface_t g_mf_dahdi_iface = {
	.mf_read_init = NULL,
	.mf_write_init = (openr2_mf_write_init_func)dahdi_mf_tx_init, 

	.mf_detect_tone = NULL,
	.mf_generate_tone = (openr2_mf_generate_tone_func)dahdi_mf_tx,

	.mf_select_tone = (openr2_mf_select_tone_func)dahdi_mf_tx_put,

	.mf_want_generate = (openr2_mf_want_generate_func)dahdi_mf_want_generate,
	.mf_read_dispose = NULL,
	.mf_write_dispose = NULL
};
#endif /* HAVE_DAHDI_USER_H */

/* Deal with audio playback feature */
int get_buf_length(const unsigned char *buf);
int get_buf_length(const unsigned char *buf)
{
	int n;
	
	for(n=0; *buf != '\0'; buf++ )
		n++;

	return(n);
}

static void show_variant_list(void);
static void show_variant_list(void)
{
	const openr2_variant_entry_t *variants;
	int i = 0;
	int j = 0;
	
	if(!(variants = openr2_proto_get_variant_list(&j))) {
		fprintf(stderr, "USER: failed to get variants list.\n");
	}

	printf("Variant\tContry\n");
	for (i = 0; i < j; i++) {
		printf("%7s\t%s\n", variants[i].name, variants[i].country);
	}
}

static void close_audiofp(openr2_chan_t *r2chan)
{
	r2chan_data_t *chandata = openr2_chan_get_client_data(r2chan);

	if(chandata->audiofp != NULL) {
		fclose(chandata->audiofp);
		chandata->audiofp = NULL;
		chandata->audio_stop = 1;
	}
}

static int get_audio(openr2_chan_t *r2chan, unsigned char *buf, int length)
{
	int ret;
	r2chan_data_t *chandata = openr2_chan_get_client_data(r2chan);
	if (chandata->audiofp == NULL) {
		printf("USER: no file opened yet, let's open it...\n");
		if (!(chandata->audiofp = fopen(chandata->conf->audiofile, "rb"))) {
			fprintf(stderr, "USER: Cannot open the '%s' audio file.\n", chandata->conf->audiofile);
			chandata->audio_stop = 1;
			return(-1);
		}	
	}	
	ret = fread(buf, 1, length, chandata->audiofp);
	if (ferror(chandata->audiofp)) {
		fprintf(stderr, "USER: Error reading file audio: %s\n", strerror(errno));
	} else if (feof(chandata->audiofp)) {
		printf("USER: end of file\n");
		close_audiofp(r2chan);
		return 0;
	} 
	return(ret);
}

static void on_call_init(openr2_chan_t *r2chan)
{
	printf("USER: new call detected on chan %d\n", openr2_chan_get_number(r2chan));
}

static void on_hardware_alarm(openr2_chan_t *r2chan, int alarm)
{
	printf("USER: alarm on chan %d!\n", openr2_chan_get_number(r2chan));
}

static void on_os_error(openr2_chan_t *r2chan, int errorcode)
{
	printf("USER: OS error on chan %d, quitting ...\n", openr2_chan_get_number(r2chan));
	close_audiofp(r2chan);
	pthread_exit((void *)1);
}

static void on_protocol_error(openr2_chan_t *r2chan, openr2_protocol_error_t reason)
{
	close_audiofp(r2chan);
	printf("USER: protocol error on chan %d, quitting ...\n", openr2_chan_get_number(r2chan));
	pthread_exit((void *)1);
}

static void on_call_offered(openr2_chan_t *r2chan, const char *ani, const char *dnis, openr2_calling_party_category_t category)
{
	r2chan_data_t *chandata = openr2_chan_get_client_data(r2chan);
	chan_group_data_t *confdata = chandata->conf;
	printf("USER: call ready on chan %d. DNIS = %s, ANI = %s, Category = %d\n", 
			openr2_chan_get_number(r2chan), dnis, ani ? ani : "(restricted)", category);
	/* if collect calls are not allowed and this is a collect call, reject it */
	if (!confdata->collect_calls && category == OR2_CALLING_PARTY_CATEGORY_COLLECT_CALL) {
		openr2_chan_disconnect_call(r2chan, OR2_CAUSE_COLLECT_CALL_REJECTED);
		return;
	}
	if (confdata->charge_calls) {
		openr2_chan_accept_call(r2chan, OR2_CALL_WITH_CHARGE);
	} else {
		openr2_chan_accept_call(r2chan, OR2_CALL_NO_CHARGE);
	}
}

static void on_call_accepted(openr2_chan_t *r2chan, openr2_call_mode_t mode)
{
	printf("USER: call has been accepted on chan %d with type: %s\n", openr2_chan_get_number(r2chan), openr2_proto_get_call_mode_string(mode));
	if (openr2_chan_get_direction(r2chan) == OR2_DIR_BACKWARD) {
		openr2_chan_answer_call(r2chan);
	}	
}

static void on_call_answered(openr2_chan_t *r2chan)
{
	printf("USER: call has been answered on chan %d\n", openr2_chan_get_number(r2chan));
}

static void on_call_read(openr2_chan_t *r2chan, const unsigned char *buf, int buflen)
{
	r2chan_data_t *chandata = openr2_chan_get_client_data(r2chan);
	unsigned char write_buf[OR2_CHAN_READ_SIZE];
	int write_buflen;
	chandata->frame_counter++;

	if (chandata->frame_counter == 50) {
		printf("USER: call read data of length %d on chan %d\n", buflen, openr2_chan_get_number(r2chan));
	}

	if (chandata->conf->playaudio == 0) {
		openr2_chan_write(r2chan, buf, buflen);
	} else if (!chandata->audio_stop) {
		if (chandata->frame_counter == 50) {
			printf("USER: playing an audio stream.\n");
		}	
		write_buflen = get_audio(r2chan, write_buf, sizeof(write_buf));
		if (-1 == write_buflen){
			fprintf(stderr, "USER: Failed to get audio\n");
			goto done;
		}
		if (0 == write_buflen) {
			goto done;
		}
		openr2_chan_write(r2chan, write_buf, write_buflen);
	}
done:
	if (chandata->frame_counter == 400) {
		chandata->frame_counter = 0;
	}
}

static void on_call_disconnected(openr2_chan_t *r2chan, openr2_call_disconnect_cause_t cause)
{
	printf("USER: got disconnect on chan %d: %s\n", openr2_chan_get_number(r2chan), openr2_proto_get_disconnect_string(cause));
	openr2_chan_disconnect_call(r2chan, OR2_CAUSE_NORMAL_CLEARING);
}

static void on_call_end(openr2_chan_t *r2chan)
{
	close_audiofp(r2chan);
	printf("USER: call ended at chan %d\n", openr2_chan_get_number(r2chan));
	
}

static void on_line_blocked(openr2_chan_t *r2chan)
{
	printf("USER: far end blocked on chan %d\n", openr2_chan_get_number(r2chan));
}

static void on_line_idle(openr2_chan_t *r2chan)
{
	int res;
	r2chan_data_t *chandata = openr2_chan_get_client_data(r2chan);
	chan_group_data_t *confdata = chandata->conf;
	printf("USER: far end unblocked on chan %d\n", openr2_chan_get_number(r2chan));
	if (chandata->called || !confdata->caller) {
		return;
	}
	printf("USER: making call on chan %d. DNID = %s, CID = %s, CPC = %s\n", 
			openr2_chan_get_number(r2chan), confdata->dnid, 
			confdata->cid, openr2_proto_get_category_string(confdata->category));
	res = openr2_chan_make_call(r2chan, confdata->cid, confdata->dnid, confdata->category);
	if (-1 == res) {
		fprintf(stderr, "Error making call on chan %d\n", openr2_chan_get_number(r2chan));
		return;
	}
	/* set the fal to not make more calls when getting back to IDLE */
	chandata->called = 1;
}

static int on_dnis_digit_received(openr2_chan_t *r2chan, char digit)
{
	printf("USER: New DNIS digit '%c' received on chan %d\n", digit, openr2_chan_get_number(r2chan));
	return 1;
}

static void on_ani_digit_received(openr2_chan_t *r2chan, char digit)
{
	printf("USER: New ANI digit '%c' received on chan %d\n", digit, openr2_chan_get_number(r2chan));
}

static void on_billing_pulse_received(openr2_chan_t *r2chan)
{
	printf("USER: Billing Pulse Received on chan %d\n", openr2_chan_get_number(r2chan));
}

static openr2_event_interface_t g_event_iface = {
	.on_call_init = on_call_init,
	.on_call_offered = on_call_offered,
	.on_call_accepted = on_call_accepted,
	.on_call_answered = on_call_answered,
	.on_call_disconnect = on_call_disconnected,
	.on_call_end = on_call_end,
	.on_call_read = on_call_read,
	.on_hardware_alarm = on_hardware_alarm,
	.on_os_error = on_os_error,
	.on_protocol_error = on_protocol_error,
	.on_line_blocked = on_line_blocked,
	.on_line_idle = on_line_idle,
	.on_context_log = NULL,
	.on_dnis_digit_received = on_dnis_digit_received,
	.on_ani_digit_received = on_ani_digit_received,
	.on_billing_pulse_received = on_billing_pulse_received
};

static int parse_config(FILE *conf, chan_group_data_t *confdata)
{
	char line[512];
	int g = 0;
	openr2_calling_party_category_t category = OR2_CALLING_PARTY_CATEGORY_NATIONAL_SUBSCRIBER;
	openr2_variant_t variant = OR2_VAR_UNKNOWN;
	openr2_log_level_t loglevel = OR2_LOG_NOTHING;
	openr2_log_level_t tmplevel = OR2_LOG_NOTHING;
	int lowchan = -1;
	int highchan = -1;
	int max_ani = 0;
	int max_dnis = 0;
	int getanifirst = 0;
	int usedahdimf = 0;
	int mf_threshold = 0;
	int mf_backtimeout = 0;
	int int_test = 0;
	int callfiles = 0;
	int meteringpulse_timeout = -1;
	int collect_calls = 0;
	int charge_calls = 1;
	int double_answer = 0;
	int immediateaccept = 0;
	char strvalue[512];
	char r2file[512];
	char audiofile[512];
	char *toklevel;
	char dnid[OR2_MAX_DNIS];
	char cid[OR2_MAX_ANI];
	int caller = 0;
	int playaudio = 0;
	dnid[0] = 0;
	cid[0] = 0;
	strvalue[0] = 0;
	r2file[0] = 0;
	while (fgets(line, sizeof(line), conf)) {
		if ('#' == line[0] || '\n' == line[0] || ' ' == line[0]) {
			continue;
		}
		if (2 == sscanf(line, "channel=%d-%d", &lowchan, &highchan)) {
			printf("found channel range = %d-%d\n", lowchan, highchan);
			if (lowchan > highchan) {
				fprintf(stderr, "invalid channel range, low chan must not be bigger than high chan\n");
				return -1;
			}
			if (caller && (!dnid[0] || !cid[0])) {
				fprintf(stderr, "No DNID or CID when caller=yes\n");
				return -1;
			}
			/* time to create a new group */
			confdata[g].category = category;
			confdata[g].variant = variant;
			confdata[g].getanifirst = getanifirst;
			confdata[g].lowchan = lowchan;
			confdata[g].highchan = highchan;
			confdata[g].caller = caller;
			confdata[g].max_dnis = max_dnis;
			confdata[g].max_ani = max_ani;
			confdata[g].loglevel = loglevel;
			confdata[g].usedahdimf = usedahdimf;
			confdata[g].mf_threshold = mf_threshold;
			confdata[g].mf_backtimeout = mf_backtimeout;
			confdata[g].callfiles = callfiles;
			confdata[g].meteringpulse_timeout = meteringpulse_timeout;
			confdata[g].collect_calls = collect_calls;
			confdata[g].double_answer = double_answer;
			confdata[g].immediateaccept = immediateaccept;
			confdata[g].charge_calls = charge_calls;
			confdata[g].playaudio = playaudio;
			strcpy(confdata[g].dnid, dnid);
			strcpy(confdata[g].cid, cid);
			strcpy(confdata[g].r2file, r2file);
			strcpy(confdata[g].audiofile, audiofile);
			g++;
			if (g == (MAX_GROUPS - 1)) {
				printf("MAX_GROUPS reached, quitting loop ...\n");
				break;
			}
		} else if (1 == sscanf(line, "advancedprotocolfile=%s", r2file)) {
			printf("found option advancedprotocolfile=%s\n", r2file);
		} else if (1 == sscanf(line, "chargecalls=%s", strvalue)) {
			printf("found option chargecalls=%s\n", strvalue);
			if (STR_IS_EQUAL(strvalue,"yes")) {
				charge_calls = 1;
			} else if (STR_IS_EQUAL(strvalue,"no")) {
				charge_calls = 0;
			} else {
				fprintf(stderr, "Invalid value '%s' for 'chargecalls' parameter.\n", strvalue);
			}
		} else if (1 == sscanf(line, "collectcalls=%s", strvalue)) {
			printf("found option collectcalls=%s\n", strvalue);
			if (STR_IS_EQUAL(strvalue,"yes")) {
				collect_calls = 1;
			} else if (STR_IS_EQUAL(strvalue,"no")) {
				collect_calls = 0;
			} else {
				fprintf(stderr, "Invalid value '%s' for 'collectcalls' parameter.\n", strvalue);
			}
		} else if (1 == sscanf(line, "callfiles=%s", strvalue)) {
			printf("found option callfiles=%s\n", strvalue);
			if (STR_IS_EQUAL(strvalue,"yes")) {
				callfiles = 1;
			} else if (STR_IS_EQUAL(strvalue,"no")) {
				callfiles = 0;
			} else {
				fprintf(stderr, "Invalid value '%s' for 'callfiles' parameter.\n", strvalue);
			}
		} else if (1 == sscanf(line, "doubleanswer=%s", strvalue)) {
			printf("found option doubleanswer=%s\n", strvalue);
			if (STR_IS_EQUAL(strvalue,"yes")) {
				double_answer = 1;
			} else if (STR_IS_EQUAL(strvalue,"no")) {
				double_answer = 0;
			} else {
				fprintf(stderr, "Invalid value '%s' for 'doubleanswer' parameter.\n", strvalue);
			}
		} else if (1 == sscanf(line, "immediateaccept=%s", strvalue)) {
			printf("found option immediateaccept=%s\n", strvalue);
			if (STR_IS_EQUAL(strvalue,"yes")) {
				immediateaccept = 1;
			} else if (STR_IS_EQUAL(strvalue,"no")) {
				immediateaccept = 0;
			} else {
				fprintf(stderr, "Invalid value '%s' for 'immediateaccept' parameter.\n", strvalue);
			}
		} else if (1 == sscanf(line, "meteringpulsetimeout=%s", strvalue)) {
			printf("found option meteringpulsetimeout=%s\n", strvalue);
			int_test = atoi(strvalue);
			if (!int_test && strvalue[0] != '0') {
				fprintf(stderr, "Invalid value '%s' for 'meteringpulsetimeout' parameter.\n", strvalue);
				continue;
			}
			meteringpulse_timeout = int_test;
		} else if (1 == sscanf(line, "loglevel=%s", strvalue)) {
			printf("found Log Level = %s\n", strvalue);
			toklevel = strtok(strvalue, ",");
			if (-1 == (tmplevel = openr2_log_get_level(toklevel))) {
				fprintf(stderr, "Invalid logging level: '%s'\n", strvalue);
				continue;
			}
			if (OR2_LOG_NOTHING == tmplevel) {
				loglevel = OR2_LOG_NOTHING;
				continue;
			}
			loglevel |= tmplevel;
			while ((toklevel = strtok(NULL, ","))) {
				if (-1 == (tmplevel = openr2_log_get_level(toklevel))) {
					fprintf(stderr, "Ignoring invalid logging level: '%s'\n", toklevel);
					continue;
				}
				loglevel |= tmplevel;
			}
		} else if (1 == sscanf(line, "mfthreshold=%s", strvalue)) {
			printf("found option MF threshold = %s\n", strvalue);
			int_test = atoi(strvalue);
			if (!int_test && strvalue[0] != '0') {
				fprintf(stderr, "Invalid value '%s' for 'mfthreshold' parameter.\n", strvalue);
				continue;
			}
			mf_threshold = int_test;
		} else if (1 == sscanf(line, "mfbacktimeout=%s", strvalue)) {
			printf("found option MF backward timeout = %s\n", strvalue);
			int_test = atoi(strvalue);
			if (!int_test && strvalue[0] != '0') {
				fprintf(stderr, "Invalid value '%s' for 'mfbacktimeout' parameter.\n", strvalue);
				continue;
			}
			mf_backtimeout = int_test;
		} else if (1 == sscanf(line, "getanifirst=%s", strvalue)) {
			printf("found option Get ANI First = %s\n", strvalue);
			if (STR_IS_EQUAL(strvalue,"yes")) {
				getanifirst = 1;
			} else if (STR_IS_EQUAL(strvalue,"no")) {
				getanifirst = 0;
			} else {
				fprintf(stderr, "Invalid value '%s' for 'getanifirst' parameter.\n", strvalue);
			}
		} else if (1 == sscanf(line, "usedahdimf=%s", strvalue)) {
#ifdef HAVE_DAHDI_USER_H
			printf("found option Use DAHDI MF = %s\n", strvalue);
			if (STR_IS_EQUAL(strvalue,"yes")) {
				usedahdimf = 1;
			} else if (STR_IS_EQUAL(strvalue,"no")) {
				usedahdimf = 0;
			} else {
				fprintf(stderr, "Invalid value '%s' for 'usedahdimf' parameter.\n", strvalue);
			}
#else
			printf("DAHDI R2 MF is not available, ignoring option usedahdimf.\n");
#endif
		} else if (1 == sscanf(line, "maxani=%d", &max_ani)) {
			printf("found MAX ANI= %d\n", max_ani);	
		} else if (1 == sscanf(line, "maxdnis=%d", &max_dnis)) {
			printf("found MAX DNIS= %d\n", max_dnis);	
		} else if (1 == sscanf(line, "cid=%s", cid)) {
			printf("found CID = %s\n", cid);
		} else if (1 == sscanf(line, "dnid=%s", dnid)) {
			printf("found DNID = %s\n", dnid);
		} else if (1 == sscanf(line, "category=%s", strvalue)) {
			printf("found category = %s\n", strvalue);
			category = openr2_proto_get_category(strvalue);
			if (OR2_CALLING_PARTY_CATEGORY_UNKNOWN == category) {
				fprintf(stderr, "Unknown category %s, defaulting to national subscriber.\n", strvalue);
				category = OR2_CALLING_PARTY_CATEGORY_NATIONAL_SUBSCRIBER;
			}
		} else if (1 == sscanf(line, "variant=%s", strvalue)) {
			printf("found R2 variant = %s\n", strvalue);
			variant = openr2_proto_get_variant(strvalue);
			if (OR2_VAR_UNKNOWN == variant) {
				fprintf(stderr, "Unknown variant %s\n", strvalue);
			}
		} else if (1 == sscanf(line, "caller=%s", strvalue)) {
			if (STR_IS_EQUAL(strvalue,"yes")) {
				caller = 1;
			} else if (STR_IS_EQUAL(strvalue,"no")) {
				caller = 0;
			} else {
				fprintf(stderr, "Invalid value '%s' for 'caller' parameter.\n", strvalue);
			}
		} else if (1 == sscanf(line, "playaudio=%s", strvalue)) {
			if (STR_IS_EQUAL(strvalue,"yes")) {
				playaudio = 1;
			} else if (STR_IS_EQUAL(strvalue,"no")) {
				playaudio = 0;
			} else {
				fprintf(stderr, "Invalid value '%s' for 'playaudio' parameter.\n", strvalue);
			}
		} else if (1 == sscanf(line, "audiofile=%s", audiofile)) {
			if (playaudio) {
				printf("found option 'audiofile=%s'\n", audiofile);
			} else {
				printf("found option 'audiofile=%s' but 'playaudio' not set, not using it.\n", audiofile);
			}		
		} else {
			fprintf(stderr, "ERROR in config file: cannot parse line: '%s'\n", line);
			return -1;
		}
	}
	if (!g) {
		fprintf(stderr, "0 groups of channels configured (did you missed a 'channel' parameter?).\n");
	}	
	return g;
}

void *wait_call(void *data);
void *wait_call(void *data)
{
	openr2_chan_t *r2chan = data;
	r2chan_data_t *chandata = openr2_chan_get_client_data(r2chan);
	chan_group_data_t *confdata = chandata->conf;
	struct timeval timeout, *timeout_ptr;
	int ms, chanfd, res, channo;
	unsigned loopcount = 0;
	fd_set chanread, chanexcept;
	openr2_variant_t variant;
	channo = openr2_chan_get_number(r2chan);
	chanfd = openr2_chan_get_fd(r2chan);
	variant = openr2_context_get_variant(confdata->context);
	printf("channel %d, variant = %s\n", channo, openr2_proto_get_variant_string(variant));
	openr2_proto_set_idle(r2chan);
	openr2_proto_handle_abcd_change(r2chan);
	while (1) {
		FD_ZERO(&chanread);
		FD_ZERO(&chanexcept);
		FD_SET(chanfd, &chanread);
		FD_SET(chanfd, &chanexcept);
		ms = openr2_chan_get_time_to_next_event(r2chan);
		if (ms < 0) { 
			timeout_ptr = NULL;
		} else {
			timeout.tv_sec = ( ms / 1000 ); 
			timeout.tv_usec = ( ( ms % 1000 ) * 1000 );
			timeout_ptr = &timeout;
		}
		/* is this is the first time we are in the loop 
		   decrement the listener thread count */
		if (!loopcount) {
			pthread_mutex_lock(&listener_count_lock);
			listener_count--;
			/* if all listener threads are ready to receive calls, 
			   let's signal the main thread */
			if (0 == listener_count) {
				pthread_cond_signal(&listener_threads_done);
			}	
			pthread_mutex_unlock(&listener_count_lock);
		}
		loopcount++;
		if (loopcount > 50000 && (loopcount % 50000) == 0) {
			printf("waiting for calls in channel %d\n", channo);
		}
		res = select(chanfd + 1, &chanread, NULL, &chanexcept, timeout_ptr);
		if (-1 == res) {
			perror("select() failed");
			break;
		}
		if (FD_ISSET(chanfd, &chanread) || FD_ISSET(chanfd, &chanexcept)) {
			openr2_chan_process_event(r2chan);
		}
	}
	return (void *)0;
}

void *make_call(void *data);
void *make_call(void *data)
{
	/* the call will be made when the other end is unblocked */
	int res, chanfd, ms;
	fd_set chanread, chanexcept;
	openr2_chan_t *r2chan = data;
	struct timeval timeout, *timeout_ptr;
	chanfd = openr2_chan_get_fd(r2chan);
	/* handle current state of ABCD bits, either blocked or idle */
	openr2_proto_set_idle(r2chan);
	openr2_proto_handle_abcd_change(r2chan);
	while (1) {
		FD_ZERO(&chanread);
		FD_ZERO(&chanexcept);
		FD_SET(chanfd, &chanread);
			FD_SET(chanfd, &chanexcept);
		ms = openr2_chan_get_time_to_next_event(r2chan);
		if (ms < 0) { 
			timeout_ptr = NULL;
		} else {
			timeout.tv_sec = ( ms / 1000 ); 
			timeout.tv_usec = ( ( ms % 1000 ) * 1000 );
			timeout_ptr = &timeout;
		}
		res = select(chanfd + 1, &chanread, NULL, &chanexcept, timeout_ptr);
		if (-1 == res) {
			perror("select() failed");
			break;
		}
		if (FD_ISSET(chanfd, &chanread) || FD_ISSET(chanfd, &chanexcept)) {
			openr2_chan_process_event(r2chan);
		}
	}
	return (void *)0;
}

int main(int argc, char *argv[])
{
	int res, c, i, numgroups, cnt, o;
	FILE *config;
	struct stat confstat;
	void *tx_mf_state = NULL;
	openr2_mflib_interface_t *mf_iface = NULL;

	extern char *optarg;
	extern int optind, optopt;

	if (argc < 2) {
		fprintf(stderr, "Usage:\n\t%s <file>\tConfiguration file\n", argv[0]);
		fprintf(stderr, "\t%s -l\tShow variants list\n", argv[0]);
		fprintf(stderr, "\t%s -v\tShow version info\n", argv[0]);
		return -1;
	}
	
	while ((o = getopt(argc, argv, ":lv")) != -1) {
		switch(o) {
		case 'l':
			show_variant_list();
			exit(0);
		case 'v':
			printf("OpenR2 version: %s, release: %s\n", openr2_get_version(), openr2_get_revision());
			exit(0);
		case '?':
			break;
		}
	}

	if (stat(argv[1], &confstat)) {
		perror("failed to stat() configuration file");
		return -1;
	}

	config = fopen(argv[1], "r");
	if (!config) {
		perror("unable to open configuration file");
		return -1;
	}

	memset(g_confdata, 0, sizeof(g_confdata));
	if ((numgroups = parse_config(config, g_confdata)) < 1) {
		fclose(config);
		return -1;
	}
	fclose(config);

	/* we have a bunch of channels, let's create contexts for each group of them */
	for (c = 0; c < numgroups; c++) {
#ifdef HAVE_DAHDI_USER_H
		if (g_confdata[c].usedahdimf) {
			mf_iface = &g_mf_dahdi_iface;
		}
#endif
		g_confdata[c].context = openr2_context_new(mf_iface, &g_event_iface, 
				NULL, g_confdata[c].variant, g_confdata[c].max_ani, g_confdata[c].max_dnis);
		if (!g_confdata[c].context) {
			fprintf(stderr, "failed to create R2 context when c = %d\n", c);
			break;
		}
		openr2_context_set_log_level(g_confdata[c].context, g_confdata[c].loglevel);
		openr2_context_set_ani_first(g_confdata[c].context, g_confdata[c].getanifirst);
		openr2_context_set_mf_threshold(g_confdata[c].context, g_confdata[c].mf_threshold);
		openr2_context_set_mf_back_timeout(g_confdata[c].context, g_confdata[c].mf_backtimeout);
		openr2_context_set_metering_pulse_timeout(g_confdata[c].context, g_confdata[c].meteringpulse_timeout);
		openr2_context_set_double_answer(g_confdata[c].context, g_confdata[c].double_answer);
		openr2_context_set_immediate_accept(g_confdata[c].context, g_confdata[c].immediateaccept);
		if (g_confdata[c].r2file[0] != 0) {
			if (openr2_context_configure_from_advanced_file(g_confdata[c].context, g_confdata[c].r2file)) {
				fprintf(stderr, "failed to configure R2 context with file %s\n", g_confdata[c].r2file);
			}
		}	
	}
	/* something failed, thus, at least 1 group did not get a context */
	if (c != numgroups) {
		/* let's free what we allocated so far and bail out */
		for (--c; c >= 0; c--) {
			openr2_context_delete(g_confdata[c].context);
		}
		fprintf(stderr, "Aborting test.\n");
		return -1;
	}

	/* now create channels for each context */
	for (c = 0; c < numgroups; c++) {
		for (i = g_confdata[c].lowchan, cnt = 0; i <= g_confdata[c].highchan; i++, cnt++) {
#ifdef HAVE_DAHDI_USER_H
			if (g_confdata[c].usedahdimf) {
				tx_mf_state = &g_confdata[c].channels[cnt].dahdi_tx_state;
			}
#endif
			g_confdata[c].channels[cnt].chan = openr2_chan_new(g_confdata[c].context, i, tx_mf_state, NULL);
			if (!g_confdata[c].channels[cnt].chan) {
				fprintf(stderr, "failed to create R2 channel %d: %s\n", i,
						openr2_context_error_string(openr2_context_get_last_error(g_confdata[c].context)));
				break;
			}
			g_confdata[c].channels[cnt].conf = &g_confdata[c];
			openr2_chan_set_client_data(g_confdata[c].channels[cnt].chan, &g_confdata[c].channels[cnt]);
			if (g_confdata[c].callfiles) {
				openr2_chan_enable_call_files(g_confdata[c].channels[cnt].chan);
			}
#ifdef HAVE_DAHDI_USER_H
			g_confdata[c].channels[cnt].dahdi_tx_state.r2chan = g_confdata[c].channels[cnt].chan;
#endif
		}
		/* something failed, thus, at least 1 channel could not be created */
		if (cnt != ((g_confdata[c].highchan - g_confdata[c].lowchan)+1)) {
			CLEAN_CONF;
			fprintf(stderr, "Aborting test.\n");
			return -1;
		}
	}

	/* grab the listener count lock */
	pthread_mutex_lock(&listener_count_lock);

	/* let's launch a listener thread for each non-caller channel.
	   I decided to launch non-caller threads first, but is not
	   strictly necessary since calling threads will wait until
	   the far end is unblocked before making the call */
	listener_count = 0;
	for (c = 0; c < numgroups; c++) {
		if (g_confdata[c].caller) {
			continue;
		}
		for (i = g_confdata[c].lowchan, cnt = 0; i<= g_confdata[c].highchan; i++, cnt++) {
			res = pthread_create(&g_confdata[c].channels[cnt].thread_id, NULL, wait_call, g_confdata[c].channels[cnt].chan);
			if (res) {
				fprintf(stderr, "Failed to create listener thread for channel %d, continuing anyway ...\n", 
						openr2_chan_get_number(g_confdata[c].channels[cnt].chan));
				continue;
			}
			listener_count++;
		}	
	}
	
	/* wait until the listener threads are ready */
	printf("Spawned %d listener threads, waiting for them to be ready ...\n", listener_count);
	if (listener_count > 0) {
		pthread_cond_wait(&listener_threads_done, &listener_count_lock);
	} else {
		pthread_mutex_unlock(&listener_count_lock);
	}	

	/* time to spawn a thread for each channel that requested to start a call */
	int caller_threads = 0;
	for (c = 0; c < numgroups; c++) {
		if (!g_confdata[c].caller) {
			continue;
		}
		for (i = g_confdata[c].lowchan, cnt = 0; i <= g_confdata[c].highchan; i++, cnt++) {
			res = pthread_create(&g_confdata[c].channels[cnt].thread_id, NULL, 
					make_call, g_confdata[c].channels[cnt].chan);
			if (res) {
				fprintf(stderr, "Failed to create calling thread for channel %d, continuing anyway ...\n", 
						openr2_chan_get_number(g_confdata[c].channels[cnt].chan));
				continue;
			}
			caller_threads++;
		}
	}
	printf("Spawned %d calling threads, waiting for all threads ... \n", caller_threads);

	/* wait for all the threads to be done */
	for (c = 0; c < numgroups; c++) {
		for (i = g_confdata[c].lowchan, cnt = 0; i <= g_confdata[c].highchan; i++, cnt++) {
			pthread_join(g_confdata[c].channels[cnt].thread_id, NULL);
		}
	}

	return 0;
}

