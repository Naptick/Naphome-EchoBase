#pragma once

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mp3_decoder mp3_decoder_t;

/**
 * @brief Create an MP3 decoder instance
 * @return Decoder handle or NULL on failure
 */
mp3_decoder_t *mp3_decoder_create(void);

/**
 * @brief Destroy an MP3 decoder instance
 * @param decoder Decoder handle
 */
void mp3_decoder_destroy(mp3_decoder_t *decoder);

/**
 * @brief Decode MP3 data to PCM
 * @param decoder Decoder handle
 * @param mp3_data Input MP3 data
 * @param mp3_len Length of MP3 data in bytes
 * @param pcm_out Output buffer for PCM samples (16-bit signed)
 * @param pcm_out_size Size of output buffer in samples
 * @param samples_decoded Output: number of samples decoded
 * @param sample_rate Output: sample rate in Hz
 * @param channels Output: number of channels (1=mono, 2=stereo)
 * @param bytes_consumed Output: number of bytes consumed from input (optional, can be NULL)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t mp3_decoder_decode(mp3_decoder_t *decoder,
                             const uint8_t *mp3_data,
                             size_t mp3_len,
                             int16_t *pcm_out,
                             size_t pcm_out_size,
                             size_t *samples_decoded,
                             int *sample_rate,
                             int *channels,
                             size_t *bytes_consumed);

#ifdef __cplusplus
}
#endif
