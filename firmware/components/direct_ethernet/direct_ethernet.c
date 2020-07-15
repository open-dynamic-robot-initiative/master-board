#include "direct_ethernet.h"

uint8_t eth_src_mac[6] = {0};
uint8_t eth_dst_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

void (*eth_recv_cb)(uint8_t src_mac[6], uint8_t *data, int len, char eth_or_wifi) = NULL; // eth_or_wifi = 'e' when eth is used, 'w' when wifi is used

void (*eth_link_state_cb)(bool link_state) = NULL;

static const char *ETH_TAG = "Direct_Ethernet";
static esp_eth_handle_t eth_handle = NULL;

/** Event handler for Ethernet events */
static void eth_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data)
{
  /* we can get the ethernet driver handle from event data */
  eth_handle = *(esp_eth_handle_t *)event_data;

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

    esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, eth_src_mac);

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
  default:
    ESP_LOGI(ETH_TAG, "Unhandled Ethernet event (id = %d)", event_id);
    break;
  }
}

static esp_err_t eth_recv_func(esp_eth_handle_t eth_handle, uint8_t *buffer, uint32_t len)
{
  eth_frame *frame = (eth_frame *)buffer;
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
  free(buffer);
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
  int err = esp_eth_transmit(eth_handle, (uint8_t *)p_frame, sizeof(eth_frame) + (p_frame->data_len) - CONFIG_MAX_ETH_DATA_LEN);

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
  ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL));

  eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
  eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();

  esp_eth_mac_t *mac = esp_eth_mac_new_esp32(&mac_config);
  esp_eth_phy_t *phy = esp_eth_phy_new_lan8720(&phy_config);

  esp_eth_config_t config = ETH_DEFAULT_CONFIG(mac, phy);

  config.stack_input = eth_recv_func;

  ESP_ERROR_CHECK(esp_eth_driver_install(&config, &eth_handle));
  printf("Driver installed\n");
  ESP_ERROR_CHECK(esp_eth_start(eth_handle));
  printf("Driver started\n");
}

void eth_deinit()
{
  ESP_ERROR_CHECK(esp_eth_stop(eth_handle));
  ESP_ERROR_CHECK(esp_eth_driver_uninstall(eth_handle));
}