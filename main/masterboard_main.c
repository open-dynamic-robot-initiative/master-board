#include <stdio.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

#include "spi_manager.h"
#include "spi_quad_packet.h"

spi_packet rx_packet, tx_packet;

void print_packet(uint8_t *data, int len) {
    for(int i=0;i<len;i++) {
        if(i%4 == 0) printf(" ");
        if(i%8 == 0) printf("\n\t\t");
        printf("%02X ", data[i]);
    }
    printf("\n");
}

void app_main()
{
    spi_init();

    while(true) {
        spi_transaction_t *trans = spi_send(1, (uint8_t*) &tx_packet, (uint8_t*) &rx_packet, sizeof(spi_packet));
        if(trans == NULL) printf("error !\n");
        printf("Send :");
        print_packet((uint8_t*) &tx_packet, sizeof(spi_packet));
        do {
            printf("%s", spi_is_finished(trans) ? "true" : "false");
            fflush(stdout);
        } while(!spi_is_finished(trans));
        printf("Received :");
        print_packet((uint8_t*) &rx_packet, sizeof(spi_packet));
        printf("\n");
        spi_finish(trans);
         vTaskDelay(100 / portTICK_RATE_MS);
    }

}
