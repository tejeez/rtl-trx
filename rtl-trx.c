#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/timerfd.h>
#include <rtl-sdr.h>

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

int main() {
	rtlsdr_dev_t *dev = NULL;
	int idx = 0;
	int sample_rate = 2.4e6;
	int if_freq = 3.6e6;
	long long center_freq = 868e6;
	int gain = 500;
	
	int tone1 = 1000, tone0 = 1220;
	int ret;
	
	RTL_CHECK(rtlsdr_open,(&dev, idx));
	RTL_CHECK(rtlsdr_set_sample_rate,(dev, sample_rate));
	RTL_CHECK(rtlsdr_set_dithering,(dev, 0));
	RTL_CHECK(rtlsdr_set_if_freq,(dev, if_freq));
	RTL_CHECK(rtlsdr_set_center_freq,(dev, center_freq));
	RTL_CHECK(rtlsdr_set_tuner_gain_mode,(dev, 1));
	RTL_CHECK(rtlsdr_set_tuner_gain,(dev, gain));
	RTL_CHECK(rtlsdr_reset_buffer,(dev));

	/*RTL_CHECK(rtlsdr_read_async,(dev, rtlsdr_callback, (void *)ds,
		      0, blocksize));*/

	long long txfreq, txfreq_tune;
	
	int tfd;
	tfd = timerfd_create(CLOCK_MONOTONIC, 0);
	
	struct itimerspec tfd_ts;
	tfd_ts.it_value.tv_sec = 0;
	tfd_ts.it_value.tv_nsec = 1;
	tfd_ts.it_interval.tv_sec = 0;
	tfd_ts.it_interval.tv_nsec = 1e9 / 45.0 + 0.5;
	ret = timerfd_settime(tfd, 0, &tfd_ts, NULL);
	//printf("%d\n", ret);
	
	int bit_i;
	
	bit_i = 0;
	while(/*bit_i < nbits*/1) {
		uint64_t tfd_e = 0;
		ret = read(tfd, &tfd_e, sizeof(uint64_t));
		if(ret < 0) break;
		//printf("%d %ld\n", ret, tfd_e);
		if(bits[bit_i % nbits] & 1)
			txfreq = center_freq + tone1;
		else
			txfreq = center_freq + tone0;
		txfreq_tune = txfreq / 4 - if_freq;
		RTL_CHECK(rtlsdr_set_center_freq,(dev, txfreq_tune));
		
		bit_i += tfd_e;
	}
	err:
	
	return 0;
}

