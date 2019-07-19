#ifndef DIRECT_ETHERNET_H
#define DIRECT_ETHERNET_H

#include <stdint.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_eth.h"
#include "tcpip_adapter.h"

#include "eth_phy/phy_lan8720.h"

#define CONFIG_PHY_CLOCK_MODE 3
#define ETHERNET_PHY_CONFIG phy_lan8720_default_ethernet_config
#define PIN_SMI_MDC 23
#define PIN_SMI_MDIO 18

#define DEVICE_IP       "192.168.1.0"
#define DEVICE_NETMASK  "255.255.255.0"
#define DEVICE_GW       "192.168.1.1"

#define WIFI_CONNECTED_BIT BIT0
#define ETHERTYPE 0xb588

EventGroupHandle_t udp_event_group;

typedef struct {
  uint8_t dst_mac[6];
  uint8_t src_mac[6];
  uint16_t ethertype;

  /* Custom payload*/
  uint16_t data_len;
  uint8_t data[CONFIG_MAX_ETH_DATA_LEN];
} eth_frame;

uint8_t eth_src_mac[6];
uint8_t eth_dst_mac[6];

void (*eth_recv_cb)(uint8_t src_mac[6], uint8_t *data, int len);

void eth_init();
esp_err_t eth_send_frame(eth_frame *p_frame);
void eth_init_frame(eth_frame *p_frame);
void eth_attach_recv_cb(void (*cb)(uint8_t src_mac[6], uint8_t *data, int len));
void eth_detach_recv_cb();

#endif
