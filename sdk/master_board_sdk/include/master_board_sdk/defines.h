#ifndef DEFINES_H
#define DEFINES_H

#include "master_board_sdk/protocol.h"
#define N_SLAVES 6
#define N_SLAVES_CONTROLED 3

struct dual_motor_driver_sensor_packet_t
{
	uint16_t status;
	uint16_t timestamp;
	int32_t position[2];
	int16_t velocity[2];
	int16_t current[2];
	uint16_t coil_resistance[2];
	uint16_t adc[2];
} __attribute__((packed));

struct dual_motor_driver_command_packet_t
{
	uint16_t mode;
	int32_t position_ref[2];
	int16_t velocity_ref[2];
	int16_t current_ref[2];
	int16_t kp[2];
	int16_t kd[2];
	int8_t i_sat[2];
} __attribute__((packed));

struct imu_packet_t
{
	int16_t accelerometer[3];
	int16_t gyroscope[3];
	int16_t attitude[3];
} __attribute__((packed));

struct imu_data_t
{
	float accelerometer[3];
	float gyroscope[3];
	float attitude[3];
} __attribute__((packed));

struct dual_motor_driver_sensor_data_t
{
	uint16_t status;
	uint16_t timestamp; // to be removed
	bool is_system_enabled;
	bool is_motor_enabled[2];
	bool is_motor_ready[2];
	bool has_index_been_detected[2];
	bool index_toggle_bit[2];
	uint8_t error_code;
	float position[2];
	float velocity[2];
	float current[2];
	float coil_resistance[2];
	float adc[2];
} __attribute__((packed));

struct sensor_packet_t
{
	struct dual_motor_driver_sensor_packet_t dual_motor_driver_sensor_packets[N_SLAVES];
	struct imu_packet_t imu;
	uint16_t sensor_index;
	uint16_t last_index;
} __attribute__((packed));

struct command_packet_t
{
	//uint16_t command[N_SLAVES][UD_LENGTH];
	struct dual_motor_driver_command_packet_t dual_motor_driver_command_packets[N_SLAVES];
	uint16_t sensor_index;
} __attribute__((packed));

#endif