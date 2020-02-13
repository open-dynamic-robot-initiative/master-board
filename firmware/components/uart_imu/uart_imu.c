#include "uart_imu.h"

#define FLOAT_TO_D16QN(a, n) ((int16_t)((a) * (1 << (n))))

#define UART_NUM UART_NUM_1
#define BUF_SIZE 100
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

union float_int {
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
// Here we use a ring buffer to protect the read/write memmory access.

uint8_t rxbuf[256];        //default buffer
uint8_t rxbuf_imu[3][256]; //buffer for  IMU packets
uint8_t rxbuf_ef[3][256];  //buffer for estimation filter packets

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
  uint8_t header[4];
  uint8_t *buffer_ptr;

  intr_cpt++;

  status = UART1.int_st.val;             // read UART interrupt Status
  rx_fifo_len = UART1.status.rxfifo_cnt; // read number of bytes in UART buffer

  while (rx_fifo_len > 4) //While there is at least 4 bytes to read (the header size)
  {
    //read header (4bytes) [0x75 - 0x65 - descriptor - payload_len]
    for (int j = 0; j < 4; j++)
    {
      header[j] = UART1.fifo.rw_byte;
      rx_fifo_len--;
    }
    //point to the corresponding packet buffer.
    switch (header[2])
    {
    case (0x80):
      read_index_imu = ((read_index_imu + 1) % 3); //position of the new valid data after this ISR
      buffer_ptr = rxbuf_imu[read_index_imu];
      break;
    case (0x82):
      read_index_ef = ((read_index_ef + 1) % 3); //position of the new valid data after this ISR
      buffer_ptr = rxbuf_ef[read_index_ef];
      break;
    default:
      buffer_ptr = rxbuf;
    }
    //copy the header
    for (int j = 0; j < 4; j++)
    {
      buffer_ptr[j] = header[j];
    }
    int len = header[3] + 2;
    if (len > rx_fifo_len)
    {
      break; // The message is too short.
    }
    //copy the rest of the packet
    for (i = 0; i < len; i++)
    {
      buffer_ptr[i + 4] = UART1.fifo.rw_byte; // read all bytes
      rx_fifo_len--;
    }
  }
  //Flush the UART fifo? is it usefull?
  while (rx_fifo_len)
  {
    UART1.fifo.rw_byte;
    rx_fifo_len--;
  }

  // after reading bytes from buffer clear UART interrupt status
  uart_clear_intr_status(UART_NUM, UART_RXFIFO_FULL_INT_CLR | UART_RXFIFO_TOUT_INT_CLR);
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
  /***IMU****/
  if (check_IMU_CRC(rxbuf_imu[read_index_imu], 34))
  {
    imu.acc_x.ul = FLOAT_FROM_BYTE_ARRAY(rxbuf_imu[read_index_imu], ACCX_POS);
    imu.acc_y.ul = FLOAT_FROM_BYTE_ARRAY(rxbuf_imu[read_index_imu], ACCY_POS);
    imu.acc_z.ul = FLOAT_FROM_BYTE_ARRAY(rxbuf_imu[read_index_imu], ACCZ_POS);
    imu.gyr_x.ul = FLOAT_FROM_BYTE_ARRAY(rxbuf_imu[read_index_imu], GYRX_POS);
    imu.gyr_y.ul = FLOAT_FROM_BYTE_ARRAY(rxbuf_imu[read_index_imu], GYRY_POS);
    imu.gyr_z.ul = FLOAT_FROM_BYTE_ARRAY(rxbuf_imu[read_index_imu], GYRZ_POS);
  }
  /***EF****/
  if (check_IMU_CRC(rxbuf_ef[read_index_ef], 38))
  {
    imu.roll.ul = FLOAT_FROM_BYTE_ARRAY(rxbuf_ef[read_index_ef], EFR_POS);
    imu.pitch.ul = FLOAT_FROM_BYTE_ARRAY(rxbuf_ef[read_index_ef], EFP_POS);
    imu.yaw.ul = FLOAT_FROM_BYTE_ARRAY(rxbuf_ef[read_index_ef], EFY_POS);
    imu.linacc_x.ul = FLOAT_FROM_BYTE_ARRAY(rxbuf_ef[read_index_ef], EFLINACCX_POS);
    imu.linacc_y.ul = FLOAT_FROM_BYTE_ARRAY(rxbuf_ef[read_index_ef], EFLINACCY_POS);
    imu.linacc_z.ul = FLOAT_FROM_BYTE_ARRAY(rxbuf_ef[read_index_ef], EFLINACCZ_POS);
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
    printf("%02x", ptr[i]);
  }
  printf("\n");
}

int imu_init()
{
  /* Init uart */
  printf("initialising uart for IMU\n");
  /* Configure parameters of an UART driver,
     * communication pins and install the driver */
  uart_config_t uart_config = {
      .baud_rate = 115200,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE};
  uart_param_config(UART_NUM, &uart_config);
  uart_set_pin(UART_NUM, PIN_TXD, PIN_RXD, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
  uart_driver_install(UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0);

  /* init imu */
  printf("initialising CX5-25 IMU\n");
  /*
    75 65 01 02 02 02 E1 C7                           // Put the Device in Idle Mode
    75 65 0C 0A 0A 08 01 02 04 00 01 05 00 01 10 73   // IMU data: acc+gyr at 1000Hz
    75 65 0C 0A 0A 0A 01 02 05 00 01 0D 00 01 1B A3   // EF data: RPY + LinACC at 500Hz (max)
    75 65 0C 07 07 0A 01 01 05 00 01 06 23            // EF data: RPY at 500Hz (max)
    75 65 0C 0A 05 11 01 01 01 05 11 01 03 01 24 CC   // Enable the data stream for IMU and EF
    75 65 0D 06 06 03 00 00 00 00 F6 E4               // set heading at 0
    75 65 01 02 02 06 E5 CB                           // Resume the Device (is it needed?)
  */
  const char cmd0[8] = {0x75, 0x65, 0x01, 0x02, 0x02, 0x02, 0xE1, 0xC7};
  const char cmd1[16] = {0x75, 0x65, 0x0C, 0x0A, 0x0A, 0x08, 0x01, 0x02, 0x04, 0x00, 0x01, 0x05, 0x00, 0x01, 0x10, 0x73}; //IMU 1000Hz
  //const char cmd1[16] = {0x75, 0x65, 0x0C, 0x0A, 0x0A, 0x08, 0x01, 0x02, 0x04, 0x00, 0x0A, 0x05, 0x00, 0x0A, 0x22, 0xa0}; //IMU 100Hz
  //const char cmd2[13] = {0x75, 0x65, 0x0C, 0x07, 0x07, 0x0A, 0x01, 0x01, 0x05, 0x00, 0x01, 0x06, 0x23}; //EF RPY 500Hz
  //const char cmd2[13] = {0x75, 0x65, 0x0C, 0x07, 0x07, 0x0A, 0x01, 0x01, 0x05, 0x00, 0x0A, 0x0f, 0x2c};//EF RPY 50Hz
  const char cmd2[16] = {0x75, 0x65, 0x0C, 0x0A, 0x0A, 0x0A, 0x01, 0x02, 0x05, 0x00, 0x01, 0x0D, 0x00, 0x01, 0x1b, 0xa3}; //EF RPY + LinACC 500Hz

  const char cmd3[16] = {0x75, 0x65, 0x0C, 0x0A, 0x05, 0x11, 0x01, 0x01, 0x01, 0x05, 0x11, 0x01, 0x03, 0x01, 0x24, 0xCC};
  const char cmd4[12] = {0x75, 0x65, 0x0D, 0x06, 0x06, 0x03, 0x00, 0x00, 0x00, 0x00, 0xF6, 0xE4};
  const char cmd5[8] = {0x75, 0x65, 0x01, 0x02, 0x02, 0x06, 0xE5, 0xCB};
  const char cmd6[13] = {0x75, 0x65, 0x0C, 0x07, 0x07, 0x40, 0x01, 0x00, 0x0E, 0x10, 0x00, 0x53, 0x9D}; // 921600 bauds

  printf("sending new baud rate setting to the IMU \n");
  vTaskDelay(30);
  uart_write_bytes(UART_NUM, cmd0, sizeof(cmd0));
  vTaskDelay(10);
  uart_write_bytes(UART_NUM, cmd6, sizeof(cmd6));
  vTaskDelay(1);
  uart_flush_input(UART_NUM);
  uart_set_baudrate(UART_NUM, 921600);
  uart_set_rx_timeout(UART_NUM, 5); //timeout in symbols
  // release the pre registered UART handler/subroutine
  uart_isr_free(UART_NUM);
  // register new UART subroutine
  uart_isr_register(UART_NUM, uart_intr_handle, NULL, ESP_INTR_FLAG_IRAM, &handle_console);
  // enable RX interrupt
  vTaskDelay(10);
  printf("Done\n");
  uart_write_bytes(UART_NUM, cmd1, sizeof(cmd1));
  //vTaskDelay(10);
  uart_write_bytes(UART_NUM, cmd2, sizeof(cmd2));
  //vTaskDelay(10);
  uart_write_bytes(UART_NUM, cmd3, sizeof(cmd3));
  //vTaskDelay(10);
  uart_write_bytes(UART_NUM, cmd4, sizeof(cmd4));
  //vTaskDelay(10);
  uart_write_bytes(UART_NUM, cmd5, sizeof(cmd5));
  //vTaskDelay(10);
  uart_enable_rx_intr(UART_NUM);

  while (0) //for debug
  {
    printf(" intr_cpt:%d\n", intr_cpt);
    parse_IMU_data();
    print_imu();
    vTaskDelay(100);
  }
  return 0;
}