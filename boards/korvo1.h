/**
 * @file korvo1.h
 * @brief Korvo1 development board pin definitions
 *
 * Korvo1 hardware:
 * - ESP32-S3 (16MB flash, 8MB PSRAM)
 * - ES8311 audio codec
 * - PDM microphone on I2S1
 * - Speaker output on I2S0
 * - 12x WS2812 RGB LED ring
 * - Grove expansion interface
 */

#ifndef KORVO1_H
#define KORVO1_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// I2S SPEAKER OUTPUT (I2S0)
// ============================================================================
// Korvo1 BSP pin definitions for speaker output via ES8311 codec

// Master clock
#define GPIO_I2S0_MCLK      (42)  // BSP_I2S0_MCLK

// Bit clock
#define GPIO_I2S0_BCLK      (40)  // BSP_I2S0_SCLK

// Frame sync / Word select (L/R clock)
#define GPIO_I2S0_LRCLK     (41)  // BSP_I2S0_LCLK

// Data output (speaker)
#define GPIO_I2S0_DOUT      (39)  // BSP_I2S0_DOUT

// Data input (not used for speaker)
#define GPIO_I2S0_DIN       (GPIO_NUM_NC)

// ============================================================================
// I2S MICROPHONE INPUT (I2S1, PDM Mode)
// ============================================================================
// Korvo1 has separate I2S1 port for PDM microphone input

// Master clock
#define GPIO_I2S1_MCLK      (0)   // Shared with I2S0

// Data input (PDM microphone)
#define GPIO_I2S1_DIN       (35)  // PDM data line

// Bit clock (PDM clock)
#define GPIO_I2S1_BCLK      (36)  // PDM clock

// Word select
#define GPIO_I2S1_WS        (37)  // PDM word select

// ============================================================================
// I2C (ES8311 Audio Codec Control)
// ============================================================================

#define GPIO_I2C_SDA        (1)   // BSP_I2C_SDA
#define GPIO_I2C_SCL        (2)   // BSP_I2C_SCL

// I2C slave address for ES8311
#define ES8311_I2C_ADDR     (0x18)

// ============================================================================
// LED (WS2812 RGB LED Ring)
// ============================================================================
// GPIO 19 - WS2812 data line (12x LED ring)
#define GPIO_RGB_LED        (19)

// Number of LEDs in the ring
#define RGB_LED_COUNT       (12)

// ============================================================================
// BUTTON & CONTROL
// ============================================================================
// Korvo1 has power button and boot button (hardware-handled)
// GPIO 0 may be available for user button

#define GPIO_BUTTON         (GPIO_NUM_NC)  // Not defined on Korvo1

// ============================================================================
// GROVE INTERFACE (I2C Expansion)
// ============================================================================
// Grove interface uses standard I2C (GPIO 1, 2)

// ============================================================================
// AUDIO CODEC CONFIGURATION
// ============================================================================
// ES8311 parameters (same on both boards)
#define AUDIO_CODEC_I2C_PORT    (I2C_NUM_0)
#define AUDIO_CODEC_SAMPLE_RATE (16000)  // Optimized for STT
#define AUDIO_CODEC_BIT_WIDTH   (16)     // 16-bit audio
#define AUDIO_CODEC_CHANNELS    (1)      // Mono

// ============================================================================
// I2S CONFIGURATION
// ============================================================================
// I2S0 for speaker output, I2S1 for PDM microphone input
#define AUDIO_I2S_NUM           (I2S_NUM_0)
#define MIC_I2S_NUM             (I2S_NUM_1)

// DMA buffer configuration
#define I2S_DMA_BUFFER_COUNT    (4)
#define I2S_DMA_BUFFER_LEN      (256)

// ============================================================================
// MEMORY CONFIGURATION
// ============================================================================
// Korvo1: ESP32-S3 (16MB flash, 8MB PSRAM)
// Larger embedded binary support available

// ============================================================================
// I2C BUS SPEED
// ============================================================================
// Standard I2C frequency for ES8311
#define I2C_FREQ_HZ             (100000)  // 100 kHz standard

#ifdef __cplusplus
}
#endif

#endif  // KORVO1_H
