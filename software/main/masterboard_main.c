#include <stdio.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

#include "direct_wifi.h"
#include "direct_ethernet.h"

#include "spi_manager.h"
#include "spi_quad_packet.h"
#include "quad_crc.h"

#include <unistd.h>
#include "esp_timer.h"


#define useWIFI false

long int nb_recv = 0;
long int nb_ok = 0;

static uint16_t spi_index_trans = 0;

static uint16_t curr_index = 0;

//TODO : handle this 32bits padding properly
typedef struct {
    uint16_t packet[SPI_TOTAL_LEN];
    uint16_t padding;
} __attribute__ ((packed)) padded_spi_packet;

static uint16_t spi_rx_packet[CONFIG_N_SLAVES][SPI_TOTAL_LEN+1]; //SPI write DMA by blocks of 32bits. +1 prevents any overflow
static struct sensor_data spi_rx_data[CONFIG_N_SLAVES];

static uint16_t spi_tx_packet_a[CONFIG_N_SLAVES][SPI_TOTAL_LEN];
static uint16_t spi_tx_packet_b[CONFIG_N_SLAVES][SPI_TOTAL_LEN];

struct wifi_eth_packet_command {
    uint16_t command[CONFIG_N_SLAVES][SPI_TOTAL_INDEX];
    uint16_t sensor_index;
} __attribute__ ((packed));

struct wifi_eth_packet_sensor {
    struct sensor_data sensor[CONFIG_N_SLAVES];
    uint8_t IMU[18]; //TODO create the appropriate struct
    uint16_t sensor_index;
    uint16_t last_index
} __attribute__ ((packed));


struct wifi_eth_packet_sensor wifi_eth_tx_data;


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
    uint16_t (*p_tx)[SPI_TOTAL_LEN] = spi_use_a ? spi_tx_packet_a : spi_tx_packet_b;

    //spi_index_trans++;

    //Add to queue all transaction
    for(int i=0;i<CONFIG_N_SLAVES;i++) {
        //p_tx[i].index = spi_index_trans;
        //packet_set_CRC(p_tx[i]);
        p_trans[i] = spi_send(i, (uint8_t*) p_tx[i], (uint8_t*) spi_rx_packet[i], SPI_TOTAL_LEN*2);
    }

    //wait for every transaction to finish
    for(int i=0;i<CONFIG_N_SLAVES;i++) {
        if(p_trans[i] == NULL) {
          printf("Error with SPI transaction number %d !\n", i);  
        } else {
            while(!spi_is_finished(p_trans[i])) {
                //Wait for it to be finished
            }
            nb_recv++;
            
            /* Flip all the data (Cf endianness) */
            for(int j=0;j<SPI_TOTAL_LEN;j++) {
                spi_rx_packet[i][j] = SPI_SWAP_DATA_RX(spi_rx_packet[i][j], 16);
            }

            if(packet_check_CRC(spi_rx_packet[i])) {
                if(i == 1) nb_ok++;

                for(int j=0;j<SPI_TOTAL_LEN;j++) {
                    spi_rx_packet[i][j] = SPI_SWAP_DATA_RX(spi_rx_packet[i][j], 16);
                }

                spi_rx_data[i].status = SPI_SWAP_DATA_RX(SPI_REG_u16(spi_rx_packet[i], SPI_SENSOR_STATUS), 16);
                spi_rx_data[i].timestamp = SPI_SWAP_DATA_RX(SPI_REG_u16(spi_rx_packet[i], SPI_SENSOR_TIMESTAMP), 16);
                spi_rx_data[i].position[0] = SPI_SWAP_DATA_RX(SPI_REG_32(spi_rx_packet[i], SPI_SENSOR_POS_1), 32);
                spi_rx_data[i].position[1] = SPI_SWAP_DATA_RX(SPI_REG_32(spi_rx_packet[i], SPI_SENSOR_POS_2), 32);
                spi_rx_data[i].velocity[0] = SPI_SWAP_DATA_RX(SPI_REG_16(spi_rx_packet[i], SPI_SENSOR_VEL_1), 16);
                spi_rx_data[i].velocity[1] = SPI_SWAP_DATA_RX(SPI_REG_16(spi_rx_packet[i], SPI_SENSOR_VEL_2), 16);
                spi_rx_data[i].current[0] = SPI_SWAP_DATA_RX(SPI_REG_16(spi_rx_packet[i], SPI_SENSOR_IQ_1), 16);
                spi_rx_data[i].current[1] = SPI_SWAP_DATA_RX(SPI_REG_16(spi_rx_packet[i], SPI_SENSOR_IQ_2), 16);
                spi_rx_data[i].coil_resistance[0] = SPI_SWAP_DATA_RX(SPI_REG_u16(spi_rx_packet[i], SPI_SENSOR_CR_1), 16);
                spi_rx_data[i].coil_resistance[1] = SPI_SWAP_DATA_RX(SPI_REG_u16(spi_rx_packet[i], SPI_SENSOR_CR_2), 16);
                spi_rx_data[i].adc[0] = SPI_SWAP_DATA_RX(SPI_REG_u16(spi_rx_packet[i], SPI_SENSOR_ADC_1), 16);
                spi_rx_data[i].adc[1] = SPI_SWAP_DATA_RX(SPI_REG_u16(spi_rx_packet[i], SPI_SENSOR_ADC_2), 16);

                memcpy(&(wifi_eth_tx_data.sensor[i]), &(spi_rx_data[i]), sizeof(struct sensor_data));
            } else {
                memset(&(wifi_eth_tx_data.sensor[i]), 0, sizeof(struct sensor_data));
            }
            
            spi_finish(p_trans[i]);
        }
    }

    //printf("Nb_ok %ld / recv %ld = %.02f\n", nb_ok, nb_recv, 600. * nb_ok / nb_recv);

    if(useWIFI) {
        wifi_send_data(&wifi_eth_tx_data, sizeof(struct wifi_eth_packet_sensor));
    } else {
        //TODO: Replace this function by eth_send_frame() -> SAVES one memcpy() and memory allocation...
        eth_send_data(&wifi_eth_tx_data, sizeof(struct wifi_eth_packet_sensor));
    }
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

void wifi_eth_receive_cb(uint8_t src_mac[6], uint8_t *data, int len) {
    //TODO: Check CRC ?
    
    uint16_t (*to_fill)[SPI_TOTAL_LEN] = spi_use_a ? spi_tx_packet_b : spi_tx_packet_a;

    for(int i=0;i<CONFIG_N_SLAVES;i++) {
        memcpy(to_fill[i], &(((struct wifi_eth_packet_command*) data)->command[i]), SPI_TOTAL_INDEX*2);

        //TODO: better prepare
        //  1- increment index @ every SPI transaction
        //  2- recalculate CRC
        //  3- SWAP data accordingly
        spi_prepare_packet(to_fill[i], curr_index);
    }

    wifi_eth_tx_data.last_index = ((struct wifi_eth_packet_command*) data)->sensor_index;

    spi_use_a = !spi_use_a;
}

void app_main()
{
    nvs_flash_init();

    //printf("The core is : %d\n",xPortGetCoreID());

    printf("ETH/WIFI command size %u\n", sizeof(struct wifi_eth_packet_command));
    printf("ETH/WIFI sensor size %u\n", sizeof(struct wifi_eth_packet_sensor));
    if(useWIFI) {
        wifi_init();
        wifi_attach_recv_cb(wifi_eth_receive_cb);
    } else {
        eth_attach_recv_cb(wifi_eth_receive_cb);
        eth_init();
    }
    
    printf("SPI size %u\n", SPI_TOTAL_LEN*2);
    setup_spi();
}
