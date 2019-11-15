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


	void Enable();
	void Disable();
	//private:
	Motor *motor1;
	Motor *motor2;
	//status
	bool is_enabled;
	int error_code;

	//commands
	bool enable;
	bool enable_position_rollover_error;
	uint8_t timeout;
};

#endif