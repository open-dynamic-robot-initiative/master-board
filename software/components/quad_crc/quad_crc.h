#ifndef QUAD_CRC_H
#define QUAD_CRC_H

#include <stdint.h>
#include <stdbool.h>

bool CRC_check(uint8_t *bytes, int len);
uint32_t CRC_compute(uint8_t *bytes, int len);

#endif