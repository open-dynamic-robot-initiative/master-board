#ifndef MOTOR_H
#define MOTOR_H
#include "master_board_sdk/motor_driver.h"
class MotorDriver;
class Motor
{
public:
  Motor();
  void SetCurrentReference(float);
  void SetVelocityReference(float);
  void SetPositionReference(float);
  void SetKp(float);
  void SetKd(float);
  void SetSaturationCurrent(float);
  void SetDriver(MotorDriver *driver);
  void Print();
  void Enable();
  void Disable();

  bool IsReady();
  bool IsEnabled();
  bool HasIndexBeenDetected();
  bool GetIndexToggleBit();
  float GetPosition();
  float GetVelocity();
  float GetCurrent();
  float GetCurrentReference();

  void SetDirection(bool reverse_motor_direction);
  void ZeroPosition();

  //protecteded:

  //state
  float position; // [rad]
  float velocity; // [rad/s]
  float current;  // [A]

  bool is_enabled;
  bool is_ready;
  bool index_toggle_bit;
  bool has_index_been_detected;

  float direction;
  float position_offset; // In absolute position coordinates.

  //commands
  float position_ref;  // [rad]
  float velocity_ref;  // [rad/s]
  float current_ref;   // [A]
  float kp;            // [A/rad]
  float kd;            // [As/rad]


  bool enable;
  bool enable_position_rollover_error;
  bool enable_index_toggle_bit;
  bool enable_index_offset_compensation;

  MotorDriver *driver;
};

#endif