#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

#define PROTOCOL_VERSION 3

/* Position of the values in the command packet */
#define UD_COMMAND_MODE 0
#define UD_COMMAND_POS_1 1
#define UD_COMMAND_POS_2 3
#define UD_COMMAND_VEL_1 5
#define UD_COMMAND_VEL_2 6
#define UD_COMMAND_IQ_1 7
#define UD_COMMAND_IQ_2 8
#define UD_COMMAND_KP_1 9
#define UD_COMMAND_KP_2 10
#define UD_COMMAND_KD_1 11
#define UD_COMMAND_KD_2 12
#define UD_COMMAND_ISAT_12 13

/* Command packet -> mode : bits */
//! \brief Enable system
#define UD_COMMAND_MODE_ES (1<<15)
//! \brief Enable motor 1
#define UD_COMMAND_MODE_EM1 (1<<14)
//! \brief Enable motor 2
#define UD_COMMAND_MODE_EM2 (1<<13)
//! \brief Raise an error if position rollover
#define UD_COMMAND_MODE_EPRE (1<<12)
//! \brief Enable Index Offset Compensation for Motor 1
#define UD_COMMAND_MODE_EI1OC (1<<11)
//! \brief Enable Index Offset Compensation for Motor 2
#define UD_COMMAND_MODE_EI2OC (1<<10)
//! \brief Disable system if no valid SPI packet is received during this time [in ms]. (0 = disabled)
#define UD_COMMAND_MODE_TIMEOUT (0xFF<<0)  //Timeout

/* Qvalues for each fields */
#define UD_QN_POS  24
#define UD_QN_VEL  11
#define UD_QN_IQ   10
#define UD_QN_ISAT 3
#define UD_QN_CR   15
#define UD_QN_ADC  16
#define UD_QN_KP   11
#define UD_QN_KD   10

#define IMU_QN_ACC 11
#define IMU_QN_GYR 11
#define IMU_QN_EF 13

/* Position of the values in the sensor packet */
#define UD_SENSOR_STATUS 0
#define UD_SENSOR_TIMESTAMP 1
#define UD_SENSOR_POS_1 2
#define UD_SENSOR_POS_2 4
#define UD_SENSOR_VEL_1 6
#define UD_SENSOR_VEL_2 7
#define UD_SENSOR_IQ_1 8
#define UD_SENSOR_IQ_2 9
#define UD_SENSOR_CR_1 10
#define UD_SENSOR_CR_2 11
#define UD_SENSOR_ADC_1 12
#define UD_SENSOR_ADC_2 13

/* sensor packet -> status : bits */
//! \brief System is enabled
#define UD_SENSOR_STATUS_SE (1<<15)
//! \brief Motor 1 is enabled
#define UD_SENSOR_STATUS_M1E (1<<14)
//! \brief Motor 1 is ready
#define UD_SENSOR_STATUS_M1R (1<<13)
//! \brief Motor 2 is enabled
#define UD_SENSOR_STATUS_M2E (1<<12)
//! \brief Motor 2 is ready
#define UD_SENSOR_STATUS_M2R (1<<11)
//! \brief Encoder 1 index has been detected
#define UD_SENSOR_STATUS_IDX1D (1<<10)
//! \brief Encoder 2 index has been detected
#define UD_SENSOR_STATUS_IDX2D (1<<9)
//! \brief Flips each time encoder 1 index is detected
#define UD_SENSOR_STATUS_IDX1T (1<<8)
//! \brief Flips each time encoder 1 index is detected
#define UD_SENSOR_STATUS_IDX2T (1<<7)

//! \brief Error code
#define UD_SENSOR_STATUS_ERROR (0xF<<0)

/* sensor packet -> status -> ERROR : values */
//! \brief No error
#define UD_SENSOR_STATUS_ERROR_NO_ERROR 0
//! \brief Encoder 1 error too high
#define UD_SENSOR_STATUS_ERROR_ENCODER1 1
//! \brief Timeout for receiving current references exceeded
#define UD_SENSOR_STATUS_ERROR_SPI_RECV_TIMEOUT 2
//! \brief Motor temperature reached critical value
//! \note This is currently unused as no temperature sensing is done.
#define UD_SENSOR_STATUS_ERROR_CRIT_TEMP 3  // currently unused
//! \brief Some error in the SpinTAC Position Convert module
#define UD_SENSOR_STATUS_ERROR_POSCONV 4
//! \brief Position Rollover occured
#define UD_SENSOR_STATUS_ERROR_POS_ROLLOVER 5
//! \brief Encoder 2 error too high
#define UD_SENSOR_STATUS_ERROR_ENCODER2 6
//! \brief Some other error
#define UD_SENSOR_STATUS_ERROR_OTHER 7
//! \brief CRC error in the SPI transaction
#define UD_SENSOR_STATUS_CRC_ERROR 15
//! \brief UD packets length in word (16 bits)
#define UD_LENGTH 14

/* To properly handle SPI type conversion */
#define SUB_REG_u16(p_packet, pos) (*((uint16_t *) ((p_packet) + (pos))))
#define SUB_REG_u32(p_packet, pos) (*((uint32_t *) ((p_packet) + (pos))))

#define SUB_REG_16(p_packet, pos) (*((int16_t *) ((p_packet) + (pos))))
#define SUB_REG_32(p_packet, pos) (*((int32_t *) ((p_packet) + (pos))))



#define D32Q24_TO_D16QN(a,n)      ((a)>>(24-(n))&0xFFFF)
#define D32Q24_TO_D8QN(a,n)       ((a)>>(24-(n))&0xFF)

#define uD16QN_TO_D32Q24(a,n)      (((uint32_t)(a))<<(24-(n)))
#define uD8QN_TO_D32Q24(a,n)       (((uint32_t)(a))<<(24-(n)))

#define D16QN_TO_D32Q24(a,n)      (((int32_t)(a))<<(24-(n)))
#define D8QN_TO_D32Q24(a,n)       (((int32_t)(a))<<(24-(n)))

#define D32QN_TO_FLOAT(a,n)       ((float)(a)) / (1<<(n))
#define D16QN_TO_FLOAT(a,n)       ((float)(a)) / (1<<(n))
#define D8QN_TO_FLOAT(a,n)        ((float)(a)) / (1<<(n))

#define FLOAT_TO_uD32QN(a,n)      ((uint32_t) std::min(std::max(((a) * (1<<(n))), 0.0), 4294967295.0))
#define FLOAT_TO_uD16QN(a,n)      ((uint16_t) std::min(std::max(((a) * (1<<(n))), 0.0), 65535.0))
#define FLOAT_TO_uD8QN(a,n)       ((uint8_t)  std::min(std::max(((a) * (1<<(n))), 0.0), 255.0))

#define FLOAT_TO_D32QN(a,n)       ((int32_t) std::min(std::max(((a) * (1<<(n))), -2147483647.0), 2147483647.0))
#define FLOAT_TO_D16QN(a,n)       ((int16_t) std::min(std::max((a) * (1<<(n)), -32767.0), 32767.0))
#define FLOAT_TO_D8QN(a,n)        ((int8_t)  std::min(std::max(((a) * (1<<(n))), -127.0, +127.0)))


#endif
