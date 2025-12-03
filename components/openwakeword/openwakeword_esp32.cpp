#include "openwakeword_esp32.h"
#include "esp_log.h"
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <string.h>
#include <cstdlib>
#include <cstring>

static const char *TAG = "openwakeword";

// OpenWakeWord integration
// Note: This is a placeholder implementation
// To use actual OpenWakeWord, you need to:
// 1. Add OpenWakeWord as a submodule or managed component
// 2. Include the OpenWakeWord headers
// 3. Link against the OpenWakeWord library

// For now, we'll create a stub that can be replaced with actual OpenWakeWord integration

struct openwakeword_context {
    uint32_t sample_rate;
    wake_word_callback_t callback;
    bool initialized;
    bool running;
    TaskHandle_t task_handle;
    QueueHandle_t audio_queue;
};

static openwakeword_context s_ctx = {
    .sample_rate = 0,
    .callback = nullptr,
    .initialized = false,
    .running = false,
    .task_handle = nullptr,
    .audio_queue = nullptr
};

// Placeholder for OpenWakeWord model
// In real implementation, this would be the actual model instance
static void *s_model = nullptr;

static void wake_word_task(void *pvParameters)
{
    openwakeword_context *ctx = (openwakeword_context *)pvParameters;
        int16_t *audio_buffer = (int16_t *)std::malloc(512 * sizeof(int16_t)); // 32ms at 16kHz
    
    if (!audio_buffer) {
        ESP_LOGE(TAG, "Failed to allocate audio buffer");
        vTaskDelete(NULL);
        return;
    }
    
    ESP_LOGI(TAG, "Wake word detection task started");
    
    while (ctx->running) {
        // Wait for audio data from queue
        if (xQueueReceive(ctx->audio_queue, audio_buffer, pdMS_TO_TICKS(100)) == pdTRUE) {
            // Process audio through OpenWakeWord
            // TODO: Replace with actual OpenWakeWord processing
            // Example:
            // float *features = preprocess_audio(audio_buffer, 512);
            // float confidence = openwakeword_predict(s_model, features);
            // if (confidence > threshold) {
            //     ctx->callback("hey_jarvis"); // or detected wake word
            // }
            
            // Placeholder: simulate wake word detection (remove in real implementation)
            static int sample_count = 0;
            sample_count += 512;
            if (sample_count > ctx->sample_rate * 3) { // Simulate detection every 3 seconds
                ESP_LOGI(TAG, "Wake word detected (simulated)");
                if (ctx->callback) {
                    ctx->callback("hey_jarvis");
                }
                sample_count = 0;
            }
        }
    }
    
    std::free(audio_buffer);
    ESP_LOGI(TAG, "Wake word detection task stopped");
    vTaskDelete(NULL);
}

esp_err_t openwakeword_init(uint32_t sample_rate, wake_word_callback_t callback)
{
    if (s_ctx.initialized) {
        ESP_LOGW(TAG, "OpenWakeWord already initialized");
        return ESP_OK;
    }
    
    if (sample_rate != 16000) {
        ESP_LOGW(TAG, "OpenWakeWord typically uses 16kHz, got %" PRIu32 " Hz", sample_rate);
    }
    
    s_ctx.sample_rate = sample_rate;
    s_ctx.callback = callback;
    s_ctx.initialized = true;
    s_ctx.running = false;
    
    // Create audio queue
    s_ctx.audio_queue = xQueueCreate(4, 512 * sizeof(int16_t));
    if (!s_ctx.audio_queue) {
        ESP_LOGE(TAG, "Failed to create audio queue");
        s_ctx.initialized = false;
        return ESP_ERR_NO_MEM;
    }
    
    // TODO: Load OpenWakeWord model
    // Example:
    // s_model = openwakeword_load_model("/spiffs/model.onnx");
    // if (!s_model) {
    //     ESP_LOGE(TAG, "Failed to load OpenWakeWord model");
    //     return ESP_ERR_NOT_FOUND;
    // }
    
    ESP_LOGI(TAG, "OpenWakeWord initialized (sample_rate=%" PRIu32 " Hz)", sample_rate);
    return ESP_OK;
}

esp_err_t openwakeword_process(const int16_t *audio_data, size_t num_samples)
{
    if (!s_ctx.initialized || !s_ctx.running) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (!audio_data || num_samples == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Send audio to processing task via queue
    // For simplicity, we'll process in chunks of 512 samples
    size_t samples_processed = 0;
    while (samples_processed < num_samples) {
        size_t chunk_size = (num_samples - samples_processed > 512) ? 512 : (num_samples - samples_processed);
        
        int16_t chunk[512];
        std::memcpy(chunk, audio_data + samples_processed, chunk_size * sizeof(int16_t));
        
        // Pad if needed
        if (chunk_size < 512) {
            std::memset(chunk + chunk_size, 0, (512 - chunk_size) * sizeof(int16_t));
        }
        
        // Send to queue (non-blocking)
        if (xQueueSend(s_ctx.audio_queue, chunk, 0) != pdTRUE) {
            // Queue full, drop this chunk
            ESP_LOGW(TAG, "Audio queue full, dropping chunk");
        }
        
        samples_processed += chunk_size;
    }
    
    return ESP_OK;
}

esp_err_t openwakeword_start(void)
{
    if (!s_ctx.initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (s_ctx.running) {
        return ESP_OK;
    }
    
    s_ctx.running = true;
    
    // Create wake word detection task
    xTaskCreate(
        wake_word_task,
        "wakeword",
        4096,
        &s_ctx,
        5,
        &s_ctx.task_handle
    );
    
    if (!s_ctx.task_handle) {
        ESP_LOGE(TAG, "Failed to create wake word task");
        s_ctx.running = false;
        return ESP_ERR_NO_MEM;
    }
    
    ESP_LOGI(TAG, "Wake word detection started");
    return ESP_OK;
}

void openwakeword_stop(void)
{
    if (!s_ctx.running) {
        return;
    }
    
    s_ctx.running = false;
    
    // Wait for task to finish
    if (s_ctx.task_handle) {
        vTaskDelay(pdMS_TO_TICKS(100));
        if (s_ctx.task_handle) {
            vTaskDelete(s_ctx.task_handle);
            s_ctx.task_handle = NULL;
        }
    }
    
    // Clear queue
    if (s_ctx.audio_queue) {
        xQueueReset(s_ctx.audio_queue);
    }
    
    ESP_LOGI(TAG, "Wake word detection stopped");
}

bool openwakeword_is_running(void)
{
    return s_ctx.running;
}

void openwakeword_deinit(void)
{
    openwakeword_stop();
    
    if (s_ctx.audio_queue) {
        vQueueDelete(s_ctx.audio_queue);
        s_ctx.audio_queue = NULL;
    }
    
    // TODO: Free OpenWakeWord model
    // if (s_model) {
    //     openwakeword_free_model(s_model);
    //     s_model = nullptr;
    // }
    
    std::memset(&s_ctx, 0, sizeof(s_ctx));
    ESP_LOGI(TAG, "OpenWakeWord deinitialized");
}
