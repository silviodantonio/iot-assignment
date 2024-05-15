#ifndef H_SAMPLING_MODULE
#define H_SAMPLING_MODULE

#include <stdbool.h>

void adc_configure();
void adc_priming_samples_buf(int *samples_buf);
void adc_sampling_start(int *samples_buf, bool log);

int window_avg(int *array_buf, const unsigned int buf_len, unsigned int tail_pos, const unsigned int window_len); 

#endif
