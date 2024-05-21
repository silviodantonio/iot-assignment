# Assignment for IoT Algorithm and Services (2023/2024)

This document contains:
- How to run the code
- Some details and considerations on the implementation.
- Evaluation of current consumption and communication latencies

## Running the code

The header file `project_constants.h` can be used to set up some of the
constants used in the project such as: wifi ssid and password, adc unit and
channel, duration of sampling time windw.

For running the code is required to have an installation of the ESP-IDF framework.

For running the project should be enough to:
- Clone the repository and open a terminal inside it
- If not already done, source the `export.sh` file found in the
esp-idf installtion folder `esp/esp-idf/`.
- Set the target with `idf.py set-target <target chip>`. For the development i've
been using an ESP32-S3.
- Connect the board
- Compile, flash and open the serial monitor with `idf.py flash monitor`. Most of
the work should be done automatically


## Project overview

Roughly speaking, the steps that are carried out in the code are:
- Initialization of WIFI and MQTT modules
- Configuration of the ADC for sampling an analog signal in input
- Estimating the maximum sampling frequency
- Executing the FFT for obtaining the frequency with highest magnitude
- Adjust the sampling frequency accordingly
- Start the continuous sampling loop
- Continuously compute the average of the samples over a fixed time window
- Periodically send these values to an MQTT broker

This project has been developed with an [Heltec WiFi LoRa 32 V3](https://heltec.org/project/wifi-lora-32-v3/)
board, that is equipped with an ESP32-S3.

Follows a more detailed explanations of key functionalities and known issues
and limitations of their implementation.

### Sampling

In this section i will show what is being said in the [TRM](https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf)
(Technical Reference Manual) of the chip about the maximum sampling
capabilities, and showing what are the performances of my implementation.
At the end of this section i will discuss some improvements of it.

#### The theory

Looking at the ADC section of the TRM, we can see that the chip is equipped
with two ADC units, each of them can be controlled by two controllers. One for
low power and one for high performances. For each these controllers different
clock sources can be chosen.
By requirements of the assignment, i will focus the discussion on finding the
limits of such controllers.

The High performance controller is called DIG ADC1. [In this section of the TRM](https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf#paragraph.39.3.7.1)
it is stated that "The ADC needs 25 DIGADC_SARCLK clock cycles per sample, so
the maximum sampling rate is limited by the DIGADC_SARCLK frequency".
The DIGADC_SARCKL is the operating clock of DIG ADC1 and its value is obtained
by a chain of divisions:
- DIGADC_SARCLK is divided from DIGADC_CLK
- DIGADC_CLK is divided from PLL_D2_CLK

For each of these divisions, the divider can be configured and there are
no indications that these dividers cannot be set to 1, making effectively
the value of DIGADC_SARCLK the same as PLL_D2_CLK.

The value of PLL_D2_CLK is relevant since it can be declared as the clock
source of the DIG ADC1 controller.
PLL_D2_CLK is half the value of the internal clock PLL_CLK that
has a (configurable) max frequency of 480 MHz. This means that PLL_D2_CLK
defines a clock of 240 MHz, that is also the maximum value of DIGADC_SARCLK.

Getting back to the sampling frequency limit, at 240 MHz one clock cycle is
about 0,004 uS. For one sample it takes 25 * 0,004 uS which is 0,1 uS. This means
that the (theoretical) maximum sampling frequency is about 1 / 0,1 uS which is **10 MHz**.

Just for completeness, if the frequency of DIGADC_SARCLK is higher than 5 MHz
the sampling precision is lowered.

#### In practice

For the implementation of the component that does the sampling i've
used the the ADC in ["Oneshot mode"](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/adc_oneshot.html).
This modes uses the digital controller and i've configured it in order to use the fastest clock:
PLL_D2.

For taking continuous measurements, the function call used to take a single
sample is called inside an endless loop.

I approximatively measured the maximium sampling frequency of this approach
taking a fixed amount of samples and getting the tick count from FreeRTOS before
and after the sampling cycle.
After having converted the tick count to a time value, knowing the amount of
samples that i took could compute the sampling frequency, which is around
**30 kHz**. This sampling frequency is useful to sample input signals that have
a maximum a frequency of 15 kHz.
The accuracy of this measurements are limited by the accuracy of the FreeRTOS tick
count function.

#### Improvements

The approach i've used is upper bounded by the speed at which the cpu executes
the sampling loop. An improvement could be to set the CPU at maximum clock speed,
that is 240MHz. The default setting is 20MHz, [as declared in the
TRM](https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf#paragraph.7.2.4.1).
Another approach could be to move the driver from oneshot mode to [continuos mode](https://docs.espressif.com/projects/esp-idf/en/v5.2.1/esp32s3/api-reference/peripherals/adc_continuous.html).
The ADC driver can be configured to work at a specified frequency in Hz. The range of
this value is declared in the `soc/soc_caps.h` header.
On my chip reading the `SOC_ADC_SAMPLE_FREQ_THRES_HIGH` constant gave me a maximum
frequency of about **83 kHz**.
However, my guess is that interfacing directly with system registers is the way
to get the maximum performance possible.

### Adjusting sampling frequency

At startup the device samples the input signal at maximum frequency, saving the
values into a temporary buffer. This buffer is then passed as a parameter to
a function that used the FFT that returs the frequency that has the highest
magnitude.

For correctly sampling a signal, the frequency at which samples are
taken must be at least 2x the maximum frequency.
So the new sampling frequency is the value returned by this function multiplied
by 2 plus a 20% safety factor.

The new sampling frequency is applied by changing the rate at wich the sampling
loop is executed, using vTaskDelay.
Starting from the required sampling frequency, the sampling period is computed,
then converted in mS. The formula used is: `delay = (unsigned int)(1000/sampling_freq_hz)`
This value is then converted in tick counts and passed to vTaskDelay.
For minimizing rounding errors all these calculations are done with float
numbers and converted to int just at the end.


#### Issues

When testing the FFT and sampling capabilities in general i've used audio
signals generated by an audio editing program (Audacity) fed into the MCU
through an audio cable.

- Signals with frequency below 1 kHz are not detected correctly.

This has been measuerd through testing. Following this observation if
the frequency returned by the FFT is less than 1 kHz it automatically discarded
and 1 kHz is returned.

- Adjusting sampling frequency does not work properly

When computing the delay for adjusting the sampling frequency, the accuracy is poor.
That's because all sampling frequencies above 1 kHz produce the same 0 delay value
(check the formula used above). This, plus the fact that the fft doesn't detect
correctly signals below 1 kHz means that unfortunately the resulting sampling
frequency always the same.

- The function that uses the FFT returns the frequency that has the maximum magnitude,
that is not necessarily the component of the signal that has the maximum frequency.

This means that if the samples are taken from a signal that is composed of two
sine waves, one with high frequency and low amplitude and one with lower
frequency and high amplitude, the function returns just the frequency of the
signal with higer amplitude. However i've empirically measured that when overlapping
two signals with the same volume the functon still returns the correct value
(i.e.: the frequency of the higher frequency signal)

- Signal that changes significantly over time

Since the adjusted sampling frequency is calculated only once at startup,
if the signal varies greatly in frequency over time, that frequency could be
either completely wrong or doesn't provide any energy consumption benefits.

### Transmitting average values

Samples produced by the sampling task are stored in a message buffer.
The task that computes the average reads from this message buffer and computes the average
over a fixed time window. For example: if the time window is 1 second, then an
average value is produced every second.

The number of samples that the average function should read is computed at
runtime using: `samples_num = sampling_frequency * time_window`

The value is then sent to an MQTT broker.
The process is repeated every `time_window` seconds, in order to wait for the
sampling task to produce an adequate number of samples.

## Performance evaluation

### Energy consumption

- How i did it
- Issues
- Results

### MQTT message RTT

- How i did it
- Results

