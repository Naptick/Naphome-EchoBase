/**
 * @file audio_abstraction.h
 * @brief Board-independent audio interface for Korvo1 and M5 Echo Base
 *
 * This header provides a unified interface for audio operations that abstracts
 * away board-specific I2S and GPIO pin configurations. The actual pin definitions
 * are located in boards/korvo1.h and boards/m5_echo_base.h.
 *
 * ARCHITECTURE:
 * - board_config.h      : Selects board-specific headers (korvo1.h or m5_echo_base.h)
 * - audio_abstraction.h : Provides unified audio interface (this file)
 * - audio_player.c      : Implements audio using board-specific pins from board_config.h
 *
 * KEY DIFFERENCES HANDLED:
 *
 * KORVO1 (ESP32-S3):
 * - I2S0: Speaker output (I2S_NUM_0)
 * - I2S1: PDM microphone input (separate I2S port)
 * - Independent I2S ports allow simultaneous record+playback
 * - 12x WS2812 RGB LED ring
 * - Full I2C control of ES8311 codec
 *
 * M5 ECHO BASE (ESP32-PICO-D4):
 * - I2S0: Speaker output
 * - I2S1: Full-duplex microphone (shares BCLK and LRCLK with speaker)
 * - Shared I2S pins limit simultaneous record+playback
 * - 1x SK6812 RGB LED
 * - Fixed I2C pins (GPIO 19 SDA, GPIO 33 SCL) - cannot be remapped
 *
 * PIN DEFINITIONS:
 * All GPIO pins are defined in the board-specific headers:
 * - GPIO_I2S0_MCLK, GPIO_I2S0_BCLK, GPIO_I2S0_LRCLK, GPIO_I2S0_DOUT
 * - GPIO_I2S1_MCLK, GPIO_I2S1_DIN, GPIO_I2S1_BCLK, GPIO_I2S1_WS
 * - GPIO_I2C_SDA, GPIO_I2C_SCL
 * - GPIO_RGB_LED, RGB_LED_COUNT
 * - AUDIO_I2S_NUM, MIC_I2S_NUM
 */

#pragma once

#include "esp_err.h"
#include "board_config.h"
#include "hal/gpio_types.h"
#include "driver/i2s.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// AUDIO ABSTRACTION LAYER - Board-Independent Interface
// ============================================================================

/**
 * Audio system initialization
 *
 * Initializes I2C, I2S0 (speaker), and ES8311 codec with board-specific pins.
 * Must be called before any audio operations.
 *
 * @param sample_rate Sample rate in Hz (e.g., 16000, 48000)
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t audio_abstraction_init(int sample_rate);

/**
 * Get the currently configured sample rate
 *
 * @return Sample rate in Hz
 */
int audio_abstraction_get_sample_rate(void);

/**
 * Get microphone availability for this board
 *
 * @return 1 if microphone available, 0 if not
 */
int audio_abstraction_has_microphone(void);

/**
 * Get microphone mode description
 *
 * @return "PDM" for Korvo1, "I2S Full-Duplex" for M5 Echo Base
 */
const char* audio_abstraction_get_microphone_mode(void);

/**
 * Check if simultaneous record+playback is supported
 *
 * @return 1 if supported (Korvo1), 0 if not supported (M5 Echo Base)
 */
int audio_abstraction_supports_simultaneous_record_playback(void);

/**
 * Get board name for logging/debugging
 *
 * @return "Korvo1" or "M5 Atom Echo Base"
 */
const char* audio_abstraction_get_board_name(void);

// ============================================================================
// LED ABSTRACTION
// ============================================================================

/**
 * Get LED strip GPIO pin for this board
 *
 * @return GPIO number for LED control
 */
gpio_num_t audio_abstraction_get_led_gpio(void);

/**
 * Get number of LEDs on this board
 *
 * @return 12 for Korvo1 (WS2812 ring), 1 for M5 Echo Base (SK6812)
 */
int audio_abstraction_get_led_count(void);

// ============================================================================
// I2C ABSTRACTION
// ============================================================================

/**
 * Get I2C SDA pin for this board
 *
 * @return GPIO number for I2C SDA
 */
gpio_num_t audio_abstraction_get_i2c_sda(void);

/**
 * Get I2C SCL pin for this board
 *
 * @return GPIO number for I2C SCL
 */
gpio_num_t audio_abstraction_get_i2c_scl(void);

/**
 * Get I2C frequency for this board
 *
 * @return I2C frequency in Hz (typically 100000)
 */
int audio_abstraction_get_i2c_freq(void);

// ============================================================================
// I2S ABSTRACTION
// ============================================================================

/**
 * Get speaker I2S port number
 *
 * @return I2S_NUM_0
 */
i2s_port_t audio_abstraction_get_speaker_i2s_port(void);

/**
 * Get microphone I2S port number
 *
 * @return I2S_NUM_1
 */
i2s_port_t audio_abstraction_get_microphone_i2s_port(void);

/**
 * Get speaker I2S pin configuration
 *
 * @param out_mclk Output: Master clock GPIO
 * @param out_bclk Output: Bit clock GPIO
 * @param out_lrclk Output: L/R clock GPIO
 * @param out_dout Output: Data output GPIO
 * @return ESP_OK
 */
esp_err_t audio_abstraction_get_speaker_pins(
    gpio_num_t *out_mclk,
    gpio_num_t *out_bclk,
    gpio_num_t *out_lrclk,
    gpio_num_t *out_dout
);

/**
 * Get microphone I2S pin configuration
 *
 * WARNING: On M5 Echo Base, microphone DIN, BCLK, and WS may share pins with speaker!
 *
 * @param out_mclk Output: Master clock GPIO
 * @param out_din Output: Data input GPIO
 * @param out_bclk Output: Bit clock GPIO
 * @param out_ws Output: Word select GPIO
 * @return ESP_OK
 */
esp_err_t audio_abstraction_get_microphone_pins(
    gpio_num_t *out_mclk,
    gpio_num_t *out_din,
    gpio_num_t *out_bclk,
    gpio_num_t *out_ws
);

// ============================================================================
// MEMORY ABSTRACTION
// ============================================================================

/**
 * Get flash size in MB for this board
 *
 * @return 16 for Korvo1, 4 for M5 Echo Base
 */
int audio_abstraction_get_flash_size_mb(void);

/**
 * Get PSRAM size in MB for this board
 *
 * @return 8 for Korvo1, 0 for M5 Echo Base (if available)
 */
int audio_abstraction_get_psram_size_mb(void);

// ============================================================================
// DEBUGGING
// ============================================================================

/**
 * Print board configuration details to console (for debugging)
 */
void audio_abstraction_print_board_info(void);

/**
 * Print I2S pin configuration to console (for debugging)
 */
void audio_abstraction_print_i2s_pins(void);

#ifdef __cplusplus
}
#endif
