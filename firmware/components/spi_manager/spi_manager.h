#ifndef SPI_MANAGER_H
#define SPI_MANAGER_H

#include "driver/spi_master.h"
#include "spi_quad_packet.h"

#define PIN_NUM_MISO 12
#define PIN_NUM_MOSI 13
#define PIN_NUM_CLK  14
#define PIN_NUM_CS   4

#define GPIO_DEMUX_OE 15
#define GPIO_DEMUX_A0 5
#define GPIO_DEMUX_A1 4
#define GPIO_DEMUX_A2 16

#define GPIO_DEMUX_PIN_SEL ((1ULL<<GPIO_DEMUX_A0) | (1ULL<<GPIO_DEMUX_A1) | (1ULL<<GPIO_DEMUX_A2))


#if CONFIG_SPI_DATARATE_1
  #define CONFIG_SPI_DATARATE_FACTOR 80
#elif CONFIG_SPI_DATARATE_2
  #define CONFIG_SPI_DATARATE_FACTOR 40
#elif CONFIG_SPI_DATARATE_3
  #define CONFIG_SPI_DATARATE_FACTOR 26
#elif CONFIG_SPI_DATARATE_4
  #define CONFIG_SPI_DATARATE_FACTOR 20
#elif CONFIG_SPI_DATARATE_6
  #define CONFIG_SPI_DATARATE_FACTOR 13
#elif CONFIG_SPI_DATARATE_8
  #define CONFIG_SPI_DATARATE_FACTOR 10
#else
  #error No valid SPI datarate specified
#endif

void spi_init();
bool spi_send(int slave, uint8_t *tx_data, uint8_t *rx_data, int len);

#endif