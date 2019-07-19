#include <stdio.h>
#include <stdint.h>

#include "direct_ethernet.h"

uint8_t dummy_data[10] = { 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA};

void my_eth_recv_cb(uint8_t src_mac[6], uint8_t *data, int len) {
    printf("Eth frame received : \n");
    printf("\tsrc mac : %02X:%02X:%02X:%02X:%02X:%02X\n", src_mac[0], src_mac[1], src_mac[2], src_mac[3], src_mac[4], src_mac[5]);
    printf("\tdata :");
    for(int i=0;i<len;i++) {
        if(i%4 == 0) printf(" ");
        if(i%8 == 0) printf("\n\t\t");
        printf("%02X ", data[i]);
    }
    printf("\n");
}

eth_frame my_frame;

void app_main()
{
    eth_init();
    eth_attach_recv_cb(&my_eth_recv_cb);
    eth_init_frame(&my_frame);

    while(true) {
        printf("Sending frame...\n");
        memcpy(my_frame.data, dummy_data, 10);
        my_frame.data_len = 10;
        eth_send_frame(&my_frame);
        printf("Sent\n");

        vTaskDelay(2000 / portTICK_RATE_MS);
    }

}
