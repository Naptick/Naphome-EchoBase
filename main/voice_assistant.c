#include "voice_assistant.h"
#include "gemini_api.h"
#include "wake_word_manager.h"
#include "audio_player.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "voice_assistant";

static voice_assistant_config_t s_config = {0};
static bool s_initialized = false;
static bool s_active = false;
static TaskHandle_t s_assistant_task = NULL;
static QueueHandle_t s_command_queue = NULL;

// Voice command processing task
static void assistant_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Voice assistant task started");
    
    while (s_active) {
        // Wait for wake word detection or manual command
        // For now, we'll process commands from the queue
        // In a full implementation, this would be triggered by wake word detection
        vTaskDelay(pdMS_TO_TICKS(100));
        
        // TODO: Get audio from wake word detection or queue
        // This is a placeholder - actual implementation would:
        // 1. Wait for wake word detection
        // 2. Record audio after wake word
        // 3. Process through STT -> LLM -> TTS -> Playback
    }
    
    ESP_LOGI(TAG, "Voice assistant task stopped");
    vTaskDelete(NULL);
}

// Wake word callback - triggered when wake word is detected
__attribute__((unused)) static void on_wake_word_detected(const char *wake_word)
{
    ESP_LOGI(TAG, "Wake word '%s' detected - starting voice command capture", wake_word);
    
    // TODO: Start recording audio for command
    // For now, this is a placeholder
    // In full implementation:
    // 1. Start recording from microphone
    // 2. Record for ~5 seconds or until silence detected
    // 3. Send to voice_assistant_process_command()
}

// Process complete voice command: STT -> LLM -> TTS -> Playback
static esp_err_t process_voice_command(const int16_t *audio_data, size_t audio_len)
{
    ESP_LOGI(TAG, "Processing voice command (%zu samples)", audio_len);
    
    // Step 1: Speech-to-Text
    char transcribed_text[512];
    esp_err_t ret = gemini_stt(audio_data, audio_len, transcribed_text, sizeof(transcribed_text));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "STT failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "Transcribed: %s", transcribed_text);
    
    // Step 2: LLM - Get response from Gemini
    char llm_response[2048];
    ret = gemini_llm(transcribed_text, llm_response, sizeof(llm_response));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LLM failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "LLM response: %s", llm_response);
    
    // Step 3: Text-to-Speech
    const size_t tts_buffer_size = 48000; // ~2 seconds at 24kHz
    int16_t *tts_audio = (int16_t *)malloc(tts_buffer_size * sizeof(int16_t));
    if (!tts_audio) {
        ESP_LOGE(TAG, "Failed to allocate TTS buffer");
        return ESP_ERR_NO_MEM;
    }
    
    size_t samples_written = 0;
    ret = gemini_tts(llm_response, tts_audio, tts_buffer_size, &samples_written);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "TTS failed: %s", esp_err_to_name(ret));
        free(tts_audio);
        return ret;
    }
    
    ESP_LOGI(TAG, "TTS generated %zu samples", samples_written);
    
    // Step 4: Play audio response
    // Note: TTS typically outputs at 24kHz, but our audio player may be at 48kHz
    // We'll need to resample or configure TTS for 48kHz
    ret = audio_player_submit_pcm(tts_audio, samples_written, 24000, 1); // Mono, 24kHz
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Audio playback failed: %s", esp_err_to_name(ret));
    }
    
    free(tts_audio);
    return ESP_OK;
}

esp_err_t voice_assistant_init(const voice_assistant_config_t *config)
{
    if (!config || strlen(config->gemini_api_key) == 0) {
        ESP_LOGE(TAG, "Invalid voice assistant configuration");
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(&s_config, config, sizeof(voice_assistant_config_t));
    
    // Initialize Gemini API
    gemini_config_t gemini_cfg = {
        .api_key = {0},
        .model = {0}
    };
    strncpy(gemini_cfg.api_key, config->gemini_api_key, sizeof(gemini_cfg.api_key) - 1);
    strncpy(gemini_cfg.model, config->gemini_model, sizeof(gemini_cfg.model) - 1);
    
    if (strlen(gemini_cfg.model) == 0) {
        strncpy(gemini_cfg.model, "gemini-2.0-flash", sizeof(gemini_cfg.model) - 1);
    }
    
    esp_err_t ret = gemini_api_init(&gemini_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize Gemini API: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Create command queue
    s_command_queue = xQueueCreate(4, sizeof(size_t)); // Store audio buffer pointers
    if (!s_command_queue) {
        ESP_LOGE(TAG, "Failed to create command queue");
        gemini_api_deinit();
        return ESP_ERR_NO_MEM;
    }
    
    s_initialized = true;
    ESP_LOGI(TAG, "Voice assistant initialized");
    return ESP_OK;
}

esp_err_t voice_assistant_start(void)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (s_active) {
        return ESP_OK;
    }
    
    s_active = true;
    
    // Create assistant task
    xTaskCreate(
        assistant_task,
        "voice_assistant",
        8192,
        NULL,
        5,
        &s_assistant_task
    );
    
    if (!s_assistant_task) {
        ESP_LOGE(TAG, "Failed to create assistant task");
        s_active = false;
        return ESP_ERR_NO_MEM;
    }
    
    ESP_LOGI(TAG, "Voice assistant started");
    return ESP_OK;
}

void voice_assistant_stop(void)
{
    if (!s_active) {
        return;
    }
    
    s_active = false;
    
    if (s_assistant_task) {
        vTaskDelay(pdMS_TO_TICKS(200));
        vTaskDelete(s_assistant_task);
        s_assistant_task = NULL;
    }
    
    ESP_LOGI(TAG, "Voice assistant stopped");
}

esp_err_t voice_assistant_process_command(const int16_t *audio_data, size_t audio_len)
{
    if (!s_initialized || !s_active) {
        return ESP_ERR_INVALID_STATE;
    }
    
    return process_voice_command(audio_data, audio_len);
}

bool voice_assistant_is_active(void)
{
    return s_active;
}

void voice_assistant_deinit(void)
{
    voice_assistant_stop();
    
    if (s_command_queue) {
        vQueueDelete(s_command_queue);
        s_command_queue = NULL;
    }
    
    gemini_api_deinit();
    s_initialized = false;
    
    ESP_LOGI(TAG, "Voice assistant deinitialized");
}

// Streaming TTS playback callback
// Called with decoded PCM audio chunks as they arrive from the API
static esp_err_t tts_playback_callback(const int16_t *samples, size_t sample_count, void *user_data)
{
    // user_data is unused, but could be used to pass state if needed
    (void)user_data;

    if (!samples || sample_count == 0) {
        return ESP_OK;
    }

    // Submit PCM chunk directly to audio player (24kHz, mono)
    // This avoids buffering entire audio in memory
    return audio_player_submit_pcm(samples, sample_count, 24000, 1);
}

// Test TTS function - generate and play audio from text using streaming
esp_err_t voice_assistant_test_tts(const char *text)
{
    if (!s_initialized) {
        ESP_LOGE(TAG, "Voice assistant not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "🎤 Testing TTS with text: \"%s\"", text);

    // Check available memory for diagnostics
    size_t total_free = esp_get_free_heap_size();
    size_t largest_default = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);

    ESP_LOGI(TAG, "Memory before streaming TTS: Total free=%lu bytes, Largest block=%lu bytes",
             (unsigned long)total_free, (unsigned long)largest_default);

    // Use streaming TTS - audio chunks are decoded and played as they arrive
    // No need to allocate 80KB buffer for entire audio
    esp_err_t ret = gemini_tts_streaming(text, tts_playback_callback, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Streaming TTS failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "✅ Streaming TTS completed successfully");

    // Log free memory after TTS
    size_t free_heap_after = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
    ESP_LOGI(TAG, "Memory after streaming TTS: %lu bytes free", (unsigned long)free_heap_after);

    return ESP_OK;
}
