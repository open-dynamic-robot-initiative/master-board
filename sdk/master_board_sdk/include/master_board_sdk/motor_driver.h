#ifndef MOTORDRIVER_H
#define MOTORDRIVER_H
#include <stdint.h>
#include "master_board_sdk/motor.h"

class Motor;
class MotorDriver
{
public:
	MotorDriver();
	void EnableMotorDriver();
	void DisableMotorDriver();
	void SetMotors(Motor *motor1, Motor *motor2);
	void Print();
	void EnablePositionRolloverError();
  void DisablePositionRolloverError();
  void SetTimeout(uint8_t time);

	bool IsConnected();
	bool IsEnabled();
	int GetErrorCode();

	void Enable();
	void Disable();

	//adc
	float adc[2];

	//private:
	Motor *motor1;
	Motor *motor2;

	//status
	bool is_connected = 1; // if we don't know, we consider it connected
	bool is_enabled;
	int error_code;

	//commands
	bool enable;
	bool enable_position_rollover_error;
	uint8_t timeout;

	// Set functions for Python binding with Boost
  void set_motor1(Motor* mot) { this->motor1 = mot; };
	void set_motor2(Motor* mot) { this->motor2 = mot; };
	void set_is_connected(bool val) { this->is_connected = val; };
	void set_is_enabled(bool val) { this->is_enabled = val; };
	void set_error_code(int val) { this->error_code = val; };
	void set_enable(bool val) { this->enable = val;};
	void set_enable_position_rollover_error(bool val) { this->enable_position_rollover_error = val; };
	void set_timeout(uint8_t val) { this->timeout = val; };
	void set_adc(float adc_val []); // See definition in .cpp

	// Get functions for Python binding with Boost
  Motor* get_motor1() { return (this->motor1); };
	Motor* get_motor2() { return (this->motor2); };
	bool get_is_connected() { return this->is_connected; };
	bool get_is_enabled() { return this->is_enabled; };
	int get_error_code() { return this->error_code; };
	bool get_enable() { return this->enable; };
	bool get_enable_position_rollover_error() { return this->enable_position_rollover_error; };
	uint8_t get_timeout() { return this->timeout; };
};

#endif
