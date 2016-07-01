#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/timerfd.h>
#include <rtl-sdr.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>

#define BAUDOT_PREAMBLE_LENGTH 8
#include "baudot.h"

int do_exit = 0;
static void sighandler(int signum) {
	(void)signum;
	do_exit = 1;
}

#define RTL_CHECK(x, y) { \
	ret = (x y); \
	if (ret < 0) { \
		fprintf(stderr, "Error: %s returned %d at %s:%d.\n", #x, ret, __FILE__, __LINE__); \
		goto err; \
	} \
}

#define TXBITS_LEN 512
int main(int argc, char *argv[]) {
	int idx = 0;
	int sample_rate = 2.4e6;
	int if_freq = 3.6e6;
	int tx_gain = 200;
	unsigned int center_freq = 1250e6;
	int tone1 = 200, shift = 440;

	rtlsdr_dev_t *dev = NULL;
	uint8_t txbits[TXBITS_LEN];
	int ret, nbytes, nbits, tfd=-1;
	unsigned int txfreq, txfreq_tune, txfreq_tune1, txfreq_tune0, txfreq_prev;
	struct sigaction sigact;
	
	if(argc != 3) {
		printf("Usage: %s transmit_frequency_in_Hz \"text to transmit\"\n", argv[0]);
		return 0;
	}
	center_freq = atof(argv[1]);
	
	// setup signals
	sigact.sa_handler = sighandler;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;
	sigaction(SIGINT, &sigact, NULL);
	sigaction(SIGTERM, &sigact, NULL);
	sigaction(SIGQUIT, &sigact, NULL);
	sigaction(SIGPIPE, &sigact, NULL);
	
	nbytes = baudot_from_ascii(txbits, TXBITS_LEN, argv[2]);
	nbits = nbytes*8;
	
	// initialize rtl-sdr
	RTL_CHECK(rtlsdr_open,(&dev, idx));
	RTL_CHECK(rtlsdr_set_sample_rate,(dev, sample_rate));
	RTL_CHECK(rtlsdr_set_dithering,(dev, 0));
	RTL_CHECK(rtlsdr_set_if_freq,(dev, if_freq));
	RTL_CHECK(rtlsdr_set_tuner_gain_mode,(dev, 1));
	RTL_CHECK(rtlsdr_set_tuner_gain,(dev, tx_gain));

	tfd = timerfd_create(CLOCK_MONOTONIC, 0);
	
	struct itimerspec tfd_ts;
	tfd_ts.it_value.tv_sec = 0;
	tfd_ts.it_value.tv_nsec = 1;
	tfd_ts.it_interval.tv_sec = 0;
	tfd_ts.it_interval.tv_nsec = 1e9 / 45.45 + 0.5;
	ret = timerfd_settime(tfd, 0, &tfd_ts, NULL);
	//printf("%d\n", ret);

	txfreq = center_freq + tone1;
	txfreq_tune1 = txfreq / 4 - if_freq;
	txfreq = center_freq + tone1 - shift;
	txfreq_tune0 = txfreq / 4 - if_freq;
	txfreq_prev = 0;

	printf("Transmitting\n");
	int bit_i = 0;
	uint64_t tfd_e = 0;
	ret = read(tfd, &tfd_e, sizeof(uint64_t));
	while(!do_exit) {
		if(txbits[bit_i / 8] & (1<<(7&bit_i)))
			txfreq_tune = txfreq_tune1;
		else
			txfreq_tune = txfreq_tune0;
		if(txfreq_tune != txfreq_prev)
			RTL_CHECK(rtlsdr_set_center_freq,(dev, txfreq_tune));
		txfreq_prev = txfreq_tune;

		ret = read(tfd, &tfd_e, sizeof(uint64_t));
		if(ret < 0) break;
		if(tfd_e > 1)
			printf("Warning: missed %ld bits\n", tfd_e-1);
		/* read from timerfd returns the number of timer
		   expirations. If we have missed some number of these,
		   we'll skip the same number of bits to keep timing
		   correct. */
		bit_i += tfd_e;
		bit_i = bit_i % nbits;
	}
	close(tfd);

	err:
	if(dev)
		rtlsdr_close(dev);
	
	return 0;
}

