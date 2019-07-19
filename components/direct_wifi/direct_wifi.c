#include "direct_wifi.h"

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

void wifi_recv_func(const uint8_t *mac, const uint8_t *rxData, int len) {

}

void wifi_init() {
/* Init WiFi */
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    cfg.ampdu_tx_enable = 0;
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_start() );
    ESP_ERROR_CHECK( esp_wifi_set_channel(CONFIG_CHANNEL, WIFI_SECOND_CHAN_NONE) );
    ESP_ERROR_CHECK( esp_wifi_internal_set_fix_rate(ESP_IF_WIFI_STA, true, WIFI_PHY_RATE_24M) );


    /* Init ESPNOW */
    ESP_ERROR_CHECK( esp_now_init() );
    ESP_ERROR_CHECK( esp_now_register_send_cb(wifi_send_cb) );
    ESP_ERROR_CHECK( esp_now_register_recv_cb(wifi_recv_func) );
    
    memset(&peer, 0, sizeof(esp_now_peer_info_t));
    peer.channel = CONFIG_CHANNEL;
    peer.ifidx = ESP_IF_WIFI_STA;
    peer.encrypt = false;
    memcpy(peer.peer_addr, s_example_broadcast_mac, ESP_NOW_ETH_ALEN);
    ESP_ERROR_CHECK( esp_now_add_peer(&peer) );
  }