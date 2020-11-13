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
  void SetPositionOffset(float);
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
  float GetPositionOffset();

  //state
  float position;       // [rad]
  float velocity;       // [rad/s]
  float current;        // [A]
  float position_offset;// [rad]

  bool is_enabled;
  bool is_ready;
  bool index_toggle_bit;
  bool has_index_been_detected;

  //commands
  float position_ref;  // [rad]
  float velocity_ref;  // [rad/s]
  float current_ref;   // [A]
  float current_sat;   // [A]
  float kp;            // [A/rad]
  float kd;            // [As/rad]


  bool enable;
  bool enable_position_rollover_error;
  bool enable_index_toggle_bit;
  bool enable_index_offset_compensation;

  MotorDriver *driver;

  // Set functions for Python binding with Boost
  void set_position(float val) { this->position = val; };
  void set_velocity(float val) { this->velocity = val; };
  void set_current(float val) { this->current = val; };
  void set_is_enabled(bool val) { this->is_enabled = val; };
  void set_is_ready(bool val) { this->is_ready = val; };
  void set_has_index_been_detected(bool val) { this->has_index_been_detected = val; };
  void set_index_toggle_bit(bool val) { this->index_toggle_bit = val; };
  void set_position_ref(float val) { this->position_ref = val; };
  void set_velocity_ref(float val) { this->velocity_ref = val; };
  void set_current_ref(float val) { this->current_ref = val; };
  void set_current_sat(float val) { this->current_sat = val; };
  void set_kp(float val) { this->kp = val; };
  void set_kd(float val) { this->kd = val; };
  void set_enable(bool val) { this->enable = val; };
  void set_enable_position_rollover_error(bool val) { this->enable_position_rollover_error = val; };
  void set_enable_index_toggle_bit(bool val) { this->enable_index_toggle_bit = val; };
  void set_enable_index_offset_compensation(bool val) { this->enable_index_offset_compensation = val; };
  void set_driver(MotorDriver* new_driver) { this->driver = new_driver; }; // driver is a pointer but in Python we will use the object itself

  // Get functions for Python binding with Boost
  float get_position() { return this->position; };
  float get_velocity() { return this->velocity; };
  float get_current() { return this->current; };
  bool get_is_enabled() { return this->is_enabled; };
  bool get_is_ready() { return this->is_ready; };
  bool get_has_index_been_detected() { return this->has_index_been_detected; };
  bool get_index_toggle_bit() { return this->index_toggle_bit; };
  float get_position_ref() { return this->position_ref; };
  float get_velocity_ref() { return this->velocity_ref; };
  float get_current_ref() { return this->current_ref; };
  float get_current_sat() { return this->current_sat; };
  float get_kp() { return this->kp; };
  float get_kd() { return this->kd; };
  bool get_enable() { return this->enable; };
  bool get_enable_position_rollover_error() { return this->enable_position_rollover_error; };
  bool get_enable_index_toggle_bit() { return this->enable_index_toggle_bit; };
  bool get_enable_index_offset_compensation() { return this->enable_index_offset_compensation; };
  MotorDriver* get_driver() { return (this->driver); }; // driver is a pointer but in Python we will use the object itself
  
};

#endif