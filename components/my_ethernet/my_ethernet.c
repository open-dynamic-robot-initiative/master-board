#include "my_ethernet.h"

void eth_init_frame(eth_frame *p_frame) {
  p_frame->ethertype = 0xb588;
  memcpy(p_frame->dst_mac, eth_dst_mac, sizeof(uint8_t) * 6);  
  memcpy(p_frame->src_mac, eth_src_mac, sizeof(uint8_t) * 6);
}

esp_err_t send_eth_frame(eth_frame *p_frame)
{
  int err = esp_eth_tx((uint8_t *) p_frame, sizeof(eth_frame) + (p_frame->data_len) - CONFIG_MAX_ETH_DATA_LEN);
  
  if (err < 0) {
    ESP_LOGE(TAG, "Erromar occurred while sending eth frame: errno %d", errno);
    return ESP_FAIL;
  }

  return ESP_OK;
}

static void eth_gpio_config_rmii(void) {
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

static esp_err_t eth_event_handler(void *ctx, system_event_t *event) {
  tcpip_adapter_ip_info_t ipInfo;

  switch(event->event_id) {
    case ETHERNET_EVENT_CONNECTED:
        ESP_LOGI(TAG, "Ethernet Link Up");
        break;
    case ETHERNET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "Ethernet Link Down");
        break;
    case ETHERNET_EVENT_START:
        ESP_LOGI(TAG, "Ethernet Started");
        break;
    case ETHERNET_EVENT_STOP:
        ESP_LOGI(TAG, "Ethernet Stopped");
        break;
    case SYSTEM_EVENT_ETH_GOT_IP:
        ESP_LOGI(TAG, "Ethernet got IP");
        ESP_ERROR_CHECK(tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_ETH, &ipInfo));
        ESP_LOGI(TAG, "TCP/IP initialization finished.");
        ESP_LOGI(TAG, "TCP|IP \t IP:"IPSTR, IP2STR(&ipInfo.ip));
        ESP_LOGI(TAG, "TCP|IP \t MASK:"IPSTR, IP2STR(&ipInfo.netmask));
        ESP_LOGI(TAG, "TCP|IP \t GW:"IPSTR, IP2STR(&ipInfo.gw));
        xEventGroupSetBits(udp_event_group, WIFI_CONNECTED_BIT);
    default:
      ESP_LOGI(TAG, "Unhandled Ethernet event (id = %d)", event->event_id);
        break;
    }
  return ESP_OK;
}

static esp_err_t *my_recv_func (void *buffer, uint16_t len, void *eb) {

  return ESP_OK;
}

void eth_init() {
  ////////////////////////////////////
  //EVENT HANDLER (CALLBACK)
  ////////////////////////////////////
    //TCP/IP event handling & group (akin to flags and semaphores)
    udp_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_init(eth_event_handler, NULL) );

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
    config.tcpip_input = my_recv_func;
    config.clock_mode = CONFIG_PHY_CLOCK_MODE;

    ESP_ERROR_CHECK(esp_eth_init(&config));
    ESP_ERROR_CHECK(esp_eth_enable());

    ESP_LOGI(TAG, "Establishing connetion...");
    xEventGroupWaitBits(udp_event_group, WIFI_CONNECTED_BIT,true, true, portMAX_DELAY);
    ESP_LOGI(TAG, "Connected");
}
