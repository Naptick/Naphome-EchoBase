/**
 * @file m5_echo_base.c
 * @brief M5 Atom Echo Base audio driver implementation
 *
 * This driver implements ES8311 audio codec support for the M5 Atom Echo Base.
 * It handles I2S configuration for speaker output and microphone input.
 *
 * NOTE: This is an initial implementation. Some features may require
 * refinement based on actual hardware testing.
 */

#include "m5_echo_base.h"

#include <string.h>
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "driver/i2s.h"

static const char *TAG = "m5_echo_base";

// ============================================================================
// ES8311 CODEC REGISTER DEFINITIONS
// ============================================================================

#define ES8311_SLAVE_ADDR       (0x18)

// Control registers
#define ES8311_REG_CHIP_ID      (0x00)  // Chip ID register
#define ES8311_REG_RESET        (0x00)  // Software reset (write 0xFF)
#define ES8311_REG_CLK_CTRL1    (0x01)  // Clock control 1
#define ES8311_REG_CLK_CTRL2    (0x02)  // Clock control 2
#define ES8311_REG_DAC_CTRL     (0x37)  // DAC control
#define ES8311_REG_ADC_CTRL     (0x38)  // ADC control
#define ES8311_REG_SYS_PWR1     (0x0E)  // System power 1
#define ES8311_REG_SYS_PWR2     (0x0F)  // System power 2
#define ES8311_REG_OUT_MIX      (0x40)  // Output mixer control

// Audio interface register
#define ES8311_REG_AUDIO_IF1    (0x03)  // Audio interface 1
#define ES8311_REG_AUDIO_IF2    (0x04)  // Audio interface 2

// Volume control registers
#define ES8311_REG_DAC_VOL      (0x32)  // DAC volume control
#define ES8311_REG_ADC_VOL      (0x39)  // ADC volume control

// ============================================================================
// I2C HELPER FUNCTIONS
// ============================================================================

/**
 * @brief Write a register to the ES8311 codec
 */
static esp_err_t es8311_write_reg(i2c_port_t i2c_port, uint8_t reg, uint8_t value)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (ES8311_SLAVE_ADDR << 1) | 0, 0);
    i2c_master_write_byte(cmd, reg, 0);
    i2c_master_write_byte(cmd, value, 0);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(i2c_port, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "I2C write failed (reg=0x%02x): %s", reg, esp_err_to_name(ret));
    }

    return ret;
}

/**
 * @brief Read a register from the ES8311 codec
 */
static esp_err_t es8311_read_reg(i2c_port_t i2c_port, uint8_t reg, uint8_t *value)
{
    ESP_ARG_CHECK(value);

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (ES8311_SLAVE_ADDR << 1) | 0, 0);
    i2c_master_write_byte(cmd, reg, 0);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (ES8311_SLAVE_ADDR << 1) | 1, 0);
    i2c_master_read_byte(cmd, value, I2C_MASTER_NACK);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(i2c_port, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "I2C read failed (reg=0x%02x): %s", reg, esp_err_to_name(ret));
    }

    return ret;
}

// ============================================================================
// CODEC INITIALIZATION
// ============================================================================

/**
 * @brief Initialize I2C bus for codec communication
 */
static esp_err_t init_i2c_bus(i2c_port_t i2c_port, gpio_num_t sda, gpio_num_t scl, uint32_t freq_hz)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = sda,
        .scl_io_num = scl,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = freq_hz,
    };

    ESP_RETURN_ON_ERROR(i2c_param_config(i2c_port, &conf), TAG, "I2C param config failed");
    ESP_RETURN_ON_ERROR(i2c_driver_install(i2c_port, conf.mode, 0, 0, 0), TAG, "I2C driver install failed");

    ESP_LOGI(TAG, "I2C initialized on SDA=%d SCL=%d, freq=%u Hz", sda, scl, freq_hz);

    return ESP_OK;
}

/**
 * @brief Initialize ES8311 audio codec
 */
static esp_err_t init_es8311_codec(i2c_port_t i2c_port, uint32_t sample_rate_hz)
{
    ESP_LOGI(TAG, "Initializing ES8311 codec...");

    // Read chip ID to verify codec is present
    uint8_t chip_id;
    ESP_RETURN_ON_ERROR(es8311_read_reg(i2c_port, ES8311_REG_CHIP_ID, &chip_id),
                        TAG, "Failed to read ES8311 chip ID");

    ESP_LOGI(TAG, "ES8311 Chip ID: 0x%02x", chip_id);

    // Software reset
    ESP_RETURN_ON_ERROR(es8311_write_reg(i2c_port, ES8311_REG_RESET, 0xFF),
                        TAG, "Software reset failed");
    vTaskDelay(pdMS_TO_TICKS(50));  // Wait for reset to complete

    // Configure clock
    // For 16 kHz sample rate with MCLK=0 (device determines clock)
    ESP_RETURN_ON_ERROR(es8311_write_reg(i2c_port, ES8311_REG_CLK_CTRL1, 0x00),
                        TAG, "Clock control 1 setup failed");

    // Set sample rate bits (depends on implementation)
    // This is a placeholder - actual values depend on clock configuration
    ESP_RETURN_ON_ERROR(es8311_write_reg(i2c_port, ES8311_REG_CLK_CTRL2, 0x00),
                        TAG, "Clock control 2 setup failed");

    // Configure audio interface (I2S mode, 16-bit, mono)
    ESP_RETURN_ON_ERROR(es8311_write_reg(i2c_port, ES8311_REG_AUDIO_IF1, 0x00),
                        TAG, "Audio interface 1 setup failed");

    ESP_RETURN_ON_ERROR(es8311_write_reg(i2c_port, ES8311_REG_AUDIO_IF2, 0x00),
                        TAG, "Audio interface 2 setup failed");

    // Enable DAC and ADC power
    ESP_RETURN_ON_ERROR(es8311_write_reg(i2c_port, ES8311_REG_SYS_PWR1, 0xFF),
                        TAG, "System power 1 setup failed");

    ESP_RETURN_ON_ERROR(es8311_write_reg(i2c_port, ES8311_REG_SYS_PWR2, 0xFF),
                        TAG, "System power 2 setup failed");

    // Enable DAC and ADC
    ESP_RETURN_ON_ERROR(es8311_write_reg(i2c_port, ES8311_REG_DAC_CTRL, 0x80),
                        TAG, "DAC control setup failed");

    ESP_RETURN_ON_ERROR(es8311_write_reg(i2c_port, ES8311_REG_ADC_CTRL, 0x80),
                        TAG, "ADC control setup failed");

    // Set output mixer
    ESP_RETURN_ON_ERROR(es8311_write_reg(i2c_port, ES8311_REG_OUT_MIX, 0xCC),
                        TAG, "Output mixer setup failed");

    // Set initial volume (mid-level)
    uint8_t dac_vol = 0x18;  // Mid-level volume
    ESP_RETURN_ON_ERROR(es8311_write_reg(i2c_port, ES8311_REG_DAC_VOL, dac_vol),
                        TAG, "DAC volume setup failed");

    ESP_LOGI(TAG, "ES8311 codec initialized successfully");

    return ESP_OK;
}

/**
 * @brief Initialize I2S for speaker output
 */
static esp_err_t init_i2s_speaker(i2s_port_t port, const m5_echo_base_config_t *config)
{
    ESP_LOGI(TAG, "Initializing I2S speaker output on port %d", port);

    // I2S configuration
    i2s_config_t i2s_conf = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX,
        .sample_rate = config->sample_rate_hz,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,  // Mono
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = config->dma_buffer_count,
        .dma_buf_len = config->dma_buffer_len,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0,
    };

    ESP_RETURN_ON_ERROR(i2s_driver_install(port, &i2s_conf, 0, NULL),
                        TAG, "I2S driver install failed");

    // I2S pin configuration (M5 Echo Base fixed pins)
    i2s_pin_config_t pin_conf = {
        .mck_io_num = config->i2s_speaker_mclk,   // GPIO 0
        .bck_io_num = config->i2s_speaker_bclk,   // GPIO 23
        .ws_io_num = config->i2s_speaker_lrclk,   // GPIO 33
        .data_out_num = config->i2s_speaker_dout, // GPIO 22
        .data_in_num = I2S_PIN_NO_CHANGE,
    };

    ESP_RETURN_ON_ERROR(i2s_set_pin(port, &pin_conf),
                        TAG, "I2S set pin failed");

    ESP_LOGI(TAG, "I2S speaker initialized: MCLK=%d BCLK=%d LRCLK=%d DOUT=%d",
             config->i2s_speaker_mclk, config->i2s_speaker_bclk,
             config->i2s_speaker_lrclk, config->i2s_speaker_dout);

    return ESP_OK;
}

/**
 * @brief Initialize I2S for microphone input (full-duplex mode)
 */
static esp_err_t init_i2s_microphone(i2s_port_t port, const m5_echo_base_config_t *config)
{
    ESP_LOGI(TAG, "Initializing I2S microphone input on port %d (full-duplex)", port);

    // I2S configuration for microphone
    i2s_config_t i2s_conf = {
        .mode = I2S_MODE_MASTER | I2S_MODE_RX,
        .sample_rate = config->sample_rate_hz,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,  // Mono
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = config->dma_buffer_count,
        .dma_buf_len = config->dma_buffer_len,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0,
    };

    ESP_RETURN_ON_ERROR(i2s_driver_install(port, &i2s_conf, 0, NULL),
                        TAG, "I2S driver install for microphone failed");

    // I2S pin configuration for microphone (shared pins with speaker)
    i2s_pin_config_t pin_conf = {
        .mck_io_num = config->i2s_mic_din,      // Shared with speaker BCLK
        .bck_io_num = config->i2s_mic_ws,       // Shared with speaker LRCLK
        .ws_io_num = config->i2s_mic_ws,        // Same as BCLK
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = config->i2s_mic_din,     // GPIO 23
    };

    ESP_RETURN_ON_ERROR(i2s_set_pin(port, &pin_conf),
                        TAG, "I2S microphone set pin failed");

    ESP_LOGW(TAG, "I2S microphone uses shared pins (full-duplex):");
    ESP_LOGW(TAG, "  DIN on GPIO %d (shared with speaker BCLK)", config->i2s_mic_din);
    ESP_LOGW(TAG, "  WS on GPIO %d (shared with speaker LRCLK)", config->i2s_mic_ws);

    return ESP_OK;
}

// ============================================================================
// PUBLIC API FUNCTIONS
// ============================================================================

esp_err_t m5_echo_base_init(m5_echo_base_t *dev, const m5_echo_base_config_t *config)
{
    ESP_RETURN_ON_FALSE(dev && config, ESP_ERR_INVALID_ARG, TAG, "Invalid arguments");

    memcpy(&dev->config, config, sizeof(m5_echo_base_config_t));

    // Initialize I2C bus for codec
    ESP_RETURN_ON_ERROR(init_i2c_bus(config->i2c_port, config->i2c_sda, config->i2c_scl,
                                     config->i2c_freq_hz),
                        TAG, "I2C bus initialization failed");

    // Initialize ES8311 codec
    ESP_RETURN_ON_ERROR(init_es8311_codec(config->i2c_port, config->sample_rate_hz),
                        TAG, "ES8311 codec initialization failed");

    // Initialize I2S for speaker output
    ESP_RETURN_ON_ERROR(init_i2s_speaker(config->i2s_port, config),
                        TAG, "I2S speaker initialization failed");

    // Initialize I2S for microphone input
    ESP_RETURN_ON_ERROR(init_i2s_microphone(config->i2s_mic_port, config),
                        TAG, "I2S microphone initialization failed");

    dev->initialized = true;
    ESP_LOGI(TAG, "M5 Echo Base audio system initialized successfully");

    return ESP_OK;
}

esp_err_t m5_echo_base_deinit(m5_echo_base_t *dev)
{
    ESP_RETURN_ON_FALSE(dev, ESP_ERR_INVALID_ARG, TAG, "Invalid device");

    // Stop I2S drivers
    i2s_driver_uninstall(dev->config.i2s_port);
    i2s_driver_uninstall(dev->config.i2s_mic_port);

    // Delete I2C driver
    i2c_driver_delete(dev->config.i2c_port);

    dev->initialized = false;
    ESP_LOGI(TAG, "M5 Echo Base audio system deinitialized");

    return ESP_OK;
}

esp_err_t m5_echo_base_write_speaker(m5_echo_base_t *dev, const void *data,
                                     size_t len, size_t *bytes_written,
                                     uint32_t ticks_to_wait)
{
    ESP_RETURN_ON_FALSE(dev && data && bytes_written, ESP_ERR_INVALID_ARG, TAG, "Invalid arguments");
    ESP_RETURN_ON_FALSE(dev->initialized, ESP_ERR_INVALID_STATE, TAG, "Device not initialized");

    return i2s_write(dev->config.i2s_port, data, len, bytes_written, ticks_to_wait);
}

esp_err_t m5_echo_base_read_microphone(m5_echo_base_t *dev, void *data,
                                       size_t len, size_t *bytes_read,
                                       uint32_t ticks_to_wait)
{
    ESP_RETURN_ON_FALSE(dev && data && bytes_read, ESP_ERR_INVALID_ARG, TAG, "Invalid arguments");
    ESP_RETURN_ON_FALSE(dev->initialized, ESP_ERR_INVALID_STATE, TAG, "Device not initialized");

    return i2s_read(dev->config.i2s_mic_port, data, len, bytes_read, ticks_to_wait);
}

esp_err_t m5_echo_base_set_volume(m5_echo_base_t *dev, uint8_t volume)
{
    ESP_RETURN_ON_FALSE(dev, ESP_ERR_INVALID_ARG, TAG, "Invalid device");
    ESP_RETURN_ON_FALSE(dev->initialized, ESP_ERR_INVALID_STATE, TAG, "Device not initialized");

    // Volume range 0-100, convert to DAC register value (0-31, approximately)
    uint8_t dac_vol = (volume * 31) / 100;

    return es8311_write_reg(dev->config.i2c_port, ES8311_REG_DAC_VOL, dac_vol);
}

esp_err_t m5_echo_base_get_volume(m5_echo_base_t *dev, uint8_t *volume)
{
    ESP_RETURN_ON_FALSE(dev && volume, ESP_ERR_INVALID_ARG, TAG, "Invalid arguments");
    ESP_RETURN_ON_FALSE(dev->initialized, ESP_ERR_INVALID_STATE, TAG, "Device not initialized");

    uint8_t dac_vol;
    ESP_RETURN_ON_ERROR(es8311_read_reg(dev->config.i2c_port, ES8311_REG_DAC_VOL, &dac_vol),
                        TAG, "Failed to read DAC volume");

    *volume = (dac_vol * 100) / 31;

    return ESP_OK;
}

esp_err_t m5_echo_base_set_led_color(m5_echo_base_t *dev, uint8_t red,
                                     uint8_t green, uint8_t blue)
{
    ESP_RETURN_ON_FALSE(dev, ESP_ERR_INVALID_ARG, TAG, "Invalid device");

    // TODO: Implement SK6812 LED control via RMT
    // This is a placeholder for future implementation
    ESP_LOGD(TAG, "Set LED color: R=%u G=%u B=%u", red, green, blue);

    return ESP_OK;
}
