#include "uart_imu.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#define FLOAT_TO_D16QN(a, n) ((int16_t)((a) * (1 << (n))))

#define UART_NUM UART_NUM_1
#define BUF_SIZE 128
#define PIN_TXD 32
#define PIN_RXD 35

//possition of the data in the IMU message
#define ACCX_POS 6
#define ACCY_POS 10
#define ACCZ_POS 14
#define GYRX_POS 20
#define GYRY_POS 24
#define GYRZ_POS 28

//possition of the data in the EF message
#define EFR_POS 6
#define EFP_POS 10
#define EFY_POS 14
#define EFLINACCX_POS 22
#define EFLINACCY_POS 26
#define EFLINACCZ_POS 30
#define FLOAT_FROM_BYTE_ARRAY(buff, n) ((buff[n] << 24) | (buff[n + 1] << 16) | (buff[n + 2] << 8) | (buff[n + 3]));

/* Qvalues for each fields */
#define IMU_QN_ACC 11
#define IMU_QN_GYR 11
#define IMU_QN_EF 13

union float_int
{
    float f;
    unsigned long ul;
};

struct strcut_imu_data
{
    union float_int acc_x;
    union float_int acc_y;
    union float_int acc_z;
    union float_int gyr_x;
    union float_int gyr_y;
    union float_int gyr_z;
    union float_int roll;
    union float_int pitch;
    union float_int yaw;
    union float_int linacc_x;
    union float_int linacc_y;
    union float_int linacc_z;
};
struct strcut_imu_data imu = {0};

static intr_handle_t handle_console;

// Receive buffer to collect incoming data
uint8_t rxbuf[128] = {0};     //default buffer
uint8_t rxbuf_imu[128] = {0}; //buffer for  IMU packets
uint8_t rxbuf_ef[128] = {0};  //buffer for estimation filter packets

/* Define mailbox for thread safe exchange between interupt and main loop*/
QueueHandle_t imu_mailbox;
QueueHandle_t ef_mailbox;

int intr_cpt = 0;
uint8_t read_index_imu = 0; //where to read the latest updated imu data
uint8_t read_index_ef = 0;  //where to read the latest updated ef data

/*
 * Define UART interrupt subroutine to ackowledge interrupt
 */
static void IRAM_ATTR uart_intr_handle(void *arg)
{
    uint16_t rx_fifo_len, status;
    uint16_t i = 0;
    status = UART1.int_st.val;             // read UART interrupt Status
    rx_fifo_len = UART1.status.rxfifo_cnt; // read number of bytes in UART buffer
    intr_cpt++;
    //read all bytes from rx fifo
    for (i = 0; i < rx_fifo_len; i++)
    {
        rxbuf[i] = UART1.fifo.rw_byte; // read all bytes
    }
    // Fix of esp32 hardware bug as in https://github.com/espressif/arduino-esp32/pull/1849
    while (UART1.status.rxfifo_cnt || (UART1.mem_rx_status.wr_addr != UART1.mem_rx_status.rd_addr))
    {
        UART1.fifo.rw_byte;
    }

    i = 0;
    //read frame, wich can be: EF only, IMU only, or EF + IMU
    while ((i + 4) < rx_fifo_len) //while at least a full header (4bytes) is in the buffer
    {
        //header strucure: [0x75 - 0x65 - descriptor - payload_len]
        int size = rxbuf[i + 3] + 2 + 4;

        if (rxbuf[i] != 0x75 || rxbuf[i + 1] != 0x65)
        {
            break; //The data doesn't look like the expected header
        }
        if (size > rx_fifo_len)
        {
            break;
        }
        switch (rxbuf[i + 2]) //descriptor
        {
        case (0x80): //IMU descriptor
            xQueueOverwriteFromISR(imu_mailbox, &rxbuf[i], NULL);
            break;
        case (0x82): //EF descriptor
            xQueueOverwriteFromISR(ef_mailbox, &rxbuf[i], NULL);
            break;
        default:
            break; // We don't deal with this descriptor
        }
        i += size;
    }
    // clear UART interrupt status
    uart_clear_intr_status(UART_NUM, status);
}

inline bool check_IMU_CRC(unsigned char *data, int len)
{
    if (len < 2)
        return false;
    unsigned char checksum_byte1 = 0;
    unsigned char checksum_byte2 = 0;
    for (int i = 0; i < (len - 2); i++)
    {
        checksum_byte1 += data[i];
        checksum_byte2 += checksum_byte1;
    }
    return (data[len - 2] == checksum_byte1 && data[len - 1] == checksum_byte2);
}

inline int parse_IMU_data()
{

    xQueuePeek(imu_mailbox, &rxbuf_imu, 0);
    xQueuePeek(ef_mailbox, &rxbuf_ef, 0);

    /***IMU****/
    if (check_IMU_CRC(rxbuf_imu, 34))
    {
        imu.acc_x.ul = FLOAT_FROM_BYTE_ARRAY(rxbuf_imu, ACCX_POS);
        imu.acc_y.ul = FLOAT_FROM_BYTE_ARRAY(rxbuf_imu, ACCY_POS);
        imu.acc_z.ul = FLOAT_FROM_BYTE_ARRAY(rxbuf_imu, ACCZ_POS);
        imu.gyr_x.ul = FLOAT_FROM_BYTE_ARRAY(rxbuf_imu, GYRX_POS);
        imu.gyr_y.ul = FLOAT_FROM_BYTE_ARRAY(rxbuf_imu, GYRY_POS);
        imu.gyr_z.ul = FLOAT_FROM_BYTE_ARRAY(rxbuf_imu, GYRZ_POS);
    }
    /***EF****/
    if (check_IMU_CRC(rxbuf_ef, 38))
    {
        imu.roll.ul = FLOAT_FROM_BYTE_ARRAY(rxbuf_ef, EFR_POS);
        imu.pitch.ul = FLOAT_FROM_BYTE_ARRAY(rxbuf_ef, EFP_POS);
        imu.yaw.ul = FLOAT_FROM_BYTE_ARRAY(rxbuf_ef, EFY_POS);
        imu.linacc_x.ul = FLOAT_FROM_BYTE_ARRAY(rxbuf_ef, EFLINACCX_POS);
        imu.linacc_y.ul = FLOAT_FROM_BYTE_ARRAY(rxbuf_ef, EFLINACCY_POS);
        imu.linacc_z.ul = FLOAT_FROM_BYTE_ARRAY(rxbuf_ef, EFLINACCZ_POS);
    }
    return 0;
}

uint16_t get_acc_x_in_D16QN() { return FLOAT_TO_D16QN(imu.acc_x.f, IMU_QN_ACC); }
uint16_t get_acc_y_in_D16QN() { return FLOAT_TO_D16QN(imu.acc_y.f, IMU_QN_ACC); }
uint16_t get_acc_z_in_D16QN() { return FLOAT_TO_D16QN(imu.acc_z.f, IMU_QN_ACC); }

uint16_t get_gyr_x_in_D16QN() { return FLOAT_TO_D16QN(imu.gyr_x.f, IMU_QN_GYR); }
uint16_t get_gyr_y_in_D16QN() { return FLOAT_TO_D16QN(imu.gyr_y.f, IMU_QN_GYR); }
uint16_t get_gyr_z_in_D16QN() { return FLOAT_TO_D16QN(imu.gyr_z.f, IMU_QN_GYR); }

uint16_t get_roll_in_D16QN() { return FLOAT_TO_D16QN(imu.roll.f, IMU_QN_EF); }
uint16_t get_pitch_in_D16QN() { return FLOAT_TO_D16QN(imu.pitch.f, IMU_QN_EF); }
uint16_t get_yaw_in_D16QN() { return FLOAT_TO_D16QN(imu.yaw.f, IMU_QN_EF); }

uint16_t get_linacc_x_in_D16QN() { return FLOAT_TO_D16QN(imu.linacc_x.f, IMU_QN_ACC); }
uint16_t get_linacc_y_in_D16QN() { return FLOAT_TO_D16QN(imu.linacc_y.f, IMU_QN_ACC); }
uint16_t get_linacc_z_in_D16QN() { return FLOAT_TO_D16QN(imu.linacc_z.f, IMU_QN_ACC); }

void print_imu()
{
    printf("\n%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t",
           imu.acc_x.f,
           imu.acc_y.f,
           imu.acc_z.f,
           imu.gyr_x.f,
           imu.gyr_y.f,
           imu.gyr_z.f,
           imu.roll.f,
           imu.pitch.f,
           imu.yaw.f,
           imu.linacc_x.f,
           imu.linacc_y.f,
           imu.linacc_z.f);
}

void print_table(uint8_t *ptr, int len)
{
    for (int i = 0; i < len; i++)
    {
        printf("%02x ", ptr[i]);
    }
    printf("\n");
}

void custom_write_uart(unsigned char *buff, int size)
{
    int i = 0;
    for (i = 0; i < size; i++)
    {
        UART1.fifo.rw_byte = buff[i];
    }
}

int imu_init()
{
    /* Init uart */
    printf("Initialising uart for IMU...\n");

    imu_mailbox = xQueueCreate(1, 128);
    ef_mailbox = xQueueCreate(1, 128);

    //Configure UART 115200 bauds
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE};

    uart_param_config(UART_NUM, &uart_config);
    uart_set_rx_timeout(UART_NUM, 3); //timeout in symbols, this will generate an interrupt per RX data frame
    uart_set_pin(UART_NUM, PIN_TXD, PIN_RXD, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    /*
    75 65 01 02 02 02 E1 C7                           // Put the Device in Idle Mode
    75 65 0C 0A 0A 08 01 02 04 00 01 05 00 01 10 73   // IMU data: acc+gyr at 1000Hz
    75 65 0C 0A 0A 0A 01 02 05 00 01 0D 00 01 1B A3   // EF data: RPY + LinACC at 500Hz (max)
    75 65 0C 07 07 0A 01 01 05 00 01 06 23            // EF data: RPY at 500Hz (max)
    75 65 0C 0A 05 11 01 01 01 05 11 01 03 01 24 CC   // Enable the data stream for IMU and EF
    75 65 0D 06 06 03 00 00 00 00 F6 E4               // set heading at 0
    75 65 01 02 02 06 E5 CB                           // Resume the Device (is it needed?)
    */
    const char cmd0[8] = {0x75, 0x65, 0x01, 0x02, 0x02, 0x02, 0xE1, 0xC7};                                                  // Put the Device in Idle Mode
    const char cmd1[16] = {0x75, 0x65, 0x0C, 0x0A, 0x0A, 0x08, 0x01, 0x02, 0x04, 0x00, 0x01, 0x05, 0x00, 0x01, 0x10, 0x73}; // IMU data: acc+gyr at 1000Hz
    const char cmd2[16] = {0x75, 0x65, 0x0C, 0x0A, 0x0A, 0x0A, 0x01, 0x02, 0x05, 0x00, 0x01, 0x0D, 0x00, 0x01, 0x1b, 0xa3}; // EF data: RPY + LinACC at 500Hz (max)
    const char cmd3[16] = {0x75, 0x65, 0x0C, 0x0A, 0x05, 0x11, 0x01, 0x01, 0x01, 0x05, 0x11, 0x01, 0x03, 0x01, 0x24, 0xCC}; // Enable the data stream for IMU and EF
    const char cmd4[12] = {0x75, 0x65, 0x0D, 0x06, 0x06, 0x03, 0x00, 0x00, 0x00, 0x00, 0xF6, 0xE4};                         // set heading at 0
    const char cmd5[8] = {0x75, 0x65, 0x01, 0x02, 0x02, 0x06, 0xE5, 0xCB};                                                  // Resume the Device
    const char cmd6[13] = {0x75, 0x65, 0x0C, 0x07, 0x07, 0x40, 0x01, 0x00, 0x0E, 0x10, 0x00, 0x53, 0x9D};                   // 921600 bauds

    vTaskDelay(100 / portTICK_PERIOD_MS); //Let the IMU some time to boot    (TODO: read uart and wait for IMU acknoledgment on cmd0 to optimize boot time and/or detect the absence of IMU)
    custom_write_uart(cmd0, sizeof(cmd0));
    vTaskDelay(3);
    custom_write_uart(cmd1, sizeof(cmd1));
    vTaskDelay(3);
    custom_write_uart(cmd2, sizeof(cmd2));
    vTaskDelay(3);
    custom_write_uart(cmd3, sizeof(cmd3));
    vTaskDelay(3);
    custom_write_uart(cmd4, sizeof(cmd4));
    vTaskDelay(3);
    custom_write_uart(cmd5, sizeof(cmd5));
    vTaskDelay(3);
    custom_write_uart(cmd6, sizeof(cmd6));
    vTaskDelay(3);
    uart_set_baudrate(UART_NUM, 921600);
    uart_driver_install(UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_disable_tx_intr(UART_NUM);
    uart_disable_rx_intr(UART_NUM);
    uart_isr_free(UART_NUM);
    uart_isr_register(UART_NUM, uart_intr_handle, NULL, ESP_INTR_FLAG_IRAM, &handle_console);
    uart_enable_rx_intr(UART_NUM);

    while (0) //for debug
    {
        parse_IMU_data();
        printf(" intr_cpt:%d\n", intr_cpt);
        printf("rxbuf:     ");
        print_table(rxbuf, 80);
        printf("rxbuf_imu: ");
        print_table(rxbuf_imu, 80);
        printf("rxbuf_ef:  ");
        print_table(rxbuf_ef, 80);
        print_imu();
        vTaskDelay(300/portTICK_PERIOD_MS);
    }
    return 0;
}
