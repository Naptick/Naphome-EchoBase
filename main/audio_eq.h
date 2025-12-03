#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// Biquad filter structure
typedef struct {
    float b0, b1, b2;  // Numerator coefficients
    float a1, a2;      // Denominator coefficients (a0 = 1.0 after normalization)
    float z1, z2;      // Delay elements (state)
} biquad_filter_t;

// EQ filter set (one per channel)
typedef struct {
    biquad_filter_t hpf_90hz;      // High-pass at 90 Hz
    biquad_filter_t peak_320hz;     // Peaking EQ at 320 Hz, -4 dB
    biquad_filter_t peak_500hz;     // Peaking EQ at 500 Hz, -2 dB
    float global_gain;              // Overall gain (-3 dB = 0.707)
    bool enabled;
} audio_eq_t;

/**
 * Initialize a biquad filter with coefficients
 */
void biquad_init(biquad_filter_t *f, float b0, float b1, float b2, float a1, float a2);

/**
 * Reset filter state (clear delay elements)
 */
void biquad_reset(biquad_filter_t *f);

/**
 * Process a single sample through a biquad filter
 */
static inline float biquad_process(biquad_filter_t *f, float x)
{
    float y = f->b0 * x + f->z1;
    f->z1 = f->b1 * x - f->a1 * y + f->z2;
    f->z2 = f->b2 * x - f->a2 * y;
    return y;
}

/**
 * Calculate biquad coefficients for a 2nd-order high-pass filter (Butterworth)
 * @param fc: Cutoff frequency in Hz
 * @param fs: Sample rate in Hz
 * @param q: Q factor (0.707 for Butterworth)
 */
void biquad_hpf_coeffs(float fc, float fs, float q, float *b0, float *b1, float *b2, float *a1, float *a2);

/**
 * Calculate biquad coefficients for a peaking EQ filter
 * @param fc: Center frequency in Hz
 * @param fs: Sample rate in Hz
 * @param gain_db: Gain in dB (negative for cut)
 * @param q: Q factor (bandwidth)
 */
void biquad_peak_coeffs(float fc, float fs, float gain_db, float q, float *b0, float *b1, float *b2, float *a1, float *a2);

/**
 * Initialize EQ with filters for 48 kHz sample rate
 * @param eq: EQ structure to initialize
 * @param sample_rate: Sample rate in Hz (must be 48000 for current coefficients)
 * @param enabled: Whether EQ is enabled
 */
esp_err_t audio_eq_init(audio_eq_t *eq, uint32_t sample_rate, bool enabled);

/**
 * Reset EQ state (clear all filter delays)
 */
void audio_eq_reset(audio_eq_t *eq);

/**
 * Process a single sample through the EQ chain
 * @param eq: EQ structure
 * @param channel: Channel index (0=left, 1=right)
 * @param sample: Input sample in range [-1.0, 1.0]
 * @return: Processed sample in range [-1.0, 1.0]
 */
float audio_eq_process(audio_eq_t *eq, int channel, float sample);

#ifdef __cplusplus
}
#endif
