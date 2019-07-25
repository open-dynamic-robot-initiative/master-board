#include "direct_wifi.h"

static const char* WIFI_TAG = "Direct_Wifi";

esp_now_peer_info_t peer;
static uint8_t s_example_broadcast_mac[ESP_NOW_ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};


void wifi_send_data(uint8_t *data, int len) {
    esp_now_send(peer.peer_addr, data, len);
}

static void wifi_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  if(status != ESP_OK) {
    //error_send++;
  }
}

void wifi_attach_recv_cb(void (*cb)(uint8_t src_mac[6], uint8_t *data, int len)) {
  ESP_ERROR_CHECK( esp_now_register_recv_cb(cb) );
}

void wifi_detach_recv_cb() {
  esp_now_unregister_recv_cb();
}

void wifi_init() {
/* Init NVS */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

/* Init WiFi */
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    cfg.ampdu_tx_enable = 0;
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_start() );
    ESP_ERROR_CHECK( esp_wifi_set_channel(CONFIG_WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE) );
    ESP_ERROR_CHECK( esp_wifi_internal_set_fix_rate(ESP_IF_WIFI_STA, true, CONFIG_WIFI_DATARATE) );


  /* Init ESPNOW */
    ESP_ERROR_CHECK( esp_now_init() );
    
    memset(&peer, 0, sizeof(esp_now_peer_info_t));
    peer.channel = CONFIG_WIFI_CHANNEL;
    peer.ifidx = ESP_IF_WIFI_STA;
    peer.encrypt = false;
    memcpy(peer.peer_addr, s_example_broadcast_mac, ESP_NOW_ETH_ALEN);
    ESP_ERROR_CHECK( esp_now_add_peer(&peer) );
  }