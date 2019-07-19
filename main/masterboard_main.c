#include <stdio.h>
#include <stdint.h>

#include "direct_wifi.h"

uint8_t dummy_data[10] = { 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA};

void my_wifi_recv_cb(uint8_t src_mac[6], uint8_t *data, int len) {
    printf("Wifi frame received : \n");
    printf("\tsrc mac : %02X:%02X:%02X:%02X:%02X:%02X\n", src_mac[0], src_mac[1], src_mac[2], src_mac[3], src_mac[4], src_mac[5]);
    printf("\tdata :");
    for(int i=0;i<len;i++) {
        if(i%4 == 0) printf(" ");
        if(i%8 == 0) printf("\n\t\t");
        printf("%02X ", data[i]);
    }
    printf("\n");
}

void app_main()
{
    wifi_init();
    wifi_attach_recv_cb(&my_wifi_recv_cb);

    while(true) {
        printf("Sending frame...\n");
        wifi_send_data(dummy_data, 10);
        printf("Sent\n");

        vTaskDelay(2000 / portTICK_RATE_MS);
    }

}
