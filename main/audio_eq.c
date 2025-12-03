#include "audio_eq.h"
#include "esp_log.h"
#include "esp_err.h"
#include <inttypes.h>
#include <math.h>

static const char *TAG = "audio_eq";

// PI constant
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void biquad_init(biquad_filter_t *f, float b0, float b1, float b2, float a1, float a2)
{
    f->b0 = b0;
    f->b1 = b1;
    f->b2 = b2;
    f->a1 = a1;
    f->a2 = a2;
    f->z1 = 0.0f;
    f->z2 = 0.0f;
}

void biquad_reset(biquad_filter_t *f)
{
    f->z1 = 0.0f;
    f->z2 = 0.0f;
}

void biquad_hpf_coeffs(float fc, float fs, float q, float *b0, float *b1, float *b2, float *a1, float *a2)
{
    float w = 2.0f * M_PI * fc / fs;
    float cos_w = cosf(w);
    float sin_w = sinf(w);
    float alpha = sin_w / (2.0f * q);
    
    float b0_calc = (1.0f + cos_w) / 2.0f;
    float b1_calc = -(1.0f + cos_w);
    float b2_calc = (1.0f + cos_w) / 2.0f;
    float a0 = 1.0f + alpha;
    float a1_calc = -2.0f * cos_w;
    float a2_calc = 1.0f - alpha;
    
    // Normalize by a0
    *b0 = b0_calc / a0;
    *b1 = b1_calc / a0;
    *b2 = b2_calc / a0;
    *a1 = a1_calc / a0;
    *a2 = a2_calc / a0;
}

void biquad_peak_coeffs(float fc, float fs, float gain_db, float q, float *b0, float *b1, float *b2, float *a1, float *a2)
{
    float w = 2.0f * M_PI * fc / fs;
    float cos_w = cosf(w);
    float sin_w = sinf(w);
    
    float A = powf(10.0f, gain_db / 40.0f);  // Convert dB to linear gain
    float alpha = sin_w / (2.0f * q);
    
    float b0_calc = 1.0f + alpha * A;
    float b1_calc = -2.0f * cos_w;
    float b2_calc = 1.0f - alpha * A;
    float a0 = 1.0f + alpha / A;
    float a1_calc = -2.0f * cos_w;
    float a2_calc = 1.0f - alpha / A;
    
    // Normalize by a0
    *b0 = b0_calc / a0;
    *b1 = b1_calc / a0;
    *b2 = b2_calc / a0;
    *a1 = a1_calc / a0;
    *a2 = a2_calc / a0;
}

esp_err_t audio_eq_init(audio_eq_t *eq, uint32_t sample_rate, bool enabled)
{
    if (eq == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    eq->enabled = enabled;
    eq->global_gain = 0.707f;  // -3 dB
    
    if (!enabled) {
        ESP_LOGI(TAG, "EQ disabled");
        return ESP_OK;
    }
    
    if (sample_rate != 48000) {
        ESP_LOGW(TAG, "EQ coefficients calculated for 48 kHz, but sample rate is %" PRIu32 " Hz", sample_rate);
        // Still proceed, but warn that coefficients may not be optimal
    }
    
    // Calculate coefficients for each filter
    float b0, b1, b2, a1, a2;
    
    // High-pass filter at 90 Hz, Q=0.7
    biquad_hpf_coeffs(90.0f, (float)sample_rate, 0.7f, &b0, &b1, &b2, &a1, &a2);
    biquad_init(&eq->hpf_90hz, b0, b1, b2, a1, a2);
    
    // Peaking EQ at 320 Hz, -4 dB, Q=1.0
    biquad_peak_coeffs(320.0f, (float)sample_rate, -4.0f, 1.0f, &b0, &b1, &b2, &a1, &a2);
    biquad_init(&eq->peak_320hz, b0, b1, b2, a1, a2);
    
    // Peaking EQ at 500 Hz, -2 dB, Q=1.0
    biquad_peak_coeffs(500.0f, (float)sample_rate, -2.0f, 1.0f, &b0, &b1, &b2, &a1, &a2);
    biquad_init(&eq->peak_500hz, b0, b1, b2, a1, a2);
    
    ESP_LOGI(TAG, "EQ initialized: HPF@90Hz, Peak@320Hz(-4dB), Peak@500Hz(-2dB), Gain=-3dB");
    ESP_LOGI(TAG, "Sample rate: %" PRIu32 " Hz", sample_rate);
    
    return ESP_OK;
}

void audio_eq_reset(audio_eq_t *eq)
{
    if (eq == NULL) return;
    
    biquad_reset(&eq->hpf_90hz);
    biquad_reset(&eq->peak_320hz);
    biquad_reset(&eq->peak_500hz);
}

float audio_eq_process(audio_eq_t *eq, int channel, float sample)
{
    if (eq == NULL || !eq->enabled) {
        return sample;
    }
    
    // Select the appropriate filter set for this channel
    // For now, we use the same filters for both channels
    // In the future, we could have separate filter sets per channel
    biquad_filter_t *hpf = &eq->hpf_90hz;
    biquad_filter_t *peak320 = &eq->peak_320hz;
    biquad_filter_t *peak500 = &eq->peak_500hz;
    
    // Process through filter chain
    float x = sample;
    x = biquad_process(hpf, x);
    x = biquad_process(peak320, x);
    x = biquad_process(peak500, x);
    
    // Apply global gain
    x *= eq->global_gain;
    
    return x;
}
