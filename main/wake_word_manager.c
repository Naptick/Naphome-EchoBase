#include "wake_word_manager.h"
#include "openwakeword_esp32.h"
#include "korvo1.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "wake_word_mgr";

static korvo1_t s_mic = {0};
static bool s_initialized = false;
static bool s_running = false;

// Wake word detection callback
static void on_wake_word_detected(const char *wake_word)
{
    ESP_LOGI(TAG, "*** WAKE WORD DETECTED: %s ***", wake_word);
    
    // TODO: Trigger Google Assistant or other action
    // Example:
    // - Start Google Assistant session
    // - Play acknowledgment sound
    // - Update LED status
    // - etc.
}

// Microphone audio capture task
static void mic_capture_task(void *pvParameters)
{
    const size_t buffer_size = 512; // 32ms at 16kHz
    int16_t *audio_buffer = (int16_t *)malloc(buffer_size * sizeof(int16_t));
    
    if (!audio_buffer) {
        ESP_LOGE(TAG, "Failed to allocate audio buffer");
        vTaskDelete(NULL);
        return;
    }
    
    ESP_LOGI(TAG, "Microphone capture task started");
    
    while (s_running) {
        size_t bytes_read = 0;
        esp_err_t ret = korvo1_read(&s_mic, audio_buffer, 
                                    buffer_size * sizeof(int16_t), 
                                    &bytes_read, 
                                    pdMS_TO_TICKS(100));
        
        if (ret == ESP_OK && bytes_read > 0) {
            size_t samples_read = bytes_read / sizeof(int16_t);
            
            // Process audio through OpenWakeWord
            openwakeword_process(audio_buffer, samples_read);
        } else if (ret != ESP_ERR_TIMEOUT) {
            ESP_LOGW(TAG, "Microphone read error: %s", esp_err_to_name(ret));
        }
    }
    
    free(audio_buffer);
    ESP_LOGI(TAG, "Microphone capture task stopped");
    vTaskDelete(NULL);
}

esp_err_t wake_word_manager_init(void)
{
    if (s_initialized) {
        ESP_LOGW(TAG, "Wake word manager already initialized");
        return ESP_OK;
    }
    
    // Initialize OpenWakeWord
    // OpenWakeWord typically uses 16kHz mono audio
    esp_err_t ret = openwakeword_init(16000, on_wake_word_detected);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize OpenWakeWord: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Initialize Korvo1 microphone
    // Korvo1 microphone uses PDM mode on I2S1
    korvo1_config_t mic_config = {
        .port = I2S_NUM_1,  // Use I2S1 for microphone (I2S0 is for speaker)
        .din_io_num = GPIO_NUM_35,  // PDM data input (check Korvo1 pinout)
        .bclk_io_num = GPIO_NUM_36, // PDM bit clock
        .ws_io_num = GPIO_NUM_37,   // PDM word select
        .mclk_io_num = GPIO_NUM_0,  // Master clock (if needed)
        .sample_rate_hz = 16000,    // 16kHz for wake word detection
        .dma_buffer_count = 4,
        .dma_buffer_len = 256,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT, // Mono
    };
    
    ret = korvo1_init(&s_mic, &mic_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize microphone: %s", esp_err_to_name(ret));
        openwakeword_deinit();
        return ret;
    }
    
    s_initialized = true;
    ESP_LOGI(TAG, "Wake word manager initialized");
    return ESP_OK;
}

esp_err_t wake_word_manager_start(void)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (s_running) {
        return ESP_OK;
    }
    
    // Start OpenWakeWord
    esp_err_t ret = openwakeword_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start OpenWakeWord: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Start microphone
    ret = korvo1_start(&s_mic);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start microphone: %s", esp_err_to_name(ret));
        openwakeword_stop();
        return ret;
    }
    
    // Create microphone capture task
    s_running = true;
    TaskHandle_t task_handle;
    xTaskCreate(
        mic_capture_task,
        "mic_capture",
        4096,
        NULL,
        5,
        &task_handle
    );
    
    if (!task_handle) {
        ESP_LOGE(TAG, "Failed to create microphone capture task");
        korvo1_stop(&s_mic);
        openwakeword_stop();
        s_running = false;
        return ESP_ERR_NO_MEM;
    }
    
    ESP_LOGI(TAG, "Wake word detection started");
    return ESP_OK;
}

void wake_word_manager_stop(void)
{
    if (!s_running) {
        return;
    }
    
    s_running = false;
    
    // Stop microphone
    korvo1_stop(&s_mic);
    
    // Stop OpenWakeWord
    openwakeword_stop();
    
    // Task will exit on its own when s_running becomes false
    vTaskDelay(pdMS_TO_TICKS(200));
    
    ESP_LOGI(TAG, "Wake word detection stopped");
}

bool wake_word_manager_is_active(void)
{
    return s_running;
}

void wake_word_manager_deinit(void)
{
    wake_word_manager_stop();
    
    if (s_initialized) {
        korvo1_deinit(&s_mic);
        openwakeword_deinit();
        s_initialized = false;
    }
    
    ESP_LOGI(TAG, "Wake word manager deinitialized");
}
