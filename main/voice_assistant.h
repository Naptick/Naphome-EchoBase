#pragma once

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Voice assistant configuration
 */
typedef struct {
    char gemini_api_key[128];  // Google Gemini API key
    char gemini_model[64];     // Gemini model name (default: "gemini-1.5-flash")
} voice_assistant_config_t;

/**
 * Initialize voice assistant
 * @param config: Configuration (API key, model, etc.)
 * @return ESP_OK on success
 */
esp_err_t voice_assistant_init(const voice_assistant_config_t *config);

/**
 * Start voice assistant
 * This begins listening for wake words and processing voice commands
 * @return ESP_OK on success
 */
esp_err_t voice_assistant_start(void);

/**
 * Stop voice assistant
 */
void voice_assistant_stop(void);

/**
 * Process a voice command manually (for testing)
 * @param audio_data: PCM audio samples (16-bit, 16kHz mono)
 * @param audio_len: Number of samples
 * @return ESP_OK on success
 */
esp_err_t voice_assistant_process_command(const int16_t *audio_data, size_t audio_len);

/**
 * Check if voice assistant is active
 * @return true if active
 */
bool voice_assistant_is_active(void);

/**
 * Deinitialize voice assistant
 */
void voice_assistant_deinit(void);

/**
 * Test TTS functionality - generate and play audio from text
 * @param text: Text to synthesize and play
 * @return ESP_OK on success
 */
esp_err_t voice_assistant_test_tts(const char *text);

#ifdef __cplusplus
}
#endif
