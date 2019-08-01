
#define N_SLAVES 6
#define N_SLAVES_CONTROLED 3

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


/* Qvalues for each fields */
#define SPI_QN_POS  24
#define SPI_QN_VEL  11
#define SPI_QN_IQ   10
#define SPI_QN_ISAT 3
#define SPI_QN_CR   15
#define SPI_QN_ADC  16
#define SPI_QN_KP   16
#define SPI_QN_KD   16

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
#define SPI_SENSOR_STATUS_ERROR (0x0F)
