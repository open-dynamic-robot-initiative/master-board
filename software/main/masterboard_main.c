#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

#include "driver/gpio.h"

#include "esp_timer.h"

#include "direct_wifi.h"
#include "direct_ethernet.h"

#include "spi_manager.h"
#include "spi_quad_packet.h"
#include "quad_crc.h"
#include "uart_imu.h"

#include "defines.h"

#define useWIFI false

long int spi_count = 0;
long int spi_ok[CONFIG_N_SLAVES] = {0};

int spi_wdt = 0;
int spi_stop = false;
int wifi_eth_first_recv = false;
int wifi_eth_start_spi = false;

static uint16_t spi_index_trans = 0;

static uint16_t spi_rx_packet[CONFIG_N_SLAVES][SPI_TOTAL_LEN + 1]; //SPI write DMA by blocks of 32bits. +1 prevents any overflow

static uint16_t spi_tx_packet_a[CONFIG_N_SLAVES][SPI_TOTAL_LEN];
static uint16_t spi_tx_packet_b[CONFIG_N_SLAVES][SPI_TOTAL_LEN];
static uint16_t spi_tx_packet_stop[CONFIG_N_SLAVES][SPI_TOTAL_LEN];

uint16_t command_index_prev = 0;

struct wifi_eth_packet_sensor wifi_eth_tx_data;

bool spi_use_a = true;

void print_packet(uint8_t *data, int len)
{
    for (int i = 0; i < len; i++)
    {
        if (i % 4 == 0)
            printf(" ");
        if (i % 8 == 0)
            printf("\n\t\t");
        printf("%02X ", data[i]);
    }
    printf("\n");
}

static void periodic_timer_callback(void *arg)
{
    /* Securities */
    if (!wifi_eth_start_spi)
        return;
    spi_wdt++;
    spi_count++;

    if (!gpio_get_level(CONFIG_BUTTON_GPIO))
    {
        spi_stop = false;
        wifi_eth_first_recv = false;
    }
    gpio_set_level(CONFIG_LED_GPIO, spi_stop ? 0 : 1);

    /* Debug */
    if (spi_count % 1000 == 0)
    {
        printf("\e[1;1H\e[2J");
        printf("--- SPI ---\n");
        for (int i = 0; i < CONFIG_N_SLAVES; i++)
        {
            printf("[%d] sent : %ld, ok : %ld, ratio : %.02f\n", i, spi_count, spi_ok[i], 100. * spi_ok[i] / spi_count);
        }
    }

    /* Prepare spi transactions */
    spi_transaction_t *p_trans[CONFIG_N_SLAVES];
    spi_index_trans++;

    /* Choose spi packets to send*/
    uint16_t(*p_tx)[SPI_TOTAL_LEN];

    if (spi_wdt < CONFIG_SPI_WDT && !spi_stop)
    {
        p_tx = spi_use_a ? spi_tx_packet_a : spi_tx_packet_b;
    }
    else
    {
        spi_stop = wifi_eth_first_recv; //Stop if we already received the first packet
        p_tx = spi_tx_packet_stop;
    }

    /* Complete and send each packet */

    //for debug:
    if (spi_count % 1000 == 0)
    {
        //print_imu();
        printf("\nlast CMD packet:\n");
        print_packet(p_tx[0], SPI_TOTAL_LEN * 2);
    }
    for (int i = 0; i < CONFIG_N_SLAVES; i++)
    {
        SPI_REG_u16(p_tx[i], SPI_TOTAL_INDEX) = SPI_SWAP_DATA_TX(spi_index_trans, 16);
        SPI_REG_u32(p_tx[i], SPI_TOTAL_CRC) = SPI_SWAP_DATA_TX(packet_compute_CRC(p_tx[i]), 32);
        p_trans[i] = spi_send(i, (uint8_t *)p_tx[i], (uint8_t *)spi_rx_packet[i], SPI_TOTAL_LEN * 2);
    }
    /* Get IMU latest data*/
    parse_IMU_data();
    wifi_eth_tx_data.imu.accelerometer[0] = get_acc_x_in_D16QN();
    wifi_eth_tx_data.imu.accelerometer[1] = get_acc_y_in_D16QN();
    wifi_eth_tx_data.imu.accelerometer[2] = get_acc_z_in_D16QN();

    wifi_eth_tx_data.imu.gyroscope[0] = get_gyr_x_in_D16QN();
    wifi_eth_tx_data.imu.gyroscope[1] = get_gyr_y_in_D16QN();
    wifi_eth_tx_data.imu.gyroscope[2] = get_gyr_z_in_D16QN();

    wifi_eth_tx_data.imu.attitude[0] = get_roll_in_D16QN();
    wifi_eth_tx_data.imu.attitude[1] = get_pitch_in_D16QN();
    wifi_eth_tx_data.imu.attitude[2] = get_yaw_in_D16QN();

    /* Wait for SPI transactions to finish */
    for (int spi_try = 0; spi_try < CONFIG_SPI_N_ATTEMPT; spi_try++)
    {
        for (int i = 0; i < CONFIG_N_SLAVES; i++)
        {
            if (p_trans[i] == NULL)
            {
                //Either the transaction failed or there it was not re-sent
            }
            else
            {
                while (!spi_is_finished(&(p_trans[i])))
                {
                    //Wait for it to be finished
                }

                if (packet_check_CRC(spi_rx_packet[i]))
                {
                    spi_ok[i]++;
                    //for debug:
                    if (spi_count % 1000 == 0 && i == 0)
                    {
                        printf("\nlast SENSOR packet:\n");
                        print_packet(spi_rx_packet[i], SPI_TOTAL_LEN * 2);
                    }
                    wifi_eth_tx_data.sensor[i].status = SPI_SWAP_DATA_RX(SPI_REG_u16(spi_rx_packet[i], SPI_SENSOR_STATUS), 16);
                    wifi_eth_tx_data.sensor[i].timestamp = SPI_SWAP_DATA_RX(SPI_REG_u16(spi_rx_packet[i], SPI_SENSOR_TIMESTAMP), 16);
                    wifi_eth_tx_data.sensor[i].position[0] = SPI_SWAP_DATA_RX(SPI_REG_32(spi_rx_packet[i], SPI_SENSOR_POS_1), 32);
                    wifi_eth_tx_data.sensor[i].position[1] = SPI_SWAP_DATA_RX(SPI_REG_32(spi_rx_packet[i], SPI_SENSOR_POS_2), 32);
                    wifi_eth_tx_data.sensor[i].velocity[0] = SPI_SWAP_DATA_RX(SPI_REG_16(spi_rx_packet[i], SPI_SENSOR_VEL_1), 16);
                    wifi_eth_tx_data.sensor[i].velocity[1] = SPI_SWAP_DATA_RX(SPI_REG_16(spi_rx_packet[i], SPI_SENSOR_VEL_2), 16);
                    wifi_eth_tx_data.sensor[i].current[0] = SPI_SWAP_DATA_RX(SPI_REG_16(spi_rx_packet[i], SPI_SENSOR_IQ_1), 16);
                    wifi_eth_tx_data.sensor[i].current[1] = SPI_SWAP_DATA_RX(SPI_REG_16(spi_rx_packet[i], SPI_SENSOR_IQ_2), 16);
                    wifi_eth_tx_data.sensor[i].coil_resistance[0] = SPI_SWAP_DATA_RX(SPI_REG_u16(spi_rx_packet[i], SPI_SENSOR_CR_1), 16);
                    wifi_eth_tx_data.sensor[i].coil_resistance[1] = SPI_SWAP_DATA_RX(SPI_REG_u16(spi_rx_packet[i], SPI_SENSOR_CR_2), 16);
                    wifi_eth_tx_data.sensor[i].adc[0] = SPI_SWAP_DATA_RX(SPI_REG_u16(spi_rx_packet[i], SPI_SENSOR_ADC_1), 16);
                    wifi_eth_tx_data.sensor[i].adc[1] = SPI_SWAP_DATA_RX(SPI_REG_u16(spi_rx_packet[i], SPI_SENSOR_ADC_2), 16);
                }
                else
                {
                    //transaction failed, try to re-send
                    if (spi_try + 1 < CONFIG_SPI_N_ATTEMPT)
                        p_trans[i] = spi_send(i, (uint8_t *)p_tx[i], (uint8_t *)spi_rx_packet[i], SPI_TOTAL_LEN * 2);

                    memset(&(wifi_eth_tx_data.sensor[i]), 0, sizeof(struct sensor_data));
                }
            }
        }
    }

    /* Send all spi_sensor packets to PC */
    wifi_eth_tx_data.sensor_index++;
    if (useWIFI)
    {
        wifi_send_data(&wifi_eth_tx_data, sizeof(struct wifi_eth_packet_sensor));
    }
    else
    {
        eth_send_data(&wifi_eth_tx_data, sizeof(struct wifi_eth_packet_sensor));
    }
}

void setup_spi()
{
    spi_init();

    for (int i = 0; i < CONFIG_N_SLAVES; i++)
    {
        memset(spi_tx_packet_stop[i], 0, SPI_TOTAL_INDEX * 2);
    }
    wifi_eth_tx_data.sensor_index = 0;
    wifi_eth_tx_data.packet_loss = 0;

    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &periodic_timer_callback,
        .name = "spi_send"};
    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, 1000));

    gpio_set_direction(CONFIG_LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(CONFIG_BUTTON_GPIO, GPIO_MODE_INPUT);
}

void wifi_eth_receive_cb(uint8_t src_mac[6], uint8_t *data, int len)
{
    struct wifi_eth_packet_command *packet_recv = (struct wifi_eth_packet_command *)data;

    /*   */
    if (!wifi_eth_first_recv)
    {
        wifi_eth_first_recv = true;
        wifi_eth_start_spi = true;
        command_index_prev = packet_recv->command_index - 1;
    }

    /* Prepare SPI packets */
    uint16_t(*to_fill)[SPI_TOTAL_LEN] = spi_use_a ? spi_tx_packet_b : spi_tx_packet_a;

    for (int i = 0; i < CONFIG_N_SLAVES; i++)
    {
        SPI_REG_u16(to_fill[i], SPI_COMMAND_MODE) = SPI_SWAP_DATA_TX(packet_recv->command[i].mode, 16);
        SPI_REG_32(to_fill[i], SPI_COMMAND_POS_1) = SPI_SWAP_DATA_TX(packet_recv->command[i].position[0], 32);
        SPI_REG_32(to_fill[i], SPI_COMMAND_POS_2) = SPI_SWAP_DATA_TX(packet_recv->command[i].position[1], 32);
        SPI_REG_16(to_fill[i], SPI_COMMAND_VEL_1) = SPI_SWAP_DATA_TX(packet_recv->command[i].velocity[0], 16);
        SPI_REG_16(to_fill[i], SPI_COMMAND_VEL_2) = SPI_SWAP_DATA_TX(packet_recv->command[i].velocity[1], 16);
        SPI_REG_16(to_fill[i], SPI_COMMAND_IQ_1) = SPI_SWAP_DATA_TX(packet_recv->command[i].current[0], 16);
        SPI_REG_16(to_fill[i], SPI_COMMAND_IQ_2) = SPI_SWAP_DATA_TX(packet_recv->command[i].current[1], 16);
        SPI_REG_u16(to_fill[i], SPI_COMMAND_KP_1) = SPI_SWAP_DATA_TX(packet_recv->command[i].kp[0], 16);
        SPI_REG_u16(to_fill[i], SPI_COMMAND_KP_2) = SPI_SWAP_DATA_TX(packet_recv->command[i].kp[1], 16);
        SPI_REG_u16(to_fill[i], SPI_COMMAND_KD_1) = SPI_SWAP_DATA_TX(packet_recv->command[i].kd[0], 16);
        SPI_REG_u16(to_fill[i], SPI_COMMAND_KD_2) = SPI_SWAP_DATA_TX(packet_recv->command[i].kd[1], 16);
        SPI_REG_u16(to_fill[i], SPI_COMMAND_ISAT_12) = SPI_SWAP_DATA_TX(packet_recv->command[i].isat, 16);
    }
    spi_use_a = !spi_use_a;

    /* Compute data for next wifi_eth_sensor packet */
    wifi_eth_tx_data.packet_loss += ((struct wifi_eth_packet_command *)data)->command_index - command_index_prev - 1;
    command_index_prev = ((struct wifi_eth_packet_command *)data)->command_index;

    /* Reset watchdog timer */
    spi_wdt = 0;
}

void app_main()
{
    uart_set_baudrate(UART_NUM_0, 2000000);
    nvs_flash_init();

    //printf("The core is : %d\n",xPortGetCoreID());

    printf("ETH/WIFI command size %u\n", sizeof(struct wifi_eth_packet_command));
    printf("ETH/WIFI sensor size %u\n", sizeof(struct wifi_eth_packet_sensor));

    printf("SPI size %u\n", SPI_TOTAL_LEN * 2);
    setup_spi();

    printf("initialise IMU\n");
    imu_init();
    printf("done?\n");

    if (useWIFI)
    {
        wifi_init();
        wifi_attach_recv_cb(wifi_eth_receive_cb);
    }
    else
    {
        eth_attach_recv_cb(wifi_eth_receive_cb);
        eth_init();
    }
}
