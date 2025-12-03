#pragma once

#include "esp_err.h"
#include "mbedtls/base64.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/**
 * Streaming Base64 Decoder
 * Handles incomplete base64 chunks by buffering incomplete groups (4 bytes -> 3 bytes)
 */
typedef struct {
    uint8_t pending[4];      // Buffer for incomplete base64 group (0-3 bytes)
    size_t pending_len;      // Number of pending bytes (0-3)
    bool started;            // True after first data received
} streaming_base64_decoder_t;

static inline void streaming_base64_decoder_init(streaming_base64_decoder_t *dec) {
    dec->pending_len = 0;
    dec->started = false;
}

/**
 * Decode base64 data incrementally
 * @param dec: decoder state
 * @param input: base64 input data
 * @param input_len: length of input
 * @param output: decoded PCM data output buffer
 * @param output_len: IN: capacity, OUT: bytes written
 * @return ESP_OK on success, ESP_ERR_NO_MEM if output buffer too small
 */
static inline esp_err_t streaming_base64_decode(
    streaming_base64_decoder_t *dec,
    const uint8_t *input,
    size_t input_len,
    uint8_t *output,
    size_t *output_len)
{
    if (!dec || !input || !output || !output_len) {
        return ESP_ERR_INVALID_ARG;
    }

    size_t out_pos = 0;
    size_t out_cap = *output_len;

    // Combine pending bytes with new input for processing
    size_t to_process = input_len + dec->pending_len;

    // Process complete 4-byte base64 groups (decode to 3 bytes)
    size_t complete_groups = to_process / 4;
    size_t remainder = to_process % 4;

    // Create a temporary buffer if we have pending bytes
    uint8_t temp_input[256];
    size_t temp_len = 0;

    if (dec->pending_len > 0) {
        // Copy pending bytes to temp buffer
        memcpy(temp_input, dec->pending, dec->pending_len);
        temp_len = dec->pending_len;

        // Copy new input bytes
        memcpy(temp_input + temp_len, input, input_len);
        temp_len += input_len;
    } else {
        memcpy(temp_input, input, input_len);
        temp_len = input_len;
    }

    // Decode complete 4-byte groups
    if (complete_groups > 0) {
        size_t decoded_len = 0;
        int ret = mbedtls_base64_decode(
            output,
            out_cap,
            &decoded_len,
            temp_input,
            complete_groups * 4);

        if (ret != 0) {
            return ESP_FAIL;
        }

        out_pos = decoded_len;
        if (out_pos > out_cap) {
            return ESP_ERR_NO_MEM;
        }
    }

    // Save remainder for next call
    if (remainder > 0) {
        memcpy(dec->pending, temp_input + (complete_groups * 4), remainder);
        dec->pending_len = remainder;
    } else {
        dec->pending_len = 0;
    }

    *output_len = out_pos;
    return ESP_OK;
}

/**
 * Finalize decoding (decode any remaining pending bytes)
 * Call this after all base64 data has been received
 * @param dec: decoder state
 * @param output: output buffer for final decoded bytes
 * @param output_len: IN: capacity, OUT: bytes written
 * @return ESP_OK on success
 */
static inline esp_err_t streaming_base64_decode_finish(
    streaming_base64_decoder_t *dec,
    uint8_t *output,
    size_t *output_len)
{
    if (!dec || !output || !output_len) {
        return ESP_ERR_INVALID_ARG;
    }

    if (dec->pending_len == 0) {
        *output_len = 0;
        return ESP_OK;
    }

    // Pad incomplete group with '=' for proper base64 decoding
    uint8_t padded[4];
    memcpy(padded, dec->pending, dec->pending_len);
    memset(padded + dec->pending_len, '=', 4 - dec->pending_len);

    size_t decoded_len = 0;
    int ret = mbedtls_base64_decode(output, *output_len, &decoded_len, padded, 4);

    dec->pending_len = 0;

    if (ret != 0) {
        return ESP_FAIL;
    }

    *output_len = decoded_len;
    return ESP_OK;
}
