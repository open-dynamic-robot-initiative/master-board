#ifndef DIRECT_WIFI_H
#define DIRECT_WIFI_H

#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "tcpip_adapter.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_now.h"
//#include "/home/etienne/.arduino15/packages/esp32/hardware/esp32/1.0.2/tools/sdk/include/esp32/esp_wifi_internal.h"

esp_err_t esp_wifi_internal_set_fix_rate(wifi_interface_t ifx, bool en, wifi_phy_rate_t rate);

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
#elif CONFIG_WIFI_DATARATE_56
  #define CONFIG_WIFI_DATARATE WIFI_PHY_RATE_56M
#else
  #error No valid WIFI datarate specified
#endif

void wifi_init();
void wifi_send_data(uint8_t *data, int len);
void wifi_attach_recv_cb(void (*cb)(uint8_t src_mac[6], uint8_t *data, int len));
void wifi_detach_recv_cb();


#endif
