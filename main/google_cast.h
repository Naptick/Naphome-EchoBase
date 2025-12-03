#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize Google Cast Audio receiver
 * @param device_name: Name to advertise for Cast discovery
 * @return ESP_OK on success
 */
esp_err_t google_cast_init(const char *device_name);

/**
 * Start Google Cast Audio service
 * @return ESP_OK on success
 */
esp_err_t google_cast_start(void);

/**
 * Stop Google Cast Audio service
 */
void google_cast_stop(void);

/**
 * Get current playback state
 * @return true if playing, false otherwise
 */
bool google_cast_is_playing(void);

#ifdef __cplusplus
}
#endif
