#ifndef QUAD_CRC_H
#define QUAD_CRC_H

#include <stdint.h>
#include <stdbool.h>

bool CRC_check(uint8_t *bytes, int len);
uint32_t CRC_compute(uint8_t *bytes, int len);
uint16_t crc16_ccitt(uint8_t * data, int len);
bool crc16_ccitt_check(uint8_t * data, int len);
#endif