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

#endif
