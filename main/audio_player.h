#pragma once

#include "esp_err.h"
#include "hal/gpio_types.h"
// Suppress deprecated I2S API warning - will migrate to new API in future
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcpp"
#include "driver/i2s.h"
#pragma GCC diagnostic pop

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    i2s_port_t i2s_port;
    gpio_num_t bclk_gpio;
    gpio_num_t lrclk_gpio;
    gpio_num_t data_gpio;
    gpio_num_t mclk_gpio;
    gpio_num_t i2c_scl_gpio;
    gpio_num_t i2c_sda_gpio;
    int default_sample_rate;
} audio_player_config_t;

typedef void (*audio_progress_callback_t)(float progress, bool playing);

esp_err_t audio_player_init(const audio_player_config_t *cfg);
esp_err_t audio_player_play_wav(const uint8_t *wav_data, size_t wav_len, audio_progress_callback_t progress_cb);
esp_err_t audio_player_submit_pcm(const int16_t *samples,
                                  size_t sample_count,
                                  int sample_rate_hz,
                                  int num_channels);
void audio_player_shutdown(void);

#ifdef __cplusplus
}
#endif
