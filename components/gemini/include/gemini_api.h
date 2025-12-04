#pragma once

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Gemini API configuration
 */
typedef struct {
    char api_key[128];  // Google Gemini API key
    char model[64];     // Model name (e.g., "gemini-1.5-flash" or "gemini-1.5-pro")
} gemini_config_t;

/**
 * Initialize Gemini API client
 * @param config: API configuration
 * @return ESP_OK on success
 */
esp_err_t gemini_api_init(const gemini_config_t *config);

/**
 * Speech-to-Text: Convert audio to text using Gemini
 * @param audio_data: PCM audio samples (16-bit, 16kHz mono)
 * @param audio_len: Number of samples
 * @param text_out: Buffer to store transcribed text
 * @param text_len: Size of text buffer
 * @return ESP_OK on success
 */
esp_err_t gemini_stt(const int16_t *audio_data, size_t audio_len, char *text_out, size_t text_len);

/**
 * LLM: Send text prompt and get response
 * @param prompt: Input text prompt
 * @param response: Buffer to store LLM response
 * @param response_len: Size of response buffer
 * @return ESP_OK on success
 */
esp_err_t gemini_llm(const char *prompt, char *response, size_t response_len);

/**
 * Text-to-Speech: Convert text to audio using Gemini
 * @param text: Text to synthesize
 * @param audio_out: Buffer to store PCM audio samples
 * @param audio_len: Size of audio buffer (in samples)
 * @param samples_written: Number of samples actually written
 * @return ESP_OK on success
 */
esp_err_t gemini_tts(const char *text, int16_t *audio_out, size_t audio_len, size_t *samples_written);

/**
 * Deinitialize Gemini API client
 */
void gemini_api_deinit(void);

#ifdef __cplusplus
}
#endif
