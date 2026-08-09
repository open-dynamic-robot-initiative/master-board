#include "direct_wifi.h"
#include "esp_mac.h"

// global variables
static const char *WIFI_TAG = "Direct_Wifi";
static esp_now_peer_info_t peer = {0};
static uint8_t s_example_broadcast_mac[ESP_NOW_ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// function pointer
void (*wifi_recv_cb)(const esp_now_recv_info_t *esp_now_info, const uint8_t *data, int data_len) = NULL; // eth_or_wifi = 'e' when eth is used, 'w' when wifi is used

// forward declarations

void wifi_send_data(uint8_t *data, int len)
{
  esp_err_t ret = esp_now_send(peer.peer_addr, data, len);
  if (ret != ESP_OK)
  {
    ESP_LOGE(WIFI_TAG, "Failed to send ESP-NOW data: %s", esp_err_to_name(ret));
  }
}

void wifi_recv_func(const esp_now_recv_info_t *esp_now_info, const uint8_t *data, int data_len)
{
  if (wifi_recv_cb == NULL)
  {
    ESP_LOGW(WIFI_TAG, "Wifi frame received but no callback function is set on received...");
  }
  else
  {
    wifi_recv_cb(esp_now_info, data, data_len);
  }
}

void wifi_attach_recv_cb(void (*cb)(const esp_now_recv_info_t *esp_now_info, const uint8_t *data, int data_len))
{
  wifi_recv_cb = cb;
}

void wifi_detach_recv_cb()
{
  wifi_recv_cb = NULL;
}

void wifi_init()
{
  // init nvs
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  // wifi/lwip init phase
  esp_netif_init(); // s1.1
  // ESP_ERROR_CHECK(esp_event_loop_create_default()); // s1.2
  esp_netif_create_default_wifi_sta(); // s1.3

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  cfg.ampdu_tx_enable = 0;
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  // wi-fi configuration phase
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_country_code("JP", false));

  // wi-fi start phase
  ESP_ERROR_CHECK(esp_wifi_start());
  ESP_ERROR_CHECK(esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE));

  // init esp-now
  ESP_ERROR_CHECK(esp_now_init());
  ESP_ERROR_CHECK(esp_now_register_recv_cb(wifi_recv_func));

  // add peer
  memset(&peer, 0, sizeof(esp_now_peer_info_t));
  peer.channel = 1;
  peer.ifidx = WIFI_IF_STA;
  peer.encrypt = false;
  memcpy(peer.peer_addr, s_example_broadcast_mac, ESP_NOW_ETH_ALEN);
  ESP_ERROR_CHECK(esp_now_add_peer(&peer));

  // config esp-now rate
  esp_now_rate_config_t rate_config = {
      .phymode = WIFI_PHY_MODE_11B,
      .rate = CONFIG_WIFI_DATARATE,
      .ersu = false,
      .dcm = false};
  ESP_ERROR_CHECK(esp_now_set_peer_rate_config(peer.peer_addr, &rate_config));
}

void wifi_deinit_func()
{
  ESP_ERROR_CHECK(esp_now_deinit());
  ESP_ERROR_CHECK(esp_wifi_stop());
  ESP_ERROR_CHECK(esp_wifi_deinit());
}

void wifi_change_channel(uint8_t wifi_channel)
{
  ESP_ERROR_CHECK(esp_wifi_set_channel(wifi_channel, WIFI_SECOND_CHAN_NONE));
  peer.channel = wifi_channel;
  ESP_ERROR_CHECK(esp_now_mod_peer(&peer));
}