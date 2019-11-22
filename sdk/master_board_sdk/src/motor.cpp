#include <stdio.h>
#include "master_board_sdk/motor.h"

Motor::Motor()
{
  position = 0;
  velocity = 0;
  current = 0;
  is_enabled = false;
  is_ready = false;
  index_toggle_bit = false;
  has_index_been_detected = false;
  direction = 1;
  position_offset = 0.;

  enable = false;
  enable_position_rollover_error = false;
  enable_index_toggle_bit = false;
  enable_index_offset_compensation = false;
}
void Motor::SetDriver(MotorDriver *driver)
{
  this->driver = driver;
}

void Motor::Print()
{
  printf("enabled:%d ", is_enabled);
  printf("ready:%d ", is_ready);
  printf("IDXT:%d ", index_toggle_bit);
  printf("Index detected:%d ", has_index_been_detected);
  printf("position:%8f ", GetPosition());
  printf("velocity:%8f ", GetVelocity());
  printf("current:%8f ", GetCurrent());
  printf("\n");
}

void Motor::Enable()
{
  enable=true;
}

void Motor::Disable()
{
  enable=false;
}

void Motor::SetPositionReference(float ref)
{
  position_ref = direction * ref + position_offset;
}

void Motor::SetVelocityReference(float ref)
{
  velocity_ref = direction * ref;
}

void Motor::SetCurrentReference(float ref)
{
  current_ref = direction * ref;
}

bool Motor::IsReady()
{
  return is_ready;
}
bool Motor::IsEnabled()
{
  return is_enabled;
}
bool Motor::HasIndexBeenDetected()
{
  return has_index_been_detected;
}
bool Motor::GetIndexToggleBit()
{
  return index_toggle_bit;
}

float Motor::GetPosition()
{
  return direction * (position - position_offset);
}
float Motor::GetVelocity()
{
  return direction * velocity;
}
float Motor::GetCurrent()
{
  return direction * current;
}

float Motor::GetCurrentReference()
{
  return direction * current_ref;
}

void Motor::SetDirection(bool reverse_motor_direction)
{
  direction = reverse_motor_direction ? -1 : 1;
}

void Motor::ZeroPosition()
{
  position_offset = position;
}