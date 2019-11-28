#ifndef SPI_QUAD_PACKET
#define SPI_QUAD_PACKET

#include <stdint.h>
#include <stdbool.h>

/* Position of the values in the command packet */
#define SPI_COMMAND_MODE 0
#define SPI_COMMAND_POS_1 1
#define SPI_COMMAND_POS_2 3
#define SPI_COMMAND_VEL_1 5
#define SPI_COMMAND_VEL_2 6
#define SPI_COMMAND_IQ_1 7
#define SPI_COMMAND_IQ_2 8
#define SPI_COMMAND_KP_1 9
#define SPI_COMMAND_KP_2 10
#define SPI_COMMAND_KD_1 11
#define SPI_COMMAND_KD_2 12
#define SPI_COMMAND_ISAT_12 13

/* Command packet -> mode : bits */
//! \brief Enable system
#define SPI_COMMAND_MODE_ES (1<<15)
//! \brief Enable motor 1
#define SPI_COMMAND_MODE_EM1 (1<<14)
//! \brief Enable motor 2
#define SPI_COMMAND_MODE_EM2 (1<<13)
//! \brief Raise an error if position rollover
#define SPI_COMMAND_MODE_EPRE (1<<12)
//! \brief Disable system if no valid SPI packet is received during this time [in ms]. (0 = disabled)
#define SPI_COMMAND_MODE_TIMEOUT (0xFF<<0)  //Timeout

/* Qvalues for each fields */
#define SPI_QN_POS  24
#define SPI_QN_VEL  11
#define SPI_QN_IQ   10
#define SPI_QN_ISAT 3
#define SPI_QN_CR   15
#define SPI_QN_ADC  16
#define SPI_QN_KP   16
#define SPI_QN_KD   16

/* Position of the values in the sensor packet */
#define SPI_SENSOR_STATUS 0
#define SPI_SENSOR_TIMESTAMP 1
#define SPI_SENSOR_POS_1 2
#define SPI_SENSOR_POS_2 4
#define SPI_SENSOR_VEL_1 6
#define SPI_SENSOR_VEL_2 7
#define SPI_SENSOR_IQ_1 8
#define SPI_SENSOR_IQ_2 9
#define SPI_SENSOR_CR_1 10
#define SPI_SENSOR_CR_2 11
#define SPI_SENSOR_ADC_1 12
#define SPI_SENSOR_ADC_2 13

/* sensor packet -> status : bits */
//! \brief System is enabled
#define SPI_SENSOR_STATUS_SE (1<<15)
//! \brief Motor 1 is enabled
#define SPI_SENSOR_STATUS_M1E (1<<14)
//! \brief Motor 1 is ready
#define SPI_SENSOR_STATUS_M1R (1<<13)
//! \brief Motor 2 is enabled
#define SPI_SENSOR_STATUS_M2E (1<<12)
//! \brief Motor 2 is ready
#define SPI_SENSOR_STATUS_M2R (1<<11)
//! \brief Error code
#define SPI_SENSOR_STATUS_ERROR (0xF<<0)

/* sensor packet -> status -> ERROR : values */
//! \brief No error
#define SPI_SENSOR_STATUS_ERROR_NO_ERROR 0
//! \brief Encoder error too high
#define SPI_SENSOR_STATUS_ERROR_ENCODER 1
//! \brief Timeout for receiving current references exceeded
#define SPI_SENSOR_STATUS_ERROR_SPI_RECV_TIMEOUT 2
//! \brief Motor temperature reached critical value
//! \note This is currently unused as no temperature sensing is done.
#define SPI_SENSOR_STATUS_ERROR_CRIT_TEMP 3  // currently unused
//! \brief Some error in the SpinTAC Position Convert module
#define SPI_SENSOR_STATUS_ERROR_POSCONV 4
//! \brief Position Rollover occured
#define SPI_SENSOR_STATUS_ERROR_POS_ROLLOVER 5
//! \brief Some other error
#define SPI_SENSOR_STATUS_ERROR_OTHER 7


#define SPI_TOTAL_INDEX 14
#define SPI_TOTAL_CRC 15

#define SPI_TOTAL_LEN 17

/* To properly handle SPI type conversion */
#define SPI_REG_u16(p_packet, pos) (*((uint16_t *) ((p_packet) + (pos))))
#define SPI_REG_u32(p_packet, pos) (*((uint32_t *) ((p_packet) + (pos))))

#define SPI_REG_16(p_packet, pos) (*((int16_t *) ((p_packet) + (pos))))
#define SPI_REG_32(p_packet, pos) (*((int32_t *) ((p_packet) + (pos))))



#define D32Q24_TO_D16QN(a,n)      ((a)>>(24-(n))&0xFFFF)
#define D32Q24_TO_D8QN(a,n)       ((a)>>(24-(n))&0xFF)

#define uD16QN_TO_D32Q24(a,n)      (((uint32_t)(a))<<(24-(n)))
#define uD8QN_TO_D32Q24(a,n)       (((uint32_t)(a))<<(24-(n)))

#define D16QN_TO_D32Q24(a,n)      (((int32_t)(a))<<(24-(n)))
#define D8QN_TO_D32Q24(a,n)       (((int32_t)(a))<<(24-(n)))

#define D32QN_TO_FLOAT(a,n)       ((float)(a)) / (1<<(n))
#define D16QN_TO_FLOAT(a,n)       ((float)(a)) / (1<<(n))
#define D8QN_TO_FLOAT(a,n)        ((float)(a)) / (1<<(n))

#define FLOAT_TO_uD32QN(a,n)      ((uint32_t) ((a) * (1<<(n))))
#define FLOAT_TO_uD16QN(a,n)      ((uint16_t) ((a) * (1<<(n))))
#define FLOAT_TO_uD8QN (a,n)      ((uint8_t)  ((a) * (1<<(n))))

#define FLOAT_TO_D32QN(a,n)       ((int32_t) ((a) * (1<<(n))))
#define FLOAT_TO_D16QN(a,n)       ((int16_t) ((a) * (1<<(n))))
#define FLOAT_TO_D8QN (a,n)       ((int8_t)  ((a) * (1<<(n))))


uint32_t packet_compute_CRC(uint16_t *packet);

bool packet_check_CRC(uint16_t *packet);

uint32_t packet_get_CRC(uint16_t *packet);

#endif
