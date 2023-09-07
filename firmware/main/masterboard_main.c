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

#define ENABLE_DEBUG_PRINTF false

#define SPI_AUTODETECT_MAX_COUNT 50 // number of spi transaction for which the master board will try to detect spi slaves

#define RGB(r, g, b) ((uint8_t)(g) << 16 | (uint8_t)(r) << 8 | (uint8_t)(b))

#define TEST_BIT(field, bit) (((field) & (1 << (bit))) >> (bit))

long int spi_count = 0;

bool spi_autodetect = false;
int spi_n_attempt = CONFIG_SPI_N_ATTEMPT;

uint8_t spi_connected = 0xff; // least significant bit: SPI0
                              // most significant bit: SPI7
                              // initialized at "every slave connected" so that sensor data can be gathered just after boot

long int spi_ok[CONFIG_N_SLAVES] = {0};

int wifi_eth_count = 0; // counter that counts the ms without a message being received from PC

uint16_t session_id = 0; // session id

uint8_t use_wifi = 0; // true if wifi is used, false if ethernet is used

int wifi_channel = 1;

enum State next_state = SETUP;    // this is updated before current_state
enum State current_state = SETUP; // updated by the 1000 Hz cb

unsigned int ms_cpt = 0;

struct led_state ws_led;

static uint16_t spi_index_trans = 0;

static uint16_t spi_rx_packet[CONFIG_N_SLAVES][SPI_TOTAL_LEN + 1]; // +1 prevents any overflow //TODO understand why we need this?
static uint16_t spi_tx_packet[CONFIG_N_SLAVES][SPI_TOTAL_LEN];

uint16_t command_index_prev = 0;

struct wifi_eth_packet_sensor wifi_eth_tx_data;
struct wifi_eth_packet_ack wifi_eth_tx_ack;

struct wifi_eth_packet_command wifi_eth_rx_cmd_reader_buffer;
QueueHandle_t wifi_eth_rx_cmd_mailbox;

bool send_zero_cmd = true; //If true, all cmd packet sent to udrivers ar set to 0.

void print_spi_connected()
{
    printf("spi connected: [ ");
    for (int i = 0; i < CONFIG_N_SLAVES; i++)
    {
        printf("%d ", TEST_BIT(spi_connected, i));
    }
    printf("]\n\n");
}

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

void set_all_leds(uint32_t rgb)
{
    for (int led = 0; led < NUM_LEDS; led++)
    {
        ws_led.leds[led] = rgb;
    }
}

static void periodic_timer_callback(void *arg)
{
    // handling state change
    if (current_state != next_state)
    {
        current_state = next_state;

        switch (current_state)
        {
        case SPI_AUTODETECT:
            //reset spi stats and count for checking connected slaves
            spi_connected = 0;
            memset(spi_ok, 0, CONFIG_N_SLAVES * sizeof(long int));
            spi_count = 0;

            spi_autodetect = true;
            spi_n_attempt = 1; // we only test each slave once while autodetecting
            break;

        case SENDING_INIT_ACK:
            spi_autodetect = false;
            spi_n_attempt = CONFIG_SPI_N_ATTEMPT;

            // updating session ids
            wifi_eth_tx_ack.session_id = session_id;
            wifi_eth_tx_data.session_id = session_id;

            wifi_eth_tx_ack.protocol_version = PROTOCOL_VERSION;

            // updating spi_connected in ack packet
            wifi_eth_tx_ack.spi_connected = spi_connected;

            // reset variables for packet loss feedback
            wifi_eth_tx_data.sensor_index = 0;
            wifi_eth_tx_data.packet_loss = 0;
            break;

        default:
            spi_autodetect = false;
            spi_n_attempt = CONFIG_SPI_N_ATTEMPT;
            break;
        }
    }

    if (current_state == SETUP)
    {
        return;
    }

    ms_cpt++;

    /* LEDs */
    bool blink = (ms_cpt % 1000) > 500;
    float fade_blink = (ms_cpt % 1000 < 500) ? (ms_cpt % 1000) / 500.0 : (1000 - (ms_cpt % 1000)) / 500.0;

    /* Prepare spi transactions */
    bool spi_done[CONFIG_N_SLAVES] = {0}; // used to keep track of which spi transactions are done
    spi_index_trans++;
    send_zero_cmd = true;
    spi_count++;

    switch (current_state)
    {
    case WAITING_FOR_INIT:
        set_all_leds(RGB(0xff * fade_blink, 0, 0)); //Red fade, Waiting for init

        // Change wifi channel to find the one used by the computer
        if ((ms_cpt % 100) == 0 && next_state == current_state)
        {
            wifi_channel = wifi_channel < 14 ? (wifi_channel + 1) : 1;
            wifi_change_channel(wifi_channel);
        }

        wifi_eth_count = 0; // we allow not receiving messages if waiting for init
        break;

    case SPI_AUTODETECT:
        set_all_leds(RGB(0xff * fade_blink, 0, 0xff * fade_blink)); //Magenta fade, SPI Autodetect

        if (spi_count > SPI_AUTODETECT_MAX_COUNT)
        {
            next_state = SENDING_INIT_ACK;
        }

        wifi_eth_count = 0; // we allow not receiving messages while checking for spi slaves
        break;

    case SENDING_INIT_ACK:
        set_all_leds(RGB(0, 0, 0xff * fade_blink)); //Blue fade, waiting for first commmand

        if (wifi_eth_count > CONFIG_WIFI_ETH_TIMEOUT_ACK)
        {
            next_state = WIFI_ETH_ERROR;
        }

        wifi_eth_count++;
        break;

    case ACTIVE_CONTROL:
        set_all_leds(RGB(0, 0xff * fade_blink, 0)); //Green fade, Active control

        if (wifi_eth_count > CONFIG_WIFI_ETH_TIMEOUT_CONTROL)
        {
            next_state = WIFI_ETH_ERROR;
        }
        else
        {
            send_zero_cmd = false;
            // Read command message in a RTOS mailbox for thread safe comunication */
            if (!xQueuePeek(wifi_eth_rx_cmd_mailbox, &wifi_eth_rx_cmd_reader_buffer, 0))
            {
                send_zero_cmd = true; //mailbox is empty -> we never got a command packet
            }
        }

        wifi_eth_count++;
        break;

    case WIFI_ETH_LINK_DOWN:
        set_all_leds(RGB(0x3f * blink, 0x3f * blink, 0)); //Yellow blink, ethernet link down state awaiting for link up

        wifi_eth_count = 0; // we can't receive any messages if link is down
        break;

    case WIFI_ETH_ERROR:
        set_all_leds(RGB(0xff * blink, 0, 0)); //Red blink, error state (communication with PC), awaiting for new init msg

        wifi_eth_count = 0; // we allow not receiving messages if waiting for init in error state
        break;

    default:
        set_all_leds(RGB(0xff * blink, 0xff * blink, 0xff * blink)); //White blink, state machine error (should never happen)
        break;
    }

    /* Debug */
    if (ENABLE_DEBUG_PRINTF && spi_count % 1000 == 0)
    {
        printf("\e[1;1H\e[2J");
        printf("current_state: %d, session_id: %d\n", current_state, session_id);

        printf("\n--- SPI ---\n");

        print_spi_connected();

        for (int i = 0; i < CONFIG_N_SLAVES; i++)
        {
            if (!TEST_BIT(spi_connected, i) && !spi_autodetect)
                continue; // ignoring this slave if it is not connected

            printf("[%d] sent : %ld, ok : %ld, ratio : %.02f\n", i, spi_count, spi_ok[i], 100. * spi_ok[i] / spi_count);
        }
        //print_imu();
        printf("\nlast CMD packet:\n");
        print_packet(spi_tx_packet[2], SPI_TOTAL_LEN * 2);
    }


    /* Prepare the outgoing spi TX packet */
    for (int i = 0; i < CONFIG_N_SLAVES; i++)
    {
        //printf("%d\n",send_zero_cmd);
        if (!TEST_BIT(spi_connected, i) && !spi_autodetect)
            continue; // ignoring this slave if it is not connected

        if (!send_zero_cmd)
        {
            SPI_REG_u16(spi_tx_packet[i], SPI_COMMAND_MODE) = SPI_SWAP_DATA_TX(wifi_eth_rx_cmd_reader_buffer.command[i].mode, 16);
            SPI_REG_32(spi_tx_packet[i], SPI_COMMAND_POS_1) = SPI_SWAP_DATA_TX(wifi_eth_rx_cmd_reader_buffer.command[i].position[0], 32);
            SPI_REG_32(spi_tx_packet[i], SPI_COMMAND_POS_2) = SPI_SWAP_DATA_TX(wifi_eth_rx_cmd_reader_buffer.command[i].position[1], 32);
            SPI_REG_16(spi_tx_packet[i], SPI_COMMAND_VEL_1) = SPI_SWAP_DATA_TX(wifi_eth_rx_cmd_reader_buffer.command[i].velocity[0], 16);
            SPI_REG_16(spi_tx_packet[i], SPI_COMMAND_VEL_2) = SPI_SWAP_DATA_TX(wifi_eth_rx_cmd_reader_buffer.command[i].velocity[1], 16);
            SPI_REG_16(spi_tx_packet[i], SPI_COMMAND_IQ_1) = SPI_SWAP_DATA_TX(wifi_eth_rx_cmd_reader_buffer.command[i].current[0], 16);
            SPI_REG_16(spi_tx_packet[i], SPI_COMMAND_IQ_2) = SPI_SWAP_DATA_TX(wifi_eth_rx_cmd_reader_buffer.command[i].current[1], 16);
            SPI_REG_u16(spi_tx_packet[i], SPI_COMMAND_KP_1) = SPI_SWAP_DATA_TX(wifi_eth_rx_cmd_reader_buffer.command[i].kp[0], 16);
            SPI_REG_u16(spi_tx_packet[i], SPI_COMMAND_KP_2) = SPI_SWAP_DATA_TX(wifi_eth_rx_cmd_reader_buffer.command[i].kp[1], 16);
            SPI_REG_u16(spi_tx_packet[i], SPI_COMMAND_KD_1) = SPI_SWAP_DATA_TX(wifi_eth_rx_cmd_reader_buffer.command[i].kd[0], 16);
            SPI_REG_u16(spi_tx_packet[i], SPI_COMMAND_KD_2) = SPI_SWAP_DATA_TX(wifi_eth_rx_cmd_reader_buffer.command[i].kd[1], 16);
            SPI_REG_u16(spi_tx_packet[i], SPI_COMMAND_ISAT_12) = SPI_SWAP_DATA_TX(wifi_eth_rx_cmd_reader_buffer.command[i].isat, 16);
        }
        else
        {
            SPI_REG_u16(spi_tx_packet[i], SPI_COMMAND_MODE) = SPI_SWAP_DATA_TX(1<<15, 16); //In case of zero commands, keep the system enabled
            SPI_REG_32(spi_tx_packet[i], SPI_COMMAND_POS_1) = 0;
            SPI_REG_32(spi_tx_packet[i], SPI_COMMAND_POS_2) = 0;
            SPI_REG_16(spi_tx_packet[i], SPI_COMMAND_VEL_1) = 0;
            SPI_REG_16(spi_tx_packet[i], SPI_COMMAND_VEL_2) = 0;
            SPI_REG_16(spi_tx_packet[i], SPI_COMMAND_IQ_1) = 0;
            SPI_REG_16(spi_tx_packet[i], SPI_COMMAND_IQ_2) = 0;
            SPI_REG_u16(spi_tx_packet[i], SPI_COMMAND_KP_1) = 0;
            SPI_REG_u16(spi_tx_packet[i], SPI_COMMAND_KP_2) = 0;
            SPI_REG_u16(spi_tx_packet[i], SPI_COMMAND_KD_1) = 0;
            SPI_REG_u16(spi_tx_packet[i], SPI_COMMAND_KD_2) = 0;
            SPI_REG_u16(spi_tx_packet[i], SPI_COMMAND_ISAT_12) = 0;
        }
        SPI_REG_u16(spi_tx_packet[i], SPI_TOTAL_INDEX) = SPI_SWAP_DATA_TX(spi_index_trans, 16);
        SPI_REG_u32(spi_tx_packet[i], SPI_TOTAL_CRC) = SPI_SWAP_DATA_TX(packet_compute_CRC(spi_tx_packet[i]), 32);
    }

    /* Perform every transaction the needed number of times */
    for (int spi_try = 0; spi_try < spi_n_attempt; spi_try++)
    {
        for (int i = 0; i < CONFIG_N_SLAVES; i++)
        {
            if (!TEST_BIT(spi_connected, i) && !spi_autodetect)
                continue; // ignoring this slave if it is not connected

            if (spi_done[i])
                continue; // ignoring this slave if the transaction has already been done
            
            spi_send(i, (uint8_t *)spi_tx_packet[i], (uint8_t *)spi_rx_packet[i], SPI_TOTAL_LEN * 2);
            //if (ms_cpt % 500 == 0) printf("%d %d\n", spi_try, i);

            // checking if data is correct
            if (packet_check_CRC(spi_rx_packet[i]))
            {
                spi_connected |= (1 << i); // noting that this slave is connected and working properly
                spi_done[i] = true;
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
                //printf("%d\n",ms_cpt);
                // zeroing sensor data in packet, except the status field
                memset(&(wifi_eth_tx_data.sensor[i]), 0, sizeof(struct sensor_data));
                wifi_eth_tx_data.sensor[i].status = 0xf; // specifying that the transaction failed in the sensor packet
            }
        }
        //Slave 7 is always the power board
        
        uint8_t tx_spi_powerboard[12]={0xaa,0xbb,0xcc,0xdd,0xee,0xff,0xaa,0xbb,0xcc,0xdd,0xee,0xff}; //Fake packet, cmd to the power board are not yet implemented
        uint8_t rx_spi_powerboard[12]={0};
        spi_send(7, tx_spi_powerboard,rx_spi_powerboard, 12);
        if (crc16_ccitt_check(rx_spi_powerboard,12)) //Packet valid?
        {
            wifi_eth_tx_data.powerboard.vbus = (rx_spi_powerboard[2]<<8) | rx_spi_powerboard[3];
            wifi_eth_tx_data.powerboard.vshunt = (rx_spi_powerboard[4]<<8) | rx_spi_powerboard[5];
            union floatbytes fb;
            fb.b[3] = rx_spi_powerboard[6];
            fb.b[2] = rx_spi_powerboard[7];
            fb.b[1] = rx_spi_powerboard[8];
            fb.b[0] = rx_spi_powerboard[9];
            wifi_eth_tx_data.powerboard.energy = fb.f;
            
        }
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

    /* Sends message to PC */
    switch (current_state)
    {

    case SENDING_INIT_ACK:
        /* Send acknowledge packets to PC */
        if (use_wifi)
        {
            wifi_send_data(&wifi_eth_tx_ack, sizeof(struct wifi_eth_packet_ack));
        }
        else
        {
            eth_send_data(&wifi_eth_tx_ack, sizeof(struct wifi_eth_packet_ack));
        }
        break;

    case WAITING_FOR_INIT:
        // nothing to be sent as we are constantly changing of wifi channel
        break;

    case SPI_AUTODETECT:
    case ACTIVE_CONTROL:
    case WIFI_ETH_ERROR:
        /* Send sensors packet to PC */
        wifi_eth_tx_data.sensor_index++;
        if (use_wifi)
        {
            wifi_send_data(&wifi_eth_tx_data, sizeof(struct wifi_eth_packet_sensor));
        }
        else
        {
            eth_send_data(&wifi_eth_tx_data, sizeof(struct wifi_eth_packet_sensor));
        }
        break;

    case WIFI_ETH_LINK_DOWN:
        // nothing can be sent, obviously
        break;

    default:
        // we send nothing to PC in case of a state machine error (should never happen)
        break;
    }
}

void setup_spi()
{
    spi_init();
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

void wifi_eth_receive_cb(uint8_t src_mac[6], uint8_t *data, int len, char eth_or_wifi)
{
    // received an init msg while waiting for one
    if (len == sizeof(struct wifi_eth_packet_init) && (current_state == WAITING_FOR_INIT || current_state == WIFI_ETH_ERROR))
    {
        struct wifi_eth_packet_init *packet_recv = (struct wifi_eth_packet_init *)data;

        if (packet_recv->protocol_version != PROTOCOL_VERSION)
        {
            wifi_eth_tx_ack.protocol_version = PROTOCOL_VERSION;
            wifi_eth_tx_ack.session_id = packet_recv->session_id;
            /* Send acknowledge packets to PC to inform version mismatch */
            if (use_wifi)
            {
                wifi_send_data(&wifi_eth_tx_ack, sizeof(struct wifi_eth_packet_ack));
            }
            else
            {
                eth_send_data(&wifi_eth_tx_ack, sizeof(struct wifi_eth_packet_ack));
            }
            ESP_LOGW("", "Wrong protocol version, got %d instead of %d, ignoring init packet.", packet_recv->protocol_version, PROTOCOL_VERSION);
            return; // ignoring packet
        }

        if (current_state == WAITING_FOR_INIT)
        {
            use_wifi = (eth_or_wifi == 'w');

            // if wifi is used, ethernet is deinitialized (eth stopped and driver uninstalled)
            // we avoid deinitializing ethernet if it has already been
            if (next_state == current_state)
            {
                next_state = SPI_AUTODETECT; // state transition before deinit for safety

                if (use_wifi)
                {
                    eth_deinit();
                }
                else
                {
                    wifi_deinit_func();
                }
            }
        }

        session_id = packet_recv->session_id; // set session id
        next_state = SPI_AUTODETECT;          // state transition

        // reset count for communication timeout
        wifi_eth_count = 0;
    }

    // received a command msg while waiting for one
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
            next_state = ACTIVE_CONTROL; // state transition
            command_index_prev = packet_recv->command_index - 1;
        }

        /* Write command message in a RTOS mailbox for thread safe comunication */
        xQueueOverwrite( wifi_eth_rx_cmd_mailbox, packet_recv );

        /* Compute data for next wifi_eth_sensor packet */
        wifi_eth_tx_data.packet_loss += ((struct wifi_eth_packet_command *)data)->command_index - command_index_prev - 1;
        wifi_eth_tx_data.last_cmd_index = ((struct wifi_eth_packet_command *)data)->command_index;
        command_index_prev = ((struct wifi_eth_packet_command *)data)->command_index;

        // reset count for communication timeout
        wifi_eth_count = 0;
    }
}

//function that will be called on a link state change
void wifi_eth_link_state_cb(bool new_state)
{
    // In WAITING_FOR_INIT, we don't know if wifi or ethernet is used
    // so we are ignoring any eth link state changes
    if (current_state == WAITING_FOR_INIT) 
        return;

    // When wifi is used, ethernet link state doesn't matter
    if (!use_wifi)
        next_state = new_state ? WIFI_ETH_ERROR : WIFI_ETH_LINK_DOWN; // transitioning to the corresponding state
}

void app_main()
{
    uart_set_baudrate(UART_NUM_0, 2000000);
    nvs_flash_init();
    wifi_eth_rx_cmd_mailbox = xQueueCreate( 1, sizeof(struct wifi_eth_packet_command));
    ws2812_control_init(); //init the LEDs
    set_all_leds(0x0f0f0f);
    ws2812_write_leds(ws_led);

    //printf("The core is : %d\n",xPortGetCoreID());
    printf("ETH/WIFI init size %u\n", sizeof(struct wifi_eth_packet_init));
    printf("ETH/WIFI command size %u\n", sizeof(struct wifi_eth_packet_command));
    printf("ETH/WIFI ack size %u\n", sizeof(struct wifi_eth_packet_ack));
    printf("ETH/WIFI sensor size %u\n", sizeof(struct wifi_eth_packet_sensor));
    printf("SPI size %u\n", SPI_TOTAL_LEN * 2);

    setup_spi();

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    eth_attach_link_state_cb(wifi_eth_link_state_cb);
    eth_attach_recv_cb(wifi_eth_receive_cb);
    eth_init();
    
    wifi_init();
    wifi_attach_recv_cb(wifi_eth_receive_cb);

    printf("initialise IMU\n");
    imu_init();

    next_state = WAITING_FOR_INIT;

    printf("Setup done\n");

    while (1)
    {
        vTaskDelay(10);
        ws2812_write_leds(ws_led);
    }
}
