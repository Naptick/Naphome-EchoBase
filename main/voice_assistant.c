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

// Test TTS function - generate and play audio from text
esp_err_t voice_assistant_test_tts(const char *text)
{
    if (!s_initialized) {
        ESP_LOGE(TAG, "Voice assistant not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGI(TAG, "🎤 Testing TTS with text: \"%s\"", text);
    
    // Allocate buffer for TTS audio (24kHz, mono, ~3 seconds max)
    // Reduced to fit in available internal RAM (192KB largest block)
    const size_t tts_buffer_size = 72000; // 3 seconds at 24kHz = 144KB
    size_t buffer_bytes = tts_buffer_size * sizeof(int16_t);
    
    int16_t *tts_audio = NULL;
    
    // Check available memory for diagnostics
    size_t total_free = esp_get_free_heap_size();
    size_t internal_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    size_t spiram_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    size_t largest_internal = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
    size_t largest_spiram = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
    size_t largest_default = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);
    
    ESP_LOGI(TAG, "Memory check: Total free=%lu bytes, Need=%lu bytes", 
             (unsigned long)total_free, (unsigned long)buffer_bytes);
    ESP_LOGI(TAG, "  Internal: free=%lu, largest=%lu", 
             (unsigned long)internal_free, (unsigned long)largest_internal);
    ESP_LOGI(TAG, "  PSRAM: free=%lu, largest=%lu", 
             (unsigned long)spiram_free, (unsigned long)largest_spiram);
    ESP_LOGI(TAG, "  Default: largest=%lu", (unsigned long)largest_default);
    
    // Try PSRAM first (if available and large enough)
    if (spiram_free >= buffer_bytes && largest_spiram >= buffer_bytes) {
        ESP_LOGI(TAG, "Attempting PSRAM allocation...");
        tts_audio = (int16_t *)heap_caps_malloc(buffer_bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (tts_audio) {
            ESP_LOGI(TAG, "✅ TTS buffer allocated from PSRAM: %lu bytes", (unsigned long)buffer_bytes);
        } else {
            ESP_LOGW(TAG, "PSRAM allocation failed, trying default heap...");
        }
    }
    
    // Fall back to default heap (which may include PSRAM if configured)
    if (!tts_audio) {
        tts_audio = (int16_t *)heap_caps_malloc(buffer_bytes, MALLOC_CAP_8BIT);
        if (tts_audio) {
            ESP_LOGI(TAG, "✅ TTS buffer allocated from default heap: %lu bytes", (unsigned long)buffer_bytes);
        }
    }
    
    // Last resort: try regular malloc
    if (!tts_audio) {
        tts_audio = (int16_t *)malloc(buffer_bytes);
        if (tts_audio) {
            ESP_LOGI(TAG, "✅ TTS buffer allocated via malloc: %lu bytes", (unsigned long)buffer_bytes);
        }
    }
    
    if (!tts_audio) {
        ESP_LOGE(TAG, "Failed to allocate TTS buffer (%lu bytes)", (unsigned long)buffer_bytes);
        ESP_LOGE(TAG, "Total free heap: %lu bytes, Largest free block: %lu bytes", 
                 (unsigned long)total_free, (unsigned long)largest_default);
        return ESP_ERR_NO_MEM;
    }
    
    size_t samples_written = 0;
    esp_err_t ret = gemini_tts(text, tts_audio, tts_buffer_size, &samples_written);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "TTS generation failed: %s", esp_err_to_name(ret));
        free(tts_audio);
        return ret;
    }
    
    ESP_LOGI(TAG, "✅ TTS generated %zu samples (%.2f seconds at 24kHz)", 
             samples_written, (float)samples_written / 24000.0f);
    
    // Play audio through audio player
    // audio_player_submit_pcm automatically handles sample rate conversion via ensure_sample_rate
    ret = audio_player_submit_pcm(tts_audio, samples_written, 24000, 1); // Mono, 24kHz
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Audio playback submission failed: %s", esp_err_to_name(ret));
        ESP_LOGW(TAG, "Note: TTS outputs 24kHz, but player may expect 48kHz - resampling may be needed");
    } else {
        ESP_LOGI(TAG, "✅ TTS audio submitted for playback");
    }
    
    // Free the buffer
    free(tts_audio);
    return ret;
}
