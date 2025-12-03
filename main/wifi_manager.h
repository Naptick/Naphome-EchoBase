#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * WiFi configuration structure
 */
typedef struct {
    char ssid[32];
    char password[64];
} wifi_manager_config_t;

/**
 * Initialize WiFi manager
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_init(void);

/**
 * Connect to WiFi network
 * @param config: WiFi credentials
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_connect(const wifi_manager_config_t *config);

/**
 * Disconnect from WiFi
 */
void wifi_manager_disconnect(void);

/**
 * Check if WiFi is connected
 * @return true if connected
 */
bool wifi_manager_is_connected(void);

/**
 * Get current IP address
 * @param ip_str: Buffer to store IP address string (must be at least 16 bytes)
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_get_ip(char *ip_str, size_t len);

#ifdef __cplusplus
}
#endif
