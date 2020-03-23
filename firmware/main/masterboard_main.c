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
#include "ws2812_led_control.h"

#include "defines.h"

#define useWIFI false

#define ENABLE_DEBUG_PRINTF false

#define RGB(r, g, b) ((uint8_t)(g) << 16 | (uint8_t)(r) << 8 | (uint8_t)(b))
long int spi_count = 0;
long int spi_ok[CONFIG_N_SLAVES] = {0};

int wifi_eth_count = 0; // counter that counts the ms without a message being received from PC

uint16_t session_id = 0; // session id
enum State current_state = WAITING_FOR_FIRST_INIT;

unsigned int ms_cpt = 0;

struct led_state ws_led;

static uint16_t spi_index_trans = 0;

static uint16_t spi_rx_packet[CONFIG_N_SLAVES][SPI_TOTAL_LEN + 1]; //SPI write DMA by blocks of 32bits. +1 prevents any overflow

static uint16_t spi_tx_packet_a[CONFIG_N_SLAVES][SPI_TOTAL_LEN];
static uint16_t spi_tx_packet_b[CONFIG_N_SLAVES][SPI_TOTAL_LEN];
static uint16_t spi_tx_packet_stop[CONFIG_N_SLAVES][SPI_TOTAL_LEN];

uint16_t command_index_prev = 0;

struct wifi_eth_packet_sensor wifi_eth_tx_data;

struct wifi_eth_packet_ack wifi_eth_tx_ack;

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
    ms_cpt++;

    //if (ms_cpt % 500 == 0) printf("current_state = %d, session_id = %d\n", current_state, session_id);

    /* LEDs */
    bool blink = (ms_cpt % 1000) > 500;
    float fade_blink = (ms_cpt % 1000 < 500) ? (ms_cpt % 1000) / 500.0 : (1000 - (ms_cpt % 1000)) / 500.0;


    if (!gpio_get_level(CONFIG_BUTTON_GPIO))
        current_state = WAITING_FOR_FIRST_INIT; // TODO: think about button behaviour

    /* Prepare spi transactions */
    spi_transaction_t *p_trans[CONFIG_N_SLAVES];
    spi_index_trans++;


    /* Choose spi packets to send*/
    uint16_t(*p_tx)[SPI_TOTAL_LEN];

    p_tx = spi_tx_packet_stop; // default command is stop


    //printf("%d\n", wifi_eth_count);
    switch (current_state)
    {
    case WAITING_FOR_FIRST_INIT:
        ws_led.leds[0] = RGB(0xff * fade_blink, 0, 0); //Red fade, Waiting for first init
        ws_led.leds[1] = RGB(0xff * fade_blink, 0, 0);

        wifi_eth_count = 0; // we allow not receiving messages if waiting for init
        break;

    case SENDING_INIT_ACK:
        ws_led.leds[0] = RGB(0, 0, 0xff * fade_blink); //Blue fade, waiting for first commmand
        ws_led.leds[1] = RGB(0, 0, 0xff * fade_blink);

        if (wifi_eth_count > CONFIG_WIFI_ETH_TIMEOUT_ACK)
        {
            current_state = WIFI_ETH_ERROR;
        }

        wifi_eth_count++;
        break;

    case ACTIVE_CONTROL:
        ws_led.leds[0] = RGB(0, 0xff * fade_blink, 0); //Green fade, Active control
        ws_led.leds[1] = RGB(0, 0xff * fade_blink, 0);

        if (wifi_eth_count > CONFIG_WIFI_ETH_TIMEOUT_CONTROL)
        {
            current_state = WIFI_ETH_ERROR;
        }
        else
        {
            p_tx = spi_use_a ? spi_tx_packet_a : spi_tx_packet_b;
        }

        wifi_eth_count++;
        break;

    case WIFI_ETH_ERROR:
        ws_led.leds[0] = RGB(0xff * blink, 0, 0); //Red blink, error state (communication with PC), awaiting for new init msg
        ws_led.leds[1] = RGB(0xff * blink, 0, 0);

        wifi_eth_count = 0; // we allow not receiving messages if waiting for init in error state
        break;

    default:
        ws_led.leds[0] = RGB(0xff * blink, 0xff * blink, 0xff * blink); //White blink, state machine error (should never happen)
        ws_led.leds[1] = RGB(0xff * blink, 0xff * blink, 0xff * blink);
        break;
    }

    spi_count++;

    /* Debug */
    if (ENABLE_DEBUG_PRINTF && spi_count % 1000 == 0)
    {
        printf("\e[1;1H\e[2J");
        printf("--- SPI ---\n");
        for (int i = 0; i < CONFIG_N_SLAVES; i++)
        {
            printf("[%d] sent : %ld, ok : %ld, ratio : %.02f\n", i, spi_count, spi_ok[i], 100. * spi_ok[i] / spi_count);
        }
        //print_imu();
        printf("\nlast CMD packet:\n");
        print_packet(p_tx[0], SPI_TOTAL_LEN * 2);
    }

    /* Complete and send each packet */

    // send and receive packets to/from every slave
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

    wifi_eth_tx_data.imu.linear_acceleration[0] = get_linacc_x_in_D16QN();
    wifi_eth_tx_data.imu.linear_acceleration[1] = get_linacc_y_in_D16QN();
    wifi_eth_tx_data.imu.linear_acceleration[2] = get_linacc_z_in_D16QN();

    /* Wait for SPI transactions to finish */
    for (int spi_try = 0; spi_try < CONFIG_SPI_N_ATTEMPT; spi_try++)
    {
        for (int i = 0; i < CONFIG_N_SLAVES; i++)
        {
            if (p_trans[i] == NULL)
            {
                //Either the transaction failed or it was not re-sent
            }
            else
            {
                while (!spi_is_finished(&(p_trans[i])))
                {
                    //Wait for it to be finished
                }

                // is received data correct ?
                if (packet_check_CRC(spi_rx_packet[i]))
                {
                    spi_ok[i]++;

                    //for debug:
                    if (ENABLE_DEBUG_PRINTF && spi_count % 1000 == 0 && i == 0)
                    {
                        printf("\nlast SENSOR packet:\n");
                        print_packet(spi_rx_packet[i], SPI_TOTAL_LEN * 2);
                    }

                    // filling the next sensor msg to PC with data just acquired
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

    // updating session ids
    wifi_eth_tx_ack.session_id = session_id;
    wifi_eth_tx_data.session_id = session_id;

    /* Sends message to PC */
    switch (current_state)
    {
    case WAITING_FOR_FIRST_INIT:
        // nothing to send to PC, else it produces errors and might not connect
        break;

    case SENDING_INIT_ACK:
        /* Send acknowledge packets to PC */
        if (useWIFI)
        {
            wifi_send_data(&wifi_eth_tx_ack, sizeof(struct wifi_eth_packet_ack));
        }
        else
        {
            eth_send_data(&wifi_eth_tx_ack, sizeof(struct wifi_eth_packet_ack));
        }
        break;

    case ACTIVE_CONTROL:
    case WIFI_ETH_ERROR:
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
        break;

    default:
        // we send nothing to PC in case of a state machine error (should never happen)
        break;
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

    gpio_set_direction(CONFIG_BUTTON_GPIO, GPIO_MODE_INPUT);
}

void wifi_eth_receive_cb(uint8_t src_mac[6], uint8_t *data, int len)
{
    if (len == sizeof(struct wifi_eth_packet_init) && (current_state == WAITING_FOR_FIRST_INIT || current_state == WIFI_ETH_ERROR))
    {
        struct wifi_eth_packet_init *packet_recv = (struct wifi_eth_packet_init *)data;

        session_id = packet_recv->session_id;
        current_state = SENDING_INIT_ACK;
        wifi_eth_count = 0;
    }

    else if (len == sizeof(struct wifi_eth_packet_command) && (current_state == SENDING_INIT_ACK || current_state == ACTIVE_CONTROL))
    {
        struct wifi_eth_packet_command *packet_recv = (struct wifi_eth_packet_command *)data;

        if (packet_recv->session_id != session_id)
        {
            //printf("Wrong session id, got %d instead of %d, ignoring packet\n", packet_recv->session_id, session_id);
            return; // ignoring packet
        }

        if (current_state == SENDING_INIT_ACK)
        {
            current_state = ACTIVE_CONTROL;
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
        wifi_eth_count = 0;
    }
}

void app_main()
{
    uart_set_baudrate(UART_NUM_0, 2000000);
    nvs_flash_init();

    ws2812_control_init(); //init the LEDs
    ws_led.leds[0] = 0x0f0f0f;
    ws_led.leds[1] = 0x0f0f0f;
    ws_led.leds[2] = 0x0f0f0f;

    ws2812_write_leds(ws_led);

    //printf("The core is : %d\n",xPortGetCoreID());
    printf("ETH/WIFI command size %u\n", sizeof(struct wifi_eth_packet_init));
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
        eth_init();
        eth_attach_recv_cb(wifi_eth_receive_cb);
    }

    while (1)
    {
        vTaskDelay(1);
        ws2812_write_leds(ws_led);
    }
}
