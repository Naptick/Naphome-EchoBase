/**
 * @file m5_echo_base.h
 * @brief M5 Atom Echo Base audio driver header
 *
 * This driver abstracts the ES8311 audio codec on the Atom Echo Base
 * with M5 Atom S3R as the main controller.
 *
 * Key characteristics:
 * - ESP32-PICO-D4 SiP (4MB flash on-chip, optional external PSRAM)
 * - ES8311 mono audio codec with full-duplex I2S
 * - Fixed GPIO pins (cannot be remapped without hardware damage)
 * - I2C control on GPIO 19 (SDA) and GPIO 33 (SCL)
 * - Speaker I2S0 on GPIO 0, 22, 23, 33
 * - Microphone I2S1 shares pins with speaker (full-duplex mode)
 */

#ifndef M5_ECHO_BASE_H
#define M5_ECHO_BASE_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "driver/i2s.h"
#include "driver/i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// TYPE DEFINITIONS
// ============================================================================

/**
 * @brief M5 Atom Echo Base configuration structure
 */
typedef struct {
    // I2S speaker output configuration
    i2s_port_t i2s_port;            ///< I2S port number (I2S_NUM_0 for speaker)
    gpio_num_t i2s_speaker_mclk;   ///< Master clock GPIO (GPIO 0)
    gpio_num_t i2s_speaker_bclk;   ///< Bit clock GPIO (GPIO 23)
    gpio_num_t i2s_speaker_lrclk;  ///< L/R clock GPIO (GPIO 33)
    gpio_num_t i2s_speaker_dout;   ///< Data out GPIO (GPIO 22)

    // I2S microphone input configuration
    i2s_port_t i2s_mic_port;        ///< I2S port for microphone (I2S_NUM_1)
    gpio_num_t i2s_mic_din;         ///< Mic data in (GPIO 23, shared with speaker)
    gpio_num_t i2s_mic_ws;          ///< Mic word select (GPIO 33, shared with speaker)

    // I2C codec control
    i2c_port_t i2c_port;            ///< I2C port number
    gpio_num_t i2c_sda;             ///< I2C SDA GPIO (GPIO 19, FIXED)
    gpio_num_t i2c_scl;             ///< I2C SCL GPIO (GPIO 33, FIXED)
    uint32_t i2c_freq_hz;           ///< I2C frequency (typically 100 kHz)
    uint8_t i2c_slave_addr;         ///< ES8311 I2C slave address (0x18)

    // Audio parameters
    uint32_t sample_rate_hz;        ///< Sample rate (typically 16000 Hz for STT)
    uint16_t bits_per_sample;       ///< Bits per sample (16-bit)
    i2s_channel_fmt_t channel_fmt;  ///< Channel format (mono, stereo)

    // DMA configuration
    int dma_buffer_count;           ///< Number of DMA buffers
    int dma_buffer_len;             ///< DMA buffer length in samples

    // LED configuration
    gpio_num_t led_gpio;            ///< GPIO for SK6812 LED (GPIO 46)
    uint8_t led_count;              ///< Number of LEDs (1 for Atom Echo Base)
} m5_echo_base_config_t;

/**
 * @brief M5 Atom Echo Base driver handle
 */
typedef struct {
    m5_echo_base_config_t config;
    bool initialized;
} m5_echo_base_t;

// ============================================================================
// FUNCTION DECLARATIONS
// ============================================================================

/**
 * @brief Initialize M5 Atom Echo Base audio system
 *
 * Initializes the ES8311 codec via I2C and configures I2S for speaker output
 * and microphone input. For full-duplex operation, both I2S0 (speaker) and
 * I2S1 (microphone) must be configured with shared GPIO pins (23, 33).
 *
 * @param[inout] dev            M5 Echo Base device handle
 * @param[in]    config         Configuration parameters
 *
 * @return
 *     - ESP_OK: Initialization successful
 *     - ESP_ERR_INVALID_ARG: Invalid arguments
 *     - ESP_FAIL: I2C communication or codec initialization failed
 */
esp_err_t m5_echo_base_init(m5_echo_base_t *dev, const m5_echo_base_config_t *config);

/**
 * @brief Deinitialize M5 Atom Echo Base audio system
 *
 * Stops I2S drivers and releases resources.
 *
 * @param[inout] dev            M5 Echo Base device handle
 *
 * @return
 *     - ESP_OK: Deinitialization successful
 *     - ESP_FAIL: Operation failed
 */
esp_err_t m5_echo_base_deinit(m5_echo_base_t *dev);

/**
 * @brief Write speaker audio data via I2S
 *
 * Sends PCM audio samples to the speaker through I2S0.
 *
 * @param[in] dev               M5 Echo Base device handle
 * @param[in] data              Audio data buffer (16-bit PCM)
 * @param[in] len               Number of bytes to write
 * @param[out] bytes_written    Number of bytes actually written
 * @param[in] ticks_to_wait     FreeRTOS ticks to wait for space
 *
 * @return
 *     - ESP_OK: Data written successfully
 *     - ESP_FAIL: Write failed
 */
esp_err_t m5_echo_base_write_speaker(m5_echo_base_t *dev, const void *data,
                                     size_t len, size_t *bytes_written,
                                     uint32_t ticks_to_wait);

/**
 * @brief Read microphone audio data via I2S
 *
 * Receives PCM audio samples from the microphone through I2S1 (full-duplex mode).
 *
 * @param[in] dev               M5 Echo Base device handle
 * @param[out] data             Audio data buffer (16-bit PCM)
 * @param[in] len               Number of bytes to read
 * @param[out] bytes_read       Number of bytes actually read
 * @param[in] ticks_to_wait     FreeRTOS ticks to wait for data
 *
 * @return
 *     - ESP_OK: Data read successfully
 *     - ESP_FAIL: Read failed
 */
esp_err_t m5_echo_base_read_microphone(m5_echo_base_t *dev, void *data,
                                       size_t len, size_t *bytes_read,
                                       uint32_t ticks_to_wait);

/**
 * @brief Set codec volume
 *
 * Sets the output volume of the ES8311 codec.
 *
 * @param[in] dev               M5 Echo Base device handle
 * @param[in] volume            Volume level (0-100, in dB)
 *
 * @return
 *     - ESP_OK: Volume set successfully
 *     - ESP_FAIL: Failed to set volume
 */
esp_err_t m5_echo_base_set_volume(m5_echo_base_t *dev, uint8_t volume);

/**
 * @brief Get current codec volume
 *
 * @param[in] dev               M5 Echo Base device handle
 * @param[out] volume           Current volume level
 *
 * @return
 *     - ESP_OK: Volume retrieved successfully
 *     - ESP_FAIL: Failed to get volume
 */
esp_err_t m5_echo_base_get_volume(m5_echo_base_t *dev, uint8_t *volume);

/**
 * @brief Set LED color
 *
 * Sets the color of the single SK6812 RGB LED on the Echo Base.
 *
 * @param[in] dev               M5 Echo Base device handle
 * @param[in] red               Red component (0-255)
 * @param[in] green             Green component (0-255)
 * @param[in] blue              Blue component (0-255)
 *
 * @return
 *     - ESP_OK: LED color set successfully
 *     - ESP_FAIL: Failed to set LED color
 */
esp_err_t m5_echo_base_set_led_color(m5_echo_base_t *dev, uint8_t red,
                                     uint8_t green, uint8_t blue);

#ifdef __cplusplus
}
#endif

#endif  // M5_ECHO_BASE_H
