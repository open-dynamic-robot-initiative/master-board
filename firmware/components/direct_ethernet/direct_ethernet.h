#ifndef DIRECT_ETHERNET_H
#define DIRECT_ETHERNET_H

#include <stdint.h>
#include <string.h>

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_eth.h"
#include "esp_event.h"
#include "esp_log.h"

#include "sdkconfig.h"

#define ETHERTYPE 0xb588

typedef struct {
  uint8_t dst_mac[6];
  uint8_t src_mac[6];
  uint16_t ethertype;

  /* Custom payload*/
  uint16_t data_len;
  uint8_t data[CONFIG_MAX_ETH_DATA_LEN];
} eth_frame;

extern uint8_t eth_src_mac[6];
extern uint8_t eth_dst_mac[6];

void (*eth_recv_cb)(uint8_t src_mac[6], uint8_t *data, int len);

void eth_init();
esp_err_t eth_send_frame(eth_frame *p_frame);
void eth_init_frame(eth_frame *p_frame);
void eth_send_data(uint8_t* data, int len);
void eth_attach_recv_cb(void (*cb)(uint8_t src_mac[6], uint8_t *data, int len));
void eth_detach_recv_cb();

#endif
