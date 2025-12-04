/**
 * @file board_config.h
 * @brief Board-specific configuration selection
 *
 * This header provides unified pin definitions by selecting the appropriate
 * board header based on compile-time configuration.
 *
 * Usage in CMakeLists:
 *   - Set CONFIG_BOARD_KORVO1=y for Korvo1
 *   - Set CONFIG_BOARD_M5_ECHO_BASE=y for M5 Atom Echo Base
 */

#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

// Select board at build time via Kconfig
#if defined(CONFIG_BOARD_M5_ECHO_BASE)
    #include "../boards/m5_echo_base.h"
    #define BOARD_NAME "M5 Atom Echo Base"

#elif defined(CONFIG_BOARD_KORVO1)
    #include "../boards/korvo1.h"
    #define BOARD_NAME "Korvo1"

#else
    #error "No board selected! Define CONFIG_BOARD_KORVO1 or CONFIG_BOARD_M5_ECHO_BASE"
#endif

// ============================================================================
// UNIFIED PIN DEFINITIONS (common across all boards)
// ============================================================================
// These pins are defined in the board-specific headers above

// I2S Speaker pins (all boards)
extern const uint32_t g_i2s_speaker_mclk;
extern const uint32_t g_i2s_speaker_bclk;
extern const uint32_t g_i2s_speaker_lrclk;
extern const uint32_t g_i2s_speaker_dout;

// I2S Microphone pins (all boards)
extern const uint32_t g_i2s_mic_mclk;
extern const uint32_t g_i2s_mic_din;
extern const uint32_t g_i2s_mic_bclk;
extern const uint32_t g_i2s_mic_ws;

// I2C codec pins (all boards)
extern const uint32_t g_i2c_sda;
extern const uint32_t g_i2c_scl;

// LED pins (all boards)
extern const uint32_t g_rgb_led_gpio;
extern const uint32_t g_rgb_led_count;

// ============================================================================
// CONFIGURATION CONSTANTS
// ============================================================================

#define AUDIO_CODEC_I2C_ADDR        ES8311_I2C_ADDR
#define AUDIO_SAMPLE_RATE_HZ        AUDIO_CODEC_SAMPLE_RATE
#define AUDIO_BIT_WIDTH             AUDIO_CODEC_BIT_WIDTH
#define AUDIO_CHANNELS              AUDIO_CODEC_CHANNELS

// ============================================================================
// BOARD-SPECIFIC FEATURES
// ============================================================================

// Microphone feature available on this board
#if defined(CONFIG_BOARD_KORVO1)
    #define HAS_MICROPHONE          1
    #define HAS_LED_RING            1
    #define MICROPHONE_MODE         "PDM"
    #define MIC_SAMPLE_RATE         16000
    #define FLASH_SIZE_MB           16
    #define PSRAM_SIZE_MB           8

#elif defined(CONFIG_BOARD_M5_ECHO_BASE)
    #define HAS_MICROPHONE          1
    #define HAS_LED_RING            0  // Only single LED
    #define MICROPHONE_MODE         "I2S Full-Duplex"
    #define MIC_SAMPLE_RATE         16000
    #define FLASH_SIZE_MB           4   // ESP32-PICO-D4
    #define PSRAM_SIZE_MB           0   // Check if available

#endif

#ifdef __cplusplus
}
#endif

#endif  // BOARD_CONFIG_H
