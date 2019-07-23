#include <stdio.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

#include "direct_wifi.h"
#include "spi_manager.h"
#include "spi_quad_packet.h"

#include <unistd.h>
#include "esp_timer.h"


#define ESPNOW_SUB_DATA_LEN (sizeof(spi_packet)-sizeof(spi_tx_packet_a[0].CRC))


spi_packet spi_rx_packet[CONFIG_N_SLAVES];

spi_packet spi_tx_packet_a[CONFIG_N_SLAVES];
spi_packet spi_tx_packet_b[CONFIG_N_SLAVES];

uint8_t espnow_tx_data[CONFIG_N_SLAVES * ESPNOW_SUB_DATA_LEN];


bool spi_use_a = true;

void print_packet(uint8_t *data, int len) {
    for(int i=0;i<len;i++) {
        if(i%4 == 0) printf(" ");
        if(i%8 == 0) printf("\n\t\t");
        printf("%02X ", data[i]);
    }
    printf("\n");
}

static void periodic_timer_callback(void* arg)
{
    spi_transaction_t* p_trans[CONFIG_N_SLAVES];
    spi_packet *p_tx = spi_use_a ? spi_tx_packet_a : spi_tx_packet_b;

    //Add to queue all transaction
    for(int i=0;i<CONFIG_N_SLAVES;i++) {
        p_trans[i] = spi_send(i, (uint8_t*) &(p_tx[i]), (uint8_t*) &(spi_rx_packet[i]), sizeof(spi_packet));
    }

    //wait for every transaction to finish
    for(int i=0;i<CONFIG_N_SLAVES;i++) {
        if(p_trans[i] == NULL) {
          printf("Error with SPI transaction number %d !\n", i);  
        } else {
            while(!spi_is_finished(p_trans[i])) {
                //Wait for it to be finished
            }
            memcpy(espnow_tx_data + ESPNOW_SUB_DATA_LEN * i, &(spi_rx_packet[i]), ESPNOW_SUB_DATA_LEN);
            spi_finish(p_trans[i]);
        }
    }

    wifi_send_data(espnow_tx_data, CONFIG_N_SLAVES * ESPNOW_SUB_DATA_LEN);

}

void setup_spi() {
    spi_init();

    const esp_timer_create_args_t periodic_timer_args = {
            .callback = &periodic_timer_callback,
            .name = "spi_send"
    };
    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, 1000));
}

void wifi_receive_cb(uint8_t src_mac[6], uint8_t *data, int len) {
    //TODO: Check CRC
    
    spi_packet *to_fill = spi_use_a ? spi_tx_packet_b : spi_tx_packet_a;
    
    for(int i=0;i<CONFIG_N_SLAVES;i++) {
        memcpy(&(to_fill[i]), data + ESPNOW_SUB_DATA_LEN*i, ESPNOW_SUB_DATA_LEN);
    }

    spi_use_a = !spi_use_a;

    //TODO: be sure that opacket cannot be modified from both side at the same time
}

void setup_wifi() {
    wifi_init();
    wifi_attach_recv_cb(wifi_receive_cb);
}

void app_main()
{
    nvs_flash_init();

    setup_spi();
    //printf("The core is : %d\n",xPortGetCoreID());
    setup_wifi();
    wifi_init();

    
}
