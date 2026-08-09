#include "direct_ethernet.h"

#include "lwip/inet.h"
#include "lwip/ip4_addr.h"

// global variables
uint8_t eth_src_mac[6] = {0};
uint8_t eth_dst_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

EventGroupHandle_t udp_event_group = NULL;
static esp_eth_handle_t eth_handle = NULL;
static esp_netif_t *eth_netif = NULL;
static esp_eth_netif_glue_handle_t eth_glue = NULL;
static const char *ETH_TAG = "Direct_Ethernet";

// function pointers
void (*eth_recv_cb)(uint8_t src_mac[6], uint8_t *data, int len, char eth_or_wifi) = NULL; // eth_or_wifi = 'e' when eth is used, 'w' when wifi is used
void (*eth_link_state_cb)(bool link_state) = NULL;

// forward declarations
static void eth_event_handler(
    void *arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data)
{
  esp_eth_handle_t local_eth_handle = *(esp_eth_handle_t *)event_data;

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

    ESP_ERROR_CHECK(esp_eth_ioctl(local_eth_handle, ETH_CMD_G_MAC_ADDR, eth_src_mac));
    ESP_LOGI(ETH_TAG, "Ethernet Link Up");
    ESP_LOGI(ETH_TAG, "Ethernet HW Addr %02x:%02x:%02x:%02x:%02x:%02x",
             eth_src_mac[0], eth_src_mac[1], eth_src_mac[2],
             eth_src_mac[3], eth_src_mac[4], eth_src_mac[5]);
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

static void got_ip_event_handler(void *arg, esp_event_base_t event_base,
                                 int32_t event_id, void *event_data)
{
  ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
  const esp_netif_ip_info_t *ip_info = &event->ip_info;

  ESP_LOGI(ETH_TAG, "Ethernet got IP");
  ESP_LOGI(ETH_TAG, "~~~~~~~~~~~");
  ESP_LOGI(ETH_TAG, "ETHIP:" IPSTR, IP2STR(&ip_info->ip));
  ESP_LOGI(ETH_TAG, "ETHMASK:" IPSTR, IP2STR(&ip_info->netmask));
  ESP_LOGI(ETH_TAG, "ETHGW:" IPSTR, IP2STR(&ip_info->gw));
  ESP_LOGI(ETH_TAG, "~~~~~~~~~~~");

  xEventGroupSetBits(udp_event_group, WIFI_CONNECTED_BIT);
}

static esp_err_t eth_recv_func(esp_eth_handle_t hdl, uint8_t *buffer, uint32_t len, void *priv)
{
  eth_frame *frame = (eth_frame *)buffer;

  if (len >= sizeof(eth_frame) - CONFIG_MAX_ETH_DATA_LEN &&
      len <= sizeof(eth_frame) &&
      frame->ethertype == ETHERTYPE &&
      frame->data_len <= len - sizeof(eth_frame) + CONFIG_MAX_ETH_DATA_LEN)
  {
    if (eth_recv_cb != NULL)
    {
      eth_recv_cb(frame->src_mac, frame->data, frame->data_len, 'e');
    }
    else
    {
      ESP_LOGW(ETH_TAG, "Ethernet frame received but no callback function is set on received...");
    }
    free(buffer);
    return ESP_OK;
  }
  else
  {
    free(buffer);
    ESP_LOGW(ETH_TAG, "Received frame does not meet the expected format or length requirements.");
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
  esp_err_t ret;

  if (eth_handle == NULL)
  {
    ESP_LOGE(ETH_TAG, "Ethernet not initialized");
    return ESP_FAIL;
  }

  ret = esp_eth_transmit(eth_handle, (void *)p_frame, sizeof(eth_frame) + (p_frame->data_len) - CONFIG_MAX_ETH_DATA_LEN);
  if (ret != ESP_OK)
  {
    ESP_LOGE(ETH_TAG, "Error occurred while sending eth frame: error code 0x%x", ret);
    return ESP_FAIL;
  }

  return ESP_OK;
}

void eth_send_data(uint8_t *data, int len)
{
  eth_frame frame;
  eth_init_frame(&frame);
  frame.data_len = len;
  memcpy(frame.data, data, len);
  (void)eth_send_frame(&(frame));
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
  udp_event_group = xEventGroupCreate();

  // config gpio for 50Hz clock input from phy
  gpio_config_t io_conf = {
      .pin_bit_mask = 1ULL << PIN_CLK_50HZ,
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = GPIO_PULLUP_DISABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE};
  ESP_ERROR_CHECK(gpio_config(&io_conf));
  ESP_ERROR_CHECK(gpio_set_level(PIN_CLK_50HZ, 1));
  vTaskDelay(pdMS_TO_TICKS(10));

  // configure emac
  eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
  eth_esp32_emac_config_t esp32_emac_config = ETH_ESP32_EMAC_DEFAULT_CONFIG();
  esp32_emac_config.smi_gpio.mdc_num = PIN_SMI_MDC;
  esp32_emac_config.smi_gpio.mdio_num = PIN_SMI_MDIO;
  esp32_emac_config.clock_config.rmii.clock_mode = EMAC_CLK_OUT;
  esp32_emac_config.clock_config.rmii.clock_gpio = PIN_CLK_50HZ;
  esp_eth_mac_t *mac = esp_eth_mac_new_esp32(&esp32_emac_config, &mac_config);

  // configure phy
  eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
  phy_config.phy_addr = -1;
  phy_config.reset_gpio_num = -1;
  esp_eth_phy_t *phy = esp_eth_phy_new_lan87xx(&phy_config);

  // install driver
  esp_eth_config_t config = ETH_DEFAULT_CONFIG(mac, phy);
  esp_eth_driver_install(&config, &eth_handle);

  esp_err_t err = esp_event_loop_create_default();
  if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
  {
    ESP_ERROR_CHECK(err);
  }
  ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL));

  // connect driver to TCP/IP stack
  esp_netif_init();
  esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();
  eth_netif = esp_netif_new(&cfg);

  ESP_ERROR_CHECK(esp_netif_dhcpc_stop(eth_netif));
  esp_netif_ip_info_t ip_info;
  esp_ip4_addr_t ip, gw, netmask;
  ESP_ERROR_CHECK(esp_netif_str_to_ip4(DEVICE_IP, &ip));
  ESP_ERROR_CHECK(esp_netif_str_to_ip4(DEVICE_GW, &gw));
  ESP_ERROR_CHECK(esp_netif_str_to_ip4(DEVICE_NETMASK, &netmask));
  IP4_ADDR(&ip_info.ip, ip4_addr1(&ip), ip4_addr2(&ip), ip4_addr3(&ip), ip4_addr4(&ip));
  IP4_ADDR(&ip_info.gw, ip4_addr1(&gw), ip4_addr2(&gw), ip4_addr3(&gw), ip4_addr4(&gw));
  IP4_ADDR(&ip_info.netmask, ip4_addr1(&netmask), ip4_addr2(&netmask), ip4_addr3(&netmask), ip4_addr4(&netmask));
  ESP_ERROR_CHECK(esp_netif_set_ip_info(eth_netif, &ip_info));

  eth_glue = esp_eth_new_netif_glue(eth_handle);
  ESP_ERROR_CHECK(esp_netif_attach(eth_netif, eth_glue));
  esp_eth_update_input_path(eth_handle, eth_recv_func, eth_netif);
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL));

  // start ethernet driver
  esp_eth_start(eth_handle);
}

void eth_deinit()
{
  if (eth_handle != NULL)
  {
    ESP_ERROR_CHECK(esp_eth_stop(eth_handle));
    ESP_ERROR_CHECK(esp_eth_driver_uninstall(eth_handle));
    eth_handle = NULL;
  }

  if (eth_glue != NULL)
  {
    esp_eth_del_netif_glue(eth_glue);
    eth_glue = NULL;
  }

  if (eth_netif != NULL)
  {
    esp_netif_destroy(eth_netif);
    eth_netif = NULL;
  }

  if (udp_event_group != NULL)
  {
    vEventGroupDelete(udp_event_group);
    udp_event_group = NULL;
  }
}