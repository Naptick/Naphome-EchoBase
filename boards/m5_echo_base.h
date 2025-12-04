/**
 * @file m5_echo_base.h
 * @brief M5 Atom Echo Base board pin definitions
 *
 * Atom Echo Base hardware:
 * - ESP32-PICO-D4 SiP (16MB flash)
 * - ES8311 audio codec
 * - MEMS microphone (full-duplex I2S)
 * - Speaker output
 * - SK6812 RGB LED (GPIO 46)
 * - Grove I2C interface
 *
 * WARNING: Many pins are FIXED and cannot be reassigned
 */

#ifndef M5_ECHO_BASE_H
#define M5_ECHO_BASE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// I2S SPEAKER OUTPUT (I2S0)
// ============================================================================
// Master clock
#define GPIO_I2S0_MCLK      (0)

// Bit clock
#define GPIO_I2S0_BCLK      (23)

// Frame sync / Word select (L/R clock)
#define GPIO_I2S0_LRCLK     (33)

// Data output (speaker)
#define GPIO_I2S0_DOUT      (22)

// Data input (not used for speaker, but for full-duplex)
#define GPIO_I2S0_DIN       (GPIO_NUM_NC)

// ============================================================================
// I2S MICROPHONE INPUT (I2S1, FULL-DUPLEX with Speaker)
// ============================================================================
// NOTE: Atom Echo Base uses full-duplex I2S with shared pins!
// In recording mode, the microphone data is available on GPIO 23 (shared BCLK)

// Master clock (shared with I2S0)
#define GPIO_I2S1_MCLK      (0)

// Data input (microphone) - SHARED with I2S0_BCLK on GPIO 23
#define GPIO_I2S1_DIN       (23)

// Bit clock - SHARED with I2S0_LRCLK on GPIO 33
#define GPIO_I2S1_BCLK      (33)

// Word select - SHARED with I2S0_LRCLK
#define GPIO_I2S1_WS        (33)

// ============================================================================
// I2C (ES8311 Audio Codec Control)
// ============================================================================
// WARNING: These pins are FIXED on Atom Echo Base!
// Do NOT attempt to remap them - hardware will be damaged

#define GPIO_I2C_SDA        (19)  // FIXED
#define GPIO_I2C_SCL        (33)  // FIXED

// I2C slave address for ES8311
#define ES8311_I2C_ADDR     (0x18)

// ============================================================================
// LED (SK6812 RGB LED, single LED)
// ============================================================================
// GPIO 46 - SK6812 (similar to WS2812, supports RMT)
#define GPIO_RGB_LED        (46)

// Number of LEDs in the strip (Atom Echo Base has 1 integrated LED)
#define RGB_LED_COUNT       (1)

// ============================================================================
// BUTTON & CONTROL
// ============================================================================
// Programmable key (user button)
#define GPIO_BUTTON         (41)

// Power button and reset handled by bootloader

// ============================================================================
// GROVE INTERFACE (I2C Expansion)
// ============================================================================
// Grove A/B connectors use the same I2C (GPIO 19, 33)
// Can connect additional I2C sensors/modules

// ============================================================================
// AUDIO CODEC CONFIGURATION
// ============================================================================
// ES8311 parameters
#define AUDIO_CODEC_I2C_PORT    (I2C_NUM_0)
#define AUDIO_CODEC_SAMPLE_RATE (16000)  // Optimized for STT
#define AUDIO_CODEC_BIT_WIDTH   (16)     // 16-bit audio
#define AUDIO_CODEC_CHANNELS    (1)      // Mono (can record + playback)

// ============================================================================
// I2S CONFIGURATION
// ============================================================================
// Use I2S0 for speaker, I2S1 for microphone (or multiplexed if needed)
#define AUDIO_I2S_NUM           (I2S_NUM_0)
#define MIC_I2S_NUM             (I2S_NUM_1)

// DMA buffer configuration
#define I2S_DMA_BUFFER_COUNT    (4)
#define I2S_DMA_BUFFER_LEN      (256)

// ============================================================================
// MEMORY CONFIGURATION
// ============================================================================
// Atom Echo Base: ESP32-PICO-D4 (4MB flash on-chip)
// Plus external PSRAM (optional)

// ============================================================================
// I2C BUS SPEED
// ============================================================================
// Standard I2C frequency for ES8311
#define I2C_FREQ_HZ             (100000)  // 100 kHz standard

#ifdef __cplusplus
}
#endif

#endif  // M5_ECHO_BASE_H
