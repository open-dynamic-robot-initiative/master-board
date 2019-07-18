#include <stdio.h>
#include <stdint.h>

#include "my_ethernet.h"

void my_eth_recv_cb(uint8_t src_mac[6], uint8_t *data, int len) {
    printf("Eth frame received : \n");
    printf("\tsrc mac : %02X:%02X:%02X:%02X:%02X:%02X\n", src_mac[0], src_mac[1], src_mac[2], src_mac[3], src_mac[4], src_mac[5]);
    printf("\tdata :");
    for(int i=0;i<len;i++) {
        if(i%4) printf(" ");
        if(i%8) printf("\n");
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
        eth_send_frame(&my_frame);
        printf("Sent\n");

        vTaskDelay(2000 / portTICK_RATE_MS);
    }

}
