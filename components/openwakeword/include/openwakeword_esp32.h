#pragma once

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Wake word detection callback
 * Called when a wake word is detected
 */
typedef void (*wake_word_callback_t)(const char *wake_word);

/**
 * Initialize OpenWakeWord for ESP32
 * @param sample_rate: Audio sample rate (typically 16000 Hz)
 * @param callback: Callback function called when wake word is detected
 * @return ESP_OK on success
 */
esp_err_t openwakeword_init(uint32_t sample_rate, wake_word_callback_t callback);

/**
 * Process audio samples through wake word detection
 * @param audio_data: 16-bit PCM audio samples (mono)
 * @param num_samples: Number of samples to process
 * @return ESP_OK on success
 */
esp_err_t openwakeword_process(const int16_t *audio_data, size_t num_samples);

/**
 * Start wake word detection
 * @return ESP_OK on success
 */
esp_err_t openwakeword_start(void);

/**
 * Stop wake word detection
 */
void openwakeword_stop(void);

/**
 * Check if wake word detection is running
 * @return true if running
 */
bool openwakeword_is_running(void);

/**
 * Deinitialize OpenWakeWord
 */
void openwakeword_deinit(void);

#ifdef __cplusplus
}
#endif
