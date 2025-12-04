/**
 * @file audio_abstraction.c
 * @brief Implementation of board-independent audio interface
 */

#include "audio_abstraction.h"
#include "board_config.h"
#include "esp_log.h"
#include "driver/i2s.h"
#include "hal/gpio_types.h"

static const char *TAG = "audio_abstraction";

// ============================================================================
// INITIALIZATION
// ============================================================================

esp_err_t audio_abstraction_init(int sample_rate)
{
    ESP_LOGI(TAG, "Initializing audio for board: %s", BOARD_NAME);
    ESP_LOGI(TAG, "  Sample rate: %d Hz", sample_rate);
    ESP_LOGI(TAG, "  Microphone mode: %s", audio_abstraction_get_microphone_mode());
    return ESP_OK;
}

// ============================================================================
// BOARD INFORMATION
// ============================================================================

int audio_abstraction_get_sample_rate(void)
{
    return AUDIO_SAMPLE_RATE_HZ;
}

int audio_abstraction_has_microphone(void)
{
    return HAS_MICROPHONE;
}

const char* audio_abstraction_get_microphone_mode(void)
{
    return MICROPHONE_MODE;
}

int audio_abstraction_supports_simultaneous_record_playback(void)
{
#if defined(CONFIG_BOARD_KORVO1)
    return 1;  // Korvo1 has independent I2S0/I2S1
#elif defined(CONFIG_BOARD_M5_ECHO_BASE)
    return 0;  // M5 Echo Base shares I2S pins
#else
    return 0;
#endif
}

const char* audio_abstraction_get_board_name(void)
{
    return BOARD_NAME;
}

// ============================================================================
// LED ABSTRACTION
// ============================================================================

gpio_num_t audio_abstraction_get_led_gpio(void)
{
    return GPIO_RGB_LED;
}

int audio_abstraction_get_led_count(void)
{
    return RGB_LED_COUNT;
}

// ============================================================================
// I2C ABSTRACTION
// ============================================================================

gpio_num_t audio_abstraction_get_i2c_sda(void)
{
    return GPIO_I2C_SDA;
}

gpio_num_t audio_abstraction_get_i2c_scl(void)
{
    return GPIO_I2C_SCL;
}

int audio_abstraction_get_i2c_freq(void)
{
    return I2C_FREQ_HZ;
}

// ============================================================================
// I2S ABSTRACTION
// ============================================================================

i2s_port_t audio_abstraction_get_speaker_i2s_port(void)
{
    return AUDIO_I2S_NUM;
}

i2s_port_t audio_abstraction_get_microphone_i2s_port(void)
{
    return MIC_I2S_NUM;
}

esp_err_t audio_abstraction_get_speaker_pins(
    gpio_num_t *out_mclk,
    gpio_num_t *out_bclk,
    gpio_num_t *out_lrclk,
    gpio_num_t *out_dout
)
{
    if (!out_mclk || !out_bclk || !out_lrclk || !out_dout) {
        return ESP_ERR_INVALID_ARG;
    }

    *out_mclk = GPIO_I2S0_MCLK;
    *out_bclk = GPIO_I2S0_BCLK;
    *out_lrclk = GPIO_I2S0_LRCLK;
    *out_dout = GPIO_I2S0_DOUT;

    return ESP_OK;
}

esp_err_t audio_abstraction_get_microphone_pins(
    gpio_num_t *out_mclk,
    gpio_num_t *out_din,
    gpio_num_t *out_bclk,
    gpio_num_t *out_ws
)
{
    if (!out_mclk || !out_din || !out_bclk || !out_ws) {
        return ESP_ERR_INVALID_ARG;
    }

    *out_mclk = GPIO_I2S1_MCLK;
    *out_din = GPIO_I2S1_DIN;
    *out_bclk = GPIO_I2S1_BCLK;
    *out_ws = GPIO_I2S1_WS;

#if defined(CONFIG_BOARD_M5_ECHO_BASE)
    ESP_LOGW(TAG, "M5 Echo Base: Microphone pins shared with speaker!");
    ESP_LOGW(TAG, "  DIN (GPIO %d) shared with speaker BCLK", GPIO_I2S1_DIN);
    ESP_LOGW(TAG, "  BCLK (GPIO %d) shared with speaker LRCLK", GPIO_I2S1_BCLK);
    ESP_LOGW(TAG, "  WS (GPIO %d) shared with speaker LRCLK", GPIO_I2S1_WS);
    ESP_LOGW(TAG, "Simultaneous record+playback not recommended");
#endif

    return ESP_OK;
}

// ============================================================================
// MEMORY ABSTRACTION
// ============================================================================

int audio_abstraction_get_flash_size_mb(void)
{
    return FLASH_SIZE_MB;
}

int audio_abstraction_get_psram_size_mb(void)
{
    return PSRAM_SIZE_MB;
}

// ============================================================================
// DEBUGGING
// ============================================================================

void audio_abstraction_print_board_info(void)
{
    ESP_LOGI(TAG, "=== BOARD CONFIGURATION ===");
    ESP_LOGI(TAG, "Board: %s", audio_abstraction_get_board_name());
    ESP_LOGI(TAG, "Flash: %d MB", audio_abstraction_get_flash_size_mb());
    ESP_LOGI(TAG, "PSRAM: %d MB", audio_abstraction_get_psram_size_mb());
    ESP_LOGI(TAG, "LED GPIO: %d (count: %d)",
             audio_abstraction_get_led_gpio(),
             audio_abstraction_get_led_count());
    ESP_LOGI(TAG, "Microphone: %s (%s)",
             audio_abstraction_has_microphone() ? "Yes" : "No",
             audio_abstraction_get_microphone_mode());
    ESP_LOGI(TAG, "Simultaneous Record+Playback: %s",
             audio_abstraction_supports_simultaneous_record_playback() ? "Yes" : "No");
}

void audio_abstraction_print_i2s_pins(void)
{
    gpio_num_t mclk, bclk, lrclk, dout;
    gpio_num_t mic_mclk, mic_din, mic_bclk, mic_ws;

    audio_abstraction_get_speaker_pins(&mclk, &bclk, &lrclk, &dout);
    audio_abstraction_get_microphone_pins(&mic_mclk, &mic_din, &mic_bclk, &mic_ws);

    ESP_LOGI(TAG, "=== I2S SPEAKER PINS (I2S%d) ===",
             audio_abstraction_get_speaker_i2s_port());
    ESP_LOGI(TAG, "  MCLK: GPIO %d", mclk);
    ESP_LOGI(TAG, "  BCLK: GPIO %d", bclk);
    ESP_LOGI(TAG, "  LRCLK: GPIO %d", lrclk);
    ESP_LOGI(TAG, "  DOUT: GPIO %d", dout);

    ESP_LOGI(TAG, "=== I2S MICROPHONE PINS (I2S%d) ===",
             audio_abstraction_get_microphone_i2s_port());
    ESP_LOGI(TAG, "  MCLK: GPIO %d", mic_mclk);
    ESP_LOGI(TAG, "  DIN: GPIO %d", mic_din);
    ESP_LOGI(TAG, "  BCLK: GPIO %d", mic_bclk);
    ESP_LOGI(TAG, "  WS: GPIO %d", mic_ws);

    ESP_LOGI(TAG, "=== I2C PINS ===");
    ESP_LOGI(TAG, "  SDA: GPIO %d", audio_abstraction_get_i2c_sda());
    ESP_LOGI(TAG, "  SCL: GPIO %d", audio_abstraction_get_i2c_scl());
    ESP_LOGI(TAG, "  Frequency: %d Hz", audio_abstraction_get_i2c_freq());
}
