#ifndef DEFINES_H
#define DEFINES_H

#include <stdint.h>

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
	uint16_t init_id;
} __attribute__ ((packed));

struct wifi_eth_packet_command {
    struct command_data command[CONFIG_N_SLAVES];
    uint16_t command_index;
} __attribute__ ((packed));

struct wifi_eth_packet_ack {
	uint16_t ack_id;
} __attribute__ ((packed));

struct wifi_eth_packet_sensor {
    struct sensor_data sensor[CONFIG_N_SLAVES];
    struct imu_data imu;
    uint16_t sensor_index;
    uint16_t packet_loss;
} __attribute__ ((packed));

enum State {
	WAITING_FOR_INIT = 0, // waiting for initialization msg
	SENDING_INIT_ACK = 1, // sending acknowlegment msg to PC
	ACTIVE_CONTROL = 2 // normal active control behaviour with the robot
};

#endif