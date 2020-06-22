#include <stdio.h>
#include "master_board_sdk/motor_driver.h"

MotorDriver::MotorDriver()
{
  enable = false;
  enable_position_rollover_error = false;
  timeout = 0;
}

void MotorDriver::SetMotors(Motor *motor1, Motor *motor2)
{
  this->motor1 = motor1;
  this->motor2 = motor2;
}

void MotorDriver::Print()
{
  printf("%9d | ", is_connected);
  printf("%7d | ", is_enabled);
  if (error_code != 0) //error code printed in red
  {
    printf("\033[0;31m");
  }
  printf("%5d", error_code);
  printf("\033[0m");
  printf(" | \n");
}

void MotorDriver::Enable()
{
  enable = true;
}

void MotorDriver::Disable()
{
  enable = false;
}

void MotorDriver::EnablePositionRolloverError()
{
  enable_position_rollover_error = true;
}

void MotorDriver::DisablePositionRolloverError()
{
  enable_position_rollover_error = false;
}

void MotorDriver::SetTimeout(uint8_t time)
{
  timeout = time;
}

bool MotorDriver::IsConnected()
{
  return is_connected;
}

bool MotorDriver::IsEnabled()
{
  return is_enabled;
}

int MotorDriver::GetErrorCode()
{
  return error_code;
}

void MotorDriver::set_adc(float adc_val [])
{
  // The adc property is defined as float adc[2]
  (this->adc)[0] = adc_val[0];
  (this->adc)[1] = adc_val[1];
}


