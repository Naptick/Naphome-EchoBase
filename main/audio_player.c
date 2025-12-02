#include "audio_player.h"

#include <string.h>

#include "driver/i2c.h"
#include "driver/gpio.h"
// Compatibility: use old I2C API for ESP-IDF v4.4
#define i2c_master_bus_handle_t i2c_port_t
#define i2c_master_dev_handle_t i2c_cmd_handle_t
#include "esp_check.h"
#include "esp_log.h"

#define AUDIO_PLAYER_I2C_FREQ_HZ 100000
#define ES8311_ADDR_7BIT 0x18  // 7-bit I2C address (becomes 0x30 when shifted for 8-bit)

// ES8311 register definitions (from es8311_reg.h)
#define ES8311_RESET_REG00       0x00
#define ES8311_CLK_MANAGER_REG01 0x01
#define ES8311_CLK_MANAGER_REG02 0x02
#define ES8311_CLK_MANAGER_REG03 0x03
#define ES8311_CLK_MANAGER_REG04 0x04
#define ES8311_CLK_MANAGER_REG05 0x05
#define ES8311_CLK_MANAGER_REG06 0x06
#define ES8311_CLK_MANAGER_REG07 0x07
#define ES8311_CLK_MANAGER_REG08 0x08
#define ES8311_SDPIN_REG09       0x09
#define ES8311_SDPOUT_REG0A      0x0A
#define ES8311_SYSTEM_REG0B      0x0B
#define ES8311_SYSTEM_REG0C      0x0C
#define ES8311_SYSTEM_REG0D      0x0D
#define ES8311_SYSTEM_REG0E      0x0E
#define ES8311_SYSTEM_REG0F      0x0F
#define ES8311_SYSTEM_REG10      0x10
#define ES8311_SYSTEM_REG11      0x11
#define ES8311_SYSTEM_REG12      0x12
#define ES8311_SYSTEM_REG13      0x13
#define ES8311_SYSTEM_REG14      0x14
#define ES8311_ADC_REG15         0x15
#define ES8311_ADC_REG16         0x16
#define ES8311_ADC_REG17         0x17
#define ES8311_ADC_REG1B         0x1B
#define ES8311_ADC_REG1C         0x1C
#define ES8311_DAC_REG31         0x31
#define ES8311_DAC_REG32         0x32
#define ES8311_DAC_REG37         0x37
#define ES8311_GPIO_REG44        0x44
#define ES8311_GP_REG45          0x45

typedef struct {
    bool initialized;
    audio_player_config_t cfg;
    int current_sample_rate;
    i2c_master_bus_handle_t i2c_bus;
    i2c_master_dev_handle_t i2c_dev;
} audio_player_state_t;

static audio_player_state_t s_audio;
static const char *TAG = "audio_player";

static esp_err_t es8311_write_reg(uint8_t reg, uint8_t value)
{
    if (s_audio.i2c_bus == I2C_NUM_MAX) {
        return ESP_ERR_INVALID_STATE;
    }
    // Use ESP-IDF v4.4 I2C API
    // ES8311_ADDR_7BIT is 7-bit address, shift to get 8-bit write address
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (ES8311_ADDR_7BIT << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write_byte(cmd, value, true);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(s_audio.i2c_bus, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ES8311 write failed reg=0x%02x val=0x%02x err=%s", reg, value, esp_err_to_name(err));
    }
    return err;
}

static esp_err_t es8311_read_reg(uint8_t reg, uint8_t *value)
{
    if (s_audio.i2c_bus == I2C_NUM_MAX || value == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (ES8311_ADDR_7BIT << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (ES8311_ADDR_7BIT << 1) | I2C_MASTER_READ, true);
    i2c_master_read_byte(cmd, value, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(s_audio.i2c_bus, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    return err;
}

static esp_err_t es8311_probe(void)
{
    // Try to read chip ID to verify device is present
    uint8_t chip_id1 = 0, chip_id2 = 0;
    esp_err_t err1 = es8311_read_reg(0xFD, &chip_id1);
    esp_err_t err2 = es8311_read_reg(0xFE, &chip_id2);
    if (err1 == ESP_OK && err2 == ESP_OK) {
        ESP_LOGI(TAG, "ES8311 detected at 0x%02x: Chip ID1=0x%02x ID2=0x%02x", ES8311_ADDR_7BIT, chip_id1, chip_id2);
        return ESP_OK;
    }
    ESP_LOGW(TAG, "ES8311 probe failed at 0x%02x: err1=%s err2=%s", ES8311_ADDR_7BIT, esp_err_to_name(err1), esp_err_to_name(err2));
    return ESP_FAIL;
}

static esp_err_t es8311_config_clock_44100(void)
{
    // Configure clock for 44100 Hz when use_mclk=false (codec generates MCLK from BCLK)
    // When use_mclk=false, pre_multi should be 3 (x8) per es8311_config_sample logic
    // BCLK = sample_rate * bits_per_sample * channels = 44100 * 16 * 2 = 1.4112 MHz
    // MCLK will be generated internally from BCLK
    
    uint8_t regv;
    
    // CLK_MANAGER_REG02: pre_div=0 (means 1), pre_multi=3 (x8) when use_mclk=false
    ESP_RETURN_ON_ERROR(es8311_read_reg(ES8311_CLK_MANAGER_REG02, &regv), TAG, "read clk mgr 2");
    regv &= 0x07;  // Keep lower 3 bits
    regv |= (0 << 5);  // pre_div = 1 (register value 0)
    regv |= (3 << 3);  // pre_multi = x8 (register value 3) when use_mclk=false
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_CLK_MANAGER_REG02, regv), TAG, "clk mgr 2");
    
    // CLK_MANAGER_REG05: adc_div=0 (means 1), dac_div=0 (means 1)
    regv = 0x00;
    regv |= (0 << 4);  // adc_div = 1 (register value 0)
    regv |= (0 << 0);  // dac_div = 1 (register value 0)
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_CLK_MANAGER_REG05, regv), TAG, "clk mgr 5");
    
    // CLK_MANAGER_REG03: fs_mode=0, adc_osr=0x10
    ESP_RETURN_ON_ERROR(es8311_read_reg(ES8311_CLK_MANAGER_REG03, &regv), TAG, "read clk mgr 3");
    regv &= 0x80;  // Keep bit 7
    regv |= (0 << 6);  // fs_mode = 0
    regv |= 0x10;  // adc_osr = 0x10
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_CLK_MANAGER_REG03, regv), TAG, "clk mgr 3");
    
    // CLK_MANAGER_REG04: dac_osr=0x10
    ESP_RETURN_ON_ERROR(es8311_read_reg(ES8311_CLK_MANAGER_REG04, &regv), TAG, "read clk mgr 4");
    regv &= 0x80;  // Keep bit 7
    regv |= 0x10;  // dac_osr = 0x10
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_CLK_MANAGER_REG04, regv), TAG, "clk mgr 4");
    
    // CLK_MANAGER_REG07: lrck_h=0
    ESP_RETURN_ON_ERROR(es8311_read_reg(ES8311_CLK_MANAGER_REG07, &regv), TAG, "read clk mgr 7");
    regv &= 0xC0;  // Keep upper 2 bits
    regv |= 0x00;  // lrck_h = 0
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_CLK_MANAGER_REG07, regv), TAG, "clk mgr 7");
    
    // CLK_MANAGER_REG08: lrck_l=0xff
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_CLK_MANAGER_REG08, 0xFF), TAG, "clk mgr 8");
    
    // CLK_MANAGER_REG06: bclk_div=4 (register value 3)
    ESP_RETURN_ON_ERROR(es8311_read_reg(ES8311_CLK_MANAGER_REG06, &regv), TAG, "read clk mgr 6");
    regv &= 0xE0;  // Keep upper 3 bits
    regv |= 0x03;  // bclk_div = 4 (register value 3)
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_CLK_MANAGER_REG06, regv), TAG, "clk mgr 6");
    
    return ESP_OK;
}

static esp_err_t es8311_init(void)
{
    // Probe device first
    ESP_LOGI(TAG, "Probing ES8311 at I2C address 0x%02x (7-bit)...", ES8311_ADDR_7BIT);
    esp_err_t probe_err = es8311_probe();
    if (probe_err != ESP_OK) {
        ESP_LOGW(TAG, "ES8311 probe failed, continuing anyway...");
    }
    
    // ES8311 initialization sequence for DAC mode (playback only)
    // Based on ESP codec dev library implementation
    
    // Initial setup (from es8311_open)
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_GPIO_REG44, 0x08), TAG, "gpio 44"); // Enhance I2C noise immunity
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_GPIO_REG44, 0x08), TAG, "gpio 44"); // Second write for reliability
    
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_CLK_MANAGER_REG01, 0x30), TAG, "clk mgr 1 init");
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_CLK_MANAGER_REG02, 0x00), TAG, "clk mgr 2 init");
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_CLK_MANAGER_REG03, 0x10), TAG, "clk mgr 3 init");
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_ADC_REG16, 0x24), TAG, "adc 16 init");
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_CLK_MANAGER_REG04, 0x10), TAG, "clk mgr 4 init");
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_CLK_MANAGER_REG05, 0x00), TAG, "clk mgr 5 init");
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_SYSTEM_REG0B, 0x00), TAG, "sys 0B init");
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_SYSTEM_REG0C, 0x00), TAG, "sys 0C init");
    // SYSTEM_REG10: HPOUT control - disable headphone output
    // Bit 7 typically enables/disables HPOUT, lower bits are volume
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_SYSTEM_REG10, 0x00), TAG, "sys 10 init"); // Disable HPOUT (bit 7=0)
    // SYSTEM_REG11: SPKOUT control - enable speaker output
    // Bit 7 typically enables SPKOUT (1=enable), lower bits are volume
    // Try 0x80 first (enable only), then we can add volume
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_SYSTEM_REG11, 0x80), TAG, "sys 11 init"); // Enable SPKOUT (bit 7=1), min volume
    
    // Reset
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_RESET_REG00, 0x80), TAG, "reset"); // Reset, slave mode
    vTaskDelay(pdMS_TO_TICKS(10)); // Small delay after reset
    
    // Clock manager: Don't use external MCLK (use_mclk=false), generate from BCLK, no invert, slave mode
    // When use_mclk=false, bit 7 should be set (0xBF = 0x3F | 0x80)
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_CLK_MANAGER_REG01, 0xBF), TAG, "clk mgr 1"); // No external MCLK, generate from BCLK
    
    // Configure clock for 44100 Hz
    ESP_RETURN_ON_ERROR(es8311_config_clock_44100(), TAG, "clock config");
    
    // I2S interface configuration - I2S format, 16-bit
    // Read-modify-write to enable DAC interface (clear bit 6) - must be done after clock config
    uint8_t dac_iface, adc_iface;
    ESP_RETURN_ON_ERROR(es8311_read_reg(ES8311_SDPIN_REG09, &dac_iface), TAG, "read sdp in");
    ESP_RETURN_ON_ERROR(es8311_read_reg(ES8311_SDPOUT_REG0A, &adc_iface), TAG, "read sdp out");
    dac_iface &= 0xBF;  // Clear bit 6 first
    adc_iface &= 0xBF;  // Clear bit 6 first
    // For DAC mode, clear bit 6 again (equivalent to &= ~(BITS(6)))
    dac_iface &= ~(0x40);  // Clear bit 6 to enable DAC interface for playback
    // Set I2S format: 0x0C = I2S format, 16-bit
    dac_iface |= 0x0C;  // I2S format, 16-bit
    adc_iface |= 0x0C;  // I2S format, 16-bit
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_SDPIN_REG09, dac_iface), TAG, "sdp in"); // DAC I2S, 16-bit, enabled
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_SDPOUT_REG0A, adc_iface), TAG, "sdp out"); // ADC I2S, 16-bit
    
    // System configuration (from es8311_start)
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_ADC_REG17, 0xBF), TAG, "adc 17");
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_SYSTEM_REG0E, 0x02), TAG, "sys 0E");
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_SYSTEM_REG12, 0x00), TAG, "sys 12"); // Enable DAC (0x00 = enable)
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_SYSTEM_REG14, 0x1A), TAG, "sys 14"); // Analog PGA gain
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_SYSTEM_REG0D, 0x01), TAG, "sys 0D"); // Power up
    // SYSTEM_REG0F: Output path selection
    // First disable all outputs, then enable only SPKOUT
    // Try disabling HPOUT explicitly by clearing bits 0-1, enabling SPKOUT with bits 2-3
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_SYSTEM_REG0F, 0x00), TAG, "sys 0F disable all"); // Disable all first
    vTaskDelay(pdMS_TO_TICKS(10)); // Small delay
    // Now enable only SPKOUT - try bits 2-3 (0x0C) or maybe it's inverted (need to clear bits 0-1)
    uint8_t reg0f_value = 0x0C;  // Enable SPKOUT (bits 2,3), disable HPOUT (bits 0,1 clear)
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_SYSTEM_REG0F, reg0f_value), TAG, "sys 0F enable SPKOUT");
    // Read back to verify
    uint8_t reg0f_readback = 0;
    if (es8311_read_reg(ES8311_SYSTEM_REG0F, &reg0f_readback) == ESP_OK) {
        ESP_LOGI(TAG, "REG0F written=0x%02x, readback=0x%02x", reg0f_value, reg0f_readback);
    }
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_ADC_REG15, 0x40), TAG, "adc 15");
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_DAC_REG37, 0x08), TAG, "dac 37"); // Ramp rate
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_GP_REG45, 0x00), TAG, "gp 45");
    
    // DAC configuration
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_DAC_REG31, 0x00), TAG, "dac 31"); // Unmute
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_DAC_REG32, 0xC0), TAG, "dac 32"); // Volume (0xC0 = ~0dB, higher value = louder)
    
    // Additional registers from es8311_open
    // SYSTEM_REG13: May control output routing or bias
    // Try 0x30 or 0x00 to see if it affects output path
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_SYSTEM_REG13, 0x30), TAG, "sys 13"); // Try different value for speaker routing
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_ADC_REG1B, 0x0A), TAG, "adc 1B");
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_ADC_REG1C, 0x6A), TAG, "adc 1C");
    
    // Enable power amplifier (GPIO38 on Korvo1)
    // This is critical for audio output!
    gpio_config_t pa_gpio_conf = {
        .pin_bit_mask = (1ULL << GPIO_NUM_38),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&pa_gpio_conf), TAG, "pa gpio config");
    ESP_RETURN_ON_ERROR(gpio_set_level(GPIO_NUM_38, 1), TAG, "pa enable"); // Set high to enable PA
    ESP_LOGI(TAG, "Power amplifier enabled on GPIO38");
    
    ESP_LOGI(TAG, "ES8311 initialized for 44100 Hz playback");
    
    // Explicitly enable/start the codec (equivalent to esp_codec_dev_open)
    // This ensures the codec is in the correct state for playback
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_SYSTEM_REG0D, 0x01), TAG, "sys 0D enable"); // Ensure power up
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_SYSTEM_REG12, 0x00), TAG, "sys 12 enable"); // Ensure DAC enabled
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_SYSTEM_REG10, 0x00), TAG, "sys 10 disable HPOUT"); // Ensure HPOUT disabled
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_SYSTEM_REG11, 0x80), TAG, "sys 11 enable SPKOUT"); // Ensure SPKOUT enabled (bit 7=1)
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_SYSTEM_REG0F, 0x0C), TAG, "sys 0F enable SPKOUT only"); // Ensure only SPKOUT enabled
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_DAC_REG31, 0x00), TAG, "dac 31 unmute"); // Ensure unmuted
    
    // Diagnostic: Read back key registers to verify configuration
    uint8_t reg10_val = 0, reg11_val = 0, reg0f_val = 0, reg12_val = 0;
    if (es8311_read_reg(ES8311_SYSTEM_REG10, &reg10_val) == ESP_OK &&
        es8311_read_reg(ES8311_SYSTEM_REG11, &reg11_val) == ESP_OK &&
        es8311_read_reg(ES8311_SYSTEM_REG0F, &reg0f_val) == ESP_OK &&
        es8311_read_reg(ES8311_SYSTEM_REG12, &reg12_val) == ESP_OK) {
        ESP_LOGI(TAG, "ES8311 output config: REG10=0x%02x (HPOUT), REG11=0x%02x (SPKOUT), REG0F=0x%02x (path), REG12=0x%02x (DAC)",
                 reg10_val, reg11_val, reg0f_val, reg12_val);
    }
    
    vTaskDelay(pdMS_TO_TICKS(50)); // Give codec time to stabilize
    
    ESP_LOGI(TAG, "ES8311 codec enabled and ready for playback");
    return ESP_OK;
}

static void scan_i2c_bus(void)
{
    ESP_LOGI(TAG, "Scanning I2C bus...");
    int devices_found = 0;
    for (uint8_t addr = 0x08; addr < 0x78; addr++) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);
        esp_err_t ret = i2c_master_cmd_begin(s_audio.i2c_bus, cmd, pdMS_TO_TICKS(50));
        i2c_cmd_link_delete(cmd);
        
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "I2C device found at address 0x%02X", addr);
            devices_found++;
        }
    }
    if (devices_found == 0) {
        ESP_LOGW(TAG, "No I2C devices found!");
    } else {
        ESP_LOGI(TAG, "Found %d I2C device(s)", devices_found);
    }
}

static esp_err_t configure_i2c(const audio_player_config_t *cfg)
{
    // Initialize I2C master bus on I2C_NUM_0 (Korvo1 uses I2C_NUM_0 for codec)
    // Use ESP-IDF v4.4 I2C API
    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = cfg->i2c_sda_gpio,
        .scl_io_num = cfg->i2c_scl_gpio,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = AUDIO_PLAYER_I2C_FREQ_HZ,
    };
    ESP_RETURN_ON_ERROR(i2c_param_config(I2C_NUM_0, &i2c_conf), TAG, "i2c param config");
    ESP_RETURN_ON_ERROR(i2c_driver_install(I2C_NUM_0, i2c_conf.mode, 0, 0, 0), TAG, "i2c driver install");
    s_audio.i2c_bus = I2C_NUM_0;
    s_audio.i2c_dev = NULL; // Not used with v4.4 API
    
    // Give I2C bus time to stabilize
    vTaskDelay(pdMS_TO_TICKS(50));
    
    // Scan I2C bus to see what devices are present
    scan_i2c_bus();
    
    return ESP_OK;
}

static esp_err_t configure_i2s(const audio_player_config_t *cfg)
{
    i2s_config_t i2s_conf = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX,
        .sample_rate = cfg->default_sample_rate,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 6,
        .dma_buf_len = 256,
        .use_apll = false,  // Don't use APLL when codec generates MCLK from BCLK
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0,  // No fixed MCLK - codec generates it from BCLK (use_mclk=false)
    };

    i2s_pin_config_t pin_conf = {
        .mck_io_num = cfg->mclk_gpio,
        .bck_io_num = cfg->bclk_gpio,
        .ws_io_num = cfg->lrclk_gpio,
        .data_out_num = cfg->data_gpio,
        .data_in_num = I2S_PIN_NO_CHANGE,
    };

    ESP_RETURN_ON_ERROR(i2s_driver_install(cfg->i2s_port, &i2s_conf, 0, NULL), TAG, "i2s install");
    ESP_RETURN_ON_ERROR(i2s_set_pin(cfg->i2s_port, &pin_conf), TAG, "i2s pins");
    ESP_RETURN_ON_ERROR(i2s_zero_dma_buffer(cfg->i2s_port), TAG, "i2s zero");
    ESP_RETURN_ON_ERROR(i2s_start(cfg->i2s_port), TAG, "i2s start"); // Start I2S driver
    ESP_LOGI(TAG, "I2S driver started on port %d", cfg->i2s_port);
    return ESP_OK;
}

esp_err_t audio_player_init(const audio_player_config_t *cfg)
{
    ESP_RETURN_ON_FALSE(cfg, ESP_ERR_INVALID_ARG, TAG, "cfg required");
    ESP_RETURN_ON_FALSE(cfg->bclk_gpio >= 0 && cfg->lrclk_gpio >= 0 && cfg->data_gpio >= 0,
                        ESP_ERR_INVALID_ARG, TAG, "invalid pins");

    if (s_audio.initialized) {
        return ESP_OK;
    }

    s_audio.cfg = *cfg;
    s_audio.current_sample_rate = cfg->default_sample_rate > 0 ? cfg->default_sample_rate : 44100;

    ESP_RETURN_ON_ERROR(configure_i2c(cfg), TAG, "i2c setup");
    vTaskDelay(pdMS_TO_TICKS(50)); // Give I2C bus more time to stabilize
    ESP_RETURN_ON_ERROR(configure_i2s(cfg), TAG, "i2s setup");
    ESP_RETURN_ON_ERROR(es8311_init(), TAG, "codec init");

    s_audio.initialized = true;
    ESP_LOGI(TAG, "Audio player ready (sr=%d)", s_audio.current_sample_rate);
    return ESP_OK;
}

static esp_err_t ensure_sample_rate(int sample_rate_hz)
{
    if (!s_audio.initialized || sample_rate_hz <= 0) {
        return ESP_ERR_INVALID_STATE;
    }
    if (sample_rate_hz == s_audio.current_sample_rate) {
        return ESP_OK;
    }
    ESP_RETURN_ON_ERROR(
        i2s_set_clk(s_audio.cfg.i2s_port,
                    sample_rate_hz,
                    I2S_BITS_PER_SAMPLE_16BIT,
                    I2S_CHANNEL_STEREO),
        TAG,
        "set clk");
    s_audio.current_sample_rate = sample_rate_hz;
    ESP_LOGI(TAG, "Playback sample rate -> %d Hz", sample_rate_hz);
    return ESP_OK;
}

static esp_err_t write_pcm_frames(const int16_t *samples, size_t sample_count, int num_channels)
{
    ESP_RETURN_ON_FALSE(samples && sample_count > 0, ESP_ERR_INVALID_ARG, TAG, "bad pcm args");
    ESP_RETURN_ON_FALSE(num_channels == 1 || num_channels == 2, ESP_ERR_INVALID_ARG, TAG, "channels");

    const size_t chunk_frames = 256;
    int16_t stereo_buffer[chunk_frames * 2];

    size_t frames_written = 0;
    while (frames_written < sample_count) {
        size_t frames_this = chunk_frames;
        if (frames_this > sample_count - frames_written) {
            frames_this = sample_count - frames_written;
        }

        if (num_channels == 1) {
            for (size_t i = 0; i < frames_this; ++i) {
                int16_t sample = samples[frames_written + i];
                stereo_buffer[2 * i] = sample;
                stereo_buffer[2 * i + 1] = sample;
            }
        } else {
            memcpy(stereo_buffer,
                   &samples[(frames_written)*2],
                   frames_this * sizeof(int16_t) * 2);
        }

        size_t bytes_to_write = frames_this * sizeof(int16_t) * 2;
        size_t total_written = 0;
        while (total_written < bytes_to_write) {
            size_t bytes_written = 0;
            esp_err_t err = i2s_write(s_audio.cfg.i2s_port,
                                      (uint8_t *)stereo_buffer + total_written,
                                      bytes_to_write - total_written,
                                      &bytes_written,
                                      portMAX_DELAY);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "I2S write failed: %s", esp_err_to_name(err));
                return err;
            }
            if (bytes_written == 0) {
                ESP_LOGW(TAG, "I2S write returned 0 bytes");
                vTaskDelay(pdMS_TO_TICKS(1));
                continue;
            }
            total_written += bytes_written;
        }
        frames_written += frames_this;
    }
    return ESP_OK;
}

typedef struct __attribute__((packed)) {
    char chunk_id[4];
    uint32_t chunk_size;
    char format[4];
} wav_header_t;

typedef struct __attribute__((packed)) {
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
} wav_fmt_body_t;

esp_err_t audio_player_play_wav(const uint8_t *wav_data, size_t wav_len)
{
    ESP_RETURN_ON_FALSE(s_audio.initialized, ESP_ERR_INVALID_STATE, TAG, "not init");
    ESP_RETURN_ON_FALSE(wav_data && wav_len > sizeof(wav_header_t), ESP_ERR_INVALID_ARG, TAG, "bad wav");

    const uint8_t *ptr = wav_data;
    const uint8_t *end = wav_data + wav_len;
    const wav_header_t *hdr = (const wav_header_t *)ptr;
    if (memcmp(hdr->chunk_id, "RIFF", 4) != 0 || memcmp(hdr->format, "WAVE", 4) != 0) {
        return ESP_ERR_INVALID_ARG;
    }
    ptr += sizeof(wav_header_t);

    wav_fmt_body_t fmt = {0};
    bool fmt_found = false;
    const uint8_t *data_ptr = NULL;
    uint32_t data_size = 0;

    while (ptr + 8 <= end) {
        char chunk_id[4];
        memcpy(chunk_id, ptr, 4);
        uint32_t chunk_size = *(const uint32_t *)(ptr + 4);
        ptr += 8;
        if (ptr + chunk_size > end) {
            return ESP_ERR_INVALID_ARG;
        }

        if (!fmt_found && memcmp(chunk_id, "fmt ", 4) == 0) {
            if (chunk_size < sizeof(wav_fmt_body_t)) {
                return ESP_ERR_INVALID_ARG;
            }
            memcpy(&fmt, ptr, sizeof(wav_fmt_body_t));
            fmt_found = true;
        } else if (memcmp(chunk_id, "data", 4) == 0) {
            data_ptr = ptr;
            data_size = chunk_size;
            break;
        } else {
            uint32_t advance = chunk_size;
            if (advance & 1) {
                advance++;
            }
            ptr += advance;
        }
    }

    ESP_RETURN_ON_FALSE(fmt_found && data_ptr, ESP_ERR_INVALID_ARG, TAG, "wav missing fmt/data");
    ESP_RETURN_ON_FALSE(fmt.audio_format == 1, ESP_ERR_NOT_SUPPORTED, TAG, "PCM required");
    ESP_RETURN_ON_FALSE(fmt.bits_per_sample == 16, ESP_ERR_NOT_SUPPORTED, TAG, "16-bit required");

    size_t sample_count = data_size / (fmt.bits_per_sample / 8);
    const int16_t *samples = (const int16_t *)data_ptr;

    ESP_RETURN_ON_ERROR(ensure_sample_rate(fmt.sample_rate), TAG, "sr");
    return write_pcm_frames(samples, sample_count / fmt.num_channels, fmt.num_channels);
}

esp_err_t audio_player_submit_pcm(const int16_t *samples,
                                  size_t sample_count,
                                  int sample_rate_hz,
                                  int num_channels)
{
    ESP_RETURN_ON_FALSE(s_audio.initialized, ESP_ERR_INVALID_STATE, TAG, "not init");
    ESP_RETURN_ON_ERROR(ensure_sample_rate(sample_rate_hz), TAG, "sr");
    return write_pcm_frames(samples, sample_count, num_channels);
}

void audio_player_shutdown(void)
{
    if (!s_audio.initialized) {
        return;
    }
    i2s_driver_uninstall(s_audio.cfg.i2s_port);
    
    // Clean up I2C
    if (s_audio.i2c_bus != I2C_NUM_MAX) {
        i2c_driver_delete(s_audio.i2c_bus);
        s_audio.i2c_bus = I2C_NUM_MAX;
    }
    s_audio.i2c_dev = NULL;
    
    memset(&s_audio, 0, sizeof(s_audio));
}
