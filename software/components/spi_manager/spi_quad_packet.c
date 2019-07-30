#include "spi_quad_packet.h"
#include "driver/spi_master.h"
#include "quad_crc.h"

uint32_t packet_compute_CRC(uint16_t *packet) {
  return CRC_compute((uint8_t*) packet, SPI_TOTAL_CRC*2);
}

bool packet_check_CRC(uint16_t *packet) {
    return (packet_compute_CRC(packet) == SPI_REG_u32(packet, SPI_TOTAL_CRC));
}

void packet_set_CRC(uint16_t *packet) {
	SPI_REG_u32(packet, SPI_TOTAL_CRC) = packet_compute_CRC(packet);
}