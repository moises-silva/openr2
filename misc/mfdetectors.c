#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <spandsp.h>

int main(int argc, char *argv[])
{
	int rc = 0;
	short readbuf[160];
	char digitsbuf[255];
	int fd = open(argv[1], O_RDONLY);

#if defined(BELL_DETECTOR)
	bell_mf_rx_state_t *rxstate = NULL;
	if (!(rxstate = bell_mf_rx_init(rxstate, NULL, NULL))) {
		fprintf(stderr, "bell_mf_rx_init\n");
		return -1;
	}
#elif defined(MF_DETECTOR_FWD)
	r2_mf_rx_state_t *rxstate = NULL;
	if (!(rxstate = r2_mf_rx_init(rxstate, 1, NULL, NULL))) {
		fprintf(stderr, "r2_mf_rx_init fwd\n");
		return -1;
	}
#elif defined(MF_DETECTOR_BWD)
	r2_mf_rx_state_t *rxstate = NULL;
	if (!(rxstate = r2_mf_rx_init(rxstate, 0, NULL, NULL))) {
		fprintf(stderr, "r2_mf_rx_init backward\n");
		return -1;
	}
#elif defined(DTMF_DETECTOR)
	dtmf_rx_state_t *rxstate = NULL;
	if (!(rxstate = dtmf_rx_init(rxstate, NULL, NULL))) {
		fprintf(stderr, "dtmf_rx_init\n");
		return -1;
	}
#else
#error "define detector type!"
#endif

	if (fd == -1) {
		perror("fopen");
		return -1;
	}

	while (1) {
		rc = read(fd, readbuf, sizeof(readbuf)/sizeof(readbuf[0]));
		if (rc < 0) {
			perror("read");
			break;
		}
		if (!rc) {
			break;
		}
#if defined(BELL_DETECTOR)
		rc = bell_mf_rx(rxstate, readbuf, rc/2);
		if (rc != 0) {
			fprintf(stderr, "Unprocessed samples: %d\n", rc);
			break;
		}
		digitsbuf[0] = '\0';
		rc = bell_mf_rx_get(rxstate, digitsbuf, sizeof(digitsbuf)-1);
		if (rc > 0) {
			printf("Rx digits: %s\n", digitsbuf);
		}
#elif defined(MF_DETECTOR_FWD) || defined(MF_DETECTOR_BWD)
		rc = r2_mf_rx(rxstate, readbuf, rc/2);
		if (rc != 0) {
			fprintf(stderr, "Unprocessed samples: %d\n", rc);
			break;
		}
		rc = r2_mf_rx_get(rxstate);
		if (rc > 0) {
			printf("Rx digit: %c\n", rc);
		}
#elif defined(DTMF_DETECTOR)
		rc = dtmf_rx(rxstate, readbuf, rc/2);
		if (rc != 0) {
			fprintf(stderr, "Unprocessed samples: %d\n", rc);
			break;
		}
		digitsbuf[0] = '\0';
		rc = dtmf_rx_get(rxstate, digitsbuf, sizeof(digitsbuf)-1);
		if (rc > 0) {
			printf("Rx digit: %s\n", digitsbuf);
		}
#else
#error "define detector type!"
#endif
	}

#if defined(BELL_DETECTOR)
	bell_mf_rx_free(rxstate);
#elif defined(MF_DETECTOR_FWD) || defined(MF_DETECTOR_BWD)
	r2_mf_rx_free(rxstate);
#elif defined(DTMF_DETECTOR)
	dtmf_rx_free(rxstate);
#endif

	close(fd);
	return 0;
}

