#ifndef SPI_QUAD_PACKET
#define SPI_QUAD_PACKET

#include <stdint.h>
#include <stdbool.h>

struct spi_control_values {
    uint16_t Iq[2];
    uint16_t Kp[2];
    uint16_t Kd[2];
    uint16_t Pos[2];
    uint16_t Vel[2];
};

#define SPI_CONTROL_CMD_FLAGS_ENA_SYS (1 << 0)
#define SPI_CONTROL_CMD_FLAGS_ENA_MTR1 (1 << 1)
#define SPI_CONTROL_CMD_FLAGS_ENA_MTR2 (1 << 2)
#define SPI_CONTROL_CMD_FLAGS_ENA_VSPR1 (1 << 3)
#define SPI_CONTROL_CMD_FLAGS_ENA_VSPR2 (1 << 4)
#define SPI_CONTROL_CMD_FLAGS_ENA_RLVRERR (1 << 5)

struct spi_control_cmd_flags {
    uint16_t flags;
    uint32_t recv_Iq_timeout;
};

struct spi_control_cmd {
    struct spi_control_cmd_flags present_flags;
    struct spi_control_cmd_flags flags_values;
};

struct spi_control {
    uint16_t cmd;
    union {
        struct spi_control_values values;
        struct spi_control_cmd commands;
    } payload;
};

struct spi_measures {
    uint16_t status;

    uint16_t Pos[2];
    uint16_t Vel[2];
    uint16_t Iq[2];
    uint16_t Temp[2];
    uint16_t ADC[2];
};

typedef struct {
    union {
        struct spi_control control;
        struct spi_measures measures;
    } payload;
    uint32_t CRC;
} spi_packet;


uint32_t packet_compute_CRC(spi_packet *packet);

bool packet_check_CRC(spi_packet *packet);

void packet_set_CRC(spi_packet *packet);


#endif