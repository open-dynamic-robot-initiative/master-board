#ifndef DEFINES_H
#define DEFINES_H

#include <stdint.h>

#define PROTOCOL_VERSION 2

struct sensor_data {
	uint16_t status;
	uint16_t timestamp;
	int32_t position[2];
	int16_t velocity[2];
	int16_t current[2];
	uint16_t coil_resistance[2];
	uint16_t adc[2];
} __attribute__((packed));

struct command_data {
	uint16_t mode;
	int32_t position[2];
	int16_t velocity[2];
	int16_t current[2];
	uint16_t kp[2];
	uint16_t kd[2];
	uint16_t isat;
} __attribute__((packed));

struct imu_data {
	int16_t accelerometer[3];
	int16_t gyroscope[3];
	int16_t attitude[3];
	int16_t linear_acceleration[3];
} __attribute__((packed));

struct wifi_eth_packet_init {
	uint16_t protocol_version; // used to ensure both the interface and the firmware use the same protocol
	uint16_t session_id;
} __attribute__ ((packed));

struct wifi_eth_packet_command {
	uint16_t session_id;
    struct command_data command[CONFIG_N_SLAVES];
    uint16_t command_index;
} __attribute__ ((packed));

struct wifi_eth_packet_ack {
	uint16_t session_id;
	uint8_t spi_connected; // least significant bit: SPI0
						   // most significant bit: SPI7
} __attribute__ ((packed));

struct wifi_eth_packet_sensor {
	uint16_t session_id;
    struct sensor_data sensor[CONFIG_N_SLAVES];
    struct imu_data imu;
    uint16_t sensor_index;
    uint16_t packet_loss;
	uint16_t last_cmd_index;
} __attribute__ ((packed));

enum State {
	SETUP = 0, // setting up every component
	WAITING_FOR_INIT = 1, // waiting for initialization msg
	SPI_AUTODETECT = 2, // detecting connected spi slaves
	SENDING_INIT_ACK = 3, // sending acknowlegment msg to PC
	ACTIVE_CONTROL = 4, // normal active control behaviour with the robot
	WIFI_ETH_LINK_DOWN = 5, // ethernet link is down
	WIFI_ETH_ERROR = 6 // master board has not received any meaningful message over wifi or ethernet
					   // over a certain period of time (timeouts to configure) in SENDING_INIT_ACK or ACTIVE_CONTROL state
};

#endif