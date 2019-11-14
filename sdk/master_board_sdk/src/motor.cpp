#include "motor.h"
#include <stdio.h>

Motor::Motor()
{
  position = 0;
  velocity = 0;
  current = 0;
  is_enabled = false;
  is_ready = false;
  index_toggle_bit = false;
  has_index_been_detected = false;

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
  printf("position:%8f ", position);
  printf("velocity:%8f ", velocity);
  printf("current:%8f ", current);
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
  position_ref = ref;
}

void Motor::SetVelocityReference(float ref)
{
  velocity_ref = ref;
}

void Motor::SetCurrentReference(float ref)
{
  current_ref = ref;
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
    return position;
  }
  float Motor::GetVelocity()
  {
    return velocity;
  }
  float Motor::GetCurrent()
  {
    return current;
  }