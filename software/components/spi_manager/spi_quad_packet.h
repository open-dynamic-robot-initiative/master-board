#ifndef SPI_QUAD_PACKET
#define SPI_QUAD_PACKET

#include <stdint.h>
#include <stdbool.h>

struct spi_sensor {
    uint16_t status;
    uint16_t timestamp;
    uint32_t pos[2];
    uint16_t vel[2];
    uint16_t iq[2];
    uint16_t temp[2];
    uint16_t adc[2];
} __attribute__ ((packed));

#define SPI_COMMAND_MODE_ES (1<<15)         //enable system
#define SPI_COMMAND_MODE_EM1 (1<<14)        //enable motor 1
#define SPI_COMMAND_MODE_EM2 (1<<13)        //enable motor 2
#define SPI_COMMAND_CALIBRATE_EM1 (1<<12)   //calibrate motor 1
#define SPI_COMMAND_CALIBRATE_EM2 (1<<11)   //calibrate motor 2
#define SPI_COMMAND_MODE_TIMEOUT (0xFF<<0)  //Timeout

struct spi_command {
    uint16_t mode;
    uint32_t pos[2];
    uint16_t vel[2];
    uint16_t iq[2];
    uint16_t Kp[2];
    uint16_t Kd[2];
    uint16_t notUsed;
} __attribute__ ((packed));

typedef struct {
    union {
        struct spi_command command;
        struct spi_sensor sensor;
    } payload;
    uint16_t index;
    uint32_t CRC;
} __attribute__ ((packed)) spi_packet;


uint32_t packet_compute_CRC(spi_packet *packet);

bool packet_check_CRC(spi_packet *packet);

void packet_set_CRC(spi_packet *packet);


#endif