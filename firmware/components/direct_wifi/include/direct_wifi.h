#ifndef DIRECT_WIFI_H
#define DIRECT_WIFI_H

#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_now.h"

#if CONFIG_WIFI_DATARATE_6
#define CONFIG_WIFI_DATARATE WIFI_PHY_RATE_6M
#elif CONFIG_WIFI_DATARATE_9
#define CONFIG_WIFI_DATARATE WIFI_PHY_RATE_9M
#elif CONFIG_WIFI_DATARATE_12
#define CONFIG_WIFI_DATARATE WIFI_PHY_RATE_12M
#elif CONFIG_WIFI_DATARATE_18
#define CONFIG_WIFI_DATARATE WIFI_PHY_RATE_18M
#elif CONFIG_WIFI_DATARATE_24
#define CONFIG_WIFI_DATARATE WIFI_PHY_RATE_24M
#elif CONFIG_WIFI_DATARATE_36
#define CONFIG_WIFI_DATARATE WIFI_PHY_RATE_36M
#elif CONFIG_WIFI_DATARATE_48
#define CONFIG_WIFI_DATARATE WIFI_PHY_RATE_48M
#elif CONFIG_WIFI_DATARATE_54
#define CONFIG_WIFI_DATARATE WIFI_PHY_RATE_54M
#else
#error No valid WIFI datarate specified
#endif

extern void (*wifi_recv_cb)(const esp_now_recv_info_t *esp_now_info, const uint8_t *data, int data_len);

void wifi_init();
void wifi_deinit_func();
void wifi_send_data(uint8_t *data, int len);
void wifi_attach_recv_cb(void (*cb)(const esp_now_recv_info_t *esp_now_info, const uint8_t *data, int data_len));
void wifi_detach_recv_cb();
void wifi_change_channel(uint8_t wifi_channel);
void wifi_recv_func(const esp_now_recv_info_t *esp_now_info, const uint8_t *data, int data_len);

#endif
