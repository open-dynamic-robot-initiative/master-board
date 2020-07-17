#include "direct_ethernet.h"

uint8_t eth_src_mac[6] = {0};
uint8_t eth_dst_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

void (*eth_recv_cb)(uint8_t src_mac[6], uint8_t *data, int len, char eth_or_wifi) = NULL; // eth_or_wifi = 'e' when eth is used, 'w' when wifi is used

void (*eth_link_state_cb)(bool link_state) = NULL;

static const char *ETH_TAG = "Direct_Ethernet";

static void eth_gpio_config_rmii(void)
{
  // RMII data pins are fixed:
  // TXD0 = GPIO19
  // TXD1 = GPIO22
  // TX_EN = GPIO21
  // RXD0 = GPIO25
  // RXD1 = GPIO26
  // CLK == GPIO0
  phy_rmii_configure_data_interface_pins();
  phy_rmii_smi_configure_pins(PIN_SMI_MDC, PIN_SMI_MDIO);
}

static void eth_event_handler(void* arg, esp_event_base_t event_base, 
                              int32_t event_id, void* event_data)
{
  tcpip_adapter_ip_info_t ipInfo;

  switch (event_id)
  {
  case ETHERNET_EVENT_CONNECTED:
    if (eth_link_state_cb == NULL)
    {
      ESP_LOGW(ETH_TAG, "Ethernet link Up but no callback function is set");
    }
    else
    {
      eth_link_state_cb(true);
    }

    esp_eth_get_mac(eth_src_mac);

    ESP_LOGI(ETH_TAG, "Ethernet Link Up");
    ESP_LOGI(ETH_TAG, "Ethernet HW Addr %02x:%02x:%02x:%02x:%02x:%02x",
             eth_src_mac[0], eth_src_mac[1], eth_src_mac[2], eth_src_mac[3], eth_src_mac[4], eth_src_mac[5]);
    break;
  case ETHERNET_EVENT_DISCONNECTED:
    if (eth_link_state_cb == NULL)
    {
      ESP_LOGW(ETH_TAG, "Ethernet link Down but no callback function is set");
    }
    else
    {
      eth_link_state_cb(false);
    }
    ESP_LOGI(ETH_TAG, "Ethernet Link Down");
    break;
  case ETHERNET_EVENT_START:
    ESP_LOGI(ETH_TAG, "Ethernet Started");
    break;
  case ETHERNET_EVENT_STOP:
    ESP_LOGI(ETH_TAG, "Ethernet Stopped");
    break;
  case IP_EVENT_ETH_GOT_IP:
    ESP_LOGI(ETH_TAG, "Ethernet got IP");
    ESP_ERROR_CHECK(tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_ETH, &ipInfo));
    ESP_LOGI(ETH_TAG, "TCP/IP initialization finished.");
    ESP_LOGI(ETH_TAG, "TCP|IP \t IP:" IPSTR, IP2STR(&ipInfo.ip));
    ESP_LOGI(ETH_TAG, "TCP|IP \t MASK:" IPSTR, IP2STR(&ipInfo.netmask));
    ESP_LOGI(ETH_TAG, "TCP|IP \t GW:" IPSTR, IP2STR(&ipInfo.gw));
    xEventGroupSetBits(udp_event_group, WIFI_CONNECTED_BIT);
    break;
  default:
    ESP_LOGI(ETH_TAG, "Unhandled Ethernet event (id = %d)", event_id);
    break;
  }
}


static esp_err_t eth_recv_func(void *buffer, uint16_t len, void *eb)
{
  eth_frame *frame = buffer;
  if (eth_recv_cb == NULL)
  {
    ESP_LOGW(ETH_TAG, "Ethernet frame received but no callback function is set on received...");
  }
  else if (len < sizeof(eth_frame) - CONFIG_MAX_ETH_DATA_LEN)
  {
    ESP_LOGW(ETH_TAG, "The ethernet frame received is too short : frame length = %dB / minimum length expected (for header) = %dB", len, sizeof(eth_frame) - CONFIG_MAX_ETH_DATA_LEN);
  }
  else if (len > sizeof(eth_frame))
  {
    ESP_LOGW(ETH_TAG, "The ethernet frame received is too long : frame length = %dB / maximum length = %dB", len, sizeof(eth_frame));
  }
  else if (frame->ethertype != ETHERTYPE)
  {
    ESP_LOGW(ETH_TAG, "Unexpected frame ethertype : got %d instead of %d", frame->ethertype, ETHERTYPE);
  }
  else if (frame->data_len > len - sizeof(eth_frame) + CONFIG_MAX_ETH_DATA_LEN)
  {
    ESP_LOGW(ETH_TAG, "Data longer than available frame length : data length = %d / available frame length = %d", frame->data_len, len - sizeof(eth_frame) + CONFIG_MAX_ETH_DATA_LEN);
  }
  else
  {
    eth_recv_cb(frame->src_mac, frame->data, frame->data_len, 'e');
  }
  return ESP_OK;
}

void eth_init_frame(eth_frame *p_frame)
{
  p_frame->ethertype = ETHERTYPE;
  memcpy(p_frame->dst_mac, eth_dst_mac, sizeof(uint8_t) * 6);
  memcpy(p_frame->src_mac, eth_src_mac, sizeof(uint8_t) * 6);
}

esp_err_t eth_send_frame(eth_frame *p_frame)
{
  int err = esp_eth_tx((uint8_t *)p_frame, sizeof(eth_frame) + (p_frame->data_len) - CONFIG_MAX_ETH_DATA_LEN);

  if (err != 0)
  {
    ESP_LOGE(ETH_TAG, "Error occurred while sending eth frame: error code 0x%x", err);
    return ESP_FAIL;
  }

  return ESP_OK;
}

void eth_send_data(uint8_t *data, int len)
{
  eth_frame frame;
  eth_init_frame(&frame);
  frame.data_len = len;
  memcpy(&(frame.data), data, len);
  eth_send_frame(&(frame));
}

void eth_detach_recv_cb()
{
  eth_recv_cb = NULL;
}

void eth_attach_recv_cb(void (*cb)(uint8_t src_mac[6], uint8_t *data, int len, char eth_or_wifi))
{
  eth_recv_cb = cb;
}

void eth_detach_link_state_cb()
{
  eth_link_state_cb = NULL;
}

void eth_attach_link_state_cb(void (*cb)(bool link_state))
{
  eth_link_state_cb = cb;
}

void eth_init()
{
  ////////////////////////////////////
  //EVENT HANDLER (CALLBACK)
  ////////////////////////////////////
  //TCP/IP event handling & group (akin to flags and semaphores)
  udp_event_group = xEventGroupCreate();
  ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL));
  

  ////////////////////////////////////
  //TCP/IP DRIVER INIT WITH A STATIC IP
  ////////////////////////////////////
  tcpip_adapter_init();
  tcpip_adapter_ip_info_t ipInfo;

  //Stop DHCP
  tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_ETH);

  //Set the static IP
  ip4addr_aton(DEVICE_IP, &ipInfo.ip);
  ip4addr_aton(DEVICE_GW, &ipInfo.gw);
  ip4addr_aton(DEVICE_NETMASK, &ipInfo.netmask);
  ESP_ERROR_CHECK(tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_ETH, &ipInfo));

  ////////////////////////////////////
  //ETHERNET CONFIGURATION & INIT
  ////////////////////////////////////

  eth_config_t config = ETHERNET_PHY_CONFIG;
  config.phy_addr = PHY1;
  config.gpio_config = eth_gpio_config_rmii;
  config.tcpip_input = &eth_recv_func;
  config.clock_mode = CONFIG_PHY_CLOCK_MODE;

  ESP_ERROR_CHECK(esp_eth_init(&config));
  ESP_ERROR_CHECK(esp_eth_enable());

  /*ESP_LOGI(ETH_TAG, "Establishing connetion...");
  xEventGroupWaitBits(udp_event_group, WIFI_CONNECTED_BIT, true, true, portMAX_DELAY);
  ESP_LOGI(ETH_TAG, "Connected");*/
}

void eth_deinit()
{
  ESP_ERROR_CHECK(esp_eth_disable());
  ESP_ERROR_CHECK(esp_eth_deinit());
}
