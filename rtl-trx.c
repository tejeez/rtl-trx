#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/timerfd.h>
#include <rtl-sdr.h>
#include <pthread.h>
#include <signal.h>
#include <liquid/liquid.h>
#include <assert.h>

#define RTL_CHECK(x, y) { \
	ret = (x y); \
	if (ret < 0) { \
		fprintf(stderr, "Error: %s returned %d at %s:%d.\n", #x, ret, __FILE__, __LINE__); \
		goto err; \
	} \
}


const char bits[] = {
0, 0,1,0,1,0, 1,
0, 0,0,0,0,1, 1,
0, 0,0,0,0,1, 1,
0, 1,0,1,0,1, 1,
0, 0,0,1,0,0, 1,
1, 1,1,1,1,1, 1,
};
const int nbits = sizeof(bits);

pthread_t control_thrd=0;
rtlsdr_dev_t *dev = NULL;
int if_freq = 3.6e6;
long long center_freq = /*1250e6*/ 433.55e6;
int tone1 = 1000, shift = 440;
int do_exit = 0;

// decimation from 2.4 MHz to 8 kHz/2
#define DECIM1 600

static void sighandler(int signum) {
	(void)signum;
	do_exit = 1;
}


static void *control(void *arg) {
	long long txfreq, txfreq_tune;
	(void)arg;
	int ret;
	
	int tfd=-1;

	tfd = timerfd_create(CLOCK_MONOTONIC, 0);
	
	struct itimerspec tfd_ts;
	tfd_ts.it_value.tv_sec = 0;
	tfd_ts.it_value.tv_nsec = 1;
	tfd_ts.it_interval.tv_sec = 0;
	tfd_ts.it_interval.tv_nsec = 1e9 / 45.45 + 0.5;
	ret = timerfd_settime(tfd, 0, &tfd_ts, NULL);
	//printf("%d\n", ret);
	
	int bit_i;
	
	bit_i = 0;
	while(bit_i < nbits) {
		uint64_t tfd_e = 0;
		ret = read(tfd, &tfd_e, sizeof(uint64_t));
		if(ret < 0) break;
		//printf("%d %ld\n", ret, tfd_e);
		if(bits[bit_i % nbits] & 1)
			txfreq = center_freq + tone1;
		else
			txfreq = center_freq + tone1 - shift;
		txfreq_tune = txfreq / 4 - if_freq;
		RTL_CHECK(rtlsdr_set_center_freq,(dev, txfreq_tune));
		
		bit_i += tfd_e;
	}
	close(tfd);
	tfd = -1;

	RTL_CHECK(rtlsdr_set_center_freq,(dev, center_freq));

	err:
	if(tfd >= 0)
		close(tfd);
	return NULL;
}


float complex decim1[DECIM1];
msresamp_crcf decim1_q;
firhilbf hilbert_q;
int audiopipe = 3;
int outsamples = 0;

#define DECIM1_BUF 2048
#define DECIM1_OUTBUF (DECIM1_BUF/DECIM1 + 1)
static void rtlsdr_callback(unsigned char *buf, uint32_t len, void *arg) {
	(void)arg;
	float complex dec_in[DECIM1_BUF], dec_out[DECIM1_OUTBUF];
	float audio_out[2*DECIM1_OUTBUF];
	unsigned int dec_in_n, i, num_out=0;
	if(do_exit) {
		printf("Canceling\n");
		rtlsdr_cancel_async(dev);
		return;
	}

	dec_in_n = len/2;
	assert(dec_in_n <= DECIM1_BUF);
	for(i = 0; i < dec_in_n; i++) {
		dec_in[i] = ((float)buf[2*i] - 127.4f) + I*((float)buf[2*i+1] - 127.4f);
	}

	msresamp_crcf_execute(decim1_q, dec_in, dec_in_n, dec_out, &num_out);
	assert(num_out <= DECIM1_OUTBUF);

	for(i = 0; i < num_out; i++) {
		float complex in = dec_out[i];

		// hilbert transform seems to swap halves of spectrum, fix it here:
		if(outsamples & 1)
			in = -in;

		firhilbf_interp_execute(hilbert_q, in, audio_out + 2*i);
		outsamples++;
	}
	write(audiopipe, audio_out, 2*num_out*sizeof(float));
}


int main() {
	int idx = 0;
	int sample_rate = 2.4e6;
	int gain = 500;
	int blocksize = DECIM1_BUF*2;

	int ret;

	struct sigaction sigact;

	// setup signals
	sigact.sa_handler = sighandler;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;
	sigaction(SIGINT, &sigact, NULL);
	sigaction(SIGTERM, &sigact, NULL);
	sigaction(SIGQUIT, &sigact, NULL);
	sigaction(SIGPIPE, &sigact, NULL);

	// initialize liquid-dsp
	decim1_q = msresamp_crcf_create(1.0/DECIM1, 60.0);
	hilbert_q = firhilbf_create(50, 60.0);


	// initialize rtl-sdr
	RTL_CHECK(rtlsdr_open,(&dev, idx));
	RTL_CHECK(rtlsdr_set_sample_rate,(dev, sample_rate));
	RTL_CHECK(rtlsdr_set_dithering,(dev, 0));
	RTL_CHECK(rtlsdr_set_if_freq,(dev, if_freq));
	RTL_CHECK(rtlsdr_set_center_freq,(dev, center_freq));
	RTL_CHECK(rtlsdr_set_tuner_gain_mode,(dev, 1));
	RTL_CHECK(rtlsdr_set_tuner_gain,(dev, gain));

	// start
	pthread_create(&control_thrd, 0, control, NULL);

	RTL_CHECK(rtlsdr_reset_buffer,(dev));
	RTL_CHECK(rtlsdr_read_async,(dev, rtlsdr_callback, NULL, 0, blocksize));

	err:
	if(control_thrd)
		pthread_kill(control_thrd, SIGTERM);
	
	return 0;
}

