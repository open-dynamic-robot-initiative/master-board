#include "spi_quad_packet.h"
#include "driver/spi_master.h"
#include "quad_crc.h"

uint32_t packet_compute_CRC(uint16_t *packet) {
  return CRC_compute((uint8_t*) packet, SPI_TOTAL_CRC*2);
}

bool packet_check_CRC(uint16_t *packet) {
    return (packet_compute_CRC(packet) == packet_get_CRC(packet));
}

uint32_t packet_get_CRC(uint16_t *packet) {
	return SPI_SWAP_DATA_RX(SPI_REG_u16(packet, SPI_TOTAL_CRC), 16) + (((uint32_t) SPI_SWAP_DATA_RX(SPI_REG_u16(packet, SPI_TOTAL_CRC+1), 16)) << 16);
}