#ifndef H_SAMPLING_MODULE
#define H_SAMPLING_MODULE

#include <stdbool.h>

void adc_configure();
void adc_priming_samples_buf(int *samples_buf);
void adc_sampling_once(int *samples_buf, float sampling_freq, bool log);
void adc_sampling_loop(int *samples_buf, float sampling_freq, bool log);

float get_max_freq(int *adc_buf, float sampling_freq);
void get_min_max(int *samples_buf, int *min_val, int *max_val);

int window_avg(int *array_buf, const unsigned int buf_len, unsigned int tail_pos, const unsigned int window_len); 

#endif
