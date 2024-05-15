#ifndef H_SAMPLING_MODULE
#define H_SAMPLING_MODULE

void adc_configure();
void adc_priming_samples_buf(int *samples_buf);
void adc_sampling_start(int *samples_buf);

int window_avg(int *array_buf, const unsigned int buf_len, unsigned int tail_pos, const unsigned int window_len); 

#endif
