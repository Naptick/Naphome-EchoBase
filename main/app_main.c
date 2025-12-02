#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "audio_player.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_strip.h"
#include "mp3_decoder.h"
#include "nvs_flash.h"

static const char *TAG = "korvo1_led_audio";

// LED strip handle
static led_strip_handle_t s_strip = NULL;

// Audio player configuration for korvo1
// Based on ES8311 codec on korvo1 board
// I2S pins per BSP: BCLK=GPIO40, LRCLK=GPIO41, DATA=GPIO39, MCLK=GPIO42
// I2C pins per BSP: SCL=GPIO2, SDA=GPIO1
static const audio_player_config_t s_audio_config = {
    .i2s_port = I2S_NUM_0,  // Korvo1 uses I2S0 for speaker
    .bclk_gpio = GPIO_NUM_40,  // BSP_I2S0_SCLK
    .lrclk_gpio = GPIO_NUM_41,  // BSP_I2S0_LCLK
    .data_gpio = GPIO_NUM_39,   // BSP_I2S0_DOUT
    .mclk_gpio = GPIO_NUM_42,   // BSP_I2S0_MCLK
    .i2c_scl_gpio = GPIO_NUM_2,  // BSP_I2C_SCL
    .i2c_sda_gpio = GPIO_NUM_1,  // BSP_I2C_SDA
    .default_sample_rate = CONFIG_AUDIO_SAMPLE_RATE,
};

// Apply brightness scaling to RGB values
static inline uint8_t apply_brightness(uint8_t value)
{
    return (uint16_t)value * CONFIG_LED_AUDIO_BRIGHTNESS / 255;
}

// Set a single LED pixel with brightness scaling
static void set_pixel_rgb(uint32_t index, uint8_t r, uint8_t g, uint8_t b)
{
    if (!s_strip || index >= CONFIG_LED_AUDIO_LED_COUNT) {
        return;
    }
    led_strip_set_pixel(s_strip, index,
                        apply_brightness(r),
                        apply_brightness(g),
                        apply_brightness(b));
}

// Generate a logarithmic frequency sweep (chirp) as PCM samples
static void generate_log_sweep(int16_t *samples, size_t num_samples, 
                                int sample_rate, 
                                float start_freq, float end_freq,
                                float duration_sec)
{
    const float two_pi = 2.0f * M_PI;
    const float log_start = logf(start_freq);
    const float log_end = logf(end_freq);
    const float log_range = log_end - log_start;
    const float sample_period = 1.0f / (float)sample_rate;
    const float amplitude = 0.3f; // 30% amplitude to avoid clipping
    const int16_t max_amplitude = (int16_t)(amplitude * 32767.0f);

    for (size_t i = 0; i < num_samples; i++) {
        float t = (float)i * sample_period;
        float normalized_t = t / duration_sec;
        
        // Logarithmic frequency sweep: f(t) = start * (end/start)^(t/T)
        float current_freq = start_freq * expf(log_range * normalized_t);
        
        // Generate sine wave at current frequency
        float phase = two_pi * current_freq * t;
        float sample = sinf(phase);
        
        // Convert to 16-bit integer
        samples[i] = (int16_t)(sample * max_amplitude);
    }
}

// Update LED animation based on audio playback progress
static void update_leds_for_audio(float progress, bool playing)
{
    if (!s_strip) {
        return;
    }

    if (!playing) {
        // All LEDs off when not playing
        for (uint32_t i = 0; i < CONFIG_LED_AUDIO_LED_COUNT; i++) {
            set_pixel_rgb(i, 0, 0, 0);
        }
        led_strip_refresh(s_strip);
        return;
    }

    // Animate LEDs based on progress: rainbow sweep
    // Calculate hue based on progress (0.0 to 1.0)
    float hue = fmodf(progress * 360.0f, 360.0f) / 360.0f;
    
    // Convert HSV to RGB
    float s = 1.0f; // Full saturation
    float v = 0.8f; // 80% brightness
    
    int hi = (int)(hue * 6.0f);
    float f = hue * 6.0f - hi;
    float p = v * (1.0f - s);
    float q = v * (1.0f - f * s);
    float t = v * (1.0f - (1.0f - f) * s);
    
    uint8_t r, g, b;
    switch (hi % 6) {
        case 0: r = (uint8_t)(v * 255); g = (uint8_t)(t * 255); b = (uint8_t)(p * 255); break;
        case 1: r = (uint8_t)(q * 255); g = (uint8_t)(v * 255); b = (uint8_t)(p * 255); break;
        case 2: r = (uint8_t)(p * 255); g = (uint8_t)(v * 255); b = (uint8_t)(t * 255); break;
        case 3: r = (uint8_t)(p * 255); g = (uint8_t)(q * 255); b = (uint8_t)(v * 255); break;
        case 4: r = (uint8_t)(t * 255); g = (uint8_t)(p * 255); b = (uint8_t)(v * 255); break;
        default: r = (uint8_t)(v * 255); g = (uint8_t)(p * 255); b = (uint8_t)(q * 255); break;
    }
    
    // Set all LEDs to the same color (or create a sweep effect)
    // Calculate active LEDs - use ceil to ensure proper rounding up
    // When progress is very close to 1.0, ensure all LEDs light up
    uint32_t active_leds;
    if (progress >= 1.0f) {
        active_leds = CONFIG_LED_AUDIO_LED_COUNT;
    } else {
        active_leds = (uint32_t)ceilf(progress * CONFIG_LED_AUDIO_LED_COUNT);
        if (active_leds > CONFIG_LED_AUDIO_LED_COUNT) {
            active_leds = CONFIG_LED_AUDIO_LED_COUNT;
        }
    }
    // Ensure at least one LED is active if progress > 0
    if (progress > 0.0f && active_leds == 0) {
        active_leds = 1;
    }
    for (uint32_t i = 0; i < CONFIG_LED_AUDIO_LED_COUNT; i++) {
        if (i < active_leds) {
            set_pixel_rgb(i, r, g, b);
        } else {
            set_pixel_rgb(i, 0, 0, 0);
        }
    }
    
    led_strip_refresh(s_strip);
}

// Play log sweep as PCM
static void play_log_sweep_pcm(void)
{
    const int sample_rate = CONFIG_AUDIO_SAMPLE_RATE;
    const float duration_sec = (float)CONFIG_LOG_SWEEP_DURATION_SEC;
    const float start_freq = (float)CONFIG_LOG_SWEEP_START_FREQ;
    const float end_freq = (float)CONFIG_LOG_SWEEP_END_FREQ;
    
    size_t total_samples = (size_t)(sample_rate * duration_sec);
    const size_t chunk_size = 1024; // Process in chunks
    
    ESP_LOGI(TAG, "Generating log sweep: %.1f Hz -> %.1f Hz over %.1f seconds",
             start_freq, end_freq, duration_sec);
    ESP_LOGI(TAG, "Total samples: %zu, sample rate: %d Hz", total_samples, sample_rate);
    
    int16_t *chunk_buffer = malloc(chunk_size * sizeof(int16_t));
    if (!chunk_buffer) {
        ESP_LOGE(TAG, "Failed to allocate chunk buffer");
        return;
    }
    
    size_t samples_played = 0;
    while (samples_played < total_samples) {
        size_t samples_this_chunk = chunk_size;
        if (samples_played + samples_this_chunk > total_samples) {
            samples_this_chunk = total_samples - samples_played;
        }
        
        // Generate chunk of log sweep
        float chunk_start_time = (float)samples_played / (float)sample_rate;
        for (size_t i = 0; i < samples_this_chunk; i++) {
            float t = chunk_start_time + (float)i / (float)sample_rate;
            float normalized_t = t / duration_sec;
            
            // Logarithmic frequency sweep
            float log_start = logf(start_freq);
            float log_end = logf(end_freq);
            float log_range = log_end - log_start;
            float current_freq = start_freq * expf(log_range * normalized_t);
            
            // Generate sine wave
            float phase = 2.0f * M_PI * current_freq * t;
            float sample = sinf(phase);
            chunk_buffer[i] = (int16_t)(sample * 0.3f * 32767.0f);
        }
        
        // Play the chunk
        esp_err_t err = audio_player_submit_pcm(chunk_buffer, samples_this_chunk, 
                                                sample_rate, 1);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to submit PCM: %s", esp_err_to_name(err));
            break;
        }
        
        // Update LED progress
        float progress = (float)samples_played / (float)total_samples;
        update_leds_for_audio(progress, true);
        
        samples_played += samples_this_chunk;
        
        // Small delay to allow I2S to process
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    // Final LED update
    update_leds_for_audio(1.0f, true);
    vTaskDelay(pdMS_TO_TICKS(100));
    update_leds_for_audio(0.0f, false);
    
    free(chunk_buffer);
    ESP_LOGI(TAG, "Log sweep playback complete");
}

// Play MP3 file (if embedded or available)
static void play_mp3_file(const uint8_t *mp3_data, size_t mp3_len)
{
    if (!mp3_data || mp3_len == 0) {
        ESP_LOGW(TAG, "No MP3 data provided");
        return;
    }
    
    ESP_LOGI(TAG, "Playing MP3 file (%zu bytes)", mp3_len);
    
    mp3_decoder_t *decoder = mp3_decoder_create();
    if (!decoder) {
        ESP_LOGE(TAG, "Failed to create MP3 decoder");
        return;
    }
    
    const size_t pcm_buffer_size = 1152 * 2; // MP3 frame size * 2 for safety
    int16_t *pcm_buffer = malloc(pcm_buffer_size * sizeof(int16_t));
    if (!pcm_buffer) {
        ESP_LOGE(TAG, "Failed to allocate PCM buffer");
        mp3_decoder_destroy(decoder);
        return;
    }
    
    const uint8_t *mp3_ptr = mp3_data;
    size_t mp3_remaining = mp3_len;
    int sample_rate = 0;
    int channels = 0;
    size_t total_samples_played = 0;
    size_t total_duration_samples = 0;
    
    while (mp3_remaining > 0) {
        size_t samples_decoded = 0;
        int frame_sample_rate = 0;
        int frame_channels = 0;
        size_t bytes_consumed = 0;
        
        esp_err_t err = mp3_decoder_decode(decoder,
                                          mp3_ptr,
                                          mp3_remaining,
                                          pcm_buffer,
                                          pcm_buffer_size,
                                          &samples_decoded,
                                          &frame_sample_rate,
                                          &frame_channels,
                                          &bytes_consumed);
        
        if (err != ESP_OK || samples_decoded == 0) {
            if (bytes_consumed == 0) {
                // Need more data or end of stream
                break;
            }
        }
        
        if (samples_decoded > 0) {
            if (sample_rate == 0) {
                sample_rate = frame_sample_rate;
                channels = frame_channels;
                ESP_LOGI(TAG, "MP3: %d Hz, %d channel(s)", sample_rate, channels);
            }
            
            // Play decoded PCM
            err = audio_player_submit_pcm(pcm_buffer, samples_decoded / channels,
                                          sample_rate, channels);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Failed to submit PCM: %s", esp_err_to_name(err));
            }
            
            total_samples_played += samples_decoded / channels;
            
            // Update LED progress (estimate)
            if (total_duration_samples == 0) {
                // Estimate total duration from first few frames
                total_duration_samples = (size_t)(sample_rate * 5.0f); // Assume 5 seconds
            }
            float progress = (float)total_samples_played / (float)total_duration_samples;
            if (progress > 1.0f) progress = 1.0f;
            update_leds_for_audio(progress, true);
        }
        
        if (bytes_consumed > 0) {
            mp3_ptr += bytes_consumed;
            mp3_remaining -= bytes_consumed;
        } else {
            break; // No progress, exit
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    // Final LED update
    update_leds_for_audio(1.0f, true);
    vTaskDelay(pdMS_TO_TICKS(100));
    update_leds_for_audio(0.0f, false);
    
    free(pcm_buffer);
    mp3_decoder_destroy(decoder);
    ESP_LOGI(TAG, "MP3 playback complete");
}

void app_main(void)
{
    ESP_LOGI(TAG, "Korvo1 LED and Audio Test");
    ESP_LOGI(TAG, "LEDs: %d pixels on GPIO %d (brightness=%d)",
             CONFIG_LED_AUDIO_LED_COUNT, CONFIG_LED_AUDIO_STRIP_GPIO, CONFIG_LED_AUDIO_BRIGHTNESS);
    ESP_LOGI(TAG, "Audio: %d Hz sample rate", CONFIG_AUDIO_SAMPLE_RATE);
    
    // Initialize NVS (required for some components)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Initialize LED strip
    led_strip_config_t strip_cfg = {
        .strip_gpio_num = CONFIG_LED_AUDIO_STRIP_GPIO,
        .max_leds = CONFIG_LED_AUDIO_LED_COUNT,
        .led_model = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_RGB,
        .flags = {
            .invert_out = false,
        },
    };
    
    led_strip_rmt_config_t rmt_cfg = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
        .with_dma = false,
    };
    
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_cfg, &rmt_cfg, &s_strip));
    ESP_ERROR_CHECK(led_strip_clear(s_strip));
    ESP_LOGI(TAG, "LED strip initialized");
    
    // Initialize audio player
    esp_err_t audio_err = audio_player_init(&s_audio_config);
    if (audio_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize audio player: %s", esp_err_to_name(audio_err));
        // Continue anyway - LEDs will still work
    } else {
        ESP_LOGI(TAG, "Audio player initialized");
    }
    
    // Startup animation: brief rainbow sweep
    ESP_LOGI(TAG, "Starting LED animation...");
    for (int i = 0; i < 360; i += 5) {
        float hue = (float)i / 360.0f;
        float s = 1.0f;
        float v = 0.5f;
        
        int hi = (int)(hue * 6.0f);
        float f = hue * 6.0f - hi;
        float p = v * (1.0f - s);
        float q = v * (1.0f - f * s);
        float t = v * (1.0f - (1.0f - f) * s);
        
        uint8_t r, g, b;
        switch (hi % 6) {
            case 0: r = (uint8_t)(v * 255); g = (uint8_t)(t * 255); b = (uint8_t)(p * 255); break;
            case 1: r = (uint8_t)(q * 255); g = (uint8_t)(v * 255); b = (uint8_t)(p * 255); break;
            case 2: r = (uint8_t)(p * 255); g = (uint8_t)(v * 255); b = (uint8_t)(t * 255); break;
            case 3: r = (uint8_t)(p * 255); g = (uint8_t)(q * 255); b = (uint8_t)(v * 255); break;
            case 4: r = (uint8_t)(t * 255); g = (uint8_t)(p * 255); b = (uint8_t)(v * 255); break;
            default: r = (uint8_t)(v * 255); g = (uint8_t)(p * 255); b = (uint8_t)(q * 255); break;
        }
        
        for (uint32_t j = 0; j < CONFIG_LED_AUDIO_LED_COUNT; j++) {
            set_pixel_rgb(j, r, g, b);
        }
        led_strip_refresh(s_strip);
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    
    // Clear LEDs
    ESP_ERROR_CHECK(led_strip_clear(s_strip));
    vTaskDelay(pdMS_TO_TICKS(500));
    
    // Main loop: play log sweep repeatedly
    while (true) {
        ESP_LOGI(TAG, "=== Playing log sweep test tone ===");
        
        if (audio_err == ESP_OK) {
            play_log_sweep_pcm();
        } else {
            ESP_LOGW(TAG, "Skipping audio (audio player not initialized)");
            // Just animate LEDs
            for (int i = 0; i < 100; i++) {
                float progress = (float)i / 100.0f;
                update_leds_for_audio(progress, true);
                vTaskDelay(pdMS_TO_TICKS(50));
            }
            update_leds_for_audio(0.0f, false);
        }
        
        ESP_LOGI(TAG, "Waiting 2 seconds before next playback...");
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
